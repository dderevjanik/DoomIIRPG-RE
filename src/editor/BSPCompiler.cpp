#include "BSPCompiler.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <map>

// Map constants matching engine
static constexpr int MAP_SIZE = 32;
static constexpr int TILE_SIZE = 64;
static constexpr int WALL_HEIGHT = 64;  // Default wall height in world units

// Coordinate conversion: world coords to byte values used in the binary format
// Engine reads vertex/bounds/line bytes with << 7, and viewX = (worldX << 4) + 8.
// So byte = worldCoord / 8 = worldCoord >> 3 (NOT >> 3 as previously documented).
// Verified against working test_map binary: tile col=1 boundary (world=64) = byte 8.
// Vertex bytes: worldCoord >> 3
// Line bytes: worldCoord >> 3
// Node bounds: worldCoord >> 3 (clamped to 255)
// Height: tile height byte << 3 = world Z

// A polygon with vertices and texture info, before BSP compilation
struct RawPolygon {
    struct Vertex {
        uint8_t x, y, z;  // each = worldCoord >> 3
        int8_t s, t;      // texture UV, each = texCoord >> 6
    };

    std::vector<Vertex> verts;
    int tileNum = 1;       // texture tile number
    bool isWall = false;
    bool isFloor = false;

    // Bounding box in node-bounds coordinates (worldCoord >> 3)
    uint8_t minX() const {
        uint8_t m = 255;
        for (auto& v : verts) m = std::min(m, v.x);
        return m;
    }
    uint8_t maxX() const {
        uint8_t m = 0;
        for (auto& v : verts) m = std::max(m, v.x);
        return m;
    }
    uint8_t minY() const {
        uint8_t m = 255;
        for (auto& v : verts) m = std::min(m, v.y);
        return m;
    }
    uint8_t maxY() const {
        uint8_t m = 0;
        for (auto& v : verts) m = std::max(m, v.y);
        return m;
    }
};

// A raw line segment before compilation
struct RawLine {
    uint8_t x1, x2, y1, y2; // worldCoord >> 3
    uint8_t flags = 0;       // 4-bit flag
};

// ---- Wall geometry generation ----

// Helper: generate a collision line on a tile edge
static RawLine makeEdgeLine(int col, int row, int dc, int dr, uint8_t flags = 0x0) {
    RawLine line;
    line.flags = flags;
    if (dc == 1) {
        uint8_t wx = (uint8_t)((col + 1) * 8);
        line.x1 = wx; line.x2 = wx;
        line.y1 = (uint8_t)(row * 8);
        line.y2 = (uint8_t)((row + 1) * 8);
    } else if (dc == -1) {
        uint8_t wx = (uint8_t)(col * 8);
        line.x1 = wx; line.x2 = wx;
        line.y1 = (uint8_t)(row * 8);
        line.y2 = (uint8_t)((row + 1) * 8);
    } else if (dr == 1) {
        uint8_t wy = (uint8_t)((row + 1) * 8);
        line.x1 = (uint8_t)(col * 8);
        line.x2 = (uint8_t)((col + 1) * 8);
        line.y1 = wy; line.y2 = wy;
    } else {
        uint8_t wy = (uint8_t)(row * 8);
        line.x1 = (uint8_t)(col * 8);
        line.x2 = (uint8_t)((col + 1) * 8);
        line.y1 = wy; line.y2 = wy;
    }
    return line;
}

