// bsp2bin — Convert D1 RPG .bsp to D2 RPG .bin (tile-grid based)
//
// Direct C++ port of tools/bsp-to-bin.py.  Reads the D1 blockmap, generates
// floor/ceiling/wall geometry, builds a crossing-minimisation BSP tree, and
// writes a D2-compatible .bin level file.
//
// Build (standalone):
//   g++ -std=c++17 -O2 tools/bsp2bin.cpp -o tools/bsp2bin
//
// Or include BspToBin.h / BspToBin::convert() from the converter.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// ── Constants ───────────────────────────────────────────────────────────────

static constexpr int MAP_SIZE          = 32;
static constexpr int TILE_SIZE         = 64;
static constexpr int WALL_HEIGHT_WORLD = 64;
static constexpr int FLOOR_HEIGHT      = 0;   // ground level (matching D2 test_map)

static constexpr int D2_WALL_TILE  = 258;
static constexpr int D2_FLOOR_TILE = 451;
static constexpr int D2_CEIL_TILE  = 462;

static constexpr uint8_t POLY_FLAG_AXIS_NONE = 24;

static constexpr uint32_t MARKER_DEAD = 0xDEADBEEF;
static constexpr uint32_t MARKER_CAFE = 0xCAFEBABE;

// ── Binary helpers ──────────────────────────────────────────────────────────

static uint8_t  rdU8 (const uint8_t* d, int& p) { return d[p++]; }
static int16_t  rdI16(const uint8_t* d, int& p) { int16_t  v; memcpy(&v, d+p, 2); p += 2; return v; }
static int32_t  rdI32(const uint8_t* d, int& p) { int32_t  v; memcpy(&v, d+p, 4); p += 4; return v; }

static void wrU8 (std::vector<uint8_t>& o, uint8_t  v) { o.push_back(v); }
static void wrI16(std::vector<uint8_t>& o, int16_t  v) { o.insert(o.end(), (uint8_t*)&v, (uint8_t*)&v+2); }
static void wrU16(std::vector<uint8_t>& o, uint16_t v) { o.insert(o.end(), (uint8_t*)&v, (uint8_t*)&v+2); }
static void wrI32(std::vector<uint8_t>& o, int32_t  v) { o.insert(o.end(), (uint8_t*)&v, (uint8_t*)&v+4); }
static void wrU32(std::vector<uint8_t>& o, uint32_t v) { o.insert(o.end(), (uint8_t*)&v, (uint8_t*)&v+4); }
static void wrMarker(std::vector<uint8_t>& o, uint32_t m) { wrU32(o, m); }

// ── D1 BSP Parser ───────────────────────────────────────────────────────────

struct D1Line { int v1x, v1y, v2x, v2y; int16_t texture; int32_t flags; };

struct D1BSP {
    char     mapName[17] = {};
    uint8_t  floorTex = 0, ceilTex = 0;
    int16_t  spawnIndex = 0;
    uint8_t  spawnDir = 0;
    uint8_t  blockMap[1024] = {};
    std::vector<D1Line> lines;
};

static D1BSP parseD1BSP(const uint8_t* d, int len) {
    D1BSP bsp;
    int p = 0;

    memcpy(bsp.mapName, d, 16); bsp.mapName[16] = 0;
    p = 16;
    p += 6; // floor/ceiling color
    bsp.floorTex = rdU8(d, p);
    bsp.ceilTex  = rdU8(d, p);
    p += 3; // intro color
    p += 1; // loadMapID
    bsp.spawnIndex = rdI16(d, p);
    bsp.spawnDir   = rdU8(d, p);
    p += 2; // cameraSpawnIndex

    // Skip D1 nodes
    int16_t nn = rdI16(d, p);
    p += nn * 10;

    // Read D1 lines
    int16_t nl = rdI16(d, p);
    bsp.lines.resize(nl);
    for (int i = 0; i < nl; i++) {
        bsp.lines[i].v1x = rdU8(d, p) << 3;
        bsp.lines[i].v1y = rdU8(d, p) << 3;
        bsp.lines[i].v2x = rdU8(d, p) << 3;
        bsp.lines[i].v2y = rdU8(d, p) << 3;
        bsp.lines[i].texture = rdI16(d, p);
        bsp.lines[i].flags   = rdI32(d, p);
    }

    // Skip sprites, events, bytecodes, strings
    int16_t ns = rdI16(d, p); p += ns * 5;
    int16_t ne = rdI16(d, p); p += ne * 4;
    int16_t nb = rdI16(d, p); p += nb * 9;
    int16_t nstr = rdI16(d, p);
    for (int i = 0; i < nstr; i++) { int16_t sl = rdI16(d, p); p += sl; }

    // BlockMap (256 bytes → 1024 tiles, 2 bits each)
    int bi = 0;
    for (int j = 0; j < 256; j++) {
        uint8_t f = rdU8(d, p);
        bsp.blockMap[bi+0] = f & 3;
        bsp.blockMap[bi+1] = (f >> 2) & 3;
        bsp.blockMap[bi+2] = (f >> 4) & 3;
        bsp.blockMap[bi+3] = (f >> 6) & 3;
        bi += 4;
    }
    return bsp;
}

