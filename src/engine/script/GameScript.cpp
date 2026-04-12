#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ranges>
#include <sstream>
#include "Log.h"

#include "GameScript.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Input.h"

GameScript::GameScript() : nextIndex(0) {}

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

bool GameScript::loadFromFile(const char* path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		LOG_ERROR("[script] Error: cannot open '%s'\n", path);
		return false;
	}

	commands.clear();
	nextIndex = 0;

	std::string line;
	int lineNum = 0;
	while (std::getline(file, line)) {
		lineNum++;

		// Strip leading whitespace
		size_t start = line.find_first_not_of(" \t");
		if (start == std::string::npos) continue;
		line = line.substr(start);

		// Skip comments and empty lines
		if (line.empty() || line[0] == '#') continue;

		// Parse "tick <N>: <ACTION>"
		int tick = 0;
		char actionBuf[64] = {};
		if (sscanf(line.c_str(), "tick %d: %63s", &tick, actionBuf) == 2) {
			int avk = parseAction(actionBuf);
			if (avk == AVK_UNDEFINED) {
				LOG_WARN("[script] Warning: unknown action '%s' at line %d\n", actionBuf, lineNum);
				continue;
			}
			commands.push_back({tick, avk});
		} else {
			LOG_WARN("[script] Warning: cannot parse line %d: %s\n", lineNum, line.c_str());
		}
	}

	// Sort by tick (in case script is unordered)
	std::ranges::sort(commands, [](const ScriptCommand& a, const ScriptCommand& b) { return a.tick < b.tick; });

	LOG_INFO("[script] Loaded %d commands from '%s'\n", (int)commands.size(), path);
	return true;
}

void GameScript::injectForTick(int currentTick) {
	Canvas* canvas = CAppContainer::getInstance()->app->canvas.get();
	while (nextIndex < commands.size() && commands[nextIndex].tick <= currentTick) {
		int avk = commands[nextIndex].avkAction;
		if (canvas->numEvents < Canvas::MAX_EVENTS) {
			canvas->events[canvas->numEvents++] = avk;
			canvas->keyPressedTime = CAppContainer::getInstance()->app->upTimeMs;
		}
		nextIndex++;
	}
}

bool GameScript::isFinished(int currentTick) const {
	if (commands.empty()) return true;
	return nextIndex >= commands.size();
}

int GameScript::lastTick() const {
	if (commands.empty()) return 0;
	return commands.back().tick;
}
