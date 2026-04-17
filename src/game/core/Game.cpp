#include <stdexcept>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <sys/stat.h>
#include "Log.h"
#include "DataNode.h"

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
#include "DialogManager.h"
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
#include "EventBus.h"
#include "GameEvents.h"

Game::Game() = default;

Game::~Game() {}

bool Game::startup() {
	this->app = CAppContainer::getInstance()->app;
	this->gameConfig = &CAppContainer::getInstance()->gameConfig;
	this->sdlGL = CAppContainer::getInstance()->sdlGL;
	Applet* app = this->app;
	LOG_INFO("[game] startup\n");

	this->maxEntities = this->gameConfig->maxEntities;
	this->entities = new Entity[this->maxEntities];
	this->ofsMayaTween = new short[6];

	this->difficulty = 1;
	this->cinematicWeapon = -1;

	for (int i = 0; i < this->maxEntities; i++) {
		this->entities[i].app = app;
		this->entities[i].gameConfig = this->gameConfig;
	}

	for (int i = 0; i < 20; i++) {
		this->scriptThreads[i].app = app;
	}

	for (int i = 0; i < 128; i++) {
		this->entityDeathFunctions[i] = -1;
	}

	this->activeMonsters = nullptr;
	this->combatMonsters = nullptr;
	this->inactiveMonsters = nullptr;
	this->mapSecretsFound = 0;
	this->totalSecrets = 0;
	this->keycode[0] = '\0';
	this->gotoTriggered = false;
	this->disableAI = false;
	this->skipMinigames = false;
	this->activeSprites = 0;
	this->activePropogators = 0;
	this->skipDialog = false;

	return true;
}