// ── Tile helpers ────────────────────────────────────────────────────────────

static bool isSolid(const uint8_t* bm, int col, int row) {
    if (col < 0 || col >= MAP_SIZE || row < 0 || row >= MAP_SIZE) return true;
    return (bm[row * MAP_SIZE + col] & 1) != 0;
}

static bool isDoorTile(const D1BSP& bsp, int col, int row) {
    int cx = col * TILE_SIZE + TILE_SIZE / 2;
    int cy = row * TILE_SIZE + TILE_SIZE / 2;
    for (auto& l : bsp.lines) {
        if (l.texture < 0 || l.texture > 10) continue;
        int mx = (l.v1x + l.v2x) / 2;
        int my = (l.v1y + l.v2y) / 2;
        if (abs(mx - cx) <= TILE_SIZE && abs(my - cy) <= TILE_SIZE) return true;
    }
    return false;
}

// ── Geometry ────────────────────────────────────────────────────────────────

struct Vertex { uint8_t x, y, z, s, t; };

struct Polygon {
    Vertex   verts[4];
    int      numVerts = 4;
    int      tileNum  = 0;
    bool     isWall   = false;
    bool     isDiag   = false; // diagonal wall (for BSP post-split)
    int      tileCenterX = 0;  // world coords — for BSP classification
    int      tileCenterY = 0;
    int      diagDx = 0, diagDy = 0; // D1 line direction (for diagonal normal selection)
};

struct CollLine { uint8_t x1, x2, y1, y2; uint8_t flags; };

