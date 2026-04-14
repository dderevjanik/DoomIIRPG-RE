#include "d2-converter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <format>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ZipFile.h"
#include "MapDataIO.h"
#include "TextureDecoder.h"
#include <yaml-cpp/yaml.h>
#include "resources_embedded.h"
#include "stb_image_write.h"

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
		auto fname = std::format("strings{:02d}.bin", i);
		std::string chunkPath = std::string(g_game->pkgPrefix) + fname;
		chunks[i] = zip.readZipFileEntry(chunkPath.c_str(), &chunkSizes[i]);
		if (chunks[i]) {
			printf("  %s: %d bytes\n", fname.c_str(), chunkSizes[i]);
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
				groupData[grp].strings[lang].push_back(escapeString({chunks[chunkId] + pos, static_cast<size_t>(len)}));
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
	if (relPath.starts_with("ComicBook/")) {
		return "comicbook/" + relPath.substr(10);
	}

	std::string name = relPath;
	size_t slash = name.rfind('/');
	if (slash != std::string::npos) {
		name = name.substr(slash + 1);
	}

	if (name.starts_with("Font") || name == "WarFont.bmp") {
		return "fonts/" + name;
	}

	if (name.starts_with("Hud") || name.starts_with("HUD") ||
	    name.starts_with("hud") ||
	    name == "cockpit.bmp" || name == "damage.bmp" || name == "damage_bot.bmp" ||
	    name == "Automap_Cursor.bmp" || name == "ui_images.bmp" ||
	    name == "DialogScroll.bmp" ||
	    name.starts_with("Switch") ||
	    name == "Soft_Key_Fill.bmp" ||
	    name == "scope.bmp" || name.starts_with("SniperScope") ||
	    name == "inGame_menu_softkey.bmp" ||
	    name.starts_with("vending_softkey")) {
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

		if (!entry.starts_with(prefix)) continue;

		if (!entry.ends_with(".bmp") && !entry.ends_with(".BMP")) continue;

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
			writeFile(outPath, {data, static_cast<size_t>(size)});
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

		if (!entry.starts_with(prefix)) continue;

		std::string fileName = entry.substr(prefix.size());
		if (fileName.empty()) continue;

		int size = 0;
		uint8_t* data = zip.readZipFileEntry(name, &size);
		if (data) {
			writeFile(audioDir + "/" + fileName, {data, static_cast<size_t>(size)});
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
				auto name = std::format("texture_{:03d}", id);
				tileIdToName[id] = name;
				appendYaml += "  " + name + ": {file: tables.bin, id: " + std::to_string(id) + "}\n";
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

		// Sky box PNG texture (extracted from tables.bin)
		const char* skyName = (((mapId - 1) / 5 % 2) == 0) ? "sky_earth" : "sky_hell";
		yaml += "sky_texture: textures/" + std::string(skyName) + ".png\n";

		// Media indices
		if (!mapData.mediaIndices.empty()) {
			yaml += "\nmedia_indices:\n";
			for (auto idx : mapData.mediaIndices) {
				if (auto nameIt = tileIdToName.find((int)idx); nameIt != tileIdToName.end()) {
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
						sYaml += std::format("{:02X}", mapData.byteCode[j]);
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
			if (auto nameIt = tileIdToName.find(tile); nameIt != tileIdToName.end()) {
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
					if (!first) yaml += ", ";
					yaml += std::format("0x{:X}", remaining);
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
// Helper: read a binary file into a vector
// ========================================================================
static std::vector<uint8_t> readBinFile(const std::string& path) {
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) return {};
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::vector<uint8_t> buf(sz);
	fread(buf.data(), 1, sz, f);
	fclose(f);
	return buf;
}

// Helper: read a little-endian int32 from a byte buffer
static int32_t readLE32(const uint8_t* p) {
	return (int32_t)p[0] | ((int32_t)p[1] << 8) | ((int32_t)p[2] << 16) | ((int32_t)p[3] << 24);
}

// Helper: read a little-endian uint16 from a byte buffer
static uint16_t readLE16(const uint8_t* p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

// Helper: read a little-endian int16 from a byte buffer
static int16_t readLE16s(const uint8_t* p) {
	return (int16_t)readLE16(p);
}

// ========================================================================
// Extract sky textures from tables.bin (tables 16-19) as PNG
// ========================================================================
static bool extractSkyTextures(const std::string& outDir, const std::vector<uint8_t>& data) {
	if (data.size() < 80) {
		fprintf(stderr, "  Failed to read tables.bin or file too small\n");
		return false;
	}

	// Read 20 table offsets from 80-byte header
	int32_t tableOffsets[20];
	for (int i = 0; i < 20; i++) {
		tableOffsets[i] = readLE32(&data[i * 4]);
	}

	const uint8_t* tableData = data.data() + 80; // data starts after header
	size_t tableDataSize = data.size() - 80;

	// Extract sky textures: table pairs (16,17) and (18,19)
	struct SkyDef {
		int palTable;
		int texTable;
		const char* name;
	};
	SkyDef skies[] = {
		{16, 17, "sky_earth"},
		{18, 19, "sky_hell"},
	};

	mkdirRecursive(outDir + "/textures");

	for (auto& sky : skies) {
		// Locate palette table
		int palStart = (sky.palTable > 0) ? tableOffsets[sky.palTable - 1] : 0;
		int palEnd = tableOffsets[sky.palTable];
		// First 4 bytes of each table are the element count
		if (palStart + 4 > (int)tableDataSize) continue;
		int palCount = readLE32(&tableData[palStart]);
		const uint8_t* palBytes = &tableData[palStart + 4];

		// Locate texel table
		int texStart = tableOffsets[sky.texTable - 1]; // texTable > 0 always
		int texEnd = tableOffsets[sky.texTable];
		if (texStart + 4 > (int)tableDataSize) continue;
		int texCount = readLE32(&tableData[texStart]);
		const uint8_t* texBytes = &tableData[texStart + 4];

		// Read palette (RGB565 uint16 array)
		std::vector<uint16_t> palette(palCount);
		for (int i = 0; i < palCount; i++) {
			palette[i] = readLE16(&palBytes[i * 2]);
		}

		// Sky texture is 256x256 (indexed, stride=256)
		int width = 256;
		int height = texCount / width;
		if (height == 0) height = 1;
		if (height * width > texCount) height = texCount / width;

		// Convert to RGBA
		std::vector<uint8_t> rgba(width * height * 4);
		for (int i = 0; i < width * height; i++) {
			uint8_t idx = texBytes[i];
			if (idx < palCount) {
				uint8_t r, g, b, a;
				TextureDecoder::rgb565ToRGBA(palette[idx], r, g, b, a);
				rgba[i * 4 + 0] = r;
				rgba[i * 4 + 1] = g;
				rgba[i * 4 + 2] = b;
				rgba[i * 4 + 3] = a;
			}
		}

		std::string pngPath = outDir + "/textures/" + sky.name + ".png";
		if (!stbi_write_png(pngPath.c_str(), width, height, 4, rgba.data(), width * 4)) {
			fprintf(stderr, "  Failed to write sky PNG: %s\n", pngPath.c_str());
			return false;
		}
		printf("  Extracted %s (%dx%d, %d palette colors)\n", sky.name, width, height, palCount);
	}

	return true;
}

// ========================================================================
// Extract intro camera data from tables.bin (tables 14-15) as YAML
// ========================================================================
static bool extractIntroCamera(const std::string& outDir, const std::vector<uint8_t>& data) {
	if (data.size() < 80) {
		fprintf(stderr, "  tables.bin too small for intro camera\n");
		return false;
	}

	int32_t tableOffsets[20];
	for (int i = 0; i < 20; i++) {
		tableOffsets[i] = readLE32(&data[i * 4]);
	}

	const uint8_t* tableData = data.data() + 80;
	size_t tableDataSize = data.size() - 80;

	// Table 14: camera keys (short array)
	int keysStart = tableOffsets[13]; // table 14 starts after table 13's end
	if (keysStart + 4 > (int)tableDataSize) return false;
	int keysCount = readLE32(&tableData[keysStart]); // element count (shorts)
	const uint8_t* keysBytes = &tableData[keysStart + 4];

	// Table 15: camera tweens (byte array)
	int tweensStart = tableOffsets[14];
	if (tweensStart + 4 > (int)tableDataSize) return false;
	int tweensCount = readLE32(&tableData[tweensStart]); // byte count
	const uint8_t* tweensBytes = &tableData[tweensStart + 4];

	// Parse camera header from keys
	int pos = 0;
	auto readShort = [&]() -> int16_t {
		int16_t v = readLE16s(&keysBytes[pos * 2]);
		pos++;
		return v;
	};

	int totalKeys = readShort();
	int sampleRate = readShort();
	int posShiftRaw = readShort();   // stored as raw value; engine does 4 - posShiftRaw
	int angleShiftRaw = readShort(); // stored as raw value; engine does 10 - angleShiftRaw

	// Build YAML
	std::string yaml;
	yaml += "# Intro camera data (extracted from tables.bin tables 14-15)\n";
	yaml += "total_keys: " + std::to_string(totalKeys) + "\n";
	yaml += "sample_rate: " + std::to_string(sampleRate) + "\n";
	yaml += "pos_shift: " + std::to_string(posShiftRaw) + "\n";
	yaml += "angle_shift: " + std::to_string(angleShiftRaw) + "\n";

	// Camera keyframes: totalKeys * 7 shorts
	yaml += "\nkeyframes:\n";
	for (int k = 0; k < totalKeys; k++) {
		yaml += "  - [";
		for (int c = 0; c < 7; c++) {
			if (c > 0) yaml += ", ";
			yaml += std::to_string(readShort());
		}
		yaml += "]\n";
	}

	// Tween indices: totalKeys * 6 shorts
	yaml += "\ntween_indices:\n";
	for (int k = 0; k < totalKeys; k++) {
		yaml += "  - [";
		for (int c = 0; c < 6; c++) {
			if (c > 0) yaml += ", ";
			yaml += std::to_string(readShort());
		}
		yaml += "]\n";
	}

	// 5 tween counts (which produce 6 cumulative offsets in the engine)
	yaml += "\ntween_counts: [";
	for (int i = 0; i < 5; i++) {
		if (i > 0) yaml += ", ";
		yaml += std::to_string(readShort());
	}
	yaml += "]\n";

	// Tween data (signed bytes)
	yaml += "\ntweens: [";
	for (int i = 0; i < tweensCount; i++) {
		if (i > 0) yaml += ", ";
		yaml += std::to_string((int8_t)tweensBytes[i]);
	}
	yaml += "]\n";

	mkdirRecursive(outDir + "/data");
	std::string yamlPath = outDir + "/data/intro_camera.yaml";
	if (!writeString(yamlPath, yaml)) {
		fprintf(stderr, "  Failed to write intro camera YAML\n");
		return false;
	}
	printf("  Extracted intro camera: %d keys, %d tween bytes\n", totalKeys, tweensCount);
	return true;
}

// ========================================================================
// Extract newMappings.bin as YAML (media_mappings.yaml)
// ========================================================================
static bool extractMediaMappings(const std::string& outDir) {
	std::string binPath = outDir + "/levels/textures/newMappings.bin";
	auto data = readBinFile(binPath);
	if (data.empty()) {
		fprintf(stderr, "  Failed to read newMappings.bin\n");
		return false;
	}

	// Parse the 5 sequential arrays (same layout as TextureDecoder::load)
	int pos = 0;

	// 1. mediaMappings: 512 int16 (tile → media ID)
	int16_t mediaMappings[MAX_MAPPINGS];
	for (int i = 0; i < MAX_MAPPINGS; i++) {
		mediaMappings[i] = readLE16s(&data[pos + i * 2]);
	}
	pos += MAX_MAPPINGS * 2;

	// 2. mediaDimensions: 1024 uint8 (packed width/height)
	const uint8_t* mediaDimensions = &data[pos];
	pos += MAX_MEDIA;

	// 3. mediaBounds: 1024*4 int16 (bounds per media)
	int16_t mediaBounds[MAX_MEDIA * 4];
	for (int i = 0; i < MAX_MEDIA * 4; i++) {
		mediaBounds[i] = readLE16s(&data[pos + i * 2]);
	}
	pos += MAX_MEDIA * 4 * 2;

	// 4. mediaPalColors: 1024 int32 (palette color counts + flags)
	int32_t mediaPalColors[MAX_MEDIA];
	for (int i = 0; i < MAX_MEDIA; i++) {
		mediaPalColors[i] = readLE32(&data[pos + i * 4]);
	}
	pos += MAX_MEDIA * 4;

	// 5. mediaTexelSizes: 1024 int32 (texel sizes + flags)
	int32_t mediaTexelSizes[MAX_MEDIA];
	for (int i = 0; i < MAX_MEDIA; i++) {
		mediaTexelSizes[i] = readLE32(&data[pos + i * 4]);
	}

	// Write as YAML with flow-style arrays for compactness
	std::string yaml;
	yaml += "# Media mappings (extracted from newMappings.bin)\n";
	yaml += "# Used by the engine to map tile IDs to texture media\n\n";

	// mappings: 512 shorts
	yaml += "mappings: [";
	for (int i = 0; i < MAX_MAPPINGS; i++) {
		if (i > 0) yaml += ", ";
		yaml += std::to_string(mediaMappings[i]);
	}
	yaml += "]\n\n";

	// dimensions: 1024 bytes
	yaml += "dimensions: [";
	for (int i = 0; i < MAX_MEDIA; i++) {
		if (i > 0) yaml += ", ";
		yaml += std::to_string(mediaDimensions[i]);
	}
	yaml += "]\n\n";

	// bounds: 1024 entries of 4 shorts each
	yaml += "bounds:\n";
	for (int i = 0; i < MAX_MEDIA; i++) {
		int base = i * 4;
		yaml += std::format("  - [{}, {}, {}, {}]\n",
			mediaBounds[base], mediaBounds[base + 1],
			mediaBounds[base + 2], mediaBounds[base + 3]);
	}
	yaml += "\n";

	// pal_colors: 1024 ints
	yaml += "pal_colors: [";
	for (int i = 0; i < MAX_MEDIA; i++) {
		if (i > 0) yaml += ", ";
		yaml += std::to_string(mediaPalColors[i]);
	}
	yaml += "]\n\n";

	// texel_sizes: 1024 ints
	yaml += "texel_sizes: [";
	for (int i = 0; i < MAX_MEDIA; i++) {
		if (i > 0) yaml += ", ";
		yaml += std::to_string(mediaTexelSizes[i]);
	}
	yaml += "]\n";

	std::string yamlPath = outDir + "/levels/textures/media_mappings.yaml";
	if (!writeString(yamlPath, yaml)) {
		fprintf(stderr, "  Failed to write media_mappings.yaml\n");
		return false;
	}
	printf("  Extracted media mappings (%d mappings, %d media entries)\n", MAX_MAPPINGS, MAX_MEDIA);
	return true;
}

// ========================================================================
// Extract palette + texel data from newPalettes.bin / newTexels*.bin
// Writes: media_palettes.yaml  (palette data per media ID)
//         media_texels/<id>.dat (raw texel bytes per unique media ID)
//         media_texels.yaml     (index: media ID → file, size, refs)
// ========================================================================
static bool extractMediaTextures(const std::string& outDir) {
	TextureDecoder decoder;
	std::string texDir = outDir + "/levels/textures";
	if (!decoder.load(texDir, g_game->numTextures)) {
		fprintf(stderr, "  Failed to load texture data for media extraction\n");
		return false;
	}

	const int32_t* palColors = decoder.getPalColors();
	const int32_t* texelSizes = decoder.getTexelSizes();
	const auto& palettes = decoder.getPalettes();
	const auto& texels = decoder.getTexels();

	// --- media_palettes.yaml ---
	// Stores palette RGB565 data per media ID.
	// Non-reference entries have "colors: [...]", reference entries have "ref: <target_id>"
	std::string palYaml;
	palYaml += "# Media palette data (extracted from newPalettes.bin)\n";
	palYaml += "# Each entry is a media ID with either palette colors or a reference\n\n";

	int palOwners = 0, palRefs = 0;
	for (int i = 0; i < MAX_MEDIA; i++) {
		bool isRef = (palColors[i] & FLAG_REFERENCE) != 0;
		int numColors = palColors[i] & 0x3FFFFFFF;
		if (numColors == 0 && !isRef) continue;

		if (isRef) {
			int refTarget = palColors[i] & 0x3FF;
			palYaml += std::format("{}: {{ref: {}}}\n", i, refTarget);
			palRefs++;
		} else {
			auto pit = palettes.find(i);
			if (pit == palettes.end()) continue;
			const auto& pal = pit->second;
			palYaml += std::to_string(i) + ": {colors: [";
			for (int c = 0; c < (int)pal.size(); c++) {
				if (c > 0) palYaml += ", ";
				palYaml += std::to_string(pal[c]);
			}
			palYaml += "]}\n";
			palOwners++;
		}
	}

	if (!writeString(texDir + "/media_palettes.yaml", palYaml)) {
		fprintf(stderr, "  Failed to write media_palettes.yaml\n");
		return false;
	}

	// --- media_texels/ directory with raw .dat files ---
	std::string texelDir = texDir + "/media_texels";
	mkdirRecursive(texelDir);

	// --- media_texels.yaml ---
	std::string texYaml;
	texYaml += "# Media texel index (extracted from newTexels*.bin)\n";
	texYaml += "# Each entry maps a media ID to its texel data file or a reference\n\n";

	int texOwners = 0, texRefs = 0;
	for (int i = 0; i < MAX_MEDIA; i++) {
		bool isRef = (texelSizes[i] & FLAG_REFERENCE) != 0;
		int size = (texelSizes[i] & 0x3FFFFFFF) + 1;
		if (size <= 1 && !isRef) continue;

		if (isRef) {
			int refTarget = texelSizes[i] & 0x3FF;
			texYaml += std::format("{}: {{ref: {}}}\n", i, refTarget);
			texRefs++;
		} else {
			auto tit = texels.find(i);
			if (tit == texels.end()) continue;
			const auto& data = tit->second;

			std::string fileName = std::format("{:04d}.dat", i);
			std::string filePath = texelDir + "/" + fileName;
			if (!writeFile(filePath, {data.data(), data.size()})) {
				fprintf(stderr, "  Failed to write texel file: %s\n", filePath.c_str());
				continue;
			}

			texYaml += std::format("{}: {{file: \"{}\", size: {}}}\n", i, fileName, (int)data.size());
			texOwners++;
		}
	}

	if (!writeString(texDir + "/media_texels.yaml", texYaml)) {
		fprintf(stderr, "  Failed to write media_texels.yaml\n");
		return false;
	}

	printf("  Extracted %d palettes (%d refs), %d texel files (%d refs)\n",
	       palOwners, palRefs, texOwners, texRefs);
	return true;
}

// ========================================================================
// Copy binary assets (maps, textures, models)
// ========================================================================
static void copyBinaryAssets(ZipFile& zip, const std::string& outDir) {
	std::vector<std::string> binaryFiles;
	for (int i = 0; i < g_game->numMaps; i++) {
		binaryFiles.push_back(std::format("Packages/map{:02d}.bin", i));
		binaryFiles.push_back(std::format("Packages/model_{:04d}.bin", i));
	}
	binaryFiles.push_back("Packages/newMappings.bin");
	binaryFiles.push_back("Packages/newPalettes.bin");
	for (int i = 0; i < g_game->numTextures; i++) {
		binaryFiles.push_back(std::format("Packages/newTexels{:03d}.bin", i));
	}
	int copied = 0;
	for (const auto& relPath : binaryFiles) {
		std::string ipaPath = std::string(g_game->ipaPrefix) + relPath;
		int size = 0;
		if (!zip.hasEntry(ipaPath.c_str()))
			continue;
		uint8_t* data = zip.readZipFileEntry(ipaPath.c_str(), &size);
		if (data) {
			std::string outFile = relPath;
			if (outFile.starts_with("Packages/")) {
				outFile = outFile.substr(9);
			}

			if (outFile.starts_with("map") && outFile.ends_with(".bin")) {
				int mapIdx = 0;
				if (sscanf(outFile.c_str(), "map%d.bin", &mapIdx) == 1) {
					int mapId = mapIdx + 1;
					outFile = "levels/" + getLevelDirName(mapId) + "/map.bin";
				}
			} else if (outFile.starts_with("model_")) {
				int modelIdx = 0;
				if (sscanf(outFile.c_str(), "model_%d.bin", &modelIdx) == 1) {
					int mapId = modelIdx + 1;
					outFile = "levels/" + getLevelDirName(mapId) + "/model.bin";
				}
			} else if (outFile.starts_with("new")) {
				outFile = "levels/textures/" + outFile;
			}

			std::string outPath = outDir + "/" + outFile;
			size_t slash = outPath.rfind('/');
			if (slash != std::string::npos) {
				mkdirRecursive(outPath.substr(0, slash));
			}

			writeFile(outPath, {data, static_cast<size_t>(size)});
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
// Extract textures/sprites as individual PNG files
// ========================================================================
// Per-tile extraction result: PNG path, frame dimensions, and frame count
struct TileExportInfo {
	std::string pngPath;
	int frameWidth = 0;
	int frameHeight = 0;
	int frameCount = 1;
};

static bool extractTexturePngs(
	const std::string& outDir,
	std::map<int, TileExportInfo>& tileExports) {

	TextureDecoder decoder;
	std::string texDir = outDir + "/levels/textures";
	if (!decoder.load(texDir, g_game->numTextures)) {
		fprintf(stderr, "  Failed to load texture data\n");
		return false;
	}
	if (!decoder.decode()) {
		fprintf(stderr, "  Failed to decode textures\n");
		return false;
	}

	// Build tile ID -> sprite name map from sprites.yaml
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
	}

	const int16_t* mappings = decoder.getMappings();

	// Create output directories
	mkdirRecursive(outDir + "/sprites");
	mkdirRecursive(outDir + "/textures");

	int spriteCount = 0, textureCount = 0;

	for (auto& [tileId, name] : tileIdToName) {
		if (tileId < 0 || tileId >= MAX_MAPPINGS - 1) continue;

		int startMid = mappings[tileId];
		int endMid = mappings[tileId + 1];
		if (startMid < 0 || endMid <= startMid) continue;

		bool isTexture = name.starts_with("texture_");
		std::string subDir = isTexture ? "textures" : "sprites";

		// Collect valid decoded frames
		int numFrames = endMid - startMid;
		std::vector<const DecodedMedia*> frames;
		for (int f = 0; f < numFrames; f++) {
			const DecodedMedia* dm = decoder.getMedia(startMid + f);
			if (dm) frames.push_back(dm);
		}
		if (frames.empty()) continue;

		int frameW = frames[0]->width;
		int frameH = frames[0]->height;
		std::string fileName = std::format("{}/{}.png", subDir, name);
		std::string fullPath = outDir + "/" + fileName;

		if (frames.size() == 1) {
			// Single frame: write directly
			if (!stbi_write_png(fullPath.c_str(), frameW, frameH, 4,
			                    frames[0]->rgba.data(), frameW * 4)) {
				fprintf(stderr, "  Failed to write PNG: %s\n", fullPath.c_str());
				continue;
			}
		} else {
			// Multiple frames: stitch horizontally into a single strip
			int totalW = frameW * (int)frames.size();
			std::vector<uint8_t> strip(totalW * frameH * 4, 0);
			for (int f = 0; f < (int)frames.size(); f++) {
				const auto& src = frames[f]->rgba;
				int fw = frames[f]->width;
				int fh = frames[f]->height;
				for (int y = 0; y < fh && y < frameH; y++) {
					memcpy(&strip[(y * totalW + f * frameW) * 4],
					       &src[y * fw * 4],
					       std::min(fw, frameW) * 4);
				}
			}
			if (!stbi_write_png(fullPath.c_str(), totalW, frameH, 4,
			                    strip.data(), totalW * 4)) {
				fprintf(stderr, "  Failed to write PNG: %s\n", fullPath.c_str());
				continue;
			}
		}

		TileExportInfo info;
		info.pngPath = fileName;
		info.frameWidth = frameW;
		info.frameHeight = frameH;
		info.frameCount = (int)frames.size();
		tileExports[tileId] = std::move(info);

		if (isTexture) textureCount++; else spriteCount++;
	}

	printf("  Extracted %d sprite PNGs, %d texture PNGs\n", spriteCount, textureCount);
	return true;
}

// ========================================================================
// Rewrite sprites.yaml with PNG file references and size info
// ========================================================================
static bool rewriteSpritesYaml(
	const std::string& outDir,
	const std::map<int, TileExportInfo>& tileExports) {

	std::string spritesPath = outDir + "/sprites.yaml";
	YAML::Node root = YAML::LoadFile(spritesPath);
	YAML::Node sprites = root["sprites"];
	if (!sprites) {
		fprintf(stderr, "  rewriteSpritesYaml: no sprites section found\n");
		return false;
	}

	int updated = 0;
	for (auto it = sprites.begin(); it != sprites.end(); ++it) {
		auto& val = it->second;
		if (!val.IsMap()) continue;
		if (!val["file"] || !val["id"]) continue;
		if (val["file"].as<std::string>() != "tables.bin") continue;

		int id = val["id"].as<int>();
		auto expIt = tileExports.find(id);
		if (expIt == tileExports.end()) continue;

		const auto& info = expIt->second;
		val["file"] = info.pngPath;

		// Size: [width, height] of a single frame
		YAML::Node sizeNode(YAML::NodeType::Sequence);
		sizeNode.push_back(info.frameWidth);
		sizeNode.push_back(info.frameHeight);
		sizeNode.SetStyle(YAML::EmitterStyle::Flow);
		val["size"] = sizeNode;

		// Multi-frame sprites: add frame_size and frames count
		if (info.frameCount > 1) {
			YAML::Node frameSizeNode(YAML::NodeType::Sequence);
			frameSizeNode.push_back(info.frameWidth);
			frameSizeNode.push_back(info.frameHeight);
			frameSizeNode.SetStyle(YAML::EmitterStyle::Flow);
			val["frame_size"] = frameSizeNode;
			val["frames"] = info.frameCount;
		}

		updated++;
	}

	// Write back with clean formatting
	YAML::Emitter emitter;
	emitter.SetIndent(2);
	emitter.SetMapFormat(YAML::Block);
	emitter.SetSeqFormat(YAML::Block);
	emitter << root;

	if (!writeString(spritesPath, emitter.c_str())) {
		fprintf(stderr, "  Failed to write updated sprites.yaml\n");
		return false;
	}

	printf("  Updated %d sprite entries with PNG paths and sizes\n", updated);
	return true;
}

// ========================================================================
// Public entry point: convert a Doom II RPG IPA
// ========================================================================
bool convertD2RPG(ZipFile& zip, const GameDef& game, const std::string& outputDir) {
	g_game = &game;

	bool ok = true;

	printf("\n[Phase 1] Writing YAML assets...\n");

	printf("Writing resource files...\n");
	ok &= writeResourceFiles(outputDir);

	printf("Converting strings...\n");
	ok &= convertStrings(zip, outputDir);

	printf("\nCopying binary assets (maps, textures, models)...\n");
	copyBinaryAssets(zip, outputDir);

	printf("\nExtracting textures as PNGs...\n");
	std::map<int, TileExportInfo> tileExports;
	ok &= extractTexturePngs(outputDir, tileExports);

	printf("\nExtracting sprites to YAML...\n");
	ok &= extractSpritesToYaml(outputDir);

	printf("\nUpdating sprites.yaml with PNG references...\n");
	ok &= rewriteSpritesYaml(outputDir, tileExports);

	// Read tables.bin from IPA into memory (not copied to output)
	std::vector<uint8_t> tablesData;
	{
		std::string tablesIpaPath = std::string(g_game->pkgPrefix) + "tables.bin";
		int tablesSize = 0;
		uint8_t* raw = zip.readZipFileEntry(tablesIpaPath.c_str(), &tablesSize);
		if (raw && tablesSize > 0) {
			tablesData.assign(raw, raw + tablesSize);
			free(raw);
		}
	}

	printf("\nExtracting sky textures from tables.bin...\n");
	ok &= extractSkyTextures(outputDir, tablesData);

	printf("\nExtracting intro camera from tables.bin...\n");
	ok &= extractIntroCamera(outputDir, tablesData);

	printf("\nExtracting media mappings from newMappings.bin...\n");
	ok &= extractMediaMappings(outputDir);

	printf("\nExtracting media palettes and texels...\n");
	ok &= extractMediaTextures(outputDir);

	printf("\nCopying audio assets...\n");
	copyAudioAssets(zip, outputDir);

	printf("\nCopying image assets...\n");
	copyImageAssets(zip, outputDir);

	return ok;
}
