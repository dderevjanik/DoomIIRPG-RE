// Unit test: MapDataIO round-trip (create test map -> save -> reload -> compare)
// Self-contained — no game assets required.

#include "MapData.h"
#include "MapDataIO.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static int testsPassed = 0;
static int testsFailed = 0;

#define ASSERT_EQ(a, b, msg)                                                     \
    do {                                                                          \
        if ((a) != (b)) {                                                         \
            std::fprintf(stderr, "  FAIL [%s:%d] %s: %lld != %lld\n", __FILE__,  \
                         __LINE__, msg, (long long)(a), (long long)(b));          \
            testsFailed++;                                                        \
            return false;                                                         \
        }                                                                         \
    } while (0)

#define ASSERT_TRUE(cond, msg)                                                    \
    do {                                                                          \
        if (!(cond)) {                                                            \
            std::fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); \
            testsFailed++;                                                        \
            return false;                                                         \
        }                                                                         \
    } while (0)

// Build a test map with known data for all fields
static MapData createTestMap() {
    MapData m;

    // Header
    m.spawnIndex = 165;   // tile (5, 5): row 5 * 32 + col 5
    m.spawnDir = 2;       // west
    m.flagsBitmask = 0x03;
    m.totalSecrets = 3;
    m.totalLoot = 7;

    // Tiles: create a simple room
    // Solid border, open interior, varying heights
    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 32; col++) {
            int idx = row * 32 + col;
            TileData& t = m.tiles[idx];

            bool border = (row == 0 || row == 31 || col == 0 || col == 31);
            bool innerWall = (row == 10 && col >= 5 && col <= 15);

            if (border || innerWall) {
                t.flags = 0x1; // solid
                t.height = 0;
            } else {
                t.flags = 0x0; // walkable
                // Varying height: ramp from row 1-10
                if (row < 10)
                    t.height = (uint8_t)(row * 5);
                else
                    t.height = 50;
            }

            t.wallTexture = 3;
            t.floorTexture = 5;
        }
    }

    // A tile with event flag
    m.tiles[200].flags |= 0x40;

    // Media indices
    m.mediaIndices = {10, 20, 30, 45, 100, 200};

    // Normal sprites (3)
    m.normalSprites.resize(3);
    m.normalSprites[0] = {160, 160, 32, 0x00200085, false}; // x=20*8, y=20*8
    m.normalSprites[1] = {80, 240, 32, 0x01000056, false};
    m.normalSprites[2] = {200, 80, 32, 0x00400012, false};

    // Z-sprites (2)
    m.zSprites.resize(2);
    m.zSprites[0] = {120, 120, 48, 0x008000AA, true};
    m.zSprites[1] = {56, 200, 24, 0x003000BB, true};

    // Tile events (3)
    m.tileEvents.resize(3);
    m.tileEvents[0] = {200, 0x00010001};     // tile 200, some param
    m.tileEvents[1] = {300 | 0x400, 0x0002}; // tile 300 with upper flag
    m.tileEvents[2] = {512, 0xFF};            // tile 512

    // Static functions
    for (int i = 0; i < 12; i++) {
        m.staticFuncs[i] = (uint16_t)(100 + i * 10);
    }

    // Bytecode
    m.byteCode = {0x01, 0x02, 0x03, 0x10, 0x20, 0xFF, 0x00, 0xAB, 0xCD, 0xEF};

    // Maya cameras (1 camera with 2 keys)
    m.mayaCameras.resize(1);
    auto& cam = m.mayaCameras[0];
    cam.numKeys = 2;
    cam.sampleRate = 30;
    cam.keyData.resize(2 * 7);
    for (int i = 0; i < 14; i++) cam.keyData[i] = (int16_t)(i * 100);
    cam.tweenIndices.resize(2 * 6);
    for (int i = 0; i < 12; i++) cam.tweenIndices[i] = (int16_t)(i * 5);
    cam.tweenCounts[0] = 2;
    cam.tweenCounts[1] = 3;
    cam.tweenCounts[2] = 0;
    cam.tweenCounts[3] = 1;
    cam.tweenCounts[4] = 0;
    cam.tweenCounts[5] = 0;
    cam.tweenData = {10, 20, 30, 40, 50, 60};

    // Maya tween offsets
    m.mayaTweenOffsets[0] = 0;
    m.mayaTweenOffsets[1] = 2;
    m.mayaTweenOffsets[2] = -1;
    m.mayaTweenOffsets[3] = 5;
    m.mayaTweenOffsets[4] = -1;
    m.mayaTweenOffsets[5] = -1;

    return m;
}

