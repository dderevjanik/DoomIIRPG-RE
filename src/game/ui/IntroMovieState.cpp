#include "IntroMovieState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Text.h"
#include "Enums.h"

void IntroMovieState::onEnter(Canvas* canvas) {
	canvas->initScrollingText((short)0, (short)133, 0, 32, 1, 800);
	canvas->stateVars[0] = 1;
}

void IntroMovieState::onExit(Canvas* canvas) {
}

void IntroMovieState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	if ((app->game->hasSeenIntro && canvas->numEvents != 0) || canvas->scrollingTextDone) {
		canvas->dialogBuffer->dispose();
		canvas->dialogBuffer = nullptr;
		app->game->hasSeenIntro = true;
		app->game->saveConfig();
		canvas->backToMain(false);
	}
}

void IntroMovieState::render(Canvas* canvas, Graphics* graphics) {
	canvas->playIntroMovie(graphics);
}

bool IntroMovieState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = canvas->app;
	if (key == 18) {
		canvas->exitIntroMovie(true);
		app->shutdown();
		return true;
	}
	app->game->skipMovie = (action == Enums::ACTION_FIRE);
	return true;
}
