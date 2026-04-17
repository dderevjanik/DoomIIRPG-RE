#include <stdexcept>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <sys/stat.h>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "Hud.h"
#include "Text.h"
#include "Game.h"
#include "Sound.h"
#include "Entity.h"
#include "Player.h"
#include "Combat.h"
#include "Canvas.h"
#include "Render.h"
#include "TinyGL.h"
#include "Resource.h"
#include "MayaCamera.h"
#include "LerpSprite.h"
#include "JavaStream.h"
#include "MenuSystem.h"
#include "ParticleSystem.h"
#include "Enums.h"
#include "Utils.h"
#include "SDLGL.h"
#include "Input.h"
#include "GLES.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include "SoundNames.h"
#include "Sounds.h"
#include "ConfigEnums.h"

void Game::prepareMonsters() {


	int n = app->canvas->loadMapID - 1;
	app->player->fillMonsterStats();
	if (!this->isLoaded && 0x0 != (app->player->completedLevels & 1 << n) &&
	    app->player->monsterStats[0] == app->player->monsterStats[1]) {
		int n2 = 0;
		int n3 = app->player->monsterStats[1];
		int n4 = n3 - app->player->monsterStats[0];
		for (int i = 0; i < this->numEntities; ++i) {
			Entity* entity = &this->entities[i];
			if (entity->isMonster()) {
				int sprite = entity->getSprite();
				EntityMonster* monster = entity->monster;
				if (0x0 != (entity->info & 0x1010000) && 0x0 == (entity->monsterFlags & (Enums::MFLAG_NOTRACK | Enums::MFLAG_NORESPAWN)) && n4 < n3 / 2 && n2 < 8 &&
				    app->nextByte() <= 100) {
					Render* render = app->render.get();
					if (this->findMapEntity(render->getSpriteX(sprite),
					                        render->getSpriteY(sprite), 15535) == nullptr) {
						entity->resurrect(render->getSpriteX(sprite),
						                  render->getSpriteY(sprite), 32);
						entity->info &= 0xFFBFFFFF;
						++n4;
						++n2;
					}
				}
			}
		}
	}
}

void Game::activate(Entity* entity) {
	this->activate(entity, true, false, true, false);
}

void Game::activate(Entity* entity, bool b, bool b2, bool b3, bool b4) {


	EntityMonster* monster = entity->monster;
	if ((app->render->getSpriteFrame(entity->getSprite()) & 0xF0) == 16 && !app->render->shotsFired) {
		return;
	}
	if (b2 && entity->distFrom(app->canvas->viewX, app->canvas->viewY) > app->combat->tileDistances[3]) {
		return;
	}
	entity->info |= 0x400000;
	if (app->player->noclip) {
		return;
	}
	if ((entity->info & 0x40000) != 0x0) {
		return;
	}
	int sprite = entity->getSprite();
	app->render->setSpriteFrame(sprite, 0);
	if (entity->nextOnList != nullptr) {
		if (entity == this->inactiveMonsters && entity->nextOnList == this->inactiveMonsters) {
			this->inactiveMonsters = nullptr;
		} else {
			if (entity == this->inactiveMonsters) {
				this->inactiveMonsters = entity->nextOnList;
			}
			entity->nextOnList->prevOnList = entity->prevOnList;
			entity->prevOnList->nextOnList = entity->nextOnList;
		}
	}
	if (this->activeMonsters == nullptr) {
		entity->nextOnList = entity;
		entity->prevOnList = entity;
		this->activeMonsters = entity;
	} else {
		entity->prevOnList = this->activeMonsters->prevOnList;
		entity->nextOnList = this->activeMonsters;
		this->activeMonsters->prevOnList->nextOnList = entity;
		this->activeMonsters->prevOnList = entity;
	}
	entity->info |= 0x40000;
	entity->monsterFlags &= ~Enums::MFLAG_NOACTIVATE;
	if (b && 0x0 != (entity->monsterFlags & Enums::MFLAG_TRIGGERONACTIVATE)) {
		this->executeStaticFunc(9);
		entity->monsterFlags &= ~Enums::MFLAG_TRIGGERONACTIVATE;
	}
	if (app->canvas->state == Canvas::ST_PLAYING && b3) {
		int MonsterSound = this->getMonsterSound(entity->def->monsterIdx, Enums::MSOUND_ALERT1);
		app->sound->playSound(MonsterSound, 0, 3, 0);
	}
}

