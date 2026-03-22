#pragma once

#include <cstdint>
#include <vector>

struct TileData {
    uint8_t height = 0;       // 0-255, world height = height << 3
    uint8_t flags = 0;        // bit 0 = solid, bit 6 = has event
    int wallTexture = 1;      // tile number for walls adjacent to this tile
    int floorTexture = 1;     // tile number for floor
};

struct SpriteData {
    int16_t x = 0;            // compressed coord (value * 8 = world)
    int16_t y = 0;
    uint8_t z = 32;           // default Z
    uint32_t info = 0;        // combined sprite info bitfield
    bool isZSprite = false;
};

struct TileEventData {
    int32_t packed = 0;       // first int: lower 10 bits = tile index, upper bits = flags
    int32_t param = 0;        // second int: event parameters
};

struct MayaCameraRaw {
    uint8_t numKeys = 0;
    int16_t sampleRate = 0;
    std::vector<int16_t> keyData;       // numKeys * 7 shorts
    std::vector<int16_t> tweenIndices;  // numKeys * 6 shorts
    int16_t tweenCounts[6] = {};        // 6 shorts
    std::vector<int8_t> tweenData;      // sum of tweenCounts bytes
};

struct MapData {
    // Header
    uint16_t spawnIndex = 0;
    uint8_t spawnDir = 0;
    uint8_t flagsBitmask = 0;
    uint8_t totalSecrets = 0;
    uint8_t totalLoot = 0;

    // Tile grid (32x32)
    TileData tiles[1024] = {};

    // Sprites (preserved from original)
    std::vector<SpriteData> normalSprites;
    std::vector<SpriteData> zSprites;

    // Media references used by this map
    std::vector<uint16_t> mediaIndices;

    // Tile events (preserved from original)
    std::vector<TileEventData> tileEvents;

    // Static functions (preserved)
    uint16_t staticFuncs[12] = {};

    // Bytecode (preserved as opaque blob)
    std::vector<uint8_t> byteCode;

    // Maya cameras (preserved per-camera)
    std::vector<MayaCameraRaw> mayaCameras;
    int16_t mayaTweenOffsets[6] = {-1, -1, -1, -1, -1, -1};

    // Dirty flag for editor
    bool dirty = false;

    // Helpers
    int tileCount() const { return 1024; }
    int mapSize() const { return 32; }
    TileData& tileAt(int col, int row) { return tiles[row * 32 + col]; }
    const TileData& tileAt(int col, int row) const { return tiles[row * 32 + col]; }
};
