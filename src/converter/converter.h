#pragma once

#include <string>
#include <cstdint>

// ========================================================================
// Game definition — all game-specific configuration in one place.
// ========================================================================
struct GameDef {
	const char* id;              // e.g. "doom2rpg"
	const char* name;            // e.g. "D2RPG"
	const char* ipaPrefix;       // IPA path prefix (e.g. "Payload/Doom2rpg.app/")
	const char* pkgPrefix;       // package prefix (e.g. "Payload/Doom2rpg.app/Packages/")
	const char* defaultOutput;   // default output dir (e.g. "games/doom2rpg")
	const char* soundsDir;       // IPA subdirectory for audio (e.g. "sounds2")
	const char* const* levelDirs;
	int numLevels;
	int numMaps;                 // number of map/model binary files
	int numTextures;             // number of newTexels files
	int maxStringGroups;
	int maxLanguages;
	const char* const* languageNames;
	const char* const* groupNames;
};

class ZipFile;

// ========================================================================
// Shared utility functions
// ========================================================================
bool dirExists(const char* path);
bool mkdirRecursive(const std::string& path);
bool writeFile(const std::string& path, const uint8_t* data, int size);
bool writeString(const std::string& path, const std::string& content);

int16_t  readShort(const uint8_t* data, int offset);
uint16_t readUShort(const uint8_t* data, int offset);
int32_t  readInt(const uint8_t* data, int offset);
uint32_t readUInt(const uint8_t* data, int offset);

std::string escapeString(const uint8_t* raw, int len);

// ========================================================================
// Game conversion entry point
// ========================================================================
bool convertD2RPG(ZipFile& zip, const GameDef& game, const std::string& outputDir);
