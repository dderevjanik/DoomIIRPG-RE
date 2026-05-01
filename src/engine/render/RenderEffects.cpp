#include <stdexcept>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "DataNode.h"
#include "BinaryStream.h"
#include "Resource.h"
#include "MayaCamera.h"
#include "Canvas.h"
#include "Image.h"
#include "Render.h"
#include "Game.h"
#include "Text.h"
#include "GLES.h"
#include "TGLVert.h"
#include "TinyGL.h"
#include "Player.h"
#include "Game.h"
#include "MenuSystem.h"
#include "Menus.h"
#include "Combat.h"
#include "Enums.h"
#include "Hud.h"
#include "Utils.h"
#include "Sound.h"
#include "EntityDef.h"
#include "SpriteDefs.h"

void Render::loadPlayerFog() {

	this->fogLerpStart = 0;
	this->fogLerpTime = 0;
	app->tinyGL->fogMin = this->playerFogMin;
	app->tinyGL->fogRange = this->playerFogRange;
	this->buildFogTables(this->playerFogColor);
}

void Render::savePlayerFog() {

	this->fogLerpStart = 0;
	this->playerFogMin = app->tinyGL->fogMin;
	this->playerFogRange = app->tinyGL->fogRange;
	this->playerFogColor = app->tinyGL->fogColor;
}

void Render::snapFogLerp() {

	if (this->fogLerpTime != 0) {
		this->fogLerpStart = 0;
		this->fogLerpTime = 0;
		app->tinyGL->fogMin = this->destFogMin;
		app->tinyGL->fogRange = this->destFogRange;
	}
}

void Render::startFogLerp(int n, int n2, int fogLerpTime) {


	this->baseFogMin = app->tinyGL->fogMin;
	this->baseFogRange = app->tinyGL->fogRange;
	this->destFogMin = n << 4;
	this->destFogRange = n2 - n << 4;
	if (this->destFogRange == 0) {
		this->destFogRange = 1;
	}
	if (fogLerpTime != 0) {
		this->fogLerpStart = app->time;
		this->fogLerpTime = fogLerpTime;
	} else {
		this->fogLerpStart = 0;
		this->fogLerpTime = 0;
		app->tinyGL->fogMin = this->destFogMin;
		app->tinyGL->fogRange = this->destFogRange;
	}
}

void Render::buildFogTables(int fogColor) {
	app->tinyGL->fogColor = fogColor;
	if ((fogColor & 0xFF000000) == 0x0) {
		app->tinyGL->fogMin = 32752;
		app->tinyGL->fogRange = 1;
	}
}

int (*Render::getImageFrameBounds(int n, int n2, int n3, int n4))[4] {
	short n5 = this->mediaMappings[n];
	this->temp[0] = n2 + n5 << 2;
	this->temp[1] = n3 + n5 << 2;
	this->temp[2] = n4 + n5 << 2;
	for (int i = 0; i < 3; ++i) {
		this->imageFrameBounds[i][0] = (this->mediaBounds[this->temp[i] + 0] & 0xFF) * 64 / 176 - 32;
		this->imageFrameBounds[i][1] = (this->mediaBounds[this->temp[i] + 1] & 0xFF) * 64 / 176 - 32;
		this->imageFrameBounds[i][2] = (176 - (this->mediaBounds[this->temp[i] + 3] & 0xFF)) * 64 / 176 - 32;
		this->imageFrameBounds[i][3] = (176 - (this->mediaBounds[this->temp[i] + 2] & 0xFF)) * 64 / 176 - 32;
	}
	return this->imageFrameBounds;
}

uint16_t* Render::getPalette(int n, int n2) {
	return this->mediaPalettes[this->mediaPalColors[this->mediaMappings[n] + n2] & 0x3FFF];
}

