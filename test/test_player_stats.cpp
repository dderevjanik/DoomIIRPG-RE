// Unit test: Player XP/level/score calculations
// Requires CAppContainer singleton for GameConfig access.

#include "CAppContainer.h"
#include "App.h"
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

// --- calcLevelXP ---
// Formula: xpLinear * n + xpCubic * ((n-1)^3 + (n-1))
// Default config: xpLinear=500, xpCubic=100

static bool test_calcLevelXP_level1() {
    Player p;
    // Level 1: 500*1 + 100*(0 + 0) = 500
    ASSERT_EQ(p.calcLevelXP(1), 500, "level 1 XP should be 500");
    return true;
}

static bool test_calcLevelXP_level2() {
    Player p;
    // Level 2: 500*2 + 100*(1 + 1) = 1000 + 200 = 1200
    ASSERT_EQ(p.calcLevelXP(2), 1200, "level 2 XP should be 1200");
    return true;
}

static bool test_calcLevelXP_level3() {
    Player p;
    // Level 3: 500*3 + 100*(8 + 2) = 1500 + 1000 = 2500
    ASSERT_EQ(p.calcLevelXP(3), 2500, "level 3 XP should be 2500");
    return true;
}

static bool test_calcLevelXP_level5() {
    Player p;
    // Level 5: 500*5 + 100*(64 + 4) = 2500 + 6800 = 9300
    ASSERT_EQ(p.calcLevelXP(5), 9300, "level 5 XP should be 9300");
    return true;
}

static bool test_calcLevelXP_level10() {
    Player p;
    // Level 10: 500*10 + 100*(729 + 9) = 5000 + 73800 = 78800
    ASSERT_EQ(p.calcLevelXP(10), 78800, "level 10 XP should be 78800");
    return true;
}

// --- calcScore ---
// Tests use default GameConfig values

static bool test_calcScore_no_deaths_all_kills() {
    Player p;
    // Set all 9 kill bits (levels 0-8)
    p.killedMonstersLevels = 0x1FF; // bits 0-8 set
    p.totalDeaths = 0;
    p.totalTime = 0;
    p.playTime = 0;
    p.totalMoves = 0;
    p.foundSecretsLevels = 0;

    // Need app->gameTime — mock with a minimal Applet
    Applet app;
    app.gameTime = 0;
    p.app = &app;

    int score = p.calcScore();
    // 9 levels * 1000 + allLevelsBonus(1000) + noDeathsBonus(1000)
    // + timeBonusMinutes(120)*timeBonusMult(15) [since n2=0 < 120]
    // + (movesThreshold(5000) - 0) / movesDivisor(2)
    // + allSecretsBonus(1000) [secrets loop is a no-op, b2 always true]
    // = 9000 + 1000 + 1000 + 1800 + 2500 + 1000 = 16300
    ASSERT_EQ(score, 16300, "perfect score with no deaths, all kills, fast time, few moves");
    return true;
}

static bool test_calcScore_some_deaths() {
    Player p;
    p.killedMonstersLevels = 0;
    p.totalDeaths = 3;
    p.totalTime = 120 * 60000; // 120 minutes (no time bonus)
    p.playTime = 0;
    p.totalMoves = 10000; // above threshold (no move bonus)
    p.foundSecretsLevels = 0;

    Applet app;
    app.gameTime = 0;
    p.app = &app;

    int score = p.calcScore();
    // 0 kills + 0 allKills + deaths penalty: (5-3)*50 = 100
    // + 0 time bonus + 0 move bonus + allSecretsBonus(1000)
    ASSERT_EQ(score, 1100, "score with 3 deaths, no kills");
    return true;
}

static bool test_calcScore_many_deaths() {
    Player p;
    p.killedMonstersLevels = 0;
    p.totalDeaths = 15; // above threshold (10)
    p.totalTime = 200 * 60000;
    p.playTime = 0;
    p.totalMoves = 10000;
    p.foundSecretsLevels = 0;

    Applet app;
    app.gameTime = 0;
    p.app = &app;

    int score = p.calcScore();
    // many deaths: -250 penalty + allSecretsBonus(1000)
    ASSERT_EQ(score, 750, "many deaths offset by secrets bonus");
    return true;
}

int main() {
    struct { const char* name; bool (*fn)(); } tests[] = {
        {"calcLevelXP_level1",          test_calcLevelXP_level1},
        {"calcLevelXP_level2",          test_calcLevelXP_level2},
        {"calcLevelXP_level3",          test_calcLevelXP_level3},
        {"calcLevelXP_level5",          test_calcLevelXP_level5},
        {"calcLevelXP_level10",         test_calcLevelXP_level10},
        {"calcScore_no_deaths_all_kills", test_calcScore_no_deaths_all_kills},
        {"calcScore_some_deaths",       test_calcScore_some_deaths},
        {"calcScore_many_deaths",       test_calcScore_many_deaths},
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
