#include "DialogState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Hud.h"
#include "TinyGL.h"

void DialogState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	if (app->canvas->isZoomedIn) {
		canvas->isZoomedIn = 0;
		app->StopAccelerometer();
		canvas->destAngle = canvas->viewAngle = (canvas->viewAngle + canvas->zoomAngle + 127) & 0xFFFFFF00;
		app->tinyGL->resetViewPort();
		canvas->drawPlayingSoftKeys();
	}
	if (app->game->isCameraActive()) {
		app->game->activeCameraTime = app->gameTime - app->game->activeCameraTime;
	}
	app->hud->repaintFlags = 47;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	app->tinyGL->resetViewPort();
	canvas->clearSoftKeys();
	canvas->clearEvents(1);
}

void DialogState::onExit(Canvas* canvas) {
}

void DialogState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	app->game->updateLerpSprites();
	canvas->updateView();
	canvas->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
	app->hud->repaintFlags |= 0x2B;
}

void DialogState::render(Canvas* canvas, Graphics* graphics) {
	canvas->dialogState(graphics);
}

bool DialogState::handleInput(Canvas* canvas, int key, int action) {
	canvas->handleDialogEvents(key);
	return true;
}
