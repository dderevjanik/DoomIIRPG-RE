// bsp2bin — Convert D1 RPG .bsp to D2 RPG .bin using the real BSP compiler
// Build: g++ -std=c++17 -I src/editor tools/bsp2bin.cpp src/editor/BSPCompiler.cpp -o tools/bsp2bin

#include "MapData.h"
#include "BSPCompiler.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

static constexpr int MAP_SIZE = 32;
static constexpr int D2_WALL_TILE = 258;
static constexpr int D2_FLOOR_TILE = 451;
static constexpr int D2_CEIL_TILE = 462;
static constexpr uint8_t FLOOR_HEIGHT = 56;

// D1 door textures (0-10)
static bool isDoorTexture(int tex) { return tex >= 0 && tex <= 10; }

// Read helpers
static uint8_t  readU8(const uint8_t* d, int& p) { return d[p++]; }
static int16_t  readI16(const uint8_t* d, int& p) { int16_t v; memcpy(&v, d+p, 2); p+=2; return v; }
static uint16_t readU16(const uint8_t* d, int& p) { uint16_t v; memcpy(&v, d+p, 2); p+=2; return v; }
static int32_t  readI32(const uint8_t* d, int& p) { int32_t v; memcpy(&v, d+p, 4); p+=4; return v; }

