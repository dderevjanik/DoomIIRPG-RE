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

void Render::drawNodeLines(short n) {


	short n2 = this->nodeChildOffset2[n];
	for (int n3 = (n2 & 0x3FF) + (n2 >> 10 & 0x3F), i = n2 & 0x3FF; i < n3; ++i) {
		int n4 = this->lineFlags[i >> 1] >> ((i & 0x1) << 2) & 0xF;
		if ((n4 & 0x7) == 0x0 || (n4 & 0x7) == 0x4) {
			TGLVert* tglVert1 = &app->tinyGL->mv[0];
			TGLVert* tglVert2 = &app->tinyGL->mv[1];
			tglVert1->x = (this->lineXs[(i << 1) + 0] & 0xFF) << 7;
			tglVert2->x = (this->lineXs[(i << 1) + 1] & 0xFF) << 7;
			tglVert1->y = (this->lineYs[(i << 1) + 0] & 0xFF) << 7;
			tglVert2->y = (this->lineYs[(i << 1) + 1] & 0xFF) << 7;
			tglVert1->z = 0;
			tglVert2->z = 0;
			TGLVert* transform2DVerts = app->tinyGL->transform2DVerts(app->tinyGL->mv, 2);
			if (app->tinyGL->clipLine(transform2DVerts)) {
				app->tinyGL->projectVerts(transform2DVerts, 2);
				if (app->tinyGL->occludeClippedLine(&transform2DVerts[0], &transform2DVerts[1]) &&
				    app->game->updateAutomap) {
					this->lineFlags[i >> 1] |= (uint8_t)(8 << ((i & 0x1) << 2));
				}
			}
		} else if ((n4 & 0x7) == 0x6) {
			this->lineFlags[i >> 1] |= (uint8_t)(8 << ((i & 0x1) << 2));
		}
	}
}

bool Render::cullBoundingBox(int n, int n2, bool b) {
	return this->cullBoundingBox((n & 0xFFFFFFC0) << 4, (n2 & 0xFFFFFFC0) << 4, (n | 0x3F) << 4, (n2 | 0x3F) << 4, b);
}

bool Render::cullBoundingBox(int n, int n2, int n3, int n4, bool b) {


	if (this->skipCull) {
		return false;
	}
	if (this->viewX >= n - TinyGL::CULL_EXTRA && this->viewX <= n3 + TinyGL::CULL_EXTRA &&
	    this->viewY >= n2 - TinyGL::CULL_EXTRA && this->viewY <= n4 + TinyGL::CULL_EXTRA) {
		return false;
	}
	TGLVert* tglVert = &app->tinyGL->mv[0];
	TGLVert* tglVert2 = &app->tinyGL->mv[1];
	if (this->viewX < n) {
		if (this->viewY < n2) {
			tglVert->x = n3;
			tglVert->y = n2;
			tglVert2->x = n;
			tglVert2->y = n4;
		} else if (this->viewY < n4) {
			tglVert->x = n;
			tglVert->y = n2;
			tglVert2->x = n;
			tglVert2->y = n4;
		} else {
			tglVert->x = n;
			tglVert->y = n2;
			tglVert2->x = n3;
			tglVert2->y = n4;
		}
	} else if (this->viewX < n3) {
		if (this->viewY < n2) {
			tglVert->x = n3;
			tglVert->y = n2;
			tglVert2->x = n;
			tglVert2->y = n2;
		} else {
			if (this->viewY < n4) {
				return false;
			}
			tglVert->x = n;
			tglVert->y = n4;
			tglVert2->x = n3;
			tglVert2->y = n4;
		}
	} else if (this->viewY < n2) {
		tglVert->x = n3;
		tglVert->y = n4;
		tglVert2->x = n;
		tglVert2->y = n2;
	} else if (this->viewY < n4) {
		tglVert->x = n3;
		tglVert->y = n4;
		tglVert2->x = n3;
		tglVert2->y = n2;
	} else {
		tglVert->x = n;
		tglVert->y = n4;
		tglVert2->x = n3;
		tglVert2->y = n2;
	}
	tglVert->z = 0;
	tglVert2->z = 0;

	TGLVert* transform2DVerts = app->tinyGL->transform2DVerts(app->tinyGL->mv, 2);
	if (app->tinyGL->clipLine(transform2DVerts)) {
		app->tinyGL->projectVerts(transform2DVerts, 2);
		return !app->tinyGL->clippedLineVisCheck(&transform2DVerts[0], &transform2DVerts[1], b);
	}
	return true;
}

