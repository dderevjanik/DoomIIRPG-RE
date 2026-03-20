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
#include "INIReader.h"

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

	// Try loading from entities.ini in CWD first
	FILE* f = std::fopen("entities.ini", "rb");
	if (f) {
		std::fclose(f);
		printf("EntityDefManager: loading from entities.ini\n");
		return this->loadFromINI("entities.ini");
	}

	// Fall back to binary from resource pack
	printf("EntityDefManager: loading from entities.bin (resource pack)\n");
	return this->loadFromBinary();
}

bool EntityDefManager::loadFromBinary() {
	Applet* app = CAppContainer::getInstance()->app;
	InputStream IS;

	if (!IS.loadFile(Resources::RES_ENTITIES_BIN_GZ, InputStream::LOADTYPE_RESOURCE)) {
		app->Error("getResource(%s) failed\n", Resources::RES_ENTITIES_BIN_GZ);
	}

	app->resource->read(&IS, sizeof(short));
	this->numDefs = (int)app->resource->shiftShort();
	this->list = new EntityDef[this->numDefs];

	for (int i = 0; i < this->numDefs; i++) {
		app->resource->read(&IS, 8);
		this->list[i].tileIndex = (int16_t)app->resource->shiftShort();
		this->list[i].eType = (uint8_t)app->resource->shiftByte();
		this->list[i].eSubType = (uint8_t)app->resource->shiftByte();
		this->list[i].parm = (uint8_t)app->resource->shiftByte();
		this->list[i].name = (int16_t)app->resource->shiftUByte();
		this->list[i].longName = (int16_t)app->resource->shiftUByte();
		this->list[i].description = (int16_t)app->resource->shiftUByte();
	}

	// Auto-compute render flags from tile indices (backward compat with binary data)
	for (int i = 0; i < this->numDefs; i++) {
		this->list[i].renderFlags = computeRenderFlags(this->list[i].tileIndex);
		this->list[i].fearEyes = computeDefaultFearEyes(this->list[i].eSubType);
		this->list[i].gunFlare = computeDefaultGunFlare(this->list[i].eSubType);
		this->list[i].bodyParts = computeDefaultBodyParts(this->list[i].eType, this->list[i].eSubType);
	}

	IS.~InputStream();
	printf("EntityDefManager: loaded %d entities from entities.bin\n", this->numDefs);
	return true;
}

