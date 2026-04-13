#include "BotDyingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Hud.h"
#include "Render.h"
#include "Player.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"

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
	Applet* app = canvas->app;
	app->hud->repaintFlags = 36;
	if (canvas->familiarSelfDestructed) {
		canvas->renderScene(canvas->viewX, canvas->viewY, canvas->viewZ, canvas->viewAngle, canvas->viewPitch, canvas->viewRoll, 290);
		canvas->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
		if (app->time >= canvas->familiarDeathTime + 1500) {
			app->sound->playSound(Sounds::getResIDByName(SoundName::EXPLOSION), 0, 3, 0);
			canvas->setState(Canvas::ST_PLAYING);
			app->player->familiarDied();
		}
		else if (!canvas->selfDestructScreenShakeStarted && app->time >= canvas->familiarDeathTime + 750) {
			canvas->selfDestructScreenShakeStarted = true;
			canvas->startShake(canvas->familiarDeathTime + 1500 - app->time, 5, 500);
		}
	}
	else if (app->time < canvas->familiarDeathTime + 750) {
		int n = (750 - (app->time - canvas->familiarDeathTime) << 16) / 750;
		canvas->viewZ = app->render->getHeight(canvas->destX, canvas->destY) + 18 + (20 * n >> 16);
		canvas->viewPitch = 96 + (-96 * n >> 16);
		int n2 = 16 + (-16 * n >> 16);
		canvas->updateView();
		canvas->renderScene(canvas->viewX, canvas->viewY, canvas->viewZ, canvas->viewAngle, canvas->viewPitch, n2, 290);
		canvas->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
	}
	else if (app->time < canvas->familiarDeathTime + 1500) {
		if (!app->render->isFading()) {
			app->render->startFade(750, 1);
		}
		canvas->renderScene(canvas->viewX, canvas->viewY, canvas->viewZ, canvas->viewAngle, canvas->viewPitch, 16, 290);
		canvas->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
	}
	else {
		canvas->setState(Canvas::ST_PLAYING);
		app->player->familiarDied();
	}
}

void BotDyingState::render(Canvas* canvas, Graphics* graphics) {
}

bool BotDyingState::handleInput(Canvas* canvas, int key, int action) {
	return false;
}
