#include "CombatState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Combat.h"
#include "Hud.h"
#include "Entity.h"
#include "AIComponent.h"
#include "MayaCamera.h"
#include "ScriptThread.h"
#include "Enums.h"

void CombatState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	app->hud->repaintFlags = 47;
	canvas->repaintFlags |= Canvas::REPAINT_HUD;
	canvas->clearSoftKeys();
	canvas->combatDone = false;
}

void CombatState::onExit(Canvas* canvas) {
	Applet* app = canvas->app;
	// Only clean up if not re-entering combat and combat stage isn't menu
	if (canvas->state != Canvas::ST_COMBAT && app->combat->stage != Canvas::ST_MENU) {
		app->combat->cleanUpAttack();
	}
}

void CombatState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	app->game->updateAutomap = true;

	// combatState() inlined
	app->game->monsterLerp();
	app->game->updateLerpSprites();
	if (canvas->combatDone) {
		if (!app->game->interpolatingMonsters) {
			if (app->combat->curAttacker == nullptr) {
				app->game->advanceTurn();
			}
			if (!app->game->isCameraActive()) {
				canvas->setState(Canvas::ST_PLAYING);
			}
			else {
				canvas->setState(Canvas::ST_CAMERA);
				app->game->activeCamera->cameraThread->run();
			}
		}
	}
	else if (app->combat->runFrame() == 0) {
		if (canvas->state == Canvas::ST_DYING || canvas->state == Canvas::ST_BOT_DYING) {
			while (app->game->combatMonsters != nullptr) {
				app->game->combatMonsters->undoAttack();
			}
			goto postCombat;
		}
		if (app->combat->curAttacker == nullptr) {
			app->game->touchTile(canvas->destX, canvas->destY, false);
			canvas->combatDone = true;
		}
		else if (canvas->knockbackDist == 0) {
			Entity* curAttacker = app->combat->curAttacker;
			if ((curAttacker->ai->goalFlags & 0x8) != 0x0) {
				curAttacker->ai->resetGoal();
				curAttacker->ai->goalType = 5;
				curAttacker->ai->goalParam = 1;
				curAttacker->aiThink(false);
			}
			Entity* nextAttacker;
			Entity* nextAttacker2;
			for (nextAttacker = curAttacker->nextAttacker; nextAttacker != nullptr && nextAttacker->ai->target == nullptr && !nextAttacker->aiIsAttackValid(); nextAttacker = nextAttacker2) {
				nextAttacker2 = nextAttacker->nextAttacker;
				nextAttacker->undoAttack();
			}
			if (nextAttacker != nullptr) {
				app->combat->performAttack(nextAttacker, nextAttacker->ai->target, 0, 0, false);
			}
			else {
				app->game->combatMonsters = nullptr;
				if (app->game->interpolatingMonsters) {
					canvas->setState(Canvas::ST_PLAYING);
				}
				else {
					app->game->endMonstersTurn();
					canvas->drawPlayingSoftKeys();
					canvas->combatDone = true;
				}
			}
		}
	}
postCombat:
	canvas->updateView();
	canvas->repaintFlags |= Canvas::REPAINT_PARTICLES;
	app->hud->repaintFlags |= 0x2B;
	if (!app->game->isCameraActive()) {
		canvas->repaintFlags |= Canvas::REPAINT_HUD;
	}
}

void CombatState::render(Canvas* canvas, Graphics* graphics) {
	// 3D rendering handled by REPAINT_VIEW3D flag before handler dispatch
	// Swipe area and HUD rendered unconditionally for ST_COMBAT
}

bool CombatState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = canvas->app;
	if (app->combat->curAttacker != nullptr && !canvas->isZoomedIn) {
		if (action == Enums::ACTION_RIGHT) {
			canvas->destAngle -= 256;
		}
		else if (action == Enums::ACTION_LEFT) {
			canvas->destAngle += 256;
		}
		canvas->startRotation(false);
	}
	if (canvas->combatDone && !app->game->interpolatingMonsters) {
		canvas->setState(Canvas::ST_PLAYING);
		if (app->combat->curAttacker == nullptr) {
			app->game->advanceTurn();
			if (canvas->state == Canvas::ST_PLAYING) {
				if (canvas->isZoomedIn) {
					return canvas->handleZoomEvents(key, action);
				}
				return canvas->handlePlayingEvents(key, action);
			}
		}
	}
	return true;
}
