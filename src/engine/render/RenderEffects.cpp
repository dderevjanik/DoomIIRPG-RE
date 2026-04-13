#include <stdexcept>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "DataNode.h"
#include "JavaStream.h"
#include "Resource.h"
#include "MayaCamera.h"
#include "Canvas.h"
#include "Image.h"
#include "Render.h"
#include "Game.h"
#include "Text.h"
#include "GLES.h"
#include "TGLVert.h"
#include "TGLEdge.h"
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
#include "Span.h"
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

void Render::buildFogTable() {
	int fogTableFrac = this->fogTableFrac;
	int fogTableColor = this->fogTableColor;
	uint16_t* fogTableBase = this->fogTableBase;
	uint16_t* fogTableDest = this->fogTableDest;
	for (int i = 0; i < this->fogTableBaseSize; i++) {
		uint16_t dat = fogTableBase[i];
		fogTableDest[i] = (((fogTableFrac >> 2) * ((dat & 0x07e0) >> 6)) & 0x07e0 |
		                   (((fogTableFrac >> 3) * (dat & 0xF81F)) >> 5) & 0xF81F) +
		                  fogTableColor;
	}
}

void Render::buildFogTable(int n, int n2, int n3) {
	int n4 = (n3 & 0xFF000000) >> 24 & 0xFF;
	int n5 = this->mediaMappings[n] + n2;
	if ((this->mediaPalColors[n5] & 0x4000) != 0x0) {
		uint16_t** array = this->mediaPalettes[this->mediaPalColors[n5] & 0x3FFF];
		this->fogTableBase = array[1];
		this->fogTableBaseSize = this->mediaPalettesSizes[this->mediaPalColors[n5] & 0x3FFF]; // Pc port only
		for (int i = 1; i < 16; ++i) {
			int n6 = (i << 8) / 16 * n4 >> 8;
			this->fogTableColor = (((n3 & 0xFF00FF00) >> 8) * n6 & 0xFF00FF00) | ((n3 & 0xFF00FF) * n6 >> 8 & 0xFF00FF);
			this->fogTableColor = Render::upSamplePixel(this->fogTableColor);
			this->fogTableFrac = 256 - n6;
			this->fogTableDest = array[i];
			this->buildFogTable();
		}
	}
}

void Render::buildFogTables(int fogColor) {


	app->tinyGL->fogColor = fogColor;

	if ((fogColor & 0xFF000000) == 0x0) {
		app->tinyGL->fogMin = 32752;
		app->tinyGL->fogRange = 1;
		return;
	}

	int n = (fogColor & 0xFF000000) >> 24 & 0xFF;
	for (int i = 1; i < 16; ++i) {
		int n2 = (i << 8) / 16 * n >> 8;
		this->fogTableColor = Render::upSamplePixel((((app->tinyGL->fogColor & 0xFF00FF00) >> 8) * n2 & 0xFF00FF00) |
		                                            ((app->tinyGL->fogColor & 0xFF00FF) * n2 >> 8 & 0xFF00FF));
		this->fogTableFrac = 256 - n2;
		for (int j = 0; j < 1024; ++j) {
			if (this->mediaPalettes[j][0] != nullptr) {
				this->fogTableBase = this->mediaPalettes[j][0];
				this->fogTableDest = this->mediaPalettes[j][i];
				this->fogTableBaseSize = this->mediaPalettesSizes[j]; // Pc port only
				buildFogTable();
			}
		}
		this->fogTableBase = this->skyMapPalette[0];
		this->fogTableDest = this->skyMapPalette[i];
		this->fogTableBaseSize = 256; // Pc port only
		this->buildFogTable();
	}
	this->buildFogTable(234, 0, ((n * 180 >> 8 & 0xFF) << 24 | (app->tinyGL->fogColor & 0xFFFFFF)));
	this->buildFogTable(234, 0, 0xFF000000); // IOS
}

