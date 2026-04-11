// Unit test: CombatEntity stat functions
// Self-contained — no engine initialization required.

#include "CombatEntity.h"

#include <cstdio>
#include <cstdlib>

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

// --- getStat / setStat ---

static bool test_setStat_basic() {
    CombatEntity ce;
    ce.setStat(1, 100); // max health
    ce.setStat(0, 80);  // current health
    ASSERT_EQ(ce.getStat(0), 80, "health should be 80");
    ASSERT_EQ(ce.getStat(1), 100, "max health should be 100");
    return true;
}

static bool test_setStat_clamps_negative() {
    CombatEntity ce;
    ce.setStat(3, -10); // defense
    ASSERT_EQ(ce.getStat(3), 0, "negative stat should clamp to 0");
    return true;
}

static bool test_setStat_clamps_to_255() {
    CombatEntity ce;
    ce.setStat(4, 300); // strength, max 255
    ASSERT_EQ(ce.getStat(4), 255, "stat should clamp to 255");
    return true;
}

static bool test_setStat_health_clamps_to_max() {
    CombatEntity ce;
    ce.setStat(1, 50);   // max health = 50
    ce.setStat(0, 100);  // try to set health to 100
    ASSERT_EQ(ce.getStat(0), 50, "health should clamp to max health");
    return true;
}

static bool test_setStat_max_health_unclamped() {
    // max health (stat 1) has no upper clamp
    CombatEntity ce;
    ce.setStat(1, 999);
    ASSERT_EQ(ce.getStat(1), 999, "max health should allow values > 255");
    return true;
}

// --- addStat ---

static bool test_addStat_increases() {
    CombatEntity ce;
    ce.setStat(1, 200); // max health
    ce.setStat(0, 50);  // health = 50
    ce.addStat(0, 30);
    ASSERT_EQ(ce.getStat(0), 80, "health should be 50+30=80");
    return true;
}

static bool test_addStat_negative() {
    CombatEntity ce;
    ce.setStat(4, 100); // strength = 100
    ce.addStat(4, -30);
    ASSERT_EQ(ce.getStat(4), 70, "strength should be 100-30=70");
    return true;
}

static bool test_addStat_clamps_at_zero() {
    CombatEntity ce;
    ce.setStat(4, 10);
    ce.addStat(4, -50);
    ASSERT_EQ(ce.getStat(4), 0, "stat should clamp at 0 when subtracting past zero");
    return true;
}

// --- calcXP ---

static bool test_calcXP_basic() {
    // Formula: ((defense + strength) * 5 + accuracy * 6 + maxHealth * 5 + 49) / 50
    CombatEntity ce;
    ce.setStat(1, 100); // max health
    ce.setStat(3, 10);  // defense
    ce.setStat(4, 10);  // strength
    ce.setStat(5, 10);  // accuracy
    // ((10 + 10) * 5 + 10 * 6 + 100 * 5 + 49) / 50
    // = (100 + 60 + 500 + 49) / 50 = 709 / 50 = 14
    ASSERT_EQ(ce.calcXP(), 14, "calcXP formula check");
    return true;
}

static bool test_calcXP_zero_stats() {
    CombatEntity ce;
    // All stats zero: (0 + 0 + 0 + 49) / 50 = 0 (integer division)
    ASSERT_EQ(ce.calcXP(), 0, "calcXP with zero stats should be 0");
    return true;
}

static bool test_calcXP_high_stats() {
    CombatEntity ce;
    ce.setStat(1, 500); // max health
    ce.setStat(3, 200); // defense
    ce.setStat(4, 200); // strength
    ce.setStat(5, 200); // accuracy
    // ((200+200)*5 + 200*6 + 500*5 + 49) / 50
    // = (2000 + 1200 + 2500 + 49) / 50 = 5749 / 50 = 114
    ASSERT_EQ(ce.calcXP(), 114, "calcXP with high stats");
    return true;
}

// --- getStatPercent ---

static bool test_getStatPercent() {
    CombatEntity ce;
    ce.setStat(4, 50); // strength = 50
    // (50 << 8) / 100 = 12800 / 100 = 128
    ASSERT_EQ(ce.getStatPercent(4), 128, "getStatPercent(50) should be 128");
    return true;
}

