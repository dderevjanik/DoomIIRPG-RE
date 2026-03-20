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
