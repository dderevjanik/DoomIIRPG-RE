#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <numbers>
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
#include "ConfigEnums.h"

void Game::gsprite_clear(int n) {
	for (int i = 0; i < 48; ++i) {
		GameSprite* gameSprite = &this->gsprites[i];
		if ((gameSprite->flags & 0x1) != 0x0) {
			if ((gameSprite->flags & n) != 0x0) {
				this->gsprite_destroy(gameSprite);
			}
		}
	}
}

GameSprite* Game::gsprite_alloc(int n, int n2, int n3) {


	int n4;
	for (n4 = 0; n4 < 48 && (this->gsprites[n4].flags & 0x1) != 0x0; ++n4) {
	}
	if (n4 == 48) {
		app->Error(34); // ERR_MAX_CUSTOMSPRITES
		return nullptr;
	}
	GameSprite* gameSprite = &this->gsprites[n4];
	gameSprite->sprite = app->render->customSprites[n4];
	for (int i = 0; i < 6; ++i) {
		gameSprite->pos[i] = 0;
	}
	gameSprite->vel[2] = 0;
	gameSprite->vel[1] = 0;
	gameSprite->vel[0] = 0;
	gameSprite->duration = 0;
	gameSprite->time = app->time;
	gameSprite->data = nullptr;
	app->render->setSpriteEnt(gameSprite->sprite, -1);
	app->render->setSpriteRenderMode(gameSprite->sprite, 0);
	app->render->setSpriteScaleFactor(gameSprite->sprite, 64);
	gameSprite->flags = (n3 | 0x1);
	app->render->setSpriteInfoRaw(gameSprite->sprite, n | n2 << 8);
	if ((gameSprite->flags & 0x2)) {
		this->activePropogators++;
	}
	this->activeSprites++;
	app->canvas->invalidateRect();
	return gameSprite;
}

GameSprite* Game::gsprite_allocAnim(int n, int n2, int n3, int n4) {


	GameSprite* gsprite_alloc = this->gsprite_alloc(n, 0, 66);
	gsprite_alloc->numAnimFrames = 4;
	gsprite_alloc->pos[3] = n2;
	gsprite_alloc->pos[0] = n2;
	app->render->setSpriteX(gsprite_alloc->sprite, n2);
	gsprite_alloc->pos[4] = n3;
	gsprite_alloc->pos[1] = n3;
	app->render->setSpriteY(gsprite_alloc->sprite, n3);
	gsprite_alloc->pos[5] = n4;
	gsprite_alloc->pos[2] = n4;
	app->render->setSpriteZ(gsprite_alloc->sprite, n4);
	gsprite_alloc->destScale = 64;
	gsprite_alloc->startScale = 64;
	gsprite_alloc->duration = 200 * gsprite_alloc->numAnimFrames;
	if (auto saIt = gSpriteAnimDefs.find(n); saIt != gSpriteAnimDefs.end()) {
		const SpriteAnimDef& sa = saIt->second;
		if (sa.renderMode >= 0)
			app->render->setSpriteRenderMode(gsprite_alloc->sprite, sa.renderMode);
		if (sa.scale >= 0)
			app->render->setSpriteScaleFactor(gsprite_alloc->sprite, sa.scale);
		if (sa.numFrames >= 0)
			gsprite_alloc->numAnimFrames = sa.numFrames;
		if (sa.duration >= 0)
			gsprite_alloc->duration = sa.duration;
		else if (sa.numFrames >= 0)
			gsprite_alloc->duration = 200 * sa.numFrames;
		if (sa.zAtGround) {
			app->render->setSpriteZ(gsprite_alloc->sprite,
			    (short)(app->render->getHeight(n2, n3) + sa.zOffset));
		}
		if (sa.randomFlip) {
			if ((app->nextInt() & 0x1) != 0x0)
				app->render->setSpriteInfoFlag(gsprite_alloc->sprite, 0x20000);
			else
				app->render->clearSpriteInfoFlag(gsprite_alloc->sprite, 0x20000);
		}
		if (sa.facePlayer) {
			gsprite_alloc->flags |= 0x1002;
			gsprite_alloc->pos[0] = sa.posOffset;
			gsprite_alloc->pos[3] = sa.posOffset;
			gsprite_alloc->pos[1] = 0;
			gsprite_alloc->pos[4] = 0;
			gsprite_alloc->vel[2] = 0;
			gsprite_alloc->vel[1] = 0;
			gsprite_alloc->vel[0] = 0;
			gsprite_alloc->pos[5] -= (short)app->canvas->viewZ;
			gsprite_alloc->pos[2] = gsprite_alloc->pos[5];
			app->render->setSpriteX(gsprite_alloc->sprite,
			    (short)(app->canvas->viewX + app->canvas->viewStepX + (this->viewStepX >> 6) * gsprite_alloc->pos[0] +
			            (this->viewRightStepX >> 6) * gsprite_alloc->pos[1]));
			app->render->setSpriteY(gsprite_alloc->sprite,
			    (short)(app->canvas->viewY + app->canvas->viewStepY + (this->viewStepY >> 6) * gsprite_alloc->pos[0] +
			            (this->viewRightStepY >> 6) * gsprite_alloc->pos[1]));
			app->render->setSpriteZ(gsprite_alloc->sprite,
			    (short)(app->canvas->viewZ + gsprite_alloc->pos[2]));
		}
	}
	app->render->relinkSprite(gsprite_alloc->sprite);
	return gsprite_alloc;
}