void Render::addSprite(short n) {


	if ((this->mapSpriteInfo[n] & 0x10000) != 0x0) {
		return;
	}
	int n2 = this->mapSpriteInfo[n] & 0xFF;
	Entity* entity = nullptr;
	if (this->mapSprites[this->S_ENT + n] != -1) {
		entity = &app->game->entities[this->mapSprites[this->S_ENT + n]];
	}
	int n3;
	if ((this->mapSpriteInfo[n] & 0x10000000) != 0x0) {
		n3 = 0x7fffffff;
	} else {
		short n4 = this->mapSprites[this->S_X + n];
		short n5 = this->mapSprites[this->S_Y + n];
		int* mvp = app->tinyGL->mvp;
		n3 = (n4 * mvp[2] + n5 * mvp[6] + this->mapSprites[this->S_Z + n] * mvp[10] >> 14) + mvp[14];
		if ((this->mapSpriteInfo[n] & 0x400000) != 0x0) {
			n3 += 6;
		} else if (n2 == 240 || n2 == 246 || n2 == 245 || n2 == 247) {
			n3 = 0x80000000;
		} else if (0x0 != (this->mapSpriteInfo[n] & 0xF000000)) {
			n3 += 5;
		} else if (entity != nullptr && (entity->info & 0x1010000) != 0x0) {
			++n3;
		} else if (entity != nullptr && entity->def->eType == Enums::ET_MONSTER) {
			this->handleMonsterIdleSounds(entity);
			--n3;
		} else if ((n2 >= 240 && n2 <= 244) || n2 == 241 || n2 == 255) {
			n3 -= 3;
		} else if (n2 == 138 || n2 == 139 || n2 == 137) {
			n3 += 2;
		} else if (n2 == 152) {
			n3 += 5;
		} else if (n2 == 239) {
			n3 -= 3;
		}
	}
	this->mapSpriteInfo[this->SINFO_SORTZ + n] = n3;
	if (app->game->updateAutomap) {
		this->mapSpriteInfo[n] |= 0x200000;
	}
	if (this->viewSprites == -1) {
		this->mapSprites[this->S_VIEWNEXT + n] = -1;
		this->viewSprites = n;
	} else if (n3 >= this->mapSpriteInfo[this->SINFO_SORTZ + this->viewSprites]) {
		this->mapSprites[this->S_VIEWNEXT + n] = this->viewSprites;
		this->viewSprites = n;
	} else {
		short viewSprites;
		for (viewSprites = this->viewSprites;
		     this->mapSprites[this->S_VIEWNEXT + viewSprites] != -1 &&
		     n3 < this->mapSpriteInfo[this->SINFO_SORTZ + this->mapSprites[this->S_VIEWNEXT + viewSprites]];
		     viewSprites = this->mapSprites[this->S_VIEWNEXT + viewSprites]) {
		}
		this->mapSprites[this->S_VIEWNEXT + n] = this->mapSprites[this->S_VIEWNEXT + viewSprites];
		this->mapSprites[this->S_VIEWNEXT + viewSprites] = n;
	}
}

void Render::addSplitSprite(int n, int n2) {
	for (int i = n; i < this->numVisibleNodes; ++i) {
		short n3 = this->nodeIdxs[i];
		int n4 = (this->nodeBounds[(n3 << 2) + 0] & 0xFF) << 3;
		int n5 = (this->nodeBounds[(n3 << 2) + 1] & 0xFF) << 3;
		int n6 = (this->nodeBounds[(n3 << 2) + 2] & 0xFF) << 3;
		int n7 = (this->nodeBounds[(n3 << 2) + 3] & 0xFF) << 3;
		short n8 = this->mapSprites[this->S_X + n2];
		short n9 = this->mapSprites[this->S_Y + n2];
		if (n4 < n8 + 8 && n6 > n8 - 8 && n5 < n9 + 8 && n7 > n9 - 8 && this->numSplitSprites < 8) {
			this->splitSprites[this->numSplitSprites++] = (n3 << 16 | n2);
			return;
		}
	}
}

