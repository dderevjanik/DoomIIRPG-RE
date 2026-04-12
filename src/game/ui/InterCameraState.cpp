#include "InterCameraState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Hud.h"

void InterCameraState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	app->hud->repaintFlags = 43;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
}

void InterCameraState::onExit(Canvas* canvas) {
}

void InterCameraState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	app->hud->repaintFlags |= 0x2B;
}

void InterCameraState::render(Canvas* canvas, Graphics* graphics) {
	// 3D rendering handled by the main pipeline (combined condition in backPaint)
}

bool InterCameraState::handleInput(Canvas* canvas, int key, int action) {
	return false;
}