static void generateGeometry(const D1BSP& bsp,
                              std::vector<Polygon>& polys,
                              std::vector<CollLine>& lines)
{
    int bzl = FLOOR_HEIGHT >> 3; // floor Z byte
    int bzh = bzl + 8;           // ceiling Z byte

    // ── Part A: Floors, ceilings, and axis-aligned walls from blockmap (proven) ──
    for (int row = 0; row < MAP_SIZE; row++) {
        for (int col = 0; col < MAP_SIZE; col++) {
            if (isSolid(bsp.blockMap, col, row)) continue;

            int wx0 = col * 64,     wy0 = row * 64;
            int wx1 = (col+1) * 64, wy1 = (row+1) * 64;
            uint8_t bx0 = wx0 >> 3, by0 = wy0 >> 3;
            uint8_t bx1 = wx1 >> 3, by1 = wy1 >> 3;
            int tcx = col * 64 + 32, tcy = row * 64 + 32;

            // Floor — CW from above
            {
                Polygon p;
                p.tileNum = D2_FLOOR_TILE; p.isWall = false;
                p.tileCenterX = tcx; p.tileCenterY = tcy;
                p.verts[0] = {bx0, by1, (uint8_t)bzl, 0, 1};
                p.verts[1] = {bx1, by1, (uint8_t)bzl, 1, 1};
                p.verts[2] = {bx1, by0, (uint8_t)bzl, 1, 0};
                p.verts[3] = {bx0, by0, (uint8_t)bzl, 0, 0};
                polys.push_back(p);
            }
            // Ceiling — CCW from above
            {
                Polygon p;
                p.tileNum = D2_CEIL_TILE; p.isWall = false;
                p.tileCenterX = tcx; p.tileCenterY = tcy;
                p.verts[0] = {bx0, by0, (uint8_t)bzh, 0, 0};
                p.verts[1] = {bx1, by0, (uint8_t)bzh, 1, 0};
                p.verts[2] = {bx1, by1, (uint8_t)bzh, 1, 1};
                p.verts[3] = {bx0, by1, (uint8_t)bzh, 0, 1};
                polys.push_back(p);
            }

            // Axis-aligned walls at solid/open tile boundaries
            uint8_t doorFlag = isDoorTile(bsp, col, row) ? 4 : 0;

            auto addWall = [&](Vertex v0, Vertex v1, Vertex v2, Vertex v3,
                               uint8_t lx1, uint8_t lx2, uint8_t ly1, uint8_t ly2) {
                Polygon p;
                p.tileNum = D2_WALL_TILE; p.isWall = true;
                p.tileCenterX = tcx; p.tileCenterY = tcy;
                p.verts[0] = v0; p.verts[1] = v1; p.verts[2] = v2; p.verts[3] = v3;
                polys.push_back(p);
                lines.push_back({lx1, lx2, ly1, ly2, doorFlag});
            };

            if (isSolid(bsp.blockMap, col, row - 1)) { // North
                uint8_t by = wy0 >> 3;
                addWall({bx1,by,(uint8_t)bzh,1,1}, {bx0,by,(uint8_t)bzh,0,1},
                        {bx0,by,(uint8_t)bzl,0,0}, {bx1,by,(uint8_t)bzl,1,0},
                        bx0, bx1, by, by);
            }
            if (isSolid(bsp.blockMap, col, row + 1)) { // South
                uint8_t by = wy1 >> 3;
                addWall({bx0,by,(uint8_t)bzh,0,1}, {bx1,by,(uint8_t)bzh,1,1},
                        {bx1,by,(uint8_t)bzl,1,0}, {bx0,by,(uint8_t)bzl,0,0},
                        bx0, bx1, by, by);
            }
            if (isSolid(bsp.blockMap, col - 1, row)) { // West
                uint8_t bx = wx0 >> 3;
                addWall({bx,by0,(uint8_t)bzh,0,1}, {bx,by1,(uint8_t)bzh,1,1},
                        {bx,by1,(uint8_t)bzl,1,0}, {bx,by0,(uint8_t)bzl,0,0},
                        bx, bx, by0, by1);
            }
            if (isSolid(bsp.blockMap, col + 1, row)) { // East
                uint8_t bx = wx1 >> 3;
                addWall({bx,by1,(uint8_t)bzh,1,1}, {bx,by0,(uint8_t)bzh,0,1},
                        {bx,by0,(uint8_t)bzl,0,0}, {bx,by1,(uint8_t)bzl,1,0},
                        bx, bx, by0, by1);
            }
        }
    }

    // ── Part B: Diagonal wall quads from D1 lines ──
    for (auto& dl : bsp.lines) {
        // Only process diagonal lines (non-axis-aligned)
        if (dl.v1x == dl.v2x || dl.v1y == dl.v2y) continue; // axis-aligned → skip
        if (dl.v1x == dl.v2x && dl.v1y == dl.v2y) continue;  // zero-length

        int v1x = dl.v1x, v1y = dl.v1y, v2x = dl.v2x, v2y = dl.v2y;
        int midX = (v1x + v2x) / 2, midY = (v1y + v2y) / 2;

        // Determine wall facing: check tile on each side of the line (32 units offset)
        int dx = v2x - v1x, dy = v2y - v1y;
        // Right normal direction = (sign(dy), sign(-dx)), scaled by half-tile
        int rnx = (dy > 0 ? 32 : (dy < 0 ? -32 : 0));
        int rny = (-dx > 0 ? 32 : (-dx < 0 ? -32 : 0));
        int rightCol = (midX + rnx) / TILE_SIZE;
        int rightRow = (midY + rny) / TILE_SIZE;

        // If right-normal side is solid, front face should point LEFT → swap endpoints
        if (isSolid(bsp.blockMap, rightCol, rightRow)) {
            std::swap(v1x, v2x);
            std::swap(v1y, v2y);
        }

        uint8_t bx1 = (uint8_t)(v1x >> 3), by1 = (uint8_t)(v1y >> 3);
        uint8_t bx2 = (uint8_t)(v2x >> 3), by2 = (uint8_t)(v2y >> 3);

        // Emit diagonal wall poly into BOTH adjacent tile leaves to prevent
        // painter's algorithm bleed-through at tile boundaries.
        // Compute the two tile centers on each side of the diagonal.
        int tile1Col = dl.v1x / TILE_SIZE, tile1Row = dl.v1y / TILE_SIZE;
        int tile2Col = dl.v2x / TILE_SIZE, tile2Row = dl.v2y / TILE_SIZE;
        int tc1x = tile1Col * TILE_SIZE + TILE_SIZE / 2;
        int tc1y = tile1Row * TILE_SIZE + TILE_SIZE / 2;
        int tc2x = tile2Col * TILE_SIZE + TILE_SIZE / 2;
        int tc2y = tile2Row * TILE_SIZE + TILE_SIZE / 2;

        // Diagonal wall poly — single copy, BSP post-split will handle ordering
        {
            Polygon p;
            p.tileNum = D2_WALL_TILE; p.isWall = true; p.isDiag = true;
            p.tileCenterX = midX; p.tileCenterY = midY;
            p.diagDx = dl.v2x - dl.v1x; p.diagDy = dl.v2y - dl.v1y;
            p.verts[0] = {bx1, by1, (uint8_t)bzh, 0, 1};
            p.verts[1] = {bx2, by2, (uint8_t)bzh, 1, 1};
            p.verts[2] = {bx2, by2, (uint8_t)bzl, 1, 0};
            p.verts[3] = {bx1, by1, (uint8_t)bzl, 0, 0};
            polys.push_back(p);
        }

        // Diagonal collision line
        lines.push_back({
            (uint8_t)(dl.v1x >> 3), (uint8_t)(dl.v2x >> 3),
            (uint8_t)(dl.v1y >> 3), (uint8_t)(dl.v2y >> 3),
            0}); // blocking
    }
}

// ── BSP Tree (crossing-minimisation) ────────────────────────────────────────

struct BSPNode {
    bool isLeaf  = false;
    int  axis    = 0;        // 0=X, 1=Y, 2=diag NE-SW, 3=diag NW-SE
    int  splitCoord = 0;     // world coords (axis-aligned) or 0 (diag uses diagOffset)
    int  diagOffset = 0;     // render-space offset for diagonal splits
    int  left = -1, right = -1;
    std::vector<int> polyIndices;
    std::vector<int> lineIndices;
    uint8_t minX = 0, minY = 0, maxX = 0, maxY = 0;
};