void Game::loadMapEntities() {

	LOG_INFO("[game] loadMapEntities: mapId={}\n", app->canvas->loadMapID);
	app->eventBus->emit(LevelLoadEvent{app->canvas->loadMapID, (app->canvas->loadType == 0)});
	this->interpolatingMonsters = false;
	this->monstersTurn = 0;
	for (int i = 0; i < 4; ++i) {
		this->openDoors[i] = nullptr;
	}
	this->watchLine = nullptr;
	app->player->setPickUpWeapon(15);
	for (int j = 0; j < 4; ++j) {
		this->placedBombs[j] = 0;
	}
	for (int k = 0; k < 275; ++k) {
		this->entities[k].reset();
	}
	for (int l = 0; l < 8; ++l) {
		this->levelVars[l] = 0;
	}
	for (int n = 0; n < 32; ++n) {
		this->baseVisitedTiles[n] = 0;
	}
	this->secretActive = false;
	this->numMonsters = 0;
	this->numAIComponents = 0;
	this->numLootComponents = 0;
	this->numLerpSprites = 0;
	for (int n2 = 0; n2 < 16; ++n2) {
		this->lerpSprites[n2].hSprite = 0;
	}
	this->numScriptThreads = 0;
	for (int n3 = 0; n3 < 20; ++n3) {
		this->scriptThreads[n3].reset();
	}
	this->numEntities = 0;
	Entity* entity = &this->entities[this->numEntities++];
	entity->def = app->entityDefManager->find(0, 0);
	entity->name = (short)(entity->def->name | 0x400);

	Entity* entity2 = &this->entities[this->numEntities++];
	entity2->def = app->entityDefManager->find(1, 0);
	entity2->name = (short)(entity2->def->name | 0x400);
	entity2->info = 0x20000;

	EntityDef* find = app->entityDefManager->find(12, 0);
	EntityDef* find2 = app->entityDefManager->find(13, 0);
	int n4 = 0;
	for (int n5 = 0; n5 < app->render->numMapSprites; ++n5) {
		int n6 = app->render->getSpriteInfoRaw(n5) & 0xFF;
		if ((app->render->getSpriteInfoRaw(n5) & 0x400000) != 0x0) {
			n6 += 257;
		}
		int n7 = app->render->mediaMappings[n6 + 1] - app->render->mediaMappings[n6];
		if (n6 == 234) {
			n7 = 4;
		}
		if (n6 == 156) {
			n7 = 2;
			app->render->setSpriteInfoRaw(n5, app->render->getSpriteInfoRaw(n5) & 0xFFFF00FF);
			app->render->setSpriteInfoFlag(n5, (n7 << 8 | 0x80200));
		}
		if (n6 == 236) {
			n7 = 3;
			app->render->setSpriteInfoRaw(n5, app->render->getSpriteInfoRaw(n5) & 0xFFFF00FF);
			app->render->setSpriteInfoFlag(n5, (n7 << 8 | 0x80300));
			app->render->setSpriteRenderMode(n5, 3);
		}
		if (n6 == 136 || n6 == 234 || n6 == 130) {
			app->render->setSpriteInfoRaw(n5, app->render->getSpriteInfoRaw(n5) & 0xFFFF00FF);
			app->render->setSpriteInfoFlag(n5, (n7 << 8 | 0x80000));
		}
		if ((app->render->getSpriteInfoRaw(n5) & 0x200000) != 0x0) {
			app->render->setSpriteInfoRaw(n5, app->render->getSpriteInfoRaw(n5) & 0xFFDFFFFF);
		} else {
			int n15 = app->render->getSpriteX(n5);
			int n16 = app->render->getSpriteY(n5);
			EntityDef* lookup = app->entityDefManager->lookup(n6);
			if (n6 >= 1 && n6 <= 12) {
				app->render->setSpriteInfoFlag(n5, 0x200);
			}
			if ((app->render->getSpriteInfoRaw(n5) & 0xF000000) != 0x0 && ((n15 & 0x3F) == 0x0 || (n16 & 0x3F) == 0x0)) {
				if ((app->render->getSpriteInfoRaw(n5) & 0x4000000) != 0x0) {
					++n15;
				} else if ((app->render->getSpriteInfoRaw(n5) & 0x2000000) != 0x0) {
					++n16;
				} else if ((app->render->getSpriteInfoRaw(n5) & 0x1000000) != 0x0) {
					--n16;
				} else if ((app->render->getSpriteInfoRaw(n5) & 0x8000000) != 0x0) {
					--n15;
				}
			}
			if (lookup != nullptr) {
				if (this->numEntities == this->maxEntities) {
					app->Error(35); // ERR_MAX_ENTITIES
					return;
				}
				Entity* entity3 = &this->entities[this->numEntities];
				entity3->info = (n5 + 1 & 0xFFFF);
				entity3->def = lookup;
				if (entity3->def->eType == Enums::ET_MONSTER) {
					if (this->numMonsters == 80) {
						app->Error(37); // ERR_MAX_MONSTERS
						return;
					}
					entity3->monster = &this->entityMonsters[this->numMonsters++];
					entity3->combat = &entity3->monster->ce;
					entity3->ai = &this->aiComponents[this->numAIComponents++];
					// EntityMonster pool slot initialized by memset in constructor
					entity3->ai->reset();

					app->render->setSpriteInfoRaw(n5, app->render->getSpriteInfoRaw(n5) & 0xF0FFFFFF);
					if ((app->nextByte() & 0x1) == 0x0 && !entity3->isBoss()) {
						app->render->setSpriteInfoFlag(n5, 0x20000);
					}
					entity3->info |= 0x40000;
					this->deactivate(entity3);

					app->sound->cacheCombatSound(
					    this->getMonsterSound(lookup->monsterIdx, Enums::MSOUND_ATTACK1));
					app->sound->cacheCombatSound(
					    this->getMonsterSound(lookup->monsterIdx, Enums::MSOUND_ATTACK2));
				} else if (entity3->def->eType == Enums::ET_ATTACK_INTERACTIVE && entity3->def->eSubType != Enums::INTERACT_PICKUP && entity3->def->eSubType != Enums::INTERACT_CRATE) {
					this->numDestroyableObj++;
				}
				entity3->initspawn();
				app->render->setSpriteEnt(n5, this->numEntities++);
				if ((app->render->getSpriteInfoRaw(n5) & 0x10000) == 0x0) {
					this->linkEntity(entity3, n15 >> 6, n16 >> 6);
				}
				if (n6 >= 140 && n6 <= 143) {
					app->render->setSpriteInfoFlag(n5, 0x10000);
				}
				++n4;
			} else if ((app->render->getSpriteInfoRaw(n5) & 0x800000) != 0x0) {
				if (this->numEntities == this->maxEntities) {
					app->Error(35); // ERR_MAX_ENTITIES
					return;
				}
				Entity* entity5 = &this->entities[this->numEntities];
				entity5->info = (n5 + 1 & 0xFFFF);
				if (n6 == 166 || n6 == 168) {
					entity5->def = find2;
				} else {
					entity5->def = find;
				}
				entity5->name = (short)(entity5->def->name | 0x400);

				app->render->setSpriteEnt(n5, this->numEntities++);
				if ((app->render->getSpriteInfoRaw(n5) & 0x10000) == 0x0) {
					this->linkEntity(entity5, n15 >> 6, n16 >> 6);
				}
				++n4;
			}
			if ((n5 & 0xF) == 0xF) {
				app->canvas->updateLoadingBar(false);
			}
		}
	}
	this->firstDropIndex = this->numEntities;
	for (int n23 = 0; n23 < 16; ++n23) {
		if (this->numEntities == this->maxEntities) {
			app->Error(35); // ERR_MAX_ENTITIES
			return;
		}
		this->entities[this->numEntities++].info = (app->render->firstDropSprite + n23 + 1 & 0xFFFF);
		app->render->setSpriteInfoFlag(app->render->dropSprites[n23], 0x10000);
		++n4;
	}
	for (int n25 = 0; n25 < 1024; ++n25) {
		if ((app->render->mapFlags[n25] & 0x1) != 0x0) {
			Entity* nextOnTile = this->entityDb[n25];
			bool b = true;
			while (nextOnTile != nullptr) {
				nextOnTile = nextOnTile->nextOnTile;
			}
			if (b) {
				this->linkWorldEntity(n25 % 32, n25 / 32);
			}
		}
	}
	LOG_INFO("[game] loadMapEntities: loaded {} entities, {} monsters\n", this->numEntities, this->numMonsters);
}

