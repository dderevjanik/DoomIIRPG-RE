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
#include "EventBus.h"
#include "GameEvents.h"
#include "ConfigEnums.h"

void Game::uncoverAutomapAt(int destX, int destY) {
	if (!this->updateAutomap) {
		return;
	}
	int tileX = destX >> 6;
	int tileY = destY >> 6;
	if (tileX < 0 || tileX >= 32 || tileY < 0 || tileY >= 32) {
		return;
	}
	for (int i = tileY - 1; i <= tileY + 1; ++i) {
		if (i >= 0) {
			if (i < 31) {
				for (int j = tileX - 1; j <= tileX + 1; ++j) {
					if (j >= 0) {
						if (j < 31) {
							uint8_t mapFlag = app->render->mapFlags[i * 32 + j];
							if ((j == tileX && i == tileY) || (mapFlag & 0x2) == 0x0) {
								app->render->mapFlags[i * 32 + j] |= (uint8_t)128;
							}
						}
					}
				}
			}
		}
	}
}

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

void Game::linkEntity(Entity* entity, int tileX, int tileY) {

	short linkIndex = (short)(tileY * 32 + tileX);

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

void Game::unlinkWorldEntity(int tileX, int tileY) {


	short linkIndex = (short)(tileY * 32 + tileX);
	Entity* nextOnTile = this->entityDb[linkIndex];

	Entity* entity = &this->entities[0];
	this->baseVisitedTiles[tileY] &= ~(1 << tileX);
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

void Game::linkWorldEntity(int tileX, int tileY) {


	short linkIndex = (short)(tileY * 32 + tileX);
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

	this->baseVisitedTiles[tileY] |= 1 << tileX;
	app->render->mapFlags[linkIndex] |= 0x1;
}

void Game::removeEntity(Entity* entity) {


	if ((entity->info & 0xFFFF) != 0x0) {
		app->render->setSpriteInfoFlag(entity->getSprite(), 0x10000);
	}
	if ((entity->info & 0x100000)) {
		this->unlinkEntity(entity);
	}
	app->player->facingEntity = nullptr;
}

void Game::trace(int startX, int startY, int endX, int endY, Entity* entity, int typeMask, int radius) {
	this->trace(startX, startY, -1, endX, endY, -1, entity, typeMask, radius, false);
}

void Game::trace(int startX, int startY, int startZ, int traceCollisionX, int traceCollisionY, int traceCollisionZ, Entity* entity,
                 int typeMask, int radius, bool checkZ) {


	int* tracePoints = this->tracePoints;
	int* traceBoundingBox = this->traceBoundingBox;
	int* traceFracs = this->traceFracs;
	Entity** traceEntities = this->traceEntities;
	this->traceEntity = nullptr;
	this->numTraceEntities = 0;
	tracePoints[0] = startX;
	tracePoints[1] = startY;
	tracePoints[2] = traceCollisionX;
	tracePoints[3] = traceCollisionY;
	traceBoundingBox[0] = std::max(std::min(startX - radius, traceCollisionX - radius), 0);
	traceBoundingBox[1] = std::max(std::min(startY - radius, traceCollisionY - radius), 0);
	traceBoundingBox[2] = std::min(std::max(startX + radius, traceCollisionX + radius), 2047);
	traceBoundingBox[3] = std::min(std::max(startY + radius, traceCollisionY + radius), 2047);
	for (int i = traceBoundingBox[0] >> 6; i < (traceBoundingBox[2] >> 6) + 1; ++i) {
		for (int j = traceBoundingBox[1] >> 6; j < (traceBoundingBox[3] >> 6) + 1; ++j) {
			Entity* nextOnTile = this->entityDb[i + 32 * j];
			if (nextOnTile != nullptr) {
				while (nextOnTile != nullptr) {
					if (nextOnTile != entity && nextOnTile->def != nullptr && 0x0 != (typeMask & 1 << nextOnTile->def->eType)) {
						if (nextOnTile->def->eType != Enums::ET_WORLD) {
							int sprite = nextOnTile->getSprite();
							int destX;
							int destY;
							int destZ;
							if (nextOnTile->ai != nullptr) {
								if ((nextOnTile->ai->goalFlags & 0x1) != 0x0) {
									destX = 32 + (nextOnTile->ai->goalX << 6);
									destY = 32 + (nextOnTile->ai->goalY << 6);
									destZ = app->render->getSpriteZ(sprite);
								} else {
									destX = app->render->getSpriteX(sprite);
									destY = app->render->getSpriteY(sprite);
									destZ = app->render->getSpriteZ(sprite);
								}
							} else if (nextOnTile->def->eType == Enums::ET_PLAYER) {
								destX = app->canvas->destX;
								destY = app->canvas->destY;
								destZ = app->canvas->destZ;
							} else {
								destX = app->render->getSpriteX(sprite);
								destY = app->render->getSpriteY(sprite);
								destZ = app->render->getSpriteZ(sprite);
							}
							if (sprite != -1 && 0x0 != (app->render->getSpriteInfoRaw(sprite) & 0xF000000)) {
								int bbX2 = destX;
								int bbY2 = destY;
								if (0x0 != (app->render->getSpriteInfoRaw(sprite) & 0x3000000)) {
									destX -= 32;
									bbX2 += 32;
								} else {
									destY -= 32;
									bbY2 += 32;
								}
								app->render->traceLine[0] = destX;
								app->render->traceLine[1] = destY;
								app->render->traceLine[2] = bbX2;
								app->render->traceLine[3] = bbY2;
								int capsuleToLineTrace =
								    app->render->CapsuleToLineTrace(tracePoints, radius * radius, app->render->traceLine);
								if (capsuleToLineTrace < 16384) {
									traceFracs[this->numTraceEntities] = capsuleToLineTrace;
									traceEntities[this->numTraceEntities++] = nextOnTile;
								}
							} else {
								int circleRadiusSq = 625;
								if (nextOnTile->def->eType == Enums::ET_ENV_DAMAGE) {
									circleRadiusSq = 256;
								}
								int capsuleToCircleTrace =
								    app->render->CapsuleToCircleTrace(tracePoints, radius * radius, destX, destY, circleRadiusSq);
								if (capsuleToCircleTrace < 16384) {
									if (startZ >= 0 && traceCollisionZ >= 0 && checkZ) {
										int relZ = startZ + ((traceCollisionZ - startZ) * capsuleToCircleTrace >> 14) - destZ;
										if (relZ > -(32 + radius) && relZ < 32 + radius) {
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
	if (0x0 != (typeMask & 0x1)) {
		int traceWorld = app->render->traceWorld(0, tracePoints, radius, traceBoundingBox, typeMask);
		if (traceWorld < 16384) {
			traceFracs[this->numTraceEntities] = traceWorld;
			traceEntities[this->numTraceEntities++] = &this->entities[0];
			this->traceCollisionX = startX + ((traceWorld * (traceCollisionX - startX)) >> 14);
			this->traceCollisionY = startY + ((traceWorld * (traceCollisionY - startY)) >> 14);
			this->traceCollisionZ = startZ + ((traceWorld * (traceCollisionZ - startZ)) >> 14);
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
					int tmpFrac = traceFracs[l];
					traceFracs[l] = traceFracs[l + 1];
					traceFracs[l + 1] = tmpFrac;
					Entity* entity2 = traceEntities[l];
					traceEntities[l] = traceEntities[l + 1];
					traceEntities[l + 1] = entity2;
				}
			}
		}
		this->traceEntity = traceEntities[0];
	}
}

bool Game::touchTile(int worldX, int worldY, bool touchAll) {
	bool touched = false;
	Entity* nextOnTile;
	// printf("touchTile %d, %d\n", worldX, worldY);
	for (Entity* mapEntity = this->findMapEntity(worldX, worldY); mapEntity != nullptr; mapEntity = nextOnTile) {
		nextOnTile = mapEntity->nextOnTile;
		if (touchAll || mapEntity->def->eType == Enums::ET_ENV_DAMAGE) {
			mapEntity->touched();
			touched = true;
		}
	}
	return touched;
}

Entity* Game::findMapEntity(int worldX, int worldY) {
	worldX >>= 6;
	worldY >>= 6;
	if (worldX < 0 || worldX >= 32 || worldY < 0 || worldY >= 32) {
		return nullptr;
	}
	return this->entityDb[worldY * 32 + worldX];
}

Entity* Game::findMapEntity(int worldX, int worldY, int typeMask) {


	if ((typeMask & 0x2) != 0x0 && worldX >> 6 == app->canvas->destX >> 6 && worldY >> 6 == app->canvas->destY >> 6) {
		return &this->entities[1];
	}
	for (Entity* entity = this->findMapEntity(worldX, worldY); entity != nullptr; entity = entity->nextOnTile) {
		if ((typeMask & 1 << entity->def->eType) != 0x0) {
			return entity;
		}
	}
	return nullptr;
}

bool Game::performDoorEvent(int action, Entity* entity, int snapMode) {
	return this->performDoorEvent(action, entity, snapMode, false);
}

bool Game::performDoorEvent(int action, Entity* entity, int snapMode, bool isSecret) {
	return this->performDoorEvent(action, nullptr, entity, snapMode, isSecret);
}

bool Game::performDoorEvent(int action, ScriptThread* scriptThread, Entity* watchLine, int snapMode, bool isSecret) {


	int sprite = watchLine->getSprite();
	int spriteInfo = app->render->getSpriteInfoRaw(sprite);
	int doorState = spriteInfo & 0xFF;
	bool unblocked = (watchLine->info & 0x100000) == 0x0;
	if ((spriteInfo & 0x400000) != 0x0) {
		doorState += 257;
	}

	bool isAnimatedDoor = doorState >= 271 && doorState < 281;
	if (watchLine->def->eSubType == Enums::DOOR_LOCKED) {
		return false;
	}
	if (action == 0 && unblocked && isAnimatedDoor) {
		this->updatePlayerDoors(watchLine, true);
		return false;
	}
	if (action == 1 && !unblocked) {
		return false;
	}
	if (unblocked && isAnimatedDoor) {
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
	app->render->setSpriteInfoRaw(sprite, (spriteInfo & 0xFFFEFFFF));
	if (isSecret) {
		this->secretActive = true;
		allocLerpSprite->flags |= Enums::LS_FLAG_SECRET_OPEN;
	} else {
		allocLerpSprite->flags |= Enums::LS_FLAG_DOOROPEN;
		app->render->setSpriteInfoFlag(sprite, 0x80000000);
	}

	allocLerpSprite->dstX = allocLerpSprite->srcX = app->render->getSpriteX(sprite);
	allocLerpSprite->dstY = allocLerpSprite->srcY = app->render->getSpriteY(sprite);
	allocLerpSprite->dstZ = allocLerpSprite->srcZ = app->render->getSpriteZ(sprite);
	allocLerpSprite->dstScale = allocLerpSprite->srcScale =
	    app->render->getSpriteScaleFactor(sprite);

	int slideDist = 32;
	if (action == 1) {
		slideDist = -slideDist;
	}
	if (0x0 != (spriteInfo & 0x3000000)) {
		if (isSecret) {
			if ((spriteInfo & 0x1000000) != 0x0) {
				allocLerpSprite->dstY += slideDist;
			} else {
				allocLerpSprite->dstY -= slideDist;
			}
		} else if ((watchLine->def->parm & 0x1) == 0x0) {
			allocLerpSprite->dstX += slideDist;
		}
	} else if (0x0 != (spriteInfo & 0xC000000)) {
		if (isSecret) {
			if ((spriteInfo & 0x8000000) != 0x0) {
				allocLerpSprite->dstX += slideDist;
			} else {
				allocLerpSprite->dstX -= slideDist;
			}
		} else if ((watchLine->def->parm & 0x1) == 0x0) {
			allocLerpSprite->dstY += slideDist;
		}
	}
	allocLerpSprite->startTime = app->gameTime;
	allocLerpSprite->travelTime = 750;
	allocLerpSprite->flags |= (Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_S_NORELINK);
	if (action == 1) {
		allocLerpSprite->flags &= ~(Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_DOOROPEN);
		allocLerpSprite->flags |= Enums::LS_FLAG_DOORCLOSE;
		allocLerpSprite->dstScale = 64;
		if (!isSecret && isAnimatedDoor) {
			this->updatePlayerDoors(watchLine, false);
		}
		app->sound->playSound(Sounds::getResIDByName(SoundName::DOOR_CLOSE), 0, 3, 0);
	} else if (action == 0 && !isSecret) {
		if (isAnimatedDoor) {
			this->updatePlayerDoors(watchLine, true);
		}
		allocLerpSprite->dstScale = 0;
		app->sound->playSound(Sounds::getResIDByName(SoundName::DOOR_OPEN), 0, 3, 0);
	}
	app->render->setSpriteScaleFactor(sprite, (uint16_t)((uint8_t)allocLerpSprite->srcScale));
	app->render->setSpriteX(sprite, (short)allocLerpSprite->srcX);
	app->render->setSpriteY(sprite, (short)allocLerpSprite->srcY);
	app->render->setSpriteZ(sprite, (short)allocLerpSprite->srcZ);
	if (isAnimatedDoor && !(allocLerpSprite->flags & Enums::LS_FLAG_DOORCLOSE)) {
		app->render->setSpriteInfoRaw(sprite, ((app->render->getSpriteInfoRaw(sprite) & 0xFFFF00FF) | 0x100));
	}
	if (snapMode == 0 || app->canvas->state == Canvas::ST_AUTOMAP ||
	    (snapMode == 2 && app->render->cullBoundingBox(allocLerpSprite->srcX + allocLerpSprite->dstX >> 1,
	                                             allocLerpSprite->srcY + allocLerpSprite->dstY >> 1, true))) {
		this->snapLerpSprites(sprite);
	}
	app->eventBus->emit(DoorEvent{watchLine, (action == 0), (int)allocLerpSprite->srcX, (int)allocLerpSprite->srcY});
	return true;
}

void Game::lerpSpriteAsDoor(int sprite, int action, ScriptThread* scriptThread) {


	LerpSprite* allocLerpSprite = this->allocLerpSprite(scriptThread, sprite, false);

	app->render->setSpriteInfoRaw(sprite, app->render->getSpriteInfoRaw(sprite) & 0xFFFEFFFF);
	int spriteInfo = app->render->getSpriteInfoRaw(sprite);
	int slideDist = 64;
	if (action == 1) {
		slideDist = -slideDist;
	}
	allocLerpSprite->flags |= Enums::LS_FLAG_DOOROPEN;
	app->render->setSpriteInfoFlag(sprite, 0x80000000);
	app->render->setSpriteScaleFactor(sprite, 256);
	allocLerpSprite->dstX = allocLerpSprite->srcX = app->render->getSpriteX(sprite);
	allocLerpSprite->dstY = allocLerpSprite->srcY = app->render->getSpriteY(sprite);
	allocLerpSprite->dstZ = allocLerpSprite->srcZ = app->render->getSpriteZ(sprite);
	allocLerpSprite->dstScale = allocLerpSprite->srcScale = 64;
	if (0x0 != (spriteInfo & 0x3000000)) {

		allocLerpSprite->dstX += slideDist;
	} else if (0x0 != (spriteInfo & 0xC000000)) {
		allocLerpSprite->dstY += slideDist;
	}
	allocLerpSprite->startTime = app->gameTime;
	allocLerpSprite->travelTime = 750;
	allocLerpSprite->flags |= (Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_S_NORELINK);
	if (action == 1) {
		allocLerpSprite->flags &= ~(Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_DOOROPEN);
		allocLerpSprite->flags |= Enums::LS_FLAG_DOORCLOSE;
	}
	app->render->setSpriteX(sprite, (short)allocLerpSprite->srcX);
	app->render->setSpriteY(sprite, (short)allocLerpSprite->srcY);
	app->render->setSpriteZ(sprite, (short)allocLerpSprite->srcZ);
}

void Game::updatePlayerDoors(Entity* entity, bool addDoor) {
	bool foundSlot = false;
	int i;
	for (i = 0; i < 6; ++i) {
		if ((addDoor && this->openDoors[i] == nullptr) || (!addDoor && this->openDoors[i] == entity)) {
			foundSlot = true;
			break;
		}
	}
	if (foundSlot) {
		if (addDoor) {
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
	int centerX = (entity->linkIndex % 32 << 6) + 32;
	int centerY = (entity->linkIndex / 32 << 6) + 32;
	int sprite = entity->getSprite();
	if (this->findMapEntity(centerX, centerY, 6) != nullptr) {
		return false;
	}
	int offsetX = 64;
	int offsetY = 64;
	if ((app->render->getSpriteInfoRaw(sprite) & 0x3000000) == 0x0) {
		offsetY = 0;
	} else {
		offsetX = 0;
	}
	return ((this->findMapEntity(centerX + offsetX, centerY + offsetY, 6) == nullptr) && (findMapEntity(centerX - offsetX, centerY - offsetY, 6) == nullptr));
}

void Game::setLineLocked(Entity* entity, bool lock) {


	int sprite = entity->getSprite();
	int lineFlags = app->render->getSpriteInfoRaw(sprite) & 0xFF;
	int newFlags;
	if (lock) {
		newFlags = (lineFlags & 0xFFFFFFFE);
	} else {
		newFlags = (lineFlags | 0x1);
	}
	app->render->setSpriteInfoRaw(sprite, ((app->render->getSpriteInfoRaw(sprite) & 0xFFFFFF00) | newFlags));
	newFlags += 257;
	if (entity->def->name == (entity->name & 0x3FF)) {
		entity->def = app->entityDefManager->lookup(newFlags);
		entity->name = (short)(entity->def->name | 0x400);
	} else {
		entity->def = app->entityDefManager->lookup(newFlags);
	}
}
