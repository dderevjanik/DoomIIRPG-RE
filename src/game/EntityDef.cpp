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
	if (!entities || !entities.IsSequence() || entities.size() == 0) {
		app->Error("entities.yaml: missing or empty 'entities' array\n");
		return false;
	}

	int count = (int)entities.size();
	this->numDefs = count;
	this->list = new EntityDef[count];

	for (int i = 0; i < count; i++) {
		YAML::Node e = entities[i];

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

		// Fear eye offsets
		EntityDef::FearEyeData defEyes = computeDefaultFearEyes(this->list[i].eSubType);
		this->list[i].fearEyes.eyeL = (int8_t)e["fear_eye_l"].as<int>(defEyes.eyeL);
		this->list[i].fearEyes.eyeR = (int8_t)e["fear_eye_r"].as<int>(defEyes.eyeR);
		this->list[i].fearEyes.zAdd = (int16_t)e["fear_eye_z_add"].as<int>(defEyes.zAdd);
		this->list[i].fearEyes.zAlwaysPre = (int16_t)e["fear_eye_z_always"].as<int>(defEyes.zAlwaysPre);
		this->list[i].fearEyes.zIdlePre = (int16_t)e["fear_eye_z_idle"].as<int>(defEyes.zIdlePre);
		this->list[i].fearEyes.singleEye = e["fear_single_eye"].as<bool>(defEyes.singleEye);
		this->list[i].fearEyes.eyeLFlip = (int8_t)e["fear_eye_l_flip"].as<int>(defEyes.eyeLFlip);
		this->list[i].fearEyes.eyeRFlip = (int8_t)e["fear_eye_r_flip"].as<int>(defEyes.eyeRFlip);

		// Gun flare offsets
		EntityDef::GunFlareData defFlare = computeDefaultGunFlare(this->list[i].eSubType);
		this->list[i].gunFlare.dualFlare = e["gun_flare_dual"].as<bool>(defFlare.dualFlare);
		this->list[i].gunFlare.flash1X = (int8_t)e["gun_flare1_x"].as<int>(defFlare.flash1X);
		this->list[i].gunFlare.flash1Z = (int16_t)e["gun_flare1_z"].as<int>(defFlare.flash1Z);
		this->list[i].gunFlare.flash2X = (int8_t)e["gun_flare2_x"].as<int>(defFlare.flash2X);
		this->list[i].gunFlare.flash2Z = (int16_t)e["gun_flare2_z"].as<int>(defFlare.flash2Z);

		// Body part offsets
		EntityDef::BodyPartData defBody = computeDefaultBodyParts(this->list[i].eType, this->list[i].eSubType);
		this->list[i].bodyParts.idleTorsoZ = (int16_t)e["idle_torso_z"].as<int>(defBody.idleTorsoZ);
		this->list[i].bodyParts.idleHeadZ = (int16_t)e["idle_head_z"].as<int>(defBody.idleHeadZ);
		this->list[i].bodyParts.walkTorsoZ = (int16_t)e["walk_torso_z"].as<int>(defBody.walkTorsoZ);
		this->list[i].bodyParts.walkHeadZ = (int16_t)e["walk_head_z"].as<int>(defBody.walkHeadZ);
		this->list[i].bodyParts.attackTorsoZF0 = (int16_t)e["attack_torso_z_f0"].as<int>(defBody.attackTorsoZF0);
		this->list[i].bodyParts.attackTorsoZF1 = (int16_t)e["attack_torso_z_f1"].as<int>(defBody.attackTorsoZF1);
		this->list[i].bodyParts.attackHeadZ = (int16_t)e["attack_head_z"].as<int>(defBody.attackHeadZ);
		this->list[i].bodyParts.attackHeadX = (int8_t)e["attack_head_x"].as<int>(defBody.attackHeadX);
		this->list[i].bodyParts.noHeadOnBack = e["no_head_on_back"].as<bool>(defBody.noHeadOnBack);
		this->list[i].bodyParts.sentryHeadFlip = e["sentry_head_flip"].as<bool>(defBody.sentryHeadFlip);
		this->list[i].bodyParts.flipTorsoWalk = e["flip_torso_walk"].as<bool>(defBody.flipTorsoWalk);
		this->list[i].bodyParts.noHeadOnAttack = e["no_head_on_attack"].as<bool>(defBody.noHeadOnAttack);
		this->list[i].bodyParts.noHeadOnMancAtk = e["no_head_on_manc_atk"].as<bool>(defBody.noHeadOnMancAtk);
		this->list[i].bodyParts.attackRevAtk2TorsoZ = (int16_t)e["attack_rev_atk2_torso_z"].as<int>(defBody.attackRevAtk2TorsoZ);

		// Floater rendering data
		EntityDef::FloaterData defFloat = computeDefaultFloater(this->list[i].eSubType);
		this->list[i].floater.type = (EntityDef::FloaterData::Type)e["floater_type"].as<int>((int)defFloat.type);
		this->list[i].floater.zOffset = (int16_t)e["floater_z_offset"].as<int>(defFloat.zOffset);
		this->list[i].floater.hasIdleFrameIncrement = e["floater_idle_frame_inc"].as<bool>(defFloat.hasIdleFrameIncrement);
		this->list[i].floater.hasBackExtraSprite = e["floater_back_extra_sprite"].as<bool>(defFloat.hasBackExtraSprite);
		this->list[i].floater.backExtraSpriteIdx = (int8_t)e["floater_back_extra_idx"].as<int>(defFloat.backExtraSpriteIdx);
		this->list[i].floater.backViewFrame = (int8_t)e["floater_back_view_frame"].as<int>(defFloat.backViewFrame);
		this->list[i].floater.idleFrontFrame = (int8_t)e["floater_idle_front_frame"].as<int>(defFloat.idleFrontFrame);
		this->list[i].floater.headZOffset = (int16_t)e["floater_head_z"].as<int>(defFloat.headZOffset);
		this->list[i].floater.hasDeadLoot = e["floater_dead_loot"].as<bool>(defFloat.hasDeadLoot);

		// Special boss rendering data
		EntityDef::SpecialBossData defBoss = computeDefaultSpecialBoss(this->list[i].tileIndex, this->list[i].eSubType);
		this->list[i].specialBoss.type = (EntityDef::SpecialBossData::Type)e["boss_type"].as<int>((int)defBoss.type);
		this->list[i].specialBoss.torsoZ = (int16_t)e["boss_torso_z"].as<int>(defBoss.torsoZ);
		this->list[i].specialBoss.idleSpriteIdx = (int8_t)e["boss_idle_sprite"].as<int>(defBoss.idleSpriteIdx);
		this->list[i].specialBoss.attackSpriteIdx = (int8_t)e["boss_attack_sprite"].as<int>(defBoss.attackSpriteIdx);
		this->list[i].specialBoss.painSpriteIdx = (int8_t)e["boss_pain_sprite"].as<int>(defBoss.painSpriteIdx);
		this->list[i].specialBoss.renderModeOverride = (int8_t)e["boss_render_mode"].as<int>(defBoss.renderModeOverride);
		this->list[i].specialBoss.painRenderMode = (int8_t)e["boss_pain_render_mode"].as<int>(defBoss.painRenderMode);
		this->list[i].specialBoss.legLateral = (int16_t)e["boss_leg_lateral"].as<int>(defBoss.legLateral);
		this->list[i].specialBoss.legBaseZ = (int16_t)e["boss_leg_base_z"].as<int>(defBoss.legBaseZ);
		this->list[i].specialBoss.idleTorsoZ = (int16_t)e["boss_idle_torso_z"].as<int>(defBoss.idleTorsoZ);
		this->list[i].specialBoss.idleBobDiv = (int8_t)e["boss_idle_bob_div"].as<int>(defBoss.idleBobDiv);
		this->list[i].specialBoss.idleTorsoZBase = (int16_t)e["boss_idle_torso_z_base"].as<int>(defBoss.idleTorsoZBase);
		this->list[i].specialBoss.walkTorsoZ = (int16_t)e["boss_walk_torso_z"].as<int>(defBoss.walkTorsoZ);
		this->list[i].specialBoss.attackTorsoZ = (int16_t)e["boss_attack_torso_z"].as<int>(defBoss.attackTorsoZ);
		this->list[i].specialBoss.painTorsoZ = (int16_t)e["boss_pain_torso_z"].as<int>(defBoss.painTorsoZ);
		this->list[i].specialBoss.painLegsZ = (int16_t)e["boss_pain_legs_z"].as<int>(defBoss.painLegsZ);
		this->list[i].specialBoss.painLegPos = (int8_t)e["boss_pain_leg_pos"].as<int>(defBoss.painLegPos);
		this->list[i].specialBoss.hasAttackFlare = e["boss_attack_flare"].as<bool>(defBoss.hasAttackFlare);
		this->list[i].specialBoss.flareZOffset = (int16_t)e["boss_flare_z"].as<int>(defBoss.flareZOffset);
		this->list[i].specialBoss.flareLateralPos = (int8_t)e["boss_flare_lateral"].as<int>(defBoss.flareLateralPos);
		this->list[i].specialBoss.flareTorsoZExtra = (int16_t)e["boss_flare_torso_z_extra"].as<int>(defBoss.flareTorsoZExtra);

		// Sprite anim overrides
		EntityDef::SpriteAnimData defAnim = computeDefaultSpriteAnim(this->list[i].tileIndex, this->list[i].eSubType);
		this->list[i].spriteAnim.clampScale = e["anim_clamp_scale"].as<bool>(defAnim.clampScale);
		this->list[i].spriteAnim.attackLegOffset = (int8_t)e["anim_attack_leg_offset"].as<int>(defAnim.attackLegOffset);
		this->list[i].spriteAnim.attackLegOffsetOnFlip = e["anim_attack_leg_on_flip"].as<bool>(defAnim.attackLegOffsetOnFlip);
		this->list[i].spriteAnim.hasFrameDependentHead = e["anim_frame_dep_head"].as<bool>(defAnim.hasFrameDependentHead);
		this->list[i].spriteAnim.headZFrame0 = (int16_t)e["anim_head_z_f0"].as<int>(defAnim.headZFrame0);
		this->list[i].spriteAnim.headXFrame0 = (int8_t)e["anim_head_x_f0"].as<int>(defAnim.headXFrame0);
		this->list[i].spriteAnim.headZFrame1 = (int16_t)e["anim_head_z_f1"].as<int>(defAnim.headZFrame1);
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
