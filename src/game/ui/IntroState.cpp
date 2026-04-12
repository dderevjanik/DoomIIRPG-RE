#include "IntroState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"

void IntroState::onEnter(Canvas* canvas) {
	canvas->clearSoftKeys();
	canvas->loadPrologueText();
	canvas->stateVars[0] = 1;
}

void IntroState::onExit(Canvas* canvas) {
}

void IntroState::update(Canvas* canvas) {
	if (canvas->storyPage >= canvas->storyTotalPages) {
		canvas->disposeIntro();
	}
}

void IntroState::render(Canvas* canvas, Graphics* graphics) {
	canvas->drawStory(graphics);
}

bool IntroState::handleInput(Canvas* canvas, int key, int action) {
	canvas->handleStoryInput(key, action);
	return true;
}