// Write helpers
static void writeU8(std::vector<uint8_t>& out, uint8_t v) { out.push_back(v); }
static void writeI16(std::vector<uint8_t>& out, int16_t v) { out.insert(out.end(), (uint8_t*)&v, (uint8_t*)&v+2); }
static void writeU16(std::vector<uint8_t>& out, uint16_t v) { out.insert(out.end(), (uint8_t*)&v, (uint8_t*)&v+2); }
static void writeI32(std::vector<uint8_t>& out, int32_t v) { out.insert(out.end(), (uint8_t*)&v, (uint8_t*)&v+4); }
static void writeU32(std::vector<uint8_t>& out, uint32_t v) { out.insert(out.end(), (uint8_t*)&v, (uint8_t*)&v+4); }
static void writeMarker(std::vector<uint8_t>& out, uint32_t m) { writeU32(out, m); }

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: bsp2bin <input.bsp> <output.bin>\n");
        return 1;
    }

    // Read D1 BSP file
    std::ifstream ifs(argv[1], std::ios::binary);
    if (!ifs) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifs)), {});
    ifs.close();

    const uint8_t* d = data.data();
    int pos = 0;

    // Parse D1 header
    char mapName[17] = {};
    memcpy(mapName, d, 16); pos = 16;
    pos += 6; // floor/ceiling color
    uint8_t floorTex = readU8(d, pos);
    uint8_t ceilTex = readU8(d, pos);
    pos += 3; // intro color
    pos += 1; // loadMapID
    int16_t spawnIndex = readI16(d, pos);
    uint8_t spawnDir = readU8(d, pos);
    pos += 2; // cameraSpawnIndex

    // Skip D1 nodes
    int16_t numNodes = readI16(d, pos);
    pos += numNodes * 10;

    // Read D1 lines (to find door textures)
    int16_t numLines = readI16(d, pos);
    struct D1Line { int v1x,v1y,v2x,v2y; int16_t texture; int32_t flags; };
    std::vector<D1Line> lines(numLines);
    for (int i = 0; i < numLines; i++) {
        lines[i].v1x = readU8(d, pos) << 3;
        lines[i].v1y = readU8(d, pos) << 3;
        lines[i].v2x = readU8(d, pos) << 3;
        lines[i].v2y = readU8(d, pos) << 3;
        lines[i].texture = readI16(d, pos);
        lines[i].flags = readI32(d, pos);
    }

    // Skip sprites, events, bytecodes, strings
    int16_t ns = readI16(d, pos); pos += ns * 5;
    int16_t ne = readI16(d, pos); pos += ne * 4;
    int16_t nb = readI16(d, pos); pos += nb * 9;
    int16_t nstr = readI16(d, pos);
    for (int i = 0; i < nstr; i++) { int16_t sl = readI16(d, pos); pos += sl; }

    // Read D1 blockmap
    uint8_t blockMap[1024] = {};
    int bi = 0;
    for (int j = 0; j < 256; j++) {
        uint8_t f = readU8(d, pos);
        blockMap[bi+0] = f & 3;
        blockMap[bi+1] = (f >> 2) & 3;
        blockMap[bi+2] = (f >> 4) & 3;
        blockMap[bi+3] = (f >> 6) & 3;
        bi += 4;
    }

    // Build MapData for BSP compiler
    MapData map;
    map.spawnIndex = (uint16_t)spawnIndex;
    map.spawnDir = (spawnDir / 32) & 7;

    for (int i = 0; i < 1024; i++) {
        map.tiles[i].height = FLOOR_HEIGHT;
        map.tiles[i].flags = (blockMap[i] & 1) ? 0x01 : 0x00;
        map.tiles[i].wallTexture = D2_WALL_TILE;
        map.tiles[i].floorTexture = D2_FLOOR_TILE;
    }

    // Clear solid flag on door tiles
    for (auto& line : lines) {
        if (!isDoorTexture(line.texture)) continue;
        int mx = (line.v1x + line.v2x) / 2;
        int my = (line.v1y + line.v2y) / 2;
        int tx = mx / 64, ty = my / 64;
        int tidx = ty * 32 + tx;
        if (tidx >= 0 && tidx < 1024)
            map.tiles[tidx].flags &= ~0x01;
    }

    map.mediaIndices = {D2_WALL_TILE, D2_FLOOR_TILE, D2_CEIL_TILE, 276/*door_unlocked*/, 301/*sky_box*/};

    // 1 dummy sprite at spawn
    SpriteData dummySprite;
    dummySprite.x = (spawnIndex % 32) * 8 + 4;
    dummySprite.y = (spawnIndex / 32) * 8 + 4;
    dummySprite.info = 0x10000; // invisible
    map.normalSprites.push_back(dummySprite);

    for (int i = 0; i < 12; i++) map.staticFuncs[i] = 0xFFFF;

    // Compile BSP using the (now fixed) BSPCompiler
    BSPCompileResult bsp;
    std::string errorMsg;
    if (!BSPCompiler::compile(map, bsp, errorMsg)) {
        fprintf(stderr, "BSP compile failed: %s\n", errorMsg.c_str());
        return 1;
    }

    int numBSPNodes = (int)bsp.nodes.size();
    int numNormals = (int)bsp.normals.size() / 3;
    int numBSPLines = (int)bsp.lines.size();
    int dataSizePolys = (int)bsp.polyData.size();

    printf("  %s -> %s\n", argv[1], argv[2]);
    printf("    nodes=%d normals=%d lines=%d polys=%d bytes\n",
           numBSPNodes, numNormals, numBSPLines, dataSizePolys);

    // Write .bin file
    std::vector<uint8_t> out;

    // Header (42 bytes)
    writeU8(out, 3);
    writeI32(out, 0x44314250);  // compileDate "D1BP"
    writeU16(out, map.spawnIndex);
    writeU8(out, map.spawnDir);
    writeU8(out, 0);  // flagsBitmask
    writeU8(out, 0);  // secrets
    writeU8(out, 0);  // loot
    writeU16(out, (uint16_t)numBSPNodes);
    writeU16(out, (uint16_t)dataSizePolys);
    writeU16(out, (uint16_t)numBSPLines);
    writeU16(out, (uint16_t)numNormals);
    writeU16(out, 1);   // numNormalSprites (1 dummy)
    writeI16(out, 0);   // numZSprites
    writeI16(out, 0);   // numTileEvents
    writeI16(out, 0);   // byteCodeSize
    writeU8(out, 0);    // mayaCameras
    writeI16(out, 0);   // mayaCameraKeys
    for (int i = 0; i < 6; i++) writeI16(out, -1);

    // 1. Media
    writeMarker(out, 0xDEADBEEF);
    writeU16(out, (uint16_t)map.mediaIndices.size());
    for (auto idx : map.mediaIndices) writeU16(out, idx);
    writeMarker(out, 0xDEADBEEF);

    // 2. Normals
    for (auto v : bsp.normals) writeI16(out, v);
    writeMarker(out, 0xCAFEBABE);

    // 3-7. BSP node arrays
    for (auto& n : bsp.nodes) writeI16(out, n.offset);
    writeMarker(out, 0xCAFEBABE);
    for (auto& n : bsp.nodes) writeU8(out, n.normalIdx);
    writeMarker(out, 0xCAFEBABE);
    for (auto& n : bsp.nodes) writeI16(out, n.child1);
    for (auto& n : bsp.nodes) writeI16(out, n.child2);
    writeMarker(out, 0xCAFEBABE);
    for (auto& n : bsp.nodes) {
        out.push_back(n.bounds[0]); out.push_back(n.bounds[1]);
        out.push_back(n.bounds[2]); out.push_back(n.bounds[3]);
    }
    writeMarker(out, 0xCAFEBABE);
    out.insert(out.end(), bsp.polyData.begin(), bsp.polyData.end());
    writeMarker(out, 0xCAFEBABE);

    // 8. Lines
    std::vector<uint8_t> lineFlags((numBSPLines + 1) / 2, 0);
    std::vector<uint8_t> lineXs(numBSPLines * 2), lineYs(numBSPLines * 2);
    for (int i = 0; i < numBSPLines; i++) {
        lineXs[i*2+0] = bsp.lines[i].x1;
        lineXs[i*2+1] = bsp.lines[i].x2;
        lineYs[i*2+0] = bsp.lines[i].y1;
        lineYs[i*2+1] = bsp.lines[i].y2;
        uint8_t nib = bsp.lines[i].flags & 0xF;
        if (i & 1) lineFlags[i>>1] |= (nib << 4);
        else       lineFlags[i>>1] |= nib;
    }
    out.insert(out.end(), lineFlags.begin(), lineFlags.end());
    out.insert(out.end(), lineXs.begin(), lineXs.end());
    out.insert(out.end(), lineYs.begin(), lineYs.end());
    writeMarker(out, 0xCAFEBABE);

    // 9. Height map
    for (int i = 0; i < 1024; i++) writeU8(out, FLOOR_HEIGHT);
    writeMarker(out, 0xCAFEBABE);

    // 10-14. Sprites
    writeU8(out, dummySprite.x & 0xFF);  // X
    writeU8(out, dummySprite.y & 0xFF);  // Y
    writeU8(out, 0);                      // infoLow
    writeMarker(out, 0xCAFEBABE);
    writeU16(out, 0x0001);                // infoHigh (invisible)
    writeMarker(out, 0xCAFEBABE);
    writeMarker(out, 0xCAFEBABE);         // zSprZ
    writeMarker(out, 0xCAFEBABE);         // zSprMid

    // 15. Static functions
    for (int i = 0; i < 12; i++) writeU16(out, 0xFFFF);
    writeMarker(out, 0xCAFEBABE);

    // 16-17. Events + bytecode
    writeMarker(out, 0xCAFEBABE);
    writeMarker(out, 0xCAFEBABE);

    // 18. Maya cameras
    writeMarker(out, 0xDEADBEEF);

    // 19. Map flags
    for (int i = 0; i < 512; i++) {
        int ta = i*2, tb = i*2+1;
        uint8_t fa = map.tiles[ta].flags & 0xF;
        uint8_t fb = (tb < 1024) ? (map.tiles[tb].flags & 0xF) : 0;
        writeU8(out, fa | (fb << 4));
    }
    writeMarker(out, 0xCAFEBABE);

    // Write output
    std::ofstream ofs(argv[2], std::ios::binary);
    ofs.write((char*)out.data(), out.size());
    ofs.close();

    printf("    output: %zu bytes\n", out.size());
    return 0;
}