void Game::loadTableCamera(int i, int i2) {

	this->cleanUpCamMemory();

	// Try loading from YAML first (data/intro_camera.yaml)
	DataNode camYaml = DataNode::loadFile("data/intro_camera.yaml");
	if (camYaml) {
		this->mayaCameras = new MayaCamera[1];
		this->mayaCameras[0].isTableCam = true;
		this->totalMayaCameras = 1;

		this->totalMayaCameraKeys = camYaml["total_keys"].asInt(0);
		this->mayaCameras[0].sampleRate = camYaml["sample_rate"].asInt(0);
		this->posShift = 4 - camYaml["pos_shift"].asInt(0);
		this->angleShift = 10 - camYaml["angle_shift"].asInt(0);

		// Keyframes: totalKeys * 7 values
		DataNode kfNode = camYaml["keyframes"];
		int keySize = this->totalMayaCameraKeys * 7;
		this->mayaCameraKeys = new int16_t[keySize];
		if (kfNode && kfNode.isSequence()) {
			int idx = 0;
			for (int k = 0; k < (int)kfNode.size() && k < this->totalMayaCameraKeys; k++) {
				DataNode row = kfNode[k];
				for (int c = 0; c < 7 && c < (int)row.size(); c++) {
					this->mayaCameraKeys[idx++] = (int16_t)row[c].asInt(0);
				}
			}
		}

		// Tween indices: totalKeys * 6 values
		DataNode tiNode = camYaml["tween_indices"];
		int tweenIdxSize = this->totalMayaCameraKeys * 6;
		this->mayaTweenIndices = new int16_t[tweenIdxSize];
		if (tiNode && tiNode.isSequence()) {
			int idx = 0;
			for (int k = 0; k < (int)tiNode.size() && k < this->totalMayaCameraKeys; k++) {
				DataNode row = tiNode[k];
				for (int c = 0; c < 6 && c < (int)row.size(); c++) {
					this->mayaTweenIndices[idx++] = (int16_t)row[c].asInt(0);
				}
			}
		}

		// Tween counts → cumulative offsets
		DataNode tcNode = camYaml["tween_counts"];
		this->ofsMayaTween[0] = 0;
		if (tcNode && tcNode.isSequence()) {
			short cumulative = 0;
			for (int c = 0; c < 5 && c < (int)tcNode.size(); c++) {
				cumulative += (short)tcNode[c].asInt(0);
				this->ofsMayaTween[c + 1] = cumulative;
			}
		}

		// Tween data (signed bytes)
		DataNode twNode = camYaml["tweens"];
		if (twNode && twNode.isSequence()) {
			int cnt = (int)twNode.size();
			this->mayaCameraTweens = new int8_t[cnt];
			for (int t = 0; t < cnt; t++) {
				this->mayaCameraTweens[t] = (int8_t)twNode[t].asInt(0);
			}
		}

		this->mayaCameras[0].keyOffset = 0;
		this->mayaCameras[0].numKeys = this->totalMayaCameraKeys;
		this->mayaCameras[0].complete = false;
		this->mayaCameras[0].keyThreadResumeCount = this->totalMayaCameraKeys;
		this->setKeyOffsets();

		LOG_INFO("[game] Loaded intro camera from YAML ({} keys)\n", this->totalMayaCameraKeys);
		return;
	}

	app->Error("data/intro_camera.yaml not found\n");
}