void Render::addNodeSprites(short n) {
	if ((this->nodeOffsets[n] & 0xFFFF) == 0xFFFF) {
		for (short n2 = this->nodeSprites[n]; n2 != -1; n2 = this->mapSprites[this->S_NODENEXT + n2]) {
			this->addSprite(n2);
		}
		for (int i = 0; i < this->numSplitSprites; ++i) {
			if ((this->splitSprites[i] & 0xFFFF0000) >> 16 == n) {
				this->addSprite((short)(this->splitSprites[i] & 0xFFFF));
				this->splitSprites[i] = -1;
			}
		}
	}
}

int Render::nodeClassifyPoint(int n, int n2, int n3, int n4) {
	int n5 = (this->nodeNormalIdxs[n] & 0xFF) * 3;
	return (((n2 * this->normals[n5]) + (n3 * this->normals[n5 + 1]) + (n4 * this->normals[n5 + 2])) >> 14) +
	       (this->nodeOffsets[n] & 0xFFFF);
}

void Render::drawNodeGeometry(short n) {


	int iVar10;
	int offset;
	bool bVar15;
	uint32_t z, s, t;

	if ((this->nodeOffsets[n] & 0xFFFF) != 0xFFFF) {
		return;
	}

	offset = this->nodeChildOffset1[n] & 0xffff;
	int meshCount = this->nodePolys[offset++];

	for (int i = 0; i < meshCount; i++) {
		uint16_t uVar3 = this->nodePolys[offset + 4] | (this->nodePolys[offset + 5] << 8);
		offset += 6;
		uint32_t uVar7 = (uint32_t)(uVar3 >> 7);
		iVar10 = Render::RENDER_NORMAL;
		app->tinyGL->faceCull = TinyGL::CULL_CCW;

		if (uVar7 == TILE_HELL_HANDS) {
			iVar10 = Render::RENDER_BLEND50;
		} else if ((uVar7 == TILE_FADE) || (uVar7 == TILE_SCORCH_MARK)) {
			iVar10 = Render::RENDER_NORMAL;
			if (!this->_gles->isInit) {
				iVar10 = Render::RENDER_SUB; // [GEC] TinyGL Only like J2ME/BREW
			}
		} else if ((uVar7 == TILE_FLAT_LAVA) || (uVar7 == TILE_FLAT_LAVA2)) {
			iVar10 = Render::RENDER_NORMAL;
			app->tinyGL->faceCull = TinyGL::CULL_NONE;
		}

		this->setupTexture(uVar7, 0, iVar10, 0);
		if (uVar7 == TILE_FLAT_LAVA) {
			z = 2 * (this->sinTable[app->time / 2 & 0x3FF] - this->sinTable[256]) >> 14;
			s = (app->time / 16 & 0x3FF);
			t = (app->time / 32 & 0x3FF);
		} else if (uVar7 == TILE_FLAT_LAVA2) {
			z = 0;
			s = 0;
			t = (app->time / 4 & 0x3FF);
		} else {
			z = 0;
			s = 0;
			t = 0;
		}

		for (int j = 0; j < (int)(uVar3 & 127); j++) {
			int polyFlags = this->nodePolys[offset++];
			int numVerts = (polyFlags & Enums::POLY_FLAG_VERTS_MASK) + 2;
			app->tinyGL->swapXY = (polyFlags & Enums::POLY_FLAG_SWAPXY) ? true : false;
			for (int k = 0; k < numVerts; k++) {
				TGLVert* vert = &app->tinyGL->mv[k];
				vert->x = (((uint32_t)this->nodePolys[offset + 0] & 0xFF) << 7);
				vert->y = (((uint32_t)this->nodePolys[offset + 1] & 0xFF) << 7);
				vert->z = (((uint32_t)this->nodePolys[offset + 2] & 0xFF) << 7) + z;
				vert->s = (((int8_t)this->nodePolys[offset + 3]) << 6) + s;
				vert->t = (((int8_t)this->nodePolys[offset + 4]) << 6) + t;
				offset += 5;
			}
			if (numVerts == 2) {
				numVerts = 4;

				TGLVert* vert = &app->tinyGL->mv[2];
				std::memcpy(&app->tinyGL->mv[2], &app->tinyGL->mv[1], sizeof(TGLVert));
				std::memcpy(&app->tinyGL->mv[1], vert, sizeof(TGLVert));

				app->tinyGL->mv[2].x = app->tinyGL->mv[0].x;
				app->tinyGL->mv[2].y = app->tinyGL->mv[0].y;
				app->tinyGL->mv[2].z = app->tinyGL->mv[0].z;
				app->tinyGL->mv[2].s = app->tinyGL->mv[0].s;
				app->tinyGL->mv[2].t = app->tinyGL->mv[0].t;

				app->tinyGL->mv[3].x = app->tinyGL->mv[2].x;
				app->tinyGL->mv[3].y = app->tinyGL->mv[2].y;
				app->tinyGL->mv[3].z = app->tinyGL->mv[2].z;
				app->tinyGL->mv[3].s = app->tinyGL->mv[2].s;
				app->tinyGL->mv[3].t = app->tinyGL->mv[2].t;

				switch (polyFlags & Enums::POLY_FLAG_AXIS_MASK) {
					case Enums::POLY_FLAG_AXIS_X: {
						app->tinyGL->mv[1].x = app->tinyGL->mv[2].x;
						app->tinyGL->mv[3].x = app->tinyGL->mv[0].x;
						break;
					}
					case Enums::POLY_FLAG_AXIS_Y: {
						app->tinyGL->mv[1].y = app->tinyGL->mv[2].y;
						app->tinyGL->mv[3].y = app->tinyGL->mv[0].y;
						break;
					}
					case Enums::POLY_FLAG_AXIS_Z: {
						app->tinyGL->mv[1].z = app->tinyGL->mv[2].z;
						app->tinyGL->mv[3].z = app->tinyGL->mv[0].z;
						break;
					}
				}

				if ((polyFlags & Enums::POLY_FLAG_UV_DELTAX)) {
					app->tinyGL->mv[1].s = app->tinyGL->mv[2].s;
					app->tinyGL->mv[3].s = app->tinyGL->mv[0].s;
				} else {
					app->tinyGL->mv[1].t = app->tinyGL->mv[2].t;
					app->tinyGL->mv[3].t = app->tinyGL->mv[0].t;
				}
			}
			app->tinyGL->drawModelVerts(app->tinyGL->mv, numVerts);
		}
		app->tinyGL->span = nullptr;
	}
}