bool EntityDefManager::loadFromINI(const char* path) {
	Applet* app = CAppContainer::getInstance()->app;
	INIReader ini;

	if (!ini.load(path)) {
		app->Error("Failed to load %s\n", path);
		return false;
	}

	int count = ini.getInt("Entities", "count", 0);
	if (count <= 0) {
		app->Error("entities.ini: invalid count\n");
		return false;
	}

	this->numDefs = count;
	this->list = new EntityDef[count];

	for (int i = 0; i < count; i++) {
		char section[32];
		snprintf(section, sizeof(section), "Entity_%d", i);

		this->list[i].tileIndex = (int16_t)ini.getInt(section, "tile_index", 0);
		this->list[i].eType = (uint8_t)entityTypeFromString(ini.getString(section, "type", "world"));
		this->list[i].eSubType = (uint8_t)ini.getInt(section, "subtype", 0);
		this->list[i].parm = (uint8_t)ini.getInt(section, "parm", 0);
		this->list[i].name = (int16_t)ini.getInt(section, "name", 0);
		this->list[i].longName = (int16_t)ini.getInt(section, "long_name", 0);
		this->list[i].description = (int16_t)ini.getInt(section, "description", 0);

		// Load render flags from INI, fall back to auto-computed defaults
		uint32_t defaultFlags = computeRenderFlags(this->list[i].tileIndex);
		this->list[i].renderFlags = EntityDef::RFLAG_NONE;
		if (ini.getBool(section, "is_floater", (defaultFlags & EntityDef::RFLAG_FLOATER) != 0)) {
			this->list[i].renderFlags |= EntityDef::RFLAG_FLOATER;
		}
		if (ini.getBool(section, "has_gun_flare", (defaultFlags & EntityDef::RFLAG_GUN_FLARE) != 0)) {
			this->list[i].renderFlags |= EntityDef::RFLAG_GUN_FLARE;
		}
		if (ini.getBool(section, "is_special_boss", (defaultFlags & EntityDef::RFLAG_SPECIAL_BOSS) != 0)) {
			this->list[i].renderFlags |= EntityDef::RFLAG_SPECIAL_BOSS;
		}

		// Load fear eye offsets, falling back to auto-computed defaults from eSubType
		EntityDef::FearEyeData defEyes = computeDefaultFearEyes(this->list[i].eSubType);
		this->list[i].fearEyes.eyeL = (int8_t)ini.getInt(section, "fear_eye_l", defEyes.eyeL);
		this->list[i].fearEyes.eyeR = (int8_t)ini.getInt(section, "fear_eye_r", defEyes.eyeR);
		this->list[i].fearEyes.zAdd = (int16_t)ini.getInt(section, "fear_eye_z_add", defEyes.zAdd);
		this->list[i].fearEyes.zAlwaysPre = (int16_t)ini.getInt(section, "fear_eye_z_always", defEyes.zAlwaysPre);
		this->list[i].fearEyes.zIdlePre = (int16_t)ini.getInt(section, "fear_eye_z_idle", defEyes.zIdlePre);
		this->list[i].fearEyes.singleEye = ini.getBool(section, "fear_single_eye", defEyes.singleEye);
		this->list[i].fearEyes.eyeLFlip = (int8_t)ini.getInt(section, "fear_eye_l_flip", defEyes.eyeLFlip);
		this->list[i].fearEyes.eyeRFlip = (int8_t)ini.getInt(section, "fear_eye_r_flip", defEyes.eyeRFlip);

		// Load gun flare offsets, falling back to auto-computed defaults from eSubType
		EntityDef::GunFlareData defFlare = computeDefaultGunFlare(this->list[i].eSubType);
		this->list[i].gunFlare.dualFlare = ini.getBool(section, "gun_flare_dual", defFlare.dualFlare);
		this->list[i].gunFlare.flash1X = (int8_t)ini.getInt(section, "gun_flare1_x", defFlare.flash1X);
		this->list[i].gunFlare.flash1Z = (int16_t)ini.getInt(section, "gun_flare1_z", defFlare.flash1Z);
		this->list[i].gunFlare.flash2X = (int8_t)ini.getInt(section, "gun_flare2_x", defFlare.flash2X);
		this->list[i].gunFlare.flash2Z = (int16_t)ini.getInt(section, "gun_flare2_z", defFlare.flash2Z);

		// Load body part offsets, falling back to auto-computed defaults
		EntityDef::BodyPartData defBody = computeDefaultBodyParts(this->list[i].eType, this->list[i].eSubType);
		this->list[i].bodyParts.idleTorsoZ = (int16_t)ini.getInt(section, "idle_torso_z", defBody.idleTorsoZ);
		this->list[i].bodyParts.idleHeadZ = (int16_t)ini.getInt(section, "idle_head_z", defBody.idleHeadZ);
		this->list[i].bodyParts.walkTorsoZ = (int16_t)ini.getInt(section, "walk_torso_z", defBody.walkTorsoZ);
		this->list[i].bodyParts.walkHeadZ = (int16_t)ini.getInt(section, "walk_head_z", defBody.walkHeadZ);
		this->list[i].bodyParts.attackTorsoZF0 =
		    (int16_t)ini.getInt(section, "attack_torso_z_f0", defBody.attackTorsoZF0);
		this->list[i].bodyParts.attackTorsoZF1 =
		    (int16_t)ini.getInt(section, "attack_torso_z_f1", defBody.attackTorsoZF1);
		this->list[i].bodyParts.attackHeadZ = (int16_t)ini.getInt(section, "attack_head_z", defBody.attackHeadZ);
		this->list[i].bodyParts.attackHeadX = (int8_t)ini.getInt(section, "attack_head_x", defBody.attackHeadX);
		this->list[i].bodyParts.noHeadOnBack = ini.getBool(section, "no_head_on_back", defBody.noHeadOnBack);
		this->list[i].bodyParts.sentryHeadFlip = ini.getBool(section, "sentry_head_flip", defBody.sentryHeadFlip);
		this->list[i].bodyParts.flipTorsoWalk = ini.getBool(section, "flip_torso_walk", defBody.flipTorsoWalk);
		this->list[i].bodyParts.noHeadOnAttack = ini.getBool(section, "no_head_on_attack", defBody.noHeadOnAttack);
		this->list[i].bodyParts.noHeadOnMancAtk = ini.getBool(section, "no_head_on_manc_atk", defBody.noHeadOnMancAtk);
		this->list[i].bodyParts.attackRevAtk2TorsoZ =
		    (int16_t)ini.getInt(section, "attack_rev_atk2_torso_z", defBody.attackRevAtk2TorsoZ);
	}

	printf("EntityDefManager: loaded %d entities from %s\n", this->numDefs, path);

	return true;
}