void Game::setKeyOffsets() {
	this->OFS_MAYAKEY_X = 0;
	this->OFS_MAYAKEY_Y = this->totalMayaCameraKeys;
	this->OFS_MAYAKEY_Z = this->totalMayaCameraKeys * 2;
	this->OFS_MAYAKEY_PITCH = this->totalMayaCameraKeys * 3;
	this->OFS_MAYAKEY_YAW = this->totalMayaCameraKeys * 4;
	this->OFS_MAYAKEY_ROLL = this->totalMayaCameraKeys * 5;
	this->OFS_MAYAKEY_MS = this->totalMayaCameraKeys * 6;
}

void Game::loadMayaCameras(InputStream* IS) {


	int keyOffset = 0;
	int n = 0;
	short array[6];
	int array2[6];

	for (int i = 0; i < 6; i++) {
		array[i] = 0;
		array2[i] = 0;
	}

	for (int i = 0; i < this->totalMayaCameras; i++) {
		app->resource->read(IS, 3);
		this->mayaCameras[i].numKeys = app->resource->shiftByte();
		this->mayaCameras[i].sampleRate = app->resource->shiftShort();
		app->resource->read(IS, this->mayaCameras[i].numKeys * 7 * sizeof(short));
		this->mayaCameras[i].keyOffset = keyOffset;

		for (int j = 0; j < 7; j++) {
			for (int k = 0; k < this->mayaCameras[i].numKeys; k++) {
				this->mayaCameraKeys[this->totalMayaCameraKeys * j + keyOffset + k] = app->resource->shiftShort();
			}
		}

		keyOffset += this->mayaCameras[i].numKeys;
		int n2 = this->mayaCameras[i].numKeys * 6 * sizeof(short);
		app->resource->read(IS, n2);
		for (int l = 0; l < n2 / 2; l++, n++) {
			short shiftShort = app->resource->shiftShort();
			if (shiftShort >= 0) {
				shiftShort += array[l % 6];
			}
			this->mayaTweenIndices[n] = shiftShort;
		}

		int n3 = 0;
		app->resource->read(IS, 6 * sizeof(short));
		for (int n4 = 0; n4 < 6; n4++) {
			array2[n4] = app->resource->shiftShort();
			n3 += array2[n4];
		}

		app->resource->readMarker(IS, 0xDEADBEEF);
		app->resource->read(IS, n3);
		for (int n5 = 0; n5 < 6; ++n5) {
			short* array3;
			int n7;
			for (int n6 = 0; n6 < array2[n5]; ++n6, array3 = array, n7 = n5, ++array3[n7]) {
				this->mayaCameraTweens[this->ofsMayaTween[n5] + array[n5]] = app->resource->shiftByte();
			}
		}
	}
}

void Game::unloadMapData() {


	this->cinematicWeapon = -1;
	this->activeSprites = 0;
	this->animatingEffects = 0;
	this->activePropogators = 0;
	this->activeCameraView = false;
	this->activeCamera = nullptr;
	app->player->facingEntity = nullptr;
	app->combat->curTarget = nullptr;
	app->combat->curAttacker = nullptr;
	app->hud->lastTarget = nullptr;
	for (int i = 0; i < 48; ++i) {
		if ((this->gsprites[i].flags & 0x1)) {
			this->gsprite_destroy(&gsprites[i]);
		}
	}
	this->inactiveMonsters = nullptr;
	this->activeMonsters = nullptr;
	this->combatMonsters = nullptr;
	this->keycode[0] = '\0';

	for (int j = 0; j < 1024; ++j) {
		this->entityDb[j] = nullptr;
	}
	for (int k = 0; k < this->numEntities; ++k) {
		this->entities[k].reset();
	}
	for (int l = 0; l < 128; ++l) {
		this->entityDeathFunctions[l] = -1;
	}
	this->totalMayaCameraKeys = 0;
	this->totalMayaCameras = 0;
	this->cleanUpCamMemory();
	app->combat->lerpWpDown = false;
	app->combat->weaponDown = false;
	this->numDestroyableObj = 0;
	this->destroyedObj = 0;
	this->lootFound = 0;
	app->dialogManager->showingLoot = false;
	this->angryVIOS = false;
	app->sound->freeMonsterSounds();
}

