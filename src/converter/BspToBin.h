#pragma once
// BspToBin — Convert D1 RPG .bsp level data to D2 RPG .bin format.
// Thin API wrapping the logic from tools/bsp2bin.cpp.

#include <cstdint>
#include <string>
#include <vector>

namespace BspToBin {

struct DoorInfo {
    int midX, midY;   // world coords of door midpoint
    bool horizontal;  // true = N/S door, false = E/W door
};

struct Result {
    std::vector<uint8_t> binData;
    int numPolys = 0, numLines = 0, numNodes = 0;
    // Extracted D1 metadata
    std::string mapName;
    int spawnIndex = 0;
    int spawnDir = 0;       // D2 direction (0-7)
    std::vector<DoorInfo> doors;
};

// Convert raw D1 .bsp bytes → D2 .bin level data.
Result convert(const uint8_t* bspData, int bspLen);

} // namespace BspToBin
