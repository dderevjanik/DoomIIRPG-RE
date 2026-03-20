#ifndef __IGAMEMODULE_H__
#define __IGAMEMODULE_H__

class Applet;
class Game;
class Player;
class Combat;
class EntityDefManager;
class HackingGame;
class SentryBotGame;
class VendingMachine;
class ComicBook;

// Interface for game modules. The engine calls these methods at well-defined
// lifecycle points. A custom game implements this interface to provide its own
// game objects (Game, Player, Combat, etc.) and hook into engine events.
//
// Default implementation: DoomIIRPGGame (src/game/DoomIIRPGGame.h)
//
// Usage:
//   IGameModule* mod = new DoomIIRPGGame();
//   app->setGameModule(mod);
//   // App::startup() will call mod->createGameObjects() and mod->startup()

class IGameModule {
public:
	virtual ~IGameModule() {}

	// --- Factory ---
	// Create game-specific objects. Called early in App::startup() before
	// any startup() calls. The module must set the Applet pointers:
	//   app->game, app->player, app->combat, app->entityDefManager,
	//   app->hackingGame, app->sentryBotGame, app->vendingMachine, app->comicBook
	virtual void createGameObjects(Applet* app) = 0;

	// --- Lifecycle ---
	// Called after engine subsystems (Canvas, Render, TinyGL, Sound) are up.
	// Start up game objects in order: EntityDefManager -> Player -> Game -> Combat
	// Return false on fatal error.
	virtual bool startup(Applet* app) = 0;

	// Called after startup completes. Load saved game configuration.
	virtual void loadConfig(Applet* app) = 0;

	// Called on engine shutdown. Clean up game objects.
	virtual void shutdown(Applet* app) = 0;

	// --- Extension points ---
	// Register custom script opcodes. Called after OpcodeRegistry is available.
	virtual void registerOpcodes(Applet* app) {}

	// Called when a new map is loaded, after engine has loaded map geometry.
	virtual void onMapLoaded(Applet* app, int mapID) {}

	// Called when a new game starts (player selected character, difficulty set).
	virtual void onNewGame(Applet* app) {}

	// --- Info ---
	// Human-readable name for this game module (for logs/debug).
	virtual const char* getName() const = 0;
};

#endif
