#pragma once

#include "converter.h"

// D1RPG game definition
extern const GameDef GAME_DOOM1RPG;

class ZipFile;

// Convert D1 RPG .bsp levels to D2 .bin format (geometry only)
bool convertD1Levels(ZipFile& zip, const GameDef& game, const std::string& outputDir);

// Extract D1 RPG wall textures to PNG files
bool extractD1Textures(ZipFile& zip, const std::string& outputDir);
