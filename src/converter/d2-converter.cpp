#include "d2-converter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ZipFile.h"
#include "MapDataIO.h"
#include <yaml-cpp/yaml.h>
#include "resources_embedded.h"

// ========================================================================
// D2RPG game definition
// ========================================================================
static const char* D2RPG_LEVEL_DIRS[] = {
	"01_tycho", "02_cichus", "03_kepler", "04_uac_admin", "05_uac_labs",
	"06_uac_r_d", "07_gehenna", "08_abaddon", "09_vios", "10_test_map"
};
static const char* D2RPG_LANGUAGES[] = {"english", "french", "german", "italian", "spanish"};
static const char* D2RPG_GROUPS[] = {
	"CodeStrings", "EntityStrings", "FileStrings", "MenuStrings",
	"M_01", "M_02", "M_03", "M_04", "M_05", "M_06", "M_07", "M_08", "M_09",
	"M_TEST", "Translations"
};

const GameDef GAME_DOOM2RPG = {
	"doom2rpg",
	"D2RPG",
	"Payload/Doom2rpg.app/",
	"Payload/Doom2rpg.app/Packages/",
	"games/doom2rpg",
	"sounds2",
	D2RPG_LEVEL_DIRS, 10,
	10,   // maps
	40,   // textures
	15,   // string groups
	5,    // languages
	D2RPG_LANGUAGES,
	D2RPG_GROUPS,
};

// Active game pointer (set by convertGame)
static const GameDef* g_game = nullptr;

// ========================================================================
// Level directory helpers
// ========================================================================
static std::string getLevelDirName(int mapId) {
	if (mapId >= 1 && mapId <= g_game->numLevels)
		return g_game->levelDirs[mapId - 1];
	return "map" + std::to_string(mapId);
}

static bool createLevelDirectories(const std::string& outDir) {
	for (int mapId = 1; mapId <= g_game->numLevels; mapId++) {
		std::string dir = outDir + "/levels/" + getLevelDirName(mapId);
		if (!mkdirRecursive(dir)) {
			fprintf(stderr, "  Failed to create level directory: %s\n", dir.c_str());
			return false;
		}
	}
	return true;
}