void Game::gsprite_destroy(GameSprite* gameSprite) {


	if ((gameSprite->flags & 0x2000) == 0x0) {
		app->render->setSpriteInfoFlag(gameSprite->sprite, 0x10000);
	} else if ((gameSprite->flags & 0x8000) == 0x0) {
		app->render->clearSpriteInfoFlag(gameSprite->sprite, 0x100);
		app->render->setSpriteX(gameSprite->sprite, gameSprite->pos[3]);
		app->render->setSpriteY(gameSprite->sprite, gameSprite->pos[4]);
		if ((gameSprite->flags & 0x4000) == 0x0) {
			app->render->setSpriteZ(gameSprite->sprite, gameSprite->pos[5]);
		} else {
			app->render->setSpriteZ(gameSprite->sprite, gameSprite->pos[2]);
		}
		app->render->relinkSprite(gameSprite->sprite);
	}
	gameSprite->flags &= 0xFFFFFFFE;
	if ((gameSprite->flags & 0x8000) != 0x0) {
		if ((gameSprite->flags & 0x4000) != 0x0) {
			gameSprite->flags &= 0xFFFF3FFF;
			gameSprite->flags |= 0x1;
			app->render->clearSpriteInfoFlag(gameSprite->sprite, 0x10000);
			gameSprite->pos[0] = app->render->getSpriteX(gameSprite->sprite);
			gameSprite->pos[1] = app->render->getSpriteY(gameSprite->sprite);
			gameSprite->pos[2] = app->render->getSpriteZ(gameSprite->sprite);
			gameSprite->pos[5] =
			    (short)(app->render->getHeight(app->render->getSpriteX(gameSprite->sprite),
			                                   app->render->getSpriteY(gameSprite->sprite)) +
			            32);
			gameSprite->time = app->time;
			gameSprite->duration = 250;
			if (gameSprite->vel[0] > 0) {
				gameSprite->pos[3] -= 31;
				gameSprite->vel[0] = -31000 / gameSprite->duration;
			} else if (gameSprite->vel[0] < 0) {
				gameSprite->pos[3] += 31;
				gameSprite->vel[0] = 31000 / gameSprite->duration;
			}
			if (gameSprite->vel[1] > 0) {
				gameSprite->pos[4] -= 31;
				gameSprite->vel[1] = -31000 / gameSprite->duration;
			} else if (gameSprite->vel[1] < 0) {
				gameSprite->pos[4] += 31;
				gameSprite->vel[1] = 31000 / gameSprite->duration;
			}
			gameSprite->vel[2] = (gameSprite->pos[5] - gameSprite->pos[2]) * 1000 / gameSprite->duration;
			this->activeSprites++;
		}
	} else if ((gameSprite->flags & 0x800) != 0x0) {
		app->render->unlinkSprite(gameSprite->sprite);
	}
}

void Game::snapGameSprites() {
	do {
		this->gsprite_update(0x40000000);
	} while (this->activeSprites != 0);
}

