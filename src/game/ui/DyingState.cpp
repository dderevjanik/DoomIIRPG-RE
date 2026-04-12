#include "DyingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Hud.h"
#include "TinyGL.h"
#include "MenuSystem.h"
#include "Enums.h"
#include "Menus.h"

void DyingState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	app->hud->repaintFlags = 47;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	if (canvas->isZoomedIn) {
		canvas->isZoomedIn = false;
		app->StopAccelerometer();
		canvas->viewAngle += canvas->zoomAngle;
		int n3 = 255;
		canvas->destAngle = (canvas->viewAngle = (canvas->viewAngle + (n3 >> 1) & ~n3));
		app->tinyGL->resetViewPort();
		canvas->drawPlayingSoftKeys();
	}
	canvas->clearSoftKeys();
	canvas->deathTime = app->time;
	canvas->destPitch = 64;
	canvas->numHelpMessages = 0;
}

void DyingState::onExit(Canvas* canvas) {
}

void DyingState::update(Canvas* canvas) {
	canvas->dyingState();
}

void DyingState::render(Canvas* canvas, Graphics* graphics) {
}

bool DyingState::handleInput(Canvas* canvas, int key, int action) {
	if (canvas->stateVars[0] > 0 && (action == Enums::ACTION_FIRE || key == 18)) {
		canvas->app->menuSystem->setMenu(Menus::MENU_INGAME_DEAD);
	}
	return true;
}