static int countCrossingsX(const uint8_t* bm, int col, int r0, int r1) {
    int c = 0;
    for (int r = r0; r < r1; r++)
        if (col > 0 && !isSolid(bm, col-1, r) && !isSolid(bm, col, r)) c++;
    return c;
}
static int countCrossingsY(const uint8_t* bm, int row, int c0, int c1) {
    int c = 0;
    for (int cc = c0; cc < c1; cc++)
        if (row > 0 && !isSolid(bm, cc, row-1) && !isSolid(bm, cc, row)) c++;
    return c;
}

static bool hasOpenTiles(const uint8_t* bm, int c0, int r0, int c1, int r1) {
    for (int r = r0; r < r1; r++)
        for (int c = c0; c < c1; c++)
            if (!isSolid(bm, c, r)) return true;
    return false;
}

static int buildBSP(const uint8_t* bm,
                     const std::vector<Polygon>& polys,
                     const std::vector<CollLine>& lines,
                     std::vector<int> polyIdxs,
                     std::vector<int> lineIdxs,
                     int minX, int minY, int maxX, int maxY,
                     int depth,
                     std::vector<BSPNode>& nodes)
{
    int idx = (int)nodes.size();
    nodes.emplace_back();
    BSPNode& node = nodes[idx];
    node.minX = (uint8_t)std::min(255, minX >> 3);
    node.minY = (uint8_t)std::min(255, minY >> 3);
    node.maxX = (uint8_t)std::min(255, maxX >> 3);
    node.maxY = (uint8_t)std::min(255, maxY >> 3);

    int c0 = minX / 64, r0 = minY / 64;
    int c1 = maxX / 64, r1 = maxY / 64;

    // Leaf condition
    if ((c1 - c0 <= 1 && r1 - r0 <= 1) || polyIdxs.empty()) {
        node.isLeaf = true;
        node.polyIndices = std::move(polyIdxs);
        node.lineIndices = std::move(lineIdxs);
        return idx;
    }

    // Find best split — minimise crossings
    int bestAxis = -1, bestSplit = 0, bestCross = 999999, bestDist = 999999;

    for (int axisTry : {depth % 2, 1 - depth % 2}) {
        if (axisTry == 0) { // X splits
            int mid = (c0 + c1) / 2;
            for (int col = c0 + 1; col < c1; col++) {
                if (!hasOpenTiles(bm, c0, r0, col, r1)) continue;
                if (!hasOpenTiles(bm, col, r0, c1, r1)) continue;
                int cross = countCrossingsX(bm, col, r0, r1);
                int dist  = abs(col - mid);
                if (cross < bestCross || (cross == bestCross && dist < bestDist)) {
                    bestCross = cross; bestDist = dist;
                    bestAxis = 0; bestSplit = col * 64;
                }
            }
        } else { // Y splits
            int mid = (r0 + r1) / 2;
            for (int row = r0 + 1; row < r1; row++) {
                if (!hasOpenTiles(bm, c0, r0, c1, row)) continue;
                if (!hasOpenTiles(bm, c0, row, c1, r1)) continue;
                int cross = countCrossingsY(bm, row, c0, c1);
                int dist  = abs(row - mid);
                if (cross < bestCross || (cross == bestCross && dist < bestDist)) {
                    bestCross = cross; bestDist = dist;
                    bestAxis = 1; bestSplit = row * 64;
                }
            }
        }
    }

    if (bestAxis < 0) {
        node.isLeaf = true;
        node.polyIndices = std::move(polyIdxs);
        node.lineIndices = std::move(lineIdxs);
        return idx;
    }

    node.axis = bestAxis;
    node.splitCoord = bestSplit;

    // Partition polys by tile center
    std::vector<int> leftP, rightP;
    for (int pi : polyIdxs) {
        int center = (node.axis == 0) ? polys[pi].tileCenterX : polys[pi].tileCenterY;
        (center < node.splitCoord ? leftP : rightP).push_back(pi);
    }

    // Partition lines by midpoint
    std::vector<int> leftL, rightL;
    for (int li : lineIdxs) {
        int center;
        if (node.axis == 0)
            center = ((int)lines[li].x1 + lines[li].x2) * 8 / 2;
        else
            center = ((int)lines[li].y1 + lines[li].y2) * 8 / 2;
        (center < node.splitCoord ? leftL : rightL).push_back(li);
    }

    if (leftP.empty() || rightP.empty()) {
        node.isLeaf = true;
        node.polyIndices = std::move(polyIdxs);
        node.lineIndices = std::move(lineIdxs);
        return idx;
    }

    node.isLeaf = false;
    int lMinX, lMinY, lMaxX, lMaxY, rMinX, rMinY, rMaxX, rMaxY;
    if (node.axis == 0) {
        lMinX = minX; lMinY = minY; lMaxX = node.splitCoord; lMaxY = maxY;
        rMinX = node.splitCoord; rMinY = minY; rMaxX = maxX; rMaxY = maxY;
    } else {
        lMinX = minX; lMinY = minY; lMaxX = maxX; lMaxY = node.splitCoord;
        rMinX = minX; rMinY = node.splitCoord; rMaxX = maxX; rMaxY = maxY;
    }

    int leftIdx  = buildBSP(bm, polys, lines, std::move(leftP),  std::move(leftL),
                             lMinX, lMinY, lMaxX, lMaxY, depth+1, nodes);
    int rightIdx = buildBSP(bm, polys, lines, std::move(rightP), std::move(rightL),
                             rMinX, rMinY, rMaxX, rMaxY, depth+1, nodes);
    nodes[idx].left  = leftIdx;
    nodes[idx].right = rightIdx;
    return idx;
}

