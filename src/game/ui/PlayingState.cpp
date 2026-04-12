#include "PlayingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Player.h"
#include "Combat.h"
#include "Hud.h"
#include "Button.h"
#include "EntityDef.h"
#include "Enums.h"

void PlayingState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	app->hud->repaintFlags = 0x2f;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	app->game->lastTurnTime = app->time;
	if (app->game->monstersTurn == 0 || canvas->oldState == Canvas::ST_CAMERA) {
		canvas->drawPlayingSoftKeys();
	}
	if (canvas->oldState == Canvas::ST_COMBAT && app->combat->curTarget != nullptr && app->combat->curTarget->def != nullptr && app->combat->curTarget->def->eType == Enums::ET_NPC) {
		app->game->executeStaticFunc(7);
	}
	canvas->updateFacingEntity = true;
	if (canvas->oldState != Canvas::ST_COMBAT && canvas->oldState != Canvas::ST_DIALOG) {
		canvas->invalidateRect();
	}
	if (CAppContainer::getInstance()->pendingEquipLevel > 0) {
		app->player->equipForLevel(CAppContainer::getInstance()->pendingEquipLevel);
		CAppContainer::getInstance()->pendingEquipLevel = 0;
	}
}

void PlayingState::onExit(Canvas* canvas) {
}

void PlayingState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	if (canvas->m_controlButton) {
		canvas->handleEvent(canvas->m_controlButton->buttonID);
	}
	app->game->updateAutomap = true;
	if (canvas->numHelpMessages == 0 && app->game->queueAdvanceTurn) {
		app->game->snapMonsters(true);
		app->game->advanceTurn();
	}
	canvas->playingState();
	if (canvas->state == Canvas::ST_PLAYING) {
		canvas->repaintFlags |= Canvas::REPAINT_HUD;
		app->hud->repaintFlags |= 0x2f;
	}
}

void PlayingState::render(Canvas* canvas, Graphics* graphics) {
	// 3D rendering handled by REPAINT_VIEW3D flag before handler dispatch
	// Swipe area and HUD rendered unconditionally for ST_PLAYING
}

bool PlayingState::handleInput(Canvas* canvas, int key, int action) {
	if (canvas->isZoomedIn) {
		return canvas->handleZoomEvents(key, action);
	}
	return canvas->handlePlayingEvents(key, action);
}