void Game::gsprite_update(int n) {


	if (this->activeSprites == 0) {
		return;
	}

	bool b = false;
	this->activeSprites = 0;
	this->activePropogators = 0;
	for (int i = 0; i < 48; ++i) {
		GameSprite* gameSprite = &this->gsprites[i];
		if ((gameSprite->flags & 0x1) != 0x0) {
			int n2 = n - gameSprite->time;
			if (n2 < 0) {
				if (0x0 != (gameSprite->flags & 0x2)) {
					this->activePropogators++;
				}
				this->activeSprites++;
			} else if ((gameSprite->flags & 0x200) == 0x0 && n2 >= gameSprite->duration) {
				if ((gameSprite->flags & 0x8) == 0x0) {
					app->canvas->invalidateRect();
				}
				this->gsprite_destroy(gameSprite);
			} else {
				this->activeSprites++;
				b = true;
				if (0x0 != (gameSprite->flags & 0x40)) {
					app->render->setSpriteFrame(gameSprite->sprite,
					    n2 / 200 % gameSprite->numAnimFrames);
				}
				if (0x0 != (gameSprite->flags & 0x1000)) {
					int n3 = app->render->getSpriteX(gameSprite->sprite) >> 6;
					int n4 = app->render->getSpriteY(gameSprite->sprite) >> 6;
					short n5 = (short)(app->canvas->viewX + this->viewStepX);
					short n6 = (short)(app->canvas->viewY + this->viewStepY);
					if (0x0 != (gameSprite->flags & 0x2)) {
						if (n2 > gameSprite->duration) {
							app->render->setSpriteX(gameSprite->sprite,
							    (short)(n5 + (this->viewStepX >> 6) * gameSprite->pos[3] +
							            (this->viewRightStepX >> 6) * gameSprite->pos[4]));
							app->render->setSpriteY(gameSprite->sprite,
							    (short)(n6 + (this->viewStepY >> 6) * gameSprite->pos[3] +
							            (this->viewRightStepY >> 6) * gameSprite->pos[4]));
							app->render->setSpriteZ(gameSprite->sprite,
							    (short)(app->canvas->viewZ + gameSprite->pos[5]));
							gameSprite->flags &= 0xFFFFFFFD;
						} else {
							int n7 = gameSprite->pos[0] + gameSprite->vel[0] * n2 / 1000;
							int n8 = gameSprite->pos[1] + gameSprite->vel[1] * n2 / 1000;
							app->render->setSpriteX(gameSprite->sprite,
							    (short)(n5 + (this->viewStepX >> 6) * n7 + (this->viewRightStepX >> 6) * n8));
							app->render->setSpriteY(gameSprite->sprite,
							    (short)(n6 + (this->viewStepY >> 6) * n7 + (this->viewRightStepY >> 6) * n8));
							app->render->setSpriteZ(gameSprite->sprite,
							    (short)(app->canvas->viewZ + gameSprite->pos[2] + gameSprite->vel[2] * n2 / 1000));
						}
						this->activePropogators++;
					} else {
						app->render->setSpriteX(gameSprite->sprite, n5);
						app->render->setSpriteY(gameSprite->sprite, n6);
						app->render->setSpriteZ(gameSprite->sprite, (short)app->canvas->viewZ);
					}
					if (0x0 == (gameSprite->flags & 0x4) &&
					    (n3 != app->render->getSpriteX(gameSprite->sprite) >> 6 ||
					     n4 != app->render->getSpriteY(gameSprite->sprite) >> 6)) {
						if (0x0 != (gameSprite->flags & 0x1000)) {
							app->render->relinkSprite(gameSprite->sprite, app->canvas->destX << 4,
							                          app->canvas->destY << 4, app->canvas->destZ << 4);
						} else {
							app->render->relinkSprite(gameSprite->sprite);
						}
					}
				} else if (0x0 != (gameSprite->flags & 0x2)) {
					this->activePropogators++;
					if (n2 >= gameSprite->duration) {
						app->render->setSpriteX(gameSprite->sprite, gameSprite->pos[3]);
						app->render->setSpriteY(gameSprite->sprite, gameSprite->pos[4]);
						if ((gameSprite->flags & 0x4000) == 0x0) {
							app->render->setSpriteZ(gameSprite->sprite, gameSprite->pos[5]);
						} else if ((gameSprite->flags & 0x8000) == 0x0) {
							app->render->setSpriteZ(gameSprite->sprite, gameSprite->pos[2]);
						}
						gameSprite->flags &= 0xFFFFFFFD;
					} else {
						app->render->setSpriteX(gameSprite->sprite,
						    (short)(gameSprite->pos[0] + gameSprite->vel[0] * n2 / 1000));
						app->render->setSpriteY(gameSprite->sprite,
						    (short)(gameSprite->pos[1] + gameSprite->vel[1] * n2 / 1000));
						if ((gameSprite->flags & 0x4000) == 0x0) {
							app->render->setSpriteZ(gameSprite->sprite,
							    (short)(gameSprite->pos[2] + gameSprite->vel[2] * n2 / 1000));
						} else {
							double progress = (gameSprite->duration != 0)
							                      ? (double)n2 / (double)gameSprite->duration
							                      : 0.0;
							double angle = progress * std::numbers::pi;
							int delta_z = gameSprite->pos[5] - gameSprite->pos[2];
							int z_offset = (int)(std::sin(angle) * (double)delta_z);
							app->render->setSpriteZ(gameSprite->sprite,
							    (short)(gameSprite->pos[2] + z_offset));
						}
						if (0x0 == (gameSprite->flags & 0x4)) {
							app->render->relinkSprite(gameSprite->sprite);
						}
					}
				}
				if (0x0 != (gameSprite->flags & 0x400)) {
					if (n2 > gameSprite->duration) {
						app->render->setSpriteScaleFactor(gameSprite->sprite,
						    gameSprite->destScale);
						gameSprite->flags &= 0xFFFFFBFF;
					} else {
						app->render->setSpriteScaleFactor(gameSprite->sprite,
						    (uint8_t)(gameSprite->startScale + gameSprite->scaleStep * n2 / 1000));
					}
				}
			}
		}
	}
	if (b) {
		app->canvas->staleView = true;
		app->canvas->invalidateRect();
	}
}

