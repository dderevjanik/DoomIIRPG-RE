#include "DyingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Hud.h"
#include "Render.h"
#include "MenuSystem.h"
#include "Enums.h"
#include "Menus.h"
#include "DialogManager.h"

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
		app->render->resetViewPort();
		canvas->drawPlayingSoftKeys();
	}
	canvas->clearSoftKeys();
	canvas->deathTime = app->time;
	canvas->destPitch = 64;
	app->dialogManager->numHelpMessages = 0;
}

void DyingState::onExit(Canvas* canvas) {
}

void DyingState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	app->hud->repaintFlags = 32;
	if (app->time < canvas->deathTime + 750) {
		int n = (750 - (app->time - canvas->deathTime) << 16) / 750;
		canvas->viewZ = app->render->getHeight(canvas->destX, canvas->destY) + 18 + (20 * n >> 16);
		canvas->viewPitch = 96 + (-96 * n >> 16);
		int n2 = 16 + (-16 * n >> 16);
		canvas->updateView();
		canvas->renderScene(canvas->viewX, canvas->viewY, canvas->viewZ, canvas->viewAngle, canvas->viewPitch, n2, 290);
		canvas->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
	}
	else if (app->time < canvas->deathTime + 2750) {
		if (!app->render->isFading()) {
			app->render->startFade(2000, 1);
		}
		canvas->renderScene(canvas->viewX, canvas->viewY, canvas->viewZ, canvas->viewAngle, canvas->viewPitch, 16, 290);
		canvas->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
	}
	else {
		app->render->baseDizzy = (app->render->destDizzy = 0);
		app->menuSystem->setMenu(Menus::MENU_INGAME_DEAD);
	}
}

void DyingState::render(Canvas* canvas, Graphics* graphics) {
}

bool DyingState::handleInput(Canvas* canvas, int key, int action) {
	if (canvas->stateVars[0] > 0 && (action == Enums::ACTION_FIRE || key == 18)) {
		canvas->app->menuSystem->setMenu(Menus::MENU_INGAME_DEAD);
	}
	return true;
}
