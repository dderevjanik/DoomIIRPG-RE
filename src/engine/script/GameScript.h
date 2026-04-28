#pragma once
#include <vector>
#include <string>
#include <cstdint>

// Drives the engine through a sequence of scripted steps. Step-paced rather
// than tick-paced: each step waits until the engine is ready before advancing,
// so dialogs/animations don't desync the script.
//
// Script format (one directive per line):
//   do <ACTION>            — dispatch ACTION when engine is ready
//   await <STATE>          — block until canvas state matches
//   expect <K> <OP> <V>    — block until condition is true (sticky)
//   timeout <N>            — set default per-step timeout in ticks (applies to following steps)
//   # comment              — ignored by engine
//   # args: <flags>        — consumed by the test runner only
//
// Actions: UP, DOWN, LEFT, RIGHT, MOVELEFT, MOVERIGHT,
//          FIRE, SELECT, USE, ATTACK, AUTOMAP, MENU, PASSTURN,
//          NEXTWEAPON, PREVWEAPON, ITEMS, DRINKS, PDA, BOTDISCARD
//
// States: LOGO, MENU, PLAYING, COMBAT, AUTOMAP, LOADING, DIALOG, INTRO,
//         INTRO_MOVIE, MINI_GAME, DYING, TRAVELMAP, CHARACTER_SELECTION,
//         LOOTING, BOT_DYING, EPILOGUE, CREDITS, SAVING, BENCHMARK,
//         INTER_CAMERA, CAMERA, ERROR, TAF, MIXING, TREADMILL,
//         WIDGET_SCREEN, EDITOR, BENCHMARKDONE
//
// Expect keys: state, health, damage_dealt, combat_turns, kills
// Expect ops:  ==, !=, <, <=, >, >=
//
// A `do` step is "ready" when the canvas state is interactive (not LOADING /
// DYING / cinematic) AND the previous input has been consumed. An `await` /
// `expect` step is "ready" when its condition holds. Each step has a timeout
// (default 600 ticks ≈ 9s). On timeout the script fails and the engine exits
// with code 2 after printing `STUCK_IN: ...` to stderr.
//
// Example:
//   # args: --map levels/10_test_map --skip-intro --seed 1337
//   await PLAYING
//   do FIRE
//   do FIRE
//   expect damage_dealt > 0

enum class StepKind {
	Do,
	Await,
	Expect,
};

enum class ExpectKey {
	State,
	Health,
	DamageDealt,
	CombatTurns,
	Kills,
};

enum class ExpectOp {
	Eq,
	Ne,
	Lt,
	Le,
	Gt,
	Ge,
};

struct ScriptStep {
	StepKind kind;
	int avkAction = 0;
	int stateValue = 0;
	ExpectKey key = ExpectKey::State;
	ExpectOp op = ExpectOp::Eq;
	int value = 0;
	int timeout = 600;
	int minDelay = 0; // applies to Do steps; ensures `do` waits at least N ticks even if engine is ready
	int waited = 0;
	int sourceLine = 0;
};

class GameScript {
public:
	GameScript();

	bool loadFromFile(const char* path);

	// Floor on per-`do` step delay (in ticks). Applied if a step's own
	// `min_delay` is lower. Used to make windowed runs watchable while
	// keeping headless/CI fast (default 0).
	void setMinDelayFloor(int ticks) { minDelayFloor = ticks; }

	// Drive the step machine for one tick. Call once per tick BEFORE DoLoop.
	// Returns true while still running, false when complete (success or failure).
	bool advance(class Applet* app);

	bool isComplete() const { return pc >= (int)steps.size() || hasFailed; }
	int failedCount() const { return failed; }
	int stepCount() const { return (int)steps.size(); }
	int maxTicksEstimate() const;

private:
	std::vector<ScriptStep> steps;
	int pc = 0;
	int failed = 0;
	bool hasFailed = false;
	int minDelayFloor = 0;

	static int parseAction(const std::string& name);
	static bool parseStateName(const std::string& name, int& out);
	static bool parseExpectKey(const std::string& name, ExpectKey& out);
	static bool parseExpectOp(const std::string& name, ExpectOp& out);
	static const char* stateName(int state);
	static bool isInteractiveState(int state);
	static int readKey(class Applet* app, ExpectKey key);
	static bool evalOp(int actual, ExpectOp op, int value);
	static const char* opStr(ExpectOp op);
	static const char* keyStr(ExpectKey key);
};
