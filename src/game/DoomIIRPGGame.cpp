#include "DoomIIRPGGame.h"
#include "GameModuleRegistry.h"
#include "GameDataParsers.h"
#include "App.h"
#include "Game.h"
#include "Player.h"
#include "Combat.h"
#include "EntityDef.h"
#include "HackingGame.h"
#include "SentryBotGame.h"
#include "VendingMachine.h"
#include "ComicBook.h"
#include "Canvas.h"
#include "CAppContainer.h"
#include "ResourceManager.h"
#include "DataNode.h"
#include "SpriteDefs.h"
#include "ItemDefs.h"
#include "EntityNames.h"
#include "Sounds.h"
#include "ConfigEnums.h"
#include "Render.h"
#include "Log.h"

REGISTER_GAME_MODULE("doom2rpg", DoomIIRPGGame);

void DoomIIRPGGame::createGameObjects(Applet* app) {
	LOG_INFO("[game] createGameObjects\n");
	app->game = new Game;
	app->player = new Player;
	app->combat = new Combat;
	app->entityDefManager = new EntityDefManager;
	app->hackingGame = new HackingGame;
	app->sentryBotGame = new SentryBotGame;
	app->vendingMachine = new VendingMachine;
	app->comicBook = new ComicBook;
}

bool DoomIIRPGGame::startup(Applet* app) {
	LOG_INFO("[game] DoomIIRPGGame::startup\n");
	if (!app->entityDefManager->startup()) return false;
	if (!app->player->startup()) return false;
	if (!app->game->startup()) return false;
	if (!app->combat->startup()) return false;

	// Register canvas state handlers
	app->canvas->registerStateHandler(Canvas::ST_LOGO, &logoState);
	app->canvas->registerStateHandler(Canvas::ST_CREDITS, &creditsState);
	app->canvas->registerStateHandler(Canvas::ST_AUTOMAP, &automapState);

	// Register minigames
	auto& mgReg = CAppContainer::getInstance()->minigameRegistry;
	mgReg.registerMinigame(0, app->sentryBotGame, "sentrybot");
	mgReg.registerMinigame(2, app->hackingGame,   "hacking");
	mgReg.registerMinigame(4, app->vendingMachine, "vending");

	return true;
}

void DoomIIRPGGame::loadConfig(Applet* app) {
	app->game->loadConfig();
}

void DoomIIRPGGame::shutdown(Applet* app) {
	LOG_INFO("[game] DoomIIRPGGame::shutdown\n");
}

void DoomIIRPGGame::registerLoaders(ResourceManager* rm) {
	Applet* app = CAppContainer::getInstance()->app;

	// tables.yaml — game data tables (must be first)
	rm->registerParser("tables", "tables.yaml",
		[app](const DataNode& d) { return parseTables(app, d); }, 10);

	// sprites.yaml — sprite definitions + sprite anims
	rm->registerParser("sprites", "sprites.yaml",
		[app](const DataNode& d) {
			if (!SpriteDefs::parse(d)) return false;
			app->render->initSpriteDefs();
			return parseSpriteAnims(app, d);
		}, 20);

	// items.yaml — item name definitions (optional)
	rm->registerParser("item_defs", "items.yaml",
		[](const DataNode& d) { return ItemDefs::parse(d); }, 30, true);

	// entities.yaml — entity type/subtype names + entity definitions
	rm->registerParser("entity_names", "entities.yaml",
		[app](const DataNode& d) {
			if (!EntityNames::parseTypes(d)) return false;
			return EntityDefManager::parse(app->entityDefManager, d);
		}, 40);

	// weapons.yaml — weapon names (before entity defs, needed for parm resolution)
	rm->registerParser("weapon_names", "weapons.yaml",
		[](const DataNode& d) { return EntityNames::parseWeapons(d); }, 35);

	// sounds.yaml — sound definitions
	rm->registerParser("sounds", "sounds.yaml",
		[](const DataNode& d) { return Sounds::parse(d); }, 60);

	// game.yaml — config enums
	rm->registerParser("config_enums", "game.yaml",
		[](const DataNode& d) { return ConfigEnums::parse(d); }, 70);

	// weapons.yaml — weapon data (after sounds for sound lookups)
	rm->registerParser("weapons", "weapons.yaml",
		[app](const DataNode& d) { return parseWeapons(app, d); }, 80);

	// projectiles.yaml (optional)
	rm->registerParser("projectiles", "projectiles.yaml",
		[app](const DataNode& d) { return parseProjectiles(app, d); }, 90, true);

	// effects.yaml (optional — parser runs with empty node to init defaults)
	rm->registerLoader("effects", [app](ResourceManager* rm) {
		DataNode data = rm->loadData("effects.yaml");
		if (!data) data = DataNode();
		return parseEffects(app, data);
	}, 100);

	// items.yaml + effects.yaml — item use definitions (multi-file)
	rm->registerLoader("items", [app](ResourceManager* rm) {
		DataNode itemsData = rm->loadData("items.yaml");
		if (!itemsData) return true; // optional
		DataNode effectsData = rm->loadData("effects.yaml");
		return parseItems(app, itemsData, effectsData);
	}, 110);

	// dialogs.yaml (optional — parser runs with empty node to init defaults)
	rm->registerLoader("dialogs", [app](ResourceManager* rm) {
		DataNode data = rm->loadData("dialogs.yaml");
		if (!data) data = DataNode();
		return parseDialogStyles(app, data);
	}, 120);

	// monsters.yaml (after weapons and sounds)
	rm->registerParser("monsters", "monsters.yaml",
		[app](const DataNode& d) { return parseMonsters(app, d); }, 130);
}

void DoomIIRPGGame::registerOpcodes(Applet* app) {
	// Base Doom II RPG uses built-in opcodes (0-97) only.
	// Custom games can override this to register extension opcodes (128-254).
}