// ========================================================================
// Conversion: strings.idx + strings00-02.bin -> strings.yaml
// ========================================================================
static bool convertStrings(ZipFile& zip, const std::string& outDir) {
	// Read strings.idx
	int idxSize = 0;
	std::string idxPath = std::string(g_game->pkgPrefix) + "strings.idx";
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
		std::string chunkPath = std::string(g_game->pkgPrefix) + fname;
		chunks[i] = zip.readZipFileEntry(chunkPath.c_str(), &chunkSizes[i]);
		if (chunks[i]) {
			printf("  %s: %d bytes\n", fname, chunkSizes[i]);
		}
	}

	// Parse file index (same logic as Resource::loadFileIndex)
	int totalEntries = readShort(idxData, 0);
	int arraySize = g_game->maxLanguages * g_game->maxStringGroups * 3 + 10;
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
		std::vector<std::vector<std::string>> strings;
		GroupData() : strings(g_game->maxLanguages) {}
	};
	std::vector<GroupData> groupData(g_game->maxStringGroups);

	int totalStrings = 0;
	for (int lang = 0; lang < g_game->maxLanguages; lang++) {
		for (int grp = 0; grp < g_game->maxStringGroups; grp++) {
			int idxPos = (grp + lang * g_game->maxStringGroups) * 3;
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
	for (int grp = 0; grp < g_game->maxStringGroups; grp++) {
		std::string yaml;
		yaml += "# " + std::string(g_game->groupNames[grp]) + " (group " + std::to_string(grp) + ") — " + GROUP_DESCRIPTIONS[grp] + "\n";
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
		for (int lang = 0; lang < g_game->maxLanguages; lang++) {
			if (!groupData[grp].strings[lang].empty()) {
				hasData = true;
				yaml += std::string(g_game->languageNames[lang]) + ":\n";
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
			filePath = stringsDir + "/" + g_game->groupNames[grp] + ".yaml";
			printPath = "strings/" + std::string(g_game->groupNames[grp]) + ".yaml";
		}
		if (!writeString(filePath, yaml)) {
			fprintf(stderr, "  Failed to write %s\n", filePath.c_str());
			return false;
		}
		int grpTotal = 0;
		for (int l = 0; l < g_game->maxLanguages; l++) grpTotal += (int)groupData[grp].strings[l].size();
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
// Copy all BMP image files from the IPA Packages directory
// ========================================================================
static std::string classifyImage(const std::string& relPath) {
	if (relPath.compare(0, 10, "ComicBook/") == 0) {
		return "comicbook/" + relPath.substr(10);
	}

	std::string name = relPath;
	size_t slash = name.rfind('/');
	if (slash != std::string::npos) {
		name = name.substr(slash + 1);
	}

	if (name.compare(0, 4, "Font") == 0 || name == "WarFont.bmp") {
		return "fonts/" + name;
	}

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

	return "ui/" + name;
}

static void copyImageAssets(ZipFile& zip, const std::string& outDir) {
	const std::string prefix = std::string(g_game->pkgPrefix);
	int copied = 0;

	for (int i = 0; i < zip.getEntryCount(); i++) {
		const char* name = zip.getEntryName(i);
		if (!name) continue;
		std::string entry(name);

		if (entry.compare(0, prefix.size(), prefix) != 0) continue;

		if (entry.size() < 4) continue;
		std::string ext = entry.substr(entry.size() - 4);
		if (ext != ".bmp" && ext != ".BMP") continue;

		std::string relPath = entry.substr(prefix.size());
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

static void copyAudioAssets(ZipFile& zip, const std::string& outDir) {
	std::string audioDir = outDir + "/audio";
	mkdirRecursive(audioDir);

	const std::string prefix = std::string(g_game->pkgPrefix) + g_game->soundsDir + "/";
	int copied = 0;

	for (int i = 0; i < zip.getEntryCount(); i++) {
		const char* name = zip.getEntryName(i);
		if (!name) continue;
		std::string entry(name);

		if (entry.compare(0, prefix.size(), prefix) != 0) continue;

		std::string fileName = entry.substr(prefix.size());
		if (fileName.empty()) continue;

		int size = 0;
		uint8_t* data = zip.readZipFileEntry(name, &size);
		if (data) {
			writeFile(audioDir + "/" + fileName, data, size);
			free(data);
			copied++;
		}
	}
	printf("  Audio assets: %d WAV files copied\n", copied);
}

// ========================================================================
// Extract sprite placement data from map binaries into level.yaml
// ========================================================================
static bool extractSpritesToYaml(const std::string& outDir) {
	// Build tile ID -> sprite name reverse lookup from sprites.yaml
	std::map<int, std::string> tileIdToName;
	{
		YAML::Node root = YAML::LoadFile(outDir + "/sprites.yaml");
		YAML::Node sprites = root["sprites"];
		if (sprites) {
			for (auto it = sprites.begin(); it != sprites.end(); ++it) {
				std::string name = it->first.as<std::string>();
				auto& val = it->second;
				if (val["file"] && val["file"].as<std::string>() == "tables.bin" && val["id"]) {
					int id = val["id"].as<int>();
					if (tileIdToName.find(id) == tileIdToName.end()) {
						tileIdToName[id] = name;
					}
				}
			}
		}
		printf("  Loaded %d sprite names for tile lookup\n", (int)tileIdToName.size());
	}

	// Collect all media indices from all maps and add unnamed ones to sprites.yaml
	{
		std::set<int> allMediaIds;
		for (int mapId = 1; mapId <= g_game->numLevels; mapId++) {
			std::string dirName = getLevelDirName(mapId);
			std::string mapBinPath = outDir + "/levels/" + dirName + "/map.bin";
			MapData md;
			std::string err;
			if (MapDataIO::loadFromBin(mapBinPath, md, err)) {
				for (auto idx : md.mediaIndices) allMediaIds.insert(idx);
			}
		}

		std::string appendYaml;
		int added = 0;
		for (int id : allMediaIds) {
			if (tileIdToName.find(id) == tileIdToName.end()) {
				char name[32];
				snprintf(name, sizeof(name), "texture_%03d", id);
				tileIdToName[id] = name;
				appendYaml += "  " + std::string(name) + ": {file: tables.bin, id: " + std::to_string(id) + "}\n";
				added++;
			}
		}
		if (added > 0) {
			std::string spritesPath = outDir + "/sprites.yaml";
			std::string existing;
			FILE* f = fopen(spritesPath.c_str(), "r");
			if (f) {
				fseek(f, 0, SEEK_END);
				long sz = ftell(f);
				fseek(f, 0, SEEK_SET);
				existing.resize(sz);
				fread(&existing[0], 1, sz, f);
				fclose(f);
			}
			std::string header = "\n  # Wall/floor textures (auto-generated from map media indices)\n";
			writeString(spritesPath, existing + header + appendYaml);
			printf("  Added %d texture names to sprites.yaml\n", added);
		}
	}

	for (int mapId = 1; mapId <= g_game->numLevels; mapId++) {
		std::string dirName = getLevelDirName(mapId);
		std::string mapBinPath = outDir + "/levels/" + dirName + "/map.bin";
		std::string levelYamlPath = outDir + "/levels/" + dirName + "/level.yaml";

		MapData mapData;
		std::string errorMsg;
		if (!MapDataIO::loadFromBin(mapBinPath, mapData, errorMsg)) {
			fprintf(stderr, "  Failed to load %s: %s\n", mapBinPath.c_str(), errorMsg.c_str());
			return false;
		}

		int totalSprites = (int)mapData.normalSprites.size() + (int)mapData.zSprites.size();

		std::string yaml;

		// Player spawn point
		{
			int spawnCol = mapData.spawnIndex % 32;
			int spawnRow = mapData.spawnIndex / 32;
			static const char* dirNames[] = {"east", "1", "south", "3", "west", "5", "north", "7"};
			const char* dirName2 = (mapData.spawnDir < 8) ? dirNames[mapData.spawnDir] : "east";
			yaml += "\nplayer_spawn:\n";
			yaml += "  x: " + std::to_string(spawnCol) + "\n";
			yaml += "  y: " + std::to_string(spawnRow) + "\n";
			yaml += "  dir: " + std::string(dirName2) + "\n";
		}

		// Header values
		yaml += "\ntotal_secrets: " + std::to_string(mapData.totalSecrets) + "\n";
		yaml += "total_loot: " + std::to_string(mapData.totalLoot) + "\n";
		if (mapData.flagsBitmask != 0) {
			yaml += "flags_bitmask: " + std::to_string(mapData.flagsBitmask) + "\n";
		}

		// Sky box sprite name (derived from original engine formula)
		const char* skyName = (((mapId - 1) / 5 % 2) == 0) ? "sky_earth" : "sky_hell";
		yaml += "sky_box: " + std::string(skyName) + "\n";

		// Media indices
		if (!mapData.mediaIndices.empty()) {
			yaml += "\nmedia_indices:\n";
			for (auto idx : mapData.mediaIndices) {
				auto nameIt = tileIdToName.find((int)idx);
				if (nameIt != tileIdToName.end()) {
					yaml += "  - " + nameIt->second + "\n";
				} else {
					yaml += "  - " + std::to_string(idx) + "\n";
				}
			}
		}

		// Maya cameras
		if (!mapData.mayaCameras.empty()) {
			yaml += "\ncameras:\n";
			for (const auto& cam : mapData.mayaCameras) {
				yaml += "  - sample_rate: " + std::to_string(cam.sampleRate) + "\n";
				if (cam.numKeys > 0) {
					yaml += "    keys:\n";
					for (int k = 0; k < cam.numKeys; k++) {
						int16_t x     = cam.keyData[0 * cam.numKeys + k];
						int16_t y     = cam.keyData[1 * cam.numKeys + k];
						int16_t z     = cam.keyData[2 * cam.numKeys + k];
						int16_t pitch = cam.keyData[3 * cam.numKeys + k];
						int16_t yaw   = cam.keyData[4 * cam.numKeys + k];
						int16_t roll  = cam.keyData[5 * cam.numKeys + k];
						int16_t ms    = cam.keyData[6 * cam.numKeys + k];
						yaml += "      - {x: " + std::to_string(x) + ", y: " + std::to_string(y)
						     + ", z: " + std::to_string(z) + ", pitch: " + std::to_string(pitch)
						     + ", yaw: " + std::to_string(yaw) + ", roll: " + std::to_string(roll)
						     + ", ms: " + std::to_string(ms) + "}\n";
					}

					yaml += "    tween_indices:\n";
					for (int k = 0; k < cam.numKeys; k++) {
						yaml += "      - [";
						for (int ch = 0; ch < 6; ch++) {
							if (ch > 0) yaml += ", ";
							yaml += std::to_string(cam.tweenIndices[k * 6 + ch]);
						}
						yaml += "]\n";
					}

					yaml += "    tween_counts: [";
					for (int ch = 0; ch < 6; ch++) {
						if (ch > 0) yaml += ", ";
						yaml += std::to_string(cam.tweenCounts[ch]);
					}
					yaml += "]\n";

					if (!cam.tweenData.empty()) {
						yaml += "    tween_data: [";
						for (int j = 0; j < (int)cam.tweenData.size(); j++) {
							if (j > 0) yaml += ", ";
							yaml += std::to_string(cam.tweenData[j]);
						}
						yaml += "]\n";
					}
				}
			}
		}

		// Height map (32x32 grid)
		yaml += "\nheight_map:\n";
		for (int row = 0; row < 32; row++) {
			yaml += "  - [";
			for (int col = 0; col < 32; col++) {
				if (col > 0) yaml += ", ";
				yaml += std::to_string(mapData.tiles[row * 32 + col].height);
			}
			yaml += "]\n";
		}

		// Generate scripts.yaml
		{
			std::string sYaml;
			sYaml += "# Script data for " + getLevelDirName(mapId) + "\n\n";

			sYaml += "static_funcs:\n";
			static const char* sfNames[] = {
				"init_map", "end_game", "boss_75", "boss_50", "boss_25", "boss_dead",
				"per_turn", "attack_npc", "monster_death", "monster_activate",
				"chicken_kicked", "item_pickup"
			};
			for (int i = 0; i < 12; i++) {
				if (mapData.staticFuncs[i] != 0xFFFF) {
					sYaml += "  " + std::string(sfNames[i]) + ": " + std::to_string(mapData.staticFuncs[i]) + "\n";
				}
			}

			if (!mapData.tileEvents.empty()) {
				sYaml += "\ntile_events:\n";
				for (const auto& ev : mapData.tileEvents) {
					int tileIdx = ev.packed & 0x3FF;
					int ip = (ev.packed >> 16) & 0xFFFF;
					int col = tileIdx % 32;
					int row = tileIdx / 32;

					sYaml += "  - tile: {x: " + std::to_string(col) + ", y: " + std::to_string(row) + "}\n";
					sYaml += "    ip: " + std::to_string(ip) + "\n";

					int param = ev.param;
					std::string triggers;
					if (param & 0x1) triggers += "enter, ";
					if (param & 0x2) triggers += "exit, ";
					if (param & 0x4) triggers += "trigger, ";
					if (param & 0x8) triggers += "face, ";
					if (!triggers.empty()) {
						triggers = triggers.substr(0, triggers.size() - 2);
						sYaml += "    on: [" + triggers + "]\n";
					}

					std::string dirs;
					if (param & 0x010) dirs += "east, ";
					if (param & 0x020) dirs += "northeast, ";
					if (param & 0x040) dirs += "north, ";
					if (param & 0x080) dirs += "northwest, ";
					if (param & 0x100) dirs += "west, ";
					if (param & 0x200) dirs += "southwest, ";
					if (param & 0x400) dirs += "south, ";
					if (param & 0x800) dirs += "southeast, ";
					if (!dirs.empty()) {
						dirs = dirs.substr(0, dirs.size() - 2);
						sYaml += "    dir: [" + dirs + "]\n";
					}

					std::string attacks;
					if (param & 0x1000) attacks += "melee, ";
					if (param & 0x2000) attacks += "ranged, ";
					if (param & 0x4000) attacks += "explosion, ";
					if (!attacks.empty()) {
						attacks = attacks.substr(0, attacks.size() - 2);
						sYaml += "    attack: [" + attacks + "]\n";
					}

					std::string eflags;
					if (param & 0x10000) eflags += "block_input, ";
					if (param & 0x20000) eflags += "exit_goto, ";
					if (param & 0x40000) eflags += "skip_turn, ";
					if (param & 0x80000) eflags += "disable, ";
					if (!eflags.empty()) {
						eflags = eflags.substr(0, eflags.size() - 2);
						sYaml += "    flags: [" + eflags + "]\n";
					}
				}
			}

			if (!mapData.byteCode.empty()) {
				sYaml += "\nbytecode: |\n";
				for (size_t i = 0; i < mapData.byteCode.size(); i += 32) {
					sYaml += "  ";
					for (size_t j = i; j < i + 32 && j < mapData.byteCode.size(); j++) {
						char hex[4];
						snprintf(hex, sizeof(hex), "%02X", mapData.byteCode[j]);
						sYaml += hex;
					}
					sYaml += "\n";
				}
			}

			std::string scriptsPath = outDir + "/levels/" + dirName + "/scripts.yaml";
			if (!writeString(scriptsPath, sYaml)) {
				fprintf(stderr, "  Failed to write %s\n", scriptsPath.c_str());
				return false;
			}
		}

		if (totalSprites == 0) {
			printf("  levels/%s: extracted (0 sprites)\n", dirName.c_str());
		}

		// Sprites section
		if (totalSprites > 0) {
		yaml += "\nsprites:\n";

		auto emitSprite = [&](const SpriteData& s, bool isZ) {
			int tile = (s.info & 0xFF);
			if (s.info & 0x400000) tile += 257;
			uint32_t flags = ((s.info >> 16) & 0xFFFF) & ~0x40u;

			yaml += "  - x: " + std::to_string(s.x) + "\n";
			yaml += "    y: " + std::to_string(s.y) + "\n";
			auto nameIt = tileIdToName.find(tile);
			if (nameIt != tileIdToName.end()) {
				yaml += "    tile: " + nameIt->second + "\n";
			} else {
				yaml += "    tile: " + std::to_string(tile) + "\n";
			}
			if (flags != 0) {
				struct FlagDef { uint32_t bit; const char* name; };
				static const FlagDef flagDefs[] = {
					{0x0001, "invisible"},
					{0x0002, "flip_h"},
					{0x0010, "animation"},
					{0x0020, "non_entity"},
					{0x0080, "sprite_wall"},
					{0x0100, "south"},
					{0x0200, "north"},
					{0x0400, "east"},
					{0x0800, "west"},
				};
				yaml += "    flags: [";
				bool first = true;
				uint32_t remaining = flags;
				for (const auto& fd : flagDefs) {
					if (remaining & fd.bit) {
						if (!first) yaml += ", ";
						yaml += fd.name;
						first = false;
						remaining &= ~fd.bit;
					}
				}
				if (remaining != 0) {
					char buf[16];
					snprintf(buf, sizeof(buf), "0x%X", remaining);
					if (!first) yaml += ", ";
					yaml += buf;
				}
				yaml += "]\n";
			}
			if (isZ) {
				yaml += "    z: " + std::to_string(s.z) + "\n";
				int zAnim = (s.info >> 8) & 0xFF;
				if (zAnim != 0) {
					yaml += "    z_anim: " + std::to_string(zAnim) + "\n";
				}
			}
		};

		for (const auto& s : mapData.normalSprites) emitSprite(s, false);
		for (const auto& s : mapData.zSprites) emitSprite(s, true);
		} // end if (totalSprites > 0)

		// Append to existing level.yaml
		std::string existing;
		{
			FILE* f = fopen(levelYamlPath.c_str(), "r");
			if (f) {
				fseek(f, 0, SEEK_END);
				long sz = ftell(f);
				fseek(f, 0, SEEK_SET);
				existing.resize(sz);
				fread(&existing[0], 1, sz, f);
				fclose(f);
			}
		}
		if (!writeString(levelYamlPath, existing + yaml)) {
			fprintf(stderr, "  Failed to write %s\n", levelYamlPath.c_str());
			return false;
		}

		printf("  levels/%s: %d sprites extracted\n", dirName.c_str(), totalSprites);
	}
	return true;
}

// ========================================================================
// Copy binary assets (maps, textures, models)
// ========================================================================
static void copyBinaryAssets(ZipFile& zip, const std::string& outDir) {
	std::vector<std::string> binaryFiles;
	for (int i = 0; i < g_game->numMaps; i++) {
		char buf[64];
		snprintf(buf, sizeof(buf), "Packages/map%02d.bin", i);
		binaryFiles.push_back(buf);
		snprintf(buf, sizeof(buf), "Packages/model_%04d.bin", i);
		binaryFiles.push_back(buf);
	}
	binaryFiles.push_back("Packages/newMappings.bin");
	binaryFiles.push_back("Packages/newPalettes.bin");
	for (int i = 0; i < g_game->numTextures; i++) {
		char buf[64];
		snprintf(buf, sizeof(buf), "Packages/newTexels%03d.bin", i);
		binaryFiles.push_back(buf);
	}
	binaryFiles.push_back("Packages/tables.bin");

	int copied = 0;
	for (const auto& relPath : binaryFiles) {
		std::string ipaPath = std::string(g_game->ipaPrefix) + relPath;
		int size = 0;
		if (!zip.hasEntry(ipaPath.c_str()))
			continue;
		uint8_t* data = zip.readZipFileEntry(ipaPath.c_str(), &size);
		if (data) {
			std::string outFile = relPath;
			if (outFile.compare(0, 9, "Packages/") == 0) {
				outFile = outFile.substr(9);
			}

			if (outFile.compare(0, 3, "map") == 0 && outFile.find(".bin") != std::string::npos) {
				int mapIdx = 0;
				if (sscanf(outFile.c_str(), "map%d.bin", &mapIdx) == 1) {
					int mapId = mapIdx + 1;
					outFile = "levels/" + getLevelDirName(mapId) + "/map.bin";
				}
			} else if (outFile.compare(0, 6, "model_") == 0) {
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
// Write embedded resource YAML files to output directory
// ========================================================================
static bool writeResourceFiles(const std::string& outDir) {
	printf("  Writing %zu resource files...\n", EMBEDDED_RESOURCES_COUNT);
	bool ok = true;
	for (size_t i = 0; i < EMBEDDED_RESOURCES_COUNT; i++) {
		std::string path = outDir + "/" + EMBEDDED_RESOURCES[i].path;
		size_t slash = path.rfind('/');
		if (slash != std::string::npos) {
			mkdirRecursive(path.substr(0, slash));
		}
		if (!writeString(path, std::string(EMBEDDED_RESOURCES[i].content, EMBEDDED_RESOURCES[i].length))) {
			fprintf(stderr, "  Failed to write resource: %s\n", path.c_str());
			ok = false;
		}
	}
	return ok;
}

// ========================================================================
// Public entry point: convert a Doom II RPG IPA
// ========================================================================
bool convertGame(ZipFile& zip, const GameDef& game, const std::string& outputDir) {
	g_game = &game;

	bool ok = true;

	printf("\n[Phase 1] Writing YAML assets...\n");

	printf("Writing resource files...\n");
	ok &= writeResourceFiles(outputDir);

	printf("Converting strings...\n");
	ok &= convertStrings(zip, outputDir);

	printf("\nCopying binary assets (maps, textures, models)...\n");
	copyBinaryAssets(zip, outputDir);

	printf("\nExtracting sprites to YAML...\n");
	ok &= extractSpritesToYaml(outputDir);

	printf("\nCopying audio assets...\n");
	copyAudioAssets(zip, outputDir);

	printf("\nCopying image assets...\n");
	copyImageAssets(zip, outputDir);

	return ok;
}
