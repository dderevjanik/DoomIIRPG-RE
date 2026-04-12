#include "SavingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Hud.h"
#include "Combat.h"
#include "MenuSystem.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Menus.h"

void SavingState::onEnter(Canvas* canvas) {
	canvas->repaintFlags &= ~Canvas::REPAINT_HUD;
	canvas->pacifierX = canvas->SCR_CX - 66;
	canvas->updateLoadingBar(false);
}

void SavingState::onExit(Canvas* canvas) {
}

void SavingState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	if ((canvas->saveType & 0x2) != 0x0 || (canvas->saveType & 0x1) != 0x0) {
		if ((canvas->saveType & 0x8) != 0x0) {
			if (app->game->spawnParam != 0) {
				int n11 = 32 + ((app->game->spawnParam & 0x1F) << 6);
				int n12 = 32 + ((app->game->spawnParam >> 5 & 0x1F) << 6);
				app->game->saveState(canvas->lastMapID, canvas->loadMapID, n11, n12, (app->game->spawnParam >> 10 & 0xFF) << 7, 0, n11, n12, n11, n12, 36, 0, 0, canvas->saveType);
			}
			else {
				app->game->saveState(canvas->lastMapID, canvas->loadMapID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, canvas->saveType);
			}
		}
		else if ((canvas->saveType & 0x10) != 0x0) {
			int n13 = 32 + ((app->game->spawnParam & 0x1F) << 6);
			int n14 = 32 + ((app->game->spawnParam >> 5 & 0x1F) << 6);
			app->game->saveState(canvas->loadMapID, app->menuSystem->LEVEL_STATS_nextMap, n13, n14, (app->game->spawnParam >> 10 & 0xFF) << 7, 0, n13, n14, n13, n14, 36, 0, 0, canvas->saveType);
		}
		else {
			app->game->saveState(canvas->loadMapID, canvas->loadMapID, canvas->destX, canvas->destY, canvas->destAngle, canvas->viewPitch, canvas->prevX, canvas->prevY, canvas->saveX, canvas->saveY, canvas->saveZ, canvas->saveAngle, canvas->savePitch, canvas->saveType);
		}
		app->hud->addMessage((short)0, (short)38);
	}
	else {
		app->Error(48); // ERR_SAVESTATE
	}

	if ((canvas->saveType & 0x4) != 0x0) {
		canvas->backToMain(false);
	}
	else if ((canvas->saveType & 0x40) != 0x0) {
		app->shutdown();
	}
	else if ((canvas->saveType & 0x8) != 0x0) {
		canvas->setState(Canvas::ST_TRAVELMAP);
	}
	else if ((canvas->saveType & 0x10) != 0x0) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_LEVEL_END), 1u, 3, canvas->saveType & 8);
		app->menuSystem->setMenu(Menus::MENU_LEVEL_STATS);
	}
	else if ((canvas->saveType & 0x100) != 0x0) {
		app->menuSystem->setMenu(Menus::MENU_END_FINALQUIT);
	}
	else {
		if ((canvas->saveType & 0x80) != 0x0) {
			app->menuSystem->returnToGame();
		}
		canvas->setState(Canvas::ST_PLAYING);
	}
	canvas->saveType = 0;
	canvas->clearEvents(1);
}

void SavingState::render(Canvas* canvas, Graphics* graphics) {
	// Saving bar rendering handled by the main pipeline
}

bool SavingState::handleInput(Canvas* canvas, int key, int action) {
	return true;
}
