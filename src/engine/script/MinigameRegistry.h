#ifndef __MINIGAMEREGISTRY_H__
#define __MINIGAMEREGISTRY_H__

#include <cstring>

class IMinigame;

// Registry mapping numeric IDs and string names to IMinigame instances.
// Numeric IDs correspond to stateVars[0] values (0=sentrybot, 2=hacking, 4=vending).
// The registry does NOT own the IMinigame pointers.
class MinigameRegistry {
public:
	static constexpr int MAX_MINIGAMES = 16;

	MinigameRegistry() {
		std::memset(minigames, 0, sizeof(minigames));
		std::memset(names, 0, sizeof(names));
	}

	// Register a minigame by numeric ID and string name.
	// Returns true on success, false if ID out of range or already registered.
	bool registerMinigame(int id, IMinigame* minigame, const char* name) {
		if (id < 0 || id >= MAX_MINIGAMES) return false;
		if (minigames[id] != nullptr) return false;
		minigames[id] = minigame;
		names[id] = name;
		return true;
	}

	// Look up by numeric ID (stateVars[0]).
	IMinigame* getById(int id) const {
		if (id < 0 || id >= MAX_MINIGAMES) return nullptr;
		return minigames[id];
	}

	// Look up by string name (for CLI --minigame flag).
	IMinigame* getByName(const char* name) const {
		if (!name) return nullptr;
		for (int i = 0; i < MAX_MINIGAMES; i++) {
			if (names[i] && strcmp(names[i], name) == 0)
				return minigames[i];
		}
		return nullptr;
	}

private:
	IMinigame* minigames[MAX_MINIGAMES];
	const char* names[MAX_MINIGAMES];
};

#endif
