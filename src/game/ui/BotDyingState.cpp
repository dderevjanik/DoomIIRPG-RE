#include "BotDyingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Hud.h"

void BotDyingState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	app->hud->repaintFlags = 47;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	canvas->clearSoftKeys();
	canvas->familiarDeathTime = app->time;
	canvas->selfDestructScreenShakeStarted = false;
	app->hud->brightenScreen(100, 0);
	if (!canvas->familiarSelfDestructed) {
		app->hud->smackScreen(100);
	}
	canvas->destPitch = 64;
}

void BotDyingState::onExit(Canvas* canvas) {
}

void BotDyingState::update(Canvas* canvas) {
	canvas->familiarDyingState();
}

void BotDyingState::render(Canvas* canvas, Graphics* graphics) {
}

bool BotDyingState::handleInput(Canvas* canvas, int key, int action) {
	return false;
}