void EntityDefManager::exportToINI(const char* path) {
	INIReader ini;

	ini.setInt("Entities", "count", this->numDefs);

	for (int i = 0; i < this->numDefs; i++) {
		char section[32];
		snprintf(section, sizeof(section), "Entity_%d", i);

		ini.setInt(section, "tile_index", this->list[i].tileIndex);
		ini.setString(section, "type", entityTypeToString(this->list[i].eType));
		ini.setInt(section, "subtype", this->list[i].eSubType);
		ini.setInt(section, "parm", this->list[i].parm);
		ini.setInt(section, "name", this->list[i].name);
		ini.setInt(section, "long_name", this->list[i].longName);
		ini.setInt(section, "description", this->list[i].description);

		// Export render flags
		if (this->list[i].renderFlags & EntityDef::RFLAG_FLOATER) {
			ini.setBool(section, "is_floater", true);
		}
		if (this->list[i].renderFlags & EntityDef::RFLAG_GUN_FLARE) {
			ini.setBool(section, "has_gun_flare", true);
		}
		if (this->list[i].renderFlags & EntityDef::RFLAG_SPECIAL_BOSS) {
			ini.setBool(section, "is_special_boss", true);
		}

		// Export fear eye offsets for monster entities
		if (this->list[i].eType == Enums::ET_MONSTER) {
			ini.setInt(section, "fear_eye_l", this->list[i].fearEyes.eyeL);
			ini.setInt(section, "fear_eye_r", this->list[i].fearEyes.eyeR);
			ini.setInt(section, "fear_eye_z_add", this->list[i].fearEyes.zAdd);
			ini.setInt(section, "fear_eye_z_always", this->list[i].fearEyes.zAlwaysPre);
			ini.setInt(section, "fear_eye_z_idle", this->list[i].fearEyes.zIdlePre);
			ini.setBool(section, "fear_single_eye", this->list[i].fearEyes.singleEye);
			if (this->list[i].fearEyes.eyeLFlip != -128) {
				ini.setInt(section, "fear_eye_l_flip", this->list[i].fearEyes.eyeLFlip);
				ini.setInt(section, "fear_eye_r_flip", this->list[i].fearEyes.eyeRFlip);
			}
		}

		// Export gun flare offsets for entities with gun flare
		if (this->list[i].renderFlags & EntityDef::RFLAG_GUN_FLARE) {
			ini.setBool(section, "gun_flare_dual", this->list[i].gunFlare.dualFlare);
			if (this->list[i].gunFlare.dualFlare) {
				ini.setInt(section, "gun_flare1_x", this->list[i].gunFlare.flash1X);
				ini.setInt(section, "gun_flare1_z", this->list[i].gunFlare.flash1Z);
			}
			ini.setInt(section, "gun_flare2_x", this->list[i].gunFlare.flash2X);
			ini.setInt(section, "gun_flare2_z", this->list[i].gunFlare.flash2Z);
		}

		// Export body part offsets for monsters and NPCs
		if (this->list[i].eType == Enums::ET_MONSTER || this->list[i].eType == Enums::ET_NPC) {
			const auto& bp = this->list[i].bodyParts;
			if (bp.idleTorsoZ)
				ini.setInt(section, "idle_torso_z", bp.idleTorsoZ);
			if (bp.idleHeadZ)
				ini.setInt(section, "idle_head_z", bp.idleHeadZ);
			if (bp.walkTorsoZ)
				ini.setInt(section, "walk_torso_z", bp.walkTorsoZ);
			if (bp.walkHeadZ)
				ini.setInt(section, "walk_head_z", bp.walkHeadZ);
			if (bp.attackTorsoZF0)
				ini.setInt(section, "attack_torso_z_f0", bp.attackTorsoZF0);
			if (bp.attackTorsoZF1)
				ini.setInt(section, "attack_torso_z_f1", bp.attackTorsoZF1);
			if (bp.attackHeadZ)
				ini.setInt(section, "attack_head_z", bp.attackHeadZ);
			if (bp.attackHeadX)
				ini.setInt(section, "attack_head_x", bp.attackHeadX);
			if (bp.noHeadOnBack)
				ini.setBool(section, "no_head_on_back", true);
			if (bp.sentryHeadFlip)
				ini.setBool(section, "sentry_head_flip", true);
			if (bp.flipTorsoWalk)
				ini.setBool(section, "flip_torso_walk", true);
			if (bp.noHeadOnAttack)
				ini.setBool(section, "no_head_on_attack", true);
			if (bp.noHeadOnMancAtk)
				ini.setBool(section, "no_head_on_manc_atk", true);
			if (bp.attackRevAtk2TorsoZ)
				ini.setInt(section, "attack_rev_atk2_torso_z", bp.attackRevAtk2TorsoZ);
		}
	}

	if (!ini.save(path)) {
		printf("EntityDefManager: failed to export to %s\n", path);
	} else {
		printf("EntityDefManager: exported %d entities to %s\n", this->numDefs, path);
	}
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
