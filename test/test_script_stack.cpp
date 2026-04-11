// Unit test: ScriptThread stack operations and bytecode argument extraction
// Links against engine lib. Mocks app->render->mapByteCode with a test array.

#include <cstdint>
#include "ScriptThread.h"
#include "CAppContainer.h"
#include "App.h"
#include "Render.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

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

// Test bytecode buffer for arg extraction tests
static uint8_t testByteCode[32];

// Minimal setup: ScriptThread needs app->render->mapByteCode
static Applet testApplet;
static Render testRender;

static void setupBytecodeEnv() {
    testRender.mapByteCode = testByteCode;
    testApplet.render = std::unique_ptr<Render>(&testRender);
}

static void teardownBytecodeEnv() {
    // Release without deleting (testRender is stack-allocated)
    testApplet.render.release();
}

// --- push / pop ---

static bool test_push_pop_basic() {
    ScriptThread st;
    st.stackPtr = 0;
    st.push(42);
    ASSERT_EQ(st.stackPtr, 1, "stackPtr should be 1 after push");
    int val = st.pop();
    ASSERT_EQ(val, 42, "pop should return pushed value");
    ASSERT_EQ(st.stackPtr, 0, "stackPtr should be 0 after pop");
    return true;
}

static bool test_push_pop_lifo() {
    ScriptThread st;
    st.stackPtr = 0;
    st.push(10);
    st.push(20);
    st.push(30);
    ASSERT_EQ(st.pop(), 30, "first pop should return 30 (LIFO)");
    ASSERT_EQ(st.pop(), 20, "second pop should return 20");
    ASSERT_EQ(st.pop(), 10, "third pop should return 10");
    return true;
}

static bool test_push_bool() {
    ScriptThread st;
    st.stackPtr = 0;
    st.push(true);
    st.push(false);
    ASSERT_EQ(st.pop(), 0, "false should push 0");
    ASSERT_EQ(st.pop(), 1, "true should push 1");
    return true;
}

static bool test_push_negative() {
    ScriptThread st;
    st.stackPtr = 0;
    st.push(-999);
    ASSERT_EQ(st.pop(), -999, "negative values should round-trip");
    return true;
}

static bool test_push_fill_stack() {
    ScriptThread st;
    st.stackPtr = 0;
    for (int i = 0; i < 16; i++) {
        st.push(i * 100);
    }
    ASSERT_EQ(st.stackPtr, 16, "stack should hold 16 elements");
    for (int i = 15; i >= 0; i--) {
        ASSERT_EQ(st.pop(), i * 100, "stack values should match in reverse");
    }
    return true;
}

// --- getUByteArg ---

static bool test_getUByteArg() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    testByteCode[1] = 0xAB;
    short result = st.getUByteArg();
    ASSERT_EQ(result, 0xAB, "getUByteArg should read unsigned byte");
    ASSERT_EQ(st.IP, 1, "IP should advance by 1");

    teardownBytecodeEnv();
    return true;
}

static bool test_getUByteArg_unsigned() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    testByteCode[1] = 0xFF;
    short result = st.getUByteArg();
    ASSERT_EQ(result, 255, "0xFF should be 255 unsigned, not -1");

    teardownBytecodeEnv();
    return true;
}

// --- getByteArg ---

static bool test_getByteArg() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    testByteCode[1] = 0x42;
    uint8_t result = st.getByteArg();
    ASSERT_EQ(result, 0x42, "getByteArg should read raw byte");
    ASSERT_EQ(st.IP, 1, "IP should advance by 1");

    teardownBytecodeEnv();
    return true;
}

// --- getUShortArg ---

static bool test_getUShortArg() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    // Big-endian: 0xABCD → byte[1]=0xAB, byte[2]=0xCD
    testByteCode[1] = 0xAB;
    testByteCode[2] = 0xCD;
    int result = st.getUShortArg();
    ASSERT_EQ(result, 0xABCD, "getUShortArg big-endian 16-bit");
    ASSERT_EQ(st.IP, 2, "IP should advance by 2");

    teardownBytecodeEnv();
    return true;
}