void Render::walkNode(short n) {
	// printf("walkNode %d\viewPitch", viewPitch);


	if (this->cullBoundingBox(
	        (this->nodeBounds[(n << 2) + 0] & 0xFF) << 7, (this->nodeBounds[(n << 2) + 1] & 0xFF) << 7,
	        (this->nodeBounds[(n << 2) + 2] & 0xFF) << 7, (this->nodeBounds[(n << 2) + 3] & 0xFF) << 7, true)) {
		return;
	}
	++this->nodeCount;
	if ((this->nodeOffsets[n] & 0xFFFF) == 0xFFFF) {
		if (this->numVisibleNodes < 256) {
			this->nodeIdxs[this->numVisibleNodes++] = n;
		}
		++this->leafCount;
		if (!this->skipLines) {
			this->drawNodeLines(n);
		}

		for (short n2 = this->nodeSprites[n]; n2 != -1; n2 = this->mapSprites[this->S_NODENEXT + n2]) {
			int n3 = this->mapSpriteInfo[n2];
			if ((n3 & 0x10000) == 0x0) {
				int n4 = this->mapSpriteInfo[n2] & 0xFF;
				Entity* entity = nullptr;
				if (this->mapSprites[this->S_ENT + n2] != -1) {
					entity = &app->game->entities[this->mapSprites[this->S_ENT + n2]];
				}
				if (entity != nullptr && (n3 & 0x400000) != 0x0) {
					n4 += 257;
					if (n4 < 450) {
						this->occludeSpriteLine(n2);
					}
				}
			}
		}
		return;
	}

	int numVisibleNodes = this->numVisibleNodes;
	if (nodeClassifyPoint(n, this->viewX, this->viewY, this->viewZ) >= 0) {
		walkNode(this->nodeChildOffset1[n]);
		walkNode(this->nodeChildOffset2[n]);
	} else {
		walkNode(this->nodeChildOffset2[n]);
		walkNode(this->nodeChildOffset1[n]);
	}
	for (short n5 = this->nodeSprites[n]; n5 != -1; n5 = this->mapSprites[this->S_NODENEXT + n5]) {
		this->addSplitSprite(numVisibleNodes, n5);
	}
}