void Render::setupTexture(int n, int n2, int renderMode, int renderFlags) {


	int n4 = this->mediaMappings[n] + n2;

	if ((app->canvas->state == Canvas::ST_AUTOMAP) || (this->renderMode & 0x10) == 0x0) {
		renderMode = 10;
	} else if ((this->renderMode & 0x20)) {
		renderMode = 9;
	}
	if (this->_gles->isInit) { // New
		this->_gles->SetupTexture(n, n2, renderMode, renderFlags);
	}

	int n5;
	int n6;
	if (n == TILE_SKY_BOX) {
		app->tinyGL->textureBaseSize = 256 * 256;
		this->isSkyMap = true;
		n5 = 8;
		n6 = 8;
	} else {
		app->tinyGL->textureBaseSize = this->mediaTexelSizes2[this->mediaTexelSizes[n4] & 0x3FFF];

		uint8_t b = this->mediaDimensions[n4];
		n6 = (b >> 4 & 0xF);
		n5 = (b & 0xF);
		app->tinyGL->imageBounds[0] = (uint16_t)(this->mediaBounds[(n4 << 2) + 0] & 0xFFF);
		app->tinyGL->imageBounds[1] = (uint16_t)(this->mediaBounds[(n4 << 2) + 1] & 0xFFF);
		app->tinyGL->imageBounds[2] = (uint16_t)(this->mediaBounds[(n4 << 2) + 2] & 0xFFF);
		app->tinyGL->imageBounds[3] = (uint16_t)(this->mediaBounds[(n4 << 2) + 3] & 0xFFF);
		this->isSkyMap = false;
	}
	app->tinyGL->sWidth = 1 << n6;
	app->tinyGL->tHeight = 1 << n5;
	// Mirror sprite-tile dimensions onto gles for DrawWorldSpaceSpriteLine
	// UV scaling (single-writer decoupling toward dropping gles → tinyGL).
	this->_gles->spriteSWidth = app->tinyGL->sWidth;
	this->_gles->spriteTHeight = app->tinyGL->tHeight;
}

void Render::unlinkSprite(int n) {
	this->unlinkSprite(n, this->mapSprites[this->S_X + n], this->mapSprites[this->S_Y + n]);
}

void Render::unlinkSprite(int n, int n2, int n3) {
	if (this->mapSprites[this->S_NODE + n] != -1) {
		short n4 = this->mapSprites[this->S_NODE + n];
		if (this->nodeSprites[n4] == n) {
			this->nodeSprites[n4] = this->mapSprites[this->S_NODENEXT + n];
		} else {
			short n5;
			for (n5 = this->nodeSprites[n4]; n5 != -1 && this->mapSprites[this->S_NODENEXT + n5] != n;
			     n5 = this->mapSprites[this->S_NODENEXT + n5]) {
			}
			if (n5 != -1) {
				this->mapSprites[this->S_NODENEXT + n5] = this->mapSprites[this->S_NODENEXT + n];
			}
		}
	}
}

void Render::relinkSprite(int n) {
	relinkSprite(n, this->mapSprites[this->S_X + n] << 4, this->mapSprites[this->S_Y + n] << 4,
	             this->mapSprites[this->S_Z + n] << 4);
}

void Render::relinkSprite(int n, int n2, int n3, int n4) {
	if (this->mapSprites[this->S_NODE + n] != -1) {
		short n5 = this->mapSprites[this->S_NODE + n];
		if (this->nodeSprites[n5] == n) {
			this->nodeSprites[n5] = this->mapSprites[this->S_NODENEXT + n];
		} else {
			short n6;
			for (n6 = this->nodeSprites[n5]; n6 != -1 && this->mapSprites[this->S_NODENEXT + n6] != n;
			     n6 = this->mapSprites[this->S_NODENEXT + n6]) {
			}
			if (n6 != -1) {
				this->mapSprites[this->S_NODENEXT + n6] = this->mapSprites[this->S_NODENEXT + n];
			}
		}
	}
	short nodeForPoint = this->getNodeForPoint(n2, n3, n4, this->mapSpriteInfo[n]);
	if ((this->mapSprites[this->S_NODE + n] = nodeForPoint) != -1) {
		this->mapSprites[this->S_NODENEXT + n] = this->nodeSprites[nodeForPoint];
		this->nodeSprites[nodeForPoint] = (short)n;
	}
}

