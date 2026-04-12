#include "CombatState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Combat.h"
#include "Hud.h"
#include "Enums.h"

void CombatState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	app->hud->repaintFlags = 47;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	canvas->clearSoftKeys();
	canvas->combatDone = false;
}

void CombatState::onExit(Canvas* canvas) {
	Applet* app = canvas->app;
	// Only clean up if not re-entering combat and combat stage isn't menu
	if (canvas->state != Canvas::ST_COMBAT && app->combat->stage != Canvas::ST_MENU) {
		app->combat->cleanUpAttack();
	}
}

void CombatState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	app->game->updateAutomap = true;
	canvas->combatState();
}

void CombatState::render(Canvas* canvas, Graphics* graphics) {
	// 3D rendering handled by REPAINT_VIEW3D flag before handler dispatch
	// Swipe area and HUD rendered unconditionally for ST_COMBAT
}

bool CombatState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = canvas->app;
	if (app->combat->curAttacker != nullptr && !canvas->isZoomedIn) {
		if (action == Enums::ACTION_RIGHT) {
			canvas->destAngle -= 256;
		}
		else if (action == Enums::ACTION_LEFT) {
			canvas->destAngle += 256;
		}
		canvas->startRotation(false);
	}
	if (canvas->combatDone && !app->game->interpolatingMonsters) {
		canvas->setState(Canvas::ST_PLAYING);
		if (app->combat->curAttacker == nullptr) {
			app->game->advanceTurn();
			if (canvas->state == Canvas::ST_PLAYING) {
				if (canvas->isZoomedIn) {
					return canvas->handleZoomEvents(key, action);
				}
				return canvas->handlePlayingEvents(key, action);
			}
		}
	}
	return true;
}
