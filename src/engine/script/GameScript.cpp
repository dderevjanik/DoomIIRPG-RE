#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include "Log.h"

#include "GameScript.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Input.h"
#include "Player.h"
#include "Game.h"
#include "Combat.h"
#include "DialogManager.h"

GameScript::GameScript() = default;

int GameScript::parseAction(const std::string& name) {
	if (name == "UP")          return AVK_UP;
	if (name == "DOWN")        return AVK_DOWN;
	if (name == "LEFT")        return AVK_LEFT;
	if (name == "RIGHT")       return AVK_RIGHT;
	if (name == "MOVELEFT")    return AVK_MOVELEFT;
	if (name == "MOVERIGHT")   return AVK_MOVERIGHT;
	if (name == "FIRE" || name == "SELECT" || name == "USE" || name == "ATTACK")
		return AVK_SELECT;
	if (name == "AUTOMAP")     return AVK_AUTOMAP;
	if (name == "MENU")        return AVK_MENUOPEN;
	if (name == "PASSTURN")    return AVK_PASSTURN;
	if (name == "NEXTWEAPON")  return AVK_NEXTWEAPON;
	if (name == "PREVWEAPON")  return AVK_PREVWEAPON;
	if (name == "ITEMS")       return AVK_ITEMS_INFO;
	if (name == "DRINKS")      return AVK_DRINKS;
	if (name == "PDA")         return AVK_PDA;
	if (name == "BOTDISCARD")  return AVK_BOTDISCARD;
	return AVK_UNDEFINED;
}

bool GameScript::parseStateName(const std::string& name, int& out) {
	if (name == "MENU")                { out = Canvas::ST_MENU; return true; }
	if (name == "INTRO_MOVIE")         { out = Canvas::ST_INTRO_MOVIE; return true; }
	if (name == "PLAYING")             { out = Canvas::ST_PLAYING; return true; }
	if (name == "INTER_CAMERA")        { out = Canvas::ST_INTER_CAMERA; return true; }
	if (name == "COMBAT")              { out = Canvas::ST_COMBAT; return true; }
	if (name == "AUTOMAP")             { out = Canvas::ST_AUTOMAP; return true; }
	if (name == "LOADING")             { out = Canvas::ST_LOADING; return true; }
	if (name == "DIALOG")              { out = Canvas::ST_DIALOG; return true; }
	if (name == "INTRO")               { out = Canvas::ST_INTRO; return true; }
	if (name == "BENCHMARK")           { out = Canvas::ST_BENCHMARK; return true; }
	if (name == "BENCHMARKDONE")       { out = Canvas::ST_BENCHMARKDONE; return true; }
	if (name == "MINI_GAME")           { out = Canvas::ST_MINI_GAME; return true; }
	if (name == "DYING")               { out = Canvas::ST_DYING; return true; }
	if (name == "EPILOGUE")            { out = Canvas::ST_EPILOGUE; return true; }
	if (name == "CREDITS")             { out = Canvas::ST_CREDITS; return true; }
	if (name == "SAVING")              { out = Canvas::ST_SAVING; return true; }
	if (name == "ERROR")               { out = Canvas::ST_ERROR; return true; }
	if (name == "CAMERA")              { out = Canvas::ST_CAMERA; return true; }
	if (name == "TAF")                 { out = Canvas::ST_TAF; return true; }
	if (name == "MIXING")              { out = Canvas::ST_MIXING; return true; }
	if (name == "TRAVELMAP")           { out = Canvas::ST_TRAVELMAP; return true; }
	if (name == "CHARACTER_SELECTION") { out = Canvas::ST_CHARACTER_SELECTION; return true; }
	if (name == "LOOTING")             { out = Canvas::ST_LOOTING; return true; }
	if (name == "TREADMILL")           { out = Canvas::ST_TREADMILL; return true; }
	if (name == "BOT_DYING")           { out = Canvas::ST_BOT_DYING; return true; }
	if (name == "LOGO")                { out = Canvas::ST_LOGO; return true; }
	if (name == "WIDGET_SCREEN")       { out = Canvas::ST_WIDGET_SCREEN; return true; }
	if (name == "EDITOR")              { out = Canvas::ST_EDITOR; return true; }
	return false;
}

