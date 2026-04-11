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

void Game::unlinkEntity(Entity* entity) {


	if (entity == &this->entities[0]) {
		app->Error(4); // ERR_BADUNLINKWORLD
		return;
	}

	if (entity == this->entityDb[entity->linkIndex]) {
		this->entityDb[entity->linkIndex] = entity->nextOnTile;
	} else if (entity->prevOnTile != nullptr) {
		entity->prevOnTile->nextOnTile = entity->nextOnTile;
	}

	if (entity->nextOnTile != nullptr) {
		entity->nextOnTile->prevOnTile = entity->prevOnTile;
	}

	entity->nextOnTile = nullptr;
	entity->prevOnTile = nullptr;
	entity->info &= 0xFFEFFFFF;
}

void Game::linkEntity(Entity* entity, int i, int i2) {

	short linkIndex = (short)(i2 * 32 + i);

	if (entity == &this->entities[0]) {
		app->Error(3); // ERR_BADLINKWORLD
		return;
	}

	if (entity->nextOnTile != nullptr || entity->prevOnTile != nullptr || entity == this->entityDb[linkIndex]) {
		app->Error(26); // ERR_LINKLINKEDENTITY
		return;
	}

	Entity* nextOnTile = this->entityDb[linkIndex];
	if (nextOnTile != nullptr) {
		nextOnTile->prevOnTile = entity;
	}

	entity->nextOnTile = nextOnTile;
	entity->prevOnTile = nullptr;
	entity->linkIndex = linkIndex;
	this->entityDb[linkIndex] = entity;
	entity->info |= 0x100000;
}

void Game::unlinkWorldEntity(int i, int i2) {


	short linkIndex = (short)(i2 * 32 + i);
	Entity* nextOnTile = this->entityDb[linkIndex];

	Entity* entity = &this->entities[0];
	this->baseVisitedTiles[i2] &= ~(1 << i);
	if (nextOnTile == entity) {
		this->entityDb[linkIndex] = nullptr;
		app->render->mapFlags[linkIndex] &= 0xFFFFFFFE;
		return;
	}

	Entity* nextEntity = nullptr;
	while (nextOnTile != nullptr) {
		if (nextOnTile == entity) {
			if (nextEntity != nullptr) {
				nextEntity->nextOnTile = nullptr;
			}
			app->render->mapFlags[linkIndex] &= 0xFFFFFFFE;
			return;
		}
		nextEntity = nextOnTile;
		nextOnTile = nextOnTile->nextOnTile;
	}

	app->Error(52); // ERR_UNLINKWORLD
}

void Game::linkWorldEntity(int i, int i2) {


	short linkIndex = (short)(i2 * 32 + i);
	Entity* nextOnTile = this->entityDb[linkIndex];
	Entity* nextEntity = nullptr;

	Entity* entity = &this->entities[0];
	while (nextOnTile != nullptr) {
		if (nextOnTile->def->eType == Enums::ET_WORLD) {
			app->Error(27); // ERR_LINKWORLD
			return;
		}
		nextEntity = nextOnTile;
		nextOnTile = nextOnTile->nextOnTile;
	}

	if (nextEntity == nullptr) {
		this->entityDb[linkIndex] = entity;
	} else {
		nextEntity->nextOnTile = entity;
	}

	entity->nextOnTile = nullptr;
	entity->prevOnTile = nullptr;
	entity->linkIndex = linkIndex;

	this->baseVisitedTiles[i2] |= 1 << i;
	app->render->mapFlags[linkIndex] |= 0x1;
}

void Game::removeEntity(Entity* entity) {


	if ((entity->info & 0xFFFF) != 0x0) {
		app->render->mapSpriteInfo[entity->getSprite()] |= 0x10000;
	}
	if ((entity->info & 0x100000)) {
		this->unlinkEntity(entity);
	}
	app->player->facingEntity = nullptr;
}

void Game::trace(int n, int n2, int n3, int n4, Entity* entity, int n5, int n6) {
	this->trace(n, n2, -1, n3, n4, -1, entity, n5, n6, false);
}

