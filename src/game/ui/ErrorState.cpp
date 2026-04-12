#include "ErrorState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Enums.h"

void ErrorState::onEnter(Canvas* canvas) {
}

void ErrorState::onExit(Canvas* canvas) {
}

void ErrorState::update(Canvas* canvas) {
}

void ErrorState::render(Canvas* canvas, Graphics* graphics) {
}

bool ErrorState::handleInput(Canvas* canvas, int key, int action) {
	if (action == Enums::ACTION_FIRE || key == 18) {
		canvas->app->shutdown();
	}
	return true;
}
