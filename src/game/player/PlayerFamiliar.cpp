#include <stdexcept>
#include <algorithm>
#include <map>
#include <string>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "Player.h"
#include "Combat.h"
#include "Canvas.h"
#include "Render.h"
#include "Game.h"
#include "Text.h"
#include "Hud.h"
#include "Enums.h"
#include "TinyGL.h"
#include "JavaStream.h"
#include "Sound.h"
#include "DataNode.h"
#include "SoundNames.h"
#include "Sounds.h"
#include "ItemDefs.h"

void Player::setFamiliar(short familiarType) {


	if (app->canvas->state == Canvas::ST_AUTOMAP) {
		return;
	}
	app->game->scriptStateVars[16] = 1;
	app->canvas->saveX = app->canvas->viewX;
	app->canvas->saveY = app->canvas->viewY;
	app->canvas->saveZ = app->canvas->viewZ;
	app->canvas->saveAngle = app->canvas->viewAngle;
	app->canvas->savePitch = app->canvas->viewPitch;
	this->isFamiliar = true;
	this->setFamiliarType(familiarType);
	app->game->updateScriptVars();
	if (!app->canvas->attemptMove(app->canvas->viewX + app->canvas->viewStepX,
	                              app->canvas->viewY + app->canvas->viewStepY)) {
		app->game->scriptStateVars[16] = 0;
		this->isFamiliar = false;
		this->setFamiliarType((short)0);
		app->game->updateScriptVars();
		app->hud->addMessage((short)0, (short)190, 3);
		return;
	}
	app->render->savePlayerFog();
	app->render->startFade(400, 2);
	this->playerEntityCopyIndex =
	    app->game->spawnPlayerEntityCopy((short)app->canvas->saveX, (short)app->canvas->saveY)->getIndex();
	this->weaponsCopy = 0;
	for (int i = 0; i < 9; ++i) {
		this->ammoCopy[i] = 0;
	}
	for (int j = 0; j < 26; ++j) {
		this->inventoryCopy[j] = 0;
	}
	this->clearOutFamiliarsStatusEffects();
	this->swapStatusEffects();
	const Combat::FamiliarDef* famDefHelp = app->combat->getFamiliarDefByType(this->familiarType);
	bool b = this->showHelp(famDefHelp ? famDefHelp->helpId : (short)12, false);
	if (!b) {
		app->canvas->drawPlayingSoftKeys();
	}
	app->sound->playSound(Sounds::getResIDByName(SoundName::SENTRYBOT_ACTIVATE), 0, 3, false);
}

short Player::unsetFamiliar(bool b) {


	app->hud->stopBrightenScreen();
	app->hud->stopScreenSmack();
	int saveAngle = app->canvas->saveAngle;
	if (b) {
		if (app->canvas->saveX > app->canvas->viewX) {
			saveAngle = Enums::ANGLE_WEST;
		} else if (app->canvas->saveX < app->canvas->viewX) {
			saveAngle = Enums::ANGLE_EAST;
		} else if (app->canvas->saveY > app->canvas->viewY) {
			saveAngle = Enums::ANGLE_NORTH;
		} else if (app->canvas->saveY < app->canvas->viewY) {
			saveAngle = Enums::ANGLE_SOUTH;
		}
	}
	app->canvas->destAngle = (app->canvas->viewAngle = saveAngle);
	app->canvas->destPitch = (app->canvas->viewPitch = app->canvas->savePitch);
	if (!b) {
		app->canvas->destX = app->canvas->saveX;
		app->canvas->destY = app->canvas->saveY;
		app->canvas->destZ = app->canvas->saveZ;
		app->canvas->viewX = app->canvas->destX;
		app->canvas->viewY = app->canvas->destY;
		app->canvas->viewZ = app->canvas->destZ;
	}
	this->swapStatusEffects();
	if (!b) {
		app->render->loadPlayerFog();
	}
	app->render->startFade(400, 2);
	app->game->removeEntity(&app->game->entities[this->playerEntityCopyIndex]);
	this->playerEntityCopyIndex = -1;
	this->unlink();
	app->canvas->finishRotation(true);
	this->relink();
	this->isFamiliar = false;
	short familiarType = this->familiarType;
	this->setFamiliarType((short)0);
	app->game->updateScriptVars();
	app->canvas->drawPlayingSoftKeys();
	return familiarType;
}

void Player::setFamiliarType(short familiarType) {
	this->familiarType = familiarType;
	this->calcViewMode();
}

void Player::calcViewMode() {


	if (this->familiarType == 0) {
		app->render->postProcessMode = 0;
	} else {
		const Combat::FamiliarDef* famDef = app->combat->getFamiliarDefByType(this->familiarType);
		app->render->postProcessMode = famDef ? famDef->postProcess : 0;
	}
}