void Game::killAll() {
	for (int i = 0; i < this->numEntities; ++i) {
		Entity* entity = &this->entities[i];
		if (entity->def != nullptr && entity->def->eType == Enums::ET_MONSTER) {
			if (!entity->isBoss()) {
				if ((entity->info & 0x20000) != 0x0) {
					if ((entity->monsterFlags & Enums::MFLAG_NOTRACK) == 0x0) {
						entity->died(false, nullptr);
					}
				}
			}
		}
	}
}

void Game::deactivate(Entity* entity) {
	EntityMonster* monster = entity->monster;
	if ((entity->info & 0x40000) == 0x0) {
		return;
	}
	if (entity->nextOnList != nullptr) {
		if (entity == this->activeMonsters && entity->nextOnList == this->activeMonsters) {
			this->activeMonsters = nullptr;
		} else {
			if (entity == this->activeMonsters) {
				this->activeMonsters = entity->nextOnList;
			}
			entity->nextOnList->prevOnList = entity->prevOnList;
			entity->prevOnList->nextOnList = entity->nextOnList;
		}
	}
	if (this->inactiveMonsters == nullptr) {
		entity->nextOnList = entity;
		entity->prevOnList = entity;
		this->inactiveMonsters = entity;
	} else {
		entity->prevOnList = this->inactiveMonsters->prevOnList;
		entity->nextOnList = this->inactiveMonsters;
		this->inactiveMonsters->prevOnList->nextOnList = entity;
		this->inactiveMonsters->prevOnList = entity;
	}
	entity->info &= ~0x40000;
}

void Game::monsterAI() {


	if (this->disableAI) {
		return;
	}
	if (this->monstersUpdated) {
		return;
	}
	this->monstersUpdated = true;
	this->combatMonsters = nullptr;
	if (this->activeMonsters != nullptr) {
		int i = 0;
		Entity* activeMonsters = this->activeMonsters;
		do {
			Entity* nextOnList = activeMonsters->nextOnList;
			if (0x0 == (activeMonsters->monsterEffects & 0x2) &&
			    (this->monstersTurn == 1 || (this->monstersTurn == 2 && activeMonsters->isHasteResistant()))) {
				activeMonsters->aiThink(false);
				if ((activeMonsters->ai->goalFlags & 0x1) != 0x0) {
					i = 1;
				}
			}
			activeMonsters = nextOnList;
		} while (activeMonsters != this->activeMonsters && this->activeMonsters != nullptr);
		while (i != 0) {
			i = 0;
			Entity* nextAttacker;
			for (Entity* combatMonsters = this->combatMonsters; combatMonsters != nullptr;
			     combatMonsters = nextAttacker) {
				nextAttacker = combatMonsters->nextAttacker;
				if (!combatMonsters->aiIsAttackValid()) {
					combatMonsters->undoAttack();
					combatMonsters->aiThink(false);
					if ((combatMonsters->ai->goalFlags & 0x1) != 0x0) {
						i = 1;
					}
				}
			}
		}
	}
}

void Game::monsterLerp() {


	bool interpolatingMonsters = false;
	int n = 0;
	if (!this->interpolatingMonsters) {
		return;
	}
	if (this->activeMonsters != nullptr) {
		Entity* entity = this->activeMonsters;
		do {
			if ((entity->ai->goalFlags & 0x1) != 0x0) {
				interpolatingMonsters = true;
				if (n == 0 && app->render->checkTileVisibilty(entity->ai->goalX, entity->ai->goalY)) {
					n = 1;
				}
			}
			entity = entity->nextOnList;
		} while (entity != this->activeMonsters);
	}
	if (interpolatingMonsters && n != 0) {
		app->canvas->invalidateRect();
	}
	this->interpolatingMonsters = interpolatingMonsters;
}