void Game::UpdatePlayerVars() {


	this->viewX = app->canvas->viewX;
	this->viewY = app->canvas->viewY;
	this->viewAngle = app->canvas->viewAngle;
	this->destX = app->canvas->destX;
	this->destY = app->canvas->destY;
	this->destAngle = app->canvas->destAngle;
	this->viewSin = app->canvas->viewSin;
	this->viewCos = app->canvas->viewCos;
	this->viewStepX = app->canvas->viewStepX;
	this->viewStepY = app->canvas->viewStepY;
	this->viewRightStepX = app->canvas->viewRightStepX;
	this->viewRightStepY = app->canvas->viewRightStepY;
}

void Game::spawnPlayer() {

	LOG_INFO("[game] spawnPlayer: mapId={}\n", app->canvas->loadMapID);
	int n;
	int n2;
	int mapSpawnDir;
	if (this->spawnParam == 0) {
		if (app->canvas->loadType == 3) {
			n = 3;
			n2 = 15;
			mapSpawnDir = 6;
		} else {
			n = app->render->mapSpawnIndex % 32;
			n2 = app->render->mapSpawnIndex / 32;
			mapSpawnDir = app->render->mapSpawnDir;
		}
	} else {
		n = (this->spawnParam & 0x1F);
		n2 = (this->spawnParam >> 5 & 0x1F);
		mapSpawnDir = (this->spawnParam >> 10 & 0xFF);
		this->spawnParam = 0;
	}
	int destAngle = mapSpawnDir << 7 & 0x3FF;
	app->canvas->viewX = (app->canvas->destX = n * 64 + 32);
	app->canvas->viewY = (app->canvas->destY = n2 * 64 + 32);
	app->canvas->viewZ = (app->canvas->destZ = app->render->getHeight(app->canvas->viewX, app->canvas->viewY) + 36);
	app->canvas->viewAngle = (app->canvas->destAngle = destAngle);
	app->player->relink();
	this->lastTurnTime = app->time;
	app->canvas->invalidateRect();
	app->eventBus->emit(LevelLoadCompleteEvent{app->canvas->loadMapID, this->numEntities});
}

void Game::eventFlagsForMovement(int n, int n2, int n3, int n4) {
	int n5 = n3 - n;
	int n6 = n4 - n2;
	this->eventFlags[0] = 2;
	this->eventFlags[1] = 1;
	this->eventFlags[0] |= this->eventFlagForDirection(n5, n6);
	this->eventFlags[1] |= this->eventFlagForDirection(-n5, -n6);
}

int Game::eventFlagForDirection(int n, int n2) {
	int n3;
	if (n > 0) {
		if (n2 < 0) {
			n3 = 32;
		} else if (n2 > 0) {
			n3 = 2048;
		} else {
			n3 = 16;
		}
	} else if (n < 0) {
		if (n2 < 0) {
			n3 = 128;
		} else if (n2 > 0) {
			n3 = 512;
		} else {
			n3 = 256;
		}
	} else if (n2 > 0) {
		n3 = 1024;
	} else {
		n3 = 64;
	}
	return n3;
}

void Game::givemap(int n, int n2, int n3, int n4) {


	for (int i = 0; i < app->render->numLines; ++i) {
		app->render->lineFlags[i >> 1] |= (uint8_t)(8 << ((i & 0x1) << 2));
	}
	for (int numMapSprites = app->render->numMapSprites, j = 0; j < numMapSprites; ++j) {
		int n6 = app->render->getSpriteX(j) >> 6;
		int n7 = app->render->getSpriteY(j) >> 6;
		if (n6 >= n && n6 < n + n3 && n7 >= n2 && n7 < n2 + n4) {
			app->render->setSpriteInfoFlag(j, 0x200000);
		}
	}
	for (int k = n2; k < n2 + n4; ++k) {
		for (int l = n; l < n + n3; ++l) {
			uint8_t b = app->render->mapFlags[k * 32 + l];
			if ((b & 0x1) == 0x0) {
				app->render->mapFlags[k * 32 + l] = (uint8_t)(b | 0x80);
			}
		}
	}
}