static bool test_getStatPercent_zero() {
    CombatEntity ce;
    ASSERT_EQ(ce.getStatPercent(4), 0, "getStatPercent(0) should be 0");
    return true;
}

// --- getIQPercent ---

static bool test_getIQPercent_below_100() {
    CombatEntity ce;
    ce.stats[7] = 50; // IQ = 50
    // max(0, min((50-100)*100/100, 100)) = max(0, -50) = 0
    ASSERT_EQ(ce.getIQPercent(), 0, "IQ below 100 should give 0%");
    return true;
}

static bool test_getIQPercent_at_100() {
    CombatEntity ce;
    ce.stats[7] = 100;
    ASSERT_EQ(ce.getIQPercent(), 0, "IQ at 100 should give 0%");
    return true;
}

static bool test_getIQPercent_at_150() {
    CombatEntity ce;
    ce.stats[7] = 150;
    // max(0, min((150-100)*100/100, 100)) = max(0, min(50, 100)) = 50
    ASSERT_EQ(ce.getIQPercent(), 50, "IQ at 150 should give 50%");
    return true;
}

static bool test_getIQPercent_at_200() {
    CombatEntity ce;
    ce.stats[7] = 200;
    // max(0, min((200-100)*100/100, 100)) = max(0, min(100, 100)) = 100
    ASSERT_EQ(ce.getIQPercent(), 100, "IQ at 200 should give 100%");
    return true;
}

static bool test_getIQPercent_above_200() {
    CombatEntity ce;
    ce.stats[7] = 300;
    // max(0, min(200, 100)) = 100 (clamped)
    ASSERT_EQ(ce.getIQPercent(), 100, "IQ above 200 should clamp to 100%");
    return true;
}

// --- Constructor with stats ---

static bool test_parameterized_constructor() {
    CombatEntity ce(100, 50, 30, 40, 60, 70);
    ASSERT_EQ(ce.getStat(0), 100, "health should equal maxHealth on construction");
    ASSERT_EQ(ce.getStat(1), 100, "max health");
    ASSERT_EQ(ce.getStat(2), 50, "armor");
    ASSERT_EQ(ce.getStat(3), 30, "defense");
    ASSERT_EQ(ce.getStat(4), 40, "strength");
    ASSERT_EQ(ce.getStat(5), 60, "accuracy");
    ASSERT_EQ(ce.getStat(6), 70, "agility");
    return true;
}

// --- clone ---

static bool test_clone() {
    CombatEntity src(100, 50, 30, 40, 60, 70);
    CombatEntity dst;
    src.weapon = 5;
    src.clone(&dst);
    ASSERT_EQ(dst.getStat(0), 100, "cloned health");
    ASSERT_EQ(dst.getStat(1), 100, "cloned max health");
    ASSERT_EQ(dst.getStat(4), 40, "cloned strength");
    ASSERT_EQ(dst.weapon, 5, "cloned weapon");
    return true;
}

int main() {
    struct { const char* name; bool (*fn)(); } tests[] = {
        {"setStat_basic",              test_setStat_basic},
        {"setStat_clamps_negative",    test_setStat_clamps_negative},
        {"setStat_clamps_to_255",      test_setStat_clamps_to_255},
        {"setStat_health_clamps",      test_setStat_health_clamps_to_max},
        {"setStat_maxHealth_unclamped", test_setStat_max_health_unclamped},
        {"addStat_increases",          test_addStat_increases},
        {"addStat_negative",           test_addStat_negative},
        {"addStat_clamps_at_zero",     test_addStat_clamps_at_zero},
        {"calcXP_basic",               test_calcXP_basic},
        {"calcXP_zero_stats",          test_calcXP_zero_stats},
        {"calcXP_high_stats",          test_calcXP_high_stats},
        {"getStatPercent",             test_getStatPercent},
        {"getStatPercent_zero",        test_getStatPercent_zero},
        {"getIQPercent_below_100",     test_getIQPercent_below_100},
        {"getIQPercent_at_100",        test_getIQPercent_at_100},
        {"getIQPercent_at_150",        test_getIQPercent_at_150},
        {"getIQPercent_at_200",        test_getIQPercent_at_200},
        {"getIQPercent_above_200",     test_getIQPercent_above_200},
        {"parameterized_constructor",  test_parameterized_constructor},
        {"clone",                      test_clone},
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
