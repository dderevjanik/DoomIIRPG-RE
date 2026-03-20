#include "AutomapState.h"
#include "Canvas.h"
#include "CAppContainer.h"
#include "App.h"
#include "Game.h"
#include "Player.h"
#include "Hud.h"
#include "Graphics.h"
#include "Button.h"

void AutomapState::onEnter(Canvas* canvas) {
	Applet* app = CAppContainer::getInstance()->app;
	canvas->automapDrawn = false;
	canvas->automapTime = app->time;
}

void AutomapState::onExit(Canvas* canvas) {
	Applet* app = CAppContainer::getInstance()->app;
	app->player->unpause(app->time - canvas->automapTime);
}

void AutomapState::update(Canvas* canvas) {
	Applet* app = CAppContainer::getInstance()->app;
	app->game->updateLerpSprites();
	if (!canvas->automapDrawn && app->game->animatingEffects == 0) {
		canvas->updateView();
		canvas->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
		if (canvas->state != Canvas::ST_AUTOMAP) {
			canvas->updateView();
		}
	}
	if (canvas->state == Canvas::ST_AUTOMAP || canvas->state == Canvas::ST_PLAYING) {
		canvas->drawPlayingSoftKeys();
	}
}

void AutomapState::render(Canvas* canvas, Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;
	canvas->drawAutomap(graphics, !canvas->automapDrawn);
	canvas->m_softKeyButtons->Render(graphics);
	app->hud->drawArrowControls(graphics);
	canvas->automapDrawn = true;
}

bool AutomapState::handleInput(Canvas* canvas, int key, int action) {
	if (key == 18) { // Back button
		return true;
	}
	canvas->automapDrawn = false;
	return canvas->handlePlayingEvents(key, action);
}