bool Game::canSnapMonsters() {
	if (this->disableAI) {
		return true;
	}
	if (this->activeMonsters != nullptr) {
		Entity* entity = this->activeMonsters;
		while (0x0 == (entity->info & 0x10000000)) {
			entity = entity->nextOnList;
			if (entity == this->activeMonsters) {
				return true;
			}
		}
		return false;
	}
	return true;
}

bool Game::snapMonsters(bool b) {


	if (this->animatingEffects != 0) {
		for (int i = 0; i < 16; ++i) {
			LerpSprite* lerpSprite = &this->lerpSprites[i];
			if (lerpSprite->hSprite != 0) {
				if ((lerpSprite->flags & Enums::LS_FLAG_ANIMATING_EFFECT) != 0x0) {
					short n = app->render->getSpriteEnt(lerpSprite->hSprite - 1);
					if (n == -1) {
						return false;
					}
					if (this->entities[n].def->eType != Enums::ET_MONSTER) {
						return false;
					}
				}
			}
		}
	}
	bool b2 = b;
	if (this->monstersTurn == 0) {
		if (!b) {
			b2 = this->canSnapMonsters();
		}
		Entity* activeMonsters = this->activeMonsters;
		if (b2 && activeMonsters != nullptr) {
			do {
				Entity* nextOnList = activeMonsters->nextOnList;
				this->snapLerpSprites(activeMonsters->getSprite());
				activeMonsters = nextOnList;
			} while (activeMonsters != this->activeMonsters);
			this->interpolatingMonsters = false;
		} else if (activeMonsters == nullptr) {
			this->interpolatingMonsters = false;
		}
		app->canvas->updateFacingEntity = true;
		app->canvas->invalidateRect();
		return b2;
	}
	this->monsterAI();
	if (!b) {
		b2 = this->canSnapMonsters();
	}
	if (b2 && this->activeMonsters != nullptr) {
		Entity* entity = this->activeMonsters;
		do {
			this->snapLerpSprites(entity->getSprite());
			entity = entity->nextOnList;
		} while (entity != this->activeMonsters);
		app->canvas->updateFacingEntity = true;
		app->canvas->invalidateRect();
	}
	this->monsterLerp();
	if (this->combatMonsters != nullptr && !this->interpolatingMonsters) {
		app->combat->performAttack(this->combatMonsters, this->combatMonsters->ai->target, 0, 0, false);
		return false;
	}
	return b2;
}

void Game::endMonstersTurn() {

	app->canvas->startRotation(true);
	this->monstersTurn = 0;
}

void Game::updateMonsters() {


	this->monsterAI();
	this->monsterLerp();
	if (!this->interpolatingMonsters) {
		app->canvas->updateFacingEntity = true;
		if (this->combatMonsters != nullptr) {
			app->combat->performAttack(this->combatMonsters, this->combatMonsters->ai->target, 0, 0, false);
		} else {
			this->endMonstersTurn();
			if (!this->isCameraActive() && app->canvas->blockInputTime == 0) {
				app->canvas->drawPlayingSoftKeys();
			}
		}
	}
}

int Game::getNextBombIndex() {
	for (int i = 0; i < 4; ++i) {
		if (this->placedBombs[i] == 0) {
			return i;
		}
	}
	return -1;
}

void Game::updateBombs() {
	for (int i = 0; i < 4; ++i) {
		if (this->placedBombs[i] != 0) {
			Entity* entity = &this->entities[placedBombs[i]];
			int n = entity->param & 0xFF;
			if (--n < 0) {
				if ((entity->info & 0x100000) != 0x0) {
					entity->pain(1, nullptr);
				}
				this->placedBombs[i] = 0;
			} else {
				entity->param = ((entity->param & 0xFFFFFF00) | n);
			}
		}
	}
}

