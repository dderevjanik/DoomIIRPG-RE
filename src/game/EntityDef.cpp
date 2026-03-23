#include <stdexcept>
#include <cstdio>
#include <string>
#include <vector>

#include "CAppContainer.h"
#include "App.h"
#include "EntityDef.h"
#include "Enums.h"
#include "JavaStream.h"
#include "Resource.h"
#include <yaml-cpp/yaml.h>

// --- Entity type name tables ---

static const char* entityTypeNames[] = {
    "world",                     // 0  ET_WORLD
    "player",                    // 1  ET_PLAYER
    "monster",                   // 2  ET_MONSTER
    "npc",                       // 3  ET_NPC
    "playerclip",                // 4  ET_PLAYERCLIP
    "door",                      // 5  ET_DOOR
    "item",                      // 6  ET_ITEM
    "decor",                     // 7  ET_DECOR
    "env_damage",                // 8  ET_ENV_DAMAGE
    "corpse",                    // 9  ET_CORPSE
    "attack_interactive",        // 10 ET_ATTACK_INTERACTIVE
    "monsterblock_item",         // 11 ET_MONSTERBLOCK_ITEM
    "spritewall",                // 12 ET_SPRITEWALL
    "nonobstructing_spritewall", // 13 ET_NONOBSTRUCTING_SPRITEWALL
    "decor_noclip"               // 14 ET_DECOR_NOCLIP
};
static const int numEntityTypes = Enums::ET_MAX;

static const char* entityTypeToString(int type) {
	if (type >= 0 && type < numEntityTypes) {
		return entityTypeNames[type];
	}
	return "unknown";
}

static int entityTypeFromString(const std::string& str) {
	for (int i = 0; i < numEntityTypes; i++) {
		if (str == entityTypeNames[i]) {
			return i;
		}
	}
	// Fallback: try parsing as integer
	try {
		return std::stoi(str);
	} catch (...) {
		return 0;
	}
}

// --- Named subtype/parm lookup tables ---

static const char* monsterSubtypeNames[] = {
    "zombie", "zombie_commando", "lost_soul", "imp", "sawcubus", "pinky",
    "cacodemon", "sentinel", "mancubus", "revenant", "arch_vile", "sentry_bot",
    "cyberdemon", "mastermind", "phantom", "boss_vios", "boss_vios2"
};
static const int numMonsterSubtypes = 17;

static const char* itemSubtypeNames[] = {
    "inventory", "weapon", "ammo", "food", "sack", "c_note", "c_string"
};
static const int numItemSubtypes = 7;

static const char* doorSubtypeNames[] = {"", "locked", "unlocked"};
static const int numDoorSubtypes = 3;

static const char* decorSubtypeNames[] = {"misc", "exithall", "mixing", "statue", "", "tombstone", "dynamite", "water_spout", "treadmill"};
static const int numDecorSubtypes = 9;

static const char* weaponParmNames[] = {
    "assault_rifle", "chainsaw", "holy_water_pistol",
    "shooting_sentry_bot", "exploding_sentry_bot",
    "red_shooting_sentry_bot", "red_exploding_sentry_bot",
    "super_shotgun", "chaingun", "assault_rifle_with_scope",
    "plasma_gun", "rocket_launcher", "bfg", "soul_cube", "item"
};
static const int numWeaponParms = 15;

static const char* ammoParmNames[] = {"none", "bullets", "shells", "holy_water", "cells", "rockets", "soul_cube"};
static const int numAmmoParms = 7;

static int lookupName(const std::string& str, const char* names[], int count) {
	for (int i = 0; i < count; i++) {
		if (names[i][0] != '\0' && str == names[i])
			return i;
	}
	try {
		return std::stoi(str);
	} catch (...) {
		return 0;
	}
}

static int resolveSubtype(int eType, const std::string& str) {
	switch (eType) {
		case Enums::ET_MONSTER:
		case Enums::ET_CORPSE:
			return lookupName(str, monsterSubtypeNames, numMonsterSubtypes);
		case Enums::ET_ITEM:
		case Enums::ET_MONSTERBLOCK_ITEM:
			return lookupName(str, itemSubtypeNames, numItemSubtypes);
		case Enums::ET_DOOR:
			return lookupName(str, doorSubtypeNames, numDoorSubtypes);
		case Enums::ET_DECOR:
		case Enums::ET_DECOR_NOCLIP:
			return lookupName(str, decorSubtypeNames, numDecorSubtypes);
		default:
			try {
				return std::stoi(str);
			} catch (...) {
				return 0;
			}
	}
}