// ── Diagonal BSP post-split ─────────────────────────────────────────────────

// Diagonal normals (int16): -16384/sqrt(2) ≈ -11585
// Index 2: NE-SW diagonal normal (-11585, -11585, 0) — perpendicular to lines with sign(dx)==sign(dy)
// Index 3: NW-SE diagonal normal (-11585,  11585, 0) — perpendicular to lines with sign(dx)!=sign(dy)
static constexpr int16_t DIAG_N = -11585;

// Classify a world-coord point against a diagonal split plane.
// Returns >= 0 for front (child1), < 0 for back (child2).
static int diagClassify(int worldX, int worldY, int normalIdx, int diagOffset) {
    int rx = worldX * 16 + 8, ry = worldY * 16 + 8;
    int nx, ny;
    if (normalIdx == 2) { nx = DIAG_N; ny = DIAG_N; }
    else                { nx = DIAG_N; ny = -DIAG_N; }
    return ((int64_t(rx) * nx + int64_t(ry) * ny) >> 14) + diagOffset;
}

// Compute the render-space offset for a diagonal split through a world-coord point.
static int diagComputeOffset(int worldX, int worldY, int normalIdx) {
    int rx = worldX * 16 + 8, ry = worldY * 16 + 8;
    int nx, ny;
    if (normalIdx == 2) { nx = DIAG_N; ny = DIAG_N; }
    else                { nx = DIAG_N; ny = -DIAG_N; }
    return -(int)((int64_t(rx) * nx + int64_t(ry) * ny) >> 14);
}

// Post-process: split leaf nodes that contain diagonal wall polys.
static void postSplitDiagonals(std::vector<BSPNode>& nodes,
                                const std::vector<Polygon>& polys,
                                const std::vector<CollLine>& lines)
{
    // Iterate over existing nodes (size may grow as we add children)
    int originalCount = (int)nodes.size();
    for (int i = 0; i < originalCount; i++) {
        if (!nodes[i].isLeaf) continue;

        // Find first diagonal poly in this leaf
        int diagPolyIdx = -1;
        for (int pi : nodes[i].polyIndices) {
            if (polys[pi].isDiag) { diagPolyIdx = pi; break; }
        }
        if (diagPolyIdx < 0) continue; // no diagonals, skip

        const Polygon& dp = polys[diagPolyIdx];

        // Determine diagonal normal index from line direction
        // sign(dx) == sign(dy) → NE-SW → normal 2; otherwise → normal 3
        int normalIdx;
        if ((dp.diagDx > 0) == (dp.diagDy > 0))
            normalIdx = 2;
        else
            normalIdx = 3;

        // Split plane passes through the diagonal's midpoint
        int midX = dp.tileCenterX, midY = dp.tileCenterY;
        int offset = diagComputeOffset(midX, midY, normalIdx);

        // Classify polys: walls by their center, floors/ceilings go in BOTH children
        // (floor/ceiling quads span the full tile and can't be cleanly split by a diagonal)
        std::vector<int> frontPolys, backPolys;
        for (int pi : nodes[i].polyIndices) {
            if (!polys[pi].isWall) {
                // Floor/ceiling → duplicate into both children
                frontPolys.push_back(pi);
                backPolys.push_back(pi);
            } else {
                int cx = polys[pi].tileCenterX, cy = polys[pi].tileCenterY;
                int side = diagClassify(cx, cy, normalIdx, offset);
                if (side >= 0)
                    frontPolys.push_back(pi);
                else
                    backPolys.push_back(pi);
            }
        }

        // Need at least one wall on each side for the split to help
        bool hasWallFront = false, hasWallBack = false;
        for (int pi : frontPolys) if (polys[pi].isWall) { hasWallFront = true; break; }
        for (int pi : backPolys) if (polys[pi].isWall) { hasWallBack = true; break; }
        if (!hasWallFront || !hasWallBack) continue;

        // Classify collision lines
        std::vector<int> frontLines, backLines;
        for (int li : nodes[i].lineIndices) {
            int lmx = ((int)lines[li].x1 + lines[li].x2) * 4; // midpoint world coords
            int lmy = ((int)lines[li].y1 + lines[li].y2) * 4;
            int side = diagClassify(lmx, lmy, normalIdx, offset);
            if (side >= 0)
                frontLines.push_back(li);
            else
                backLines.push_back(li);
        }

        // Convert leaf → interior node with diagonal split
        nodes[i].isLeaf = false;
        nodes[i].axis = normalIdx; // 2 or 3
        nodes[i].diagOffset = offset;

        // Create front child (child1) — open side + diagonal wall
        int frontIdx = (int)nodes.size();
        nodes.emplace_back();
        nodes[frontIdx].isLeaf = true;
        nodes[frontIdx].polyIndices = std::move(frontPolys);
        nodes[frontIdx].lineIndices = std::move(frontLines);
        nodes[frontIdx].minX = nodes[i].minX; nodes[frontIdx].minY = nodes[i].minY;
        nodes[frontIdx].maxX = nodes[i].maxX; nodes[frontIdx].maxY = nodes[i].maxY;

        // Create back child (child2) — solid side
        int backIdx = (int)nodes.size();
        nodes.emplace_back();
        nodes[backIdx].isLeaf = true;
        nodes[backIdx].polyIndices = std::move(backPolys);
        nodes[backIdx].lineIndices = std::move(backLines);
        nodes[backIdx].minX = nodes[i].minX; nodes[backIdx].minY = nodes[i].minY;
        nodes[backIdx].maxX = nodes[i].maxX; nodes[backIdx].maxY = nodes[i].maxY;

        nodes[i].left = frontIdx;
        nodes[i].right = backIdx;
        nodes[i].polyIndices.clear();
        nodes[i].lineIndices.clear();
    }
}