int Render::dot(int n, int n2, int n3, int n4) {
	return (n * n3) + (n2 * n4);
}

int Render::CapsuleToCircleTrace(int* array, int n, int n2, int n3, int n4) {
	int n5 = array[2] - array[0];
	int n6 = array[3] - array[1];
	int n7 = n2 - array[0];
	int n8 = n3 - array[1];
	int n9 = n5 * n5 + n6 * n6;
	int n10 = n7 * n5 + n8 * n6;
	if (n9 == 0) {
		return 0;
	}
	if (n10 < 0) {
		n10 = 0;
	}
	if (n10 > n9) {
		n10 = n9;
	}
	int n11 = (int)(((int64_t)n10 << 16) / n9);
	int n12 = n2 - (array[0] + (n5 * n11 >> 16));
	int n13 = n3 - (array[1] + (n6 * n11 >> 16));
	if (n12 * n12 + n13 * n13 < n + n4) {
		return (n11 >> 2) - 1;
	}
	return 16384;
}

int Render::CapsuleToLineTrace(int* array, int n, int* array2) {
	int n2 = array[2] - array[0];
	int n3 = array[3] - array[1];
	int n4 = array2[2] - array2[0];
	int n5 = array2[3] - array2[1];
	int n6 = array[0] - array2[0];
	int n7 = array[1] - array2[1];
	int dot = this->dot(n2, n3, n2, n3);
	int dot2 = this->dot(n2, n3, n4, n5);
	int dot3 = this->dot(n4, n5, n4, n5);
	int dot4 = this->dot(n2, n3, n6, n7);
	int dot5 = this->dot(n4, n5, n6, n7);
	int64_t n9;
	int64_t n8;
	int64_t n10;
	int64_t n11;
	if ((n8 = (n9 = dot * (int64_t)dot3 - dot2 * (int64_t)dot2)) < 0LL) {
		n10 = 0LL;
		n9 = 1LL;
		n11 = dot5;
		n8 = dot3;
	} else {
		n10 = dot2 * (int64_t)dot5 - dot3 * (int64_t)dot4;
		n11 = dot * (int64_t)dot5 - dot2 * (int64_t)dot4;
		if (n10 < 0LL) {
			n10 = 0LL;
			n11 = dot5;
			n8 = dot3;
		} else if (n10 > n9) {
			n10 = n9;
			n11 = dot5 + dot2;
			n8 = dot3;
		}
	}
	if (n11 < 0LL) {
		n11 = 0LL;
		if (-dot4 < 0) {
			n10 = 0LL;
		} else if (-dot4 > dot) {
			n10 = n9;
		} else {
			n10 = -dot4;
			n9 = dot;
		}
	} else if (n11 > n8) {
		n11 = n8;
		if (-dot4 + dot2 < 0) {
			n10 = 0LL;
		} else if (-dot4 + dot2 > dot) {
			n10 = n9;
		} else {
			n10 = -dot4 + dot2;
			n9 = dot;
		}
	}
	int n12;
	if (n10 == 0LL) {
		n12 = 0;
	} else {
		n12 = (int)((n10 << 16) / n9);
	}
	int n13;
	if (n11 == 0LL) {
		n13 = 0;
	} else {
		n13 = (int)((n11 << 16) / n8);
	}
	int n14 = (n6 << 16) + n12 * n2 - n13 * n4 >> 16;
	int n15 = (n7 << 16) + n12 * n3 - n13 * n5 >> 16;
	if (this->dot(n14, n15, n14, n15) < n) {
		return (n12 >> 2) - 1;
	}
	return 16384;
}

