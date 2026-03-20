#include "CreditsState.h"
#include "Canvas.h"
#include "CAppContainer.h"
#include "App.h"
#include "Game.h"
#include "Sound.h"
#include "MenuSystem.h"
#include "Image.h"
#include "Graphics.h"
#include "Text.h"
#include "Enums.h"
#include "Menus.h"
#include "Resource.h"

void CreditsState::onEnter(Canvas* canvas) {
	Applet* app = CAppContainer::getInstance()->app;
	app->localization->loadText(2);
	canvas->initScrollingText(2, 0, false, 16, 5, 500);
	app->localization->unloadText(2);
}

void CreditsState::onExit(Canvas* canvas) {
	canvas->dialogBuffer->dispose();
	canvas->dialogBuffer = nullptr;
}

void CreditsState::update(Canvas* canvas) {
	// Credits has no per-frame update logic — rendering drives the scroll
}

void CreditsState::render(Canvas* canvas, Graphics* graphics) {
	canvas->drawCredits(graphics);
}

bool CreditsState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = CAppContainer::getInstance()->app;

	if (key == 18) { // Back button — handled same as other states
		return true;
	}

	if (canvas->endingGame) {
		if ((action == Enums::ACTION_FIRE && canvas->scrollingTextDone) || key == 18) {
			canvas->endingGame = false;
			app->sound->soundStop();
			app->menuSystem->imgMainBG->~Image();
			app->menuSystem->imgLogo->~Image();
			app->menuSystem->imgMainBG = app->loadImage(Resources::RES_LOGO_BMP_GZ, true);
			app->menuSystem->imgLogo = app->loadImage(Resources::RES_LOGO2_BMP_GZ, true);
			app->menuSystem->background = app->menuSystem->imgMainBG;
			app->menuSystem->setMenu(Menus::MENU_END_);
		}
	}
	else if (action == Enums::ACTION_BACK || action == Enums::ACTION_FIRE) {
		if (canvas->loadMapID == 0) {
			int n2 = 3;
			if (app->game->hasSavedState()) {
				++n2;
			}
			app->menuSystem->pushMenu(3, n2, 0, 0, 0);
			app->menuSystem->setMenu(Menus::MENU_MAIN_HELP);
		}
		else {
			app->sound->soundStop();
			app->menuSystem->pushMenu(29, 7, 0, 0, 0);
			app->menuSystem->setMenu(Menus::MENU_INGAME_HELP);
		}
	}

	return true;
}