static int resolveParm(int eType, int eSubType, const std::string& str) {
	if (eType == Enums::ET_ITEM || eType == Enums::ET_MONSTERBLOCK_ITEM) {
		if (eSubType == 1) // weapon
			return lookupName(str, weaponParmNames, numWeaponParms);
		if (eSubType == 2) // ammo
			return lookupName(str, ammoParmNames, numAmmoParms);
	}
	try {
		return std::stoi(str);
	} catch (...) {
		return 0;
	}
}

// Auto-compute render flags from tile index (for backward compat with binary data and INI defaults)
static uint32_t computeRenderFlags(int16_t tileIndex) {
	uint32_t flags = EntityDef::RFLAG_NONE;
	int n = tileIndex;

	// Floaters: Sentinel (44-46), Lost Soul (29-31), Cacodemon (41-43)
	if ((n >= Enums::TILENUM_MONSTER_SENTINEL && n <= Enums::TILENUM_MONSTER_SENTINEL3) ||
	    (n >= Enums::TILENUM_MONSTER_LOST_SOUL && n <= Enums::TILENUM_MONSTER_LOST_SOUL3) ||
	    (n >= Enums::TILENUM_MONSTER_CACODEMON && n <= Enums::TILENUM_MONSTER_CACODEMON3)) {
		flags |= EntityDef::RFLAG_FLOATER;
	}

	// Gun flare: Mancubus (38-40), Revenant (35-37), Sentry Bot (19), Red Sentry Bot (18),
	//            Cyberdemon (54), Mastermind (57)
	if ((n >= Enums::TILENUM_MONSTER_MANCUBUS && n <= Enums::TILENUM_MONSTER_MANCUBUS3) ||
	    (n >= Enums::TILENUM_MONSTER_REVENANT && n <= Enums::TILENUM_MONSTER_REVENANT3) ||
	    n == Enums::TILENUM_MONSTER_SENTRY_BOT || n == Enums::TILENUM_MONSTER_RED_SENTRY_BOT ||
	    n == Enums::TILENUM_BOSS_CYBERDEMON || n == Enums::TILENUM_BOSS_MASTERMIND) {
		flags |= EntityDef::RFLAG_GUN_FLARE;
	}

	// Special boss: Mastermind (57), Arachnotron (53), Boss Pinky (56), VIOS (58-62)
	if (n == Enums::TILENUM_BOSS_MASTERMIND || n == Enums::TILENUM_MONSTER_ARACHNOTRON ||
	    n == Enums::TILENUM_BOSS_PINKY || (n >= Enums::TILENUM_BOSS_VIOS && n <= Enums::TILENUM_BOSS_VIOS5)) {
		flags |= EntityDef::RFLAG_SPECIAL_BOSS;
	}

	// NPC: tile range 65-80
	if (n >= Enums::TILENUM_FIRST_NPC && n <= Enums::TILENUM_LAST_NPC) {
		flags |= EntityDef::RFLAG_NPC;
	}

	// Imp-like: Imp (20-22)
	if (n >= Enums::TILENUM_MONSTER_IMP && n <= Enums::TILENUM_MONSTER_IMP3) {
		flags |= EntityDef::RFLAG_IMP_TYPE;
	}

	// Revenant-like: Revenant (35-37)
	if (n >= Enums::TILENUM_MONSTER_REVENANT && n <= Enums::TILENUM_MONSTER_REVENANT3) {
		flags |= EntityDef::RFLAG_REVENANT_TYPE;
	}

	return flags;
}

