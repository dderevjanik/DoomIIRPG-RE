// Unit test: Player inventory queries (requireStat, requireItem)
// Links against engine lib. Tests pure member-data logic.

#include "Player.h"
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

#define ASSERT_FALSE(cond, msg) ASSERT_TRUE(!(cond), msg)

// --- requireStat ---

static bool test_requireStat_above() {
    Player p;
    CombatEntity ce(100, 50, 30, 40, 60, 70);
    p.ce = &ce;
    // stat 4 (strength) = 40, require >= 30 → true
    ASSERT_TRUE(p.requireStat(4, 30), "strength 40 >= 30 should be true");
    return true;
}

static bool test_requireStat_exact() {
    Player p;
    CombatEntity ce(100, 50, 30, 40, 60, 70);
    p.ce = &ce;
    // stat 4 (strength) = 40, require >= 40 → true
    ASSERT_TRUE(p.requireStat(4, 40), "strength 40 >= 40 should be true");
    return true;
}

static bool test_requireStat_below() {
    Player p;
    CombatEntity ce(100, 50, 30, 40, 60, 70);
    p.ce = &ce;
    // stat 4 (strength) = 40, require >= 50 → false
    ASSERT_FALSE(p.requireStat(4, 50), "strength 40 >= 50 should be false");
    return true;
}

static bool test_requireStat_zero() {
    Player p;
    CombatEntity ce;
    p.ce = &ce;
    // All stats 0, require >= 0 → true
    ASSERT_TRUE(p.requireStat(3, 0), "0 >= 0 should be true");
    return true;
}

// --- requireItem (inventory mode, n=0) ---

static bool test_requireItem_inventory_in_range() {
    Player p;
    p.inventory[5] = 50;
    // Check: inventory[5] >= 40 && inventory[5] <= 60
    ASSERT_TRUE(p.requireItem(0, 5, 40, 60),
        "inventory[5]=50 in range [40,60] should be true");
    return true;
}

static bool test_requireItem_inventory_exact() {
    Player p;
    p.inventory[5] = 50;
    ASSERT_TRUE(p.requireItem(0, 5, 50, 50),
        "inventory[5]=50 in range [50,50] should be true");
    return true;
}

static bool test_requireItem_inventory_below() {
    Player p;
    p.inventory[5] = 30;
    ASSERT_FALSE(p.requireItem(0, 5, 40, 60),
        "inventory[5]=30 below range [40,60] should be false");
    return true;
}

static bool test_requireItem_inventory_above() {
    Player p;
    p.inventory[5] = 70;
    ASSERT_FALSE(p.requireItem(0, 5, 40, 60),
        "inventory[5]=70 above range [40,60] should be false");
    return true;
}

// --- requireItem (weapon mode, n=1) ---

static bool test_requireItem_weapon_has() {
    Player p;
    p.weapons = (1 << 3); // has weapon 3
    // n4 != 0 → check weapon present
    ASSERT_TRUE(p.requireItem(1, 3, 0, 1),
        "should have weapon 3");
    return true;
}

static bool test_requireItem_weapon_missing() {
    Player p;
    p.weapons = 0; // no weapons
    ASSERT_FALSE(p.requireItem(1, 3, 0, 1),
        "should not have weapon 3");
    return true;
}

static bool test_requireItem_weapon_absence_check() {
    Player p;
    p.weapons = 0;
    // n4 == 0 → check weapon NOT present
    ASSERT_TRUE(p.requireItem(1, 3, 0, 0),
        "weapon 3 absence should be true when not owned");
    return true;
}

static bool test_requireItem_weapon_absence_when_present() {
    Player p;
    p.weapons = (1 << 3);
    // n4 == 0 → check weapon NOT present, but it IS present
    ASSERT_FALSE(p.requireItem(1, 3, 0, 0),
        "weapon 3 absence should be false when owned");
    return true;
}

// --- requireItem (invalid type) ---

static bool test_requireItem_invalid_type() {
    Player p;
    // n=2 (not 0 or 1) → should return false
    ASSERT_FALSE(p.requireItem(2, 0, 0, 100),
        "invalid item type should return false");
    return true;
}

int main() {
    struct { const char* name; bool (*fn)(); } tests[] = {
        {"requireStat_above",               test_requireStat_above},
        {"requireStat_exact",               test_requireStat_exact},
        {"requireStat_below",               test_requireStat_below},
        {"requireStat_zero",                test_requireStat_zero},
        {"requireItem_inventory_in_range",  test_requireItem_inventory_in_range},
        {"requireItem_inventory_exact",     test_requireItem_inventory_exact},
        {"requireItem_inventory_below",     test_requireItem_inventory_below},
        {"requireItem_inventory_above",     test_requireItem_inventory_above},
        {"requireItem_weapon_has",          test_requireItem_weapon_has},
        {"requireItem_weapon_missing",      test_requireItem_weapon_missing},
        {"requireItem_weapon_absence",      test_requireItem_weapon_absence_check},
        {"requireItem_weapon_absence_fail", test_requireItem_weapon_absence_when_present},
        {"requireItem_invalid_type",        test_requireItem_invalid_type},
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
