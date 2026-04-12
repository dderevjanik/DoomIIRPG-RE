#include "TreadmillState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Combat.h"
#include "Hud.h"

void TreadmillState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	canvas->clearSoftKeys();
	app->combat->shiftWeapon(true);
	app->hud->repaintFlags |= 0x20;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	canvas->treadmillNumSteps = 0;
	canvas->treadmillLastStep = 1;
	canvas->treadmillLastStepTime = app->time;
	canvas->treadmillReturnCode = 0;
}

void TreadmillState::onExit(Canvas* canvas) {
}

void TreadmillState::update(Canvas* canvas) {
	canvas->treadmillState();
}

void TreadmillState::render(Canvas* canvas, Graphics* graphics) {
	canvas->drawTreadmillReadout(graphics);
}

bool TreadmillState::handleInput(Canvas* canvas, int key, int action) {
	canvas->handleTreadmillEvents(action);
	return true;
}