// Helper: generate a wall quad on a tile edge between two Z levels
// dc,dr define the edge direction. zLo/zHi are world coords.
static RawPolygon makeWallQuad(int col, int row, int dc, int dr,
                                int zLo, int zHi, int tileNum) {
    RawPolygon poly;
    poly.tileNum = tileNum;
    poly.isWall = true;

    // Edge world coordinates.
    // Nudge wall 1 unit toward the open tile (dc,dr direction) so it
    // partitions with the adjacent floor, not the solid tile. This ensures
    // walls and their floors are in the same BSP leaf for correct draw order.
    int nudge = 1; // 1 world unit toward open tile
    int wx0, wy0, wx1, wy1;
    if (dc == 1) {       // East — wall at right edge, nudge right
        wx0 = (col + 1) * 64 + nudge; wy0 = row * 64;
        wx1 = (col + 1) * 64 + nudge; wy1 = (row + 1) * 64;
    } else if (dc == -1) { // West — wall at left edge, nudge left
        wx0 = col * 64 - nudge; wy0 = (row + 1) * 64;
        wx1 = col * 64 - nudge; wy1 = row * 64;
    } else if (dr == 1) { // South — wall at bottom edge, nudge down
        wx0 = (col + 1) * 64; wy0 = (row + 1) * 64 + nudge;
        wx1 = col * 64;       wy1 = (row + 1) * 64 + nudge;
    } else {              // North — wall at top edge, nudge up
        wx0 = col * 64;       wy0 = row * 64 - nudge;
        wx1 = (col + 1) * 64; wy1 = row * 64 - nudge;
    }

    uint8_t bx0 = (uint8_t)(wx0 >> 3), by0 = (uint8_t)(wy0 >> 3);
    uint8_t bx1 = (uint8_t)(wx1 >> 3), by1 = (uint8_t)(wy1 >> 3);
    uint8_t bzl = (uint8_t)(zLo >> 3),  bzh = (uint8_t)(zHi >> 3);

    // Winding: top-start → top-end → bottom-end → bottom-start (CW from front)
    // Matches test_map convention
    RawPolygon::Vertex v0 = {bx0, by0, bzh, 0, 1};
    RawPolygon::Vertex v1 = {bx1, by1, bzh, 1, 1};
    RawPolygon::Vertex v2 = {bx1, by1, bzl, 1, 0};
    RawPolygon::Vertex v3 = {bx0, by0, bzl, 0, 0};
    poly.verts = {v0, v1, v2, v3};
    return poly;
}

