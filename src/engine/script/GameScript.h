#ifndef __GAMESCRIPT_H__
#define __GAMESCRIPT_H__

#include <vector>
#include <string>
#include <cstdint>

// Reads a script file and injects game actions at specified ticks.
//
// Script format (one command per line):
//   tick <N>: <ACTION>
//   # comment
//
// Actions: UP, DOWN, LEFT, RIGHT, MOVELEFT, MOVERIGHT,
//          FIRE, SELECT, AUTOMAP, MENU, PASSTURN,
//          NEXTWEAPON, PREVWEAPON, ITEMS, DRINKS, PDA, BOTDISCARD
//
// Example:
//   tick 50: FIRE
//   tick 80: UP
//   tick 95: UP
//   tick 120: SELECT

struct ScriptCommand {
	int tick;
	int avkAction;
};

class GameScript {
public:
	GameScript();

	bool loadFromFile(const char* path);
	void injectForTick(int currentTick);
	bool isFinished(int currentTick) const;
	int lastTick() const;
	int commandCount() const { return (int)commands.size(); }

private:
	std::vector<ScriptCommand> commands;
	size_t nextIndex; // index of next command to process

	static int parseAction(const std::string& name);
};

#endif
