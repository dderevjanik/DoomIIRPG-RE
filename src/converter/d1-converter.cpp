#include "d1-converter.h"

// ========================================================================
// D1RPG game definition
// ========================================================================
// Note: For now this uses the same IPA structure as D2RPG.
// D1RPG-specific conversion logic will be added on top later.

static const char* D1RPG_LEVEL_DIRS[] = {
	"01_sector1", "02_sector2", "03_sector3", "04_sector4", "05_sector5",
	"06_sector6", "07_sector7", "08_sector8", "09_junction", "10_reactor",
	"11_enpro"
};

static const char* D1RPG_LANGUAGES[] = {"english", "french", "german", "italian", "spanish"};

static const char* D1RPG_GROUPS[] = {
	"CodeStrings", "EntityStrings", "FileStrings", "MenuStrings",
	"M_01", "M_02", "M_03", "M_04", "M_05", "M_06", "M_07", "M_08", "M_09",
	"M_10", "M_11", "Translations"
};

const GameDef GAME_DOOM1RPG = {
	"doom1rpg",
	"D1RPG",
	"Payload/Doom2rpg.app/",          // Uses D2RPG IPA for now
	"Payload/Doom2rpg.app/Packages/",
	"games/doom1rpg",
	"sounds2",
	D1RPG_LEVEL_DIRS, 11,
	10,   // maps (same as D2RPG for now)
	40,   // textures
	15,   // string groups
	5,    // languages
	D1RPG_LANGUAGES,
	D1RPG_GROUPS,
};
