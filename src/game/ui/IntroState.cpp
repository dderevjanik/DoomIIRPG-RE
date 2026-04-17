#include "IntroState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "StoryRenderer.h"

void IntroState::onEnter(Canvas* canvas) {
	canvas->clearSoftKeys();
	canvas->app->storyRenderer->loadPrologueText(canvas);
	canvas->stateVars[0] = 1;
}

void IntroState::onExit(Canvas* canvas) {
}

void IntroState::update(Canvas* canvas) {
	StoryRenderer* sr = canvas->app->storyRenderer.get();
	if (sr->storyPage >= sr->storyTotalPages) {
		sr->disposeIntro(canvas);
	}
}

void IntroState::render(Canvas* canvas, Graphics* graphics) {
	canvas->app->storyRenderer->drawStory(canvas, graphics);
}

bool IntroState::handleInput(Canvas* canvas, int key, int action) {
	canvas->app->storyRenderer->handleStoryInput(canvas, key, action);
	return true;
}