// ── Polygon packing ─────────────────────────────────────────────────────────

static std::vector<uint8_t> packPolys(const std::vector<Polygon>& polys,
                                       const std::vector<int>& indices)
{
    // Group by texture, floors first then walls
    struct Entry { int tileNum; const Polygon* poly; };
    std::vector<Entry> floors, walls;
    for (int pi : indices) {
        if (polys[pi].isWall) walls.push_back({polys[pi].tileNum, &polys[pi]});
        else                  floors.push_back({polys[pi].tileNum, &polys[pi]});
    }

    // Merge floors + walls, then group by texture
    std::vector<Entry> ordered;
    ordered.insert(ordered.end(), floors.begin(), floors.end());
    ordered.insert(ordered.end(), walls.begin(), walls.end());

    // Build texture→entries map preserving insertion order
    std::vector<int> texOrder;
    std::map<int, std::vector<const Polygon*>> byTex;
    for (auto& e : ordered) {
        if (byTex.find(e.tileNum) == byTex.end()) texOrder.push_back(e.tileNum);
        byTex[e.tileNum].push_back(e.poly);
    }

    std::vector<uint8_t> data;
    data.push_back((uint8_t)texOrder.size()); // meshCount

    for (int tex : texOrder) {
        auto& entries = byTex[tex];
        int count = std::min((int)entries.size(), 127);

        // 4 reserved bytes before mesh header
        data.push_back(0); data.push_back(0); data.push_back(0); data.push_back(0);
        uint16_t packed = ((tex & 0x1FF) << 7) | (count & 0x7F);
        data.push_back(packed & 0xFF);
        data.push_back((packed >> 8) & 0xFF);

        for (int i = 0; i < count; i++) {
            auto* p = entries[i];
            uint8_t flags = ((p->numVerts - 2) & 0x7) | POLY_FLAG_AXIS_NONE;
            if (p->isWall) flags |= 0x40; // SWAPXY
            data.push_back(flags);
            for (int v = 0; v < p->numVerts; v++) {
                data.push_back(p->verts[v].x);
                data.push_back(p->verts[v].y);
                data.push_back(p->verts[v].z);
                data.push_back(p->verts[v].s);
                data.push_back(p->verts[v].t);
            }
        }
    }
    return data;
}

// ── Full conversion ─────────────────────────────────────────────────────────

struct ConvertResult {
    std::vector<uint8_t> binData;
    int numPolys, numLines, numNodes;
};

