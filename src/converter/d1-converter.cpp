#include "d1-converter.h"
#include "BspToBin.h"
#include "../engine/ZipFile.h"
#include <cstdio>
#include <cstring>
#include <stb_image_write.h>

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
// Extract D1 wall textures to PNG (one horizontal strip per texture file)
// ========================================================================

bool extractD1Textures(ZipFile& zip, const std::string& outputDir) {
	printf("\n[d1-converter] Extracting D1 textures...\n");

	// Read binary files from zip
	int palLen = 0, mapLen = 0, wtxLen = 0;
	uint8_t* palData = zip.readZipFileEntry("palettes.bin", &palLen);
	uint8_t* mapData = zip.readZipFileEntry("mappings.bin", &mapLen);
	uint8_t* wtxData = zip.readZipFileEntry("wtexels.bin", &wtxLen);

	if (!palData || !mapData || !wtxData) {
		printf("  Missing D1 texture files (palettes.bin, mappings.bin, wtexels.bin)\n");
		if (palData) free(palData);
		if (mapData) free(mapData);
		if (wtxData) free(wtxData);
		return false;
	}

	// Parse palettes (BGR565 → RGB8)
	int palPos = 0;
	uint32_t palDataSize = *(uint32_t*)(palData + palPos); palPos += 4;
	int numColors = palDataSize / 2;
	struct RGB { uint8_t r, g, b; };
	std::vector<RGB> palette(numColors);
	for (int i = 0; i < numColors; i++) {
		uint16_t c = *(uint16_t*)(palData + palPos); palPos += 2;
		uint8_t b5 = (c >> 11) & 0x1F;
		uint8_t g6 = (c >> 5) & 0x3F;
		uint8_t r5 = c & 0x1F;
		palette[i] = {
			(uint8_t)((r5 << 3) | (r5 >> 2)),
			(uint8_t)((g6 << 2) | (g6 >> 4)),
			(uint8_t)((b5 << 3) | (b5 >> 2))
		};
	}

	// Parse mappings
	int mPos = 0;
	int texelCntRaw = *(int32_t*)(mapData + mPos); mPos += 4;
	int bitshapeCntRaw = *(int32_t*)(mapData + mPos); mPos += 4;
	int textureCnt = *(int32_t*)(mapData + mPos); mPos += 4;
	int spriteCnt = *(int32_t*)(mapData + mPos); mPos += 4;

	int texelCnt = texelCntRaw * 2;
	std::vector<int32_t> texelOffsets(texelCnt);
	for (int i = 0; i < texelCnt; i++) {
		texelOffsets[i] = *(int32_t*)(mapData + mPos); mPos += 4;
	}

	// Skip bitshape offsets
	mPos += bitshapeCntRaw * 2 * 4;

	// Read texture IDs
	std::vector<int16_t> textureIds(textureCnt);
	for (int i = 0; i < textureCnt; i++) {
		textureIds[i] = *(int16_t*)(mapData + mPos); mPos += 2;
	}

	// Collect unique texture IDs in order
	std::vector<int> uniqueIds;
	std::vector<bool> seen(texelCnt / 2, false);
	for (int texIdx = 0; texIdx < textureCnt; texIdx++) {
		int texId = textureIds[texIdx];
		if (texId < 0 || texId * 2 + 1 >= texelCnt) continue;
		if (seen[texId]) continue;
		seen[texId] = true;
		uniqueIds.push_back(texId);
	}

	int count = (int)uniqueIds.size();
	if (count == 0) {
		printf("  No textures found\n");
		free(palData); free(mapData); free(wtxData);
		return false;
	}

	// Build one horizontal atlas: count * 64 wide, 64 tall
	int atlasW = count * 64, atlasH = 64;
	std::vector<uint8_t> atlas(atlasW * atlasH * 3, 0);

	for (int i = 0; i < count; i++) {
		int texId = uniqueIds[i];
		int texelOffset = texelOffsets[texId * 2];
		int paletteOffset = texelOffsets[texId * 2 + 1];
		int base = 4 + texelOffset;

		for (int y = 0; y < 64; y++) {
			for (int x = 0; x < 64; x += 2) {
				int byteIdx = base + (y * 32) + (x / 2);
				if (byteIdx < 0 || byteIdx >= wtxLen) continue;

				uint8_t byteVal = wtxData[byteIdx];
				int pix1 = byteVal & 0x0F;
				int pix2 = (byteVal >> 4) & 0x0F;
				int palIdx1 = paletteOffset + pix1;
				int palIdx2 = paletteOffset + pix2;

				int ax = i * 64 + x; // atlas X position
				if (palIdx1 >= 0 && palIdx1 < numColors) {
					atlas[(y * atlasW + ax) * 3 + 0] = palette[palIdx1].r;
					atlas[(y * atlasW + ax) * 3 + 1] = palette[palIdx1].g;
					atlas[(y * atlasW + ax) * 3 + 2] = palette[palIdx1].b;
				}
				if (palIdx2 >= 0 && palIdx2 < numColors) {
					atlas[(y * atlasW + (ax + 1)) * 3 + 0] = palette[palIdx2].r;
					atlas[(y * atlasW + (ax + 1)) * 3 + 1] = palette[palIdx2].g;
					atlas[(y * atlasW + (ax + 1)) * 3 + 2] = palette[palIdx2].b;
				}
			}
		}
	}

	// Write atlas PNG
	std::string texDir = outputDir + "/levels/textures";
	mkdirRecursive(texDir);
	std::string outPath = texDir + "/d1_wtexels.png";
	stbi_write_png(outPath.c_str(), atlasW, atlasH, 3, atlas.data(), atlasW * 3);

	printf("  Exported %d wall textures as %dx%d atlas: %s\n",
	       count, atlasW, atlasH, outPath.c_str());

	free(palData);
	free(mapData);
	free(wtxData);
	return true;
}

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
