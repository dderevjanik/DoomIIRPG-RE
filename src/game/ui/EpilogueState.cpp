#include "EpilogueState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Player.h"
#include "Sound.h"
#include "StoryRenderer.h"

void EpilogueState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	app->player->levelGrade(true);
	bool soundEnabled = app->sound->allowSounds;
	app->sound->allowSounds = canvas->areSoundsAllowed;
	app->sound->allowSounds = soundEnabled;
	canvas->clearSoftKeys();
	app->storyRenderer->loadEpilogueText(canvas);
	canvas->stateVars[0] = 1;
}

void EpilogueState::onExit(Canvas* canvas) {
}

void EpilogueState::update(Canvas* canvas) {
	if (canvas->app->storyRenderer->scrollingTextDone) {
		canvas->app->storyRenderer->disposeEpilogue(canvas);
	}
}

void EpilogueState::render(Canvas* canvas, Graphics* graphics) {
	canvas->app->storyRenderer->drawScrollingText(canvas, graphics);
}

bool EpilogueState::handleInput(Canvas* canvas, int key, int action) {
	if (key == 18) {
		canvas->app->storyRenderer->disposeEpilogue(canvas);
	}
	return true;
}