void Player::familiarDying(bool familiarSelfDestructed) {


	if (app->canvas->state == Canvas::ST_BOT_DYING) {
		return;
	}

	if (familiarSelfDestructed) {
		app->combat->curAttacker = nullptr;
	} else {
		app->sound->playSound(Sounds::getResIDByName(SoundName::SENTRYBOT_DEATH), 0, 3, false);
		app->canvas->startShake(350, 5, 500);
	}

	app->canvas->familiarSelfDestructed = familiarSelfDestructed;
	app->canvas->setState(Canvas::ST_BOT_DYING);
}

void Player::familiarDied() {


	if (app->combat->curAttacker != nullptr) {
		int sprite = app->combat->curAttacker->getSprite();
		if (app->combat->curAttacker->def->eType == Enums::ET_MONSTER) {
			app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x0);
		}
		app->localization->resetTextArgs();
		if (app->combat->accumRoundDamage > 0) {
			app->localization->addTextArg((short)1, (short)(app->combat->curAttacker->def->name & 0x3FF));
			app->localization->addTextArg(app->combat->accumRoundDamage);
			app->hud->addMessage((short)0, (short)112);
		}
		app->canvas->shakeTime = 0;
		app->hud->damageDir = 0;
		app->hud->damageTime = 0;
		app->combat->curAttacker->monster->flags |= 0x400;
		app->game->gsprite_clear(64);
		app->canvas->invalidateRect();
	}
	int viewX = app->canvas->viewX;
	int viewY = app->canvas->viewY;
	short unsetFamiliar = this->unsetFamiliar(false);
	app->hud->addMessage((short)0, (short)192, 2);
	const Combat::FamiliarDef* famDef = app->combat->getFamiliarDefByType(unsetFamiliar);
	if (famDef && famDef->explodes) {
		this->explodeFamiliar(viewX >> 6, viewY >> 6, unsetFamiliar);
	}
	int n = famDef ? famDef->deathRemainsWeapon : 3;
	if (this->noFamiliarRemains) {
		this->noFamiliarRemains = false;
	} else {
		this->handleBotRemains(viewX, viewY, n);
	}
	this->give(1, n, -1, true);
}

void Player::familiarReturnsToPlayer(bool b) {

	app->sound->playSound(Sounds::getResIDByName(SoundName::SENTRYBOT_RETURN), 0, 3, false);

	this->unsetFamiliar(b);
	this->tookBotsInventory = this->stealFamiliarsInventory();
}

void Player::forceFamiliarReturnDueToMonster() {
	this->familiarReturnsToPlayer(false);
	this->botReturnedDueToMonster = true;
}

void Player::explodeFamiliar(int n, int n2, int n3) {


	const Combat::FamiliarDef* famDef = app->combat->getFamiliarDefByType(n3);
	app->combat->attackerWeaponId = (famDef && famDef->explodeWeaponIndex >= 0) ? famDef->explodeWeaponIndex : 4;
	int n4 = app->combat->attackerWeaponId * 9;

	int n5 = app->combat->weapons[n4 + 0] & 0xFF;
	int n6 = app->combat->weapons[n4 + 1] & 0xFF;
	if (n5 != n6) {
		n5 += app->nextByte() % (n6 - n5);
	}
	app->combat->radiusHurtEntities(n, n2, 0, n5, this->getPlayerEnt(), nullptr);
}

bool Player::stealFamiliarsInventory() {
	int weaponsCopy = this->weaponsCopy;
	short* inventoryCopy = this->inventoryCopy;
	short* ammoCopy = this->ammoCopy;

	bool b = false;
	for (int i = 0; i < 26; ++i) {
		if (inventoryCopy[i] != 0) {
			this->give(0, i, inventoryCopy[i], true);
			b = true;
		}
	}
	for (int j = 0; j < 9; ++j) {
		if (ammoCopy[j] != 0) {
			this->give(2, j, ammoCopy[j], true);
			b = true;
		}
	}
	for (int k = 0; k < 15; ++k) {
		if ((1 << k & weaponsCopy) != 0x0) {
			this->give(1, k, 1, true);
			b = true;
		}
	}
	return b;
}

