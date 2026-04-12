#pragma once
#include <vector>
#include <string>

class IGameModule;

// Factory function type for creating game modules.
typedef IGameModule* (*GameModuleFactory)();

// Registry for game modules. Each module registers itself with a unique string
// ID (e.g. "doom2rpg") and a factory function. Main.cpp reads the "module"
// field from game.yaml and creates the corresponding IGameModule via this
// registry.
//
// Game modules self-register using the REGISTER_GAME_MODULE macro at static
// init time, so Main.cpp never needs to #include game-specific headers.
class GameModuleRegistry {
public:
	// Register a game module factory by ID. Called at static init time.
	static void registerModule(const char* id, GameModuleFactory factory);

	// Create a game module by ID. Returns nullptr if not found.
	// Caller owns the returned pointer.
	static IGameModule* create(const char* id);

	// List all registered module IDs (for --help / game discovery).
	static std::vector<std::string> listModules();
};

// Self-registration macro. Place in a game module's .cpp file:
//   REGISTER_GAME_MODULE("doom2rpg", DoomIIRPGGame);
#define REGISTER_GAME_MODULE(id, Class) \
	static struct _GameModuleReg_##Class { \
		_GameModuleReg_##Class() { \
			GameModuleRegistry::registerModule(id, []() -> IGameModule* { return new Class(); }); \
		} \
	} _gameModuleReg_##Class;
