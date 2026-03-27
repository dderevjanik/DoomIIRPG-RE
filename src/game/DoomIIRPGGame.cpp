#include <cstdio>
#include "DoomIIRPGGame.h"
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
#include "SpriteDefs.h"
#include "ItemDefs.h"
#include "EntityNames.h"
#include "Sounds.h"
#include "ConfigEnums.h"
#include "Render.h"
#include <yaml-cpp/yaml.h>

void DoomIIRPGGame::createGameObjects(Applet* app) {
	printf("[game] createGameObjects\n");
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
	printf("[game] DoomIIRPGGame::startup\n");
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
	printf("[game] DoomIIRPGGame::shutdown\n");
}

void DoomIIRPGGame::registerLoaders(ResourceManager* rm) {
	Applet* app = CAppContainer::getInstance()->app;

	// tables.yaml — game data tables (priority 10, must be first)
	rm->registerLoader("tables", [app](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("tables.yaml");
		if (!node) return false;
		return app->loadTablesFromNode(*node);
	}, 10);

	// sprites.yaml — sprite definitions + sprite anims (priority 20)
	rm->registerLoader("sprites", [app](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("sprites.yaml");
		if (!node) return false;
		if (!SpriteDefs::loadFromNode(*node)) return false;
		app->render->initSpriteDefs();
		app->loadSpriteAnimsFromNode(*node);
		return true;
	}, 20);

	// items.yaml — item name definitions (priority 30, optional)
	rm->registerLoader("item_defs", [](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("items.yaml");
		if (!node) return true; // optional file
		return ItemDefs::loadFromNode(*node);
	}, 30);

	// entities.yaml — entity type/subtype names (priority 40)
	rm->registerLoader("entity_names", [](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("entities.yaml");
		if (!node) return false;
		return EntityNames::loadEntityTypesFromNode(*node);
	}, 40);

	// weapons.yaml — weapon names (priority 50)
	rm->registerLoader("weapon_names", [](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("weapons.yaml");
		if (!node) return false;
		return EntityNames::loadWeaponNamesFromNode(*node);
	}, 50);

	// sounds.yaml — sound definitions (priority 60)
	rm->registerLoader("sounds", [](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("sounds.yaml");
		if (!node) return false;
		return Sounds::loadFromNode(*node);
	}, 60);

	// game.yaml — config enums (priority 70)
	rm->registerLoader("config_enums", [](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("game.yaml");
		if (!node) return false;
		return ConfigEnums::loadFromNode(*node);
	}, 70);

	// weapons.yaml — weapon data (priority 80, after sounds for sound lookups)
	rm->registerLoader("weapons", [app](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("weapons.yaml");
		if (!node) return false;
		return app->loadWeaponsFromNode(*node);
	}, 80);

	// projectiles.yaml (priority 90)
	rm->registerLoader("projectiles", [app](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("projectiles.yaml");
		if (!node) {
			printf("[resource] projectiles.yaml not found, using defaults\n");
			return true;
		}
		return app->loadProjectilesFromNode(*node);
	}, 90);

	// effects.yaml (priority 100)
	rm->registerLoader("effects", [app](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("effects.yaml");
		if (!node) {
			printf("[resource] effects.yaml not found, using defaults\n");
			return true;
		}
		return app->loadEffectsFromNode(*node);
	}, 100);

	// items.yaml + effects.yaml — item use definitions (priority 110, optional)
	rm->registerLoader("items", [app](ResourceManager* rm) {
		const YAML::Node* itemsNode = rm->loadYAML("items.yaml");
		if (!itemsNode) return true; // optional file
		const YAML::Node* effectsNode = rm->loadYAML("effects.yaml");
		YAML::Node emptyNode;
		return app->loadItemsFromNode(*itemsNode, effectsNode ? *effectsNode : emptyNode);
	}, 110);

	// dialogs.yaml (priority 120)
	rm->registerLoader("dialogs", [app](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("dialogs.yaml");
		if (!node) {
			printf("[resource] dialogs.yaml not found, using defaults\n");
			return true;
		}
		return app->loadDialogStylesFromNode(*node);
	}, 120);

	// monsters.yaml (priority 130, after weapons and sounds)
	rm->registerLoader("monsters", [app](ResourceManager* rm) {
		const YAML::Node* node = rm->loadYAML("monsters.yaml");
		if (!node) return false;
		return app->loadMonstersFromNode(*node);
	}, 130);
}

void DoomIIRPGGame::registerOpcodes(Applet* app) {
	// Base Doom II RPG uses built-in opcodes (0-97) only.
	// Custom games can override this to register extension opcodes (128-254).
}