void Game::advanceTurn() {

	LOG_INFO("[game] advanceTurn: playerMoves={} hp={}/{} pos=({},{})\n",
		app->player->totalMoves,
		app->player->ce->getStat(Enums::STAT_HEALTH),
		app->player->ce->getStat(Enums::STAT_MAX_HEALTH),
		app->canvas->viewX, app->canvas->viewY);
	this->queueAdvanceTurn = false;
	if (this->interpolatingMonsters) {
		app->Error(95); // ERR_NONSNAPPEDMONSTERS
	}
	bool b = false;
	if (app->player->statusEffects[2] > 0) {
		++app->player->statusEffects[20];
		if (app->player->statusEffects[20] % 2 == 0) {
			b = true;
		}
	} else {
		b = true;
	}
	app->canvas->pushedWall = false;
	app->player->advanceTurn();
	this->updateBombs();
	if (b) {
		this->monstersTurn = 1;
	} else {
		this->monstersTurn = 2;
	}
	this->monstersUpdated = false;
	this->lastTurnTime = app->time;
	if (b) {
		for (int i = 0; i < this->numEntities; ++i) {
			this->entities[i].updateMonsterFX();
		}
		app->canvas->updateFacingEntity = true;
	}
	for (int j = 0; j < 6; ++j) {
		Entity* openDoors = this->openDoors[j];
		if (openDoors != nullptr) {
			if (this->CanCloseDoor(openDoors)) {
				this->performDoorEvent(1, openDoors, 2);
			}
		}
	}
	this->executeStaticFunc(6);
	app->canvas->startRotation(true);
}

void Game::snapAllMovers() {
	while (this->numLerpSprites > 0) {
		this->snapGameSprites();
		this->snapLerpSprites(-1);
	}
}

void Game::skipCinematic() {


	bool allowSounds = app->sound->allowSounds;
	app->sound->allowSounds = false;
	app->sound->soundStop();
	this->skippingCinematic = true;
	while (this->skippingCinematic) {
		this->snapGameSprites();
		this->snapLerpSprites(-1);
		if (this->activeCameraKey != -1) {
			this->activeCamera->Snap(this->activeCameraKey);
		}
		if (this->skippingCinematic && nullptr == this->activeCamera->keyThread) {
			this->activeCamera->cameraThread->attemptResume(app->gameTime + 0x40000000);
		}
	}
	app->hud->subTitleID = -1;
	app->hud->cinTitleID = -1;
	this->snapGameSprites();
	this->snapLerpSprites(-1);
	this->cinematicWeapon = -1;
	app->canvas->shakeTime = 0;
	if (!app->render->isFading() || (app->render->getFadeFlags() & 0xC) == 0x0) {
		app->render->startFade(500, 2);
	}
	app->canvas->updateFacingEntity = true;
	app->sound->allowSounds = allowSounds;
	app->canvas->shakeTime = 0;
	app->canvas->blockInputTime = 0;
	app->particleSystem->freeAllParticles();
	if (app->canvas->state == Canvas::ST_DIALOG && this->isCameraActive()) {
		this->activeCameraTime = 65536;
	}
	if (app->canvas->state == Canvas::ST_PLAYING) {
		app->canvas->drawPlayingSoftKeys();
	}
}

void Game::runScriptThreads(int n) {


	if (this->numScriptThreads == 0) {
		return;
	}
	for (int i = 0; i < 20; ++i) {
		ScriptThread* scriptThread = &this->scriptThreads[i];
		if (scriptThread->inuse) {
			if (scriptThread->state == 2) {
				if (this->isInputBlockedByScript() && app->canvas->state == Canvas::ST_AUTOMAP) {
					app->canvas->setState(Canvas::ST_PLAYING);
				}
				if (app->canvas->state == Canvas::ST_PLAYING || app->canvas->state == Canvas::ST_CAMERA) {
					scriptThread->attemptResume(n);
				}
			}
			if (scriptThread->state != 2 || scriptThread->stackPtr == 0) {
				this->freeScriptThread(scriptThread);
			}
		}
	}
}

ScriptThread* Game::allocScriptThread() {


	for (int i = 0; i < 20; ++i) {
		ScriptThread* scriptThread = &this->scriptThreads[i];
		if (!scriptThread->inuse) {
			scriptThread->init();
			scriptThread->inuse = true;
			this->numScriptThreads++;
			return scriptThread;
		}
	}
	app->Error(40); // ERR_MAX_SCRIPTTHREADS
	return nullptr;
}