const char* GameScript::stateName(int state) {
	switch (state) {
		case Canvas::ST_MENU: return "MENU";
		case Canvas::ST_INTRO_MOVIE: return "INTRO_MOVIE";
		case Canvas::ST_PLAYING: return "PLAYING";
		case Canvas::ST_INTER_CAMERA: return "INTER_CAMERA";
		case Canvas::ST_COMBAT: return "COMBAT";
		case Canvas::ST_AUTOMAP: return "AUTOMAP";
		case Canvas::ST_LOADING: return "LOADING";
		case Canvas::ST_DIALOG: return "DIALOG";
		case Canvas::ST_INTRO: return "INTRO";
		case Canvas::ST_BENCHMARK: return "BENCHMARK";
		case Canvas::ST_BENCHMARKDONE: return "BENCHMARKDONE";
		case Canvas::ST_MINI_GAME: return "MINI_GAME";
		case Canvas::ST_DYING: return "DYING";
		case Canvas::ST_EPILOGUE: return "EPILOGUE";
		case Canvas::ST_CREDITS: return "CREDITS";
		case Canvas::ST_SAVING: return "SAVING";
		case Canvas::ST_ERROR: return "ERROR";
		case Canvas::ST_CAMERA: return "CAMERA";
		case Canvas::ST_TAF: return "TAF";
		case Canvas::ST_MIXING: return "MIXING";
		case Canvas::ST_TRAVELMAP: return "TRAVELMAP";
		case Canvas::ST_CHARACTER_SELECTION: return "CHARACTER_SELECTION";
		case Canvas::ST_LOOTING: return "LOOTING";
		case Canvas::ST_TREADMILL: return "TREADMILL";
		case Canvas::ST_BOT_DYING: return "BOT_DYING";
		case Canvas::ST_LOGO: return "LOGO";
		case Canvas::ST_WIDGET_SCREEN: return "WIDGET_SCREEN";
		case Canvas::ST_EDITOR: return "EDITOR";
		default: return "?";
	}
}

bool GameScript::isInteractiveState(int state) {
	// States where the player has agency / engine is ready for fresh input.
	// `do` steps wait until canvas state is one of these — explicitly
	// excludes COMBAT (the engine is busy resolving an attack/turn there;
	// inputs during COMBAT get consumed without effect) and any animation
	// or transition state.
	switch (state) {
		case Canvas::ST_PLAYING:
		case Canvas::ST_DIALOG:
		case Canvas::ST_LOOTING:
		case Canvas::ST_MINI_GAME:
		case Canvas::ST_AUTOMAP:
		case Canvas::ST_MENU:
		case Canvas::ST_CHARACTER_SELECTION:
		case Canvas::ST_TRAVELMAP:
		case Canvas::ST_INTRO:
		case Canvas::ST_INTRO_MOVIE:
		case Canvas::ST_LOGO:
		case Canvas::ST_EPILOGUE:
		case Canvas::ST_CREDITS:
		case Canvas::ST_MIXING:
		case Canvas::ST_TAF:
		case Canvas::ST_TREADMILL:
		case Canvas::ST_BENCHMARK:
		case Canvas::ST_BENCHMARKDONE:
		case Canvas::ST_WIDGET_SCREEN:
		case Canvas::ST_EDITOR:
			return true;
		default:
			return false;
	}
}

bool GameScript::parseExpectKey(const std::string& name, ExpectKey& out) {
	if (name == "state")         { out = ExpectKey::State; return true; }
	if (name == "health")        { out = ExpectKey::Health; return true; }
	if (name == "damage_dealt")  { out = ExpectKey::DamageDealt; return true; }
	if (name == "combat_turns")  { out = ExpectKey::CombatTurns; return true; }
	if (name == "kills")         { out = ExpectKey::Kills; return true; }
	return false;
}

bool GameScript::parseExpectOp(const std::string& name, ExpectOp& out) {
	if (name == "==") { out = ExpectOp::Eq; return true; }
	if (name == "!=") { out = ExpectOp::Ne; return true; }
	if (name == "<")  { out = ExpectOp::Lt; return true; }
	if (name == "<=") { out = ExpectOp::Le; return true; }
	if (name == ">")  { out = ExpectOp::Gt; return true; }
	if (name == ">=") { out = ExpectOp::Ge; return true; }
	return false;
}

const char* GameScript::keyStr(ExpectKey key) {
	switch (key) {
		case ExpectKey::State:       return "state";
		case ExpectKey::Health:      return "health";
		case ExpectKey::DamageDealt: return "damage_dealt";
		case ExpectKey::CombatTurns: return "combat_turns";
		case ExpectKey::Kills:       return "kills";
	}
	return "?";
}

const char* GameScript::opStr(ExpectOp op) {
	switch (op) {
		case ExpectOp::Eq: return "==";
		case ExpectOp::Ne: return "!=";
		case ExpectOp::Lt: return "<";
		case ExpectOp::Le: return "<=";
		case ExpectOp::Gt: return ">";
		case ExpectOp::Ge: return ">=";
	}
	return "?";
}

int GameScript::readKey(Applet* app, ExpectKey key) {
	switch (key) {
		case ExpectKey::State:       return app->canvas->state;
		case ExpectKey::Health:      return app->player->getHealth();
		case ExpectKey::DamageDealt: return app->player->counters[2];
		case ExpectKey::CombatTurns: return app->player->counters[6];
		case ExpectKey::Kills:       return app->player->counters[7];
	}
	return 0;
}