void Game::trace(int n, int n2, int n3, int traceCollisionX, int traceCollisionY, int traceCollisionZ, Entity* entity,
                 int n4, int n5, bool b) {


	int* tracePoints = this->tracePoints;
	int* traceBoundingBox = this->traceBoundingBox;
	int* traceFracs = this->traceFracs;
	Entity** traceEntities = this->traceEntities;
	this->traceEntity = nullptr;
	this->numTraceEntities = 0;
	tracePoints[0] = n;
	tracePoints[1] = n2;
	tracePoints[2] = traceCollisionX;
	tracePoints[3] = traceCollisionY;
	traceBoundingBox[0] = std::max(std::min(n - n5, traceCollisionX - n5), 0);
	traceBoundingBox[1] = std::max(std::min(n2 - n5, traceCollisionY - n5), 0);
	traceBoundingBox[2] = std::min(std::max(n + n5, traceCollisionX + n5), 2047);
	traceBoundingBox[3] = std::min(std::max(n2 + n5, traceCollisionY + n5), 2047);
	for (int i = traceBoundingBox[0] >> 6; i < (traceBoundingBox[2] >> 6) + 1; ++i) {
		for (int j = traceBoundingBox[1] >> 6; j < (traceBoundingBox[3] >> 6) + 1; ++j) {
			Entity* nextOnTile = this->entityDb[i + 32 * j];
			if (nextOnTile != nullptr) {
				while (nextOnTile != nullptr) {
					if (nextOnTile != entity && 0x0 != (n4 & 1 << nextOnTile->def->eType)) {
						if (nextOnTile->def->eType != Enums::ET_WORLD) {
							int sprite = nextOnTile->getSprite();
							int destX;
							int destY;
							int destZ;
							if (nextOnTile->monster != nullptr) {
								if ((nextOnTile->monster->goalFlags & 0x1) != 0x0) {
									destX = 32 + (nextOnTile->monster->goalX << 6);
									destY = 32 + (nextOnTile->monster->goalY << 6);
									destZ = app->render->mapSprites[app->render->S_Z + sprite];
								} else {
									destX = app->render->mapSprites[app->render->S_X + sprite];
									destY = app->render->mapSprites[app->render->S_Y + sprite];
									destZ = app->render->mapSprites[app->render->S_Z + sprite];
								}
							} else if (nextOnTile->def->eType == Enums::ET_PLAYER) {
								destX = app->canvas->destX;
								destY = app->canvas->destY;
								destZ = app->canvas->destZ;
							} else {
								destX = app->render->mapSprites[app->render->S_X + sprite];
								destY = app->render->mapSprites[app->render->S_Y + sprite];
								destZ = app->render->mapSprites[app->render->S_Z + sprite];
							}
							if (sprite != -1 && 0x0 != (app->render->mapSpriteInfo[sprite] & 0xF000000)) {
								int n6 = destX;
								int n7 = destY;
								if (0x0 != (app->render->mapSpriteInfo[sprite] & 0x3000000)) {
									destX -= 32;
									n6 += 32;
								} else {
									destY -= 32;
									n7 += 32;
								}
								app->render->traceLine[0] = destX;
								app->render->traceLine[1] = destY;
								app->render->traceLine[2] = n6;
								app->render->traceLine[3] = n7;
								int capsuleToLineTrace =
								    app->render->CapsuleToLineTrace(tracePoints, n5 * n5, app->render->traceLine);
								if (capsuleToLineTrace < 16384) {
									traceFracs[this->numTraceEntities] = capsuleToLineTrace;
									traceEntities[this->numTraceEntities++] = nextOnTile;
								}
							} else {
								int n8 = 625;
								if (nextOnTile->def->eType == Enums::ET_ENV_DAMAGE) {
									n8 = 256;
								}
								int capsuleToCircleTrace =
								    app->render->CapsuleToCircleTrace(tracePoints, n5 * n5, destX, destY, n8);
								if (capsuleToCircleTrace < 16384) {
									if (n3 >= 0 && traceCollisionZ >= 0 && b) {
										int n9 = n3 + ((traceCollisionZ - n3) * capsuleToCircleTrace >> 14) - destZ;
										if (n9 > -(32 + n5) && n9 < 32 + n5) {
											traceFracs[this->numTraceEntities] = capsuleToCircleTrace;
											traceEntities[this->numTraceEntities++] = nextOnTile;
										}
									} else {
										traceFracs[this->numTraceEntities] = capsuleToCircleTrace;
										traceEntities[this->numTraceEntities++] = nextOnTile;
									}
								}
							}
						}
					}
					nextOnTile = nextOnTile->nextOnTile;
				}
			}
		}
	}
	if (0x0 != (n4 & 0x1)) {
		int traceWorld = app->render->traceWorld(0, tracePoints, n5, traceBoundingBox, n4);
		if (traceWorld < 16384) {
			traceFracs[this->numTraceEntities] = traceWorld;
			traceEntities[this->numTraceEntities++] = &this->entities[0];
			this->traceCollisionX = n + ((traceWorld * (traceCollisionX - n)) >> 14);
			this->traceCollisionY = n2 + ((traceWorld * (traceCollisionY - n2)) >> 14);
			this->traceCollisionZ = n3 + ((traceWorld * (traceCollisionZ - n3)) >> 14);
		} else {
			this->traceCollisionX = traceCollisionX;
			this->traceCollisionY = traceCollisionY;
			this->traceCollisionZ = traceCollisionZ;
		}
	}
	if (this->numTraceEntities > 0) {
		for (int k = 0; k < this->numTraceEntities - 1; ++k) {
			for (int l = 0; l < this->numTraceEntities - 1 - k; ++l) {
				if (traceFracs[l + 1] < traceFracs[l]) {
					int n10 = traceFracs[l];
					traceFracs[l] = traceFracs[l + 1];
					traceFracs[l + 1] = n10;
					Entity* entity2 = traceEntities[l];
					traceEntities[l] = traceEntities[l + 1];
					traceEntities[l + 1] = entity2;
				}
			}
		}
		this->traceEntity = traceEntities[0];
	}
}