int Game::updateLerpSprite(LerpSprite* lerpSprite) {


	int n = 0;
	int n2 = lerpSprite->hSprite - 1;
	int n3 = app->canvas->viewX >> 6;
	int n4 = app->canvas->viewY >> 6;
	int n5 = app->gameTime - lerpSprite->startTime;
	int n6 = app->render->getSpriteInfoRaw(n2);
	int n7 = n6 & 0xFF;
	if ((n6 & Enums::SPRITE_FLAG_TILE) != 0x0) {
		n7 += 257;
	}
	if (n5 >= lerpSprite->travelTime) {
		this->freeLerpSprite(lerpSprite);
		return 3;
	}
	if (n5 < 0) {
		return 4;
	}
	int n8 = 0;
	if (lerpSprite->travelTime != 0) {
		n8 = (n5 << 16) / (lerpSprite->travelTime << 8);
	}
	app->render->setSpriteX(n2,
	    (short)(lerpSprite->srcX + (n8 * (lerpSprite->dstX - lerpSprite->srcX << 8) >> 16)));
	app->render->setSpriteY(n2,
	    (short)(lerpSprite->srcY + (n8 * (lerpSprite->dstY - lerpSprite->srcY << 8) >> 16)));
	app->render->setSpriteScaleFactor(n2,
	    (uint8_t)(lerpSprite->srcScale + (n8 * (lerpSprite->dstScale - lerpSprite->srcScale << 8) >> 16)));
	app->render->setSpriteZ(n2,
	    (short)(lerpSprite->srcZ + (n8 * (lerpSprite->dstZ - lerpSprite->srcZ << 8) >> 16)));
	if ((lerpSprite->flags & Enums::LS_FLAG_PARABOLA) != 0x0) {
		double progress = (lerpSprite->travelTime != 0)
		                      ? (double)n5 / (double)lerpSprite->travelTime
		                      : 0.0;
		double angle = progress * std::numbers::pi;
		if ((lerpSprite->flags & Enums::LS_FLAG_TRUNC) != 0x0) {
			angle *= 0.75;
		}
		int z_offset = (int)(std::sin(angle) * (double)lerpSprite->height);
		if (!(lerpSprite->flags & Enums::LS_FLAG_S_NORELINK)) {
			app->render->relinkSprite(n2, app->render->getSpriteX(n2) << 4,
			                          app->render->getSpriteY(n2) << 4,
			                          app->render->getSpriteZ(n2) << 4);
		}
		app->render->setSpriteZ(n2, app->render->getSpriteZ(n2) + (short)z_offset);
	} else if (!(lerpSprite->flags & Enums::LS_FLAG_S_NORELINK)) {
		app->render->relinkSprite(n2);
	}
	int n12 = app->render->getSpriteX(n2) >> 6;
	int n13 = app->render->getSpriteY(n2) >> 6;
	if (app->render->getSpriteEnt(n2) != -1 && !(app->render->getSpriteInfoRaw(n2) & Enums::SPRITE_FLAG_HIDDEN)) {
		Entity* entity = &this->entities[app->render->getSpriteEnt(n2)];
		int n14 = entity->linkIndex % 32;
		int n15 = entity->linkIndex / 32;
		if (entity->def->eType == Enums::ET_NPC || entity->isMonster()) {
			int anim = ((app->render->getSpriteInfoRaw(n2) & 0xFF00) >> 8) & Enums::MANIM_MASK;
			int n17 = lerpSprite->dstX - lerpSprite->srcX;
			int n18 = lerpSprite->dstY - lerpSprite->srcY;

			if ((anim == Enums::MANIM_IDLE || anim == Enums::MANIM_WALK_FRONT ||
			     (anim == Enums::MANIM_WALK_BACK && (lerpSprite->flags & Enums::LS_FLAG_AUTO_FACE) != 0x0)) &&
			    (n17 | n18) != 0x0) {
				int abs = std::abs((app->render->viewAngle & 0x3FF) - this->VecToDir(n17, n18, true));
				if (app->combat->WorldDistToTileDist(
				        std::max((lerpSprite->dstX - lerpSprite->srcX) * (lerpSprite->dstX - lerpSprite->srcX),
				                 (lerpSprite->dstY - lerpSprite->srcY) * (lerpSprite->dstY - lerpSprite->srcY))) > 1 &&
				    abs < 256) {
					anim = Enums::MANIM_WALK_BACK;
					lerpSprite->flags |= Enums::LS_FLAG_AUTO_FACE;
				} else {
					anim = Enums::MANIM_WALK_FRONT;
				}
			} else if (anim == Enums::MANIM_IDLE_BACK) {
				anim = Enums::MANIM_WALK_BACK;
			}

			if (anim == Enums::MANIM_WALK_FRONT || anim == Enums::MANIM_WALK_BACK) {

				if (entity->def->eType == Enums::ET_MONSTER) {
					int walkSnd = app->combat->monsterBehaviors[entity->def->monsterIdx].walkSoundResId;
					if (walkSnd >= 0) {
						if ((1 + (n8 * lerpSprite->dist >> 12) & 0x3) >> 1 !=
						    ((((app->render->getSpriteInfoRaw(n2)) & 0xFF00) >> 8) & 3) >> 1) {
							app->sound->playSound(walkSnd, 0, true, 0);
						}
					}
				}

				app->render->setSpriteFrame(n2,
				                                  ((1 + (n8 * lerpSprite->dist >> 12) & 0x3) | anim));
			}
		}
		if (!(lerpSprite->flags & Enums::LS_FLAG_ENT_NORELINK) && (n12 != n14 || n13 != n15)) {
			this->unlinkEntity(entity);
			this->linkEntity(entity, app->render->getSpriteX(n2) >> 6,
			                 app->render->getSpriteY(n2) >> 6);
			n |= 0x2;
		}
	}
	if (!app->canvas->staleView && ((n3 == n12 && n4 == n13) || app->render->checkTileVisibilty(n12, n13))) {
		n |= 0x4;
	}
	return n;
}

