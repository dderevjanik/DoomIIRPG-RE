#include "d1-converter.h"
#include "BspToBin.h"
#include "../engine/ZipFile.h"
#include <cstdio>

// ========================================================================
// D1RPG game definition
// ========================================================================

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
	10,   // maps
	40,   // textures
	15,   // string groups
	5,    // languages
	D1RPG_LANGUAGES,
	D1RPG_GROUPS,
};

// ========================================================================
// D1 BSP file names → level directory mapping
// ========================================================================

struct D1LevelMapping {
	const char* bspName;    // filename inside DoomRPG.zip (e.g. "DoomRPG/intro.bsp")
	const char* levelDir;   // output level directory name
	int mapId;              // 1-based map ID
};

static const D1LevelMapping D1_LEVELS[] = {
	{"intro.bsp",                "01_sector1",  1},
	{"junction.bsp",             "02_sector2",  2},
	{"level01.bsp",              "03_sector3",  3},
	{"level02.bsp",              "04_sector4",  4},
	{"level03.bsp",              "05_sector5",  5},
	{"level04.bsp",              "06_sector6",  6},
	{"level05.bsp",              "07_sector7",  7},
	{"level06.bsp",              "08_sector8",  8},
	{"level07.bsp",              "09_junction",  9},
	{"junction_destroyed.bsp",   "10_reactor",  10},
	{"reactor.bsp",              "11_enpro",    11},
};
static constexpr int D1_NUM_LEVELS = sizeof(D1_LEVELS) / sizeof(D1_LEVELS[0]);

// ========================================================================
// Generate level.yaml for a converted D1 level
// ========================================================================

static std::string generateLevelYaml(const BspToBin::Result& r, const D1LevelMapping& lvl) {
	static const char* DIR_NAMES[] = {
		"east", "northeast", "north", "northwest",
		"west", "southwest", "south", "southeast"
	};

	int spawnX = r.spawnIndex % 32;
	int spawnY = r.spawnIndex / 32;
	const char* dirName = DIR_NAMES[r.spawnDir & 7];

	std::string yaml;
	yaml += "# D1RPG converted level: " + std::string(r.mapName) + "\n\n";
	yaml += "name: " + std::string(lvl.levelDir) + "\n";
	yaml += "map_id: " + std::to_string(lvl.mapId) + "\n\n";

	yaml += "player_spawn:\n";
	yaml += "  x: " + std::to_string(spawnX) + "\n";
	yaml += "  y: " + std::to_string(spawnY) + "\n";
	yaml += "  dir: " + std::string(dirName) + "\n\n";

	yaml += "total_secrets: 0\n";
	yaml += "total_loot: 0\n\n";

	yaml += "sky_box: sky_hell\n\n";

	yaml += "media_indices:\n";
	yaml += "  - texture_258\n";
	yaml += "  - texture_451\n";
	yaml += "  - texture_462\n";
	yaml += "  - door_unlocked\n";
	yaml += "  - sky_box\n";
	yaml += "  - fade\n\n";

	// Door sprites
	if (!r.doors.empty()) {
		yaml += "sprites:\n";
		for (auto& d : r.doors) {
			yaml += "  - x: " + std::to_string(d.midX) + "\n";
			yaml += "    y: " + std::to_string(d.midY) + "\n";
			yaml += "    tile: door_unlocked\n";
			yaml += "    flags: [animation, ";
			yaml += d.horizontal ? "north, south" : "east, west";
			yaml += "]\n";
		}
	} else {
		yaml += "sprites: []\n";
	}

	return yaml;
}

// ========================================================================
// Convert D1 RPG levels (.bsp → .bin)
// ========================================================================

bool convertD1Levels(ZipFile& zip, const GameDef& game, const std::string& outputDir) {
	printf("\n[d1-converter] Converting D1 RPG levels...\n");

	int converted = 0;

	for (int i = 0; i < D1_NUM_LEVELS; i++) {
		auto& lvl = D1_LEVELS[i];

		// Read .bsp from zip
		int bspLen = 0;
		uint8_t* bspData = zip.readZipFileEntry(lvl.bspName, &bspLen);
		if (!bspData || bspLen <= 0) {
			printf("  [%s] BSP not found in archive, skipping\n", lvl.bspName);
			continue;
		}

		// Convert .bsp → .bin
		BspToBin::Result result = BspToBin::convert(bspData, bspLen);
		free(bspData);

		if (result.binData.empty()) {
			printf("  [%s] Conversion failed, skipping\n", lvl.bspName);
			continue;
		}

		// Create level directory
		std::string levelPath = outputDir + "/levels/" + lvl.levelDir;
		mkdirRecursive(levelPath);

		// Write map.bin
		writeFile(levelPath + "/map.bin",
		          result.binData.data(), (int)result.binData.size());

		// Write level.yaml
		std::string yaml = generateLevelYaml(result, lvl);
		writeString(levelPath + "/level.yaml", yaml);

		// Write empty scripts.yaml
		writeString(levelPath + "/scripts.yaml",
		            "# Empty scripts for D1RPG converted level\n\nstatic_funcs: {}\ntile_events: []\n");

		// Write empty strings.yaml
		writeString(levelPath + "/strings.yaml",
		            "# D1RPG level strings\nstrings: []\n");

		printf("  %-35s -> %s/map.bin  (polys=%d lines=%d nodes=%d doors=%d)\n",
		       lvl.bspName, lvl.levelDir,
		       result.numPolys, result.numLines, result.numNodes,
		       (int)result.doors.size());
		converted++;
	}

	printf("[d1-converter] Converted %d/%d levels\n\n", converted, D1_NUM_LEVELS);
	return converted > 0;
}