int Game::setDynamite(int x, int y, bool b) {


	int nextBombIndex = this->getNextBombIndex();
	if (nextBombIndex == -1) {
		app->Error("Too many bombs placed");
	}
	int flags = 0;
	if (!b) {
		switch (this->destAngle & 0x3FF) {
			case Enums::ANGLE_EAST: {
				flags |= Enums::SPRITE_FLAG_WEST;
				x--;
				break;
			}
			case Enums::ANGLE_NORTH: {
				flags |= Enums::SPRITE_FLAG_SOUTH;
				y++;
				break;
			}
			case Enums::ANGLE_WEST: {
				flags |= Enums::SPRITE_FLAG_EAST;
				x++;
				break;
			}
			case Enums::ANGLE_SOUTH: {
				flags |= Enums::SPRITE_FLAG_NORTH;
				y--;
				break;
			}
		}
	} else {
		flags |= (Enums::SPRITE_FLAG_TWO_SIDED | Enums::SPRITE_FLAG_HIDDEN);
	}
	this->allocDynamite((short)x, (short)y, (short)(app->render->getHeight(x, y) + (b ? 31 : 32)), flags, nextBombIndex,
	                    0);
	return nextBombIndex;
}

Entity* Game::getFreeDropEnt() {
	int dropIndex = this->dropIndex;
	int lastDropEntIndex = this->firstDropIndex + dropIndex;
	if ((this->entities[lastDropEntIndex].info & 0x100000) != 0x0) {
		for (dropIndex = (dropIndex + 1) % 16;
		     dropIndex != this->dropIndex && (this->entities[lastDropEntIndex].info & 0x100000) != 0x0;
		     dropIndex = (dropIndex + 1) % 16, lastDropEntIndex = this->firstDropIndex + dropIndex) {
		}
		if (this->dropIndex == dropIndex) {
		}
	}
	this->dropIndex = (dropIndex + 1) % 16;
	this->lastDropEntIndex = lastDropEntIndex;
	return &this->entities[lastDropEntIndex];
}

void Game::allocDynamite(int n, int n2, int n3, int n4, int n5, int n6) {


	Entity* freeDropEnt = this->getFreeDropEnt();
	freeDropEnt->def = app->entityDefManager->find(14, 6);
	freeDropEnt->name = (short)(freeDropEnt->def->name | 0x400);
	freeDropEnt->param = (n6 << 8 | 0x3);
	if (0x0 != (freeDropEnt->info & 0x100000)) {
		this->unlinkEntity(freeDropEnt);
	}
	this->linkEntity(freeDropEnt, n >> 6, n2 >> 6);
	freeDropEnt->info |= 0x420000;
	int sprite = freeDropEnt->getSprite();
	app->render->setSpriteX(sprite, (short)n);
	app->render->setSpriteY(sprite, (short)n2);
	app->render->setSpriteZ(sprite, (short)n3);
	app->render->setSpriteScaleFactor(sprite, 32);
	app->render->setSpriteRenderMode(sprite, 0);
	app->render->setSpriteInfoRaw(sprite, n4 | freeDropEnt->def->tileIndex);
	app->render->relinkSprite(sprite);
	this->placedBombs[n5] = this->lastDropEntIndex;
}

Entity* Game::spawnDropItem(int n, int n2, int n3, EntityDef* def, int param, bool b) {


	Entity* freeDropEnt = this->getFreeDropEnt();
	freeDropEnt->def = def;
	freeDropEnt->name = (short)(freeDropEnt->def->name | 0x400);
	if (n3 >= 1 && n3 < 14) {
		n3 |= 0x200;
	}
	if (0x0 != (freeDropEnt->info & 0x100000)) {
		this->unlinkEntity(freeDropEnt);
	}
	freeDropEnt->info &= 0xFFFF;
	freeDropEnt->info |= 0x400000;
	if (freeDropEnt->def->eType == Enums::ET_ATTACK_INTERACTIVE) {
		freeDropEnt->info |= 0x20000;
	}
	int sprite = freeDropEnt->getSprite();
	int height = app->render->getHeight(n, n2);
	app->render->setSpriteInfoRaw(sprite, n3);
	app->render->setSpriteX(sprite, (short)n);
	app->render->setSpriteY(sprite, (short)n2);
	app->render->setSpriteZ(sprite, (short)(32 + height));
	app->render->setSpriteEnt(sprite, (short)this->lastDropEntIndex);
	app->render->setSpriteScaleFactor(sprite, 64);
	freeDropEnt->param = param;
	app->render->relinkSprite(sprite);
	this->linkEntity(freeDropEnt, n >> 6, n2 >> 6);
	if (b) {
		this->throwDropItem(n, n2, height, freeDropEnt);
	}
	return freeDropEnt;
}