short Render::getNodeForPoint(int n, int n2, int n3, int n4) {
	short n5 = 0;
	int i = this->nodeOffsets[n5] & 0xFFFF;
	bool b = (n4 & Enums::SPRITE_FLAGS_ORIENTED) != 0x0;
	int n6 = n4 & 0xFF;
	if ((n4 & Enums::SPRITE_FLAG_TILE) != 0x0) {
		n6 += 256 + 1;
	}

	bool b2 = n6 == 240;
	while (i != 65535) {
		int nodeClassifyPoint = this->nodeClassifyPoint(n5, n, n2, n3);
		if (nodeClassifyPoint == 0 && b) {
			if ((n4 & (Enums::SPRITE_FLAG_NORTH | Enums::SPRITE_FLAG_WEST)) != 0x0) {
				n5 = this->nodeChildOffset1[n5];
			} else {
				n5 = this->nodeChildOffset2[n5];
			}
		} else {
			if (!b2 && nodeClassifyPoint > -128 && nodeClassifyPoint < 128) {
				return n5;
			}
			if (nodeClassifyPoint > 0) {
				n5 = this->nodeChildOffset1[n5];
			} else {
				n5 = this->nodeChildOffset2[n5];
			}
		}
		i = (this->nodeOffsets[n5] & 0xFFFF);
	}
	int n7 = (this->nodeBounds[(n5 << 2) + 0] & 0xFF) << 7;
	int n8 = (this->nodeBounds[(n5 << 2) + 1] & 0xFF) << 7;
	int n9 = (this->nodeBounds[(n5 << 2) + 2] & 0xFF) << 7;
	int n10 = (this->nodeBounds[(n5 << 2) + 3] & 0xFF) << 7;
	if (n < n7 || n2 < n8 || n > n9 || n2 > n10) {
		return -1;
	}
	return n5;
}

int Render::getHeight(int x, int y) {
	if (this->heightMap == nullptr) {
		return 0;
	}
	x &= 0x7FF;
	y &= 0x7FF;
	return this->heightMap[(y >> 6) * 32 + (x >> 6)] << 3;
}

bool Render::checkTileVisibilty(int n, int n2) {
	int n3 = n << 6;
	int n4 = n2 << 6;
	return !this->cullBoundingBox(n3 << 4, n4 << 4, n3 + 64 << 4, n4 + 64 << 4, true);
}

void Render::postProcessSprites() {
	int* mapSpriteInfo = this->mapSpriteInfo;
	short* mapSprites = this->mapSprites;
	int i;
	for (i = 0; i < this->numMapSprites; ++i) {
		mapSprites[this->S_Z + i] += (short)getHeight(mapSprites[this->S_X + i], mapSprites[this->S_Y + i]);
		if (i >= this->numNormalSprites) {
			// Z-sprites are anchored so yaml_z is absolute-from-floor; the
			// default sprite offset is subtracted here (see Render::SPRITE_Z_DEFAULT).
			mapSprites[this->S_Z + i] -= Render::SPRITE_Z_DEFAULT;
		}
		int n3 = (uint8_t)(mapSpriteInfo[i] & 0xFF);
		if ((mapSpriteInfo[i] & Enums::SPRITE_FLAG_TILE) != 0x0) {
			n3 += 257;
		}
		if (n3 == 479) {
			mapSprites[this->S_RENDERMODE + i] = 0;
		} else if (n3 == 208 || n3 == 234 || n3 == 130 || n3 == 242) {
			mapSprites[this->S_RENDERMODE + i] = 3;
		} else if (n3 == 178) {
			mapSprites[this->S_RENDERMODE + i] = 3;
		} else if (n3 == 236) {
			mapSprites[this->S_RENDERMODE + i] = 3;
		} else if (n3 == 212) {
			mapSprites[this->S_RENDERMODE + i] = 7;
		} else if (n3 == 161) {
			mapSprites[this->S_RENDERMODE + i] = 2;
		} else if (n3 == 244) {
			mapSprites[this->S_RENDERMODE + i] = 4;
		} else {
			mapSprites[this->S_RENDERMODE + i] = 0;
		}
		this->relinkSprite(i);
	}
	for (int j = 0; j < 48; ++j) {
		this->customSprites[j] = i;
		mapSprites[this->S_NODE + i] = -1;
		mapSpriteInfo[i] = 0;
		mapSprites[this->S_NODENEXT + i] = -1;
		mapSprites[this->S_VIEWNEXT + i] = -1;
		mapSprites[this->S_ENT + i] = -1;
		mapSprites[this->S_RENDERMODE + i] = 0;
		mapSprites[this->S_SCALEFACTOR + i] = 64;
		mapSprites[this->S_X + i] = (mapSprites[this->S_Y + i] = 0);
		mapSprites[this->S_Z + i] = Render::SPRITE_Z_DEFAULT;
		++i;
	}
	this->firstDropSprite = i;
	for (int k = 0; k < 16; ++k) {
		this->dropSprites[k] = i;
		mapSprites[this->S_NODE + i] = -1;
		mapSpriteInfo[i] = 0;
		mapSprites[this->S_NODENEXT + i] = -1;
		mapSprites[this->S_VIEWNEXT + i] = -1;
		mapSprites[this->S_ENT + i] = -1;
		mapSprites[this->S_RENDERMODE + i] = 0;
		mapSprites[this->S_SCALEFACTOR + i] = 64;
		mapSprites[this->S_X + i] = (mapSprites[this->S_Y + i] = 0);
		mapSprites[this->S_Z + i] = Render::SPRITE_Z_DEFAULT;
		++i;
	}
}

