#include <stdexcept>
#include <algorithm>
#include <cmath>
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
#include "LootComponent.h"
#include "BinaryStream.h"
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


	int mapIdx = app->canvas->loadMapID - 1;
	app->player->fillMonsterStats();
	if (!this->isLoaded && 0x0 != (app->player->completedLevels & 1 << mapIdx) &&
	    app->player->monsterStats[0] == app->player->monsterStats[1]) {
		int respawnedThisCall = 0;
		int totalMonsters = app->player->monsterStats[1];
		int respawnQuota = totalMonsters - app->player->monsterStats[0];
		for (int i = 0; i < this->numEntities; ++i) {
			Entity* entity = &this->entities[i];
			if (entity->isMonster()) {
				int sprite = entity->getSprite();
				EntityMonster* monster = entity->monster;
				if (0x0 != (entity->info & 0x1010000) && 0x0 == (entity->monsterFlags & (Enums::MFLAG_NOTRACK | Enums::MFLAG_NORESPAWN)) && respawnQuota < totalMonsters / 2 && respawnedThisCall < 8 &&
				    app->nextByte() <= 100) {
					Render* render = app->render.get();
					if (this->findMapEntity(render->getSpriteX(sprite),
					                        render->getSpriteY(sprite), 15535) == nullptr) {
						entity->resurrect(render->getSpriteX(sprite),
						                  render->getSpriteY(sprite), 32);
						entity->info &= 0xFFBFFFFF;
						++respawnQuota;
						++respawnedThisCall;
					}
				}
			}
		}
	}
}

void Game::activate(Entity* entity) {
	this->activate(entity, true, false, true, false);
}