void Game::freeScriptThread(ScriptThread* scriptThread) {


	if (!scriptThread->inuse) {
		app->Error(102); // ERR_SCRIPTTHREAD_FREE
		return;
	}
	scriptThread->reset();
	this->numScriptThreads--;
}

bool Game::isCameraActive() {
	return this->activeCameraView && this->activeCamera != nullptr;
}

int Game::executeTile(int n, int n2, int n3) {
	return this->executeTile(n, n2, n3, false);
}

bool Game::doesScriptExist(int n, int n2, int n3) {


	if (n < 0 || n >= 32 || n2 < 0 || n2 >= 32) {
		return false;
	}
	int n4 = n2 * 32 + n;
	if ((app->render->mapFlags[n4] & 0x40) != 0x0) {
		for (int i = app->render->findEventIndex(n4); i != -1; i = app->render->getNextEventIndex()) {
			int n5 = app->render->tileEvents[i + 1];
			int n6 = n5 & n3;
			if ((n5 & 0x80000) == 0x0 && (n6 & 0xF) != 0x0 && (n6 & 0xFF0) != 0x0 &&
			    (((n5 & 0x7000) == 0x0 && (n3 & 0x7000) == 0x0) || (n6 & 0x7000) != 0x0)) {
				return true;
			}
		}
	}
	return false;
}

int Game::executeTile(int n, int n2, int n3, bool b) {


	ScriptThread* allocScriptThread = this->allocScriptThread();
	int n4;
	if (app->canvas->state == Canvas::ST_DIALOG) {
		n4 = allocScriptThread->queueTile(n, n2, n3, b);
		allocScriptThread->unpauseTime = 0;
	} else {
		n4 = allocScriptThread->executeTile(n, n2, n3, b);
	}
	if (n4 != 2) {
		this->freeScriptThread(allocScriptThread);
	}
	return n4;
}

int Game::executeStaticFunc(int n) {


	if (n >= 12 || app->render->staticFuncs[n] == 65535) {
		return 0;
	}
	ScriptThread* allocScriptThread = this->allocScriptThread();
	allocScriptThread->alloc(app->render->staticFuncs[n]);
	return allocScriptThread->run();
}

int Game::queueStaticFunc(int n) {


	if (n >= 12 || app->render->staticFuncs[n] == 65535) {
		return 0;
	}
	ScriptThread* allocScriptThread = this->allocScriptThread();
	allocScriptThread->alloc(app->render->staticFuncs[n]);
	allocScriptThread->flags |= 0x2;
	allocScriptThread->unpauseTime = 0;
	app->canvas->blockInputTime = app->gameTime + 1;
	return 2;
}

bool Game::isInputBlockedByScript() {
	if (this->numScriptThreads > 0) {
		for (int i = 0; i < 20; ++i) {
			ScriptThread* scriptThread = &this->scriptThreads[i];
			if (scriptThread->inuse) {
				if ((scriptThread->type & 0x10000) != 0x0) {
					return true;
				}
			}
		}
	}
	return false;
}

void Game::updateScriptVars() {


	this->scriptStateVars[0] = (short)app->player->statusEffects[33];
	this->scriptStateVars[1] = (short)app->player->ce->getStat(0);
	this->scriptStateVars[2] = (short)(app->canvas->viewX >> 6);
	this->scriptStateVars[3] = (short)(app->canvas->viewY >> 6);
	this->scriptStateVars[14] = app->player->characterChoice;
	this->scriptStateVars[16] = (short)(app->player->isFamiliar ? 1 : 0);
	this->scriptStateVars[8] = app->player->inventory[24];
}

void Game::cleanUpCamMemory() {
	if (this->mayaCameras) {
		delete[] this->mayaCameras;
	}
	this->mayaCameras = nullptr;

	if (this->mayaCameraKeys) {
		delete[] this->mayaCameraKeys;
	}
	this->mayaCameraKeys = nullptr;

	if (this->mayaCameraTweens) {
		delete[] this->mayaCameraTweens;
	}
	this->mayaCameraTweens = nullptr;

	if (this->mayaTweenIndices) {
		delete[] this->mayaTweenIndices;
	}
	this->mayaTweenIndices = nullptr;
}