bool Render::isFading() {
	return this->fadeFlags != 0;
}

int Render::getFadeFlags() {
	return this->fadeFlags;
}

void Render::startFade(int fadeDuration, int fadeFlags) {

	this->fadeTime = app->upTimeMs;
	this->fadeDuration = fadeDuration;
	this->fadeFlags = fadeFlags;
	// printf("startFade\n");
}

void Render::endFade() {
	this->fadeFlags = 0;
	this->fadeTime = -1;
}

bool Render::fadeScene(Graphics* graphics) {

	int fadeRect[4] = {0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT};

	int fadeDuration = app->upTimeMs - this->fadeTime;
	if (fadeDuration >= this->fadeDuration - 50) {
		fadeDuration = this->fadeDuration;
		if ((this->fadeFlags & Render::FADE_FLAG_CHANGEMAP) != 0x0) {
			this->fadeFlags &= Render::FADE_SPECIAL_FLAG_MASK;
			graphics->fade(fadeRect, 0, 0x00000000);
			app->canvas->loadMap(app->menuSystem->LEVEL_STATS_nextMap, false, false);
			return false;
		}
		if ((this->fadeFlags & Render::FADE_FLAG_SHOWSTATS) != 0x0) {
			this->fadeFlags &= Render::FADE_SPECIAL_FLAG_MASK;
			graphics->fade(fadeRect, 0, 0x00000000);
			app->canvas->saveState(51, (short)3, (short)196);
			return false;
		}
		if ((this->fadeFlags & Render::FADE_FLAG_EPILOGUE) != 0x0) {
			this->endFade();
			app->canvas->setState(Canvas::ST_EPILOGUE);
			return false;
		}
		if ((this->fadeFlags & Render::FADE_FLAG_FADEIN) != 0x0) {
			this->endFade();
			return true;
		}
		if ((this->fadeFlags & Render::FADE_FLAG_FADEOUT) != 0x0) {
			// this->fadeAll(255); // J2ME
			graphics->fade(fadeRect, 0, 0x00000000);
			return true;
		}
	}
	int alpha = 65280 * ((fadeDuration << 16) / (this->fadeDuration << 8)) >> 16;
	if ((this->fadeFlags & Render::FADE_FLAG_FADEOUT) != 0x0) {
		alpha = 256 - alpha;
	}
	graphics->fade(fadeRect, alpha, 0x00000000);

	// this->fadeAll(alpha);  // J2ME
	return true;
}

