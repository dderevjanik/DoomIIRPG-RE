#pragma once

#include "MapData.h"
#include <string>

namespace MapDataIO {

// Load a MapData from a .bin map file
// Returns true on success, false on error (with errorMsg set)
bool loadFromBin(const std::string& path, MapData& out, std::string& errorMsg);

// Save a MapData to a .bin map file (uses BSP compiler internally)
// Returns true on success, false on error (with errorMsg set)
bool saveToBin(const MapData& map, const std::string& path, std::string& errorMsg);

} // namespace MapDataIO