static bool test_getUShortArg_max() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    testByteCode[1] = 0xFF;
    testByteCode[2] = 0xFF;
    int result = st.getUShortArg();
    ASSERT_EQ(result, 65535, "0xFFFF unsigned should be 65535");

    teardownBytecodeEnv();
    return true;
}

// --- getShortArg ---

static bool test_getShortArg_positive() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    testByteCode[1] = 0x00;
    testByteCode[2] = 0x64; // 100
    short result = st.getShortArg();
    ASSERT_EQ(result, 100, "getShortArg positive value");
    ASSERT_EQ(st.IP, 2, "IP should advance by 2");

    teardownBytecodeEnv();
    return true;
}

static bool test_getShortArg_negative() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    // -1 in big-endian signed short: 0xFF 0xFF
    testByteCode[1] = 0xFF;
    testByteCode[2] = 0xFF;
    short result = st.getShortArg();
    ASSERT_EQ(result, -1, "0xFFFF signed should be -1");

    teardownBytecodeEnv();
    return true;
}

static bool test_getShortArg_min() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    // -32768: 0x80 0x00
    testByteCode[1] = 0x80;
    testByteCode[2] = 0x00;
    short result = st.getShortArg();
    ASSERT_EQ(result, -32768, "0x8000 signed should be -32768");

    teardownBytecodeEnv();
    return true;
}

// --- getIntArg ---

static bool test_getIntArg() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    // 0x12345678 big-endian
    testByteCode[1] = 0x12;
    testByteCode[2] = 0x34;
    testByteCode[3] = 0x56;
    testByteCode[4] = 0x78;
    int result = st.getIntArg();
    ASSERT_EQ(result, 0x12345678, "getIntArg big-endian 32-bit");
    ASSERT_EQ(st.IP, 4, "IP should advance by 4");

    teardownBytecodeEnv();
    return true;
}

static bool test_getIntArg_negative() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    // -1: 0xFF 0xFF 0xFF 0xFF
    testByteCode[1] = 0xFF;
    testByteCode[2] = 0xFF;
    testByteCode[3] = 0xFF;
    testByteCode[4] = 0xFF;
    int result = st.getIntArg();
    ASSERT_EQ(result, -1, "0xFFFFFFFF signed should be -1");

    teardownBytecodeEnv();
    return true;
}

static bool test_getIntArg_zero() {
    setupBytecodeEnv();
    ScriptThread st;
    st.app = &testApplet;
    st.IP = 0;

    testByteCode[1] = 0x00;
    testByteCode[2] = 0x00;
    testByteCode[3] = 0x00;
    testByteCode[4] = 0x00;
    int result = st.getIntArg();
    ASSERT_EQ(result, 0, "zero should read as 0");

    teardownBytecodeEnv();
    return true;
}

int main() {
    struct { const char* name; bool (*fn)(); } tests[] = {
        {"push_pop_basic",       test_push_pop_basic},
        {"push_pop_lifo",        test_push_pop_lifo},
        {"push_bool",            test_push_bool},
        {"push_negative",        test_push_negative},
        {"push_fill_stack",      test_push_fill_stack},
        {"getUByteArg",          test_getUByteArg},
        {"getUByteArg_unsigned", test_getUByteArg_unsigned},
        {"getByteArg",           test_getByteArg},
        {"getUShortArg",         test_getUShortArg},
        {"getUShortArg_max",     test_getUShortArg_max},
        {"getShortArg_positive", test_getShortArg_positive},
        {"getShortArg_negative", test_getShortArg_negative},
        {"getShortArg_min",      test_getShortArg_min},
        {"getIntArg",            test_getIntArg},
        {"getIntArg_negative",   test_getIntArg_negative},
        {"getIntArg_zero",       test_getIntArg_zero},
    };

    int total = sizeof(tests) / sizeof(tests[0]);
    for (int i = 0; i < total; i++) {
        if (tests[i].fn()) {
            std::printf("  PASS %s\n", tests[i].name);
            testsPassed++;
        }
    }

    std::printf("\n%d/%d tests passed, %d failed\n", testsPassed, total, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