Entity* Game::spawnDropItem(int n, int n2, int n3, int n4, int n5, int n6, int n7, bool b) {


	EntityDef* find = app->entityDefManager->find(n4, n5, n6);
	if (find != nullptr) {
		return spawnDropItem(n, n2, n3, find, n7, b);
	}
	app->Error("Cannot find a def for the spawnDropItem.");
	return nullptr;
}

void Game::spawnDropItem(Entity* entity) {


	bool b = entity->loot != nullptr;
	bool b2;
	if (!entity->isMonster()) {
		b2 = (b & entity->param == 0);
	} else {
		b2 = (b & (entity->monsterFlags & Enums::MFLAG_LOOTED) == 0x0);
	}
	if (b2) {
		int n = entity->loot->lootSet[0];
		int n2 = n >> 12 & 0xF;
		if (n2 != 6) {
			int n3 = (n & 0xFC0) >> 6;
			int n4 = n & 0x3F;
			if (n4 > 0) {
				EntityDef* find = app->entityDefManager->find(6, n2, n3);
				if (find != nullptr) {
					if (find->tileIndex != 0) {
						int sprite = entity->getSprite();
						this->spawnDropItem(app->render->getSpriteX(sprite),
						                    app->render->getSpriteY(sprite), find->tileIndex, find,
						                    n4, false);
					}
				} else {
					app->Error("Cannot find a def for the dropItem.", 117); // ERR_ENT_LOOTSET
				}
			}
		}
	}
}

Entity* Game::spawnPlayerEntityCopy(int n, int n2) {


	int n3 = 72;
	int n4 = 224;
	switch (app->player->characterChoice) {
		case 1: {
			n3 = 68;
			n4 = 224;
			break;
		}
		case 3: {
			n3 = 66;
			n4 = 225;
			break;
		}
		case 2: {
			n3 = 72;
			n4 = 226;
			break;
		}
	}
	Entity* freeDropEnt = this->getFreeDropEnt();
	freeDropEnt->def = app->entityDefManager->lookup(n3);
	freeDropEnt->name = (short)(n4 | 0x0);
	if (0x0 != (freeDropEnt->info & 0x100000)) {
		this->unlinkEntity(freeDropEnt);
	}
	freeDropEnt->info &= 0xFFFF;
	freeDropEnt->info |= 0x400000;
	int sprite = freeDropEnt->getSprite();
	int height = app->render->getHeight(n, n2);
	app->render->setSpriteInfoRaw(sprite, n3);
	app->render->setSpriteX(sprite, (short)n);
	app->render->setSpriteY(sprite, (short)n2);
	app->render->setSpriteZ(sprite, (short)(32 + height));
	app->render->setSpriteEnt(sprite, (short)this->lastDropEntIndex);
	app->render->setSpriteRenderMode(sprite, 0);
	app->render->setSpriteScaleFactor(sprite, 64);
	freeDropEnt->param = 0;
	app->render->relinkSprite(sprite);
	this->linkEntity(freeDropEnt, n >> 6, n2 >> 6);
	return freeDropEnt;
}

Entity* Game::spawnSentryBotCorpse(int n, int n2, int n3, int n4, int n5) {


	int n6 = 19;
	int n7 = 0;
	switch (n3) {
		case 3:
		case 4: {
			n6 = 19;
			n7 = 0;
			break;
		}
		case 5:
		case 6: {
			n6 = 18;
			n7 = 1;
			break;
		}
	}
	Entity* freeDropEnt = this->getFreeDropEnt();
	freeDropEnt->def = app->entityDefManager->find(Enums::ET_MONSTER, 11, n7)->corpseDef;
	freeDropEnt->name = (short)(freeDropEnt->def->name | 0x400);
	freeDropEnt->populateDefaultLootSet();
	freeDropEnt->param = 0;
	if (n3 == 3 || n3 == 5) {
		freeDropEnt->addToLootSet(2, 1, 10);
	} else {
		freeDropEnt->addToLootSet(0, 12, 1);
	}
	freeDropEnt->addToLootSet(0, 16, n4);
	freeDropEnt->addToLootSet(0, 13, n5);
	if (0x0 != (freeDropEnt->info & 0x100000)) {
		this->unlinkEntity(freeDropEnt);
	}
	freeDropEnt->info &= 0xFFFF;
	freeDropEnt->info |= 0x1420000;
	int sprite = freeDropEnt->getSprite();
	int height = app->render->getHeight(n, n2);
	app->render->setSpriteInfoRaw(sprite, n6);
	app->render->setSpriteInfoFlag(sprite, 0xD00);
	app->render->setSpriteX(sprite, (short)n);
	app->render->setSpriteY(sprite, (short)n2);
	app->render->setSpriteZ(sprite, (short)(32 + height));
	app->render->setSpriteEnt(sprite, (short)this->lastDropEntIndex);
	app->render->setSpriteRenderMode(sprite, 0);
	app->render->setSpriteScaleFactor(sprite, 64);
	app->render->relinkSprite(sprite);
	this->linkEntity(freeDropEnt, n >> 6, n2 >> 6);
	return freeDropEnt;
}

