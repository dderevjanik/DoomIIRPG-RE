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
	if (eType == 6) { // item
		if (eSubType == 1 && parm >= 0 && parm < NUM_PLAYER_WEAPONS) // weapon
			return WeaponNames::TABLE[parm];
		if (eSubType == 2 && parm >= 0 && parm < NUM_AMMO_PARMS) // ammo
			return AMMO_PARM_NAMES[parm];
	}
	return std::to_string(parm);
}

static std::string generateEntityName(int eType, int eSubType, int parm, const char* typeName, std::map<std::string, int>& nameCount) {
	std::string base;
	switch (eType) {
	case 2: // monster
		base = entitySubtypeName(eType, eSubType);
		break;
	case 9: // corpse (subtypes are monster indices, prefix with type name)
		base = std::string(typeName) + "_" + entitySubtypeName(eType, eSubType);
		break;
	case 6: { // item
		std::string sub = entitySubtypeName(eType, eSubType);
		std::string p = entityParmName(eType, eSubType, parm);
		if (eSubType == 1 || eSubType == 2) // weapon or ammo - parm is the name
			base = p;
		else
			base = sub + "_" + p;
		break;
	}
	case 5: { // door
		std::string sub = entitySubtypeName(eType, eSubType);
		base = std::string(typeName) + "_" + sub;
		break;
	}
	default:
		base = std::string(typeName) + "_" + std::to_string(eSubType);
		break;
	}

	// Add parm suffix for monsters and corpses
	if (eType == 2 || eType == 9)
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
	if (eType == 2) { // monster
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
	} else if (eType == 3) { // NPC
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
	if (eType == 3) return true; // NPC always gets body_parts
	if (eType != 2) return false; // Only monsters otherwise

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
	out << YAML::BeginMap;
	out << YAML::Key << "entities" << YAML::Value;
	out << YAML::BeginSeq;

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

	// Second pass: emit YAML
	for (int i = 0; i < count; i++) {
		auto& e = entities[i];
		std::string typeName = entityTypeName(e.eType);
		std::string subtypeName = entitySubtypeName(e.eType, e.eSubType);
		std::string parmName = entityParmName(e.eType, e.eSubType, e.parm);

		out << YAML::Comment("Entity " + std::to_string(i));
		out << YAML::BeginMap;
		out << YAML::Key << "name" << YAML::Value << entityNames[i];
		out << YAML::Key << "tile_index" << YAML::Value << (int)e.tileIndex;
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

	out << YAML::EndSeq;
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
		wout << YAML::Key << "weapons" << YAML::Value << YAML::BeginSeq;

		for (int i = 0; i < (int)allWeapons.size(); i++) {
			const auto& w = allWeapons[i];
			wout << YAML::BeginMap;
			wout << YAML::Key << "name" << YAML::Value << weaponName(i);
			wout << YAML::Key << "index" << YAML::Value << i;

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
				wout << YAML::Key << "sprite" << YAML::Value << YAML::BeginMap;
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

			wout << YAML::EndMap;
		}

		wout << YAML::EndSeq;
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

		out << YAML::Key << "level_names" << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int i = 0; i < count; i++) {
			out << (int)r.readShort();
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
	mout << YAML::Key << "monsters" << YAML::Value << YAML::BeginSeq;

	for (int i = 0; i < numTypes; i++) {
		mout << YAML::BeginMap;
		mout << YAML::Key << "name" << YAML::Value << monsterName(i);
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

	mout << YAML::EndSeq;
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
static bool generateProjectilesYaml(const std::string& outDir) {
	YAML::Emitter out;
	out << YAML::Comment("Projectile visual definitions");
	out << YAML::Comment("Controls missile animation, render mode, and impact effects");
	out << YAML::Newline;
	out << YAML::BeginMap;
	out << YAML::Key << "projectiles" << YAML::Value << YAML::BeginSeq;

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
		out << YAML::BeginMap;
		out << YAML::Key << "name" << YAML::Value << d.name;

		if (d.launchRM >= 0 || d.animFromWeapon) {
			out << YAML::Key << "launch" << YAML::Value << YAML::BeginMap;
			if (d.launchRM >= 0) out << YAML::Key << "render_mode" << YAML::Value << d.launchRM;
			if (d.launchAnimMon != 0) {
				out << YAML::Key << "anim_player" << YAML::Value << d.launchAnim;
				out << YAML::Key << "anim_monster" << YAML::Value << d.launchAnimMon;
			} else if (d.launchAnim != 0) {
				out << YAML::Key << "anim" << YAML::Value << d.launchAnim;
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
			if (d.animFromWeapon) out << YAML::Key << "anim_from_weapon" << YAML::Value << true;
			out << YAML::EndMap;
		}

		if (d.impactAnim != 0 || d.impactSound != nullptr || d.shake) {
			out << YAML::Key << "impact" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "anim" << YAML::Value << d.impactAnim;
			if (d.impactRM != 0) out << YAML::Key << "render_mode" << YAML::Value << d.impactRM;
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

	out << YAML::EndSeq;
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

	out << YAML::Key << "buffs" << YAML::Value << YAML::BeginSeq;

	for (int i = 0; i < NUM_BUFFS; i++) {
		const auto& b = buffs[i];
		out << YAML::BeginMap;
		out << YAML::Key << "name" << YAML::Value << b.name;
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

	out << YAML::EndSeq;
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
	out << YAML::Comment("");
	out << YAML::Comment("Menu types: default, list, confirm, confirm2, main,");
	out << YAML::Comment("            help, vcenter, notebook, main_list, vending_machine");
	out << YAML::Comment("");
	out << YAML::Comment("Item flags: normal, noselect, nodehyphenate, disabled,");
	out << YAML::Comment("            align_center, showdetails, divider, selector,");
	out << YAML::Comment("            block_text, highlight, checked,");
	out << YAML::Comment("            right_arrow, left_arrow, hidden");
	out << YAML::Comment("");
	out << YAML::Comment("string_id values are indices into menu strings (group 3)");
	out << YAML::Comment("goto targets reference menu names instead of numeric IDs");
	out << YAML::Newline;
	out << YAML::BeginMap;
	out << YAML::Key << "menus" << YAML::Value << YAML::BeginSeq;

	for (int i = 0; i < menuDataCount; i++) {
		uint32_t md = menuData[i];
		int mid = md & 0xFF;
		int signedId = (mid >= 250) ? (mid - 256) : mid;
		int itemEnd = (md & 0xFFFF00) >> 8;
		int mtype = (md & 0xFF000000) >> 24;
		int itemStart = (i > 0) ? ((menuData[i - 1] & 0xFFFF00) >> 8) : 0;
		int numItems = (itemEnd - itemStart) / 2;

		out << YAML::BeginMap;
		const char* mname = menuIdToName(signedId);
		if (mname) {
			out << YAML::Key << "name" << YAML::Value << mname;
		}
		out << YAML::Key << "menu_id" << YAML::Value << signedId;
		if (mtype < 10) {
			out << YAML::Key << "type" << YAML::Value << MENU_TYPE_NAMES[mtype];
		} else {
			out << YAML::Key << "type" << YAML::Value << mtype;
		}

		if (numItems > 0) {
			out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;
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

	out << YAML::EndSeq;
	out << YAML::EndMap;

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

	// Build YAML using manual string building for better control of string quoting
	std::string yaml;
	yaml += "# String tables for DoomIIRPG\n";
	yaml += "# Special character escapes:\n";
	yaml += "#   \\x80 = line separator    \\x84 = cursor2      \\x85 = ellipsis\n";
	yaml += "#   \\x87 = checkmark         \\x88 = mini-dash    \\x89 = grey line\n";
	yaml += "#   \\x8A = cursor            \\x8B = shield       \\x8D = heart\n";
	yaml += "#   \\x90 = pointer           \\xA0 = hard space\n";
	yaml += "#   | = newline (in-game)     - = soft hyphen (word-break hint)\n";
	yaml += "#\n";
	yaml += "# String IDs in code: STRINGID(group, index) = (group << 10) | index\n";
	yaml += "\n";
	yaml += "strings:\n";

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

			// Count and extract strings
			std::vector<std::string> strings;
			int pos = byteOffset;
			int endPos = byteOffset + byteSize;
			while (pos < endPos) {
				int nul = pos;
				while (nul < endPos && chunks[chunkId][nul] != 0)
					nul++;
				int len = nul - pos;
				strings.push_back(escapeString(chunks[chunkId] + pos, len));
				pos = nul + 1;
			}

			yaml += "  # " + std::string(LANGUAGE_NAMES[lang]) + " / " + GROUP_NAMES[grp] + "\n";
			yaml += "  - language: " + std::to_string(lang) + "\n";
			yaml += "    group: " + std::to_string(grp) + "\n";
			yaml += "    values:\n";
			for (const auto& s : strings) {
				yaml += "      - \"" + s + "\"\n";
			}

			totalStrings += (int)strings.size();
		}
	}

	free(idxData);
	for (int i = 0; i < 3; i++) {
		if (chunks[i])
			free(chunks[i]);
	}

	std::string path = outDir + "/strings.yaml";
	if (!writeString(path, yaml)) {
		fprintf(stderr, "  Failed to write %s\n", path.c_str());
		return false;
	}
	printf("  -> %s (%d strings)\n", path.c_str(), totalStrings);
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
	out << YAML::Key << "sounds" << YAML::Value << YAML::BeginSeq;
	for (const auto& fileName : soundNames) {
		// Generate name from filename: strip .wav, lowercase
		std::string sndName = fileName;
		size_t dotPos = sndName.rfind('.');
		if (dotPos != std::string::npos) {
			sndName = sndName.substr(0, dotPos);
		}
		std::transform(sndName.begin(), sndName.end(), sndName.begin(), ::tolower);

		out << YAML::BeginMap;
		out << YAML::Key << "name" << YAML::Value << sndName;
		out << YAML::Key << "file" << YAML::Value << fileName;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
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
	yaml += "  # Map IDs where fog is disabled (e.g. outdoor maps)\n";
	yaml += "  no_fog_maps: [2]\n";
	yaml += "\n";
	yaml += "  # Subdirectories to search when resolving asset files.\n";
	yaml += "  # The engine tries the exact path first, then searches these dirs in order.\n";
	yaml += "  search_dirs:\n";
	yaml += "    - config\n";
	yaml += "    - ui\n";
	yaml += "    - hud\n";
	yaml += "    - fonts\n";
	yaml += "    - levels/maps\n";
	yaml += "    - levels/textures\n";
	yaml += "    - levels/models\n";
	yaml += "    - audio\n";
	yaml += "    - comicbook\n";
	yaml += "    - data\n";

	std::string path = outDir + "/game.yaml";
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
	yaml += "  - name: Major\n";
	yaml += "    defense: 8\n";
	yaml += "    strength: 9\n";
	yaml += "    accuracy: 97\n";
	yaml += "    agility: 12\n";
	yaml += "    iq: 110\n";
	yaml += "    credits: 30\n";
	yaml += "\n";
	yaml += "  - name: Sarge\n";
	yaml += "    defense: 12\n";
	yaml += "    strength: 14\n";
	yaml += "    accuracy: 92\n";
	yaml += "    agility: 6\n";
	yaml += "    iq: 100\n";
	yaml += "    credits: 10\n";
	yaml += "\n";
	yaml += "  - name: Scientist\n";
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
				outFile = "levels/maps/" + outFile;
			} else if (outFile.compare(0, 6, "model_") == 0) {
				outFile = "levels/models/" + outFile;
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

	printf("Generating characters.yaml...\n");
	ok &= generateCharactersYaml(outputDir);

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