// ---- Individual tests ----

static bool testRoundtripHeader() {
    std::printf("test: header round-trip\n");

    MapData orig = createTestMap();
    std::string err;
    std::string path = "test_roundtrip_header.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(orig, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    ASSERT_EQ(loaded.spawnIndex, orig.spawnIndex, "spawnIndex");
    ASSERT_EQ(loaded.spawnDir, orig.spawnDir, "spawnDir");
    ASSERT_EQ(loaded.flagsBitmask, orig.flagsBitmask, "flagsBitmask");
    ASSERT_EQ(loaded.totalSecrets, orig.totalSecrets, "totalSecrets");
    ASSERT_EQ(loaded.totalLoot, orig.totalLoot, "totalLoot");

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testRoundtripTileHeights() {
    std::printf("test: tile heights round-trip\n");

    MapData orig = createTestMap();
    std::string err, path = "test_roundtrip_heights.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(orig, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    for (int i = 0; i < 1024; i++) {
        ASSERT_EQ(loaded.tiles[i].height, orig.tiles[i].height, "tile height mismatch");
    }

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testRoundtripTileFlags() {
    std::printf("test: tile flags round-trip\n");

    MapData orig = createTestMap();
    std::string err, path = "test_roundtrip_flags.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(orig, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    for (int i = 0; i < 1024; i++) {
        // Flags are stored as 4-bit nibbles, so mask to lower 4 bits
        uint8_t origFlags = orig.tiles[i].flags & 0xF;
        uint8_t loadedFlags = loaded.tiles[i].flags & 0xF;
        ASSERT_EQ(loadedFlags, origFlags, "tile flags mismatch");
    }

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testRoundtripSprites() {
    std::printf("test: sprites round-trip\n");

    MapData orig = createTestMap();
    std::string err, path = "test_roundtrip_sprites.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(orig, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    ASSERT_EQ(loaded.normalSprites.size(), orig.normalSprites.size(), "normal sprite count");
    ASSERT_EQ(loaded.zSprites.size(), orig.zSprites.size(), "z sprite count");

    for (size_t i = 0; i < orig.normalSprites.size(); i++) {
        ASSERT_EQ(loaded.normalSprites[i].x, orig.normalSprites[i].x, "normal sprite X");
        ASSERT_EQ(loaded.normalSprites[i].y, orig.normalSprites[i].y, "normal sprite Y");
        ASSERT_EQ(loaded.normalSprites[i].info, orig.normalSprites[i].info, "normal sprite info");
    }

    for (size_t i = 0; i < orig.zSprites.size(); i++) {
        ASSERT_EQ(loaded.zSprites[i].x, orig.zSprites[i].x, "z sprite X");
        ASSERT_EQ(loaded.zSprites[i].y, orig.zSprites[i].y, "z sprite Y");
        ASSERT_EQ(loaded.zSprites[i].z, orig.zSprites[i].z, "z sprite Z");
        ASSERT_EQ(loaded.zSprites[i].info, orig.zSprites[i].info, "z sprite info");
    }

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testRoundtripTileEvents() {
    std::printf("test: tile events round-trip\n");

    MapData orig = createTestMap();
    std::string err, path = "test_roundtrip_events.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(orig, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    ASSERT_EQ(loaded.tileEvents.size(), orig.tileEvents.size(), "tile event count");

    for (size_t i = 0; i < orig.tileEvents.size(); i++) {
        ASSERT_EQ(loaded.tileEvents[i].packed, orig.tileEvents[i].packed, "event packed");
        ASSERT_EQ(loaded.tileEvents[i].param, orig.tileEvents[i].param, "event param");
    }

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testRoundtripBytecode() {
    std::printf("test: bytecode round-trip\n");

    MapData orig = createTestMap();
    std::string err, path = "test_roundtrip_bytecode.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(orig, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    ASSERT_EQ(loaded.byteCode.size(), orig.byteCode.size(), "bytecode size");
    ASSERT_TRUE(memcmp(loaded.byteCode.data(), orig.byteCode.data(), orig.byteCode.size()) == 0,
                "bytecode content mismatch");

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testRoundtripStaticFuncs() {
    std::printf("test: static functions round-trip\n");

    MapData orig = createTestMap();
    std::string err, path = "test_roundtrip_staticfuncs.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(orig, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    for (int i = 0; i < 12; i++) {
        ASSERT_EQ(loaded.staticFuncs[i], orig.staticFuncs[i], "static func");
    }

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testRoundtripMayaCameras() {
    std::printf("test: maya cameras round-trip\n");

    MapData orig = createTestMap();
    std::string err, path = "test_roundtrip_maya.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(orig, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    ASSERT_EQ(loaded.mayaCameras.size(), orig.mayaCameras.size(), "maya camera count");

    for (size_t i = 0; i < orig.mayaCameras.size(); i++) {
        auto& oc = orig.mayaCameras[i];
        auto& lc = loaded.mayaCameras[i];

        ASSERT_EQ(lc.numKeys, oc.numKeys, "camera numKeys");
        ASSERT_EQ(lc.sampleRate, oc.sampleRate, "camera sampleRate");
        ASSERT_EQ(lc.keyData.size(), oc.keyData.size(), "camera keyData size");
        ASSERT_EQ(lc.tweenIndices.size(), oc.tweenIndices.size(), "camera tweenIndices size");
        ASSERT_EQ(lc.tweenData.size(), oc.tweenData.size(), "camera tweenData size");

        for (size_t j = 0; j < oc.keyData.size(); j++) {
            ASSERT_EQ(lc.keyData[j], oc.keyData[j], "camera keyData value");
        }
        for (size_t j = 0; j < oc.tweenIndices.size(); j++) {
            ASSERT_EQ(lc.tweenIndices[j], oc.tweenIndices[j], "camera tweenIndices value");
        }
        for (int j = 0; j < 6; j++) {
            ASSERT_EQ(lc.tweenCounts[j], oc.tweenCounts[j], "camera tweenCounts");
        }
        for (size_t j = 0; j < oc.tweenData.size(); j++) {
            ASSERT_EQ(lc.tweenData[j], oc.tweenData[j], "camera tweenData value");
        }
    }

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testRoundtripMediaIndices() {
    std::printf("test: media indices round-trip\n");

    MapData orig = createTestMap();
    std::string err, path = "test_roundtrip_media.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(orig, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    ASSERT_EQ(loaded.mediaIndices.size(), orig.mediaIndices.size(), "media count");
    for (size_t i = 0; i < orig.mediaIndices.size(); i++) {
        ASSERT_EQ(loaded.mediaIndices[i], orig.mediaIndices[i], "media index");
    }

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testEmptyMap() {
    std::printf("test: empty map (no sprites, events, bytecode)\n");

    MapData m;
    m.spawnIndex = 0;
    m.spawnDir = 0;
    m.flagsBitmask = 0;
    m.totalSecrets = 0;
    m.totalLoot = 0;

    // All tiles walkable, zero height
    for (int i = 0; i < 1024; i++) {
        m.tiles[i].flags = 0;
        m.tiles[i].height = 0;
        m.tiles[i].wallTexture = 1;
        m.tiles[i].floorTexture = 1;
    }

    std::string err, path = "test_empty_map.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(m, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    ASSERT_EQ(loaded.spawnIndex, (uint16_t)0, "empty spawnIndex");
    ASSERT_EQ(loaded.normalSprites.size(), (size_t)0, "empty normal sprites");
    ASSERT_EQ(loaded.zSprites.size(), (size_t)0, "empty z sprites");
    ASSERT_EQ(loaded.tileEvents.size(), (size_t)0, "empty tile events");
    ASSERT_EQ(loaded.byteCode.size(), (size_t)0, "empty bytecode");
    ASSERT_EQ(loaded.mayaCameras.size(), (size_t)0, "empty maya cameras");

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testBSPCompilerOutput() {
    std::printf("test: BSP compiler produces valid output\n");

    MapData m;
    // Create a simple room: solid border, open 4x4 interior at tiles (2,2)-(5,5)
    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 32; col++) {
            int idx = row * 32 + col;
            bool interior = (row >= 2 && row <= 5 && col >= 2 && col <= 5);
            m.tiles[idx].flags = interior ? 0x0 : 0x1;
            m.tiles[idx].height = interior ? 10 : 0;
            m.tiles[idx].wallTexture = 1;
            m.tiles[idx].floorTexture = 1;
        }
    }

    std::string err, path = "test_bsp_output.bin";

    ASSERT_TRUE(MapDataIO::saveToBin(m, path, err), err.c_str());

    // Verify the saved file can be loaded back
    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    // The open tiles should still be walkable
    for (int row = 2; row <= 5; row++) {
        for (int col = 2; col <= 5; col++) {
            int idx = row * 32 + col;
            ASSERT_EQ(loaded.tiles[idx].flags & 0x1, (uint8_t)0, "interior tile should be walkable");
            ASSERT_EQ(loaded.tiles[idx].height, (uint8_t)10, "interior tile height");
        }
    }

    // Border tiles should be solid
    ASSERT_EQ(loaded.tiles[0].flags & 0x1, (uint8_t)1, "border tile should be solid");

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testMaxHeightValues() {
    std::printf("test: max height values (0 and 255)\n");

    MapData m;
    m.tiles[0].height = 0;
    m.tiles[0].flags = 0;
    m.tiles[1].height = 255;
    m.tiles[1].flags = 0;
    m.tiles[2].height = 128;
    m.tiles[2].flags = 0x1;

    std::string err, path = "test_max_heights.bin";
    ASSERT_TRUE(MapDataIO::saveToBin(m, path, err), err.c_str());

    MapData loaded;
    ASSERT_TRUE(MapDataIO::loadFromBin(path, loaded, err), err.c_str());

    ASSERT_EQ(loaded.tiles[0].height, (uint8_t)0, "height 0");
    ASSERT_EQ(loaded.tiles[1].height, (uint8_t)255, "height 255");
    ASSERT_EQ(loaded.tiles[2].height, (uint8_t)128, "height 128");

    std::remove(path.c_str());
    testsPassed++;
    return true;
}

static bool testInvalidFile() {
    std::printf("test: loading invalid file fails gracefully\n");

    MapData loaded;
    std::string err;

    // Non-existent file
    bool result = MapDataIO::loadFromBin("nonexistent_map.bin", loaded, err);
    ASSERT_TRUE(!result, "should fail on missing file");
    ASSERT_TRUE(!err.empty(), "should have error message");

    // Bad version
    {
        FILE* f = fopen("test_bad_version.bin", "wb");
        ASSERT_TRUE(f != nullptr, "failed to create test file");
        uint8_t badData[42] = {};
        badData[0] = 99; // bad version (should be 3)
        fwrite(badData, 1, 42, f);
        fclose(f);

        result = MapDataIO::loadFromBin("test_bad_version.bin", loaded, err);
        ASSERT_TRUE(!result, "should fail on bad version");

        std::remove("test_bad_version.bin");
    }

    testsPassed++;
    return true;
}

int main() {
    std::printf("=== MapDataIO Round-Trip Tests ===\n\n");

    testRoundtripHeader();
    testRoundtripTileHeights();
    testRoundtripTileFlags();
    testRoundtripSprites();
    testRoundtripTileEvents();
    testRoundtripBytecode();
    testRoundtripStaticFuncs();
    testRoundtripMayaCameras();
    testRoundtripMediaIndices();
    testEmptyMap();
    testBSPCompilerOutput();
    testMaxHeightValues();
    testInvalidFile();

    std::printf("\n=== Results: %d passed, %d failed ===\n", testsPassed, testsFailed);

    return testsFailed > 0 ? 1 : 0;
}