void Render::postProcessView(Graphics* graphics) {
	static float update = 0.f;
	static int y = 0;


	// final int[] pixels = TinyGL.pixels;
	int screenVScrollOffset = 0;
	if (this->vScrollVelocity != 0) {
		int screenVScrollOffset2 = this->screenVScrollOffset;
		int n = -(5 * (app->time - this->lastScrollChangeTime)) / 30;
		if (this->vScrollVelocity + n < 0) {
			if (app->tinyGL->viewportHeight - screenVScrollOffset2 <= 15 || screenVScrollOffset2 <= 15) {
				screenVScrollOffset2 = 0;
				this->vScrollVelocity = 0;
			} else {
				this->vScrollVelocity = std::max(this->vScrollVelocity + n, -gameConfig->renderMaxVScrollVelocity);
			}
		} else {
			this->vScrollVelocity += n;
		}
		screenVScrollOffset =
		    (screenVScrollOffset2 + this->vScrollVelocity + app->tinyGL->viewportHeight) % app->tinyGL->viewportHeight;
	}
	this->lastScrollChangeTime = app->time;
	this->screenVScrollOffset = screenVScrollOffset;
	int max = 32;
	int max2 = 0;
	if (this->brightenPostProcess) {
		if (this->brightenPPMaxReachedTime != 0 && app->time > this->brightenPPMaxReachedTime + gameConfig->renderHoldBrightnessTime + gameConfig->renderFadeBrightnessTime) {
			app->hud->stopBrightenScreen();
		} else {
			int n2;
			if (this->brightenPPMaxReachedTime == 0) {
				n2 = (app->time - this->brightenPostProcessBeginTime >> 4 & 0x7F) + this->brightnessInitialBoost;
				if (n2 >= this->maxLocalBrightness) {
					this->brightenPPMaxReachedTime = app->time;
				}
			} else if (app->time < this->brightenPPMaxReachedTime + gameConfig->renderHoldBrightnessTime) {
				n2 = this->maxLocalBrightness;
			} else {
				n2 = std::min(this->brightenPPMaxReachedTime + gameConfig->renderHoldBrightnessTime + gameConfig->renderFadeBrightnessTime - app->time >> 4 & 0x7F,
				              this->maxLocalBrightness);
			}
			max = std::max((n2 * n2 << 10) / 127 >> 5, 32);
		}
	}

	if (app->player->isFamiliar) {
		if (this->brightenPostProcess) {
			graphics->FMGL_fillRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 1.0, 1.0, 1.0, (float)max / 2226.0);
		}
		update += 0.05f;
		y = (y + 1) % 30;
		float alpha = sin(update) * 0.0500000007 + 0.349999994;
		if ((uint16_t)(app->player->familiarType - 1) > 1u) {
			graphics->FMGL_fillRect(0, 0, Applet::IOS_WIDTH, 256 /*320*/, 1.0, 0.0, 0.0, alpha);
		} else {
			graphics->FMGL_fillRect(0, 0, Applet::IOS_WIDTH, 256 /*320*/, 0.0, 1.0, 0.0, alpha);
		}
		for (int i = 0; i < 256 /*300*/; i += 10) {
			graphics->FMGL_fillRect(0, i + (y / 3), Applet::IOS_WIDTH, 5, 0.0, 0.0, 0.0, 0.15);
		}
	}

#if 0
	for (int i = 1; i < Render.screenHeight - 1; ++i) {
		final int n3 = ((i & 0x3) == 0x3) ? 1 : 0;
		for (int j = 1; j < Render.screenWidth - 1; ++j) {
			if (Render.brightenPostProcess && n3 == 0) {
				max2 = Math.max(max, 32);
			}
			final int n4 = pixels[i * Render.screenWidth + j];
			int min = 2 * (n4 >> 8 & 0xFF) + (n4 >> 16 & 0xFF) + (n4 & 0xFF) >> 2;
			if (Render.brightenPostProcess && n3 == 0) {
				int max3 = Math.max(1, min);
				if (max2 != 0) {
					max3 = max3 * max2 >> 5;
				}
				min = Math.min(255, max3);
			}
			switch (Render.postProcessMode) {
			case 1: {
				pixels[i * Render.screenWidth + j] = (0xFF000000 | min >> 1 << 16 | min >> n3 << 8 | min >> 1);
				break;
			}
			case 2: {
				pixels[i * Render.screenWidth + j] = (0xFF000000 | min >> n3 << 16 | min >> 1 << 8 | min >> 1);
				break;
			}
			}
		}
	}