// Generate wall quads, floor quads, and height-step walls from the tile grid
static void generateGeometry(const MapData& map,
                             std::vector<RawPolygon>& polys,
                             std::vector<RawLine>& lines) {
    auto isSolid = [&](int col, int row) -> bool {
        if (col < 0 || col >= MAP_SIZE || row < 0 || row >= MAP_SIZE) return true;
        return (map.tiles[row * MAP_SIZE + col].flags & 0x1) != 0;
    };

    auto tileHeight = [&](int col, int row) -> uint8_t {
        if (col < 0 || col >= MAP_SIZE || row < 0 || row >= MAP_SIZE) return 0;
        return map.tiles[row * MAP_SIZE + col].height;
    };

    struct Dir { int dc, dr; };
    Dir dirs[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    for (int row = 0; row < MAP_SIZE; row++) {
        for (int col = 0; col < MAP_SIZE; col++) {
            const TileData& tile = map.tiles[row * MAP_SIZE + col];
            bool solid = (tile.flags & 0x1) != 0;

            if (solid) {
                // Generate wall quads on edges adjacent to non-solid tiles
                for (auto& d : dirs) {
                    int nc = col + d.dc;
                    int nr = row + d.dr;
                    if (!isSolid(nc, nr)) {
                        uint8_t neighborH = tileHeight(nc, nr);
                        int zLo = neighborH * 8;
                        int zHi = zLo + WALL_HEIGHT;

                        lines.push_back(makeEdgeLine(col, row, d.dc, d.dr));
                        polys.push_back(makeWallQuad(col, row, d.dc, d.dr,
                                                     zLo, zHi, tile.wallTexture));
                    }
                }

                // Generate corner fill quads at inside corners.
                // An inside corner is where this solid tile and a diagonal neighbor
                // are both solid, but both adjacent tiles (sharing an edge) are open.
                // This creates a thin wall quad sealing the diagonal gap.
                struct Corner { int dc1, dr1, dc2, dr2; }; // two adjacent directions
                Corner corners[] = {{1,0,0,1}, {1,0,0,-1}, {-1,0,0,1}, {-1,0,0,-1}};
                for (auto& cn : corners) {
                    // Check: solid diagonal, both edges open
                    if (isSolid(col+cn.dc1+cn.dc2, row+cn.dr1+cn.dr2) &&
                        !isSolid(col+cn.dc1, row+cn.dr1) &&
                        !isSolid(col+cn.dc2, row+cn.dr2)) {
                        // Corner point in world coords
                        int cx = (cn.dc1 > 0 ? (col+1)*64 : col*64);
                        int cy = (cn.dr2 > 0 ? (row+1)*64 : row*64);
                        uint8_t bh = tileHeight(col+cn.dc1, row+cn.dr1);
                        int zLo = bh * 8;
                        int zHi = zLo + WALL_HEIGHT;
                        uint8_t bcx = (uint8_t)(cx >> 3);
                        uint8_t bcy = (uint8_t)(cy >> 3);
                        uint8_t bzl = (uint8_t)(zLo >> 3);
                        uint8_t bzh = (uint8_t)(zHi >> 3);

                        // Thin quad: 1 unit in each direction from the corner
                        uint8_t bx2 = (uint8_t)(bcx + (cn.dc1 > 0 ? 0 : 1));
                        uint8_t by2 = (uint8_t)(bcy + (cn.dr2 > 0 ? 0 : 1));
                        RawPolygon fill;
                        fill.tileNum = tile.wallTexture;
                        fill.isWall = true;
                        fill.verts = {
                            {bcx, bcy, bzl, 0, 0},
                            {bx2, by2, bzl, 0, 0},
                            {bx2, by2, bzh, 0, 1},
                            {bcx, bcy, bzh, 0, 1}
                        };
                        polys.push_back(fill);
                    }
                }
            } else {
                // Floor quad
                int wx0 = col * 64, wy0 = row * 64;
                int wx1 = (col + 1) * 64, wy1 = (row + 1) * 64;
                int wz = tile.height * 8;

                RawPolygon poly;
                poly.tileNum = tile.floorTexture;
                poly.isFloor = true;

                // Floor: CW winding viewed from above (matching test_map convention)
                RawPolygon::Vertex fv0 = {(uint8_t)(wx0 >> 3), (uint8_t)(wy1 >> 3), (uint8_t)(wz >> 3), 0, 1};
                RawPolygon::Vertex fv1 = {(uint8_t)(wx1 >> 3), (uint8_t)(wy1 >> 3), (uint8_t)(wz >> 3), 1, 1};
                RawPolygon::Vertex fv2 = {(uint8_t)(wx1 >> 3), (uint8_t)(wy0 >> 3), (uint8_t)(wz >> 3), 1, 0};
                RawPolygon::Vertex fv3 = {(uint8_t)(wx0 >> 3), (uint8_t)(wy0 >> 3), (uint8_t)(wz >> 3), 0, 0};
                poly.verts = {fv0, fv1, fv2, fv3};
                polys.push_back(poly);

                // Ceiling quad (CCW from above = CW from below)
                int wzCeil = wz + WALL_HEIGHT;
                RawPolygon cpoly;
                cpoly.tileNum = tile.floorTexture; // same texture for now
                cpoly.isFloor = true;
                RawPolygon::Vertex cv0 = {(uint8_t)(wx0 >> 3), (uint8_t)(wy0 >> 3), (uint8_t)(wzCeil >> 3), 0, 0};
                RawPolygon::Vertex cv1 = {(uint8_t)(wx1 >> 3), (uint8_t)(wy0 >> 3), (uint8_t)(wzCeil >> 3), 1, 0};
                RawPolygon::Vertex cv2 = {(uint8_t)(wx1 >> 3), (uint8_t)(wy1 >> 3), (uint8_t)(wzCeil >> 3), 1, 1};
                RawPolygon::Vertex cv3 = {(uint8_t)(wx0 >> 3), (uint8_t)(wy1 >> 3), (uint8_t)(wzCeil >> 3), 0, 1};
                cpoly.verts = {cv0, cv1, cv2, cv3};
                polys.push_back(cpoly);

                // Height-step walls: vertical face between adjacent non-solid tiles
                // at different heights
                for (auto& d : dirs) {
                    int nc = col + d.dc;
                    int nr = row + d.dr;
                    if (nc < 0 || nc >= MAP_SIZE || nr < 0 || nr >= MAP_SIZE) continue;
                    if (isSolid(nc, nr)) continue;

                    uint8_t neighborH = tileHeight(nc, nr);
                    if (neighborH == tile.height) continue;

                    // Only generate the step wall from the higher tile to avoid duplicates
                    if (tile.height < neighborH) continue;

                    int zLo = neighborH * 8;
                    int zHi = tile.height * 8;

                    polys.push_back(makeWallQuad(col, row, d.dc, d.dr,
                                                 zLo, zHi, tile.wallTexture));
                }
            }
        }
    }
}

// ---- BSP tree construction ----

struct BSPBuildNode {
    bool isLeaf = false;
    int nodeIndex = -1;  // index in output nodes array

    // For interior nodes
    int axis = 0;       // 0 = X, 1 = Y
    int splitCoord = 0; // world coordinate of splitting plane
    int leftChild = -1;
    int rightChild = -1;

    // For leaf nodes
    std::vector<int> polyIndices;
    std::vector<int> lineIndices;

    // Bounds
    uint8_t minX = 255, minY = 255, maxX = 0, maxY = 0;
};

static int buildBSPRecursive(
    std::vector<BSPBuildNode>& buildNodes,
    const std::vector<RawPolygon>& polys,
    const std::vector<RawLine>& lines,
    std::vector<int> polyIdxs,
    std::vector<int> lineIdxs,
    int minX, int minY, int maxX, int maxY,
    int depth)
{
    int idx = (int)buildNodes.size();
    buildNodes.emplace_back();
    BSPBuildNode& node = buildNodes[idx];

    // Compute bounds in node-byte coords (world >> 3)
    node.minX = (uint8_t)std::min(255, minX >> 3);
    node.minY = (uint8_t)std::min(255, minY >> 3);
    node.maxX = (uint8_t)std::min(255, maxX >> 3);
    node.maxY = (uint8_t)std::min(255, maxY >> 3);

    // Leaf condition: few polys or small region or max depth
    if (polyIdxs.size() <= 4 || depth >= 10 ||
        (maxX - minX <= 128 && maxY - minY <= 128)) {
        node.isLeaf = true;
        node.polyIndices = std::move(polyIdxs);
        node.lineIndices = std::move(lineIdxs);
        return idx;
    }

    // Choose split axis and coordinate
    node.axis = depth % 2;
    int splitWorld;
    if (node.axis == 0) {
        splitWorld = (minX + maxX) / 2;
        // Align to tile boundary (64 world units)
        splitWorld = (splitWorld / 64) * 64;
    } else {
        splitWorld = (minY + maxY) / 2;
        splitWorld = (splitWorld / 64) * 64;
    }
    node.splitCoord = splitWorld;

    // Partition polygons
    std::vector<int> leftPolys, rightPolys;
    for (int pi : polyIdxs) {
        auto& p = polys[pi];
        // Compute polygon center on split axis
        int center;
        if (node.axis == 0) {
            center = ((int)p.minX() + (int)p.maxX()) * 8 / 2; // byte * 8 = world center
        } else {
            center = ((int)p.minY() + (int)p.maxY()) * 8 / 2;
        }
        if (center < splitWorld) {
            leftPolys.push_back(pi);
        } else {
            rightPolys.push_back(pi);
        }
    }

    // Partition lines
    std::vector<int> leftLines, rightLines;
    for (int li : lineIdxs) {
        auto& l = lines[li];
        int center;
        if (node.axis == 0) {
            // Lines use 8-per-tile coords, convert to world: lineCoord * (64/8) = lineCoord * 8
            center = ((int)l.x1 + (int)l.x2) * 8 / 2;
        } else {
            center = ((int)l.y1 + (int)l.y2) * 8 / 2;
        }
        if (center < splitWorld) {
            leftLines.push_back(li);
        } else {
            rightLines.push_back(li);
        }
    }

    // If partition failed (all on one side), make leaf
    if (depth < 4) {
    }
    if (leftPolys.empty() || rightPolys.empty()) {
        node.isLeaf = true;
        node.polyIndices = std::move(polyIdxs);
        node.lineIndices = std::move(lineIdxs);
        return idx;
    }

    node.isLeaf = false;

    // Recurse
    int leftMinX = minX, leftMaxX, leftMinY = minY, leftMaxY;
    int rightMinX, rightMaxX = maxX, rightMinY, rightMaxY = maxY;
    if (node.axis == 0) {
        leftMaxX = splitWorld;
        leftMaxY = maxY;
        rightMinX = splitWorld;
        rightMinY = minY;
        rightMaxY = maxY;
        leftMinY = minY;
    } else {
        leftMaxX = maxX;
        leftMaxY = splitWorld;
        rightMinX = minX;
        rightMinY = splitWorld;
        rightMaxX = maxX;
        leftMinX = minX;
    }

    // Build children (must be done after emplace since buildNodes may realloc)
    int leftIdx = buildBSPRecursive(buildNodes, polys, lines, std::move(leftPolys), std::move(leftLines),
                                    leftMinX, leftMinY, leftMaxX, leftMaxY, depth + 1);
    int rightIdx = buildBSPRecursive(buildNodes, polys, lines, std::move(rightPolys), std::move(rightLines),
                                     rightMinX, rightMinY, rightMaxX, rightMaxY, depth + 1);

    buildNodes[idx].leftChild = leftIdx;
    buildNodes[idx].rightChild = rightIdx;

    return idx;
}

// ---- Polygon packing ----

static void packPolygons(
    const std::vector<RawPolygon>& polys,
    const std::vector<int>& polyIndices,
    std::vector<uint8_t>& polyData,
    int& polyDataOffset)
{
    // Group polygons by texture tile number
    std::map<int, std::vector<int>> byTexture;
    for (int pi : polyIndices) {
        byTexture[polys[pi].tileNum].push_back(pi);
    }

    // Count total meshes needed (max 127 polys per mesh, split large groups)
    int meshCount = 0;
    for (auto& [tileNum, indices] : byTexture) {
        meshCount += ((int)indices.size() + 126) / 127;
    }
    polyDataOffset = (int)polyData.size();

    // Write mesh count
    polyData.push_back((uint8_t)meshCount);

    for (auto& [tileNum, indices] : byTexture) {
      for (int chunk = 0; chunk < (int)indices.size(); chunk += 127) {
        int polyCount = std::min(127, (int)indices.size() - chunk);

        // 4 reserved bytes
        polyData.push_back(0);
        polyData.push_back(0);
        polyData.push_back(0);
        polyData.push_back(0);

        // Packed mesh info: (tileNum << 7) | polyCount
        uint16_t packed = (uint16_t)((tileNum << 7) | (polyCount & 0x7F));
        polyData.push_back(packed & 0xFF);
        polyData.push_back((packed >> 8) & 0xFF);

        for (int ci = chunk; ci < chunk + polyCount; ci++) {
            int pi = indices[ci];
            auto& poly = polys[pi];
            int numVerts = (int)poly.verts.size();

            // Vertex count in lower 3 bits, axis flag in bits 3-4
            // AXIS_NONE (24) = explicit vertices, no 2-vert compression
            // Walls use SWAPXY (64) flag — verified from working test_map
            // Do NOT set POLY_FLAG_WALL_TEXTURE (32) — test_map doesn't use it
            uint8_t polyFlags = (uint8_t)(((numVerts - 2) & 0x7) | 24); // POLY_FLAG_AXIS_NONE
            if (poly.isWall)
                polyFlags |= 64; // POLY_FLAG_SWAPXY
            polyData.push_back(polyFlags);

            for (auto& v : poly.verts) {
                polyData.push_back(v.x);
                polyData.push_back(v.y);
                polyData.push_back(v.z);
                polyData.push_back((uint8_t)v.s);
                polyData.push_back((uint8_t)v.t);
            }
        }
      } // chunk loop
    }
}

// ---- Main compilation ----

bool BSPCompiler::compile(const MapData& map, BSPCompileResult& result, std::string& errorMsg) {
    result = BSPCompileResult{};

    // Generate geometry
    std::vector<RawPolygon> polys;
    std::vector<RawLine> rawLines;
    generateGeometry(map, polys, rawLines);

    // Handle empty map
    if (polys.empty()) {
        // Create minimal BSP with a single empty leaf
        BSPNode leaf;
        leaf.offset = (int16_t)0xFFFF; // leaf
        leaf.normalIdx = 0;
        leaf.child1 = 0; // polyData offset
        leaf.child2 = 0; // no lines
        leaf.bounds[0] = 0;
        leaf.bounds[1] = 0;
        leaf.bounds[2] = 16; // 32 tiles * 64 / 128
        leaf.bounds[3] = 16;

        result.nodes.push_back(leaf);
        result.normals = {-16384, 0, 0}; // single dummy normal
        result.polyData.push_back(0);   // meshCount = 0
        return true;
    }

    // Set up normals for axis-aligned BSP
    // Normal 0: +X (16384, 0, 0)
    // Normal 1: +Y (0, 16384, 0)
    result.normals = {
        -16384, 0, 0,  // -X (negative, matching engine convention)
        0, -16384, 0   // -Y
    };

    // Build BSP tree
    std::vector<int> allPolyIdx(polys.size());
    for (int i = 0; i < (int)polys.size(); i++) allPolyIdx[i] = i;

    std::vector<int> allLineIdx(rawLines.size());
    for (int i = 0; i < (int)rawLines.size(); i++) allLineIdx[i] = i;

    std::vector<BSPBuildNode> buildNodes;
    int rootIdx = buildBSPRecursive(buildNodes, polys, rawLines,
                                     allPolyIdx, allLineIdx,
                                     0, 0, MAP_SIZE * 64, MAP_SIZE * 64, 0);

    // Flatten BSP tree into output arrays
    // First, collect all lines in leaf order so each leaf gets a contiguous range
    result.lines.clear();
    std::vector<int> leafLineStart(buildNodes.size(), 0);
    std::vector<int> leafLineCount(buildNodes.size(), 0);

    for (int i = 0; i < (int)buildNodes.size(); i++) {
        if (buildNodes[i].isLeaf) {
            leafLineStart[i] = (int)result.lines.size();
            leafLineCount[i] = (int)buildNodes[i].lineIndices.size();
            for (int li : buildNodes[i].lineIndices) {
                LineSegment seg;
                seg.x1 = rawLines[li].x1;
                seg.x2 = rawLines[li].x2;
                seg.y1 = rawLines[li].y1;
                seg.y2 = rawLines[li].y2;
                seg.flags = rawLines[li].flags;
                result.lines.push_back(seg);
            }
        }
    }

    // Pack polygon data for each leaf
    std::vector<int> leafPolyOffset(buildNodes.size(), 0);
    for (int i = 0; i < (int)buildNodes.size(); i++) {
        if (buildNodes[i].isLeaf) {
            int offset;
            packPolygons(polys, buildNodes[i].polyIndices, result.polyData, offset);
            leafPolyOffset[i] = offset;
        }
    }

    // Convert build nodes to output BSP nodes
    result.nodes.resize(buildNodes.size());
    for (int i = 0; i < (int)buildNodes.size(); i++) {
        auto& bn = buildNodes[i];
        auto& on = result.nodes[i];

        on.bounds[0] = bn.minX;
        on.bounds[1] = bn.minY;
        on.bounds[2] = bn.maxX;
        on.bounds[3] = bn.maxY;

        if (bn.isLeaf) {
            on.offset = (int16_t)0xFFFF;
            on.normalIdx = 0;
            on.child1 = (int16_t)(leafPolyOffset[i] & 0xFFFF);

            // Pack line reference: lower 10 bits = first line, bits 10-15 = count
            int lineStart = leafLineStart[i];
            int lineCount = leafLineCount[i];
            on.child2 = (int16_t)((lineStart & 0x3FF) | ((lineCount & 0x3F) << 10));
        } else {
            // Interior node
            on.normalIdx = (uint8_t)(bn.axis); // 0=X normal, 1=Y normal

            // With negative normals: classifyPoint = -viewX + offset
            // viewX = worldX * 16 (render space). Split at worldX = S.
            // Classify = 0 at split: -S*16 + offset = 0 → offset = S * 16
            int splitWorld = bn.splitCoord;
            on.offset = (int16_t)(splitWorld * 16);

            on.child1 = (int16_t)bn.leftChild;
            on.child2 = (int16_t)bn.rightChild;
        }
    }

    // Validate sizes
    if (result.polyData.size() > 65535) {
        errorMsg = "Polygon data exceeds 65535 bytes limit (got " +
                   std::to_string(result.polyData.size()) + ")";
        return false;
    }
    if (result.lines.size() > 1023) {
        errorMsg = "Too many line segments (got " +
                   std::to_string(result.lines.size()) + ", max 1023)";
        return false;
    }

    return true;
}
