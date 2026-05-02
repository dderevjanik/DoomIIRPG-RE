#include <algorithm>
#include "DialogState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Hud.h"
#include "Render.h"
#include "Enums.h"
#include "DialogManager.h"

void DialogState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	if (app->canvas->isZoomedIn) {
		canvas->isZoomedIn = 0;
		app->StopAccelerometer();
		canvas->destAngle = canvas->viewAngle = (canvas->viewAngle + canvas->zoomAngle + 127) & 0xFFFFFF00;
		app->render->resetViewPort();
		canvas->drawPlayingSoftKeys();
	}
	if (app->game->isCameraActive()) {
		app->game->activeCameraTime = app->gameTime - app->game->activeCameraTime;
	}
	app->hud->repaintFlags = 47;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	app->render->resetViewPort();
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
	canvas->app->dialogManager->render(canvas, graphics);
}

bool DialogState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = canvas->app;
	DialogManager* dm = app->dialogManager.get();
	int keyAction = canvas->getKeyAction(key);

	if (keyAction == Enums::ACTION_FIRE) {
		if (dm->dialogTypeLineIdx < dm->dialogViewLines && dm->dialogTypeLineIdx < dm->numDialogLines - dm->currentDialogLine) {
			dm->dialogTypeLineIdx = dm->dialogViewLines;
		}
		else if (dm->currentDialogLine < dm->numDialogLines - dm->dialogViewLines) {
			dm->dialogLineStartTime = app->time;
			dm->dialogTypeLineIdx = 0;
			dm->currentDialogLine += dm->dialogViewLines;
			if (((dm->dialogFlags & 0x4) != 0x0 || (dm->dialogFlags & 0x1) != 0x0) && dm->currentDialogLine + dm->dialogViewLines > dm->numDialogLines) {
				dm->currentDialogLine = dm->numDialogLines - dm->dialogViewLines;
			}
		}
		else {
			dm->close(canvas, false);
		}
	}
	else if (keyAction == Enums::ACTION_UP) {
		if (dm->currentDialogLine >= dm->numDialogLines - dm->dialogViewLines && 0x0 != (dm->dialogFlags & 0x2)) {
			if (app->game->scriptStateVars[4] == 0) {
				dm->currentDialogLine--;
				if (dm->currentDialogLine < 0) dm->currentDialogLine = 0;
			}
			else {
				app->game->scriptStateVars[4]--;
			}
		}
		else {
			dm->currentDialogLine--;
			if (dm->currentDialogLine < 0) dm->currentDialogLine = 0;
		}
	}
	else if (keyAction == Enums::ACTION_DOWN) {
		if (dm->currentDialogLine >= dm->numDialogLines - dm->dialogViewLines && 0x0 != (dm->dialogFlags & 0x2)) {
			if (app->game->scriptStateVars[4] < 1) {
				app->game->scriptStateVars[4]++;
			}
		}
		else {
			dm->currentDialogLine++;
			if (dm->currentDialogLine > dm->numDialogLines - dm->dialogViewLines) {
				dm->currentDialogLine = dm->numDialogLines - dm->dialogViewLines;
				if (0x0 == (dm->dialogFlags & 0x2)) {
					if (dm->currentDialogLine < 0) dm->currentDialogLine = 0;
				}
			}
			else {
				dm->dialogLineStartTime = app->time;
				dm->dialogTypeLineIdx = dm->dialogViewLines - 1;
			}
		}
	}
	else if ((keyAction == Enums::ACTION_LEFT || keyAction == Enums::ACTION_RIGHT) && (dm->dialogFlags & 0x5) != 0x0 && dm->currentDialogLine >= dm->numDialogLines - dm->dialogViewLines) {
		app->game->scriptStateVars[4] ^= 0x1;
	}
	else if (keyAction == Enums::ACTION_PASSTURN || keyAction == Enums::ACTION_AUTOMAP) {
		dm->close(canvas, true);
	}
	else if (keyAction == Enums::ACTION_MENU || keyAction == Enums::ACTION_LEFT) {
		dm->currentDialogLine -= dm->dialogViewLines;
		if (dm->currentDialogLine < 0) dm->currentDialogLine = 0;
	}
	else if (keyAction == Enums::ACTION_RIGHT) {
		dm->currentDialogLine += dm->dialogViewLines;
		if (dm->currentDialogLine > dm->numDialogLines - dm->dialogViewLines) {
			dm->currentDialogLine = std::max(dm->numDialogLines - dm->dialogViewLines, 0);
		}
	}
	if (canvas->state == Canvas::ST_PLAYING && app->game->monstersTurn == 0) {
		dm->dequeueHelpDialog(canvas);
	}
	return true;
}