static ConvertResult convertBSPtoBIN(const D1BSP& bsp) {
    // 1. Geometry
    std::vector<Polygon>  polys;
    std::vector<CollLine> lines;
    generateGeometry(bsp, polys, lines);

    // 2. BSP tree
    std::vector<int> allPolyIdx(polys.size()), allLineIdx(lines.size());
    for (int i = 0; i < (int)polys.size(); i++) allPolyIdx[i] = i;
    for (int i = 0; i < (int)lines.size(); i++) allLineIdx[i] = i;

    std::vector<BSPNode> nodes;
    buildBSP(bsp.blockMap, polys, lines, allPolyIdx, allLineIdx,
             0, 0, MAP_SIZE * TILE_SIZE, MAP_SIZE * TILE_SIZE, 0, nodes);

    // 2b. Post-split leaves containing diagonal walls
    postSplitDiagonals(nodes, polys, lines);

    // 3. Collect lines in leaf order
    std::vector<CollLine> orderedLines;
    std::map<int,int> leafLineStart, leafLineCount;
    for (int i = 0; i < (int)nodes.size(); i++) {
        if (!nodes[i].isLeaf) continue;
        leafLineStart[i] = (int)orderedLines.size();
        leafLineCount[i] = (int)nodes[i].lineIndices.size();
        for (int li : nodes[i].lineIndices) orderedLines.push_back(lines[li]);
    }

    // 4. Pack poly data per leaf
    std::map<int,int> leafPolyOffset;
    std::vector<uint8_t> allPolyData;
    for (int i = 0; i < (int)nodes.size(); i++) {
        if (!nodes[i].isLeaf) continue;
        leafPolyOffset[i] = (int)allPolyData.size();
        auto d = packPolys(polys, nodes[i].polyIndices);
        allPolyData.insert(allPolyData.end(), d.begin(), d.end());
    }

    // 5. Normals: X-axis, Y-axis, diagonal NE-SW, diagonal NW-SE
    int16_t normals[] = {
        -16384, 0, 0,       // 0: X-axis
        0, -16384, 0,       // 1: Y-axis
        DIAG_N, DIAG_N, 0,  // 2: NE-SW diagonal (-11585, -11585, 0)
        DIAG_N, -DIAG_N, 0, // 3: NW-SE diagonal (-11585,  11585, 0)
    };
    int numNormals = 4;

    // 6. Output arrays
    int numNodes  = (int)nodes.size();
    int numLinesOut = (int)orderedLines.size();
    int dataSizePolys = (int)allPolyData.size();

    // ── Write .bin ──
    std::vector<uint8_t> out;

    uint8_t d2SpawnDir = (bsp.spawnDir / 32) & 7;

    // Header (42 bytes)
    wrU8(out, 3);
    wrI32(out, 0x44314250);
    wrU16(out, (uint16_t)bsp.spawnIndex);
    wrU8(out, d2SpawnDir);
    wrU8(out, 0); // flagsBitmask
    wrU8(out, 0); // totalSecrets
    wrU8(out, 0); // totalLoot
    wrU16(out, (uint16_t)numNodes);
    wrU16(out, (uint16_t)dataSizePolys);
    wrU16(out, (uint16_t)numLinesOut);
    wrU16(out, (uint16_t)numNormals);
    wrU16(out, 1);  // numNormalSprites (1 dummy)
    wrI16(out, 0);  // numZSprites
    wrI16(out, 0);  // numTileEvents
    wrI16(out, 0);  // mapByteCodeSize
    wrU8(out, 0);   // totalMayaCameras
    wrI16(out, 0);  // totalMayaCameraKeys
    for (int i = 0; i < 6; i++) wrI16(out, -1);

    // 1. Media
    uint16_t mediaIndices[] = {D2_FLOOR_TILE, D2_WALL_TILE, D2_CEIL_TILE}; // sorted
    // actually sort them
    std::vector<uint16_t> media = {D2_WALL_TILE, D2_FLOOR_TILE, D2_CEIL_TILE};
    std::sort(media.begin(), media.end());
    wrMarker(out, MARKER_DEAD);
    wrU16(out, (uint16_t)media.size());
    for (auto m : media) wrU16(out, m);
    wrMarker(out, MARKER_DEAD);

    // 2. Normals
    for (int i = 0; i < numNormals * 3; i++) wrI16(out, normals[i]);
    wrMarker(out, MARKER_CAFE);

    // 3. Node offsets
    for (auto& n : nodes) {
        if (n.isLeaf)
            wrU16(out, 0xFFFF);
        else if (n.axis >= 2) // diagonal split
            wrU16(out, (uint16_t)(n.diagOffset & 0xFFFF));
        else // axis-aligned split
            wrU16(out, (uint16_t)((n.splitCoord * 16) & 0xFFFF));
    }
    wrMarker(out, MARKER_CAFE);

    // 4. Node normal indices
    for (auto& n : nodes) wrU8(out, n.isLeaf ? 0 : (uint8_t)n.axis);
    wrMarker(out, MARKER_CAFE);

    // 5. Node children
    for (auto& n : nodes) {
        if (n.isLeaf)
            wrU16(out, (uint16_t)(leafPolyOffset.count((&n - &nodes[0])) ?
                                   leafPolyOffset[&n - &nodes[0]] : 0));
        else
            wrU16(out, (uint16_t)(n.left & 0xFFFF));
    }
    for (int i = 0; i < (int)nodes.size(); i++) {
        auto& n = nodes[i];
        if (n.isLeaf) {
            int ls = leafLineStart.count(i) ? leafLineStart[i] : 0;
            int lc = leafLineCount.count(i) ? leafLineCount[i] : 0;
            wrU16(out, (uint16_t)((ls & 0x3FF) | ((lc & 0x3F) << 10)));
        } else {
            wrU16(out, (uint16_t)(n.right & 0xFFFF));
        }
    }
    wrMarker(out, MARKER_CAFE);

    // 6. Node bounds
    for (auto& n : nodes) {
        out.push_back(n.minX); out.push_back(n.minY);
        out.push_back(n.maxX); out.push_back(n.maxY);
    }
    wrMarker(out, MARKER_CAFE);

    // 7. Poly data
    out.insert(out.end(), allPolyData.begin(), allPolyData.end());
    wrMarker(out, MARKER_CAFE);

    // 8. Lines
    std::vector<uint8_t> lineFlags((numLinesOut + 1) / 2, 0);
    std::vector<uint8_t> lineXs(numLinesOut * 2), lineYs(numLinesOut * 2);
    for (int i = 0; i < numLinesOut; i++) {
        lineXs[i*2+0] = orderedLines[i].x1;
        lineXs[i*2+1] = orderedLines[i].x2;
        lineYs[i*2+0] = orderedLines[i].y1;
        lineYs[i*2+1] = orderedLines[i].y2;
        uint8_t nib = orderedLines[i].flags & 0xF;
        if (i & 1) lineFlags[i>>1] |= (nib << 4);
        else       lineFlags[i>>1] |= nib;
    }
    out.insert(out.end(), lineFlags.begin(), lineFlags.end());
    out.insert(out.end(), lineXs.begin(), lineXs.end());
    out.insert(out.end(), lineYs.begin(), lineYs.end());
    wrMarker(out, MARKER_CAFE);

    // 9. Height map
    int floorZByte = FLOOR_HEIGHT >> 3;
    for (int i = 0; i < 1024; i++)
        wrU8(out, (bsp.blockMap[i] & 1) == 0 ? floorZByte : 0);
    wrMarker(out, MARKER_CAFE);

    // 10-14. Sprites (1 invisible dummy at spawn)
    uint8_t spawnTX = (bsp.spawnIndex % 32) * 8 + 4;
    uint8_t spawnTY = (bsp.spawnIndex / 32) * 8 + 4;
    wrU8(out, spawnTX);
    wrU8(out, spawnTY);
    wrU8(out, 0);           // infoLow
    wrMarker(out, MARKER_CAFE);
    wrU16(out, 0x0001);     // infoHigh (invisible)
    wrMarker(out, MARKER_CAFE);
    wrMarker(out, MARKER_CAFE); // Z-sprite heights
    wrMarker(out, MARKER_CAFE); // Z-sprite info mid

    // 15. Static functions
    for (int i = 0; i < 12; i++) wrU16(out, 0xFFFF);
    wrMarker(out, MARKER_CAFE);

    // 16-17. Tile events + bytecode (empty)
    wrMarker(out, MARKER_CAFE);
    wrMarker(out, MARKER_CAFE);

    // 18. Maya cameras
    wrMarker(out, MARKER_DEAD);

    // 19. Map flags (nibble-packed)
    for (int i = 0; i < 512; i++) {
        int ta = i * 2, tb = i * 2 + 1;
        uint8_t fa = (ta < 1024) ? (bsp.blockMap[ta] & 0xF) : 0;
        uint8_t fb = (tb < 1024) ? (bsp.blockMap[tb] & 0xF) : 0;
        wrU8(out, (fa & 0xF) | ((fb & 0xF) << 4));
    }
    // Clear solid flag on door tiles
    for (auto& l : bsp.lines) {
        if (l.texture < 0 || l.texture > 10) continue;
        int mx = (l.v1x + l.v2x) / 2, my = (l.v1y + l.v2y) / 2;
        int tx = mx / 64, ty = my / 64, tidx = ty * 32 + tx;
        if (tidx >= 0 && tidx < 1024) {
            int byteIdx = tidx / 2;
            // Fix up the map flags we already wrote
            int flagsOffset = (int)out.size() - 512 - 4 + byteIdx; // -4 for pending marker
            // Actually we haven't written marker yet, recalc
            flagsOffset = (int)out.size() - 512 + byteIdx;
            if (tidx & 1)
                out[flagsOffset] &= 0x0F;
            else
                out[flagsOffset] &= 0xF0;
        }
    }
    wrMarker(out, MARKER_CAFE);

    return {out, (int)polys.size(), numLinesOut, numNodes};
}

// ── Main ────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bsp2bin <input.bsp> [output.bin]\n");
        return 1;
    }

    std::ifstream ifs(argv[1], std::ios::binary);
    if (!ifs) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifs)), {});
    ifs.close();

    D1BSP bsp = parseD1BSP(data.data(), (int)data.size());
    auto result = convertBSPtoBIN(bsp);

    // Output path
    std::string outPath;
    if (argc >= 3) {
        outPath = argv[2];
    } else {
        outPath = argv[1];
        auto dot = outPath.rfind('.');
        if (dot != std::string::npos) outPath = outPath.substr(0, dot);
        outPath += ".bin";
    }

    std::ofstream ofs(outPath, std::ios::binary);
    ofs.write((char*)result.binData.data(), result.binData.size());
    ofs.close();

    printf("  %-30s -> %s\n", argv[1], outPath.c_str());
    printf("    polys=%5d  lines=%4d  nodes=%4d  size=%zu -> %zu\n",
           result.numPolys, result.numLines, result.numNodes,
           data.size(), result.binData.size());

    return 0;
}
