#include "LogoState.h"
#include "Canvas.h"
#include "CAppContainer.h"
#include "App.h"
#include "Game.h"
#include "Player.h"
#include "Sound.h"
#include "MenuSystem.h"
#include "Image.h"
#include "Graphics.h"
#include "Enums.h"
#include "CombatEntity.h"

void LogoState::onEnter(Canvas* canvas) {
	// No special setup needed — pacLogoTime is zeroed by memset
}

void LogoState::onExit(Canvas* canvas) {
	// No cleanup needed
}

void LogoState::update(Canvas* canvas) {
	Applet* app = CAppContainer::getInstance()->app;

	if (!app->sound->soundsLoaded) {
		app->sound->cacheSounds();
	}

	if (CAppContainer::getInstance()->customMapFile) {
		// Replicate startGame(true) + character selection init
		if (app->menuSystem->background != app->menuSystem->imgMainBG) {
			app->menuSystem->background->~Image();
		}
		app->menuSystem->imgMainBG->~Image();
		app->menuSystem->background = nullptr;
		app->menuSystem->imgMainBG = nullptr;
		app->sound->soundStop();

		canvas->setLoadingBarText(-1, -1);
		app->game->removeState(true);
		app->game->activeLoadType = 0;
		canvas->loadType = 0;
		canvas->loadMapID = 0;
		canvas->lastMapID = 0;
		app->player->reset();
		app->player->totalDeaths = 0;
		app->player->currentLevelDeaths = 0;
		app->player->helpBitmask = 0;
		app->player->invHelpBitmask = 0;
		app->player->ammoHelpBitmask = 0;
		app->player->weaponHelpBitmask = 0;
		app->player->armorHelpBitmask = 0;
		app->player->currentGrades = 0;
		app->player->ce->weapon = -1;
		canvas->clearEvents(1);

		app->game->difficulty = 1; // Normal
		app->player->setCharacterChoice(1); // Major (default)
		app->player->reset();

		canvas->loadMap(canvas->startupMap, false, true);
		return;
	}

	if (canvas->pacLogoTime <= 120) {
		canvas->pacLogoTime++;
		if (canvas->imgStartupLogo == nullptr) {
			canvas->imgStartupLogo = app->loadImage("l2.bmp", true);
		}
		canvas->repaintFlags |= Canvas::REPAINT_STARTUP_LOGO;
	}
	else {
		canvas->imgStartupLogo->~Image();
		canvas->imgStartupLogo = nullptr;

		canvas->setState(Canvas::ST_INTRO_MOVIE);
		canvas->numEvents = 0;
		canvas->keyDown = false;
		canvas->keyDownCausedMove = false;
		canvas->ignoreFrameInput = 1;
	}
}

void LogoState::render(Canvas* canvas, Graphics* graphics) {
	// Logo rendering is handled via REPAINT_STARTUP_LOGO flag in backPaint
	// (the flag-based rendering runs after the state dispatch)
}

bool LogoState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = CAppContainer::getInstance()->app;

	if (key == 18) { // Back button
		app->shutdown();
		return true;
	}
	return false;
}
