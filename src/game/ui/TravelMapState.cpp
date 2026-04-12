#include "TravelMapState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"

void TravelMapState::onEnter(Canvas* canvas) {
	if (CAppContainer::getInstance()->skipTravelMap) {
		canvas->setLoadingBarText((short)0, (short)41);
		canvas->setState(Canvas::ST_LOADING);
		return;
	}
	canvas->initTravelMap();
}

void TravelMapState::onExit(Canvas* canvas) {
}

void TravelMapState::update(Canvas* canvas) {
}

void TravelMapState::render(Canvas* canvas, Graphics* graphics) {
	canvas->drawTravelMap(graphics);
}

bool TravelMapState::handleInput(Canvas* canvas, int key, int action) {
	canvas->handleTravelMapInput(key, action);
	return true;
}