int Render::traceWorld(int n, int* array, int n2, int* array2, int n3) {
	if (array2[0] > ((this->nodeBounds[(n << 2) + 2] & 0xFF) << 3)) {
		return 16384;
	}
	if (array2[2] < ((this->nodeBounds[(n << 2) + 0] & 0xFF) << 3)) {
		return 16384;
	}
	if (array2[1] > ((this->nodeBounds[(n << 2) + 3] & 0xFF) << 3)) {
		return 16384;
	}
	if (array2[3] < ((this->nodeBounds[(n << 2) + 1] & 0xFF) << 3)) {
		return 16384;
	}
	if ((this->nodeOffsets[n] & 0xFFFF) == 0xFFFF) {
		int n4 = 16384;
		short n5 = this->nodeChildOffset2[n];
		int n6 = (n5 & 0x3FF) + (n5 >> 10 & 0x3F);
		int i = n5 & 0x3FF;
		while (i < n6) {
			int* traceLine = this->traceLine;
			int n7 = this->lineFlags[i >> 1] >> ((i & 0x1) << 2) & 0xF & 0x7;
			traceLine[0] = (this->lineXs[(i << 1) + 0] & 0xFF) << 3;
			traceLine[2] = (this->lineXs[(i << 1) + 1] & 0xFF) << 3;
			traceLine[1] = (this->lineYs[(i << 1) + 0] & 0xFF) << 3;
			traceLine[3] = (this->lineYs[(i << 1) + 1] & 0xFF) << 3;
			++i;
			if (n7 != 4) {
				if (n7 == 6) {
					continue;
				}
				if (n7 == 5 && (n3 & 0x10) == 0x0 && (n3 & 0x800) == 0x0) {
					continue;
				}
				if (n7 == 7 && (traceLine[0] - array[0]) * (traceLine[3] - traceLine[1]) +
				                       (traceLine[1] - array[1]) * -(traceLine[2] - traceLine[0]) <=
				                   0) {
					continue;
				}
				if (traceLine[0] > array2[2] && traceLine[2] > array2[2]) {
					continue;
				}
				if (traceLine[0] < array2[0] && traceLine[2] < array2[0]) {
					continue;
				}
				if (traceLine[1] > array2[3] && traceLine[3] > array2[3]) {
					continue;
				}
				if (traceLine[1] < array2[1] && traceLine[3] < array2[1]) {
					continue;
				}
				int capsuleToLineTrace = this->CapsuleToLineTrace(array, n2 * n2, traceLine);
				if (capsuleToLineTrace >= n4) {
					continue;
				}
				n4 = capsuleToLineTrace;
			}
		}
		return n4;
	}
	if (this->nodeClassifyPoint(n, array[0] << 4, array[1] << 4, this->viewZ) >= 0) {
		int traceWorld = this->traceWorld(this->nodeChildOffset1[n], array, n2, array2, n3);
		if (traceWorld == 16384) {
			return this->traceWorld(this->nodeChildOffset2[n], array, n2, array2, n3);
		}
		return traceWorld;
	} else {
		int traceWorld2 = traceWorld(this->nodeChildOffset2[n], array, n2, array2, n3);
		if (traceWorld2 == 16384) {
			return traceWorld(this->nodeChildOffset1[n], array, n2, array2, n3);
		}
		return traceWorld2;
	}
}

