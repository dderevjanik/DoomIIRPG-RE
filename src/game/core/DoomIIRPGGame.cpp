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
#include "EventBus.h"
#include "GameEvents.h"

REGISTER_GAME_MODULE("doom2rpg", DoomIIRPGGame);

void DoomIIRPGGame::createGameObjects(Applet* app) {
	LOG_INFO("[game] createGameObjects\n");
	app->game = std::make_unique<Game>();
	app->player = std::make_unique<Player>();
	app->combat = std::make_unique<Combat>();
	app->entityDefManager = std::make_unique<EntityDefManager>();
	app->hackingGame = std::make_unique<HackingGame>();
	app->sentryBotGame = std::make_unique<SentryBotGame>();
	app->vendingMachine = std::make_unique<VendingMachine>();
	app->comicBook = std::make_unique<ComicBook>();
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
	app->canvas->registerStateHandler(Canvas::ST_INTRO_MOVIE, &introMovieState);
	app->canvas->registerStateHandler(Canvas::ST_INTRO, &introState);
	app->canvas->registerStateHandler(Canvas::ST_EPILOGUE, &epilogueState);
	app->canvas->registerStateHandler(Canvas::ST_CHARACTER_SELECTION, &characterSelectionState);
	app->canvas->registerStateHandler(Canvas::ST_TRAVELMAP, &travelMapState);
	app->canvas->registerStateHandler(Canvas::ST_DYING, &dyingState);
	app->canvas->registerStateHandler(Canvas::ST_BOT_DYING, &botDyingState);
	app->canvas->registerStateHandler(Canvas::ST_ERROR, &errorState);
	app->canvas->registerStateHandler(Canvas::ST_BENCHMARK, &benchmarkState);
	app->canvas->registerStateHandler(Canvas::ST_BENCHMARKDONE, &benchmarkState);
	app->canvas->registerStateHandler(Canvas::ST_DIALOG, &dialogState);
	app->canvas->registerStateHandler(Canvas::ST_LOOTING, &lootingState);
	app->canvas->registerStateHandler(Canvas::ST_TREADMILL, &treadmillState);
	app->canvas->registerStateHandler(Canvas::ST_CAMERA, &cameraState);
	app->canvas->registerStateHandler(Canvas::ST_INTER_CAMERA, &interCameraState);
	app->canvas->registerStateHandler(Canvas::ST_MINI_GAME, &miniGameState);
	app->canvas->registerStateHandler(Canvas::ST_PLAYING, &playingState);
	app->canvas->registerStateHandler(Canvas::ST_COMBAT, &combatState);
	app->canvas->registerStateHandler(Canvas::ST_MENU, &menuState);
	// ST_LOADING and ST_SAVING remain inline — their update() needs early return from run()
	// app->canvas->registerStateHandler(Canvas::ST_LOADING, &loadingState);
	// app->canvas->registerStateHandler(Canvas::ST_SAVING, &savingState);

	// Register minigames
	auto& mgReg = CAppContainer::getInstance()->minigameRegistry;
	mgReg.registerMinigame(0, app->sentryBotGame.get(), "sentrybot");
	mgReg.registerMinigame(2, app->hackingGame.get(),   "hacking");
	mgReg.registerMinigame(4, app->vendingMachine.get(), "vending");

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
		[app](const DataNode& d) -> ResourceManager::ParseResult {
			auto r = SpriteDefs::parse(d);
			if (!r) return r;
			app->render->initSpriteDefs();
			return parseSpriteAnims(app, d);
		}, 20);

	// items.yaml — item name definitions (optional)
	rm->registerParser("item_defs", "items.yaml",
		[](const DataNode& d) { return ItemDefs::parse(d); }, 30, true);

	// Entity definition files — load all files listed in game.yaml entities:, merge, then parse
	rm->registerLoader("entity_names", [app](ResourceManager* rm) -> ResourceManager::ParseResult {
		const auto& files = CAppContainer::getInstance()->gameConfig.entityFiles;
		DataNode merged;
		for (const auto& path : files) {
			DataNode data = rm->loadData(path.c_str());
			if (!data) {
				return std::unexpected("entity_names: failed to load " + path);
			}
			merged = merged ? DataNode::mergeDeep(merged, data) : data;
		}
		auto r = EntityNames::parseTypes(merged);
		if (!r) return r;
		return EntityDefManager::parse(app->entityDefManager.get(), merged);
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
	rm->registerLoader("effects", [app](ResourceManager* rm) -> ResourceManager::ParseResult {
		DataNode data = rm->loadData("effects.yaml");
		if (!data) data = DataNode();
		return parseEffects(app, data);
	}, 100);

	// items.yaml + effects.yaml — item use definitions (multi-file)
	rm->registerLoader("items", [app](ResourceManager* rm) -> ResourceManager::ParseResult {
		DataNode itemsData = rm->loadData("items.yaml");
		if (!itemsData) return {}; // optional
		DataNode effectsData = rm->loadData("effects.yaml");
		return parseItems(app, itemsData, effectsData);
	}, 110);

	// dialogs.yaml (optional — parser runs with empty node to init defaults)
	rm->registerLoader("dialogs", [app](ResourceManager* rm) -> ResourceManager::ParseResult {
		DataNode data = rm->loadData("dialogs.yaml");
		if (!data) data = DataNode();
		return parseDialogStyles(app, data);
	}, 120);

	// Monster combat data from entity files (after weapons and sounds)
	rm->registerLoader("monster_combat", [app](ResourceManager* rm) -> ResourceManager::ParseResult {
		const auto& files = CAppContainer::getInstance()->gameConfig.entityFiles;
		DataNode merged;
		for (const auto& path : files) {
			DataNode data = rm->loadData(path.c_str());
			if (!data) {
				return std::unexpected("monster_combat: failed to load " + path);
			}
			merged = merged ? DataNode::mergeDeep(merged, data) : data;
		}
		return parseMonsterCombatFromEntities(app, merged);
	}, 130);
}

void DoomIIRPGGame::registerOpcodes(Applet* app) {
	// Base Doom II RPG uses built-in opcodes (0-97) only.
	// Custom games can override this to register extension opcodes (128-254).
}

void DoomIIRPGGame::registerEventListeners(Applet* app) {
	app->eventBus->subscribe<MonsterDeathEvent>([](const MonsterDeathEvent& e) {
		LOG_INFO("[event] MonsterDeath: xp={} explosion={} pos=({},{})\n",
			e.xpAwarded, e.byExplosion, e.mapX, e.mapY);
	});
	app->eventBus->subscribe<ItemPickupEvent>([](const ItemPickupEvent& e) {
		LOG_INFO("[event] ItemPickup: type={} param={} dropped={}\n",
			e.itemType, e.itemParam, e.wasDropped);
	});
	app->eventBus->subscribe<WeaponSwitchEvent>([](const WeaponSwitchEvent& e) {
		LOG_INFO("[event] WeaponSwitch: {} -> {}\n", e.previousWeapon, e.newWeapon);
	});
	app->eventBus->subscribe<LevelLoadEvent>([](const LevelLoadEvent& e) {
		LOG_INFO("[event] LevelLoad: mapID={} newGame={}\n", e.mapID, e.isNewGame);
	});
	app->eventBus->subscribe<LevelLoadCompleteEvent>([](const LevelLoadCompleteEvent& e) {
		LOG_INFO("[event] LevelLoadComplete: mapID={} entities={}\n", e.mapID, e.entityCount);
	});
	app->eventBus->subscribe<DoorEvent>([](const DoorEvent& e) {
		LOG_INFO("[event] Door: opening={} pos=({},{})\n", e.opening, e.mapX, e.mapY);
	});
	app->eventBus->subscribe<PlayerHealEvent>([](const PlayerHealEvent& e) {
		LOG_INFO("[event] PlayerHeal: +{} ({} -> {})\n", e.amount, e.healthBefore, e.healthAfter);
	});
	app->eventBus->subscribe<PlayerDamageEvent>([](const PlayerDamageEvent& e) {
		LOG_INFO("[event] PlayerDamage: -{} ({} -> {})\n", e.damage, e.healthBefore, e.healthAfter);
	});
}
