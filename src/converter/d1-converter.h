#pragma once

#include "converter.h"

// D1RPG game definition
extern const GameDef GAME_DOOM1RPG;

// Convert D1 RPG .bsp levels to D2 .bin format (geometry only)
class ZipFile;
bool convertD1Levels(ZipFile& zip, const GameDef& game, const std::string& outputDir);
