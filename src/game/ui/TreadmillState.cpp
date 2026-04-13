#include "TreadmillState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Player.h"
#include "Combat.h"
#include "Hud.h"
#include "Render.h"
#include "Graphics.h"
#include "Text.h"
#include "Button.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Enums.h"

static bool treadmillFall(Canvas* canvas) {
	Applet* app = canvas->app;
	if (app->time > canvas->treadmillLastStepTime + 1000 + 500 + 500) {
		canvas->viewX = canvas->destX;
		canvas->viewY = canvas->destY;
		canvas->viewZ = canvas->destZ;
		canvas->viewPitch = canvas->destPitch;
		return true;
	}
	int n = (canvas->viewStepX >> 6) * 32;
	int n2 = (canvas->viewStepY >> 6) * 32;
	if (canvas->treadmillLastStepTime + 1000 > app->time) {
		int n3 = 1000 - (app->time - canvas->treadmillLastStepTime);
		int n4 = (n3 << 16) / 1000;
		canvas->viewX = canvas->destX + (-n + (n * n4 >> 16));
		canvas->viewY = canvas->destY + (-n2 + (n2 * n4 >> 16));
		canvas->viewZ = 36;
		if (n3 < 250) {
			canvas->viewZ -= 12 - (12 * n4 >> 16);
		}
		if (n3 > 500) {
			canvas->viewPitch = 128 - (128 * n4 >> 16);
		}
		else {
			canvas->viewPitch = 128 * n4 >> 16;
		}
	}
	else if (canvas->treadmillLastStepTime + 1000 + 500 > app->time) {
		canvas->viewX = canvas->destX - n;
		canvas->viewY = canvas->destY - n2;
		canvas->viewZ = 24;
		canvas->viewPitch = 0;
	}
	else {
		int n5 = 500 - (app->time - 1000 - 500 - canvas->treadmillLastStepTime);
		int n6 = (n5 << 16) / 500;
		canvas->viewX = canvas->destX - (n * n6 >> 16);
		canvas->viewY = canvas->destY - (n2 * n6 >> 16);
		canvas->viewZ = 36 - (12 * n6 >> 16);
		if (n5 > 250) {
			canvas->viewPitch = -128 + (128 * n6 >> 16);
		}
		else {
			canvas->viewPitch = -(128 * n6 >> 16);
		}
	}
	canvas->viewZ += app->render->getHeight(canvas->destX, canvas->destY);
	canvas->invalidateRect();
	canvas->renderScene(canvas->viewX, canvas->viewY, canvas->viewZ, canvas->viewAngle, canvas->viewPitch, canvas->viewRoll, 290);
	return false;
}

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
	Applet* app = canvas->app;
	app->hud->repaintFlags |= 0x22;
	canvas->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_VIEW3D);
	app->hud->repaintFlags &= 0xFFFFFFBF;
	bool b = false;
	if (canvas->treadmillReturnCode != 0) {
		if (canvas->treadmillReturnCode == 1 && app->time > canvas->treadmillLastStepTime + 300) {
			app->localization->resetTextArgs();
			app->localization->addTextArg(2);
			if (app->player->modifyStat(Enums::STAT_AGILITY, 2) == 0) {
				app->hud->addMessage((short)244, 3);
			}
			else {
				app->hud->addMessage((short)243, 3);
			}
			b = true;
		}
		else if (canvas->treadmillReturnCode == 3) {
			if (treadmillFall(canvas)) {
				app->hud->addMessage((short)245, 3);
				b = true;
			}
		}
		else if (canvas->treadmillReturnCode == 2) {
			canvas->attemptMove(canvas->viewX - canvas->viewStepX, canvas->viewY - canvas->viewStepY);
			app->hud->addMessage((short)246, 3);
			b = true;
		}
	}
	if (b) {
		app->combat->shiftWeapon(false);
		canvas->setState(Canvas::ST_PLAYING);
		return;
	}
	if (canvas->treadmillLastStep == 1) {
		canvas->updateView();
		return;
	}
	if (canvas->treadmillReturnCode == 0 && app->time > 1500 + canvas->treadmillLastStepTime) {
		canvas->treadmillReturnCode = 3;
		canvas->treadmillLastStepTime = app->time;
		return;
	}
	if (app->time <= canvas->treadmillLastStepTime + 300) {
		bool b2 = canvas->treadmillLastStep == 9;
		bool b3 = canvas->treadmillLastStepTime + 150 > app->time;
		bool b4 = b2 ^ !b3;
		int n = 4 + (-4 * ((std::abs(150 - (app->time - canvas->treadmillLastStepTime)) << 16) / 150) >> 16);
		if (b4 == b3) {
			n = -n;
		}
		int n2 = n * n;
		canvas->viewX = canvas->destX + n * (canvas->viewRightStepX >> 6);
		canvas->viewY = canvas->destY + n * (canvas->viewRightStepY >> 6);
		canvas->viewZ = 36 + n2 + app->render->getHeight(canvas->destX, canvas->destY);
		canvas->invalidateRect();
	}
	canvas->renderScene(canvas->viewX, canvas->viewY, canvas->viewZ, canvas->viewAngle, canvas->viewPitch, canvas->viewRoll, 290);
}

void TreadmillState::render(Canvas* canvas, Graphics* graphics) {
	Applet* app = canvas->app;
	Text* smallBuffer = app->localization->getSmallBuffer();
	app->localization->resetTextArgs();
	app->localization->addTextArg(canvas->treadmillNumSteps * 2);
	app->localization->composeText((short)0, (short)242, smallBuffer);
	app->hud->drawImportantMessage(graphics, smallBuffer, 0xFF666666);
	smallBuffer->dispose();
	canvas->m_treadmillButtons->Render(graphics);
}

bool TreadmillState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = canvas->app;
	if (canvas->treadmillReturnCode != 0) {
		return true;
	}
	if (canvas->treadmillLastStepTime + 300 > app->time) {
		if (canvas->numEvents == 4) {
			app->hud->addMessage((short)247, 3);
		}
		return true;
	}
	if (action == Enums::ACTION_DOWN) {
		canvas->treadmillReturnCode = 2;
	}
	else if (action == Enums::ACTION_STRAFELEFT || action == Enums::ACTION_STRAFERIGHT) {
		if (action == canvas->treadmillLastStep) {
			canvas->treadmillReturnCode = 3;
		}
		else {
			canvas->treadmillLastStep = action;
			if (++canvas->treadmillNumSteps * 2 >= 100) {
				canvas->treadmillReturnCode = 1;
			}
		}
		canvas->treadmillLastStepTime = app->time;
	}
	else if (action == Enums::ACTION_AUTOMAP) {
		canvas->treadmillReturnCode = 2;
	}
	return true;
}