void Render::setupPalette(uint16_t* spanPalette, int renderMode, int renderFlags) {
	// Pendiente


	bool isMultiply = (renderFlags & Render::RENDER_FLAG_MULTYPLYSHIFT) ? true : false; // [GEC]

	uint16_t* array = app->tinyGL->scratchPalette;
	app->tinyGL->spanPalette = spanPalette;
	if (renderFlags & 0x4) { // RENDER_FLAG_GREYSHIFT
		for (int i = 0; i < app->tinyGL->paletteBaseSize; ++i) {
			uint16_t n2 = spanPalette[i];
			// int n3 = ((n2 & 0xFF) + (n2 >> 8 & 0xFF) + (n2 >> 16 & 0xFF)) / 3;
			// array[i] = (n3 << 16 | n3 << 8 | n3);

			// int64_t v10 = 0x55555556 * (int64_t)(LOBYTE(*(uint16_t*)&spanPalette[i]) +
			// HIBYTE(*(uint16_t*)&spanPalette[i])); array[i] = (uint16_t)((int)v10 << 8) | (uint16_t)v10;

			uint16_t grayColor = (((n2 & 0xf800) >> 10) + ((n2 >> 5) & 0x3f) + ((n2 & 0x1f) << 1)) / 3; // RGB
			array[i] = ((grayColor >> 1) << 11) | (grayColor << 5) | (grayColor >> 1);
		}
		app->tinyGL->paletteBase = &app->tinyGL->scratchPalette;
		app->tinyGL->spanPalette = array;
	} else if ((renderFlags & 0x1E8) !=
	           0x0) { //(RENDER_FLAG_WHITESHIFT | RENDER_FLAG_BLUESHIFT | RENDER_FLAG_GREENSHIFT | RENDER_FLAG_REDSHIFT
		              //| RENDER_FLAG_BRIGHTREDSHIFT)
		int n4 = 0;
		switch (renderFlags & 0x1E8) {
			case 8: {                                    // RENDER_FLAG_WHITESHIFT
				n4 = Render::RGB888ToRGB565(64, 64, 64); // 16904;
				break;
			}
			case 32: { // RENDER_FLAG_BLUESHIFT
				if (isMultiply) {
					n4 = Render::RGB888ToRGB565(255, 255, 0); // color inverted 0x0000FF
				} else {
					n4 = Render::RGB888ToRGB565(0, 0, 64); // 8;
				}
				break;
			}
			case 64: { // RENDER_FLAG_GREENSHIFT
				if (isMultiply) {
					n4 = Render::RGB888ToRGB565(255, 0, 255); // color inverted 0x00FF00
				} else {
					n4 = Render::RGB888ToRGB565(0, 64, 0); // 512;
				}
				break;
			}
			case 128: { // RENDER_FLAG_REDSHIFT
				if (isMultiply) {
					n4 = Render::RGB888ToRGB565(0, 255, 255); // color inverted 0xFF0000
				} else {
					n4 = Render::RGB888ToRGB565(64, 0, 0); // 16384;
				}
				break;
			}
			case 256: {                                 // RENDER_FLAG_BRIGHTREDSHIFT
				n4 = Render::RGB888ToRGB565(128, 0, 0); // 32768; // Player Only
				break;
			}
		}
		for (int j = 0; j < app->tinyGL->paletteBaseSize; ++j) {
			uint32_t n5, n6, n7;
			if (isMultiply) {
				n5 = (spanPalette[j] | 0x10821) - (n4 & 0xFFFFF7DE);
				n6 = n5 & 0x10820;
				n7 = (n5 ^ n6) & n6 - (n6 >> 4);
			} else {
				n5 = (spanPalette[j] & 0xF7DE) + (n4 & 0xffff);
				n6 = (n5 & 0x10820);
				n7 = (uint16_t)n5 ^ (uint16_t)n6;
				if (n6 & 0x1f000) {
					n7 |= 0xF800;
				}
				if (n6 & 0xfc0) {
					n7 |= 0x7E0;
				}
				if (n6 & 0x3e) {
					n7 |= 0x1F;
				}
			}

			array[j] = n7;
		}
		app->tinyGL->paletteBase = &app->tinyGL->scratchPalette;
		app->tinyGL->spanPalette = array;
	} else if ((renderFlags & 0x200) != 0x0) { // RENDER_FLAG_PULSATE
		int n7 = app->time >> 2 & 0x1FF;
		if ((n7 & 0x100) != 0x0) {
			n7 = 511 - n7;
		}
		uint16_t n8 = (n7 >> 1) + 127;
		uint16_t rgb565 = Render::upSamplePixel(n8 << 16 | n8 << 8 | n8);
		for (int k = 0; k < app->tinyGL->paletteBaseSize; ++k) {
			array[k] = rgb565;
		}
		app->tinyGL->paletteBase = &app->tinyGL->scratchPalette;
		app->tinyGL->spanPalette = array;
	} else if (renderMode != 0) {
		app->tinyGL->paletteBase = &app->tinyGL->scratchPalette;
		app->tinyGL->spanPalette = array;
		int n9 = 0;
		int n10 = 0;
		switch (renderMode) {
			default: {
				n9 = 0xFFFFE79C;
				n10 = 1;
				break;
			}
			case 1: {
				n9 = 0xFFFFE79C;
				n10 = 2;
				break;
			}
			case 3: {
				n9 = 0xFFFFF7DE;
				n10 = 0;
				break;
			}
			case 5: {
				n9 = 0xFFFFE79C;
				n10 = 1;
				break;
			}
			case 6: {
				n9 = 0xFFFFC718;
				n10 = 2;
				break;
			}
			case 4: {
				for (int l = 0; l < app->tinyGL->paletteBaseSize; ++l) {
					array[l] = ((spanPalette[l] & 0xFFFFE79C) >> 1) + ((spanPalette[l] & 0xFFFFC718) >> 2);
				}
				return;
			}
		}
		for (int n11 = 0; n11 < app->tinyGL->paletteBaseSize; ++n11) {
			array[n11] = (spanPalette[n11] & n9) >> n10;
		}
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

uint16_t* Render::getPalette(int n, int n2, int n3) {
	return this->mediaPalettes[this->mediaPalColors[this->mediaMappings[n] + n2] & 0x3FFF][n3];
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

	if (n < 257 && n != 175 && n != 162 && n != 129 && n != 173 && n != 184) {
		app->tinyGL->span = &this->_spanTrans[renderMode];
	} else {
		app->tinyGL->span = &this->_spanTexture[renderMode];
	}

	int n5;
	int n6;
	if (n == TILE_SKY_BOX) {
		app->tinyGL->textureBase = this->skyMapTexels;
		app->tinyGL->paletteBase = this->skyMapPalette;
		app->tinyGL->textureBaseSize = 256 * 256; // new
		app->tinyGL->paletteBaseSize = 256;       // new
		app->tinyGL->mediaID = -1;                // new
		this->isSkyMap = true;
		n5 = 8;
		n6 = 8;

	} else {
		app->tinyGL->textureBase = this->mediaTexels[this->mediaTexelSizes[n4] & 0x3FFF];
		app->tinyGL->paletteBase = this->mediaPalettes[this->mediaPalColors[n4] & 0x3FFF];
		app->tinyGL->textureBaseSize = this->mediaTexelSizes2[this->mediaTexelSizes[n4] & 0x3FFF];  // [GEC] new
		app->tinyGL->paletteBaseSize = this->mediaPalettesSizes[this->mediaPalColors[n4] & 0x3FFF]; // [GEC] new
		app->tinyGL->mediaID = n4;                                                                  // new

		// [GEC] new
		app->tinyGL->paletteTransparentMask = -1;
		for (int i = 0; i < app->tinyGL->paletteBaseSize; i++) {
			if (app->tinyGL->paletteBase[0][i] == 0xF81F) {
				app->tinyGL->paletteTransparentMask = i;
			}
		}

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
	app->tinyGL->sShift = 26 - n6;
	app->tinyGL->sMask = app->tinyGL->sWidth - 1;
	app->tinyGL->tHeight = 1 << n5;
	app->tinyGL->tShift = 26 - (n5 + n6);
	app->tinyGL->tMask = (app->tinyGL->tHeight - 1) * app->tinyGL->sWidth;
}

void Render::drawSkyMap(int n2) {


	uint8_t* p_skyMapTexels; // r4
	uint16_t* FogPalette;    // r0
	int16_t v7;              // r2
	int16_t v9;              // r12
	int v11;                 // lr
	int v12;                 // r5
	int v13;                 // r12
	uint16_t* pixels;        // r1
	int v15;                 // r3
	int v16;                 // r3

	p_skyMapTexels = this->skyMapTexels;
	app->tinyGL->paletteBase = this->skyMapPalette;
	FogPalette = app->tinyGL->getFogPalette(0x10000000);

	v7 = n2 + 255;
	if (n2 >= 0) {
		v7 = n2;
	}
	v9 = (v7 & 0xFF00) + (app->tinyGL->viewportY << 8) - 256;

	// Original BREW version
#if 0
	for (int i = app->tinyGL->viewportY; i < app->tinyGL->viewportY2; i++) {
		v11 = v9 & 0xFF00;
		v12 = app->tinyGL->viewportX2 - app->tinyGL->viewportX;
		v13 = n2;
		pixels = &app->tinyGL->pixels[app->tinyGL->viewportX + i * app->tinyGL->screenWidth];
		while (v12 >= 8) {
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 & 255)]];
			v13++;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 & 255)]];
			v13++;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 & 255)]];
			v13++;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 & 255)]];
			v13++;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 & 255)]];
			v13++;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 & 255)]];
			v13++;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 & 255)]];
			v13++;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 & 255)]];
			v13++;
			v12 -= 8;
		}
		while (--v12 >= 0) {
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 & 255)]];
			v13++;
		}
		v9 = v11 + 256;
	}