bool Game::touchTile(int n, int n2, bool b) {
	bool b2 = false;
	Entity* nextOnTile;
	// printf("touchTile %d, %d\n", n, n2);
	for (Entity* mapEntity = this->findMapEntity(n, n2); mapEntity != nullptr; mapEntity = nextOnTile) {
		nextOnTile = mapEntity->nextOnTile;
		if (b || mapEntity->def->eType == Enums::ET_ENV_DAMAGE) {
			mapEntity->touched();
			b2 = true;
		}
	}
	return b2;
}

Entity* Game::findMapEntity(int n, int n2) {
	n >>= 6;
	n2 >>= 6;
	if (n < 0 || n >= 32 || n2 < 0 || n2 >= 32) {
		return nullptr;
	}
	return this->entityDb[n2 * 32 + n];
}

Entity* Game::findMapEntity(int n, int n2, int n3) {


	if ((n3 & 0x2) != 0x0 && n >> 6 == app->canvas->destX >> 6 && n2 >> 6 == app->canvas->destY >> 6) {
		return &this->entities[1];
	}
	for (Entity* entity = this->findMapEntity(n, n2); entity != nullptr; entity = entity->nextOnTile) {
		if ((n3 & 1 << entity->def->eType) != 0x0) {
			return entity;
		}
	}
	return nullptr;
}

bool Game::performDoorEvent(int n, Entity* entity, int n2) {
	return this->performDoorEvent(n, entity, n2, false);
}

bool Game::performDoorEvent(int n, Entity* entity, int n2, bool b) {
	return this->performDoorEvent(n, nullptr, entity, n2, b);
}

