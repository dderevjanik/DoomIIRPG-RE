#include "CameraState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Hud.h"
#include "Render.h"
#include "TinyGL.h"
#include "MayaCamera.h"
#include "Enums.h"

void CameraState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	app->hud->msgCount = 0;
	app->hud->subTitleID = -1;
	app->hud->cinTitleID = -1;
	app->render->disableRenderActivate = true;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	app->hud->repaintFlags = 24;
	canvas->clearSoftKeys();
	app->render->setViewport(canvas->cinRect[0], canvas->cinRect[1], canvas->cinRect[2], canvas->cinRect[3]);
}

void CameraState::onExit(Canvas* canvas) {
	Applet* app = canvas->app;
	app->render->disableRenderActivate = false;
	app->game->skippingCinematic = false;
}

void CameraState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	if (app->game->activeCameraKey != -1) {
		app->game->activeCamera->Update(app->game->activeCameraKey, app->gameTime - app->game->activeCameraTime);
	}
	app->game->updateLerpSprites();
	canvas->updateView();
	if (canvas->state == Canvas::ST_CAMERA && app->gameTime > app->game->cinUnpauseTime && canvas->softKeyRightID == -1) {
		canvas->clearLeftSoftKey();
		canvas->setRightSoftKey((short)0, (short)40);
	}
}

void CameraState::render(Canvas* canvas, Graphics* graphics) {
	// 3D rendering handled by the main pipeline (combined condition in backPaint)
}

bool CameraState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = canvas->app;
	if (!canvas->changeMapStarted && app->gameTime > app->game->cinUnpauseTime && (action == Enums::ACTION_PASSTURN || action == Enums::ACTION_AUTOMAP || action == Enums::ACTION_FIRE || key == 18)) {
		app->game->skipCinematic();
	}
	return true;
}