void Game::snapLerpSprites(int n) {
	if (this->numLerpSprites == 0) {
		return;
	}
	this->numCallThreads = 0;
	for (int i = 0; i < 16; ++i) {
		LerpSprite* lerpSprite = &this->lerpSprites[i];
		if (lerpSprite->hSprite != 0) {
			if (n == -1 || lerpSprite->hSprite == n + 1) {
				if (lerpSprite->thread != nullptr && !(lerpSprite->flags & Enums::LS_FLAG_ASYNC)) {
					int n2;
					for (n2 = 0; n2 < this->numCallThreads && this->callThreads[n2] != lerpSprite->thread; ++n2) {
					}
					if (n2 == this->numCallThreads) {
						this->callThreads[this->numCallThreads++] = lerpSprite->thread;
					}
				}
				lerpSprite->startTime = 0;
				lerpSprite->travelTime = 0;
				this->updateLerpSprite(lerpSprite);
			}
		}
	}
	for (int j = 0; j < this->numCallThreads; ++j) {
		this->callThreads[j]->run();
	}
}

void Game::updateLerpSprites() {


	bool b = false;
	bool b2 = false;
	this->numCallThreads = 0;
	if (this->numLerpSprites > 0) {
		for (int i = 0; i < 16; ++i) {
			LerpSprite* lerpSprite = &this->lerpSprites[i];
			if (lerpSprite->hSprite != 0) {
				ScriptThread* thread = lerpSprite->thread;
				int flags = lerpSprite->flags;
				int updateLerpSprite = this->updateLerpSprite(lerpSprite);
				if (0x0 != (updateLerpSprite & 0x1) && !(flags & Enums::LS_FLAG_ASYNC) && nullptr != thread) {
					int n;
					for (n = 0; n < this->numCallThreads && this->callThreads[n] != thread; ++n) {
					}
					if (n == this->numCallThreads) {
						this->callThreads[this->numCallThreads++] = thread;
					}
				}
				b2 = (b2 || (updateLerpSprite & 0x2) != 0x0);
				b = (b || (updateLerpSprite & 0x4) != 0x0);
			}
		}
		for (int j = 0; j < this->numCallThreads; ++j) {
			if (this->callThreads[j]->inuse) {
				this->callThreads[j]->run();
			}
		}
		if (b2) {
			app->canvas->updateFacingEntity = true;
		}
		if (b) {
			app->canvas->invalidateRect();
		}
	}
}