bool Game::performDoorEvent(int n, ScriptThread* scriptThread, Entity* watchLine, int n2, bool b) {


	int sprite = watchLine->getSprite();
	int n3 = app->render->mapSpriteInfo[sprite];
	int n4 = n3 & 0xFF;
	bool b2 = (watchLine->info & 0x100000) == 0x0;
	if ((n3 & 0x400000) != 0x0) {
		n4 += 257;
	}

	bool b3 = n4 >= 271 && n4 < 281;
	if (watchLine->def->eSubType == Enums::DOOR_LOCKED) {
		return false;
	}
	if (n == 0 && b2 && b3) {
		this->updatePlayerDoors(watchLine, true);
		return false;
	}
	if (n == 1 && !b2) {
		return false;
	}
	if (b2 && b3) {
		for (Entity* entity = this->findMapEntity(watchLine->linkIndex % 32 << 6, watchLine->linkIndex / 32 << 6);
		     entity != nullptr; entity = entity->nextOnTile) {
			if (entity->def->eType == Enums::ET_MONSTER && entity->def->eSubType != Enums::CORPSE_SKELETON) {
				this->watchLine = watchLine;
				return false;
			}
		}
		if (this->watchLine == watchLine) {
			this->watchLine = nullptr;
		}
		this->linkEntity(watchLine, watchLine->linkIndex % 32, watchLine->linkIndex / 32);
	}
	LerpSprite* allocLerpSprite = this->allocLerpSprite(scriptThread, sprite, true);
	app->render->mapSpriteInfo[sprite] = (n3 & 0xFFFEFFFF);
	if (b) {
		this->secretActive = true;
		allocLerpSprite->flags |= Enums::LS_FLAG_SECRET_OPEN;
	} else {
		allocLerpSprite->flags |= Enums::LS_FLAG_DOOROPEN;
		app->render->mapSpriteInfo[sprite] |= 0x80000000;
	}

	allocLerpSprite->dstX = allocLerpSprite->srcX = app->render->mapSprites[app->render->S_X + sprite];
	allocLerpSprite->dstY = allocLerpSprite->srcY = app->render->mapSprites[app->render->S_Y + sprite];
	allocLerpSprite->dstZ = allocLerpSprite->srcZ = app->render->mapSprites[app->render->S_Z + sprite];
	allocLerpSprite->dstScale = allocLerpSprite->srcScale =
	    app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];

	int n10 = 32;
	if (n == 1) {
		n10 = -n10;
	}
	if (0x0 != (n3 & 0x3000000)) {
		if (b) {
			if ((n3 & 0x1000000) != 0x0) {
				allocLerpSprite->dstY += n10;
			} else {
				allocLerpSprite->dstY -= n10;
			}
		} else if ((watchLine->def->parm & 0x1) == 0x0) {
			allocLerpSprite->dstX += n10;
		}
	} else if (0x0 != (n3 & 0xC000000)) {
		if (b) {
			if ((n3 & 0x8000000) != 0x0) {
				allocLerpSprite->dstX += n10;
			} else {
				allocLerpSprite->dstX -= n10;
			}
		} else if ((watchLine->def->parm & 0x1) == 0x0) {
			allocLerpSprite->dstY += n10;
		}
	}
	allocLerpSprite->startTime = app->gameTime;
	allocLerpSprite->travelTime = 750;
	allocLerpSprite->flags |= (Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_S_NORELINK);
	if (n == 1) {
		allocLerpSprite->flags &= ~(Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_DOOROPEN);
		allocLerpSprite->flags |= Enums::LS_FLAG_DOORCLOSE;
		allocLerpSprite->dstScale = 64;
		if (!b && b3) {
			this->updatePlayerDoors(watchLine, false);
		}
		app->sound->playSound(Sounds::getResIDByName(SoundName::DOOR_CLOSE), 0, 3, 0);
	} else if (n == 0 && !b) {
		if (b3) {
			this->updatePlayerDoors(watchLine, true);
		}
		allocLerpSprite->dstScale = 0;
		app->sound->playSound(Sounds::getResIDByName(SoundName::DOOR_OPEN), 0, 3, 0);
	}
	app->render->mapSprites[app->render->S_SCALEFACTOR + sprite] = (uint16_t)((uint8_t)allocLerpSprite->srcScale);
	app->render->mapSprites[app->render->S_X + sprite] = (short)allocLerpSprite->srcX;
	app->render->mapSprites[app->render->S_Y + sprite] = (short)allocLerpSprite->srcY;
	app->render->mapSprites[app->render->S_Z + sprite] = (short)allocLerpSprite->srcZ;
	if (b3 && !(allocLerpSprite->flags & Enums::LS_FLAG_DOORCLOSE)) {
		app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x100);
	}
	if (n2 == 0 || app->canvas->state == Canvas::ST_AUTOMAP ||
	    (n2 == 2 && app->render->cullBoundingBox(allocLerpSprite->srcX + allocLerpSprite->dstX >> 1,
	                                             allocLerpSprite->srcY + allocLerpSprite->dstY >> 1, true))) {
		this->snapLerpSprites(sprite);
	}
	return true;
}