// Auto-compute default fear eye offsets from monster subtype
static EntityDef::FearEyeData computeDefaultFearEyes(uint8_t eSubType) {
	EntityDef::FearEyeData d;
	switch (eSubType) {
		case Enums::MONSTER_ARCH_VILE:
			d.eyeL = -3;
			d.eyeR = 3;
			d.zAdd = 403;
			d.zIdlePre = 48;
			break;
		case Enums::MONSTER_SAW_GOBLIN:
			d.eyeL = -3;
			d.eyeR = 3;
			d.zAdd = 388;
			break;
		case Enums::MONSTER_IMP:
			d.eyeL = -3;
			d.eyeR = 3;
			d.zAdd = 294;
			break;
		case Enums::MONSTER_LOST_SOUL:
			d.eyeL = -4;
			d.eyeR = 4;
			d.zAdd = -192;
			d.zAlwaysPre = 192;
			break;
		case Enums::MONSTER_MANCUBUS:
			d.eyeL = -3;
			d.eyeR = 3;
			d.zAdd = 280;
			break;
		case Enums::MONSTER_PINKY:
			d.eyeL = -5;
			d.eyeR = 5;
			d.zAdd = 35;
			break;
		case Enums::MONSTER_CACODEMON:
			d.eyeL = 0;
			d.eyeR = -1;
			d.zAdd = 32;
			d.singleEye = true;
			break;
		case Enums::MONSTER_REVENANT:
			d.eyeL = -2;
			d.eyeR = 5;
			d.zAdd = 324;
			d.zAlwaysPre = 160;
			d.zIdlePre = 32;
			d.eyeLFlip = -5;
			d.eyeRFlip = 2;
			break;
		case Enums::MONSTER_SENTINEL:
			d.eyeL = -1;
			d.eyeR = 4;
			d.zAdd = 274;
			break;
		default:
			break;
	}
	return d;
}

// Auto-compute default gun flare offsets from monster subtype
static EntityDef::GunFlareData computeDefaultGunFlare(uint8_t eSubType) {
	EntityDef::GunFlareData d;
	switch (eSubType) {
		case Enums::MONSTER_MANCUBUS:
			d.dualFlare = true;
			d.flash1X = -24;
			d.flash1Z = 160;
			d.flash2X = 22;
			d.flash2Z = 128;
			break;
		case Enums::MONSTER_REVENANT:
			d.dualFlare = true;
			d.flash1X = -9;
			d.flash1Z = 736;
			d.flash2X = 15;
			d.flash2Z = 736;
			break;
		case Enums::MONSTER_SENTRY_BOT:
			d.flash2Z = -64;
			break;
		case Enums::BOSS_CYBERDEMON:
			d.flash2X = 14;
			d.flash2Z = 352;
			break;
		default:
			break;
	}
	return d;
}

// Auto-compute default body part offsets from monster subtype and entity type
static EntityDef::BodyPartData computeDefaultBodyParts(uint8_t eType, uint8_t eSubType) {
	EntityDef::BodyPartData d;
	if (eType == Enums::ET_MONSTER) {
		switch (eSubType) {
			case Enums::MONSTER_REVENANT:
				d.idleTorsoZ = -30;
				d.idleHeadZ = 140;
				d.walkHeadZ = 160;
				d.attackHeadZ = 20;
				d.attackHeadX = 2;
				d.attackRevAtk2TorsoZ = 130;
				break;
			case Enums::MONSTER_ARCH_VILE:
				d.idleTorsoZ = -36;
				d.idleHeadZ = 109;
				d.walkTorsoZ = -36;
				d.walkHeadZ = 109;
				d.flipTorsoWalk = true;
				d.attackTorsoZF0 = 288;
				d.noHeadOnAttack = true; // ArchVile hides head in attack (n19=0)
				break;
			case Enums::MONSTER_SAW_GOBLIN:
				d.attackTorsoZF0 = 96;
				d.attackTorsoZF1 = -100;
				d.attackHeadZ = 16;
				d.attackHeadX = 0;
				break;
			case Enums::MONSTER_MANCUBUS:
				d.attackHeadZ = -112;
				d.attackHeadX = 7;
				d.noHeadOnMancAtk = true;
				break;
			case Enums::MONSTER_PINKY:
				d.noHeadOnBack = true;
				break;
			case Enums::MONSTER_ZOMBIE:
				d.noHeadOnAttack = true;
				break;
			case Enums::MONSTER_IMP:
				d.noHeadOnAttack = true;
				break;
			case Enums::MONSTER_SENTRY_BOT:
				d.sentryHeadFlip = true;
				break;
			case Enums::BOSS_CYBERDEMON:
				d.idleTorsoZ = -30;
				d.idleHeadZ = -15;
				break;
			default:
				break;
		}
	} else if (eType == Enums::ET_NPC) {
		// NPC defaults (specific NPCs get overrides in INI)
		d.idleTorsoZ = -18;
		d.flipTorsoWalk = true;
	}
	return d;
}