LerpSprite* Game::allocLerpSprite(ScriptThread* thread, int n, bool b) {


	LerpSprite* lerpSprite = nullptr;
	LerpSprite* lerpSprite2 = nullptr;
	for (int i = 0; i < 16; ++i) {
		LerpSprite* lerpSprite3 = &this->lerpSprites[i];
		if (lerpSprite3->hSprite == n + 1) {
			lerpSprite = lerpSprite3;
			break;
		}
		if (lerpSprite2 == nullptr && lerpSprite3->hSprite == 0) {
			lerpSprite2 = lerpSprite3;
		}
	}
	if (lerpSprite2 != nullptr && lerpSprite == nullptr) {
		lerpSprite = lerpSprite2;
		lerpSprite->flags = 0;
		lerpSprite->hSprite = n + 1;
		lerpSprite->thread = thread;
		this->numLerpSprites++;
	} else {
		if ((lerpSprite->flags & Enums::LS_FLAG_ANIMATING_EFFECT) != 0x0) {
			this->animatingEffects--;
		}
		int n2 = lerpSprite->hSprite - 1;
		Entity* entity = nullptr;
		if (-1 != app->render->getSpriteEnt(n2)) {
			entity = &this->entities[app->render->getSpriteEnt(n2)];
		}
		if (entity != nullptr && entity->isMonster()) {
			int n3 = (app->render->getSpriteInfoRaw(n2) & 0xFF00) >> 8 & 0xF0;
			if (n3 == 32 || (lerpSprite->flags & Enums::LS_FLAG_AUTO_FACE) != 0x0) {
				n3 = 0;
			} else if (n3 == 48) {
				n3 = 16;
			}
			app->render->setSpriteInfoRaw(n2, ((app->render->getSpriteInfoRaw(n2) & 0xFFFF00FF) | n3 << 8));
		}
	}
	if (lerpSprite == nullptr) {
		app->Error(36); // ERR_MAX_LERPSPRITES
	}
	if (thread == nullptr) {
		lerpSprite->flags |= Enums::LS_FLAG_ASYNC;
	}
	if (b) {
		lerpSprite->flags |= Enums::LS_FLAG_ANIMATING_EFFECT;
		this->animatingEffects++;
	}
	return lerpSprite;
}

