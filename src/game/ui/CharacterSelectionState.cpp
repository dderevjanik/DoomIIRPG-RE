#include "CharacterSelectionState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"

void CharacterSelectionState::onEnter(Canvas* canvas) {
	canvas->clearSoftKeys();
	canvas->setupCharacterSelection();
	canvas->stateVars[0] = 1;
	canvas->stateVars[1] = 0;
	canvas->stateVars[2] = 1;
	canvas->stateVars[8] = 0;
}

void CharacterSelectionState::onExit(Canvas* canvas) {
}

void CharacterSelectionState::update(Canvas* canvas) {
}

void CharacterSelectionState::render(Canvas* canvas, Graphics* graphics) {
	canvas->drawCharacterSelection(graphics);
}

bool CharacterSelectionState::handleInput(Canvas* canvas, int key, int action) {
	canvas->handleCharacterSelectionInput(key, action);
	return true;
}
