#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <sys/stat.h>

#include "ZipFile.h"
#include "WeaponNames.h"
#include "Enums.h"
#include <yaml-cpp/yaml.h>

// IPA internal path prefix
static const char* IPA_PREFIX = "Payload/Doom2rpg.app/";
// Binary assets are under Packages/
static const char* PKG_PREFIX = "Payload/Doom2rpg.app/Packages/";

// ========================================================================
// Utility
// ========================================================================

static void printUsage(const char* progName) {
	printf("Usage: %s --ipa <path> [--output <dir>]\n", progName);
	printf("\n");
	printf("  --ipa <path>     Path to Doom 2 RPG.ipa file\n");
	printf("  --output <dir>   Output directory (default: games/doom2rpg)\n");
}

static bool dirExists(const char* path) {
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool mkdirRecursive(const std::string& path) {
	size_t pos = 0;
	while ((pos = path.find('/', pos + 1)) != std::string::npos) {
		std::string sub = path.substr(0, pos);
		if (!dirExists(sub.c_str())) {
			if (mkdir(sub.c_str(), 0755) != 0) {
				return false;
			}
		}
	}
	if (!dirExists(path.c_str())) {
		return mkdir(path.c_str(), 0755) == 0;
	}
	return true;
}

static bool writeFile(const std::string& path, const uint8_t* data, int size) {
	FILE* f = fopen(path.c_str(), "wb");
	if (!f)
		return false;
	fwrite(data, 1, size, f);
	fclose(f);
	return true;
}

static bool writeString(const std::string& path, const std::string& content) {
	FILE* f = fopen(path.c_str(), "w");
	if (!f)
		return false;
	fwrite(content.data(), 1, content.size(), f);
	fclose(f);
	return true;
}

// Little-endian readers from a byte buffer
static int16_t readShort(const uint8_t* data, int offset) {
	return (int16_t)(data[offset] | (data[offset + 1] << 8));
}

static uint16_t readUShort(const uint8_t* data, int offset) {
	return (uint16_t)(data[offset] | (data[offset + 1] << 8));
}

static int32_t readInt(const uint8_t* data, int offset) {
	return (int32_t)(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
}

static uint32_t readUInt(const uint8_t* data, int offset) {
	return (uint32_t)(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
}

// ========================================================================
// Entity type names (from Enums.h)
// ========================================================================
static const char* ENTITY_TYPE_NAMES[] = {"world",
                                          "player",
                                          "monster",
                                          "npc",
                                          "playerclip",
                                          "door",
                                          "item",
                                          "decor",
                                          "env_damage",
                                          "corpse",
                                          "attack_interactive",
                                          "monsterblock_item",
                                          "spritewall",
                                          "nonobstructing_spritewall",
                                          "decor_noclip"};
static const int NUM_ENTITY_TYPES = 15;

static const char* entityTypeName(int etype) {
	if (etype >= 0 && etype < NUM_ENTITY_TYPES)
		return ENTITY_TYPE_NAMES[etype];
	return "unknown";
}

// ========================================================================
// Weapon/monster names for tables
// ========================================================================

static const char* PROJ_TYPE_NAMES[] = {"bullet", "melee",       "water",  "plasma", "rocket",   "bfg",       "flesh",
                                        "fire",   "caco_plasma", "thorns", "acid",   "electric", "soul_cube", "item"};

static const char* MONSTER_NAMES[] = {
    "zombie",   "zombie_commando", "lost_soul", "imp",       "sawcubus",   "pinky",      "cacodemon",
    "sentinel", "mancubus",        "revenant",  "arch_vile", "sentry_bot", "cyberdemon", "mastermind",
    "phantom",  "boss_vios",       "boss_vios2",
};
static const int NUM_MONSTER_NAMES = 17;

static std::string weaponName(int idx) {
	return WeaponNames::fromIndex(idx);
}

static std::string projTypeName(int val) {
	if (val == -1)
		return "none";
	if (val >= 0 && val < 14)
		return PROJ_TYPE_NAMES[val];
	return std::to_string(val);
}

static const char* RENDER_MODE_NAMES[] = {
    "normal", "blend25", "blend50", "add", "add75", "add50", "add25", "sub",
    "unk", "perf", "none", nullptr, "blend75", "blend_special_alpha"
};
static const int NUM_RENDER_MODES = 14;

static std::string renderModeName(int val) {
	if (val >= 0 && val < NUM_RENDER_MODES && RENDER_MODE_NAMES[val] != nullptr)
		return RENDER_MODE_NAMES[val];
	return std::to_string(val);
}

// Reverse map: tile index -> name for YAML output
static std::map<int, std::string> tileIndexToName = {
	{225, "missile_player_rocket"}, {226, "missile_rocket"}, {227, "flesh"},
	{232, "shadow"}, {234, "anim_fire"}, {235, "anim_water"},
	{236, "air_vent"}, {239, "soul_cube_attack"}, {240, "water_stream"},
	{241, "caco_plasma"}, {242, "fire_ball"}, {243, "plasma_ball"},
	{244, "bfg_ball"}, {245, "monster_claw"}, {246, "monster_bite"},
	{247, "monster_blunt_trauma"}, {248, "electric_slide"},
	{251, "fear_eye"}, {252, "acid_spit"},
	{170, "sentinel_spikes"}, {171, "sentinel_spikes_dummy"},
};

static std::string tileName(int idx) {
	if (idx == 0) return "0";
	auto it = tileIndexToName.find(idx);
	if (it != tileIndexToName.end()) return it->second;
	return std::to_string(idx);
}

static const char* AMMO_TYPE_NAMES[] = {"none", "bullets", "shells", "holy_water", "cells", "rockets", "soul_cube", "sentry_bot", "item"};
static const int NUM_AMMO_TYPES = 9;

static std::string ammoTypeName(int val) {
	if (val >= 0 && val < NUM_AMMO_TYPES)
		return AMMO_TYPE_NAMES[val];
	return std::to_string(val);
}

static std::string monsterName(int idx) {
	if (idx >= 0 && idx < NUM_MONSTER_NAMES)
		return MONSTER_NAMES[idx];
	return "monster_" + std::to_string(idx);
}

// ========================================================================
// Entity subtype/parm name helpers for named entity output
// ========================================================================
static const char* ITEM_SUBTYPE_NAMES[] = {"inventory", "weapon", "ammo", "food", "sack", "c_note", "c_string"};
static const int NUM_ITEM_SUBTYPES = 7;

static const char* DOOR_SUBTYPE_NAMES[] = {"", "locked", "unlocked"};
static const int NUM_DOOR_SUBTYPES = 3;

static const char* DECOR_SUBTYPE_NAMES[] = {"misc", "exithall", "mixing", "statue", "", "tombstone", "dynamite", "water_spout", "treadmill"};
static const int NUM_DECOR_SUBTYPES = 9;

// Player weapon names (first 15 of WeaponNames::TABLE)
static const int NUM_PLAYER_WEAPONS = 15;

// Weapon HUD data (tex_row, show_ammo)
struct WeaponHudData { int texRow; bool showAmmo; };
static const WeaponHudData WEAPON_HUD[] = {
	{0, true},   // 0: assault_rifle
	{1, false},  // 1: chainsaw
	{2, true},   // 2: holy_water_pistol
	{10, true},  // 3: shooting_sentry_bot
	{10, true},  // 4: exploding_sentry_bot
	{11, true},  // 5: red_shooting_sentry_bot
	{11, true},  // 6: red_exploding_sentry_bot
	{3, true},   // 7: super_shotgun
	{4, true},   // 8: chaingun
	{5, true},   // 9: assault_rifle_with_scope
	{6, true},   // 10: plasma_gun
	{7, true},   // 11: rocket_launcher
	{8, true},   // 12: bfg
	{9, true},   // 13: soul_cube
	{12, true},  // 14: item
};

// Weapon behavior flags per weapon index
struct WeaponBehaviorData {
	bool vibrateAnim;
	bool chainsawHitEvent;
	bool isThrowableItem;
	bool showFlashFrame;
	bool hasFlashSprite;
	bool soulAmmoDisplay;
	bool isMelee;
	bool noRecoil;
	bool splashDamage;
	int splashDivisor;
	bool noBloodOnHit;
	bool drawDoubleSprite;
	bool consumeOnUse;
	bool chargeAttack;
	bool flipSpriteOnEnd;
};
static const WeaponBehaviorData WEAPON_BEHAVIOR[] = {
	// 0: assault_rifle
	{false, false, false, false, true,  false, false, false, false, 0, false, false, false, false, false},
	// 1: chainsaw
	{true,  true,  false, false, false, false, true,  true,  false, 0, false, false, false, false, false},
	// 2: holy_water_pistol
	{false, false, false, false, false, false, false, false, false, 0, true,  false, false, false, false},
	// 3: shooting_sentry_bot
	{false, false, false, false, false, false, false, false, false, 0, false, false, false, false, false},
	// 4: exploding_sentry_bot
	{false, false, false, false, false, false, false, true,  true,  2, false, false, true,  false, false},
	// 5: red_shooting_sentry_bot
	{false, false, false, false, false, false, false, false, false, 0, false, false, false, false, false},
	// 6: red_exploding_sentry_bot
	{false, false, false, false, false, false, false, true,  true,  2, false, false, true,  false, false},
	// 7: super_shotgun
	{false, false, false, false, true,  false, false, false, false, 0, false, false, false, false, false},
	// 8: chaingun
	{false, false, false, true,  true,  false, false, false, false, 0, false, true,  false, false, false},
	// 9: assault_rifle_with_scope
	{false, false, false, false, false, false, false, false, false, 0, false, false, false, false, false},
	// 10: plasma_gun
	{false, false, false, false, false, false, false, false, false, 0, false, false, false, false, false},
	// 11: rocket_launcher
	{false, false, false, false, false, false, false, false, true,  2, false, false, false, false, false},
	// 12: bfg
	{false, false, false, false, false, false, false, false, true,  4, false, false, false, false, false},
	// 13: soul_cube
	{false, false, false, true,  false, true,  false, false, false, 0, false, false, false, true,  true},
	// 14: item
	{false, false, true,  false, false, false, false, false, false, 0, false, false, true,  false, false},
};

// Ammo parm names (ammo subtype parm values)
static const char* AMMO_PARM_NAMES[] = {"none", "bullets", "shells", "holy_water", "cells", "rockets", "soul_cube"};
static const int NUM_AMMO_PARMS = 7;

static std::string entitySubtypeName(int eType, int eSubType) {
	switch (eType) {
	case 2: // monster
	case 9: // corpse (subtypes are monster indices)
		if (eSubType >= 0 && eSubType < NUM_MONSTER_NAMES)
			return MONSTER_NAMES[eSubType];
		break;
	case 5: // door
		if (eSubType >= 0 && eSubType < NUM_DOOR_SUBTYPES && DOOR_SUBTYPE_NAMES[eSubType][0] != '\0')
			return DOOR_SUBTYPE_NAMES[eSubType];
		break;
	case 6: // item
		if (eSubType >= 0 && eSubType < NUM_ITEM_SUBTYPES)
			return ITEM_SUBTYPE_NAMES[eSubType];
		break;
	case 7:  // decor
	case 14: // decor_noclip
		if (eSubType >= 0 && eSubType < NUM_DECOR_SUBTYPES && DECOR_SUBTYPE_NAMES[eSubType][0] != '\0')
			return DECOR_SUBTYPE_NAMES[eSubType];
		break;
	}
	return std::to_string(eSubType);
}

static std::string entityParmName(int eType, int eSubType, int parm) {
	if (eType == Enums::ET_ITEM) { // item
		if (eSubType == Enums::ITEM_WEAPON && parm >= 0 && parm < NUM_PLAYER_WEAPONS)
			return WeaponNames::TABLE[parm];
		if (eSubType == Enums::ITEM_AMMO && parm >= 0 && parm < NUM_AMMO_PARMS)
			return AMMO_PARM_NAMES[parm];
	}
	return std::to_string(parm);
}

static std::string generateEntityName(int eType, int eSubType, int parm, const char* typeName, std::map<std::string, int>& nameCount) {
	std::string base;
	switch (eType) {
	case Enums::ET_MONSTER: // monster
		base = entitySubtypeName(eType, eSubType);
		break;
	case Enums::ET_CORPSE: // corpse (subtypes are monster indices, prefix with type name)
		base = std::string(typeName) + "_" + entitySubtypeName(eType, eSubType);
		break;
	case Enums::ET_ITEM: { // item
		std::string sub = entitySubtypeName(eType, eSubType);
		std::string p = entityParmName(eType, eSubType, parm);
		if (eSubType == Enums::ITEM_WEAPON || eSubType == Enums::ITEM_AMMO)
			base = p;
		else
			base = sub + "_" + p;
		break;
	}
	case Enums::ET_DOOR: { // door
		std::string sub = entitySubtypeName(eType, eSubType);
		base = std::string(typeName) + "_" + sub;
		break;
	}
	default:
		base = std::string(typeName) + "_" + std::to_string(eSubType);
		break;
	}

	// Add parm suffix for monsters and corpses
	if (eType == Enums::ET_MONSTER || eType == Enums::ET_CORPSE)
		base += "_parm" + std::to_string(parm);

	// Deduplicate
	int& cnt = nameCount[base];
	cnt++;
	if (cnt > 1)
		base += "_" + std::to_string(cnt);

	return base;
}

// ========================================================================
// Menu constants
// ========================================================================
static const char* MENU_TYPE_NAMES[] = {"default", "list",    "confirm",  "confirm2",  "main",
                                        "help",    "vcenter", "notebook", "main_list", "vending_machine"};

struct ActionEntry {
	int id;
	const char* name;
};
static const ActionEntry ACTION_TABLE[] = {
    {0, "none"},
    {1, "goto"},
    {2, "back"},
    {3, "load"},
    {4, "save"},
    {5, "backtomain"},
    {6, "togsound"},
    {7, "newgame"},
    {8, "exit"},
    {9, "changestate"},
    {10, "difficulty"},
    {11, "returntogame"},
    {12, "restartlevel"},
    {13, "savequit"},
    {14, "offersuccess"},
    {15, "changesfxvolume"},
    {16, "showdetails"},
    {17, "changemap"},
    {18, "useitemweapon"},
    {19, "select_language"},
    {20, "useitemsyring"},
    {21, "useitemother"},
    {22, "continue"},
    {23, "main_special"},
    {24, "confirmuse"},
    {25, "saveexit"},
    {26, "backtwo"},
    {27, "minigame"},
    {28, "confirmbuy"},
    {29, "buydrink"},
    {30, "buysnack"},
    {33, "return_to_player"},
    {35, "flip_controls"},
    {36, "control_layout"},
    {37, "changemusicvolume"},
    {38, "changealpha"},
    {39, "change_vid_mode"},
    {40, "tog_vsync"},
    {41, "change_resolution"},
    {42, "apply_changes"},
    {43, "set_binding"},
    {44, "default_bindings"},
    {45, "tog_vibration"},
    {46, "change_vibration_intensity"},
    {47, "change_deadzone"},
    {48, "tog_tinygl"},
    {100, "debug"},
    {102, "giveall"},
    {103, "givemap"},
    {104, "noclip"},
    {105, "disableai"},
    {106, "nohelp"},
    {107, "godmode"},
    {108, "showlocation"},
    {109, "rframes"},
    {110, "rspeeds"},
    {111, "rskipflats"},
    {112, "rskipcull"},
    {114, "rskipbsp"},
    {115, "rskiplines"},
    {116, "rskipsprites"},
    {117, "ronlyrender"},
    {118, "rskipdecals"},
    {119, "rskip2dstretch"},
    {120, "driving_mode"},
    {121, "render_mode"},
    {122, "equipformap"},
    {123, "oneshot"},
    {124, "debug_font"},
    {125, "sys_test"},
    {126, "skip_minigames"},
    {127, "show_heap"},
};

static std::string actionName(int id) {
	for (const auto& e : ACTION_TABLE) {
		if (e.id == id)
			return e.name;
	}
	return std::to_string(id);
}

struct FlagEntry {
	int bit;
	const char* name;
};
static const FlagEntry FLAG_TABLE[] = {
    {0x0001, "noselect"},    {0x0002, "nodehyphenate"}, {0x0004, "disabled"},    {0x0008, "align_center"},
    {0x0020, "showdetails"}, {0x0040, "divider"},       {0x0080, "selector"},    {0x0100, "block_text"},
    {0x0200, "highlight"},   {0x0400, "checked"},       {0x2000, "right_arrow"}, {0x4000, "left_arrow"},
    {0x8000, "hidden"},
    {0x20000, "scrollbar"},    {0x40000, "scrollbartwo"}, {0x80000, "disabledtwo"},
    {0x100000, "padding"},     {0x200000, "binding"},
};

static std::string flagsToNames(int flags) {
	if (flags == 0)
		return "normal";
	std::string result;
	for (const auto& e : FLAG_TABLE) {
		if (flags & e.bit) {
			if (!result.empty())
				result += ",";
			result += e.name;
		}
	}
	return result.empty() ? "normal" : result;
}

// ========================================================================
// String constants
// ========================================================================
static const int MAXTEXT = 15;
static const int MAX_LANGUAGES = 5;

static const char* LANGUAGE_NAMES[] = {"english", "french", "german", "italian", "spanish"};
static const char* GROUP_NAMES[] = {"CodeStrings", "EntityStrings", "FileStrings", "MenuStrings", "M_01",
                                    "M_02",        "M_03",          "M_04",        "M_05",        "M_06",
                                    "M_07",        "M_08",          "M_09",        "M_TEST",      "Translations"};

// Shared data between conversion steps — populated by convertTables/convertStrings,
// consumed by level config generation to derive level names from string tables.
static std::vector<int16_t> g_levelNameIds;       // from tables.bin table 7
static std::vector<std::string> g_menuStrings;     // English MenuStrings (group 3)

// Derive a snake_case level name from the string tables (e.g. "tycho", "uac_labs")
static std::string levelNameFromId(int mapId) {
	int idx = mapId - 1;
	if (idx >= 0 && idx < (int)g_levelNameIds.size()) {
		int strId = g_levelNameIds[idx];
		if (strId >= 0 && strId < (int)g_menuStrings.size()) {
			std::string name = g_menuStrings[strId];
			// Strip soft hyphens (used for word-break hints in original strings)
			std::string clean;
			for (char c : name) {
				if (c != '-') clean += c;
			}
			// Convert to snake_case: lowercase, spaces/& to underscores
			std::string result;
			for (char c : clean) {
				if (c == ' ' || c == '&') result += '_';
				else result += (char)std::tolower((unsigned char)c);
			}
			// Collapse consecutive underscores
			std::string final_name;
			for (char c : result) {
				if (c == '_' && !final_name.empty() && final_name.back() == '_') continue;
				final_name += c;
			}
			return final_name;
		}
	}
	return "map" + std::to_string(mapId);
}

// Get numbered level directory name, e.g. "01_tycho", "10_test_map"
static std::string getLevelDirName(int mapId) {
	char buf[64];
	snprintf(buf, sizeof(buf), "%02d_%s", mapId, levelNameFromId(mapId).c_str());
	return buf;
}

// Create all per-level directories under outDir/levels/
static bool createLevelDirectories(const std::string& outDir) {
	for (int mapId = 1; mapId <= 10; mapId++) {
		std::string dir = outDir + "/levels/" + getLevelDirName(mapId);
		if (!mkdirRecursive(dir)) {
			fprintf(stderr, "  Failed to create level directory: %s\n", dir.c_str());
			return false;
		}
	}
	return true;
}

// Escape special bytes for YAML string storage
static std::string escapeString(const uint8_t* raw, int len) {
	std::string result;
	for (int i = 0; i < len; i++) {
		uint8_t ch = raw[i];
		if (ch == '"') {
			result += "\\\"";
		} else if (ch == '\\') {
			result += "\\\\";
		} else if (ch == 0x0A) {
			result += "\\n";
		} else if (ch == 0x0D) {
			result += "\\r";
		} else if (ch == 0x09) {
			result += "\\t";
		} else if (ch >= 0x80 && ch <= 0x9F) {
			char buf[8];
			snprintf(buf, sizeof(buf), "\\x%02X", ch);
			result += buf;
		} else if (ch >= 0xA0) {
			// Non-ASCII bytes: escape as hex to keep YAML valid UTF-8
			char buf[8];
			snprintf(buf, sizeof(buf), "\\x%02X", ch);
			result += buf;
		} else {
			result += (char)ch;
		}
	}
	return result;
}

// Sprite tile name/index mapping (used by sprites.yaml and entities.yaml generators)
struct TileEntry { const char* name; int index; };
static const TileEntry spriteEntries[] = {
	// View weapons
	{"assault_rifle", 1}, {"chainsaw", 2}, {"holy_water_pistol", 3},
	{"sentry_bot_shooting", 4}, {"sentry_bot_exploding", 5},
	{"super_shotgun", 6}, {"chaingun", 7}, {"assault_rifle_with_scope", 8},
	{"plasma_gun", 9}, {"rocket_launcher", 10}, {"bfg", 11}, {"soul_cube", 12},
	{"sentry_bot_red_shooting", 13}, {"sentry_bot_red_exploding", 14},
	{"world_weapon", 15},
	// Monsters
	{"monster_red_sentry_bot", 18}, {"monster_sentry_bot", 19},
	{"monster_zombie", 20}, {"monster_zombie2", 21}, {"monster_zombie3", 22},
	{"monster_imp", 23}, {"monster_imp2", 24}, {"monster_imp3", 25},
	{"monster_saw_goblin", 26}, {"monster_saw_goblin2", 27}, {"monster_saw_goblin3", 28},
	{"monster_lost_soul", 29}, {"monster_lost_soul2", 30}, {"monster_lost_soul3", 31},
	{"monster_pinky", 32}, {"monster_pinky2", 33}, {"monster_pinky3", 34},
	{"monster_revenant", 35}, {"monster_revenant2", 36}, {"monster_revenant3", 37},
	{"monster_mancubus", 38}, {"monster_mancubus2", 39}, {"monster_mancubus3", 40},
	{"monster_cacodemon", 41}, {"monster_cacodemon2", 42}, {"monster_cacodemon3", 43},
	{"monster_sentinel", 44}, {"monster_sentinel2", 45}, {"monster_sentinel3", 46},
	{"monster_arch_vile", 50}, {"monster_arch_vile2", 51}, {"monster_arch_vile3", 52},
	{"monster_arachnotron", 53},
	{"boss_cyberdemon", 54}, {"boss_pinky", 56}, {"boss_mastermind", 57},
	{"boss_vios", 58}, {"boss_vios2", 59}, {"boss_vios3", 60},
	{"boss_vios4", 61}, {"boss_vios5", 62},
	// NPCs
	{"npc_riley_oconnor", 66}, {"npc_major", 68}, {"npc_bob", 69},
	{"npc_civilian", 71}, {"npc_sarge", 72}, {"npc_female", 73},
	{"npc_evil_scientist", 74}, {"npc_scientist", 75}, {"npc_researcher", 76},
	{"npc_civilian2", 77},
	// Items
	{"ammo_bullets", 85}, {"ammo_shells", 86}, {"ammo_rockets", 88},
	{"ammo_cells", 89}, {"ammo_holy_water", 90}, {"one_uac_credit", 107},
	{"key_red", 110}, {"key_blue", 111}, {"armor_jacket", 112},
	{"satchel", 113}, {"pack_item", 114}, {"worker_pack", 115},
	{"health_pack", 116}, {"food_plate", 117}, {"armor_shard", 119},
	// World objects
	{"obj_table", 121}, {"hell_seal", 122}, {"toilet", 123},
	{"dirt_decal", 124}, {"ladder", 125}, {"blood_splatter", 126},
	{"sink", 127}, {"barred_window", 128}, {"hazard_bar", 129},
	{"obj_fire", 130}, {"treadmill_monitor", 131}, {"tech_station", 133},
	{"water_spout", 134}, {"obj_chair", 135}, {"obj_torchiere", 136},
	{"obj_scientist_corpse", 137}, {"obj_corpse", 138}, {"obj_other_corpse", 139},
	{"dummy_pain", 140}, {"attack_dummy", 141}, {"exit_dummy", 142},
	{"use_dummy", 143}, {"septic_station", 147}, {"practice_target", 149},
	{"obj_printer", 150}, {"obj_crate", 152}, {"vending_machine", 153},
	{"armor_repair", 154}, {"closed_portal_eye", 155}, {"eye_portal", 156},
	{"portal_socket", 157}, {"treadmill_side", 158}, {"treadmill_front", 159},
	{"hell_hands", 161}, {"doorjamb_decal", 162}, {"stones_and_skulls", 164},
	{"nonobstructing_spritewall", 166}, {"fence", 168},
	{"sentinel_spikes", 170}, {"sentinel_spikes_dummy", 171},
	{"switch", 173}, {"tech_detail", 175}, {"hell_skulls", 177},
	{"glass", 178}, {"terminal_target", 179}, {"terminal_general", 180},
	{"terminal_vios", 181}, {"terminal_bot", 182}, {"terminal_hacking", 183},
	{"elevator_nums", 184}, {"bush", 187}, {"tree_top", 188},
	{"tree_trunk", 189}, {"sfx_lightglow1", 193}, {"window3", 197},
	{"glaevenscope", 201}, {"fog_gray", 208}, {"scorch_mark", 212},
	{"static_flame", 223},
	// Projectiles and effects
	{"missile_player_rocket", 225}, {"missile_rocket", 226}, {"flesh", 227},
	{"shadow", 232}, {"anim_fire", 234}, {"anim_water", 235},
	{"air_vent", 236}, {"soul_cube_attack", 239}, {"water_stream", 240},
	{"caco_plasma", 241}, {"fire_ball", 242}, {"plasma_ball", 243},
	{"bfg_ball", 244}, {"monster_claw", 245}, {"monster_bite", 246},
	{"monster_blunt_trauma", 247}, {"electric_slide", 248},
	{"fear_eye", 251}, {"acid_spit", 252}, {"npc_chat", 254}, {"alert", 255},
	// Walls and doors
	{"doorjamb", 257}, {"red_door_locked", 271}, {"red_door_unlocked", 272},
	{"blue_door_locked", 273}, {"blue_door_unlocked", 274},
	{"door_locked", 275}, {"door_unlocked", 276},
	{"level_door_locked", 277}, {"level_door_unlocked", 278},
	{"sky_box", 301}, {"fade", 302},
	// Flats
	{"flat_lava", 479}, {"flat_lava2", 480},
};

// ========================================================================
// Sound filenames (144 default sounds from the original IPA)
// ========================================================================
static const char* DEFAULT_SOUND_NAMES[] = {
    "Arachnotron_active.wav",
    "Arachnotron_death.wav",
    "Arachnotron_sight.wav",
    "Arachnotron_walk.wav",
    "Archvile_active.wav",
    "Archvile_attack.wav",
    "Archvile_death.wav",
    "Archvile_pain.wav",
    "Archvile_sight.wav",
    "BFG.wav",
    "block_drop.wav",
    "block_pick.wav",
    "Cacodemon_death.wav",
    "Cacodemon_sight.wav",
    "chaingun.wav",
    "chainsaw.wav",
    "ChainsawGoblin_attack.wav",
    "ChainsawGoblin_death.wav",
    "ChainsawGoblin_pain.wav",
    "ChainsawGoblin_sight.wav",
    "chime.wav",
    "claw.wav",
    "Cyberdemon_death.wav",
    "Cyberdemon_sight.wav",
    "demon_active.wav",
    "demon_pain.wav",
    "demon_small_active.wav",
    "Dialog_Help.wav",
    "door_close.wav",
    "door_close_slow.wav",
    "door_open.wav",
    "door_open_slow.wav",
    "explosion.wav",
    "fireball.wav",
    "fireball_impact.wav",
    "footstep1.wav",
    "footstep2.wav",
    "gib.wav",
    "glass.wav",
    "hack_clear.wav",
    "hack_fail.wav",
    "hack_failed.wav",
    "hack_submit.wav",
    "hack_success.wav",
    "hack_successful.wav",
    "HolyWaterPistol.wav",
    "HolyWaterPistol_refill.wav",
    "hoof.wav",
    "impClaw.wav",
    "Imp_active.wav",
    "Imp_death1.wav",
    "Imp_death2.wav",
    "Imp_sight1.wav",
    "Imp_sight2.wav",
    "item_pickup.wav",
    "loot.wav",
    "LostSoul_attack.wav",
    "Maglev_arrive.wav",
    "Maglev_depart.wav",
    "malfunction.wav",
    "Mancubus_attack.wav",
    "Mancubus_death.wav",
    "Mancubus_pain.wav",
    "Mancubus_sight.wav",
    "menu_open.wav",
    "menu_scroll.wav",
    "monster_pain.wav",
    "Music_Boss.wav",
    "Music_General.wav",
    "Music_Level_End.wav",
    "Music_LevelUp.wav",
    "Music_Title.wav",
    "no_use.wav",
    "no_use2.wav",
    "Pinkinator_attack.wav",
    "Pinkinator_death.wav",
    "Pinkinator_pain.wav",
    "Pinkinator_sight.wav",
    "Pinkinator_spawn.wav",
    "Pinky_attack.wav",
    "Pinky_death.wav",
    "Pinky_sight.wav",
    "Pinky_small_attack.wav",
    "Pinky_small_death.wav",
    "Pinky_small_pain.wav",
    "Pinky_small_sight.wav",
    "pistol.wav",
    "plasma.wav",
    "platform_start.wav",
    "platform_stop.wav",
    "player_death1.wav",
    "player_death2.wav",
    "player_death3.wav",
    "player_pain1.wav",
    "player_pain2.wav",
    "Revenant_active.wav",
    "Revenant_attack.wav",
    "Revenant_death.wav",
    "Revenant_punch.wav",
    "Revenant_sight.wav",
    "Revenant_swing.wav",
    "rocketLauncher.wav",
    "rocket_explode.wav",
    "secret.wav",
    "Sentinel_attack.wav",
    "Sentinel_death.wav",
    "Sentinel_pain.wav",
    "Sentinel_sight.wav",
    "SentryBot_activate.wav",
    "SentryBot_death.wav",
    "SentryBot_discard.wav",
    "SentryBot_pain.wav",
    "SentryBot_return.wav",
    "slider.wav",
    "Soulcube.wav",
    "SpiderMastermind_death.wav",
    "SpiderMastermind_sight.wav",
    "supershotgun.wav",
    "supershotgun_close.wav",
    "supershotgun_load.wav",
    "supershotgun_open.wav",
    "switch.wav",
    "switch_exit.wav",
    "teleport.wav",
    "use_item.wav",
    "vending_sale.wav",
    "vent.wav",
    "VIOS_attack1.wav",
    "VIOS_attack2.wav",
    "VIOS_death.wav",
    "VIOS_pain.wav",
    "VIOS_sight.wav",
    "weapon_pickup.wav",
    "Weapon_Sniper_Scope.wav",
    "Weapon_Toilet_Pull.wav",
    "Weapon_Toilet_Smash.wav",
    "Weapon_Toilet_Throw.wav",
    "Weapon_Toilet_Throw2.wav",
    "zombie_active.wav",
    "zombie_death1.wav",
    "zombie_death2.wav",
    "zombie_death3.wav",
    "zombie_sight1.wav",
    "zombie_sight2.wav",
    "zombie_sight3.wav",
};
static const int NUM_DEFAULT_SOUNDS = 144;

// ========================================================================
// Conversion: entities.bin -> entities.yaml
// ========================================================================
// ========================================================================
// Entity rendering data helpers (mirrors computeDefault*() in EntityDef.cpp)
// ========================================================================

// Tile index constants (from Enums.h)
static const int TILE_SENTRY_BOT       = 19;
static const int TILE_RED_SENTRY_BOT   = 18;
static const int TILE_ZOMBIE           = 20;
static const int TILE_IMP              = 23;
static const int TILE_SAWCUBUS         = 26;
static const int TILE_LOST_SOUL        = 29;
static const int TILE_PINKY            = 32;
static const int TILE_REVENANT         = 35;
static const int TILE_MANCUBUS         = 38;
static const int TILE_CACODEMON        = 41;
static const int TILE_SENTINEL         = 44;
static const int TILE_ARCH_VILE        = 50;
static const int TILE_ARACHNOTRON      = 53;
static const int TILE_BOSS_CYBERDEMON  = 54;
static const int TILE_BOSS_PINKY       = 56;
static const int TILE_BOSS_MASTERMIND  = 57;
static const int TILE_BOSS_VIOS        = 58;
static const int TILE_BOSS_VIOS5       = 62;

// Monster subtype IDs (from Enums.h)
static const int M_ZOMBIE     = 0;
static const int M_LOST_SOUL  = 2;
static const int M_IMP        = 3;
static const int M_SAWCUBUS   = 4;
static const int M_PINKY      = 5;
static const int M_CACODEMON  = 6;
static const int M_SENTINEL   = 7;
static const int M_MANCUBUS   = 8;
static const int M_REVENANT   = 9;
static const int M_ARCH_VILE  = 10;
static const int M_SENTRY_BOT = 11;
static const int M_CYBERDEMON = 12;

static bool tileInRange(int tile, int base, int variants = 3) {
	return tile >= base && tile < base + variants;
}

static void emitRenderFlags(YAML::Emitter& out, int16_t tileIndex) {
	bool isFloater = tileInRange(tileIndex, TILE_SENTINEL) ||
	                 tileInRange(tileIndex, TILE_LOST_SOUL) ||
	                 tileInRange(tileIndex, TILE_CACODEMON);
	bool hasGunFlare = tileInRange(tileIndex, TILE_MANCUBUS) ||
	                   tileInRange(tileIndex, TILE_REVENANT) ||
	                   tileIndex == TILE_SENTRY_BOT || tileIndex == TILE_RED_SENTRY_BOT ||
	                   tileIndex == TILE_BOSS_CYBERDEMON || tileIndex == TILE_BOSS_MASTERMIND;
	bool isSpecialBoss = tileIndex == TILE_BOSS_MASTERMIND || tileIndex == TILE_ARACHNOTRON ||
	                     tileIndex == TILE_BOSS_PINKY ||
	                     (tileIndex >= TILE_BOSS_VIOS && tileIndex <= TILE_BOSS_VIOS5);

	if (isFloater || hasGunFlare || isSpecialBoss) {
		out << YAML::Key << "flags" << YAML::Value << YAML::BeginSeq;
		if (isFloater)     out << "floater";
		if (hasGunFlare)   out << "gun_flare";
		if (isSpecialBoss) out << "special_boss";
		out << YAML::EndSeq;
	}
}

static void emitFearEyes(YAML::Emitter& out, int subtype) {
	struct { int eyeL, eyeR, zAdd, zAlwaysPre, zIdlePre; bool singleEye; int eyeLFlip, eyeRFlip; } d =
		{0, 0, 0, 0, 0, false, 0, 0};
	switch (subtype) {
		case M_ARCH_VILE:  d = {-3, 3, 403, 0, 48, false, 0, 0}; break;
		case M_SAWCUBUS:   d = {-3, 3, 388, 0, 0, false, 0, 0}; break;
		case M_IMP:        d = {-3, 3, 294, 0, 0, false, 0, 0}; break;
		case M_LOST_SOUL:  d = {-4, 4, -192, 192, 0, false, 0, 0}; break;
		case M_MANCUBUS:   d = {-3, 3, 280, 0, 0, false, 0, 0}; break;
		case M_PINKY:      d = {-5, 5, 35, 0, 0, false, 0, 0}; break;
		case M_CACODEMON:  d = {0, -1, 32, 0, 0, true, 0, 0}; break;
		case M_REVENANT:   d = {-2, 5, 324, 160, 32, false, -5, 2}; break;
		case M_SENTINEL:   d = {-1, 4, 274, 0, 0, false, 0, 0}; break;
		default: return; // no data = skip section
	}
	out << YAML::Key << "fear_eyes" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "eye_l" << YAML::Value << d.eyeL;
	out << YAML::Key << "eye_r" << YAML::Value << d.eyeR;
	out << YAML::Key << "z_add" << YAML::Value << d.zAdd;
	if (d.zAlwaysPre != 0) out << YAML::Key << "z_always_pre" << YAML::Value << d.zAlwaysPre;
	if (d.zIdlePre != 0) out << YAML::Key << "z_idle_pre" << YAML::Value << d.zIdlePre;
	if (d.singleEye) out << YAML::Key << "single_eye" << YAML::Value << true;
	if (d.eyeLFlip != 0) out << YAML::Key << "eye_l_flip" << YAML::Value << d.eyeLFlip;
	if (d.eyeRFlip != 0) out << YAML::Key << "eye_r_flip" << YAML::Value << d.eyeRFlip;
	out << YAML::EndMap;
}

static void emitGunFlare(YAML::Emitter& out, int subtype) {
	struct { bool dual; int f1x, f1z, f2x, f2z; } d = {false, 0, 0, 0, 0};
	switch (subtype) {
		case M_MANCUBUS:   d = {true, -24, 160, 22, 128}; break;
		case M_REVENANT:   d = {true, -9, 736, 15, 736}; break;
		case M_SENTRY_BOT: d = {false, 0, 0, 0, -64}; break;
		case M_CYBERDEMON:  d = {false, 0, 0, 14, 352}; break;
		default: return;
	}
	out << YAML::Key << "gun_flare" << YAML::Value << YAML::BeginMap;
	if (d.dual) out << YAML::Key << "dual" << YAML::Value << true;
	if (d.f1x != 0) out << YAML::Key << "flash1_x" << YAML::Value << d.f1x;
	if (d.f1z != 0) out << YAML::Key << "flash1_z" << YAML::Value << d.f1z;
	if (d.f2x != 0) out << YAML::Key << "flash2_x" << YAML::Value << d.f2x;
	if (d.f2z != 0) out << YAML::Key << "flash2_z" << YAML::Value << d.f2z;
	out << YAML::EndMap;
}

static void emitBodyParts(YAML::Emitter& out, int eType, int subtype) {
	struct { int idleTZ, idleHZ, walkTZ, walkHZ, atkTZF0, atkTZF1, atkHZ, atkHX;
	         bool noHBack, sentryFlip, flipTorso, noHAtk, noHMancAtk; int revAtk2TZ; } d =
		{0, 0, 0, 0, 0, 0, 0, 0, false, false, false, false, false, 0};
	bool hasData = false;
	if (eType == Enums::ET_MONSTER) { // monster
		hasData = true;
		switch (subtype) {
			case M_REVENANT:   d = {-30, 140, 0, 160, 0, 0, 20, 2, false, false, false, false, false, 130}; break;
			case M_ARCH_VILE:  d = {-36, 109, -36, 109, 288, 0, 0, 0, false, false, true, true, false, 0}; break;
			case M_SAWCUBUS:   d = {0, 0, 0, 0, 96, -100, 16, 0, false, false, false, false, false, 0}; break;
			case M_MANCUBUS:   d = {0, 0, 0, 0, 0, 0, -112, 7, false, false, false, false, true, 0}; break;
			case M_PINKY:      d = {0, 0, 0, 0, 0, 0, 0, 0, true, false, false, false, false, 0}; break;
			case M_ZOMBIE:     d = {0, 0, 0, 0, 0, 0, 0, 0, false, false, false, true, false, 0}; break;
			case M_IMP:        d = {0, 0, 0, 0, 0, 0, 0, 0, false, false, false, true, false, 0}; break;
			case M_SENTRY_BOT: d = {0, 0, 0, 0, 0, 0, 0, 0, false, true, false, false, false, 0}; break;
			case M_CYBERDEMON:  d = {-30, -15, 0, 0, 0, 0, 0, 0, false, false, false, false, false, 0}; break;
			default: hasData = false; break;
		}
	} else if (eType == Enums::ET_NPC) { // NPC
		hasData = true;
		d.idleTZ = -18;
		d.flipTorso = true;
	}
	if (!hasData) return;

	out << YAML::Key << "body_parts" << YAML::Value << YAML::BeginMap;
	if (d.idleTZ != 0) out << YAML::Key << "idle_torso_z" << YAML::Value << d.idleTZ;
	if (d.idleHZ != 0) out << YAML::Key << "idle_head_z" << YAML::Value << d.idleHZ;
	if (d.walkTZ != 0) out << YAML::Key << "walk_torso_z" << YAML::Value << d.walkTZ;
	if (d.walkHZ != 0) out << YAML::Key << "walk_head_z" << YAML::Value << d.walkHZ;
	if (d.atkTZF0 != 0) out << YAML::Key << "attack_torso_z_f0" << YAML::Value << d.atkTZF0;
	if (d.atkTZF1 != 0) out << YAML::Key << "attack_torso_z_f1" << YAML::Value << d.atkTZF1;
	if (d.atkHZ != 0) out << YAML::Key << "attack_head_z" << YAML::Value << d.atkHZ;
	if (d.atkHX != 0) out << YAML::Key << "attack_head_x" << YAML::Value << d.atkHX;
	if (d.noHBack) out << YAML::Key << "no_head_on_back" << YAML::Value << true;
	if (d.sentryFlip) out << YAML::Key << "sentry_head_flip" << YAML::Value << true;
	if (d.flipTorso) out << YAML::Key << "flip_torso_walk" << YAML::Value << true;
	if (d.noHAtk) out << YAML::Key << "no_head_on_attack" << YAML::Value << true;
	if (d.noHMancAtk) out << YAML::Key << "no_head_on_manc_atk" << YAML::Value << true;
	if (d.revAtk2TZ != 0) out << YAML::Key << "attack_rev_atk2_torso_z" << YAML::Value << d.revAtk2TZ;
	out << YAML::EndMap;
}

static void emitFloater(YAML::Emitter& out, int subtype) {
	bool hasData = false;
	int type = 0, zOffset = 0, backViewFrame = 0, idleFrontFrame = 0, headZOffset = 0, backExtraSpriteIdx = 0;
	bool hasIdleFrameInc = false, hasBackExtraSprite = false, hasDeadLoot = false;
	switch (subtype) {
		case M_SENTINEL:
			hasData = true;
			type = 1; // FLOATER_MULTIPART
			backViewFrame = 6; idleFrontFrame = 2; headZOffset = -11; hasDeadLoot = true;
			break;
		case M_LOST_SOUL:
			hasData = true;
			zOffset = 192; hasIdleFrameInc = true;
			break;
		case M_CACODEMON:
			hasData = true;
			hasBackExtraSprite = true; backExtraSpriteIdx = 7;
			break;
		default: break;
	}
	if (!hasData) return;

	const char* floaterTypes[] = {"default", "multipart"};
	out << YAML::Key << "floater" << YAML::Value << YAML::BeginMap;
	if (type != 0) out << YAML::Key << "type" << YAML::Value << floaterTypes[type];
	if (zOffset != 0) out << YAML::Key << "z_offset" << YAML::Value << zOffset;
	if (hasIdleFrameInc) out << YAML::Key << "has_idle_frame_increment" << YAML::Value << true;
	if (hasBackExtraSprite) out << YAML::Key << "has_back_extra_sprite" << YAML::Value << true;
	if (backExtraSpriteIdx != 0) out << YAML::Key << "back_extra_sprite_idx" << YAML::Value << backExtraSpriteIdx;
	if (backViewFrame != 0) out << YAML::Key << "back_view_frame" << YAML::Value << backViewFrame;
	if (idleFrontFrame != 0) out << YAML::Key << "idle_front_frame" << YAML::Value << idleFrontFrame;
	if (headZOffset != 0) out << YAML::Key << "head_z_offset" << YAML::Value << headZOffset;
	if (hasDeadLoot) out << YAML::Key << "has_dead_loot" << YAML::Value << true;
	out << YAML::EndMap;
}

static void emitSpecialBoss(YAML::Emitter& out, int16_t tileIndex) {
	const char* bossTypes[] = {"none", "multipart", "ethereal", "spider"};
	int type = 0;
	struct { int torsoZ, idleSprite, atkSprite, painSprite, renderMode, painRenderMode,
	         legLateral, legBaseZ, idleTorsoZ, idleBobDiv, idleTorsoZBase,
	         walkTorsoZ, atkTorsoZ, painTorsoZ, painLegsZ, painLegPos;
	         bool hasAtkFlare; int flareZ, flareLat, flareTorsoZExtra; } d =
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, 0};

	if (tileIndex == TILE_BOSS_PINKY) {
		type = 1; // multipart
		d.torsoZ = 384; d.idleSprite = 3; d.atkSprite = 8; d.painSprite = 12;
	} else if (tileIndex >= TILE_BOSS_VIOS && tileIndex <= TILE_BOSS_VIOS5) {
		type = 2; // ethereal
		d.renderMode = 3; d.painRenderMode = 5;
	} else if (tileIndex == TILE_BOSS_MASTERMIND) {
		type = 3; // spider
		d.legLateral = -44; d.legBaseZ = 6; d.idleTorsoZ = 35; d.idleBobDiv = 1;
		d.idleTorsoZBase = 0; d.walkTorsoZ = 35; d.atkTorsoZ = 40;
		d.painTorsoZ = 70; d.painLegsZ = 100; d.painLegPos = 2;
		d.hasAtkFlare = true; d.flareZ = 288; d.flareLat = 0; d.flareTorsoZExtra = 10;
	} else if (tileIndex == TILE_ARACHNOTRON) {
		type = 3; // spider
		d.legLateral = -29; d.legBaseZ = 6; d.idleTorsoZ = 50; d.idleBobDiv = 2;
		d.idleTorsoZBase = 50; d.walkTorsoZ = 50; d.atkTorsoZ = 50;
		d.painTorsoZ = 100; d.painLegsZ = 70; d.painLegPos = 1;
	} else {
		return;
	}

	out << YAML::Key << "special_boss" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "type" << YAML::Value << bossTypes[type];
	if (d.torsoZ != 0) out << YAML::Key << "torso_z" << YAML::Value << d.torsoZ;
	if (d.idleSprite != 0) out << YAML::Key << "idle_sprite_idx" << YAML::Value << d.idleSprite;
	if (d.atkSprite != 0) out << YAML::Key << "attack_sprite_idx" << YAML::Value << d.atkSprite;
	if (d.painSprite != 0) out << YAML::Key << "pain_sprite_idx" << YAML::Value << d.painSprite;
	if (d.renderMode != 0) out << YAML::Key << "render_mode_override" << YAML::Value << d.renderMode;
	if (d.painRenderMode != 0) out << YAML::Key << "pain_render_mode" << YAML::Value << d.painRenderMode;
	if (d.legLateral != 0) out << YAML::Key << "leg_lateral" << YAML::Value << d.legLateral;
	if (d.legBaseZ != 0) out << YAML::Key << "leg_base_z" << YAML::Value << d.legBaseZ;
	if (d.idleTorsoZ != 0) out << YAML::Key << "idle_torso_z" << YAML::Value << d.idleTorsoZ;
	if (d.idleBobDiv != 0) out << YAML::Key << "idle_bob_div" << YAML::Value << d.idleBobDiv;
	// idleTorsoZBase: emit even if 0 for mastermind (differs from arachnotron's 50)
	if (type == 3) out << YAML::Key << "idle_torso_z_base" << YAML::Value << d.idleTorsoZBase;
	if (d.walkTorsoZ != 0) out << YAML::Key << "walk_torso_z" << YAML::Value << d.walkTorsoZ;
	if (d.atkTorsoZ != 0) out << YAML::Key << "attack_torso_z" << YAML::Value << d.atkTorsoZ;
	if (d.painTorsoZ != 0) out << YAML::Key << "pain_torso_z" << YAML::Value << d.painTorsoZ;
	if (d.painLegsZ != 0) out << YAML::Key << "pain_legs_z" << YAML::Value << d.painLegsZ;
	if (d.painLegPos != 0) out << YAML::Key << "pain_leg_pos" << YAML::Value << d.painLegPos;
	if (d.hasAtkFlare) out << YAML::Key << "has_attack_flare" << YAML::Value << true;
	if (d.flareZ != 0) out << YAML::Key << "flare_z_offset" << YAML::Value << d.flareZ;
	// flareLat: emit even if 0 when flare exists (mastermind has 0)
	if (d.hasAtkFlare) out << YAML::Key << "flare_lateral_pos" << YAML::Value << d.flareLat;
	if (d.flareTorsoZExtra != 0) out << YAML::Key << "flare_torso_z_extra" << YAML::Value << d.flareTorsoZExtra;
	out << YAML::EndMap;
}

static void emitSpriteAnim(YAML::Emitter& out, int16_t tileIndex, int subtype) {
	if (tileIndex == TILE_BOSS_CYBERDEMON) {
		out << YAML::Key << "sprite_anim" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "clamp_scale" << YAML::Value << true;
		out << YAML::Key << "attack_leg_offset" << YAML::Value << -3;
		out << YAML::Key << "attack_leg_offset_on_flip" << YAML::Value << true;
		out << YAML::EndMap;
	} else if (subtype == M_SAWCUBUS) {
		out << YAML::Key << "sprite_anim" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "has_frame_dependent_head" << YAML::Value << true;
		out << YAML::Key << "head_z_frame0" << YAML::Value << 17;
		out << YAML::Key << "head_x_frame0" << YAML::Value << 2;
		out << YAML::Key << "head_z_frame1" << YAML::Value << 16;
		out << YAML::EndMap;
	}
}

// Check if an entity needs any rendering data at all
static bool entityNeedsRendering(int eType, int subtype, int16_t tileIndex) {
	if (eType == Enums::ET_NPC) return true; // NPC always gets body_parts
	if (eType != Enums::ET_MONSTER) return false; // Only monsters otherwise

	// Check all rendering categories
	switch (subtype) {
		case M_ZOMBIE: case M_IMP: case M_SAWCUBUS: case M_LOST_SOUL:
		case M_PINKY: case M_CACODEMON: case M_SENTINEL: case M_MANCUBUS:
		case M_REVENANT: case M_ARCH_VILE: case M_SENTRY_BOT: case M_CYBERDEMON:
			return true;
	}
	// Special boss tiles
	if (tileIndex == TILE_BOSS_PINKY || tileIndex == TILE_BOSS_MASTERMIND ||
	    tileIndex == TILE_ARACHNOTRON || tileIndex == TILE_BOSS_CYBERDEMON ||
	    (tileIndex >= TILE_BOSS_VIOS && tileIndex <= TILE_BOSS_VIOS5))
		return true;

	return false;
}

static bool convertEntities(ZipFile& zip, const std::string& outDir) {
	int size = 0;
	std::string ipaPath = std::string(PKG_PREFIX) + "entities.bin";
	uint8_t* data = zip.readZipFileEntry(ipaPath.c_str(), &size);
	if (!data) {
		fprintf(stderr, "  entities.bin not found in IPA\n");
		return false;
	}

	int count = readShort(data, 0);
	printf("  entities.bin: %d entities\n", count);

	YAML::Emitter out;
	out << YAML::Comment("Entity definitions");
	out << YAML::Comment(std::to_string(count) + " entities with type, rendering, and animation data");
	out << YAML::Newline;

	// Entity type/subtype lookup tables are now hardcoded in EntityNames.cpp
	out << YAML::BeginMap;
	out << YAML::Key << "entities" << YAML::Value;
	out << YAML::BeginMap;

	// First pass: read all entities to generate unique names
	struct RawEntity {
		int16_t tileIndex;
		uint8_t eType, eSubType, parm, name, longName, description;
	};
	std::vector<RawEntity> entities(count);
	int offset = 2;
	for (int i = 0; i < count; i++) {
		entities[i].tileIndex = readShort(data, offset);
		entities[i].eType = data[offset + 2];
		entities[i].eSubType = data[offset + 3];
		entities[i].parm = data[offset + 4];
		entities[i].name = data[offset + 5];
		entities[i].longName = data[offset + 6];
		entities[i].description = data[offset + 7];
		offset += 8;
	}

	// Generate unique names
	std::map<std::string, int> nameCount;
	std::vector<std::string> entityNames(count);
	for (int i = 0; i < count; i++) {
		auto& e = entities[i];
		const char* typeName = entityTypeName(e.eType);
		entityNames[i] = generateEntityName(e.eType, e.eSubType, e.parm, typeName, nameCount);
	}

	// Build tile index → sprite name lookup
	std::map<int, std::string> tileIdToName;
	for (const auto& te : spriteEntries) {
		if (tileIdToName.find(te.index) == tileIdToName.end()) {
			tileIdToName[te.index] = te.name;
		}
	}

	// Second pass: emit YAML
	for (int i = 0; i < count; i++) {
		auto& e = entities[i];
		std::string typeName = entityTypeName(e.eType);
		std::string subtypeName = entitySubtypeName(e.eType, e.eSubType);
		std::string parmName = entityParmName(e.eType, e.eSubType, e.parm);

		out << YAML::Comment("Entity " + std::to_string(i));
		out << YAML::Key << entityNames[i] << YAML::Value << YAML::BeginMap;
		auto tit = tileIdToName.find((int)e.tileIndex);
		if (tit != tileIdToName.end()) {
			out << YAML::Key << "sprite" << YAML::Value << tit->second;
		} else {
			out << YAML::Key << "sprite" << YAML::Value << (int)e.tileIndex;
		}
		out << YAML::Key << "type" << YAML::Value << typeName;
		out << YAML::Key << "subtype" << YAML::Value << subtypeName;
		out << YAML::Key << "parm" << YAML::Value << parmName;
		out << YAML::Key << "text" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "name" << YAML::Value << (int)e.name;
		out << YAML::Key << "long_name" << YAML::Value << (int)e.longName;
		out << YAML::Key << "description" << YAML::Value << (int)e.description;
		out << YAML::EndMap;

		// Rendering data (only for monsters and NPCs that need it)
		if (entityNeedsRendering(e.eType, e.eSubType, e.tileIndex)) {
			out << YAML::Key << "rendering" << YAML::Value << YAML::BeginMap;
			emitRenderFlags(out, e.tileIndex);
			emitFearEyes(out, e.eSubType);
			emitGunFlare(out, e.eSubType);
			emitBodyParts(out, e.eType, e.eSubType);
			emitFloater(out, e.eSubType);
			emitSpecialBoss(out, e.tileIndex);
			emitSpriteAnim(out, e.tileIndex, e.eSubType);
			out << YAML::EndMap;
		}

		out << YAML::EndMap;
	}

	out << YAML::EndMap;
	out << YAML::EndMap;

	free(data);

	std::string path = outDir + "/entities.yaml";
	if (!writeString(path, out.c_str())) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s (%d entities)\n", path.c_str(), count);
	return true;
}

// ========================================================================
// Conversion: tables.bin -> tables.yaml
// ========================================================================

// Simple cursor for reading table data sequentially
struct TableReader {
	const uint8_t* data;
	int pos;

	int8_t readByte() {
		int8_t v = (int8_t)data[pos];
		pos += 1;
		return v;
	}
	uint8_t readUByte() {
		uint8_t v = data[pos];
		pos += 1;
		return v;
	}
	int16_t readShort() {
		int16_t v = ::readShort(data, pos);
		pos += 2;
		return v;
	}
	int32_t readInt() {
		int32_t v = ::readInt(data, pos);
		pos += 4;
		return v;
	}
};

static bool convertTables(ZipFile& zip, const std::string& outDir) {
	int size = 0;
	std::string ipaPath = std::string(PKG_PREFIX) + "tables.bin";
	uint8_t* data = zip.readZipFileEntry(ipaPath.c_str(), &size);
	if (!data) {
		fprintf(stderr, "  tables.bin not found in IPA\n");
		return false;
	}

	// Parse 20 x int32 offset header
	int offsets[20];
	for (int i = 0; i < 20; i++) {
		offsets[i] = readInt(data, i * 4);
	}
	int dataStart = 80;

	auto tableRange = [&](int index, int& start, int& end) {
		start = (index == 0) ? 0 : offsets[index - 1];
		end = offsets[index];
	};

	YAML::Emitter out;
	out << YAML::Comment("Combat and game data tables");
	out << YAML::Newline;
	out << YAML::BeginMap;

	// --- Tables 2+1: Weapons + WeaponInfo → weapons.yaml ---
	struct WeaponData {
		int strMin, strMax, rangeMin, rangeMax, ammoType, ammoUsage, projType, numShots, shotHold;
	};
	struct WpInfoData {
		int idleX, idleY, atkX, atkY, flashX, flashY;
	};
	std::vector<WeaponData> allWeapons;
	std::vector<WpInfoData> allWpInfo;

	// Read Table 2: Weapons (stride 9)
	{
		int start, end;
		tableRange(2, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();
		int numWeapons = count / 9;
		allWeapons.resize(numWeapons);
		for (int i = 0; i < numWeapons; i++) {
			allWeapons[i].strMin = (int)r.readUByte();
			allWeapons[i].strMax = (int)r.readUByte();
			allWeapons[i].rangeMin = (int)r.readUByte();
			allWeapons[i].rangeMax = (int)r.readUByte();
			allWeapons[i].ammoType = (int)r.readUByte();
			allWeapons[i].ammoUsage = (int)r.readUByte();
			uint8_t pt = r.readUByte();
			allWeapons[i].projType = (pt < 128) ? pt : (pt - 256);
			allWeapons[i].numShots = (int)r.readUByte();
			allWeapons[i].shotHold = (int)r.readUByte();
		}
	}

	// Read Table 1: WeaponInfo (stride 6)
	{
		int start, end;
		tableRange(1, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();
		int numInfo = count / 6;
		allWpInfo.resize(numInfo);
		for (int i = 0; i < numInfo; i++) {
			allWpInfo[i].idleX = (int)r.readByte();
			allWpInfo[i].idleY = (int)r.readByte();
			allWpInfo[i].atkX = (int)r.readByte();
			allWpInfo[i].atkY = (int)r.readByte();
			allWpInfo[i].flashX = (int)r.readByte();
			allWpInfo[i].flashY = (int)r.readByte();
		}
	}

	// Emit consolidated weapons.yaml
	{
		YAML::Emitter wout;
		wout << YAML::Comment("Weapon definitions - consolidated combat and sprite data");
		wout << YAML::Newline;
		wout << YAML::BeginMap;
		wout << YAML::Key << "weapons" << YAML::Value << YAML::BeginMap;

		for (int i = 0; i < (int)allWeapons.size(); i++) {
			const auto& w = allWeapons[i];
			wout << YAML::Key << weaponName(i) << YAML::Value << YAML::BeginMap;
			wout << YAML::Key << "index" << YAML::Value << i;

			// First-person weapon view sprite (references wp_view_* in sprites.yaml)
			if (i < NUM_PLAYER_WEAPONS) {
				std::string wpViewName = "wp_view_" + std::string(weaponName(i));
				wout << YAML::Key << "sprite" << YAML::Value << wpViewName;
			}

			// Attack sound (hardcoded defaults per weapon)
			{
				const char* snd = nullptr;
				const char* sndAlt = nullptr;
				switch (i) {
					case 0: case 8: snd = "chaingun"; break;
					case 1:  snd = "chainsaw"; break;
					case 2:  snd = "holywaterpistol"; break;
					case 3: case 5: case 9: snd = "pistol"; break;
					case 7:  snd = "supershotgun"; break;
					case 10: snd = "plasma"; break;
					case 11: snd = "rocketlauncher"; break;
					case 12: snd = "bfg"; break;
					case 13: snd = "soulcube"; break;
					case 14: snd = "weapon_toilet_throw"; sndAlt = "weapon_toilet_throw2"; break;
				}
				if (snd) wout << YAML::Key << "attack_sound" << YAML::Value << snd;
				if (sndAlt) wout << YAML::Key << "attack_sound_alt" << YAML::Value << sndAlt;
			}

			wout << YAML::Key << "damage" << YAML::Value << YAML::BeginMap;
			wout << YAML::Key << "min" << YAML::Value << w.strMin;
			wout << YAML::Key << "max" << YAML::Value << w.strMax;
			wout << YAML::EndMap;

			wout << YAML::Key << "range" << YAML::Value << YAML::BeginMap;
			wout << YAML::Key << "min" << YAML::Value << w.rangeMin;
			wout << YAML::Key << "max" << YAML::Value << w.rangeMax;
			wout << YAML::EndMap;

			wout << YAML::Key << "ammo" << YAML::Value << YAML::BeginMap;
			wout << YAML::Key << "type" << YAML::Value << ammoTypeName(w.ammoType);
			wout << YAML::Key << "usage" << YAML::Value << w.ammoUsage;
			wout << YAML::EndMap;

			wout << YAML::Key << "projectile" << YAML::Value << projTypeName(w.projType);
			wout << YAML::Key << "shots" << YAML::Value << w.numShots;
			wout << YAML::Key << "shot_hold" << YAML::Value << w.shotHold;

			// Sprite info only for weapons with weapon_info data
			if (i < (int)allWpInfo.size()) {
				const auto& wi = allWpInfo[i];
				wout << YAML::Key << "view_offsets" << YAML::Value << YAML::BeginMap;
				wout << YAML::Key << "idle" << YAML::Value << YAML::Flow << YAML::BeginMap;
				wout << YAML::Key << "x" << YAML::Value << wi.idleX;
				wout << YAML::Key << "y" << YAML::Value << wi.idleY;
				wout << YAML::EndMap;
				wout << YAML::Key << "attack" << YAML::Value << YAML::Flow << YAML::BeginMap;
				wout << YAML::Key << "x" << YAML::Value << wi.atkX;
				wout << YAML::Key << "y" << YAML::Value << wi.atkY;
				wout << YAML::EndMap;
				wout << YAML::Key << "flash" << YAML::Value << YAML::Flow << YAML::BeginMap;
				wout << YAML::Key << "x" << YAML::Value << wi.flashX;
				wout << YAML::Key << "y" << YAML::Value << wi.flashY;
				wout << YAML::EndMap;
				wout << YAML::EndMap;
			}

			// Display offsets (hardcoded defaults per weapon index)
			{
				int offY = 0, swX = 0, swY = 0;
				switch (i) {
					case 1:  offY = 3;  swX = -7; swY = 4;  break; // chainsaw
					case 2:  offY = 10; swY = 3;            break; // holy_water_pistol
					case 3:  offY = 12; swX = -4; swY = 12; break; // shooting_sentry_bot
					case 4:  offY = 12; swX = -4; swY = 12; break; // exploding_sentry_bot
					case 5:  offY = 12; swX = -4; swY = 12; break; // red_shooting_sentry_bot
					case 6:  offY = 12; swX = -4; swY = 12; break; // red_exploding_sentry_bot
					case 7:  swX = 1;                        break; // super_shotgun
					case 8:  swX = -3;                       break; // chaingun
					case 10: swX = 3;                        break; // plasma_gun
					case 11: swX = 3;                        break; // rocket_launcher
					case 12: swX = -11;                      break; // bfg
					case 13: swX = -3;                       break; // soul_cube
				}
				if (offY != 0 || swX != 0 || swY != 0) {
					wout << YAML::Key << "display" << YAML::Value << YAML::BeginMap;
					if (offY != 0)
						wout << YAML::Key << "offset_y" << YAML::Value << offY;
					if (swX != 0 || swY != 0) {
						wout << YAML::Key << "sw_offset" << YAML::Value << YAML::Flow << YAML::BeginMap;
						if (swX != 0) wout << YAML::Key << "x" << YAML::Value << swX;
						if (swY != 0) wout << YAML::Key << "y" << YAML::Value << swY;
						wout << YAML::EndMap;
					}
					wout << YAML::EndMap;
				}
			}

			// HUD data (player weapons only)
			if (i < NUM_PLAYER_WEAPONS) {
				const auto& h = WEAPON_HUD[i];
				wout << YAML::Key << "hud" << YAML::Value << YAML::BeginMap;
				wout << YAML::Key << "tex_row" << YAML::Value << h.texRow;
				wout << YAML::Key << "show_ammo" << YAML::Value << h.showAmmo;
				wout << YAML::EndMap;
			}

			// On-kill stat grant (chainsaw: every 30 kills grants +2 strength)
			if (i == 1) {
				wout << YAML::Key << "on_kill_grant" << YAML::Value << YAML::BeginMap;
				wout << YAML::Key << "kills" << YAML::Value << 30;
				wout << YAML::Key << "strength" << YAML::Value << 2;
				wout << YAML::EndMap;
			}

			// Behavior flags (player weapons only)
			if (i < NUM_PLAYER_WEAPONS) {
				const auto& b = WEAPON_BEHAVIOR[i];
				bool anySet = b.vibrateAnim || b.chainsawHitEvent || b.isThrowableItem ||
					b.showFlashFrame || b.hasFlashSprite || b.soulAmmoDisplay ||
					b.isMelee || b.noRecoil || b.splashDamage || b.noBloodOnHit ||
					b.drawDoubleSprite || b.consumeOnUse || b.chargeAttack ||
					b.flipSpriteOnEnd;
				if (anySet) {
					wout << YAML::Key << "behavior" << YAML::Value << YAML::BeginMap;
					if (b.vibrateAnim) wout << YAML::Key << "vibrate_anim" << YAML::Value << true;
					if (b.chainsawHitEvent) wout << YAML::Key << "chainsaw_hit_event" << YAML::Value << true;
					if (b.isThrowableItem) wout << YAML::Key << "is_throwable_item" << YAML::Value << true;
					if (b.showFlashFrame) wout << YAML::Key << "show_flash_frame" << YAML::Value << true;
					if (b.hasFlashSprite) wout << YAML::Key << "has_flash_sprite" << YAML::Value << true;
					if (b.soulAmmoDisplay) wout << YAML::Key << "soul_ammo_display" << YAML::Value << true;
					if (b.isMelee) wout << YAML::Key << "is_melee" << YAML::Value << true;
					if (b.noRecoil) wout << YAML::Key << "no_recoil" << YAML::Value << true;
					if (b.splashDamage) {
						wout << YAML::Key << "splash_damage" << YAML::Value << true;
						wout << YAML::Key << "splash_divisor" << YAML::Value << b.splashDivisor;
					}
					if (b.noBloodOnHit) wout << YAML::Key << "no_blood_on_hit" << YAML::Value << true;
					if (b.drawDoubleSprite) wout << YAML::Key << "draw_double_sprite" << YAML::Value << true;
					if (b.consumeOnUse) wout << YAML::Key << "consume_on_use" << YAML::Value << true;
					if (b.chargeAttack) wout << YAML::Key << "charge_attack" << YAML::Value << true;
					if (b.flipSpriteOnEnd) wout << YAML::Key << "flip_sprite_on_end" << YAML::Value << true;
					wout << YAML::EndMap;
				}
			}

			wout << YAML::EndMap;
		}

		wout << YAML::EndMap;

		// Ammo parm name → index lookup
		wout << YAML::Key << "ammo_parms" << YAML::Value << YAML::BeginMap;
		for (int i = 0; i < NUM_AMMO_PARMS; i++) {
			wout << YAML::Key << AMMO_PARM_NAMES[i] << YAML::Value << i;
		}
		wout << YAML::EndMap;

		wout << YAML::EndMap;

		std::string wpPath = outDir + "/weapons.yaml";
		if (!writeString(wpPath, wout.c_str())) {
			fprintf(stderr, "  Failed to write %s\n", wpPath.c_str());
			return false;
		}
		printf("  -> %s (%d weapons, %d with sprites)\n", wpPath.c_str(), (int)allWeapons.size(), (int)allWpInfo.size());
	}

	// --- Table 4: CombatMasks (int table) ---
	{
		int start, end;
		tableRange(4, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();

		out << YAML::Key << "combat_masks" << YAML::Value << YAML::BeginSeq;
		for (int i = 0; i < count; i++) {
			uint32_t val = (uint32_t)r.readInt();
			char buf[16];
			snprintf(buf, sizeof(buf), "0x%08X", val);
			out << std::string(buf);
		}
		out << YAML::EndSeq;
	}

	// --- Table 5: KeysNumeric (flat byte array) ---
	{
		int start, end;
		tableRange(5, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();

		out << YAML::Key << "keys_numeric" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "data" << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int i = 0; i < count; i++) {
			out << (int)r.readByte();
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	// --- Table 6: OSCCycle (flat byte array) ---
	{
		int start, end;
		tableRange(6, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();

		out << YAML::Key << "osc_cycle" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "data" << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int i = 0; i < count; i++) {
			out << (int)r.readByte();
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	// --- Table 7: LevelNames (short table) ---
	{
		int start, end;
		tableRange(7, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();

		g_levelNameIds.clear();
		out << YAML::Key << "level_names" << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int i = 0; i < count; i++) {
			int16_t id = r.readShort();
			g_levelNameIds.push_back(id);
			out << (int)id;
		}
		out << YAML::EndSeq;
	}

	// --- Table 9: SinTable (flat int array) ---
	{
		int start, end;
		tableRange(9, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();

		out << YAML::Key << "sin_table" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "data" << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int i = 0; i < count; i++) {
			out << r.readInt();
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	// --- Table 10: EnergyDrinkData (flat short array) ---
	{
		int start, end;
		tableRange(10, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();

		out << YAML::Key << "energy_drink_data" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "data" << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int i = 0; i < count; i++) {
			out << (int)r.readShort();
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	// --- Table 12: MovieEffects (flat int array) ---
	{
		int start, end;
		tableRange(12, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();

		out << YAML::Key << "movie_effects" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "data" << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int i = 0; i < count; i++) {
			out << r.readInt();
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	out << YAML::EndMap;

	std::string path = outDir + "/tables.yaml";
	if (!writeString(path, out.c_str())) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s\n", path.c_str());

	// === Write consolidated monsters.yaml ===

	// Read all monster tables into temp arrays
	int numTypes = 0;

	// Table 0: MonsterAttacks
	struct AttackTier { std::string atk1, atk2; int chance; };
	std::vector<std::array<AttackTier, 3>> allAttacks;
	{
		int start, end;
		tableRange(0, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();
		int num = count / 9;
		numTypes = std::max(numTypes, num);
		allAttacks.resize(num);
		for (int i = 0; i < num; i++) {
			for (int p = 0; p < 3; p++) {
				allAttacks[i][p].atk1 = weaponName(r.readShort());
				allAttacks[i][p].atk2 = weaponName(r.readShort());
				allAttacks[i][p].chance = (int)r.readShort();
			}
		}
	}

	// Table 3: MonsterStats
	struct StatTier { int vals[6]; };
	std::vector<std::array<StatTier, 3>> allStats;
	{
		int start, end;
		tableRange(3, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();
		int numEntities = count / 6;
		int num = numEntities / 3;
		numTypes = std::max(numTypes, num);
		allStats.resize(num);
		for (int t = 0; t < num; t++) {
			for (int p = 0; p < 3; p++) {
				for (int s = 0; s < 6; s++) {
					allStats[t][p].vals[s] = (int)r.readByte();
				}
			}
		}
	}

	// Table 8: MonsterColors
	std::vector<std::array<int, 3>> allColors;
	int numColors = 0;
	{
		int start, end;
		tableRange(8, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();
		numColors = count / 3;
		allColors.resize(numColors);
		for (int i = 0; i < numColors; i++) {
			allColors[i][0] = r.readUByte();
			allColors[i][1] = r.readUByte();
			allColors[i][2] = r.readUByte();
		}
	}

	// Table 11: MonsterWeakness
	std::vector<std::array<std::array<int, 8>, 3>> allWeakness;
	{
		int start, end;
		tableRange(11, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();
		int numEntities = count / 8;
		int num = numEntities / 3;
		numTypes = std::max(numTypes, num);
		allWeakness.resize(num);
		for (int t = 0; t < num; t++) {
			for (int p = 0; p < 3; p++) {
				for (int w = 0; w < 8; w++) {
					allWeakness[t][p][w] = (int)r.readByte();
				}
			}
		}
	}

	// Table 13: MonsterSounds
	struct SoundSet { int vals[8]; };
	std::vector<SoundSet> allSounds;
	{
		int start, end;
		tableRange(13, start, end);
		TableReader r = {data, dataStart + start};
		int count = r.readInt();
		int num = count / 8;
		numTypes = std::max(numTypes, num);
		allSounds.resize(num);
		for (int i = 0; i < num; i++) {
			for (int f = 0; f < 8; f++) {
				allSounds[i].vals[f] = (int)r.readUByte();
			}
		}
	}

	// Emit consolidated monsters.yaml
	const char* soundFields[] = {"alert1", "alert2", "alert3", "attack1", "attack2", "idle", "pain", "death"};
	const char* statNames[] = {"health", "armor", "defense", "strength", "accuracy", "agility"};

	YAML::Emitter mout;
	mout << YAML::Comment("Monster combat data - consolidated definitions");
	mout << YAML::Newline;
	mout << YAML::BeginMap;
	mout << YAML::Key << "monsters" << YAML::Value << YAML::BeginMap;

	for (int i = 0; i < numTypes; i++) {
		mout << YAML::Key << monsterName(i) << YAML::Value << YAML::BeginMap;
		mout << YAML::Key << "index" << YAML::Value << i;

		// Blood color as hex "#RRGGBB" (only for first numColors monsters)
		if (i < numColors) {
			char hexColor[8];
			snprintf(hexColor, sizeof(hexColor), "#%02X%02X%02X",
				allColors[i][0], allColors[i][1], allColors[i][2]);
			mout << YAML::Key << "blood_color" << YAML::Value << std::string(hexColor);
		}

		// Sounds (output as names, not numeric indices)
		if (i < (int)allSounds.size()) {
			mout << YAML::Key << "sounds" << YAML::Value << YAML::BeginMap;
			for (int f = 0; f < 8; f++) {
				int sndIdx = allSounds[i].vals[f];
				if (sndIdx == 255) {
					mout << YAML::Key << soundFields[f] << YAML::Value << "none";
				} else if (sndIdx < NUM_DEFAULT_SOUNDS) {
					// Convert index to name: strip .wav, lowercase
					std::string sndName = DEFAULT_SOUND_NAMES[sndIdx];
					size_t dotPos = sndName.rfind('.');
					if (dotPos != std::string::npos) {
						sndName = sndName.substr(0, dotPos);
					}
					std::transform(sndName.begin(), sndName.end(), sndName.begin(), ::tolower);
					mout << YAML::Key << soundFields[f] << YAML::Value << sndName;
				} else {
					mout << YAML::Key << soundFields[f] << YAML::Value << sndIdx;
				}
			}
			mout << YAML::EndMap;
		}

		// Tiers
		mout << YAML::Key << "tiers" << YAML::Value << YAML::BeginSeq;
		for (int p = 0; p < 3; p++) {
			mout << YAML::Comment("parm " + std::to_string(p));
			mout << YAML::BeginMap;

			// Stats
			if (i < (int)allStats.size()) {
				mout << YAML::Key << "stats" << YAML::Value << YAML::BeginMap;
				for (int s = 0; s < 6; s++) {
					mout << YAML::Key << statNames[s] << YAML::Value << allStats[i][p].vals[s];
				}
				mout << YAML::EndMap;
			}

			// Attacks
			if (i < (int)allAttacks.size()) {
				mout << YAML::Key << "attacks" << YAML::Value << YAML::BeginMap;
				mout << YAML::Key << "attack1" << YAML::Value << allAttacks[i][p].atk1;
				mout << YAML::Key << "attack2" << YAML::Value << allAttacks[i][p].atk2;
				mout << YAML::Key << "chance" << YAML::Value << allAttacks[i][p].chance;
				mout << YAML::EndMap;
			}

			// Weakness: output as named weapon -> damage percentage map
			if (i < (int)allWeakness.size()) {
				mout << YAML::Key << "weakness" << YAML::Comment("damage % per weapon (unlisted = 100%)");
				mout << YAML::Value << YAML::BeginMap;
				for (int wi = 0; wi < 15; wi++) { // 15 player weapons
					int byteIdx = wi / 2;
					uint8_t ub = (uint8_t)allWeakness[i][p][byteIdx];
					int nibble = (wi % 2 == 0) ? (ub & 0xF) : ((ub >> 4) & 0xF);
					double pct = (nibble + 1) * 32.0 / 256.0 * 100.0;
					if (pct != 100.0) {
						mout << YAML::Key << WeaponNames::TABLE[wi] << YAML::Value;
						if (pct == (int)pct) {
							mout << (int)pct;
						} else {
							mout << pct;
						}
					}
				}
				mout << YAML::EndMap;
			}

			mout << YAML::EndMap;
		}
		mout << YAML::EndSeq;

		mout << YAML::EndMap;
	}

	mout << YAML::EndMap;
	mout << YAML::EndMap;

	std::string mpath = outDir + "/monsters.yaml";
	if (!writeString(mpath, mout.c_str())) {
		fprintf(stderr, "  Failed to write %s\n", mpath.c_str());
		free(data);
		return false;
	}
	printf("  -> %s (%d monsters)\n", mpath.c_str(), numTypes);

	free(data);
	return true;
}

// ========================================================================
// Generate: projectiles.yaml (hardcoded defaults, not from binary)
// ========================================================================
static bool generateAnimationsYaml(const std::string& outDir) {
	YAML::Emitter out;
	out << YAML::Comment("Tile/sprite name-to-index mapping");
	out << YAML::Comment("Used by projectiles.yaml, entities.yaml, and other configs");
	out << YAML::Comment("Modders can remap sprite sheet layouts by changing the indices here.");
	out << YAML::Newline;

	out << YAML::BeginMap;
	out << YAML::Key << "sprites" << YAML::Value << YAML::BeginMap;
	for (const auto& e : spriteEntries) {
		out << YAML::Key << e.name << YAML::Value << YAML::Flow << YAML::BeginMap;
		out << YAML::Key << "file" << YAML::Value << "tables.bin";
		out << YAML::Key << "id" << YAML::Value << e.index;
		out << YAML::EndMap;
	}

	// First-person weapon view sprite aliases (used by weapons.yaml sprite: field)
	struct WpViewEntry { const char* name; int id; };
	static const WpViewEntry wpViewEntries[] = {
		{"wp_view_assault_rifle", 1},
		{"wp_view_chainsaw", 2},
		{"wp_view_holy_water_pistol", 3},
		{"wp_view_shooting_sentry_bot", 4},
		{"wp_view_exploding_sentry_bot", 5},
		{"wp_view_super_shotgun", 8},
		{"wp_view_chaingun", 9},
		{"wp_view_assault_rifle_with_scope", 10},
		{"wp_view_plasma_gun", 11},
		{"wp_view_rocket_launcher", 12},
		{"wp_view_bfg", 13},
		{"wp_view_soul_cube", 14},
		{"wp_view_red_shooting_sentry_bot", 13},
		{"wp_view_red_exploding_sentry_bot", 14},
		{"wp_view_item", 15},
	};
	for (const auto& wv : wpViewEntries) {
		out << YAML::Key << wv.name << YAML::Value << YAML::Flow << YAML::BeginMap;
		out << YAML::Key << "file" << YAML::Value << "tables.bin";
		out << YAML::Key << "id" << YAML::Value << wv.id;
		out << YAML::EndMap;
	}

	// UI image assets (BMP files referenced by ui.yaml)
	struct BmpEntry { const char* name; const char* file; };
	static const BmpEntry bmpEntries[] = {
		{"menu_btn_bg", "menu_button_background.bmp"},
		{"menu_btn_bg_on", "menu_button_background_on.bmp"},
		{"menu_arrow_down", "menu_arrow_down.bmp"},
		{"menu_arrow_up", "menu_arrow_up.bmp"},
		{"switch_left_normal", "Switch_Left_Normal.bmp"},
		{"switch_left_active", "Switch_Left_Active.bmp"},
		{"slider_bar", "Menu_Option_SliderBar.bmp"},
		{"slider_on", "Menu_Option_SliderON.bmp"},
		{"slider_off", "Menu_Option_SliderOFF.bmp"},
		{"info_btn_normal", "gameMenu_infoButton_Normal.bmp"},
		{"info_btn_pressed", "gameMenu_infoButton_Pressed.bmp"},
		{"vending_arrow_up_glow", "vending_arrow_up_glow.bmp"},
		{"vending_arrow_down_glow", "vending_arrow_down_glow.bmp"},
		{"vending_btn_large", "vending_button_large.bmp"},
		{"vending_btn_help", "vending_button_help.bmp"},
		{"ingame_option_btn", "inGame_menu_option_button.bmp"},
		{"menu_dial", "Menu_Dial.bmp"},
		{"menu_dial_knob", "Menu_Dial_Knob.bmp"},
		{"menu_main_box", "Menu_Main_BOX.bmp"},
		{"main_menu_overlay", "Main_Menu_OverLay.bmp"},
		{"main_help_overlay", "Main_Help_OverLay.bmp"},
		{"main_about_overlay", "Main_About_OverLay.bmp"},
		{"menu_yesno_box", "Menu_YesNo_BOX.bmp"},
		{"menu_choose_diff_box", "Menu_ChooseDIFF_BOX.bmp"},
		{"menu_language_box", "Menu_Language_BOX.bmp"},
		{"menu_option_box3", "Menu_Option_BOX3.bmp"},
		{"menu_option_box4", "Menu_Option_BOX4.bmp"},
		{"hud_numbers", "Hud_Numbers.bmp"},
		{"game_menu_panel_bottom", "gameMenu_Panel_bottom.bmp"},
		{"game_menu_panel_bottom_sentry", "gameMenu_Panel_bottom_sentrybot.bmp"},
		{"game_menu_health", "gameMenu_Health.bmp"},
		{"game_menu_shield", "gameMenu_Shield.bmp"},
		{"game_menu_torn_page", "gameMenu_TornPage.bmp"},
		{"game_menu_background", "gameMenu_Background.bmp"},
		{"main_menu_dial_a_anim", "Main_Menu_DialA_anim.bmp"},
		{"main_menu_dial_c_anim", "Main_Menu_DialC_anim.bmp"},
		{"main_menu_slide_anim", "Main_Menu_Slide_anim.bmp"},
		{"game_menu_scrollbar", "gameMenu_ScrollBar.bmp"},
		{"game_menu_top_slider", "gameMenu_topSlider.bmp"},
		{"game_menu_mid_slider", "gameMenu_midSlider.bmp"},
		{"game_menu_bottom_slider", "gameMenu_bottomSlider.bmp"},
		{"vending_scrollbar", "vending_scroll_bar.bmp"},
		{"vending_scroll_btn_top", "vending_scroll_button_top.bmp"},
		{"vending_scroll_btn_mid", "vending_scroll_button_middle.bmp"},
		{"vending_scroll_btn_bottom", "vending_scroll_button_bottom.bmp"},
		{"logo", "logo2.bmp"},
	};
	for (const auto& img : bmpEntries) {
		out << YAML::Key << img.name << YAML::Value << YAML::Flow << YAML::BeginMap;
		out << YAML::Key << "file" << YAML::Value << img.file;
		out << YAML::EndMap;
	}
	out << YAML::EndMap;

	out << YAML::EndMap;

	std::string path = outDir + "/sprites.yaml";
	FILE* f = fopen(path.c_str(), "w");
	if (!f) return false;
	fprintf(f, "%s\n", out.c_str());
	fclose(f);
	printf("  -> %s\n", path.c_str());
	return true;
}

static bool generateProjectilesYaml(const std::string& outDir) {
	YAML::Emitter out;
	out << YAML::Comment("Projectile visual definitions");
	out << YAML::Comment("Controls missile animation, render mode, and impact effects");
	out << YAML::Newline;
	out << YAML::BeginMap;
	out << YAML::Key << "projectiles" << YAML::Value << YAML::BeginMap;

	struct ProjDef {
		const char* name;
		int launchRM, launchAnim, launchAnimMon, launchSpeed, launchSpeedAdd;
		int launchOffXR, launchOffZ, launchZOff;
		bool animFromWeapon;
		int impactAnim, impactRM;
		const char* impactSound;
		bool shake; int shakeDur, shakeInt, shakeFade;
	};
	ProjDef defs[] = {
		{"bullet",      -1, 0,0,0,0, 0,0,0, false,  0,0, nullptr, false,0,0,0},
		{"melee",       -1, 0,0,0,0, 0,0,0, false,  0,0, nullptr, false,0,0,0},
		{"water",        3, 240,0,512,0, 0,0,0, false,  235,4, nullptr, false,0,0,0},
		{"plasma",       3, 243,0,512,0, 4,15,0, false,  243,4, nullptr, false,0,0,0},
		{"rocket",       0, 225,226,0,64, 4,10,0, false,  242,4, "explosion", true,500,4,500},
		{"bfg",          4, 244,0,0,0, 0,0,0, false,  244,4, nullptr, false,0,0,0},
		{"flesh",        0, 227,0,0,0, 0,0,0, false,  0,0, nullptr, false,0,0,0},
		{"fire",         3, 242,0,0,0, 0,0,0, false,  242,4, "fireball_impact", false,0,0,0},
		{"caco_plasma",  3, 241,0,0,0, 0,0,0, false,  0,0, nullptr, false,0,0,0},
		{"thorns",       0, 171,0,0,0, 0,0,-32, false,  170,0, nullptr, false,0,0,0},
		{"acid",         3, 252,0,0,0, 0,0,48, false,  252,4, nullptr, false,0,0,0},
		{"electric",     3, 248,0,0,0, 0,0,0, false,  0,0, nullptr, false,0,0,0},
		{"soul_cube",   -1, 0,0,0,0, 0,0,0, false,  0,0, nullptr, false,0,0,0},
		{"item",         0, 0,0,0,0, 0,0,0, true,  0,0, "weapon_pickup", false,0,0,0},
	};

	for (const auto& d : defs) {
		out << YAML::Key << d.name << YAML::Value << YAML::BeginMap;

		if (d.launchRM >= 0 || d.animFromWeapon) {
			out << YAML::Key << "launch" << YAML::Value << YAML::BeginMap;
			if (d.launchRM >= 0) out << YAML::Key << "render_mode" << YAML::Value << renderModeName(d.launchRM);
			if (d.launchAnimMon != 0) {
				out << YAML::Key << "sprite_player" << YAML::Value << tileName(d.launchAnim);
				out << YAML::Key << "sprite_monster" << YAML::Value << tileName(d.launchAnimMon);
			} else if (d.launchAnim != 0) {
				out << YAML::Key << "sprite" << YAML::Value << tileName(d.launchAnim);
			}
			if (d.launchSpeed > 0) out << YAML::Key << "speed" << YAML::Value << d.launchSpeed;
			if (d.launchSpeedAdd != 0) out << YAML::Key << "speed_add" << YAML::Value << d.launchSpeedAdd;
			if (d.launchOffXR != 0 || d.launchOffZ != 0) {
				out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginMap;
				if (d.launchOffXR != 0) {
					out << YAML::Key << "x_right" << YAML::Value << d.launchOffXR;
					out << YAML::Key << "y_right" << YAML::Value << d.launchOffXR;
				}
				if (d.launchOffZ != 0) out << YAML::Key << "z" << YAML::Value << d.launchOffZ;
				out << YAML::EndMap;
			}
			if (d.launchZOff != 0) out << YAML::Key << "z_offset" << YAML::Value << d.launchZOff;
			if (d.animFromWeapon) out << YAML::Key << "sprite_from_weapon" << YAML::Value << true;
			out << YAML::EndMap;
		}

		if (d.impactAnim != 0 || d.impactSound != nullptr || d.shake) {
			out << YAML::Key << "impact" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "sprite" << YAML::Value << tileName(d.impactAnim);
			if (d.impactRM != 0) out << YAML::Key << "render_mode" << YAML::Value << renderModeName(d.impactRM);
			if (d.impactSound) out << YAML::Key << "impact_sound" << YAML::Value << d.impactSound;
			if (d.shake) {
				out << YAML::Key << "screen_shake" << YAML::Value << YAML::Flow << YAML::BeginMap;
				out << YAML::Key << "duration" << YAML::Value << d.shakeDur;
				out << YAML::Key << "intensity" << YAML::Value << d.shakeInt;
				out << YAML::Key << "fade" << YAML::Value << d.shakeFade;
				out << YAML::EndMap;
			}
			out << YAML::EndMap;
		}

		out << YAML::EndMap;
	}

	out << YAML::EndMap;
	out << YAML::EndMap;

	std::string path = outDir + "/projectiles.yaml";
	if (!writeString(path, out.c_str())) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s (14 projectile types)\n", path.c_str());
	return true;
}

// ========================================================================
// Generate: effects.yaml (buff/status effect definitions)
// ========================================================================
static bool generateEffectsYaml(const std::string& outDir) {
	struct BuffDef {
		const char* name;
		int maxStacks;
		bool hasAmount;
		bool drawAmount;
		const char* blockedBy;     // nullptr = none
		const char* onApplySound;  // nullptr = none
		int perTurnDamage;         // 0 = none
		const char* perTurn;       // nullptr = none
	};

	static const BuffDef buffs[] = {
		// name          stacks  hasAmt  drawAmt  blockedBy  applySound        ptDmg  perTurn
		{ "reflect",     3,      false,  false,   nullptr,   nullptr,          0,     nullptr },
		{ "purify",      3,      false,  false,   nullptr,   nullptr,          0,     nullptr },
		{ "haste",       3,      false,  false,   nullptr,   nullptr,          0,     nullptr },
		{ "regen",       3,      true,   true,    nullptr,   nullptr,          0,     "heal_by_amount" },
		{ "defense",     3,      true,   true,    nullptr,   nullptr,          0,     nullptr },
		{ "strength",    3,      true,   true,    nullptr,   nullptr,          0,     nullptr },
		{ "agility",     3,      true,   true,    nullptr,   nullptr,          0,     nullptr },
		{ "focus",       3,      true,   true,    nullptr,   nullptr,          0,     nullptr },
		{ "anger",       3,      true,   true,    nullptr,   nullptr,          0,     nullptr },
		{ "antifire",    1,      false,  false,   nullptr,   nullptr,          0,     nullptr },
		{ "fortitude",   3,      true,   true,    nullptr,   nullptr,          0,     nullptr },
		{ "fear",        3,      false,  false,   nullptr,   nullptr,          0,     nullptr },
		{ "wp_poison",   1,      false,  false,   nullptr,   nullptr,          0,     nullptr },
		{ "fire",        3,      true,   false,   "antifire","fireball_impact",3,     nullptr },
		{ "disease",     3,      true,   true,    nullptr,   nullptr,          0,     nullptr },
	};
	static const int NUM_BUFFS = sizeof(buffs) / sizeof(buffs[0]);

	YAML::Emitter out;
	out << YAML::Comment("Buff/status effect definitions");
	out << YAML::Comment("Loaded by the engine to configure buff behavior");
	out << YAML::Newline;
	out << YAML::BeginMap;

	out << YAML::Key << "warning_time" << YAML::Value << 10;
	out << YAML::Newline;

	out << YAML::Key << "buffs" << YAML::Value << YAML::BeginMap;

	for (int i = 0; i < NUM_BUFFS; i++) {
		const auto& b = buffs[i];
		out << YAML::Key << b.name << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "max_stacks" << YAML::Value << b.maxStacks;
		out << YAML::Key << "has_amount" << YAML::Value << b.hasAmount;
		out << YAML::Key << "draw_amount" << YAML::Value << b.drawAmount;
		if (b.blockedBy) {
			out << YAML::Key << "blocked_by" << YAML::Value << b.blockedBy;
		}
		if (b.onApplySound) {
			out << YAML::Key << "on_apply_sound" << YAML::Value << b.onApplySound;
		}
		if (b.perTurnDamage > 0) {
			out << YAML::Key << "per_turn_damage" << YAML::Value << b.perTurnDamage;
		}
		if (b.perTurn) {
			out << YAML::Key << "per_turn" << YAML::Value << b.perTurn;
		}
		out << YAML::EndMap;
	}

	out << YAML::EndMap;
	out << YAML::EndMap;

	std::string path = outDir + "/effects.yaml";
	if (!writeString(path, out.c_str())) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s (%d buff types)\n", path.c_str(), NUM_BUFFS);
	return true;
}

// ========================================================================
// Conversion: menus.bin -> menus.yaml
// ========================================================================
static bool convertMenus(ZipFile& zip, const std::string& outDir) {
	int size = 0;
	std::string ipaPath = std::string(PKG_PREFIX) + "menus.bin";
	uint8_t* data = zip.readZipFileEntry(ipaPath.c_str(), &size);
	if (!data) {
		fprintf(stderr, "  menus.bin not found in IPA\n");
		return false;
	}

	uint16_t menuDataCount = readUShort(data, 0);
	uint16_t menuItemsCount = readUShort(data, 2);
	printf("  menus.bin: %d menus, %d item words\n", menuDataCount, menuItemsCount);

	// Read menuData array
	std::vector<uint32_t> menuData(menuDataCount);
	int offset = 4;
	for (int i = 0; i < menuDataCount; i++) {
		menuData[i] = readUInt(data, offset);
		offset += 4;
	}

	// Read menuItems array
	std::vector<uint32_t> menuItems(menuItemsCount);
	for (int i = 0; i < menuItemsCount; i++) {
		menuItems[i] = readUInt(data, offset);
		offset += 4;
	}

	// Menu name lookup by ID
	struct MenuNameEntry { int id; const char* name; };
	static const MenuNameEntry MENU_NAMES[] = {
		{-6, "main_controller"}, {-5, "main_bindings"}, {-4, "main_controls"},
		{-3, "main_options_sound"}, {-2, "main_options_video"}, {-1, "main_options_input"},
		{0, "none"}, {1, "level_stats"}, {2, "drawsworld"},
		{3, "main"}, {4, "main_help"}, {5, "main_armorhelp"},
		{6, "main_effecthelp"}, {7, "main_itemhelp"}, {8, "main_about"},
		{9, "main_general"}, {10, "main_move"}, {11, "main_attack"},
		{12, "main_sniper"}, {13, "main_exit"}, {14, "main_confirmnew"},
		{15, "main_confirmnew2"}, {16, "main_difficulty"}, {17, "main_options"},
		{18, "main_minigame"}, {19, "main_more_games"}, {20, "main_hacker_help"},
		{21, "main_matrix_skip_help"}, {22, "main_power_up_help"}, {23, "select_language"},
		{24, "end_ranking"}, {25, "enable_sounds"}, {26, "end"},
		{27, "end_finalquit"}, {28, "inherit_backmenu"}, {29, "ingame"},
		{30, "ingame_status"}, {31, "ingame_player"}, {32, "ingame_level"},
		{33, "ingame_grades"}, {35, "ingame_options"}, {36, "ingame_language"},
		{37, "ingame_help"}, {38, "ingame_general"}, {39, "ingame_move"},
		{40, "ingame_attack"}, {41, "ingame_sniper"}, {42, "ingame_exit"},
		{43, "ingame_armorhelp"}, {44, "ingame_effecthelp"}, {45, "ingame_itemhelp"},
		{46, "ingame_questlog"}, {47, "ingame_recipes"}, {48, "ingame_save"},
		{49, "ingame_load"}, {50, "ingame_loadnosave"}, {51, "ingame_dead"},
		{52, "ingame_restartlvl"}, {53, "ingame_savequit"},
		{57, "ingame_kicking"}, {58, "ingame_special_exit"},
		{59, "ingame_hacker_help"}, {60, "ingame_matrix_skip_help"},
		{61, "ingame_power_up_help"}, {62, "ingame_controls"},
		{65, "debug"}, {66, "debug_maps"}, {67, "debug_stats"},
		{68, "debug_cheats"}, {69, "developer_vars"}, {70, "debug_sys"},
		{71, "showdetails"}, {72, "items"}, {73, "items_weapons"},
		{75, "items_drinks"}, {77, "items_confirm"},
		{79, "items_healthmsg"}, {80, "items_armormsg"},
		{81, "items_syringemsg"}, {82, "items_holy_water_max"},
		{83, "vending_machine"}, {84, "vending_machine_drinks"},
		{85, "vending_machine_snacks"}, {86, "vending_machine_confirm"},
		{87, "vending_machine_cant_buy"}, {88, "vending_machine_details"},
		{89, "comic_book"},
	};
	auto menuIdToName = [&](int id) -> const char* {
		for (const auto& e : MENU_NAMES) {
			if (e.id == id) return e.name;
		}
		return nullptr;
	};

	YAML::Emitter out;
	out << YAML::Comment("Menu definitions for DoomIIRPG");
	out << YAML::Comment("string_id values are indices into menu strings (group 3)");
	out << YAML::Comment("goto targets reference menu names instead of numeric IDs");
	out << YAML::Newline;
	out << YAML::BeginMap;

	// Helper lambdas for theme emission
	// Emit inlined button/info_button theme properties (no component references)
	auto emitButtonMain = [](YAML::Emitter& e) {
		e << YAML::Key << "button" << YAML::Value << YAML::Flow << YAML::BeginMap;
		e << YAML::Key << "image" << YAML::Value << "menu_btn_bg";
		e << YAML::Key << "image_highlight" << YAML::Value << "menu_btn_bg_on";
		e << YAML::Key << "render_mode" << YAML::Value << 1;
		e << YAML::Key << "highlight_render_mode" << YAML::Value << 0;
		e << YAML::EndMap;
	};
	auto emitInfoButton = [](YAML::Emitter& e) {
		e << YAML::Key << "info_button" << YAML::Value << YAML::Flow << YAML::BeginMap;
		e << YAML::Key << "image" << YAML::Value << "info_btn_normal";
		e << YAML::Key << "image_highlight" << YAML::Value << "info_btn_pressed";
		e << YAML::Key << "render_mode" << YAML::Value << 1;
		e << YAML::Key << "highlight_render_mode" << YAML::Value << 0;
		e << YAML::EndMap;
	};
	auto emitThemeMain = [&](YAML::Emitter& e) {
		emitButtonMain(e);
		emitInfoButton(e);
		e << YAML::Key << "item_height" << YAML::Value << 46;
		e << YAML::Key << "scrollbar" << YAML::Value << YAML::Flow << YAML::BeginMap;
		e << YAML::Key << "style" << YAML::Value << "dial";
		e << YAML::Key << "default_x" << YAML::Value << 408;
		e << YAML::Key << "default_y" << YAML::Value << 81;
		e << YAML::EndMap;
	};
	auto emitThemeMainWide = [&](YAML::Emitter& e) {
		e << YAML::Key << "button" << YAML::Value << YAML::Flow << YAML::BeginMap;
		e << YAML::Key << "image" << YAML::Value << "menu_btn_bg_ext";
		e << YAML::Key << "image_highlight" << YAML::Value << "menu_btn_bg_ext_on";
		e << YAML::Key << "render_mode" << YAML::Value << 1;
		e << YAML::Key << "highlight_render_mode" << YAML::Value << 0;
		e << YAML::EndMap;
		emitInfoButton(e);
		e << YAML::Key << "item_height" << YAML::Value << 46;
	};
	auto emitThemeIngame = [&](YAML::Emitter& e) {
		e << YAML::Key << "button" << YAML::Value << YAML::Flow << YAML::BeginMap;
		e << YAML::Key << "image" << YAML::Value << "ingame_option_btn";
		e << YAML::Key << "render_mode" << YAML::Value << 1;
		e << YAML::Key << "highlight_render_mode" << YAML::Value << 0;
		e << YAML::EndMap;
		emitInfoButton(e);
		e << YAML::Key << "item_padding_bottom" << YAML::Value << 10;
		e << YAML::Key << "scrollbar" << YAML::Value << YAML::Flow << YAML::BeginMap;
		e << YAML::Key << "style" << YAML::Value << "bar";
		e << YAML::Key << "images" << YAML::Value << YAML::Flow << YAML::BeginSeq
		   << "game_scrollbar" << "game_top_slider" << "game_mid_slider" << "game_bottom_slider" << YAML::EndSeq;
		e << YAML::Key << "x" << YAML::Value << 430;
		e << YAML::Key << "width" << YAML::Value << 50;
		e << YAML::EndMap;
	};
	auto emitLayoutStr = [](YAML::Emitter& e, const char* x, int y, const char* w, int h) {
		e << YAML::Key << "layout" << YAML::Value << YAML::Flow << YAML::BeginMap;
		e << YAML::Key << "x" << YAML::Value << x;
		e << YAML::Key << "y" << YAML::Value << y;
		e << YAML::Key << "w" << YAML::Value << w;
		e << YAML::Key << "h" << YAML::Value << h;
		e << YAML::EndMap;
	};
	auto emitLayoutInt = [](YAML::Emitter& e, int x, int y, int w, int h) {
		e << YAML::Key << "layout" << YAML::Value << YAML::Flow << YAML::BeginMap;
		e << YAML::Key << "x" << YAML::Value << x;
		e << YAML::Key << "y" << YAML::Value << y;
		e << YAML::Key << "w" << YAML::Value << w;
		e << YAML::Key << "h" << YAML::Value << h;
		e << YAML::EndMap;
	};
	auto emitVisibleButtons2 = [](YAML::Emitter& e) {
		e << YAML::Key << "visible_buttons" << YAML::Value << YAML::Flow << YAML::BeginSeq << "softkey_left" << "softkey_right" << YAML::EndSeq;
	};
	auto emitBgItem = [](YAML::Emitter& e) {
		e << YAML::BeginMap;
		e << YAML::Key << "type" << YAML::Value << "background";
		e << YAML::Key << "image" << YAML::Value << "main_bg";
		e << YAML::Key << "x" << YAML::Value << 240;
		e << YAML::Key << "y" << YAML::Value << 160;
		e << YAML::Key << "anchor" << YAML::Value << "center";
		e << YAML::EndMap;
	};
	auto emitLogoItem = [](YAML::Emitter& e) {
		e << YAML::BeginMap;
		e << YAML::Key << "type" << YAML::Value << "image";
		e << YAML::Key << "image" << YAML::Value << "logo";
		e << YAML::Key << "x" << YAML::Value << "center";
		e << YAML::Key << "y" << YAML::Value << 0;
		e << YAML::EndMap;
	};

	out << YAML::Key << "menus" << YAML::Value << YAML::BeginMap;

	for (int i = 0; i < menuDataCount; i++) {
		uint32_t md = menuData[i];
		int mid = md & 0xFF;
		int signedId = (mid >= 250) ? (mid - 256) : mid;
		int itemEnd = (md & 0xFFFF00) >> 8;
		int mtype = (md & 0xFF000000) >> 24;
		int itemStart = (i > 0) ? ((menuData[i - 1] & 0xFFFF00) >> 8) : 0;
		int numItems = (itemEnd - itemStart) / 2;

		const char* mname = menuIdToName(signedId);
		std::string menuKey = mname ? mname : ("menu_" + std::to_string(signedId));
		out << YAML::Key << menuKey << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "menu_id" << YAML::Value << signedId;
		if (mtype < 10) {
			out << YAML::Key << "type" << YAML::Value << MENU_TYPE_NAMES[mtype];
		} else {
			out << YAML::Key << "type" << YAML::Value << mtype;
		}

		// --- Emit presentation properties based on menu name ---
		std::string mn(mname ? mname : "");
		bool needsBg = false, needsLogo = false;

		// Main menus with logo, background, visible_buttons [11]
		if (mn == "main" || mn == "main_more_games" || mn == "main_options" || mn == "select_language") {
			emitThemeMain(out);
			emitLayoutStr(out, "center", 126, "btn_width", 176);
			needsLogo = true; needsBg = true;
			out << YAML::Key << "visible_buttons" << YAML::Value << YAML::Flow << YAML::BeginSeq << "softkey_left" << YAML::EndSeq;
		} else if (mn == "main_help") {
			emitThemeMain(out);
			emitLayoutStr(out, "center", 126, "btn_width", 176);
			needsLogo = true; needsBg = true;
			out << YAML::Key << "load_start_item" << YAML::Value << 1;
			out << YAML::Key << "visible_buttons" << YAML::Value << YAML::Flow << YAML::BeginSeq << "softkey_left" << YAML::EndSeq;
		} else if (mn == "main_minigame" || mn == "main_difficulty") {
			emitThemeMain(out);
			emitLayoutStr(out, "center", 126, "btn_width", 176);
			needsLogo = true; needsBg = true;
			out << YAML::Key << "selected_index" << YAML::Value << 1;
			out << YAML::Key << "load_start_item" << YAML::Value << 1;
			out << YAML::Key << "visible_buttons" << YAML::Value << YAML::Flow << YAML::BeginSeq << "softkey_left" << YAML::EndSeq;
		}
		// Main YesNo menus
		else if (mn == "main_exit") {
			emitThemeMain(out);
			emitLayoutStr(out, "center", 126, "btn_width", 176);
			needsLogo = true; needsBg = true;
			out << YAML::Key << "yesno" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "string_id" << YAML::Value << 106;
			out << YAML::Key << "select_yes" << YAML::Value << false;
			out << YAML::Key << "yes_action" << YAML::Value << "exit";
			out << YAML::Key << "yes_param" << YAML::Value << 0;
			out << YAML::EndMap;
		} else if (mn == "main_confirmnew") {
			emitThemeMain(out);
			emitLayoutStr(out, "center", 126, "btn_width", 176);
			needsLogo = true; needsBg = true;
			out << YAML::Key << "yesno" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "string_id" << YAML::Value << 107;
			out << YAML::Key << "select_yes" << YAML::Value << false;
			out << YAML::Key << "yes_action" << YAML::Value << "goto";
			out << YAML::Key << "yes_param" << YAML::Value << 16;
			out << YAML::EndMap;
		} else if (mn == "main_confirmnew2") {
			emitThemeMain(out);
			emitLayoutStr(out, "center", 126, "btn_width", 176);
			needsBg = true;
			out << YAML::Key << "yesno" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "string_id" << YAML::Value << 108;
			out << YAML::Key << "select_yes" << YAML::Value << false;
			out << YAML::Key << "yes_action" << YAML::Value << "newgame";
			out << YAML::Key << "yes_param" << YAML::Value << 0;
			out << YAML::EndMap;
		} else if (mn == "enable_sounds") {
			emitThemeMain(out);
			emitLayoutStr(out, "center", 126, "btn_width", 176);
			needsLogo = true; needsBg = true;
			out << YAML::Key << "yesno" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "string_id" << YAML::Value << 168;
			out << YAML::Key << "select_yes" << YAML::Value << true;
			out << YAML::Key << "yes_action" << YAML::Value << "togsound";
			out << YAML::Key << "yes_param" << YAML::Value << 1;
			out << YAML::Key << "no_action" << YAML::Value << "togsound";
			out << YAML::Key << "no_param" << YAML::Value << 0;
			out << YAML::Key << "clear_stack" << YAML::Value << true;
			out << YAML::EndMap;
		}
		// end_ranking, end, end_finalquit
		else if (mn == "end_ranking") {
			emitThemeMain(out);
			emitLayoutInt(out, 23, 0, 407, 320);
		} else if (mn == "end" || mn == "end_finalquit") {
			emitThemeMain(out);
			emitLayoutStr(out, "center", 126, "btn_width", 176);
		}
		// Ingame menus
		else if (mn == "ingame") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
			out << YAML::Key << "selected_index" << YAML::Value << 1;
			emitVisibleButtons2(out);
			out << YAML::Key << "show_info_buttons" << YAML::Value << true;
		} else if (mn == "ingame_kicking") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
			emitVisibleButtons2(out);
			out << YAML::Key << "show_info_buttons" << YAML::Value << true;
		} else if (mn == "ingame_loadnosave") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
			out << YAML::Key << "scroll_index" << YAML::Value << 0;
		} else if (mn == "ingame_help" || mn == "ingame_status") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
			emitVisibleButtons2(out);
		} else if (mn == "ingame_player") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
			out << YAML::Key << "selected_index" << YAML::Value << 2;
			emitVisibleButtons2(out);
		} else if (mn == "ingame_level" || mn == "ingame_grades") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
			emitVisibleButtons2(out);
		} else if (mn == "level_stats") {
			emitThemeIngame(out);
			emitLayoutInt(out, 23, 0, 407, 320);
		} else if (mn == "ingame_exit" || mn == "ingame_dead") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
		} else if (mn == "ingame_options" || mn == "ingame_controls") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
			emitVisibleButtons2(out);
		}
		// Items menus
		else if (mn == "items" || mn == "items_weapons" || mn == "items_drinks") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
			emitVisibleButtons2(out);
			out << YAML::Key << "show_info_buttons" << YAML::Value << true;
		} else if (mn == "items_healthmsg" || mn == "items_armormsg" || mn == "items_syringemsg" || mn == "items_holy_water_max") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
		}
		// Vending machine menus
		else if (mn == "vending_machine" || mn == "vending_machine_drinks" || mn == "vending_machine_snacks" || mn == "vending_machine_confirm") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 11, 203, 250);
			emitVisibleButtons2(out);
			out << YAML::Key << "show_info_buttons" << YAML::Value << true;
		} else if (mn == "vending_machine_cant_buy") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 11, 203, 250);
			emitVisibleButtons2(out);
		}
		// Debug menus
		else if (mn == "debug" || mn == "debug_stats" || mn == "debug_sys") {
			emitThemeIngame(out);
			emitLayoutInt(out, 70, 10, 340, 241);
			emitVisibleButtons2(out);
		}

		if (numItems > 0 || needsBg || needsLogo) {
			out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;
			if (needsBg) emitBgItem(out);
			if (needsLogo) emitLogoItem(out);
			for (int j = 0; j < numItems; j++) {
				int idx = itemStart + j * 2;
				uint32_t w0 = menuItems[idx];
				uint32_t w1 = menuItems[idx + 1];

				uint16_t stringId = (w0 >> 16) & 0xFFFF;
				uint16_t flags = w0 & 0xFFFF;
				int action = (w1 >> 8) & 0xFF;
				int param = w1 & 0xFF;
				uint16_t helpString = (w1 >> 16) & 0xFFFF;

				out << YAML::BeginMap;
				out << YAML::Key << "string_id" << YAML::Value << (int)stringId;
				if (flags != 0) {
					out << YAML::Key << "flags" << YAML::Value << flagsToNames(flags);
				}
				if (action != 0) {
					out << YAML::Key << "action" << YAML::Value << actionName(action);
				}
				if (action == 1 && param != 0) {
					// goto action: output named target
					const char* targetName = menuIdToName(param);
					if (targetName) {
						out << YAML::Key << "goto" << YAML::Value << targetName;
					} else {
						out << YAML::Key << "param" << YAML::Value << param;
					}
				} else if (param != 0) {
					out << YAML::Key << "param" << YAML::Value << param;
				}
				if (helpString != 0) {
					out << YAML::Key << "help_string" << YAML::Value << (int)helpString;
				}
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}
		out << YAML::EndMap;
	}

	// --- Inject menus (not in binary data) ---

	// Main help injects
	struct HelpInject { const char* name; int id; int res; };
	HelpInject mainHelps[] = {
		{"main_general", 9, 1}, {"main_move", 10, 2}, {"main_attack", 11, 3},
		{"main_sniper", 12, 7}, {"main_armorhelp", 5, 4}, {"main_effecthelp", 6, 5},
		{"main_itemhelp", 7, 6}, {"main_about", 8, 11}, {"main_hacker_help", 20, 8},
		{"main_matrix_skip_help", 21, 9}, {"main_power_up_help", 22, 10},
	};
	for (auto& h : mainHelps) {
		out << YAML::Key << h.name << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "inject" << YAML::Value << true;
		out << YAML::Key << "menu_id" << YAML::Value << h.id;
		out << YAML::Key << "type" << YAML::Value << "help";
		emitThemeMain(out);
		emitLayoutInt(out, 85, 0, 300, 320);
		out << YAML::Key << "visible_buttons" << YAML::Value << YAML::Flow << YAML::BeginSeq << "softkey_left" << YAML::EndSeq;
		out << YAML::Key << "help_resource" << YAML::Value << h.res;
		out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;
		emitBgItem(out);
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	// Simple main injects
	struct SimpleInject { const char* name; int id; const char* type; };
	SimpleInject simpleMainInjects[] = {
		{"main_exit_injected", 13, "main"},
		{"main_confirmnew_injected", 14, "vcenter"},
		{"main_confirmnew2_injected", 15, "vcenter"},
		{"select_language_injected", 23, "vcenter"},
		{"end_ranking_injected", 24, "help"},
		{"enable_sounds_injected", 25, "main"},
	};
	for (auto& s : simpleMainInjects) {
		out << YAML::Key << s.name << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "inject" << YAML::Value << true;
		out << YAML::Key << "menu_id" << YAML::Value << s.id;
		out << YAML::Key << "type" << YAML::Value << s.type;
		emitThemeMain(out);
		out << YAML::EndMap;
	}

	// Ingame help injects
	HelpInject ingameHelps[] = {
		{"ingame_general", 38, 1}, {"ingame_move", 39, 2}, {"ingame_attack", 40, 3},
		{"ingame_sniper", 41, 7}, {"ingame_armorhelp", 43, 4}, {"ingame_effecthelp", 44, 5},
		{"ingame_itemhelp", 45, 6}, {"ingame_hacker_help", 59, 8},
		{"ingame_matrix_skip_help", 60, 9}, {"ingame_power_up_help", 61, 10},
	};
	for (auto& h : ingameHelps) {
		out << YAML::Key << h.name << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "inject" << YAML::Value << true;
		out << YAML::Key << "menu_id" << YAML::Value << h.id;
		out << YAML::Key << "type" << YAML::Value << "help";
		emitThemeIngame(out);
		emitLayoutInt(out, 23, 10, 434, 241);
		emitVisibleButtons2(out);
		out << YAML::Key << "help_resource" << YAML::Value << h.res;
		out << YAML::EndMap;
	}

	// ingame_questlog
	out << YAML::Key << "ingame_questlog" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << 46;
	out << YAML::Key << "type" << YAML::Value << "notebook";
	emitThemeIngame(out);
	emitLayoutInt(out, 23, 10, 434, 241);
	emitVisibleButtons2(out);
	out << YAML::EndMap;

	// Ingame yesno injects
	struct YesNoInject { const char* name; int id; int stringId; const char* action; };
	YesNoInject yesnoMenus[] = {
		{"ingame_load", 49, 136, "load"},
		{"ingame_restartlvl", 52, 134, "restartlevel"},
		{"ingame_savequit", 53, 135, "savequit"},
	};
	for (auto& yn : yesnoMenus) {
		out << YAML::Key << yn.name << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "inject" << YAML::Value << true;
		out << YAML::Key << "menu_id" << YAML::Value << yn.id;
		out << YAML::Key << "type" << YAML::Value << "vcenter";
		emitThemeIngame(out);
		emitLayoutInt(out, 70, 10, 340, 241);
		out << YAML::Key << "yesno" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "string_id" << YAML::Value << yn.stringId;
		out << YAML::Key << "select_yes" << YAML::Value << true;
		out << YAML::Key << "yes_action" << YAML::Value << yn.action;
		out << YAML::Key << "yes_param" << YAML::Value << 0;
		out << YAML::EndMap;
		out << YAML::EndMap;
	}

	// ingame_special_exit
	out << YAML::Key << "ingame_special_exit" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << 58;
	out << YAML::Key << "type" << YAML::Value << "vcenter";
	emitThemeIngame(out);
	emitVisibleButtons2(out);
	out << YAML::Key << "yesno" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "string_id" << YAML::Value << 137;
	out << YAML::Key << "select_yes" << YAML::Value << false;
	out << YAML::Key << "yes_action" << YAML::Value << "main_special";
	out << YAML::Key << "yes_param" << YAML::Value << 0;
	out << YAML::EndMap;
	out << YAML::EndMap;

	// Simple ingame injects
	struct TypeInject { const char* name; int id; const char* type; };
	TypeInject simpleIngameInjects[] = {
		{"ingame_recipes", 47, "list"},
		{"ingame_save", 48, "list"},
		{"showdetails", 71, "notebook"},
		{"vending_machine_details", 88, "vending_machine"},
		{"debug_maps", 66, "list"},
	};
	for (auto& t : simpleIngameInjects) {
		out << YAML::Key << t.name << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "inject" << YAML::Value << true;
		out << YAML::Key << "menu_id" << YAML::Value << t.id;
		out << YAML::Key << "type" << YAML::Value << t.type;
		emitThemeIngame(out);
		out << YAML::EndMap;
	}

	// comic_book
	out << YAML::Key << "comic_book" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << 89;
	out << YAML::Key << "type" << YAML::Value << "default";
	emitThemeMain(out);
	out << YAML::EndMap;

	// GEC extension injects (main screen)
	out << YAML::Key << "main_options_sound" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << -3;
	out << YAML::Key << "type" << YAML::Value << "main_list";
	emitThemeMain(out);
	emitLayoutStr(out, "center", 10, "btn_width", 300);
	out << YAML::Key << "item_width" << YAML::Value << 204;
	out << YAML::Key << "visible_buttons" << YAML::Value << YAML::Flow << YAML::BeginSeq << "softkey_left" << "sfx_volume_slider" << YAML::EndSeq;
	out << YAML::Key << "visible_buttons_conditional" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "music_volume_slider" << YAML::Value << "music_on";
	out << YAML::EndMap;
	out << YAML::Key << "vibration_y" << YAML::Value << 169;
	out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;
	emitBgItem(out);
	out << YAML::EndSeq;
	out << YAML::EndMap;

	out << YAML::Key << "main_options_video" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << -2;
	out << YAML::Key << "type" << YAML::Value << "main_list";
	emitThemeMainWide(out);
	emitLayoutStr(out, "center", 10, "btn_width", 300);
	out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;
	emitBgItem(out);
	out << YAML::EndSeq;
	out << YAML::EndMap;

	out << YAML::Key << "main_options_input" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << -1;
	out << YAML::Key << "type" << YAML::Value << "main_list";
	emitThemeMain(out);
	emitLayoutStr(out, "center", 10, "btn_width", 300);
	out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;
	emitBgItem(out);
	out << YAML::EndSeq;
	out << YAML::EndMap;

	out << YAML::Key << "main_controls" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << -4;
	out << YAML::Key << "type" << YAML::Value << "main_list";
	emitThemeMain(out);
	emitLayoutStr(out, "center", 10, "btn_width", 300);
	out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;
	emitBgItem(out);
	out << YAML::EndSeq;
	out << YAML::EndMap;

	out << YAML::Key << "main_bindings" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << -5;
	out << YAML::Key << "type" << YAML::Value << "main_list";
	emitThemeMainWide(out);
	emitLayoutStr(out, "center", 10, "btn_width", 300);
	out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;
	emitBgItem(out);
	out << YAML::EndSeq;
	out << YAML::EndMap;

	out << YAML::Key << "main_controller" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << -6;
	out << YAML::Key << "type" << YAML::Value << "main_list";
	emitThemeMain(out);
	emitLayoutStr(out, "center", 10, "btn_width", 300);
	out << YAML::Key << "item_width" << YAML::Value << 204;
	out << YAML::Key << "visible_buttons" << YAML::Value << YAML::Flow << YAML::BeginSeq << "softkey_left" << "vibration_slider" << "deadzone_slider" << YAML::EndSeq;
	out << YAML::Key << "vibration_y" << YAML::Value << 169;
	out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;
	emitBgItem(out);
	out << YAML::EndSeq;
	out << YAML::EndMap;

	// Ingame extension injects
	out << YAML::Key << "ingame_options_sound" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << 55;
	out << YAML::Key << "type" << YAML::Value << "list";
	emitThemeIngame(out);
	emitLayoutInt(out, 70, 10, 340, 241);
	out << YAML::Key << "item_width" << YAML::Value << 204;
	out << YAML::Key << "visible_buttons" << YAML::Value << YAML::Flow << YAML::BeginSeq << "softkey_left" << "sfx_volume_slider" << "softkey_right" << YAML::EndSeq;
	out << YAML::Key << "visible_buttons_conditional" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "music_volume_slider" << YAML::Value << "music_on";
	out << YAML::EndMap;
	out << YAML::Key << "vibration_y" << YAML::Value << 169;
	out << YAML::EndMap;

	struct IngameExt { const char* name; int id; };
	IngameExt ingameExts[] = {
		{"ingame_options_video", 56},
		{"ingame_options_input", 63},
		{"ingame_bindings", 54},
	};
	for (auto& ie : ingameExts) {
		out << YAML::Key << ie.name << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "inject" << YAML::Value << true;
		out << YAML::Key << "menu_id" << YAML::Value << ie.id;
		out << YAML::Key << "type" << YAML::Value << "list";
		emitThemeIngame(out);
		emitLayoutInt(out, 70, 10, 340, 241);
		emitVisibleButtons2(out);
		out << YAML::EndMap;
	}

	out << YAML::Key << "ingame_controller" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "inject" << YAML::Value << true;
	out << YAML::Key << "menu_id" << YAML::Value << 64;
	out << YAML::Key << "type" << YAML::Value << "list";
	emitThemeIngame(out);
	emitLayoutInt(out, 70, 10, 340, 241);
	out << YAML::Key << "item_width" << YAML::Value << 204;
	out << YAML::Key << "visible_buttons" << YAML::Value << YAML::Flow << YAML::BeginSeq << "softkey_left" << "softkey_right" << "vibration_slider" << "deadzone_slider" << YAML::EndSeq;
	out << YAML::Key << "vibration_y" << YAML::Value << 169;
	out << YAML::EndMap;

	out << YAML::EndMap;  // menus
	out << YAML::EndMap;  // root

	free(data);

	std::string path = outDir + "/menus.yaml";
	if (!writeString(path, out.c_str())) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s (%d menus)\n", path.c_str(), menuDataCount);
	return true;
}

// ========================================================================
// Conversion: strings.idx + strings00-02.bin -> strings.yaml
// ========================================================================
static bool convertStrings(ZipFile& zip, const std::string& outDir) {
	// Read strings.idx
	int idxSize = 0;
	std::string idxPath = std::string(PKG_PREFIX) + "strings.idx";
	uint8_t* idxData = zip.readZipFileEntry(idxPath.c_str(), &idxSize);
	if (!idxData) {
		fprintf(stderr, "  strings.idx not found in IPA\n");
		return false;
	}

	// Read chunk files (strings00.bin - strings02.bin)
	uint8_t* chunks[3] = {};
	int chunkSizes[3] = {};
	for (int i = 0; i < 3; i++) {
		char fname[32];
		snprintf(fname, sizeof(fname), "strings%02d.bin", i);
		std::string chunkPath = std::string(PKG_PREFIX) + fname;
		chunks[i] = zip.readZipFileEntry(chunkPath.c_str(), &chunkSizes[i]);
		if (chunks[i]) {
			printf("  %s: %d bytes\n", fname, chunkSizes[i]);
		}
	}

	// Parse file index (same logic as Resource::loadFileIndex)
	int totalEntries = readShort(idxData, 0);
	int arraySize = MAX_LANGUAGES * MAXTEXT * 3 + 10;
	std::vector<int> array(arraySize, 0);

	int off = 2;
	int n4 = 0;
	for (int i = 0; i < totalEntries; i++) {
		uint8_t fb = idxData[off];
		int32_t fi = readInt(idxData, off + 1);
		off += 5;

		if (fi != 0) {
			array[(n4 * 3) - 1] = fi - array[(n4 * 3) - 2];
		}
		if (fb != 0xFF) {
			array[(n4 * 3) + 0] = fb; // chunk file number
			array[(n4 * 3) + 1] = fi; // byte offset
			n4++;
		}
	}
	// Final sentinel
	int32_t fi = readInt(idxData, off + 1);
	array[(n4 * 3) - 1] = fi - array[(n4 * 3) - 2];

	printf("  strings.idx: %d index entries, %d real groups\n", totalEntries, n4);

	static const char* GROUP_DESCRIPTIONS[] = {
		"UI messages, combat text, system strings",
		"Entity/NPC names and descriptions",
		"File/story strings and mission updates",
		"Menu labels, button text, help screens",
		"Level 1 dialog and story text",
		"Level 2 dialog and story text",
		"Level 3 dialog and story text",
		"Level 4 dialog and story text",
		"Level 5 dialog and story text",
		"Level 6 dialog and story text",
		"Level 7 dialog and story text",
		"Level 8 dialog and story text",
		"Level 9 dialog and story text",
		"Test strings",
		"Translation overrides",
	};

	// Collect all strings per group per language
	struct GroupData {
		std::vector<std::string> strings[MAX_LANGUAGES];
	};
	GroupData groupData[MAXTEXT];

	int totalStrings = 0;
	for (int lang = 0; lang < MAX_LANGUAGES; lang++) {
		for (int grp = 0; grp < MAXTEXT; grp++) {
			int idxPos = (grp + lang * MAXTEXT) * 3;
			int chunkId = array[idxPos + 0];
			int byteOffset = array[idxPos + 1];
			int byteSize = array[idxPos + 2];

			if (byteSize <= 0)
				continue;
			if (!chunks[chunkId])
				continue;

			int pos = byteOffset;
			int endPos = byteOffset + byteSize;
			while (pos < endPos) {
				int nul = pos;
				while (nul < endPos && chunks[chunkId][nul] != 0)
					nul++;
				int len = nul - pos;
				groupData[grp].strings[lang].push_back(escapeString(chunks[chunkId] + pos, len));
				pos = nul + 1;
			}
			totalStrings += (int)groupData[grp].strings[lang].size();
		}
	}

	// Store English MenuStrings (group 3) for level name resolution
	if (!groupData[3].strings[0].empty()) {
		g_menuStrings = groupData[3].strings[0];
	}

	// Create per-level directories (needed before writing per-level strings)
	createLevelDirectories(outDir);

	// Create strings/ directory
	std::string stringsDir = outDir + "/strings";
#ifdef _WIN32
	_mkdir(stringsDir.c_str());
#else
	mkdir(stringsDir.c_str(), 0755);
#endif

	// Write one file per group
	for (int grp = 0; grp < MAXTEXT; grp++) {
		std::string yaml;
		yaml += "# " + std::string(GROUP_NAMES[grp]) + " (group " + std::to_string(grp) + ") — " + GROUP_DESCRIPTIONS[grp] + "\n";
		yaml += "# STRINGID(" + std::to_string(grp) + ", index)\n";
		yaml += "#\n";
		yaml += "# Special character escapes:\n";
		yaml += "#   \\x80 = line separator    \\x84 = cursor2      \\x85 = ellipsis\n";
		yaml += "#   \\x87 = checkmark         \\x88 = mini-dash    \\x89 = grey line\n";
		yaml += "#   \\x8A = cursor            \\x8B = shield       \\x8D = heart\n";
		yaml += "#   \\x90 = pointer           \\xA0 = hard space\n";
		yaml += "#   | = newline (in-game)     - = soft hyphen (word-break hint)\n";
		yaml += "\n";

		bool hasData = false;
		for (int lang = 0; lang < MAX_LANGUAGES; lang++) {
			if (!groupData[grp].strings[lang].empty()) {
				hasData = true;
				yaml += std::string(LANGUAGE_NAMES[lang]) + ":\n";
				for (const auto& s : groupData[grp].strings[lang]) {
					yaml += "  - \"" + s + "\"\n";
				}
			}
		}
		if (!hasData) {
			yaml += "english:\n  []\n";
		}

		std::string filePath;
		std::string printPath;
		if (grp >= 4 && grp <= 13) {
			// Per-level strings: group 4=map1, ..., 12=map9, 13=map10(test)
			int mapId = (grp == 13) ? 10 : (grp - 3);
			std::string dirName = getLevelDirName(mapId);
			filePath = outDir + "/levels/" + dirName + "/strings.yaml";
			printPath = "levels/" + dirName + "/strings.yaml";
		} else {
			filePath = stringsDir + "/" + GROUP_NAMES[grp] + ".yaml";
			printPath = "strings/" + std::string(GROUP_NAMES[grp]) + ".yaml";
		}
		if (!writeString(filePath, yaml)) {
			fprintf(stderr, "  Failed to write %s\n", filePath.c_str());
			return false;
		}
		int grpTotal = 0;
		for (int l = 0; l < MAX_LANGUAGES; l++) grpTotal += (int)groupData[grp].strings[l].size();
		printf("  -> %s (%d strings)\n", printPath.c_str(), grpTotal);
	}

	free(idxData);
	for (int i = 0; i < 3; i++) {
		if (chunks[i])
			free(chunks[i]);
	}

	printf("  -> strings/ (%d total strings)\n", totalStrings);
	return true;
}

// ========================================================================
// Conversion: sounds -> sounds.yaml + sounds2/ directory
// ========================================================================
static bool convertSounds(ZipFile& zip, const std::string& outDir) {
	// Create audio/ directory for sound assets
	std::string soundsDir = outDir + "/audio";
	if (!mkdirRecursive(soundsDir)) {
		fprintf(stderr, "  Failed to create %s\n", soundsDir.c_str());
		return false;
	}

	// Extract all WAV files from IPA and build sounds list
	std::vector<std::string> soundNames;
	int extracted = 0;

	for (int i = 0; i < NUM_DEFAULT_SOUNDS; i++) {
		std::string ipaPath = std::string(PKG_PREFIX) + "sounds2/" + DEFAULT_SOUND_NAMES[i];
		int size = 0;
		if (!zip.hasEntry(ipaPath.c_str())) {
			soundNames.push_back(DEFAULT_SOUND_NAMES[i]);
			continue;
		}
		uint8_t* data = zip.readZipFileEntry(ipaPath.c_str(), &size);
		if (data) {
			std::string outPath = soundsDir + "/" + DEFAULT_SOUND_NAMES[i];
			writeFile(outPath, data, size);
			free(data);
			extracted++;
		}
		soundNames.push_back(DEFAULT_SOUND_NAMES[i]);
	}

	printf("  Sounds: %d/%d WAV files extracted\n", extracted, NUM_DEFAULT_SOUNDS);

	// Write sounds.yaml with named entries
	YAML::Emitter out;
	out << YAML::Comment("Sound resource definitions - Named sound entries");
	out << YAML::Comment("Each sound has a name (lookup key) and file (WAV filename)");
	out << YAML::Comment("Index = position in array, ResID = index + 1000");
	out << YAML::Newline;
	out << YAML::BeginMap;
	out << YAML::Key << "sounds" << YAML::Value << YAML::BeginMap;
	for (const auto& fileName : soundNames) {
		// Generate name from filename: strip .wav, lowercase
		std::string sndName = fileName;
		size_t dotPos = sndName.rfind('.');
		if (dotPos != std::string::npos) {
			sndName = sndName.substr(0, dotPos);
		}
		std::transform(sndName.begin(), sndName.end(), sndName.begin(), ::tolower);

		out << YAML::Key << sndName << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "file" << YAML::Value << fileName;
		out << YAML::EndMap;
	}
	out << YAML::EndMap;

	// UI sound aliases (referenced by ui.yaml button definitions)
	out << YAML::Newline;
	out << YAML::Comment("UI sound aliases - maps descriptive names to sound entries above");
	out << YAML::Key << "ui_sounds" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "menu_highlight" << YAML::Value << "dialog_help";
	out << YAML::Key << "softkey_click" << YAML::Value << "switch_exit";
	out << YAML::Key << "scroll" << YAML::Value << "menu_scroll";
	out << YAML::EndMap;

	out << YAML::EndMap;

	std::string path = outDir + "/sounds.yaml";
	if (!writeString(path, out.c_str())) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s (%d sounds)\n", path.c_str(), (int)soundNames.size());
	return true;
}

// ========================================================================
// Generate game.yaml
// ========================================================================
static bool generateGameYaml(const std::string& outDir) {
	std::string yaml;
	yaml += "# Doom II RPG - Game Definition\n";
	yaml += "# This is the default game that ships with the engine.\n";
	yaml += "\n";
	yaml += "game:\n";
	yaml += "  name: Doom II RPG\n";
	yaml += "  description: The classic Doom II RPG, reverse-engineered and enhanced\n";
	yaml += "  version: \"1.0\"\n";
	yaml += "\n";
	yaml += "  # Window title (defaults to name if not set)\n";
	yaml += "  window_title: Doom II RPG By [GEC]\n";
	yaml += "\n";
	yaml += "  # Save directory for config/profile data\n";
	yaml += "  save_dir: Doom2rpg.app\n";
	yaml += "\n";
	yaml += "  # Entry map (first map loaded on new game)\n";
	yaml += "  entry_map: map00\n";
	yaml += "\n";
	yaml += "  # Per-level directory mappings (map_id -> directory path)\n";
	yaml += "  levels:\n";
	for (int mapId = 1; mapId <= 10; mapId++) {
		yaml += "    " + std::to_string(mapId) + ": levels/" + getLevelDirName(mapId) + "\n";
	}
	yaml += "\n";
	yaml += "  # String tables — maps group index to YAML file path.\n";
	yaml += "  # Modders can add/reorder groups or point to custom files.\n";
	yaml += "  strings:\n";
	for (int g = 0; g < MAXTEXT; g++) {
		if (g >= 4 && g <= 13) {
			// Per-level strings: group 4=map1, ..., 12=map9, 13=map10(test)
			int mapId = (g == 13) ? 10 : (g - 3);
			yaml += "    " + std::to_string(g) + ": levels/" + getLevelDirName(mapId) + "/strings.yaml\n";
		} else {
			yaml += "    " + std::to_string(g) + ": strings/" + GROUP_NAMES[g] + ".yaml\n";
		}
	}
	yaml += "\n";
	yaml += "  # Subdirectories to search when resolving asset files.\n";
	yaml += "  # The engine tries the exact path first, then searches these dirs in order.\n";
	yaml += "  search_dirs:\n";
	yaml += "    - config\n";
	yaml += "    - ui\n";
	yaml += "    - hud\n";
	yaml += "    - fonts\n";
	yaml += "    - levels/textures\n";
	yaml += "    - audio\n";
	yaml += "    - comicbook\n";
	yaml += "    - data\n";
	yaml += "\n";
	yaml += "  # Player configuration\n";
	yaml += "  player:\n";
	yaml += "    starting_max_health: 100\n";
	yaml += "\n";
	yaml += "    # Level-up stat grants\n";
	yaml += "    level_up:\n";
	yaml += "      health: 10\n";
	yaml += "      defense: 1\n";
	yaml += "      strength: 2\n";
	yaml += "      accuracy: 1\n";
	yaml += "      agility: 3\n";
	yaml += "\n";
	yaml += "    # Out-of-combat cooldown (turns since last combat)\n";
	yaml += "    out_of_combat_turns: 4\n";
	yaml += "\n";
	yaml += "    # XP formula coefficients: linear*n + cubic*((n-1)^3 + (n-1))\n";
	yaml += "    xp_formula:\n";
	yaml += "      linear: 500\n";
	yaml += "      cubic: 100\n";
	yaml += "\n";
	yaml += "    # Inventory/ammo capacity caps\n";
	yaml += "    caps:\n";
	yaml += "      credits: 9999\n";
	yaml += "      inventory: 999\n";
	yaml += "      ammo: 100\n";
	yaml += "      bot_fuel: 5\n";
	yaml += "\n";
	yaml += "  # Scoring formula constants\n";
	yaml += "  scoring:\n";
	yaml += "    per_level: 1000\n";
	yaml += "    all_levels_bonus: 1000\n";
	yaml += "    no_deaths_bonus: 1000\n";
	yaml += "    death_penalty_base: 5\n";
	yaml += "    death_penalty_mult: 50\n";
	yaml += "    many_deaths_penalty: 250\n";
	yaml += "    death_threshold: 10\n";
	yaml += "    time_bonus_minutes: 120\n";
	yaml += "    time_bonus_mult: 15\n";
	yaml += "    move_threshold: 5000\n";
	yaml += "    move_divisor: 2\n";
	yaml += "    per_secret: 1000\n";
	yaml += "    all_secrets_bonus: 1000\n";
	yaml += "\n";
	yaml += "  # Render mode name -> ID lookup\n";
	yaml += "  render_modes:\n";
	yaml += "    normal: 0\n";
	yaml += "    blend25: 1\n";
	yaml += "    blend50: 2\n";
	yaml += "    add: 3\n";
	yaml += "    add75: 4\n";
	yaml += "    add50: 5\n";
	yaml += "    add25: 6\n";
	yaml += "    sub: 7\n";
	yaml += "    perf: 9\n";
	yaml += "    none: 10\n";
	yaml += "    blend75: 12\n";
	yaml += "    blend_special_alpha: 13\n";
	yaml += "\n";
	yaml += "# Config enum name/value mappings (used by settings UI)\n";
	yaml += "config_enums:\n";
	yaml += "  difficulty:\n";
	yaml += "    baby: 0\n";
	yaml += "    normal: 1\n";
	yaml += "    hard: 2\n";
	yaml += "    nightmare: 3\n";
	yaml += "  language:\n";
	yaml += "    english: 0\n";
	yaml += "    french: 1\n";
	yaml += "    german: 2\n";
	yaml += "    italian: 3\n";
	yaml += "    spanish: 4\n";
	yaml += "  window_mode:\n";
	yaml += "    windowed: 0\n";
	yaml += "    fullscreen: 1\n";
	yaml += "    borderless: 2\n";
	yaml += "  control_layout:\n";
	yaml += "    chevrons: 0\n";
	yaml += "    dpad: 1\n";

	std::string path = outDir + "/game.yaml";
	if (!writeString(path, yaml)) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s\n", path.c_str());
	return true;
}

// ========================================================================
// Generate ui.yaml
// ========================================================================
static bool generateUIYaml(const std::string& outDir) {
	std::string yaml;
	yaml += "# UI element definitions for DoomIIRPG\n";
	yaml += "# Describes all UI widget types, button instances, and their properties\n";
	yaml += "\n";
	yaml += "# Scrollbar config\n";
	yaml += "scrollbar:\n";
	yaml += "  sound: scroll\n";
	yaml += "  vertical: true\n";
	yaml += "\n";
	yaml += "# Screen layouts — button containers (menu | info | vending)\n";
	yaml += "screens:\n";
	yaml += "  menu:\n";
	yaml += "    menu_items:\n";
	yaml += "      image: menu_btn_bg\n";
	yaml += "      image_highlight: menu_btn_bg_on\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      render_mode: 1\n";
	yaml += "      highlight_render_mode: 0\n";
	yaml += "      id_range: [0, 8]\n";
	yaml += "      x: 159\n";
	yaml += "      start_y: 139\n";
	yaml += "      step_y: 46\n";
	yaml += "      width: 162\n";
	yaml += "      height: 46\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "    arrow_up:\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      size_from_image: true\n";
	yaml += "      id: 9\n";
	yaml += "      x: 331\n";
	yaml += "      y: image_relative\n";
	yaml += "      y_base: 190\n";
	yaml += "      y_offset: -height\n";
	yaml += "      image: menu_arrow_up\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "    arrow_down:\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      size_from_image: true\n";
	yaml += "      id: 10\n";
	yaml += "      x: 331\n";
	yaml += "      y: 210\n";
	yaml += "      image: menu_arrow_down\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "    softkey_left:\n";
	yaml += "      sound: softkey_click\n";
	yaml += "      id: 11\n";
	yaml += "      x: 0\n";
	yaml += "      y: 256\n";
	yaml += "      width: 52\n";
	yaml += "      height: 64\n";
	yaml += "      image: switch_left_normal\n";
	yaml += "      image_highlight: switch_left_active\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "    sfx_volume_slider:\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      id: 12\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "    music_volume_slider:\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      id: 13\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "    alpha_slider:\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      id: 14\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "    softkey_right:\n";
	yaml += "      sound: softkey_click\n";
	yaml += "      id: 15\n";
	yaml += "      x: 428\n";
	yaml += "      y: 256\n";
	yaml += "      width: 52\n";
	yaml += "      height: 64\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "    # Controller sliders [GEC]\n";
	yaml += "    vibration_slider:\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      id: 16\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "    deadzone_slider:\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      id: 17\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "  info:\n";
	yaml += "    info_items:\n";
	yaml += "      image: info_btn_normal\n";
	yaml += "      image_highlight: info_btn_pressed\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      render_mode: 1\n";
	yaml += "      highlight_render_mode: 0\n";
	yaml += "      id_range: [0, 8]\n";
	yaml += "      visible: false\n";
	yaml += "\n";
	yaml += "  vending:\n";
	yaml += "    arrow_up:\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      size_from_image: true\n";
	yaml += "      render_mode: 1\n";
	yaml += "      highlight_render_mode: 0\n";
	yaml += "      id: 0\n";
	yaml += "      x: 320\n";
	yaml += "      y: 130\n";
	yaml += "      image: vending_arrow_up_glow\n";
	yaml += "      visible: true\n";
	yaml += "\n";
	yaml += "    arrow_down:\n";
	yaml += "      sound: menu_highlight\n";
	yaml += "      size_from_image: true\n";
	yaml += "      render_mode: 1\n";
	yaml += "      highlight_render_mode: 0\n";
	yaml += "      id: 1\n";
	yaml += "      x: 320\n";
	yaml += "      y: image_relative\n";
	yaml += "      y_base: 150\n";
	yaml += "      y_offset: menu_arrow_up.height\n";
	yaml += "      image: vending_arrow_down_glow\n";
	yaml += "      visible: true\n";
	yaml += "\n";
	// menu_presentation data is now emitted directly in convertMenus() within menus.yaml


	std::string path = outDir + "/ui.yaml";
	if (!writeString(path, yaml)) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s\n", path.c_str());
	return true;
}

// ========================================================================
// Generate characters.yaml
// ========================================================================
static bool generateCharactersYaml(const std::string& outDir) {
	std::string yaml;
	yaml += "# Character definitions\n";
	yaml += "# characterChoice: 1=Major, 2=Sarge, 3=Scientist\n";
	yaml += "\n";
	yaml += "characters:\n";
	yaml += "  Major:\n";
	yaml += "    defense: 8\n";
	yaml += "    strength: 9\n";
	yaml += "    accuracy: 97\n";
	yaml += "    agility: 12\n";
	yaml += "    iq: 110\n";
	yaml += "    credits: 30\n";
	yaml += "\n";
	yaml += "  Sarge:\n";
	yaml += "    defense: 12\n";
	yaml += "    strength: 14\n";
	yaml += "    accuracy: 92\n";
	yaml += "    agility: 6\n";
	yaml += "    iq: 100\n";
	yaml += "    credits: 10\n";
	yaml += "\n";
	yaml += "  Scientist:\n";
	yaml += "    defense: 8\n";
	yaml += "    strength: 8\n";
	yaml += "    accuracy: 87\n";
	yaml += "    agility: 6\n";
	yaml += "    iq: 150\n";
	yaml += "    credits: 80\n";

	std::string path = outDir + "/characters.yaml";
	if (!writeString(path, yaml)) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s\n", path.c_str());
	return true;
}

// ========================================================================
// Generate per-level level.yaml files
// ========================================================================
static bool generateLevelConfigs(const std::string& outDir) {
	// Hardcoded loadout data from the original game binary.
	// Each entry represents the starting equipment when entering a map.
	struct LoadoutEntry {
		int map;
		std::vector<int> weapons;           // weapon indices (give with amount=1)
		std::vector<std::pair<int,int>> weapon_ammo; // weapon index, amount
		int armor;
		std::vector<std::pair<int,int>> inventory;   // inv index, amount
		std::vector<std::pair<int,int>> ammo;        // ammo type, amount
		int xp;
		int defense, strength, accuracy, agility, iq;
		int vios_mallocs;
	};

	// Inventory index → name
	auto invName = [](int idx) -> std::string {
		switch (idx) {
			case 11: return "armor_large";
			case 12: return "armor_small";
			case 16: return "health_ration_bar";
			case 17: return "health_pack";
			case 24: return "uac_credit";
			default: return std::to_string(idx);
		}
	};

	// Ammo index → name
	auto ammoName = [](int idx) -> std::string {
		switch (idx) {
			case 1: return "bullets";
			case 2: return "shells";
			case 3: return "holy_water";
			case 4: return "cells";
			case 5: return "rockets";
			case 6: return "soul_cube";
			case 7: return "sentry_bot";
			default: return std::to_string(idx);
		}
	};

	LoadoutEntry entries[] = {
		{2, {1,0}, {}, 33, {{17,23},{11,4},{12,3},{24,4}}, {{1,100}}, 638, 4,2,11,0,4, 0},
		{3, {2,1,0}, {{3,100}}, 4, {{17,34},{11,12},{12,6},{24,39}}, {{1,100},{3,90}}, 1519, 13,5,10,6,10, 1},
		{4, {7,2,1,0}, {{4,100}}, 4, {{17,44},{11,5},{12,26},{24,59}}, {{1,73},{3,30},{2,24}}, 2622, 14,10,9,14,20, 1},
		{5, {9,8,7,2,1,0}, {{5,100}}, 34, {{17,79},{16,31},{11,9},{12,40},{24,73}}, {{1,84},{3,85},{2,88},{4,55}}, 3594, 14,12,9,18,28, 2},
		{6, {10,9,8,7,2,1,0}, {{5,100}}, 24, {{17,116},{16,54},{11,22},{12,2},{24,101}}, {{1,88},{3,60},{2,100},{4,100},{5,20}}, 4749, 24,13,9,18,34, 3},
		{7, {11,10,9,8,7,2,1,0}, {{5,100}}, 44, {{17,132},{16,72},{11,23},{12,12},{24,168}}, {{1,70},{3,40},{2,66},{4,60},{5,63}}, 5971, 25,14,8,18,40, 3},
		{8, {12,11,10,9,8,7,2,1,0}, {{5,100}}, 38, {{17,146},{16,94},{11,27},{12,20},{24,168}}, {{1,89},{3,25},{2,94},{4,52},{5,100}}, 7198, 27,16,8,22,46, 4},
		{9, {13,12,11,10,9,8,7,2,1,0}, {{5,100}}, 27, {{17,160},{16,90},{11,23},{12,28},{24,174}}, {{1,29},{3,8},{2,44},{4,55},{5,90},{6,5}}, 8281, 27,17,8,22,46, 4},
	};

	// Joke items dropped by corpses on each map
	struct JokeEntry { int map; std::vector<int> items; };
	JokeEntry jokeEntries[] = {
		{1, {180, 181, 182, 183, 184}},
		{2, {149, 150, 151, 152, 153}},
		{3, {130, 131, 132, 133, 134}},
		{4, {129, 130, 131, 132, 133}},
		{5, {131, 132, 133, 134, 135}},
		{6, {78, 79, 80, 81, 82}},
		{7, {24, 25, 26, 27, 28}},
		{8, {34, 35, 36, 37, 38}},
		{9, {15, 16, 17, 18, 19}},
		{10, {9, 10, 11, 12, 13}},
	};
	std::map<int, std::vector<int>> jokeMap;
	for (const auto& je : jokeEntries) {
		jokeMap[je.map] = je.items;
	}

	// Build loadout lookup by map ID
	std::map<int, const LoadoutEntry*> loadoutMap;
	for (const auto& e : entries) {
		loadoutMap[e.map] = &e;
	}

	// Generate one level.yaml per level directory
	for (int mapId = 1; mapId <= 10; mapId++) {
		std::string dirName = getLevelDirName(mapId);
		std::string yaml;
		yaml += "# Level configuration for " + levelNameFromId(mapId) + "\n\n";
		yaml += "name: " + levelNameFromId(mapId) + "\n";
		yaml += "map_id: " + std::to_string(mapId) + "\n";

		// Fog disabled on map 2 (outdoor map)
		if (mapId == 2) {
			yaml += "fog: false\n";
		}

		// Joke items
		auto jit = jokeMap.find(mapId);
		if (jit != jokeMap.end()) {
			yaml += "joke_items: [";
			for (size_t i = 0; i < jit->second.size(); i++) {
				if (i > 0) yaml += ", ";
				yaml += std::to_string(jit->second[i]);
			}
			yaml += "]\n";
		}

		// Starting loadout (only some levels have one)
		auto lit = loadoutMap.find(mapId);
		if (lit != loadoutMap.end()) {
			const LoadoutEntry& e = *lit->second;

			yaml += "starting_loadout:\n";

			// Weapons
			yaml += "  weapons: [";
			for (size_t i = 0; i < e.weapons.size(); i++) {
				if (i > 0) yaml += ", ";
				yaml += weaponName(e.weapons[i]);
			}
			yaml += "]\n";

			// Weapon ammo
			if (!e.weapon_ammo.empty()) {
				yaml += "  weapon_ammo:\n";
				for (const auto& wa : e.weapon_ammo)
					yaml += "    " + weaponName(wa.first) + ": " + std::to_string(wa.second) + "\n";
			}

			// Armor
			yaml += "  armor: " + std::to_string(e.armor) + "\n";

			// Inventory
			yaml += "  inventory:\n";
			for (const auto& inv : e.inventory)
				yaml += "    " + invName(inv.first) + ": " + std::to_string(inv.second) + "\n";

			// Ammo
			yaml += "  ammo:\n";
			for (const auto& a : e.ammo)
				yaml += "    " + ammoName(a.first) + ": " + std::to_string(a.second) + "\n";

			// XP
			yaml += "  xp: " + std::to_string(e.xp) + "\n";

			// Stats
			yaml += "  stats:\n";
			yaml += "    defense: " + std::to_string(e.defense) + "\n";
			yaml += "    strength: " + std::to_string(e.strength) + "\n";
			yaml += "    accuracy: " + std::to_string(e.accuracy) + "\n";
			if (e.agility > 0)
				yaml += "    agility: " + std::to_string(e.agility) + "\n";
			yaml += "    iq: " + std::to_string(e.iq) + "\n";

			// VIOS mallocs
			yaml += "  vios_mallocs: " + std::to_string(e.vios_mallocs) + "\n";
		}

		std::string path = outDir + "/levels/" + dirName + "/level.yaml";
		if (!writeString(path, yaml)) {
			fprintf(stderr, "  Failed to write %s\n", path.c_str());
			return false;
		}
		printf("  -> levels/%s/level.yaml\n", dirName.c_str());
	}

	return true;
}

// ========================================================================
// Copy all BMP image files from the IPA Packages directory
// ========================================================================
// Determine the output subdirectory for a BMP file based on its filename
static std::string classifyImage(const std::string& relPath) {
	// ComicBook images -> comicbook/
	if (relPath.compare(0, 10, "ComicBook/") == 0) {
		return "comicbook/" + relPath.substr(10);
	}

	// Extract just the filename for classification
	std::string name = relPath;
	size_t slash = name.rfind('/');
	if (slash != std::string::npos) {
		name = name.substr(slash + 1);
	}

	// Fonts
	if (name.compare(0, 4, "Font") == 0 || name == "WarFont.bmp") {
		return "fonts/" + name;
	}

	// HUD elements
	if (name.compare(0, 3, "Hud") == 0 || name.compare(0, 3, "HUD") == 0 ||
	    name.compare(0, 3, "hud") == 0 ||
	    name == "cockpit.bmp" || name == "damage.bmp" || name == "damage_bot.bmp" ||
	    name == "Automap_Cursor.bmp" || name == "ui_images.bmp" ||
	    name == "DialogScroll.bmp" ||
	    name.compare(0, 6, "Switch") == 0 ||
	    name == "Soft_Key_Fill.bmp" ||
	    name == "scope.bmp" || name.compare(0, 11, "SniperScope") == 0 ||
	    name == "inGame_menu_softkey.bmp" ||
	    name.compare(0, 15, "vending_softkey") == 0) {
		return "hud/" + name;
	}

	// Everything else goes to ui/
	return "ui/" + name;
}

static void copyImageAssets(ZipFile& zip, const std::string& outDir) {
	const std::string prefix = std::string(PKG_PREFIX);
	int copied = 0;

	for (int i = 0; i < zip.getEntryCount(); i++) {
		const char* name = zip.getEntryName(i);
		if (!name) continue;
		std::string entry(name);

		// Must be under the Packages prefix
		if (entry.compare(0, prefix.size(), prefix) != 0) continue;

		// Must end with .bmp (case-insensitive check)
		if (entry.size() < 4) continue;
		std::string ext = entry.substr(entry.size() - 4);
		if (ext != ".bmp" && ext != ".BMP") continue;

		// Strip prefix to get relative path (e.g. "logo.bmp" or "ComicBook/Cover.bmp")
		std::string relPath = entry.substr(prefix.size());

		// Classify into subdirectory
		std::string classifiedPath = classifyImage(relPath);

		int size = 0;
		uint8_t* data = zip.readZipFileEntry(name, &size);
		if (data) {
			std::string outPath = outDir + "/" + classifiedPath;
			size_t slash = outPath.rfind('/');
			if (slash != std::string::npos) {
				mkdirRecursive(outPath.substr(0, slash));
			}
			writeFile(outPath, data, size);
			free(data);
			copied++;
		}
	}
	printf("  Image assets: %d BMP files copied\n", copied);
}

// ========================================================================
// Copy binary assets that aren't converted yet (maps, textures, models)
// ========================================================================
static void copyBinaryAssets(ZipFile& zip, const std::string& outDir) {
	// Copy binary resource files that the engine loads directly via VFS
	const char* binaryFiles[] = {
	    // Maps
	    "Packages/map00.bin",
	    "Packages/map01.bin",
	    "Packages/map02.bin",
	    "Packages/map03.bin",
	    "Packages/map04.bin",
	    "Packages/map05.bin",
	    "Packages/map06.bin",
	    "Packages/map07.bin",
	    "Packages/map08.bin",
	    "Packages/map09.bin",
	    // Models
	    "Packages/model_0000.bin",
	    "Packages/model_0001.bin",
	    "Packages/model_0002.bin",
	    "Packages/model_0003.bin",
	    "Packages/model_0004.bin",
	    "Packages/model_0005.bin",
	    "Packages/model_0006.bin",
	    "Packages/model_0007.bin",
	    "Packages/model_0008.bin",
	    "Packages/model_0009.bin",
	    // Textures
	    "Packages/newMappings.bin",
	    "Packages/newPalettes.bin",
	    "Packages/newTexels000.bin",
	    "Packages/newTexels001.bin",
	    "Packages/newTexels002.bin",
	    "Packages/newTexels003.bin",
	    "Packages/newTexels004.bin",
	    "Packages/newTexels005.bin",
	    "Packages/newTexels006.bin",
	    "Packages/newTexels007.bin",
	    "Packages/newTexels008.bin",
	    "Packages/newTexels009.bin",
	    "Packages/newTexels010.bin",
	    "Packages/newTexels011.bin",
	    "Packages/newTexels012.bin",
	    "Packages/newTexels013.bin",
	    "Packages/newTexels014.bin",
	    "Packages/newTexels015.bin",
	    "Packages/newTexels016.bin",
	    "Packages/newTexels017.bin",
	    "Packages/newTexels018.bin",
	    "Packages/newTexels019.bin",
	    "Packages/newTexels020.bin",
	    "Packages/newTexels021.bin",
	    "Packages/newTexels022.bin",
	    "Packages/newTexels023.bin",
	    "Packages/newTexels024.bin",
	    "Packages/newTexels025.bin",
	    "Packages/newTexels026.bin",
	    "Packages/newTexels027.bin",
	    "Packages/newTexels028.bin",
	    "Packages/newTexels029.bin",
	    "Packages/newTexels030.bin",
	    "Packages/newTexels031.bin",
	    "Packages/newTexels032.bin",
	    "Packages/newTexels033.bin",
	    "Packages/newTexels034.bin",
	    "Packages/newTexels035.bin",
	    "Packages/newTexels036.bin",
	    "Packages/newTexels037.bin",
	    "Packages/newTexels038.bin",
	    // Binary data (sky/camera tables still loaded at runtime)
	    "Packages/tables.bin",
	};

	int copied = 0;
	for (const char* relPath : binaryFiles) {
		std::string ipaPath = std::string(IPA_PREFIX) + relPath;
		int size = 0;
		if (!zip.hasEntry(ipaPath.c_str()))
			continue;
		uint8_t* data = zip.readZipFileEntry(ipaPath.c_str(), &size);
		if (data) {
			// Strip "Packages/" prefix and route to domain subdirectory
			std::string outFile = relPath;
			if (outFile.compare(0, 9, "Packages/") == 0) {
				outFile = outFile.substr(9);
			}

			// Classify binary assets into subdirectories
			if (outFile.compare(0, 3, "map") == 0 && outFile.find(".bin") != std::string::npos) {
				// map00.bin -> levels/01_tycho/map.bin
				int mapIdx = 0;
				if (sscanf(outFile.c_str(), "map%d.bin", &mapIdx) == 1) {
					int mapId = mapIdx + 1;
					outFile = "levels/" + getLevelDirName(mapId) + "/map.bin";
				}
			} else if (outFile.compare(0, 6, "model_") == 0) {
				// model_0000.bin -> levels/01_tycho/model.bin
				int modelIdx = 0;
				if (sscanf(outFile.c_str(), "model_%d.bin", &modelIdx) == 1) {
					int mapId = modelIdx + 1;
					outFile = "levels/" + getLevelDirName(mapId) + "/model.bin";
				}
			} else if (outFile.compare(0, 3, "new") == 0) {
				outFile = "levels/textures/" + outFile;
			} else if (outFile == "tables.bin") {
				outFile = "data/" + outFile;
			}

			std::string outPath = outDir + "/" + outFile;

			// Ensure parent dir exists
			size_t slash = outPath.rfind('/');
			if (slash != std::string::npos) {
				mkdirRecursive(outPath.substr(0, slash));
			}

			writeFile(outPath, data, size);
			free(data);
			copied++;
		}
	}
	printf("  Binary assets: %d files copied\n", copied);
}

// ========================================================================
// Main
// ========================================================================
int main(int argc, char* argv[]) {
	const char* ipaPath = nullptr;
	std::string outputDir = "games/doom2rpg";
	bool force = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--ipa") == 0 && i + 1 < argc) {
			ipaPath = argv[++i];
		} else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
			outputDir = argv[++i];
		} else if (strcmp(argv[i], "--force") == 0 || strcmp(argv[i], "-f") == 0) {
			force = true;
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			printUsage(argv[0]);
			return 0;
		}
	}

	if (!ipaPath) {
		fprintf(stderr, "Error: --ipa <path> is required\n\n");
		printUsage(argv[0]);
		return 1;
	}

	// Check if output already exists
	if (dirExists(outputDir.c_str()) && !force) {
		fprintf(stderr, "Output directory '%s' already exists. Use --force to overwrite.\n", outputDir.c_str());
		return 1;
	}

	// Open IPA
	printf("Opening IPA: %s\n", ipaPath);
	ZipFile zip;
	zip.openZipFile(ipaPath);

	// Create output directory structure
	printf("Creating output directory: %s\n", outputDir.c_str());
	if (!mkdirRecursive(outputDir)) {
		fprintf(stderr, "Error: failed to create directory '%s'\n", outputDir.c_str());
		return 1;
	}

	bool ok = true;

	// Phase 1: Convert text-based assets to YAML
	printf("\n[Phase 1] Converting text assets to YAML...\n");

	printf("Converting entities...\n");
	ok &= convertEntities(zip, outputDir);

	printf("Converting tables...\n");
	ok &= convertTables(zip, outputDir);

	printf("Generating sprites.yaml...\n");
	ok &= generateAnimationsYaml(outputDir);

	printf("Generating projectiles.yaml...\n");
	ok &= generateProjectilesYaml(outputDir);

	printf("Generating effects.yaml...\n");
	ok &= generateEffectsYaml(outputDir);

	printf("Converting menus...\n");
	ok &= convertMenus(zip, outputDir);

	printf("Converting strings...\n");
	ok &= convertStrings(zip, outputDir);

	printf("Converting sounds...\n");
	ok &= convertSounds(zip, outputDir);

	printf("Generating game.yaml...\n");
	ok &= generateGameYaml(outputDir);

	printf("Generating ui.yaml...\n");
	ok &= generateUIYaml(outputDir);

	printf("Generating characters.yaml...\n");
	ok &= generateCharactersYaml(outputDir);

	printf("Generating per-level configs...\n");
	ok &= generateLevelConfigs(outputDir);

	// Copy binary assets that still need the original format
	printf("\nCopying binary assets (maps, textures, models)...\n");
	copyBinaryAssets(zip, outputDir);

	// Copy all BMP images from the IPA
	printf("\nCopying image assets...\n");
	copyImageAssets(zip, outputDir);

	zip.closeZipFile();

	if (ok) {
		printf("\nConversion complete: %s\n", outputDir.c_str());
		printf("Run the game with: ./DoomIIRPG --game doom2rpg\n");
	} else {
		printf("\nConversion completed with errors.\n");
	}

	return ok ? 0 : 1;
}