void Game::throwDropItem(int dstX, int dstY, int n, Entity* entity) {


	LerpSprite* allocLerpSprite = this->allocLerpSprite(nullptr, entity->getSprite(), entity->def->eType != Enums::ET_ITEM);
	if (allocLerpSprite != nullptr) {
		allocLerpSprite->srcX = dstX;
		allocLerpSprite->srcY = dstY;
		allocLerpSprite->srcZ = (short)(32 + n);
		allocLerpSprite->dstX = dstX;
		allocLerpSprite->dstY = dstY;
		allocLerpSprite->dstZ = allocLerpSprite->srcZ;
		allocLerpSprite->height = 48;
		allocLerpSprite->flags |= (Enums::LS_FLAG_ASYNC | Enums::LS_FLAG_PARABOLA);
		allocLerpSprite->srcScale = allocLerpSprite->dstScale =
		    app->render->getSpriteScaleFactor(entity->getSprite());
		allocLerpSprite->startTime = app->gameTime;
		allocLerpSprite->travelTime = 850;
		int n3 = app->nextByte() & 0x7;
		int n4 = 8;
		while (--n4 >= 0) {
			dstX = allocLerpSprite->srcX + this->dropDirs[n3 << 1];
			dstY = allocLerpSprite->srcY + this->dropDirs[(n3 << 1) + 1];
			this->trace(allocLerpSprite->srcX, allocLerpSprite->srcY, dstX, dstY, entity, 15855, 25);
			if (this->traceEntity == nullptr && (this->baseVisitedTiles[dstY >> 6] & 1 << (dstX >> 6)) == 0x0) {
				allocLerpSprite->dstX = dstX;
				allocLerpSprite->dstY = dstY;
				allocLerpSprite->dstZ =
				    (short)(app->render->getHeight(allocLerpSprite->dstX, allocLerpSprite->dstY) + 32);
				break;
			}
			n3 = (n3 + 1 & 0x7);
		}
	}
}

bool Game::tileObstructsAttack(int n, int n2) {


	int n3 = app->canvas->destX >> 6;
	int n4 = app->canvas->destY >> 6;
	if (n != n3 && n2 != n4) {
		return false;
	}
	for (Entity* entity = this->combatMonsters; entity != nullptr; entity = entity->nextAttacker) {
		int* calcPosition = entity->calcPosition();
		int n5 = calcPosition[0] >> 6;
		int n6 = calcPosition[1] >> 6;
		if (n5 == n || n6 == n2) {
			if (n5 != n3) {
				int n7 = 0;
				int n8 = 0;
				if (n5 < n3) {
					n7 = n5;
					n8 = n3;
				} else if (n5 > n3) {
					n7 = n3;
					n8 = n5;
				}
				if (n > n7 && n < n8) {
					return true;
				}
			}
			if (n6 != n4) {
				int n9 = 0;
				int n10 = 0;
				if (n6 < n4) {
					n9 = n6;
					n10 = n4;
				} else if (n6 > n4) {
					n9 = n4;
					n10 = n6;
				}
				if (n2 > n9 && n2 < n10) {
					return true;
				}
			}
		}
	}
	return false;
}

