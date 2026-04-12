#include "MiniGameState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "IMinigame.h"

void MiniGameState::onEnter(Canvas* canvas) {
}

void MiniGameState::onExit(Canvas* canvas) {
}

void MiniGameState::update(Canvas* canvas) {
	// Minigame update is driven by render (updateGame called in render)
}

void MiniGameState::render(Canvas* canvas, Graphics* graphics) {
	IMinigame* mg = CAppContainer::getInstance()->minigameRegistry.getById(canvas->stateVars[0]);
	if (mg) mg->updateGame(graphics);
}

bool MiniGameState::handleInput(Canvas* canvas, int key, int action) {
	IMinigame* mg = CAppContainer::getInstance()->minigameRegistry.getById(canvas->stateVars[0]);
	if (mg) mg->handleInput(action);
	return true;
}
