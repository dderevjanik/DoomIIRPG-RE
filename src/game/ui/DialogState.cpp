#include "DialogState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Hud.h"
#include "TinyGL.h"
#include "Enums.h"

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
	Applet* app = canvas->app;
	int keyAction = canvas->getKeyAction(key);

	if (keyAction == Enums::ACTION_FIRE) {
		if (canvas->dialogTypeLineIdx < canvas->dialogViewLines && canvas->dialogTypeLineIdx < canvas->numDialogLines - canvas->currentDialogLine) {
			canvas->dialogTypeLineIdx = canvas->dialogViewLines;
		}
		else if (canvas->currentDialogLine < canvas->numDialogLines - canvas->dialogViewLines) {
			canvas->dialogLineStartTime = app->time;
			canvas->dialogTypeLineIdx = 0;
			canvas->currentDialogLine += canvas->dialogViewLines;
			if (((canvas->dialogFlags & 0x4) != 0x0 || (canvas->dialogFlags & 0x1) != 0x0) && canvas->currentDialogLine + canvas->dialogViewLines > canvas->numDialogLines) {
				canvas->currentDialogLine = canvas->numDialogLines - canvas->dialogViewLines;
			}
		}
		else {
			canvas->closeDialog(false);
		}
	}
	else if (keyAction == Enums::ACTION_UP) {
		if (canvas->currentDialogLine >= canvas->numDialogLines - canvas->dialogViewLines && 0x0 != (canvas->dialogFlags & 0x2)) {
			if (app->game->scriptStateVars[4] == 0) {
				canvas->currentDialogLine--;
				if (canvas->currentDialogLine < 0) canvas->currentDialogLine = 0;
			}
			else {
				app->game->scriptStateVars[4]--;
			}
		}
		else {
			canvas->currentDialogLine--;
			if (canvas->currentDialogLine < 0) canvas->currentDialogLine = 0;
		}
	}
	else if (keyAction == Enums::ACTION_DOWN) {
		if (canvas->currentDialogLine >= canvas->numDialogLines - canvas->dialogViewLines && 0x0 != (canvas->dialogFlags & 0x2)) {
			if (app->game->scriptStateVars[4] < 1) {
				app->game->scriptStateVars[4]++;
			}
		}
		else {
			canvas->currentDialogLine++;
			if (canvas->currentDialogLine > canvas->numDialogLines - canvas->dialogViewLines) {
				canvas->currentDialogLine = canvas->numDialogLines - canvas->dialogViewLines;
				if (0x0 == (canvas->dialogFlags & 0x2)) {
					if (canvas->currentDialogLine < 0) canvas->currentDialogLine = 0;
				}
			}
			else {
				canvas->dialogLineStartTime = app->time;
				canvas->dialogTypeLineIdx = canvas->dialogViewLines - 1;
			}
		}
	}
	else if ((keyAction == Enums::ACTION_LEFT || keyAction == Enums::ACTION_RIGHT) && (canvas->dialogFlags & 0x5) != 0x0 && canvas->currentDialogLine >= canvas->numDialogLines - canvas->dialogViewLines) {
		app->game->scriptStateVars[4] ^= 0x1;
	}
	else if (keyAction == Enums::ACTION_PASSTURN || keyAction == Enums::ACTION_AUTOMAP) {
		canvas->closeDialog(true);
	}
	else if (keyAction == Enums::ACTION_MENU || keyAction == Enums::ACTION_LEFT) {
		canvas->currentDialogLine -= canvas->dialogViewLines;
		if (canvas->currentDialogLine < 0) canvas->currentDialogLine = 0;
	}
	else if (keyAction == Enums::ACTION_RIGHT) {
		canvas->currentDialogLine += canvas->dialogViewLines;
		if (canvas->currentDialogLine > canvas->numDialogLines - canvas->dialogViewLines) {
			canvas->currentDialogLine = std::max(canvas->numDialogLines - canvas->dialogViewLines, 0);
		}
	}
	if (canvas->state == Canvas::ST_PLAYING && app->game->monstersTurn == 0) {
		canvas->dequeueHelpDialog();
	}
	return true;
}