void Player::handleBotRemains(int n, int n2, int n3) {

	int weaponsCopy = this->weaponsCopy;
	short* inventoryCopy = this->inventoryCopy;
	short* ammoCopy = this->ammoCopy;
	short n4 = 0;
	short n5 = 0;
	for (int i = 0; i < 26; ++i) {
		if (inventoryCopy[i] != 0) {
			if (i == 24 || (i >= 0 && i < 11)) {
				short n6 = inventoryCopy[i];
				this->give(0, i, n6, true, true);
				EntityDef* find = app->entityDefManager->find(6, 0, i);
				Text* messageBuffer = app->hud->getMessageBuffer();
				app->localization->resetTextArgs();
				app->localization->addTextArg(n6);
				Text* smallBuffer = app->localization->getSmallBuffer();
				app->localization->composeText((short)1, find->longName, smallBuffer);
				app->localization->addTextArg(smallBuffer);
				app->localization->composeText((short)0, (short)86, messageBuffer);
				smallBuffer->dispose();
				app->hud->finishMessageBuffer();
			} else {
				switch (i) {
					case 13: {
						n5 += inventoryCopy[i];
						break;
					}
					case 16: {
						n4 += inventoryCopy[i];
						break;
					}
					case 11:
					case 12:
					case 17:
					case 19:
					case 20: {
						EntityDef* find = app->entityDefManager->find(6, 0, i);
						app->game->spawnDropItem(n, n2, find->tileIndex, find, inventoryCopy[i], true);
						break;
					}
				}
			}
		}
	}
	for (int j = 1; j < 9; ++j) {
		if (ammoCopy[j] != 0) {
			switch (j) {
				case 1:
				case 2:
				case 4:
				case 5: {
					EntityDef* find = app->entityDefManager->find(6, 2, j);
					app->game->spawnDropItem(n, n2, find->tileIndex, find, ammoCopy[j], true);
					break;
				}
			}
		}
	}
	for (int k = 0; k < 15; ++k) {
		if ((1 << k & weaponsCopy) != 0x0) {
			int weaponTileNum = app->combat->getWeaponTileNum(k);
			app->game->spawnDropItem(n, n2, weaponTileNum, app->entityDefManager->lookup(weaponTileNum), 1, true);
		}
	}
	app->game->spawnSentryBotCorpse(n, n2, n3, n4, n5);
}

void Player::attemptToDeploySentryBot() {


	if (app->game->activeMonsters != nullptr) {
		Entity* activeMonsters = app->game->activeMonsters;
		do {
			Entity* nextOnList = activeMonsters->monster->nextOnList;
			if (activeMonsters->distFrom(app->canvas->destX, app->canvas->destY) <= app->combat->tileDistances[0] &&
			    activeMonsters->aiIsAttackValid()) {
				app->hud->addMessage((short)0, (short)219, 3);
				return;
			}
			activeMonsters = nextOnList;
		} while (activeMonsters != app->game->activeMonsters && app->game->activeMonsters != nullptr);
	}
	if ((app->render->mapFlags[(app->canvas->viewY + app->canvas->viewStepY >> 6) * 32 +
	                           (app->canvas->viewX + app->canvas->viewStepX >> 6)] &
	     0x10) != 0x0) {
		app->hud->addMessage((short)0, (short)217, 3);
		return;
	}
	const Combat::FamiliarDef* famDef = app->combat->getFamiliarDefByWeapon(this->ce->weapon);
	short familiar = famDef ? famDef->familiarType : (short)app->combat->defaultFamiliarType;
	this->setFamiliar(familiar);
}

void Player::attemptToDiscardFamiliar(int n) {


	if ((app->render->mapFlags[(app->canvas->viewY >> 6) * 32 + (app->canvas->viewX >> 6)] & 0x20) != 0x0) {
		app->hud->addMessage((short)0, (short)221, 3);
	} else {
		app->game->spawnDropItem(app->canvas->viewX, app->canvas->viewY, app->combat->getWeaponTileNum(n), 6, 1, n,
		                         this->ammo[app->combat->familiarAmmoType], false);
		this->give(1, n, -1);
	}
}

void Player::startSelfDestructDialog() {


	if (app->canvas->state != Canvas::ST_AUTOMAP) {
		this->attemptingToSelfDestructFamiliar = true;
		Text* smallBuffer = app->localization->getSmallBuffer();
		app->localization->composeText((short)0, (short)194, smallBuffer);
		app->canvas->startDialog(nullptr, smallBuffer, 12, 1, false);
		smallBuffer->dispose();
	}
}

bool Player::weaponIsASentryBot(int n) {
	return app->combat->getFamiliarDefByWeapon(n) != nullptr;
}

bool Player::hasASentryBot() {
	for (int i = 0; i < app->combat->familiarDefCount; i++) {
		int wpIdx = app->combat->familiarDefs[i].weaponIndex;
		if ((this->weapons & (1 << wpIdx)) != 0)
			return true;
	}
	return false;
}