#endif

	// [GEC] It is adjusted like this to be consistent with the IOS version
	for (int i = app->tinyGL->viewportY; i < app->tinyGL->viewportY2; i++) {
		v11 = v9 & 0xFF00;
		v12 = app->tinyGL->viewportX2 - app->tinyGL->viewportX;
		v13 = (n2 + 132) << 8;
		pixels = &app->tinyGL->pixels[app->tinyGL->viewportX + i * app->tinyGL->screenWidth];
		while (v12 >= 8) {
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 >> 8 & 255)]];
			v13 += 128;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 >> 8 & 255)]];
			v13 += 128;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 >> 8 & 255)]];
			v13 += 128;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 >> 8 & 255)]];
			v13 += 128;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 >> 8 & 255)]];
			v13 += 128;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 >> 8 & 255)]];
			v13 += 128;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 >> 8 & 255)]];
			v13 += 128;
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 >> 8 & 255)]];
			v13 += 128;
			v12 -= 8;
		}
		while (--v12 >= 0) {
			*pixels++ = FogPalette[p_skyMapTexels[v11 | (v13 >> 8 & 255)]];
			v13 += 128;
		}
		v9 = v11 + 256;
	}
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
	bool b = (n4 & 0xF000000) != 0x0;
	int n6 = n4 & 0xFF;
	if ((n4 & 0x400000) != 0x0) {
		n6 += 256 + 1;
	}

	bool b2 = n6 == 240;
	while (i != 65535) {
		int nodeClassifyPoint = this->nodeClassifyPoint(n5, n, n2, n3);
		if (nodeClassifyPoint == 0 && b) {
			if ((n4 & 0x9000000) != 0x0) {
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
			mapSprites[this->S_Z + i] -= 32;
		}
		int n3 = (uint8_t)(mapSpriteInfo[i] & 0xFF);
		if ((mapSpriteInfo[i] & 0x400000) != 0x0) {
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
		mapSprites[this->S_Z + i] = 32;
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
		mapSprites[this->S_Z + i] = 32;
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

void Render::fillWeaponRect(int n, int n2, int n3, int n4, int n5) {

	int n6 = n2 * this->screenWidth + n;
	int n7 = n6 + n3;
	int i = n6;
	int j = 0;
	uint16_t* pixels = app->tinyGL->pixels;
	while (j < n4) {
		while (i < n7) {
			pixels[i] = n5;
			++i;
		}
		n6 += this->screenWidth;
		n7 += this->screenWidth;
		i = n6;
		++j;
	}
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

//--------------------
void DrawBitmap(short* buffer, int buffW, int buffH, int x, int y, int w, int h) {
	static GLuint textureName = -1;
	float vp[12]; // [sp+18h] [bp-68h] BYREF
	float st[8];  // [sp+48h] [bp-38h] BYREF

	PFNGLACTIVETEXTUREPROC glActiveTexture = (PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");

	vp[2] = 0.0;
	vp[5] = 0.0;
	vp[8] = 0.0;
	vp[11] = 0.0;
	vp[0] = (float)(x + w);
	vp[1] = (float)y;
	vp[6] = vp[0];
	vp[4] = (float)y;
	vp[3] = (float)x;
	vp[9] = (float)x;
	vp[7] = (float)(y + h);
	vp[10] = vp[7];
	memset(&st[1], 0, 12);
	st[6] = 0.0;
	st[0] = (float)w / (float)buffW;
	st[4] = st[0];
	st[5] = (float)h / (float)buffH;
	st[7] = st[5];
	if (textureName == -1) {
		glGenTextures(1, &textureName);
	}
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buffW, buffH, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, buffer);
	glVertexPointer(3, GL_FLOAT, 0, vp);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, st);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Render::Render3dScene(void) {
	short* backBuffer = (short*)this->app->backBuffer->pBmp;

	int cx, cy;
	int w = this->sdlGL->vidWidth;
	int h = this->sdlGL->vidHeight;
	SDL_GL_GetDrawableSize(this->sdlGL->window, &cx, &cy);
	if (w != cx || h != cy) {
		w = cx;
		h = cy;
	}
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, -1.0, 1.0);
	// glRotatef(90.0, 0.0, 0.0, 1.0);
	// glTranslatef(0.0, -320.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// DrawBitmap(backBuffer, 480, 320, 0, 21, 480, 320); // <- Old
	DrawBitmap(backBuffer, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
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