#endif
}

int Render::convertToGrayscale(int color) {
	int n2 = (color & 0xFF) + ((color >> 16 & 0xFF) - (color & 0xFF) >> 1);
	int n3 = n2 + ((color >> 8 & 0xFF) - n2 >> 1);
	return 0xFF000000 | n3 >> 1 << 16 | n3 >> 1 << 8 | n3 >> 1;
}


void Render::drawRGB(Graphics* graphics) {


	if (app->hud->cockpitOverlayRaw) { // [GEC]
		return;
	}

	this->bltTime = app->upTimeMs;
	int viewportX = app->canvas->viewRect[0] + app->tinyGL->viewportX;
	int viewportY = app->canvas->viewRect[1] + app->tinyGL->viewportY;

	int viewportWidth = app->tinyGL->viewportWidth;
	int viewportHeight = app->tinyGL->viewportHeight;

	if (!app->render->_gles->isInit) { // [GEC] Adjusted like this to match the Y position on the GL version
		viewportY = 0;
		viewportHeight = Applet::IOS_HEIGHT;
	}

	graphics->setColor(0x00000000);
	graphics->drawRect(viewportX - 1, viewportY - 1, viewportWidth + 2 - 1, viewportHeight + 2 - 1);
	if (this->fadeFlags != 0) {
		this->fadeScene(graphics);
	}
	this->_gles->SwapBuffers();
	this->bltTime = app->upTimeMs - this->bltTime;
}

void Render::rockView(int rockViewDur, int x, int y, int z) {

	this->rockViewDur = rockViewDur;
	this->rockViewTime = app->upTimeMs;
	this->rockViewX = x << 4;
	this->rockViewY = y << 4;
	this->rockViewZ = z << 4;
}

bool Render::isNPC(int n) {
	EntityDef* def = app->entityDefManager->lookup(n);
	return def && def->hasRenderFlag(EntityDef::RFLAG_NPC);
}

bool Render::hasNoFlareAltAttack(int n) {
	EntityDef* def = app->entityDefManager->lookup(n);
	return def && def->hasRenderFlag(EntityDef::RFLAG_NO_FLARE_ALT_ATTACK);
}

bool Render::hasGunFlare(int n) {
	EntityDef* def = app->entityDefManager->lookup(n);
	return def && def->hasRenderFlag(EntityDef::RFLAG_GUN_FLARE);
}

bool Render::isFloater(int n) {
	EntityDef* def = app->entityDefManager->lookup(n);
	return def && def->hasRenderFlag(EntityDef::RFLAG_FLOATER);
}

bool Render::isSpecialBoss(int n) {
	EntityDef* def = app->entityDefManager->lookup(n);
	return def && def->hasRenderFlag(EntityDef::RFLAG_SPECIAL_BOSS);
}

void Render::fixTexels(int offset, int i, int mediaID, int* rowHeight) { // [GEC] New
	if (mediaID == 814 && this->fixWaterAnim1) {
		if (offset == 5614 && (i & 1) == 0) {
			*rowHeight = 16;
		}
	} else if (mediaID == 815 && this->fixWaterAnim2) {
		if (offset == 7521 && (i & 1) == 0) {
			*rowHeight = 16;
		}
		if (offset == 7522 && (i & 1) == 1) {
			*rowHeight = 18;
		}
		if (offset == 7542 && (i & 1) == 1) {
			*rowHeight = 0;
		}
	} else if (mediaID == 816 && this->fixWaterAnim3) {
		if (offset == 7397 && (i & 1) == 0) {
			*rowHeight = 16;
		}
		if (offset == 7397 && (i & 1) == 1) {
			*rowHeight = 17;
		}
		if (offset == 7415 && (i & 1) == 0) {
			*rowHeight = 0;
		}
	} else if (mediaID == 817 && this->fixWaterAnim4) {
		if (offset == 6889 && (i & 1) == 1) {
			*rowHeight = 17;
		}
	}
}