bool GameScript::evalOp(int actual, ExpectOp op, int value) {
	switch (op) {
		case ExpectOp::Eq: return actual == value;
		case ExpectOp::Ne: return actual != value;
		case ExpectOp::Lt: return actual <  value;
		case ExpectOp::Le: return actual <= value;
		case ExpectOp::Gt: return actual >  value;
		case ExpectOp::Ge: return actual >= value;
	}
	return false;
}

bool GameScript::loadFromFile(const char* path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		LOG_ERROR("[script] Error: cannot open '{}'\n", path);
		return false;
	}

	steps.clear();
	pc = 0;
	failed = 0;
	hasFailed = false;

	int defaultTimeout = 600;
	int defaultMinDelay = 0;
	std::string line;
	int lineNum = 0;
	while (std::getline(file, line)) {
		lineNum++;

		// Strip leading whitespace
		size_t start = line.find_first_not_of(" \t");
		if (start == std::string::npos) continue;
		line = line.substr(start);

		// Strip trailing whitespace and inline `# ...` comments
		size_t hash = line.find('#');
		if (hash != std::string::npos && hash > 0) line = line.substr(0, hash);
		while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) line.pop_back();

		// Full-line comments / `# args:` consumed by runner only
		if (line.empty() || line[0] == '#') continue;

		char buf1[64] = {};
		char buf2[8] = {};
		char buf3[64] = {};
		int n = 0;

		// "timeout <N>"
		if (sscanf(line.c_str(), "timeout %d", &n) == 1) {
			if (n > 0) defaultTimeout = n;
			else LOG_WARN("[script] line {}: timeout must be > 0\n", lineNum);
			continue;
		}

		// "min_delay <N>" — minimum ticks before next `do` step fires
		if (sscanf(line.c_str(), "min_delay %d", &n) == 1) {
			if (n >= 0) defaultMinDelay = n;
			else LOG_WARN("[script] line {}: min_delay must be >= 0\n", lineNum);
			continue;
		}

		// "do <ACTION>"
		if (sscanf(line.c_str(), "do %63s", buf1) == 1) {
			int avk = parseAction(buf1);
			if (avk == AVK_UNDEFINED) {
				LOG_WARN("[script] line {}: unknown action '{}'\n", lineNum, buf1);
				continue;
			}
			ScriptStep s;
			s.kind = StepKind::Do;
			s.avkAction = avk;
			s.timeout = defaultTimeout;
			s.minDelay = defaultMinDelay;
			s.sourceLine = lineNum;
			steps.push_back(s);
			continue;
		}

		// "await <STATE>"
		if (sscanf(line.c_str(), "await %63s", buf1) == 1) {
			int stateVal = 0;
			if (!parseStateName(buf1, stateVal)) {
				LOG_WARN("[script] line {}: unknown state '{}'\n", lineNum, buf1);
				continue;
			}
			ScriptStep s;
			s.kind = StepKind::Await;
			s.stateValue = stateVal;
			s.timeout = defaultTimeout;
			s.sourceLine = lineNum;
			steps.push_back(s);
			continue;
		}

		// "expect <KEY> <OP> <VALUE>"
		if (sscanf(line.c_str(), "expect %63s %7s %63s", buf1, buf2, buf3) == 3) {
			ExpectKey key;
			ExpectOp op;
			if (!parseExpectKey(buf1, key)) {
				LOG_WARN("[script] line {}: unknown expect key '{}'\n", lineNum, buf1);
				continue;
			}
			if (!parseExpectOp(buf2, op)) {
				LOG_WARN("[script] line {}: unknown op '{}'\n", lineNum, buf2);
				continue;
			}
			int value = 0;
			if (key == ExpectKey::State) {
				if (!parseStateName(buf3, value)) {
					LOG_WARN("[script] line {}: unknown state '{}'\n", lineNum, buf3);
					continue;
				}
			} else {
				if (sscanf(buf3, "%d", &value) != 1) {
					LOG_WARN("[script] line {}: expected integer, got '{}'\n", lineNum, buf3);
					continue;
				}
			}
			ScriptStep s;
			s.kind = StepKind::Expect;
			s.key = key;
			s.op = op;
			s.value = value;
			s.timeout = defaultTimeout;
			s.sourceLine = lineNum;
			steps.push_back(s);
			continue;
		}

		LOG_WARN("[script] line {}: cannot parse: {}\n", lineNum, line.c_str());
	}

	LOG_INFO("[script] Loaded {} steps from '{}'\n", (int)steps.size(), path);
	return true;
}

int GameScript::maxTicksEstimate() const {
	int total = 100; // slack
	for (const auto& s : steps) total += s.timeout;
	return total;
}

