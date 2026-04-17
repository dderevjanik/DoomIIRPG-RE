#include "PlayingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "DialogManager.h"
#include "Canvas.h"
#include "Game.h"
#include "Player.h"
#include "Combat.h"
#include "Hud.h"
#include "Render.h"
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
	if (app->dialogManager->numHelpMessages == 0 && app->game->queueAdvanceTurn) {
		app->game->snapMonsters(true);
		app->game->advanceTurn();
	}
	// playingState() inlined
	if (canvas->pushedWall && canvas->pushedTime <= app->gameTime) {
		app->combat->shiftWeapon(false);
		canvas->pushedWall = false;
	}
	if (app->player->ce->getStat(0) <= 0) {
		app->player->died();
		return;
	}
	if (app->player->isFamiliar && app->player->ammo[app->combat->familiarAmmoType] <= 0) {
		app->player->ammo[app->combat->familiarAmmoType] = 0;
		app->player->familiarDying(false);
	}
	if (app->hud->isShiftingCenterMsg()) {
		canvas->staleView = true;
	}
	if (canvas->knockbackDist == 0 && app->game->activePropogators == 0 && app->game->animatingEffects == 0 && app->game->monstersTurn != 0 && app->dialogManager->numHelpMessages == 0) {
		app->game->updateMonsters();
	}
	app->game->updateLerpSprites();
	canvas->updateView();
	if (canvas->state == Canvas::ST_LOADING || canvas->state == Canvas::ST_SAVING) {
		return;
	}
	if (canvas->state != Canvas::ST_PLAYING && canvas->state != Canvas::ST_INTER_CAMERA) {
		return;
	}
	canvas->repaintFlags |= Canvas::REPAINT_PARTICLES;
	if (!app->game->isCameraActive() || canvas->state == Canvas::ST_INTER_CAMERA) {
		canvas->repaintFlags |= Canvas::REPAINT_HUD;
		app->hud->repaintFlags |= 0x2B;
		app->hud->update();
	}
	if (canvas->state == Canvas::ST_INTER_CAMERA || (!app->game->isCameraActive() && canvas->state == Canvas::ST_PLAYING) || canvas->state == Canvas::ST_AUTOMAP) {
		app->dialogManager->dequeueHelpDialog(canvas);
	}
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
