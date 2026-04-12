#pragma once

#include "MapData.h"
#include <cstdint>
#include <string>
#include <vector>

struct BSPNode {
    int16_t offset = 0;       // 0xFFFF for leaf, else plane offset
    uint8_t normalIdx = 0;    // index into normals array
    int16_t child1 = 0;       // interior: child index; leaf: byte offset into polyData
    int16_t child2 = 0;       // interior: child index; leaf: packed line refs
    uint8_t bounds[4] = {};   // minX, minY, maxX, maxY (each = worldCoord >> 7)
};

struct LineSegment {
    uint8_t x1, x2;           // worldCoord >> 7
    uint8_t y1, y2;
    uint8_t flags = 0;        // 4-bit flag per line
};

struct BSPCompileResult {
    std::vector<int16_t> normals;    // numNormals * 3
    std::vector<BSPNode> nodes;
    std::vector<uint8_t> polyData;   // packed polygon meshes
    std::vector<LineSegment> lines;
};

namespace BSPCompiler {

// Compile a MapData's tile grid into BSP tree, polygon data, and line segments.
// Returns true on success.
bool compile(const MapData& map, BSPCompileResult& result, std::string& errorMsg);

} // namespace BSPCompiler