void Game::awardSecret(bool b) {


	LOG_INFO("[game] awardSecret: {}/{} on map {}\n", this->mapSecretsFound + 1, this->totalSecrets, app->canvas->loadMapID);
	app->hud->addMessage((short)119);
	this->mapSecretsFound++;

	if (b) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::SECRET), 0, 3, 0);
	} else {
		app->sound->playSound(Sounds::getResIDByName(SoundName::CHIME), 0, 3, 0);
	}

	if (this->mapSecretsFound == this->totalSecrets &&
	    (app->player->foundSecretsLevels & 1 << app->canvas->loadMapID - 1) == 0x0) {
		app->player->showAchievementMessage(1);
		app->player->foundSecretsLevels |= 1 << app->canvas->loadMapID - 1;
	} else {
		app->player->addXP(5);
	}
}

void Game::addEntityDeathFunc(Entity* entity, int n) {


	int i;
	for (i = 0; i < 64; ++i) {
		if (this->entityDeathFunctions[i * 2] == -1) {
			this->entityDeathFunctions[i * 2 + 0] = (short)entity->getSprite();
			this->entityDeathFunctions[i * 2 + 1] = (short)n;
			break;
		}
	}
	if (i == 64) {
		app->Error("Too many entity death functions set");
	}
	entity->info |= 0x2000000;
}

void Game::removeEntityFunc(Entity* entity) {
	int sprite;
	int n;
	for (sprite = entity->getSprite(), n = 0; n < 64 && this->entityDeathFunctions[n * 2] != sprite; ++n) {
	}
	if (n != 64) {
		this->entityDeathFunctions[n * 2 + 0] = -1;
		this->entityDeathFunctions[n * 2 + 1] = -1;
	}
	entity->info &= 0xFDFFFFFF;
}

void Game::executeEntityFunc(Entity* entity, bool throwAwayLoot) {
	int sprite;
	int n;
	for (sprite = entity->getSprite(), n = 0; n < 64 && this->entityDeathFunctions[n * 2] != sprite; ++n) {
	}
	if (n != 64) {
		ScriptThread* allocScriptThread = this->allocScriptThread();
		allocScriptThread->throwAwayLoot = throwAwayLoot;
		allocScriptThread->alloc(this->entityDeathFunctions[n * 2 + 1]);
		allocScriptThread->run();
		this->entityDeathFunctions[n * 2 + 1] = (this->entityDeathFunctions[n * 2 + 0] = -1);
	}
}

void Game::foundLoot(int n, int n2) {

	this->foundLoot(app->render->getSpriteX(n), app->render->getSpriteY(n),
	                app->render->getSpriteZ(n), n2);
}

void Game::foundLoot(int n, int n2, int n3, int n4) {
	this->lootFound += (short)n4;
}

void Game::destroyedObject(int n) {
	this->destroyedObj++;
}

void Game::raiseCorpses() {


	Entity* inactiveMonsters = this->inactiveMonsters;
	int n = 0;
	if (inactiveMonsters != nullptr) {
		do {
			Entity* nextOnList = inactiveMonsters->nextOnList;
			int sprite = inactiveMonsters->getSprite();
			if ((inactiveMonsters->info & 0x10000) == 0x0 && (inactiveMonsters->info & 0x8000000) == 0x0 &&
			    (inactiveMonsters->info & 0x1000000) != 0x0 && !inactiveMonsters->isBoss() &&
			    (inactiveMonsters->monsterFlags & Enums::MFLAG_NORAISE) == 0x0 &&
			    this->findMapEntity(app->render->getSpriteX(sprite),
			                        app->render->getSpriteY(sprite), 15535) == nullptr &&
			    (this->difficulty != Enums::DIFFICULTY_NIGHTMARE || 0x0 == (inactiveMonsters->monsterEffects & 0x4))) {
				this->gridEntities[n++] = inactiveMonsters;
				if (n == 9) {
					break;
				}
			}
			inactiveMonsters = nextOnList;
		} while (inactiveMonsters != this->inactiveMonsters && n != 4);
		for (int i = 0; i < n; ++i) {
			Entity* entity = this->gridEntities[i];
			entity->info |= 0x8000000;
			int sprite = entity->getSprite();
			entity->resurrect(app->render->getSpriteX(sprite),
			                  app->render->getSpriteY(sprite),
			                  app->render->getSpriteZ(sprite));
			activate(entity, false, false, true, true);
			GameSprite* gameSprite = gsprite_allocAnim(241, app->render->getSpriteX(sprite),
			                                           app->render->getSpriteY(sprite),
			                                           app->render->getSpriteZ(sprite) - 20);
			gameSprite->flags |= 0x400;
			gameSprite->startScale = 64;
			gameSprite->destScale = 96;
			gameSprite->scaleStep = 40;
			int* calcPosition = entity->calcPosition();
			app->particleSystem->spawnParticles(1, -3355444, calcPosition[0], calcPosition[1],
			                                    app->render->getHeight(calcPosition[0], calcPosition[1]) + 48);
		}
	}
}