void Game::activate(Entity* entity, bool triggerScript, bool checkRange, bool playAlertSound, bool b4) {


	EntityMonster* monster = entity->monster;
	if ((app->render->getSpriteFrame(entity->getSprite()) & 0xF0) == 16 && !app->render->shotsFired) {
		return;
	}
	if (checkRange && entity->distFrom(app->canvas->viewX, app->canvas->viewY) > app->combat->tileDistances[3]) {
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
	if (triggerScript && 0x0 != (entity->monsterFlags & Enums::MFLAG_TRIGGERONACTIVATE)) {
		this->executeStaticFunc(9);
		entity->monsterFlags &= ~Enums::MFLAG_TRIGGERONACTIVATE;
	}
	if (app->canvas->state == Canvas::ST_PLAYING && playAlertSound) {
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
		int needsRevalidation = 0;
		Entity* activeMonsters = this->activeMonsters;
		do {
			Entity* nextOnList = activeMonsters->nextOnList;
			if (0x0 == (activeMonsters->monsterEffects & 0x2) &&
			    (this->monstersTurn == 1 || (this->monstersTurn == 2 && activeMonsters->isHasteResistant()))) {
				activeMonsters->aiThink(false);
				if ((activeMonsters->ai->goalFlags & 0x1) != 0x0) {
					needsRevalidation = 1;
				}
			}
			activeMonsters = nextOnList;
		} while (activeMonsters != this->activeMonsters && this->activeMonsters != nullptr);
		while (needsRevalidation != 0) {
			needsRevalidation = 0;
			Entity* nextAttacker;
			for (Entity* combatMonsters = this->combatMonsters; combatMonsters != nullptr;
			     combatMonsters = nextAttacker) {
				nextAttacker = combatMonsters->nextAttacker;
				if (!combatMonsters->aiIsAttackValid()) {
					combatMonsters->undoAttack();
					combatMonsters->aiThink(false);
					if ((combatMonsters->ai->goalFlags & 0x1) != 0x0) {
						needsRevalidation = 1;
					}
				}
			}
		}
	}
}

void Game::monsterLerp() {


	bool interpolatingMonsters = false;
	int anyVisible = 0;
	if (!this->interpolatingMonsters) {
		return;
	}
	if (this->activeMonsters != nullptr) {
		Entity* entity = this->activeMonsters;
		do {
			if ((entity->ai->goalFlags & 0x1) != 0x0) {
				interpolatingMonsters = true;
				if (anyVisible == 0 && app->render->checkTileVisibilty(entity->ai->goalX, entity->ai->goalY)) {
					anyVisible = 1;
				}
			}
			entity = entity->nextOnList;
		} while (entity != this->activeMonsters);
	}
	if (interpolatingMonsters && anyVisible != 0) {
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

bool Game::snapMonsters(bool forceSnap) {


	if (this->animatingEffects != 0) {
		for (int i = 0; i < 16; ++i) {
			LerpSprite* lerpSprite = &this->lerpSprites[i];
			if (lerpSprite->hSprite != 0) {
				if ((lerpSprite->flags & Enums::LS_FLAG_ANIMATING_EFFECT) != 0x0) {
					short entIdx = app->render->getSpriteEnt(lerpSprite->hSprite - 1);
					if (entIdx == -1) {
						return false;
					}
					if (this->entities[entIdx].def->eType != Enums::ET_MONSTER) {
						return false;
					}
				}
			}
		}
	}
	bool canSnap = forceSnap;
	if (this->monstersTurn == 0) {
		if (!forceSnap) {
			canSnap = this->canSnapMonsters();
		}
		Entity* activeMonsters = this->activeMonsters;
		if (canSnap && activeMonsters != nullptr) {
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
		return canSnap;
	}
	this->monsterAI();
	if (!forceSnap) {
		canSnap = this->canSnapMonsters();
	}
	if (canSnap && this->activeMonsters != nullptr) {
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
	return canSnap;
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
			int bombTimer = entity->param & 0xFF;
			if (--bombTimer < 0) {
				if ((entity->info & 0x100000) != 0x0) {
					entity->pain(1, nullptr);
				}
				this->placedBombs[i] = 0;
			} else {
				entity->param = ((entity->param & 0xFFFFFF00) | bombTimer);
			}
		}
	}
}

int Game::setDynamite(int x, int y, bool centered) {


	int nextBombIndex = this->getNextBombIndex();
	if (nextBombIndex == -1) {
		app->Error("Too many bombs placed");
	}
	int flags = 0;
	if (!centered) {
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
	this->allocDynamite((short)x, (short)y, (short)(app->render->getHeight(x, y) + (centered ? 31 : 32)), flags, nextBombIndex,
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

void Game::allocDynamite(int x, int y, int z, int spriteFlags, int bombSlotIdx, int bombTimer) {


	Entity* freeDropEnt = this->getFreeDropEnt();
	freeDropEnt->def = app->entityDefManager->find(14, 6);
	freeDropEnt->name = (short)(freeDropEnt->def->name | 0x400);
	freeDropEnt->param = (bombTimer << 8 | 0x3);
	if (0x0 != (freeDropEnt->info & 0x100000)) {
		this->unlinkEntity(freeDropEnt);
	}
	this->linkEntity(freeDropEnt, x >> 6, y >> 6);
	freeDropEnt->info |= 0x420000;
	int sprite = freeDropEnt->getSprite();
	app->render->setSpriteX(sprite, (short)x);
	app->render->setSpriteY(sprite, (short)y);
	app->render->setSpriteZ(sprite, (short)z);
	app->render->setSpriteScaleFactor(sprite, 32);
	app->render->setSpriteRenderMode(sprite, 0);
	app->render->setSpriteInfoRaw(sprite, spriteFlags | freeDropEnt->def->tileIndex);
	app->render->relinkSprite(sprite);
	this->placedBombs[bombSlotIdx] = this->lastDropEntIndex;
}

Entity* Game::spawnDropItem(int x, int y, int tileIdx, EntityDef* def, int param, bool throwAnim) {


	Entity* freeDropEnt = this->getFreeDropEnt();
	freeDropEnt->def = def;
	freeDropEnt->name = (short)(freeDropEnt->def->name | 0x400);
	if (tileIdx >= 1 && tileIdx < 14) {
		tileIdx |= 0x200;
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
	int height = app->render->getHeight(x, y);
	app->render->setSpriteInfoRaw(sprite, tileIdx);
	app->render->setSpriteX(sprite, (short)x);
	app->render->setSpriteY(sprite, (short)y);
	app->render->setSpriteZ(sprite, (short)(32 + height));
	app->render->setSpriteEnt(sprite, (short)this->lastDropEntIndex);
	app->render->setSpriteScaleFactor(sprite, 64);
	freeDropEnt->param = param;
	app->render->relinkSprite(sprite);
	this->linkEntity(freeDropEnt, x >> 6, y >> 6);
	if (throwAnim) {
		this->throwDropItem(x, y, height, freeDropEnt);
	}
	return freeDropEnt;
}

Entity* Game::spawnDropItem(int x, int y, int tileIdx, int eType, int eSubType, int defParm, int param, bool throwAnim) {


	EntityDef* find = app->entityDefManager->find(eType, eSubType, defParm);
	if (find != nullptr) {
		return spawnDropItem(x, y, tileIdx, find, param, throwAnim);
	}
	app->Error("Cannot find a def for the spawnDropItem.");
	return nullptr;
}

void Game::spawnDropItem(Entity* entity) {


	bool hasLoot = entity->loot != nullptr;
	bool shouldDrop;
	if (!entity->isMonster()) {
		shouldDrop = (hasLoot & entity->param == 0);
	} else {
		shouldDrop = (hasLoot & (entity->monsterFlags & Enums::MFLAG_LOOTED) == 0x0);
	}
	if (shouldDrop) {
		for (int i = 0; i < LootComponent::MAX_SLOTS; ++i) {
			int lootEntry = entity->loot->lootSet[i];
			if (lootEntry == 0) break; // sentinel: zero terminates the slot list
			int lootCategory = lootEntry >> 12 & 0xF;
			if (lootCategory == 6) continue; // trinket — handled elsewhere
			int lootItemIdx = (lootEntry & 0xFC0) >> 6;
			int lootCount = lootEntry & 0x3F;
			if (lootCount <= 0) continue;
			EntityDef* find = app->entityDefManager->find(6, lootCategory, lootItemIdx);
			if (find != nullptr) {
				if (find->tileIndex != 0) {
					int sprite = entity->getSprite();
					this->spawnDropItem(app->render->getSpriteX(sprite),
					                    app->render->getSpriteY(sprite), find->tileIndex, find,
					                    lootCount, false);
				}
			} else {
				app->Error("Cannot find a def for the dropItem.", 117); // ERR_ENT_LOOTSET
			}
		}
	}
}

Entity* Game::spawnPlayerEntityCopy(int x, int y) {


	int defLookup = 72;
	int nameId = 224;
	switch (app->player->characterChoice) {
		case 1: {
			defLookup = 68;
			nameId = 224;
			break;
		}
		case 3: {
			defLookup = 66;
			nameId = 225;
			break;
		}
		case 2: {
			defLookup = 72;
			nameId = 226;
			break;
		}
	}
	Entity* freeDropEnt = this->getFreeDropEnt();
	freeDropEnt->def = app->entityDefManager->lookup(defLookup);
	freeDropEnt->name = (short)(nameId | 0x0);
	if (0x0 != (freeDropEnt->info & 0x100000)) {
		this->unlinkEntity(freeDropEnt);
	}
	freeDropEnt->info &= 0xFFFF;
	freeDropEnt->info |= 0x400000;
	int sprite = freeDropEnt->getSprite();
	int height = app->render->getHeight(x, y);
	app->render->setSpriteInfoRaw(sprite, defLookup);
	app->render->setSpriteX(sprite, (short)x);
	app->render->setSpriteY(sprite, (short)y);
	app->render->setSpriteZ(sprite, (short)(32 + height));
	app->render->setSpriteEnt(sprite, (short)this->lastDropEntIndex);
	app->render->setSpriteRenderMode(sprite, 0);
	app->render->setSpriteScaleFactor(sprite, 64);
	freeDropEnt->param = 0;
	app->render->relinkSprite(sprite);
	this->linkEntity(freeDropEnt, x >> 6, y >> 6);
	return freeDropEnt;
}

Entity* Game::spawnSentryBotCorpse(int x, int y, int botSubType, int ammoLoot, int healthLoot) {


	int corpseTile = 19;
	int corpseVariant = 0;
	switch (botSubType) {
		case 3:
		case 4: {
			corpseTile = 19;
			corpseVariant = 0;
			break;
		}
		case 5:
		case 6: {
			corpseTile = 18;
			corpseVariant = 1;
			break;
		}
	}
	Entity* freeDropEnt = this->getFreeDropEnt();
	freeDropEnt->def = app->entityDefManager->find(Enums::ET_MONSTER, 11, corpseVariant)->corpseDef;
	freeDropEnt->name = (short)(freeDropEnt->def->name | 0x400);
	freeDropEnt->populateDefaultLootSet();
	freeDropEnt->param = 0;
	if (botSubType == 3 || botSubType == 5) {
		freeDropEnt->addToLootSet(2, 1, 10);
	} else {
		freeDropEnt->addToLootSet(0, 12, 1);
	}
	freeDropEnt->addToLootSet(0, 16, ammoLoot);
	freeDropEnt->addToLootSet(0, 13, healthLoot);
	if (0x0 != (freeDropEnt->info & 0x100000)) {
		this->unlinkEntity(freeDropEnt);
	}
	freeDropEnt->info &= 0xFFFF;
	freeDropEnt->info |= 0x1420000;
	int sprite = freeDropEnt->getSprite();
	int height = app->render->getHeight(x, y);
	app->render->setSpriteInfoRaw(sprite, corpseTile);
	app->render->setSpriteInfoFlag(sprite, 0xD00);
	app->render->setSpriteX(sprite, (short)x);
	app->render->setSpriteY(sprite, (short)y);
	app->render->setSpriteZ(sprite, (short)(32 + height));
	app->render->setSpriteEnt(sprite, (short)this->lastDropEntIndex);
	app->render->setSpriteRenderMode(sprite, 0);
	app->render->setSpriteScaleFactor(sprite, 64);
	app->render->relinkSprite(sprite);
	this->linkEntity(freeDropEnt, x >> 6, y >> 6);
	return freeDropEnt;
}

void Game::throwDropItem(int dstX, int dstY, int height, Entity* entity) {


	LerpSprite* allocLerpSprite = this->allocLerpSprite(nullptr, entity->getSprite(), entity->def->eType != Enums::ET_ITEM);
	if (allocLerpSprite != nullptr) {
		allocLerpSprite->srcX = dstX;
		allocLerpSprite->srcY = dstY;
		allocLerpSprite->srcZ = (short)(32 + height);
		allocLerpSprite->dstX = dstX;
		allocLerpSprite->dstY = dstY;
		allocLerpSprite->dstZ = allocLerpSprite->srcZ;
		allocLerpSprite->height = 48;
		allocLerpSprite->flags |= (Enums::LS_FLAG_ASYNC | Enums::LS_FLAG_PARABOLA);
		allocLerpSprite->srcScale = allocLerpSprite->dstScale =
		    app->render->getSpriteScaleFactor(entity->getSprite());
		allocLerpSprite->startTime = app->gameTime;
		allocLerpSprite->travelTime = 850;
		int dirIdx = app->nextByte() & 0x7;
		int triesLeft = 8;
		while (--triesLeft >= 0) {
			dstX = allocLerpSprite->srcX + this->dropDirs[dirIdx << 1];
			dstY = allocLerpSprite->srcY + this->dropDirs[(dirIdx << 1) + 1];
			this->trace(allocLerpSprite->srcX, allocLerpSprite->srcY, dstX, dstY, entity, 15855, 25);
			if (this->traceEntity == nullptr && (this->baseVisitedTiles[dstY >> 6] & 1 << (dstX >> 6)) == 0x0) {
				allocLerpSprite->dstX = dstX;
				allocLerpSprite->dstY = dstY;
				allocLerpSprite->dstZ =
				    (short)(app->render->getHeight(allocLerpSprite->dstX, allocLerpSprite->dstY) + 32);
				break;
			}
			dirIdx = (dirIdx + 1 & 0x7);
		}
	}
}

bool Game::tileObstructsAttack(int tileX, int tileY) {


	int playerTileX = app->canvas->destX >> 6;
	int playerTileY = app->canvas->destY >> 6;
	if (tileX != playerTileX && tileY != playerTileY) {
		return false;
	}
	for (Entity* entity = this->combatMonsters; entity != nullptr; entity = entity->nextAttacker) {
		int* calcPosition = entity->calcPosition();
		int monsterTileX = calcPosition[0] >> 6;
		int monsterTileY = calcPosition[1] >> 6;
		if (monsterTileX == tileX || monsterTileY == tileY) {
			if (monsterTileX != playerTileX) {
				int minX = 0;
				int maxX = 0;
				if (monsterTileX < playerTileX) {
					minX = monsterTileX;
					maxX = playerTileX;
				} else if (monsterTileX > playerTileX) {
					minX = playerTileX;
					maxX = monsterTileX;
				}
				if (tileX > minX && tileX < maxX) {
					return true;
				}
			}
			if (monsterTileY != playerTileY) {
				int minY = 0;
				int maxY = 0;
				if (monsterTileY < playerTileY) {
					minY = monsterTileY;
					maxY = playerTileY;
				} else if (monsterTileY > playerTileY) {
					minY = playerTileY;
					maxY = monsterTileY;
				}
				if (tileY > minY && tileY < maxY) {
					return true;
				}
			}
		}
	}
	return false;
}

void Game::awardSecret(bool playSecretSound) {


	LOG_INFO("[game] awardSecret: {}/{} on map {}\n", this->mapSecretsFound + 1, this->totalSecrets, app->canvas->loadMapID);
	app->hud->addMessage((short)119);
	this->mapSecretsFound++;

	if (playSecretSound) {
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

void Game::addEntityDeathFunc(Entity* entity, int funcId) {


	int i;
	for (i = 0; i < 64; ++i) {
		if (this->entityDeathFunctions[i * 2] == -1) {
			this->entityDeathFunctions[i * 2 + 0] = (short)entity->getSprite();
			this->entityDeathFunctions[i * 2 + 1] = (short)funcId;
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
	int slot;
	for (sprite = entity->getSprite(), slot = 0; slot < 64 && this->entityDeathFunctions[slot * 2] != sprite; ++slot) {
	}
	if (slot != 64) {
		this->entityDeathFunctions[slot * 2 + 0] = -1;
		this->entityDeathFunctions[slot * 2 + 1] = -1;
	}
	entity->info &= 0xFDFFFFFF;
}

void Game::executeEntityFunc(Entity* entity, bool throwAwayLoot) {
	int sprite;
	int slot;
	for (sprite = entity->getSprite(), slot = 0; slot < 64 && this->entityDeathFunctions[slot * 2] != sprite; ++slot) {
	}
	if (slot != 64) {
		ScriptThread* allocScriptThread = this->allocScriptThread();
		allocScriptThread->throwAwayLoot = throwAwayLoot;
		allocScriptThread->alloc(this->entityDeathFunctions[slot * 2 + 1]);
		allocScriptThread->run();
		this->entityDeathFunctions[slot * 2 + 1] = (this->entityDeathFunctions[slot * 2 + 0] = -1);
	}
}

void Game::foundLoot(int sprite, int lootValue) {

	this->foundLoot(app->render->getSpriteX(sprite), app->render->getSpriteY(sprite),
	                app->render->getSpriteZ(sprite), lootValue);
}

void Game::foundLoot(int x, int y, int z, int lootValue) {
	this->lootFound += (short)lootValue;
}

void Game::destroyedObject(int unused) {
	this->destroyedObj++;
}

void Game::raiseCorpses() {


	Entity* inactiveMonsters = this->inactiveMonsters;
	int numToRaise = 0;
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
				this->gridEntities[numToRaise++] = inactiveMonsters;
				if (numToRaise == 9) {
					break;
				}
			}
			inactiveMonsters = nextOnList;
		} while (inactiveMonsters != this->inactiveMonsters && numToRaise != 4);
		for (int i = 0; i < numToRaise; ++i) {
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

bool Game::isInFront(int tileX, int tileY) {


	int viewAngleOffset = 256 - (app->canvas->viewAngle & 0x3FF);
	int dx = (tileX << 6) + 32 - app->canvas->viewX;
	int dy = (tileY << 6) + 32 - app->canvas->viewY;
	if (dx == 0 && dy == 0) {
		return true;
	}
	int relativeAngle = this->VecToDir(dx, dy, true) + viewAngleOffset & 0x3FF;
	return relativeAngle >= 128 && relativeAngle <= 384;
}

int Game::VecToDir(int dx, int dy, bool highShift) {
	int dir = -1;
	int shift = highShift ? 7 : 0;
	if (dx <= -32) {
		dir = 4;
	} else if (dx >= 32) {
		dir = 0;
	}
	if (dy >= 32) {
		if (dir == 4) {
			dir = 5;
		} else if (dir == 0) {
			dir = 7;
		} else {
			dir = 6;
		}
	} else if (dy <= -32) {
		if (dir == 4) {
			dir = 3;
		} else if (dir == 0) {
			dir = 1;
		} else {
			dir = 2;
		}
	}
	return dir << shift;
}

void Game::NormalizeVec(int vecX, int vecY, int* array) {
	// Returns the unit vector (vecX, vecY) scaled by 256 (Q8 fixed-point).
	double mag = std::hypot((double)vecX, (double)vecY);
	if (mag == 0.0) {
		array[0] = 0;
		array[1] = 0;
		return;
	}
	array[0] = (int)((double)vecX * 256.0 / mag);
	array[1] = (int)((double)vecY * 256.0 / mag);
}

void Game::setMonsterClip(int tileX, int tileY) {
	this->baseVisitedTiles[tileY] |= 1 << tileX;
}

void Game::unsetMonsterClip(int tileX, int tileY) {
	this->baseVisitedTiles[tileY] &= ~(1 << tileX);
}

bool Game::monsterClipExists(int tileX, int tileY) {
	return (this->baseVisitedTiles[tileY] >> tileX & 0x1) == 0x1;
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