// Auto-compute default floater rendering data from monster subtype
static EntityDef::FloaterData computeDefaultFloater(uint8_t eSubType) {
	EntityDef::FloaterData d;
	switch (eSubType) {
		case Enums::MONSTER_SENTINEL:
			d.type = EntityDef::FloaterData::FLOATER_MULTIPART;
			d.backViewFrame = 6;
			d.idleFrontFrame = 2;
			d.headZOffset = -11;
			d.hasDeadLoot = true;
			break;
		case Enums::MONSTER_LOST_SOUL:
			d.zOffset = 192;
			d.hasIdleFrameIncrement = true;
			break;
		case Enums::MONSTER_CACODEMON:
			d.hasBackExtraSprite = true;
			d.backExtraSpriteIdx = 7;
			break;
		default:
			break;
	}
	return d;
}

// Auto-compute default special boss rendering data from tile index and subtype
static EntityDef::SpecialBossData computeDefaultSpecialBoss(int16_t tileIndex, uint8_t eSubType) {
	EntityDef::SpecialBossData d;
	if (tileIndex == Enums::TILENUM_BOSS_PINKY) {
		d.type = EntityDef::SpecialBossData::BOSS_MULTIPART;
		d.torsoZ = 384;
		d.idleSpriteIdx = 3;
		d.attackSpriteIdx = 8;
		d.painSpriteIdx = 12;
	} else if (tileIndex >= Enums::TILENUM_BOSS_VIOS && tileIndex <= Enums::TILENUM_BOSS_VIOS5) {
		d.type = EntityDef::SpecialBossData::BOSS_ETHEREAL;
		d.renderModeOverride = 3;
		d.painRenderMode = 5;
	} else if (tileIndex == Enums::TILENUM_BOSS_MASTERMIND) {
		d.type = EntityDef::SpecialBossData::BOSS_SPIDER;
		d.legLateral = -44;
		d.legBaseZ = 6;
		d.idleTorsoZ = 35;
		d.idleBobDiv = 1;
		d.idleTorsoZBase = 0;
		d.walkTorsoZ = 35;
		d.attackTorsoZ = 40;
		d.painTorsoZ = 70;
		d.painLegsZ = 100;
		d.painLegPos = 2;
		d.hasAttackFlare = true;
		d.flareZOffset = 288;
		d.flareLateralPos = 0;
		d.flareTorsoZExtra = 10;
	} else if (tileIndex == Enums::TILENUM_MONSTER_ARACHNOTRON) {
		d.type = EntityDef::SpecialBossData::BOSS_SPIDER;
		d.legLateral = -29;
		d.legBaseZ = 6;
		d.idleTorsoZ = 50;
		d.idleBobDiv = 2;
		d.idleTorsoZBase = 50;
		d.walkTorsoZ = 50;
		d.attackTorsoZ = 50;
		d.painTorsoZ = 100;
		d.painLegsZ = 70;
		d.painLegPos = 1;
		d.hasAttackFlare = false;
	}
	return d;
}

// Auto-compute default sprite anim overrides
static EntityDef::SpriteAnimData computeDefaultSpriteAnim(int16_t tileIndex, uint8_t eSubType) {
	EntityDef::SpriteAnimData d;
	if (tileIndex == Enums::TILENUM_BOSS_CYBERDEMON) {
		d.clampScale = true;
		d.attackLegOffset = -3;
		d.attackLegOffsetOnFlip = true;
	} else if (eSubType == Enums::MONSTER_SAW_GOBLIN) {
		d.hasFrameDependentHead = true;
		d.headZFrame0 = 17;
		d.headXFrame0 = 2;
		d.headZFrame1 = 16;
	}
	return d;
}

// -----------------------
// EntityDefManager Class
// -----------------------

EntityDefManager::EntityDefManager() {
	std::memset(this, 0, sizeof(EntityDefManager));
}

EntityDefManager::~EntityDefManager() {}

bool EntityDefManager::startup() {
	printf("EntityDefManager::startup\n");

	this->numDefs = 0;

	printf("EntityDefManager: loading from entities.yaml\n");
	return this->loadFromYAML("entities.yaml");
}

