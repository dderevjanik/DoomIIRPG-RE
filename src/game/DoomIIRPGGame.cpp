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

void DoomIIRPGGame::createGameObjects(Applet* app) {
	printf("DoomIIRPGGame::createGameObjects\n");
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
	printf("DoomIIRPGGame::startup\n");
	if (!app->entityDefManager->startup()) return false;
	if (!app->player->startup()) return false;
	if (!app->game->startup()) return false;
	if (!app->combat->startup()) return false;

	// Register canvas state handlers
	app->canvas->registerStateHandler(Canvas::ST_LOGO, &logoState);
	app->canvas->registerStateHandler(Canvas::ST_CREDITS, &creditsState);
	app->canvas->registerStateHandler(Canvas::ST_AUTOMAP, &automapState);

	return true;
}

void DoomIIRPGGame::loadConfig(Applet* app) {
	app->game->loadConfig();
}

void DoomIIRPGGame::shutdown(Applet* app) {
	printf("DoomIIRPGGame::shutdown\n");
}

void DoomIIRPGGame::registerOpcodes(Applet* app) {
	// Base Doom II RPG uses built-in opcodes (0-97) only.
	// Custom games can override this to register extension opcodes (128-254).
}