void Game::lerpSpriteAsDoor(int n, int n2, ScriptThread* scriptThread) {


	LerpSprite* allocLerpSprite = this->allocLerpSprite(scriptThread, n, false);

	app->render->mapSpriteInfo[n] &= 0xFFFEFFFF;
	int n3 = app->render->mapSpriteInfo[n];
	int n4 = 64;
	if (n2 == 1) {
		n4 = -n4;
	}
	allocLerpSprite->flags |= Enums::LS_FLAG_DOOROPEN;
	app->render->mapSpriteInfo[n] |= 0x80000000;
	app->render->mapSprites[app->render->S_SCALEFACTOR + n] = 256;
	allocLerpSprite->dstX = allocLerpSprite->srcX = app->render->mapSprites[app->render->S_X + n];
	allocLerpSprite->dstY = allocLerpSprite->srcY = app->render->mapSprites[app->render->S_Y + n];
	allocLerpSprite->dstZ = allocLerpSprite->srcZ = app->render->mapSprites[app->render->S_Z + n];
	allocLerpSprite->dstScale = allocLerpSprite->srcScale = 64;
	if (0x0 != (n3 & 0x3000000)) {

		allocLerpSprite->dstX += n4;
	} else if (0x0 != (n3 & 0xC000000)) {
		allocLerpSprite->dstY += n4;
	}
	allocLerpSprite->startTime = app->gameTime;
	allocLerpSprite->travelTime = 750;
	allocLerpSprite->flags |= (Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_S_NORELINK);
	if (n2 == 1) {
		allocLerpSprite->flags &= ~(Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_DOOROPEN);
		allocLerpSprite->flags |= Enums::LS_FLAG_DOORCLOSE;
	}
	app->render->mapSprites[app->render->S_X + n] = (short)allocLerpSprite->srcX;
	app->render->mapSprites[app->render->S_Y + n] = (short)allocLerpSprite->srcY;
	app->render->mapSprites[app->render->S_Z + n] = (short)allocLerpSprite->srcZ;
}

void Game::updatePlayerDoors(Entity* entity, bool b) {
	bool b2 = false;
	int i;
	for (i = 0; i < 6; ++i) {
		if ((b && this->openDoors[i] == nullptr) || (!b && this->openDoors[i] == entity)) {
			b2 = true;
			break;
		}
	}
	if (b2) {
		if (b) {
			this->openDoors[i] = entity;
		} else {
			this->openDoors[i] = nullptr;
		}
	}
}

bool Game::CanCloseDoor(Entity* entity) {


	if (app->player->isFamiliar) {
		return false;
	}
	int n = (entity->linkIndex % 32 << 6) + 32;
	int n2 = (entity->linkIndex / 32 << 6) + 32;
	int sprite = entity->getSprite();
	if (this->findMapEntity(n, n2, 6) != nullptr) {
		return false;
	}
	int n3 = 64;
	int n4 = 64;
	if ((app->render->mapSpriteInfo[sprite] & 0x3000000) == 0x0) {
		n4 = 0;
	} else {
		n3 = 0;
	}
	return ((this->findMapEntity(n + n3, n2 + n4, 6) == nullptr) && (findMapEntity(n - n3, n2 - n4, 6) == nullptr));
}

void Game::setLineLocked(Entity* entity, bool b) {


	int sprite = entity->getSprite();
	int n = app->render->mapSpriteInfo[sprite] & 0xFF;
	int n2;
	if (b) {
		n2 = (n & 0xFFFFFFFE);
	} else {
		n2 = (n | 0x1);
	}
	app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFFFF00) | n2);
	n2 += 257;
	if (entity->def->name == (entity->name & 0x3FF)) {
		entity->def = app->entityDefManager->lookup(n2);
		entity->name = (short)(entity->def->name | 0x400);
	} else {
		entity->def = app->entityDefManager->lookup(n2);
	}
}
