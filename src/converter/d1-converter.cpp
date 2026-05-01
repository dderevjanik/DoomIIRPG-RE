#include "d1-converter.h"
#include "BspToBin.h"
#include "ZipFile.h"
#include <cstdio>
#include <cstring>
#include <format>
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
		// D1 palettes.bin stores BGR565: B in high bits, R in low bits
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

	// Decode a 64x64 D1 texture at native resolution.
	// The engine's GL_REPEAT will handle tiling with UV 0-16.
	static constexpr int TEX_SIZE = 64;

	auto decodeTex64 = [&](int texelOffset, int paletteOffset, uint8_t* out) {
		int base = 4 + texelOffset;
		for (int y = 0; y < TEX_SIZE; y++) {
			for (int x = 0; x < TEX_SIZE; x += 2) {
				int byteIdx = base + (y * 32) + (x / 2);
				if (byteIdx < 0 || byteIdx >= wtxLen) continue;
				uint8_t byteVal = wtxData[byteIdx];
				int p1 = paletteOffset + (byteVal & 0x0F);
				int p2 = paletteOffset + ((byteVal >> 4) & 0x0F);
				if (p1 >= 0 && p1 < numColors) {
					out[(y*TEX_SIZE+x)*3+0] = palette[p1].r;
					out[(y*TEX_SIZE+x)*3+1] = palette[p1].g;
					out[(y*TEX_SIZE+x)*3+2] = palette[p1].b;
				}
				if (p2 >= 0 && p2 < numColors) {
					out[(y*TEX_SIZE+x+1)*3+0] = palette[p2].r;
					out[(y*TEX_SIZE+x+1)*3+1] = palette[p2].g;
					out[(y*TEX_SIZE+x+1)*3+2] = palette[p2].b;
				}
			}
		}
	};

	std::string texDir = outputDir + "/levels/textures";
	mkdirRecursive(texDir);

	// 1. Export horizontal atlas of all raw texel entries (d1_wtexels.png)
	std::vector<int> uniqueTexelIds;
	{
		std::vector<bool> seen(texelCnt / 2, false);
		for (int i = 0; i < textureCnt; i++) {
			int tid = textureIds[i];
			if (tid < 0 || tid * 2 + 1 >= texelCnt || seen[tid]) continue;
			seen[tid] = true;
			uniqueTexelIds.push_back(tid);
		}
	}
	if (!uniqueTexelIds.empty()) {
		int cnt = (int)uniqueTexelIds.size();
		int atlasW = cnt * TEX_SIZE;
		std::vector<uint8_t> atlasImg(atlasW * TEX_SIZE * 3, 0);
		for (int i = 0; i < cnt; i++) {
			std::vector<uint8_t> tmp(TEX_SIZE * TEX_SIZE * 3, 0);
			decodeTex64(texelOffsets[uniqueTexelIds[i]*2], texelOffsets[uniqueTexelIds[i]*2+1], tmp.data());
			for (int y = 0; y < TEX_SIZE; y++)
				memcpy(&atlasImg[(y * atlasW + i * TEX_SIZE) * 3], &tmp[y * TEX_SIZE * 3], TEX_SIZE * 3);
		}
		std::string atlasPath = texDir + "/d1_wtexels.png";
		stbi_write_png(atlasPath.c_str(), atlasW, TEX_SIZE, 3, atlasImg.data(), atlasW * 3);
		printf("  Atlas: %d textures as %dx%d: %s\n", cnt, atlasW, TEX_SIZE, atlasPath.c_str());
	}

	// 2. Export individual PNGs per BSP texture index (d1_wall_NNN.png)
	//    BSP line texture → textureIds[bsp_tex] → actual texel/palette
	int count = 0;
	std::vector<bool> bspExported(textureCnt, false);
	for (int bspTex = 0; bspTex < textureCnt; bspTex++) {
		int actualId = textureIds[bspTex];
		if (actualId < 0 || actualId * 2 + 1 >= texelCnt) continue;
		if (bspExported[bspTex]) continue;

		std::vector<uint8_t> pixels(TEX_SIZE * TEX_SIZE * 3, 0);
		decodeTex64(texelOffsets[actualId * 2], texelOffsets[actualId * 2 + 1], pixels.data());

		auto filename = std::format("d1_wall_{:03d}.png", bspTex);
		stbi_write_png((texDir + "/" + filename).c_str(), TEX_SIZE, TEX_SIZE, 3, pixels.data(), TEX_SIZE * 3);

		bspExported[bspTex] = true;
		count++;
	}

	printf("  Individual: %d wall textures (d1_wall_NNN.png)\n", count);

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
	yaml += "id: " + std::to_string(lvl.mapId) + "\n\n";

	yaml += "player_spawn:\n";
	yaml += "  x: " + std::to_string(spawnX) + "\n";
	yaml += "  y: " + std::to_string(spawnY) + "\n";
	yaml += "  dir: " + std::string(dirName) + "\n\n";

	yaml += "total_secrets: 0\n";
	yaml += "total_loot: 0\n\n";

	yaml += "no_fog: true\n\n";

	yaml += "textures:\n";
	yaml += "  - texture_258\n";
	yaml += "  - texture_451\n";
	yaml += "  - texture_462\n";
	yaml += "  - door_unlocked\n";
	yaml += "  - sky_box\n";
	yaml += "  - fade\n\n";

	// Door sprites
	if (!r.doors.empty()) {
		yaml += "entities:\n";
		for (auto& d : r.doors) {
			yaml += "  - x: " + std::to_string(d.midX) + "\n";
			yaml += "    y: " + std::to_string(d.midY) + "\n";
			yaml += "    tile: door_unlocked\n";
			yaml += "    flags: [animation, ";
			yaml += d.horizontal ? "north, south" : "east, west";
			yaml += "]\n";
		}
	} else {
		yaml += "entities: []\n";
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
		          std::span(result.binData));

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
