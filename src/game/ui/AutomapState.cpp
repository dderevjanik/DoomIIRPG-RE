#include "AutomapState.h"
#include "AutomapRenderer.h"
#include "Canvas.h"
#include "CAppContainer.h"
#include "App.h"
#include "Game.h"
#include "Player.h"
#include "Hud.h"
#include "Render.h"
#include "Graphics.h"
#include "Image.h"
#include "Text.h"
#include "Button.h"
#include "Entity.h"
#include "EntityDef.h"
#include "Enums.h"

static void drawAutomap(Canvas* canvas, Graphics* graphics, bool b) {
	Applet* app = canvas->app;
	const auto& gc = *canvas->gameConfig;
	graphics->drawRegion(canvas->imgGameHelpBG, 0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0, 0, 0, 0, 0);

	AutomapRenderParams params;
	params.originX = gc.automapOffsetX + canvas->screenRect[0];
	params.originY = gc.automapOffsetY + canvas->screenRect[1];
	params.cellSize = gc.automapCellSize;
	params.showCursor = true;
	params.showQuestMarkers = true;
	params.showEntities = true;
	params.showWalls = true;
	renderAutomapContent(graphics, app, params);

	Text* LargeBuffer = app->localization->getLargeBuffer();
	LargeBuffer->setLength(0);
	app->localization->composeText(canvas->softKeyLeftID, LargeBuffer);
	LargeBuffer->dehyphenate();
	app->setFontRenderMode(2);
	if (canvas->m_softKeyButtons->GetButton(19)->highlighted) {
		app->setFontRenderMode(0);
	}
	graphics->drawString(LargeBuffer, 15, 310, 36);
	app->setFontRenderMode(0);

	LargeBuffer->setLength(0);
	app->localization->composeText(canvas->softKeyRightID, LargeBuffer);
	LargeBuffer->dehyphenate();
	app->setFontRenderMode(2);
	if (canvas->m_softKeyButtons->GetButton(20)->highlighted) {
		app->setFontRenderMode(0);
	}
	graphics->drawString(LargeBuffer, 465, 310, 40);
	app->setFontRenderMode(0);
	LargeBuffer->dispose();
}

void AutomapState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	canvas->automapDrawn = false;
	canvas->automapTime = app->time;
}

void AutomapState::onExit(Canvas* canvas) {
	Applet* app = canvas->app;
	app->player->unpause(app->time - canvas->automapTime);
}

void AutomapState::update(Canvas* canvas) {
	Applet* app = canvas->app;
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
	Applet* app = canvas->app;
	drawAutomap(canvas, graphics, !canvas->automapDrawn);
	canvas->m_softKeyButtons->Render(graphics);
	app->hud->drawArrowControls(graphics);
	canvas->automapDrawn = true;
}

bool AutomapState::handleInput(Canvas* canvas, int key, int action) {
	if (key == 18) {
		return true;
	}
	canvas->automapDrawn = false;
	return canvas->handlePlayingEvents(key, action);
}