bool GameScript::advance(Applet* app) {
	if (isComplete()) return false;
	if (pc >= (int)steps.size()) return false;

	ScriptStep& s = steps[pc];
	Canvas* canvas = app->canvas.get();

	switch (s.kind) {
		case StepKind::Do: {
			// Engine "ready for input" condition — mirrors the gating used in
			// PlayingState.cpp:65 and Combat.cpp's combat-stage advance:
			//   - state is interactive (not LOADING/DYING/cinematic)
			//   - input queue drained
			//   - no animations / propagators / knockback in flight
			//   - not the monster's turn
			//   - no help/auto dialog displayed
			//   - no projectile in flight or combat-stage cooldown active
			int requiredDelay = std::max(s.minDelay, minDelayFloor);
			bool ready = s.waited >= requiredDelay
				&& isInteractiveState(canvas->state)
				&& canvas->numEvents == 0
				&& canvas->knockbackDist == 0;

			// Gameplay-flow gates only apply in PLAYING. In modal UI states
			// (DIALOG, LOOTING, MENU, MINI_GAME, ...) the engine accepts input
			// directly — combat/turn flags from the prior round may linger
			// uncleared and shouldn't block dismissal of post-combat dialogs.
			if (ready && canvas->state == Canvas::ST_PLAYING) {
				ready = app->game && app->game->animatingEffects == 0
					&& app->game->activePropogators == 0
					&& app->game->monstersTurn == 0
					&& app->dialogManager && app->dialogManager->numHelpMessages == 0
					&& app->combat && app->combat->numActiveMissiles == 0
					&& app->combat->nextStageTime == 0;
			}

			if (ready) {
				if (canvas->numEvents < Canvas::MAX_EVENTS) {
					canvas->events[canvas->numEvents++] = s.avkAction;
					canvas->keyPressedTime = app->upTimeMs;
				}
				LOG_INFO("[script] step {} (line {}): do action={} state={} after {} ticks\n",
					pc, s.sourceLine, s.avkAction, stateName(canvas->state), s.waited);
				pc++;
				return true;
			}
			break;
		}
		case StepKind::Await: {
			if (canvas->state == s.stateValue) {
				LOG_INFO("[script] step {} (line {}): await {} reached after {} ticks\n",
					pc, s.sourceLine, stateName(s.stateValue), s.waited);
				pc++;
				return true;
			}
			break;
		}
		case StepKind::Expect: {
			int actual = readKey(app, s.key);
			if (evalOp(actual, s.op, s.value)) {
				LOG_INFO("[script] step {} (line {}): expect ok {}{}{} actual={} after {} ticks\n",
					pc, s.sourceLine, keyStr(s.key), opStr(s.op),
					(s.key == ExpectKey::State ? stateName(s.value) : std::to_string(s.value).c_str()),
					actual, s.waited);
				pc++;
				return true;
			}
			break;
		}
	}

	s.waited++;
	if (s.waited > s.timeout) {
		hasFailed = true;
		failed++;
		const char* kindName = (s.kind == StepKind::Do)     ? "do"
		                     : (s.kind == StepKind::Await)  ? "await"
		                                                    : "expect";
		// Compose failure description
		char detail[160] = {};
		switch (s.kind) {
			case StepKind::Do:
				snprintf(detail, sizeof(detail),
					"do action=%d state=%s numEvents=%d animating=%d monstersTurn=%d missiles=%d nextStage=%d helpMsgs=%d knockback=%d",
					s.avkAction, stateName(canvas->state), canvas->numEvents,
					app->game ? app->game->animatingEffects : -1,
					app->game ? app->game->monstersTurn : -1,
					app->combat ? app->combat->numActiveMissiles : -1,
					app->combat ? app->combat->nextStageTime : -1,
					app->dialogManager ? app->dialogManager->numHelpMessages : -1,
					canvas->knockbackDist);
				break;
			case StepKind::Await:
				snprintf(detail, sizeof(detail), "await %s actual=%s",
					stateName(s.stateValue), stateName(canvas->state));
				break;
			case StepKind::Expect: {
				int actual = readKey(app, s.key);
				if (s.key == ExpectKey::State) {
					snprintf(detail, sizeof(detail), "expect %s%s%s actual=%s",
						keyStr(s.key), opStr(s.op), stateName(s.value), stateName(actual));
				} else {
					snprintf(detail, sizeof(detail), "expect %s%s%d actual=%d",
						keyStr(s.key), opStr(s.op), s.value, actual);
				}
				break;
			}
		}
		fprintf(stderr, "STUCK_IN: step=%d line=%d kind=%s timeout=%d %s\n",
			pc, s.sourceLine, kindName, s.timeout, detail);
		LOG_ERROR("[script] STUCK_IN step={} line={} kind={} timeout={} {}\n",
			pc, s.sourceLine, kindName, s.timeout, detail);
		return false;
	}

	return true;
}