bool EntityDefManager::loadFromYAML(const char* path) {
	Applet* app = CAppContainer::getInstance()->app;

	YAML::Node config;
	try {
		config = YAML::LoadFile(path);
	} catch (const YAML::Exception& e) {
		app->Error("Failed to load %s: %s\n", path, e.what());
		return false;
	}

	YAML::Node entities = config["entities"];
	if (!entities || !entities.IsMap() || entities.size() == 0) {
		app->Error("entities.yaml: missing or empty 'entities' map\n");
		return false;
	}

	int count = (int)entities.size();
	this->numDefs = count;
	this->list = new EntityDef[count];

	int i = 0;
	for (auto eit = entities.begin(); eit != entities.end(); ++eit, ++i) {
		YAML::Node e = eit->second;

		this->list[i].tileIndex = (int16_t)e["tile_index"].as<int>(0);
		this->list[i].eType = (uint8_t)entityTypeFromString(e["type"].as<std::string>("world").c_str());
		this->list[i].eSubType = (uint8_t)resolveSubtype(this->list[i].eType, e["subtype"].as<std::string>("0"));
		this->list[i].parm = (uint8_t)resolveParm(this->list[i].eType, this->list[i].eSubType, e["parm"].as<std::string>("0"));

		// Text resource IDs: support both nested text: group and flat fields
		if (YAML::Node text = e["text"]) {
			this->list[i].name = (int16_t)text["name"].as<int>(0);
			this->list[i].longName = (int16_t)text["long_name"].as<int>(0);
			this->list[i].description = (int16_t)text["description"].as<int>(0);
		} else {
			this->list[i].name = (int16_t)e["name"].as<int>(0);
			this->list[i].longName = (int16_t)e["long_name"].as<int>(0);
			this->list[i].description = (int16_t)e["description"].as<int>(0);
		}

		// Load render flags, fall back to auto-computed defaults
		uint32_t defaultFlags = computeRenderFlags(this->list[i].tileIndex);
		this->list[i].renderFlags = EntityDef::RFLAG_NONE;
		if (e["is_floater"].as<bool>((defaultFlags & EntityDef::RFLAG_FLOATER) != 0)) {
			this->list[i].renderFlags |= EntityDef::RFLAG_FLOATER;
		}
		if (e["has_gun_flare"].as<bool>((defaultFlags & EntityDef::RFLAG_GUN_FLARE) != 0)) {
			this->list[i].renderFlags |= EntityDef::RFLAG_GUN_FLARE;
		}
		if (e["is_special_boss"].as<bool>((defaultFlags & EntityDef::RFLAG_SPECIAL_BOSS) != 0)) {
			this->list[i].renderFlags |= EntityDef::RFLAG_SPECIAL_BOSS;
		}
		if (e["is_npc"].as<bool>((defaultFlags & EntityDef::RFLAG_NPC) != 0)) {
			this->list[i].renderFlags |= EntityDef::RFLAG_NPC;
		}
		if (e["is_imp_type"].as<bool>((defaultFlags & EntityDef::RFLAG_IMP_TYPE) != 0)) {
			this->list[i].renderFlags |= EntityDef::RFLAG_IMP_TYPE;
		}
		if (e["is_revenant_type"].as<bool>((defaultFlags & EntityDef::RFLAG_REVENANT_TYPE) != 0)) {
			this->list[i].renderFlags |= EntityDef::RFLAG_REVENANT_TYPE;
		}

		// Rendering data: nested rendering: group or flat keys, with computeDefault*() fallback
		YAML::Node r = e["rendering"];

		// Render flags from nested rendering.flags array
		if (r && r["flags"]) {
			for (int fi = 0; fi < (int)r["flags"].size(); fi++) {
				std::string flag = r["flags"][fi].as<std::string>("");
				if (flag == "floater") this->list[i].renderFlags |= EntityDef::RFLAG_FLOATER;
				else if (flag == "gun_flare") this->list[i].renderFlags |= EntityDef::RFLAG_GUN_FLARE;
				else if (flag == "special_boss") this->list[i].renderFlags |= EntityDef::RFLAG_SPECIAL_BOSS;
				else if (flag == "npc") this->list[i].renderFlags |= EntityDef::RFLAG_NPC;
				else if (flag == "imp_type") this->list[i].renderFlags |= EntityDef::RFLAG_IMP_TYPE;
				else if (flag == "revenant_type") this->list[i].renderFlags |= EntityDef::RFLAG_REVENANT_TYPE;
			}
		}

		// Helper: read from rendering.section.key, then flat key, then default
		#define R_INT(section, key, flat, def) \
			((r && r[section] && r[section][key]) ? r[section][key].as<int>(def) : e[flat].as<int>(def))
		#define R_BOOL(section, key, flat, def) \
			((r && r[section] && r[section][key]) ? r[section][key].as<bool>(def) : e[flat].as<bool>(def))

		// Fear eye offsets
		EntityDef::FearEyeData defEyes = computeDefaultFearEyes(this->list[i].eSubType);
		this->list[i].fearEyes.eyeL = (int8_t)R_INT("fear_eyes", "eye_l", "fear_eye_l", defEyes.eyeL);
		this->list[i].fearEyes.eyeR = (int8_t)R_INT("fear_eyes", "eye_r", "fear_eye_r", defEyes.eyeR);
		this->list[i].fearEyes.zAdd = (int16_t)R_INT("fear_eyes", "z_add", "fear_eye_z_add", defEyes.zAdd);
		this->list[i].fearEyes.zAlwaysPre = (int16_t)R_INT("fear_eyes", "z_always_pre", "fear_eye_z_always", defEyes.zAlwaysPre);
		this->list[i].fearEyes.zIdlePre = (int16_t)R_INT("fear_eyes", "z_idle_pre", "fear_eye_z_idle", defEyes.zIdlePre);
		this->list[i].fearEyes.singleEye = R_BOOL("fear_eyes", "single_eye", "fear_single_eye", defEyes.singleEye);
		this->list[i].fearEyes.eyeLFlip = (int8_t)R_INT("fear_eyes", "eye_l_flip", "fear_eye_l_flip", defEyes.eyeLFlip);
		this->list[i].fearEyes.eyeRFlip = (int8_t)R_INT("fear_eyes", "eye_r_flip", "fear_eye_r_flip", defEyes.eyeRFlip);

		// Gun flare offsets
		EntityDef::GunFlareData defFlare = computeDefaultGunFlare(this->list[i].eSubType);
		this->list[i].gunFlare.dualFlare = R_BOOL("gun_flare", "dual", "gun_flare_dual", defFlare.dualFlare);
		this->list[i].gunFlare.flash1X = (int8_t)R_INT("gun_flare", "flash1_x", "gun_flare1_x", defFlare.flash1X);
		this->list[i].gunFlare.flash1Z = (int16_t)R_INT("gun_flare", "flash1_z", "gun_flare1_z", defFlare.flash1Z);
		this->list[i].gunFlare.flash2X = (int8_t)R_INT("gun_flare", "flash2_x", "gun_flare2_x", defFlare.flash2X);
		this->list[i].gunFlare.flash2Z = (int16_t)R_INT("gun_flare", "flash2_z", "gun_flare2_z", defFlare.flash2Z);

		// Body part offsets
		EntityDef::BodyPartData defBody = computeDefaultBodyParts(this->list[i].eType, this->list[i].eSubType);
		this->list[i].bodyParts.idleTorsoZ = (int16_t)R_INT("body_parts", "idle_torso_z", "idle_torso_z", defBody.idleTorsoZ);
		this->list[i].bodyParts.idleHeadZ = (int16_t)R_INT("body_parts", "idle_head_z", "idle_head_z", defBody.idleHeadZ);
		this->list[i].bodyParts.walkTorsoZ = (int16_t)R_INT("body_parts", "walk_torso_z", "walk_torso_z", defBody.walkTorsoZ);
		this->list[i].bodyParts.walkHeadZ = (int16_t)R_INT("body_parts", "walk_head_z", "walk_head_z", defBody.walkHeadZ);
		this->list[i].bodyParts.attackTorsoZF0 = (int16_t)R_INT("body_parts", "attack_torso_z_f0", "attack_torso_z_f0", defBody.attackTorsoZF0);
		this->list[i].bodyParts.attackTorsoZF1 = (int16_t)R_INT("body_parts", "attack_torso_z_f1", "attack_torso_z_f1", defBody.attackTorsoZF1);
		this->list[i].bodyParts.attackHeadZ = (int16_t)R_INT("body_parts", "attack_head_z", "attack_head_z", defBody.attackHeadZ);
		this->list[i].bodyParts.attackHeadX = (int8_t)R_INT("body_parts", "attack_head_x", "attack_head_x", defBody.attackHeadX);
		this->list[i].bodyParts.noHeadOnBack = R_BOOL("body_parts", "no_head_on_back", "no_head_on_back", defBody.noHeadOnBack);
		this->list[i].bodyParts.sentryHeadFlip = R_BOOL("body_parts", "sentry_head_flip", "sentry_head_flip", defBody.sentryHeadFlip);
		this->list[i].bodyParts.flipTorsoWalk = R_BOOL("body_parts", "flip_torso_walk", "flip_torso_walk", defBody.flipTorsoWalk);
		this->list[i].bodyParts.noHeadOnAttack = R_BOOL("body_parts", "no_head_on_attack", "no_head_on_attack", defBody.noHeadOnAttack);
		this->list[i].bodyParts.noHeadOnMancAtk = R_BOOL("body_parts", "no_head_on_manc_atk", "no_head_on_manc_atk", defBody.noHeadOnMancAtk);
		this->list[i].bodyParts.attackRevAtk2TorsoZ = (int16_t)R_INT("body_parts", "attack_rev_atk2_torso_z", "attack_rev_atk2_torso_z", defBody.attackRevAtk2TorsoZ);

		// Floater rendering data
		EntityDef::FloaterData defFloat = computeDefaultFloater(this->list[i].eSubType);
		this->list[i].floater.type = (EntityDef::FloaterData::Type)R_INT("floater", "type", "floater_type", (int)defFloat.type);
		this->list[i].floater.zOffset = (int16_t)R_INT("floater", "z_offset", "floater_z_offset", defFloat.zOffset);
		this->list[i].floater.hasIdleFrameIncrement = R_BOOL("floater", "has_idle_frame_increment", "floater_idle_frame_inc", defFloat.hasIdleFrameIncrement);
		this->list[i].floater.hasBackExtraSprite = R_BOOL("floater", "has_back_extra_sprite", "floater_back_extra_sprite", defFloat.hasBackExtraSprite);
		this->list[i].floater.backExtraSpriteIdx = (int8_t)R_INT("floater", "back_extra_sprite_idx", "floater_back_extra_idx", defFloat.backExtraSpriteIdx);
		this->list[i].floater.backViewFrame = (int8_t)R_INT("floater", "back_view_frame", "floater_back_view_frame", defFloat.backViewFrame);
		this->list[i].floater.idleFrontFrame = (int8_t)R_INT("floater", "idle_front_frame", "floater_idle_front_frame", defFloat.idleFrontFrame);
		this->list[i].floater.headZOffset = (int16_t)R_INT("floater", "head_z_offset", "floater_head_z", defFloat.headZOffset);
		this->list[i].floater.hasDeadLoot = R_BOOL("floater", "has_dead_loot", "floater_dead_loot", defFloat.hasDeadLoot);

		// Special boss rendering data
		EntityDef::SpecialBossData defBoss = computeDefaultSpecialBoss(this->list[i].tileIndex, this->list[i].eSubType);
		// Parse type as string or int
		{
			int bossType = (int)defBoss.type;
			if (r && r["special_boss"] && r["special_boss"]["type"]) {
				std::string ts = r["special_boss"]["type"].as<std::string>("");
				if (ts == "multipart") bossType = 0;
				else if (ts == "ethereal") bossType = 1;
				else if (ts == "spider") bossType = 2;
				else bossType = r["special_boss"]["type"].as<int>(bossType);
			} else {
				bossType = e["boss_type"].as<int>(bossType);
			}
			this->list[i].specialBoss.type = (EntityDef::SpecialBossData::Type)bossType;
		}
		this->list[i].specialBoss.torsoZ = (int16_t)R_INT("special_boss", "torso_z", "boss_torso_z", defBoss.torsoZ);
		this->list[i].specialBoss.idleSpriteIdx = (int8_t)R_INT("special_boss", "idle_sprite_idx", "boss_idle_sprite", defBoss.idleSpriteIdx);
		this->list[i].specialBoss.attackSpriteIdx = (int8_t)R_INT("special_boss", "attack_sprite_idx", "boss_attack_sprite", defBoss.attackSpriteIdx);
		this->list[i].specialBoss.painSpriteIdx = (int8_t)R_INT("special_boss", "pain_sprite_idx", "boss_pain_sprite", defBoss.painSpriteIdx);
		this->list[i].specialBoss.renderModeOverride = (int8_t)R_INT("special_boss", "render_mode_override", "boss_render_mode", defBoss.renderModeOverride);
		this->list[i].specialBoss.painRenderMode = (int8_t)R_INT("special_boss", "pain_render_mode", "boss_pain_render_mode", defBoss.painRenderMode);
		this->list[i].specialBoss.legLateral = (int16_t)R_INT("special_boss", "leg_lateral", "boss_leg_lateral", defBoss.legLateral);
		this->list[i].specialBoss.legBaseZ = (int16_t)R_INT("special_boss", "leg_base_z", "boss_leg_base_z", defBoss.legBaseZ);
		this->list[i].specialBoss.idleTorsoZ = (int16_t)R_INT("special_boss", "idle_torso_z", "boss_idle_torso_z", defBoss.idleTorsoZ);
		this->list[i].specialBoss.idleBobDiv = (int8_t)R_INT("special_boss", "idle_bob_div", "boss_idle_bob_div", defBoss.idleBobDiv);
		this->list[i].specialBoss.idleTorsoZBase = (int16_t)R_INT("special_boss", "idle_torso_z_base", "boss_idle_torso_z_base", defBoss.idleTorsoZBase);
		this->list[i].specialBoss.walkTorsoZ = (int16_t)R_INT("special_boss", "walk_torso_z", "boss_walk_torso_z", defBoss.walkTorsoZ);
		this->list[i].specialBoss.attackTorsoZ = (int16_t)R_INT("special_boss", "attack_torso_z", "boss_attack_torso_z", defBoss.attackTorsoZ);
		this->list[i].specialBoss.painTorsoZ = (int16_t)R_INT("special_boss", "pain_torso_z", "boss_pain_torso_z", defBoss.painTorsoZ);
		this->list[i].specialBoss.painLegsZ = (int16_t)R_INT("special_boss", "pain_legs_z", "boss_pain_legs_z", defBoss.painLegsZ);
		this->list[i].specialBoss.painLegPos = (int8_t)R_INT("special_boss", "pain_leg_pos", "boss_pain_leg_pos", defBoss.painLegPos);
		this->list[i].specialBoss.hasAttackFlare = R_BOOL("special_boss", "has_attack_flare", "boss_attack_flare", defBoss.hasAttackFlare);
		this->list[i].specialBoss.flareZOffset = (int16_t)R_INT("special_boss", "flare_z_offset", "boss_flare_z", defBoss.flareZOffset);
		this->list[i].specialBoss.flareLateralPos = (int8_t)R_INT("special_boss", "flare_lateral_pos", "boss_flare_lateral", defBoss.flareLateralPos);
		this->list[i].specialBoss.flareTorsoZExtra = (int16_t)R_INT("special_boss", "flare_torso_z_extra", "boss_flare_torso_z_extra", defBoss.flareTorsoZExtra);

		// Sprite anim overrides
		EntityDef::SpriteAnimData defAnim = computeDefaultSpriteAnim(this->list[i].tileIndex, this->list[i].eSubType);
		this->list[i].spriteAnim.clampScale = R_BOOL("sprite_anim", "clamp_scale", "anim_clamp_scale", defAnim.clampScale);
		this->list[i].spriteAnim.attackLegOffset = (int8_t)R_INT("sprite_anim", "attack_leg_offset", "anim_attack_leg_offset", defAnim.attackLegOffset);
		this->list[i].spriteAnim.attackLegOffsetOnFlip = R_BOOL("sprite_anim", "attack_leg_offset_on_flip", "anim_attack_leg_on_flip", defAnim.attackLegOffsetOnFlip);
		this->list[i].spriteAnim.hasFrameDependentHead = R_BOOL("sprite_anim", "has_frame_dependent_head", "anim_frame_dep_head", defAnim.hasFrameDependentHead);
		this->list[i].spriteAnim.headZFrame0 = (int16_t)R_INT("sprite_anim", "head_z_frame0", "anim_head_z_f0", defAnim.headZFrame0);
		this->list[i].spriteAnim.headXFrame0 = (int8_t)R_INT("sprite_anim", "head_x_frame0", "anim_head_x_f0", defAnim.headXFrame0);
		this->list[i].spriteAnim.headZFrame1 = (int16_t)R_INT("sprite_anim", "head_z_frame1", "anim_head_z_f1", defAnim.headZFrame1);

		#undef R_INT
		#undef R_BOOL
	}

	printf("EntityDefManager: loaded %d entities from %s\n", this->numDefs, path);

	return true;
}

EntityDef* EntityDefManager::find(int eType, int eSubType) {
	return this->find(eType, eSubType, -1);
}

EntityDef* EntityDefManager::find(int eType, int eSubType, int parm) {
	for (int i = 0; i < this->numDefs; i++) {
		if (this->list[i].eType == eType && this->list[i].eSubType == eSubType &&
		    (parm == -1 || this->list[i].parm == parm)) {
			return &this->list[i];
		}
	}
	return nullptr;
}

EntityDef* EntityDefManager::lookup(int tileIndex) {
	for (int i = 0; i < this->numDefs; ++i) {
		if (this->list[i].tileIndex == tileIndex) {
			return &this->list[i];
		}
	}
	return nullptr;
}

// ----------------
// EntityDef Class
// ----------------

EntityDef::EntityDef() {
	std::memset(this, 0, sizeof(EntityDef));
}