void Game::freeLerpSprite(LerpSprite* lerpSprite) {

	Entity* entity = nullptr;
	int sprite = lerpSprite->hSprite - 1;
	int n2 = app->render->getSpriteInfoRaw(sprite) & 0xFF;
	if ((app->render->getSpriteInfoRaw(sprite) & Enums::SPRITE_FLAG_TILE) != 0x0) {
		n2 += 257;
	}
	app->render->setSpriteX(sprite, (short)lerpSprite->dstX);
	app->render->setSpriteY(sprite, (short)lerpSprite->dstY);
	if (!(lerpSprite->flags & Enums::LS_FLAG_TRUNC)) {
		app->render->setSpriteZ(sprite, (short)lerpSprite->dstZ);
	}
	app->render->setSpriteScaleFactor(sprite, (short)lerpSprite->dstScale);
	if (!(lerpSprite->flags & Enums::LS_FLAG_S_NORELINK)) {
		app->render->relinkSprite(sprite);
	}
	if (app->render->getSpriteEnt(sprite) != -1) {
		entity = &this->entities[app->render->getSpriteEnt(sprite)];
	}
	int n3 = lerpSprite->dstX >> 6;
	int n4 = lerpSprite->dstY >> 6;
	if (nullptr != entity && 0x0 == (app->render->getSpriteInfoRaw(sprite) & Enums::SPRITE_FLAG_HIDDEN)) {
		if (entity->def->eType == Enums::ET_NPC || entity->def->eType == Enums::ET_MONSTER) {
			if (entity->ai != nullptr) {
				entity->ai->frameTime = 0;
			}
			int n5 = (app->render->getSpriteInfoRaw(sprite) & 0xFF00) >> 8 & 0xF0;
			if ((n5 == 16 || n5 == 48) && !(lerpSprite->flags & Enums::LS_FLAG_AUTO_FACE)) {
				app->render->setSpriteInfoRaw(sprite, ((app->render->getSpriteInfoRaw(sprite) & 0xFFFF00FF) | 0x1000));
			} else {
				app->render->setSpriteInfoRaw(sprite, ((app->render->getSpriteInfoRaw(sprite) & 0xFFFF00FF) | 0x0));
			}
		}
		int n6 = entity->linkIndex % 32;
		int n7 = entity->linkIndex / 32;
		if (!(lerpSprite->flags & Enums::LS_FLAG_ENT_NORELINK) &&
		    ((entity->info & 0x100000) == 0x0 || n3 != n6 || n4 != n7)) {
			this->unlinkEntity(entity);
			this->linkEntity(entity, app->render->getSpriteX(sprite) >> 6,
			                 app->render->getSpriteY(sprite) >> 6);
		}
	}
	if ((lerpSprite->flags & Enums::LS_FLAG_DOORCLOSE) != 0x0) {
		app->render->setSpriteScaleFactor(sprite, 64);
		app->render->setSpriteInfoRaw(sprite, app->render->getSpriteInfoRaw(sprite) & 0xFFFF00FF);
		app->canvas->updateFacingEntity = true;
		app->canvas->automapDrawn = false;
		app->render->setSpriteInfoRaw(sprite, app->render->getSpriteInfoRaw(sprite) & 0x7fffffff);
	} else if ((lerpSprite->flags & Enums::LS_FLAG_DOOROPEN) || (lerpSprite->flags & Enums::LS_FLAG_SECRET_HIDE)) {
		app->canvas->updateFacingEntity = true;
		this->secretActive = false;
		app->canvas->automapDrawn = false;
		this->unlinkEntity(entity);
		if (entity->def->eType != Enums::ET_DOOR) {
			app->render->setSpriteInfoFlag(sprite, Enums::SPRITE_FLAG_HIDDEN);
		}
	} else {
		if ((lerpSprite->flags & Enums::LS_MASK_CHICKEN_BOUNCE) == Enums::LS_MASK_CHICKEN_BOUNCE) {
			int sx = lerpSprite->srcX - lerpSprite->dstX;
			int sy = lerpSprite->srcY - lerpSprite->dstY;
			lerpSprite->dstX = lerpSprite->srcX = app->render->getSpriteX(sprite);
			lerpSprite->dstY = lerpSprite->srcY = app->render->getSpriteY(sprite);
			lerpSprite->srcZ = app->render->getSpriteZ(sprite);
			lerpSprite->flags &= ~Enums::LS_FLAG_TRUNC;
			if (sx != 0) {
				sx /= std::abs(sx);
			}
			if (sy != 0) {
				sy /= std::abs(sy);
			}
			int dx;
			int dy;
			if ((lerpSprite->dstX & 0x3F) == 32 && (lerpSprite->dstY & 0x3F) == 32) {
				dx = sx * 64;
				dy = sy * 64;
			} else {
				dx = sx * 31;
				dy = sy * 31;
			}
			lerpSprite->dstX = (lerpSprite->dstX + dx & 0xFFFFFFC0) + 32;
			lerpSprite->dstY = (lerpSprite->dstY + dy & 0xFFFFFFC0) + 32;
			lerpSprite->dstZ = app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + 32;
			lerpSprite->height >>= 1;
			lerpSprite->startTime = app->gameTime - 100;
			lerpSprite->travelTime = 300;
			this->updateLerpSprite(lerpSprite);
			return;
		}

		if (lerpSprite->flags & Enums::LS_FLAG_SECRET_OPEN) {
			lerpSprite->flags &= ~Enums::LS_FLAG_SECRET_OPEN;
			lerpSprite->flags |= Enums::LS_FLAG_SECRET_HIDE;
			lerpSprite->srcX = lerpSprite->dstX;
			lerpSprite->srcY = lerpSprite->dstY;
			app->render->setSpriteInfoFlag(sprite, Enums::SPRITE_FLAG_DOORLERP);
			app->render->setSpriteScaleFactor(sprite, (uint8_t)lerpSprite->dstScale);
			lerpSprite->dstScale = 0;
			lerpSprite->startTime = app->gameTime;
			switch (app->render->getSpriteInfoRaw(sprite) & Enums::SPRITE_FLAGS_ORIENTED) {
				case Enums::SPRITE_FLAG_EAST: {
					lerpSprite->dstY += 32;
					break;
				}
				case Enums::SPRITE_FLAG_NORTH: {
					lerpSprite->dstX += 32;
					break;
				}
				case Enums::SPRITE_FLAG_WEST: {
					lerpSprite->dstY -= 32;
					break;
				}
				case Enums::SPRITE_FLAG_SOUTH: {
					lerpSprite->dstX -= 32;
					break;
				}
			}
			lerpSprite->travelTime = 750;
			app->canvas->automapDrawn = false;
			return;
		}
	}
	if ((lerpSprite->flags & Enums::LS_FLAG_CHICKEN_KICK) != 0x0) {
		app->render->setSpriteInfoRaw(sprite, app->render->getSpriteInfoRaw(sprite) & 0xFFFF00FF);
	}
	if ((lerpSprite->flags & Enums::LS_FLAG_ANIMATING_EFFECT) != 0x0) {
		lerpSprite->flags &= ~Enums::LS_FLAG_ANIMATING_EFFECT;
		this->animatingEffects--;
	}
	lerpSprite->hSprite = 0;
	this->numLerpSprites--;
	if (entity != nullptr) {
		uint8_t eSubType = entity->def->eSubType;
		if (entity->ai != nullptr) {
			if ((entity->ai->goalFlags & 0x1) != 0x0) {
				entity->aiFinishLerp();
			}
			if (app->player->isFamiliar &&
			    entity->distFrom(app->canvas->saveX, app->canvas->saveY) <= app->combat->tileDistances[0]) {
				int* calcPosition = entity->calcPosition();
				if (calcPosition[0] - app->canvas->saveX == 0 ^ calcPosition[1] - app->canvas->saveY == 0) {
					if (app->canvas->state == Canvas::ST_CAMERA) {
						app->player->unsetFamiliarOnceOutOfCinematic = true;
					} else {
						app->player->forceFamiliarReturnDueToMonster();
					}
				}
			}
		}
		if (entity->def->eType == Enums::ET_MONSTER) {
			Entity* mapEntity = this->findMapEntity(lerpSprite->dstX, lerpSprite->dstY, 256);
			if (mapEntity != nullptr) {
				if (mapEntity->def->eSubType == Enums::INTERACT_BARRICADE && entity->def->eType == Enums::ET_MONSTER) {
					if (eSubType != Enums::MONSTER_LOST_SOUL) {
						entity->monsterEffects |= 0x60008;
						entity->monsterEffects &= 0xFFFFE1FD;
					}
				} else {
					entity->pain(5, mapEntity);
				}
			}
		}
	}
}