bool Game::isInFront(int n, int n2) {


	int n3 = 256 - (app->canvas->viewAngle & 0x3FF);
	int n4 = (n << 6) + 32 - app->canvas->viewX;
	int n5 = (n2 << 6) + 32 - app->canvas->viewY;
	if (n4 == 0 && n5 == 0) {
		return true;
	}
	int n6 = this->VecToDir(n4, n5, true) + n3 & 0x3FF;
	return n6 >= 128 && n6 <= 384;
}

int Game::VecToDir(int n, int n2, bool b) {
	int n3 = -1;
	int n4 = b ? 7 : 0;
	if (n <= -32) {
		n3 = 4;
	} else if (n >= 32) {
		n3 = 0;
	}
	if (n2 >= 32) {
		if (n3 == 4) {
			n3 = 5;
		} else if (n3 == 0) {
			n3 = 7;
		} else {
			n3 = 6;
		}
	} else if (n2 <= -32) {
		if (n3 == 4) {
			n3 = 3;
		} else if (n3 == 0) {
			n3 = 1;
		} else {
			n3 = 2;
		}
	}
	return n3 << n4;
}

void Game::NormalizeVec(int n, int n2, int* array) {
	int n3 = (int)this->FixedSqrt(n * n + n2 * n2);
	array[0] = (n << 12) / n3;
	array[1] = (n2 << 12) / n3;
}

int64_t Game::FixedSqrt(int64_t n) {
	if (n == 0LL) {
		return 0LL;
	}
	n <<= 8;
	int64_t n2 = 256LL;
	int64_t n3 = n2 + n / n2 >> 1;
	int64_t n4 = n3 + n / n3 >> 1;
	int64_t n5 = n4 + n / n4 >> 1;
	int64_t n6 = n5 + n / n5 >> 1;
	for (int i = 0; i < 12; ++i) {
		int64_t n7 = n6 + n / n6 >> 1;
		if (n7 == n6) {
			break;
		}
		n6 = n7;
	}
	return n6;
}

void Game::setMonsterClip(int n, int n2) {
	this->baseVisitedTiles[n2] |= 1 << n;
}

void Game::unsetMonsterClip(int n, int n2) {
	this->baseVisitedTiles[n2] &= ~(1 << n);
}

bool Game::monsterClipExists(int n, int n2) {
	return (this->baseVisitedTiles[n2] >> n & 0x1) == 0x1;
}

int Game::getMonsterSound(int monsterIdx, int soundType) {

	int monsterSoundResId = 0;
	int rand = 0;

	if (soundType == Enums::MSOUND_ALERT1) {
		rand = app->nextByte() % 3;
	}

	// Sound lookup by monsterIdx
	monsterSoundResId = this->monsterSounds[monsterIdx * Enums::MSOUND_TYPES + soundType + rand];
	if (monsterSoundResId != Enums::MSOUND_NONE) {
		monsterSoundResId += 1000;
	}

	// Random death sound override (e.g. imp has 2 death sounds, zombie has 3)
	const MonsterBehaviors& mb = app->combat->monsterBehaviors[monsterIdx];
	if (soundType == Enums::MSOUND_DEATH && mb.numRandomDeathSounds > 0) {
		monsterSoundResId = mb.randomDeathSounds[app->nextInt() % mb.numRandomDeathSounds];
	}

	return monsterSoundResId;
}
