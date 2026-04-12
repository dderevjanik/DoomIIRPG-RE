#include "LoadingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Hud.h"

void LoadingState::onEnter(Canvas* canvas) {
	canvas->repaintFlags &= ~Canvas::REPAINT_HUD;
	canvas->pacifierX = canvas->SCR_CX - 66;
	canvas->updateLoadingBar(false);
}

void LoadingState::onExit(Canvas* canvas) {
}

void LoadingState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	if (canvas->loadType == 0) {
		if (!canvas->loadMedia()) {
			canvas->flushGraphics();
			return;
		}
	}
	else {
		app->game->loadState(canvas->loadType);
		app->hud->addMessage((short)0, (short)39);
		canvas->loadType = 0;
	}
}

void LoadingState::render(Canvas* canvas, Graphics* graphics) {
	// Loading bar rendering handled by the main pipeline
}

bool LoadingState::handleInput(Canvas* canvas, int key, int action) {
	return true;
}
