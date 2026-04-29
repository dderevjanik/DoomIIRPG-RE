#include <span>
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
#include "EntityDef.h"
#include "SpriteDefs.h"

void Render::draw2DSprite(int tileNum, int frame, int x, int y, int flags, int renderMode, int renderFlags,
                          int scaleFactor) {

	TinyGL* tinyGL = app->tinyGL.get();

	TGLVert* vert1 = &tinyGL->cv[0];
	TGLVert* vert2 = &tinyGL->cv[1];
	TGLVert* vert3 = &tinyGL->cv[2];

	// RENDER_FLAG_SCALE_WEAPON
	if (renderFlags &
	    Render::RENDER_FLAG_SCALE_WEAPON) { // [GEC] Adjusted like this to match the scaleFactor on the GL version
		scaleFactor = (int)((float)scaleFactor * 1.35f);
	}

	this->setupTexture(tileNum, frame, renderMode, renderFlags);

	int scaledHeight = (176 * scaleFactor) / 0x10000;

	vert1->x = x << 3;
	vert1->y = y + scaledHeight << 3;
	vert1->z = 8192;
	vert1->s = 0;

	vert2->x = x + scaledHeight << 3;
	vert2->y = y + scaledHeight << 3;
	vert2->z = 8192;
	vert2->s = 0x800000;

	vert3->x = x << 3;
	vert3->y = y << 3;
	vert3->z = 8192;
	vert3->s = 0;

	if (vert1->x < app->tinyGL->viewportClampX1) {
		vert1->s += (app->tinyGL->viewportClampX1 - vert1->x) * ((vert2->s - vert1->s) / (vert2->x - vert1->x));
		vert1->x = app->tinyGL->viewportClampX1;
	}

	if (vert2->x > app->tinyGL->viewportClampX2) {
		vert2->s -= (vert2->x - app->tinyGL->viewportClampX2) * ((vert2->s - vert1->s) / (vert2->x - vert1->x));
		vert2->x = app->tinyGL->viewportClampX2;
	}

	this->setupPalette(app->tinyGL->getFogPalette(0x40000000), renderMode, renderFlags);

	tinyGL->mv[0].x = tinyGL->viewX - ((int)(5 * ((tinyGL->view[2] & 0xFFFFFFE0) + (8 * (tinyGL->view[2] >> 5)))) >> 8);
	tinyGL->mv[0].y = tinyGL->viewY - ((int)(5 * ((tinyGL->view[6] & 0xFFFFFFE0) + (8 * (tinyGL->view[6] >> 5)))) >> 8);
	tinyGL->mv[0].z =
	    tinyGL->viewZ - ((int)(5 * ((tinyGL->view[10] & 0xFFFFFFE0) + (8 * (tinyGL->view[10] >> 5)))) >> 8);

	int projX = ((x - tinyGL->viewportWidth / 2) << 15) / tinyGL->viewportWidth;
	int projY = (((y + scaledHeight) - tinyGL->viewportHeight / 2) << 15) / tinyGL->viewportWidth;
	int projSize = (scaledHeight << 15) / tinyGL->viewportWidth;

	int view0 = (tinyGL->view[0] >> 5);
	int view1 = (tinyGL->view[1] >> 5);
	int view4 = (tinyGL->view[4] >> 5);
	int view5 = (tinyGL->view[5] >> 5);
	int view8 = (tinyGL->view[8] >> 5);
	int view9 = (tinyGL->view[9] >> 5);

	tinyGL->mv[0].x += (((projY * view1) + (projX * view0)) >> 14);
	tinyGL->mv[0].y += (((projY * view5) + (projX * view4)) >> 14);
	tinyGL->mv[0].z += (((projY * view9) + (projX * view8)) >> 14);

	tinyGL->mv[1].x = tinyGL->mv[0].x + ((projSize * view0) >> 14);
	tinyGL->mv[1].y = tinyGL->mv[0].y + ((projSize * view4) >> 14);
	tinyGL->mv[1].z = tinyGL->mv[0].z + ((projSize * view8) >> 14);

	tinyGL->mv[2].x = tinyGL->mv[1].x - ((projSize * view1) >> 14);
	tinyGL->mv[2].y = tinyGL->mv[1].y - ((projSize * view5) >> 14);
	tinyGL->mv[2].z = tinyGL->mv[1].z - ((projSize * view9) >> 14);

	this->_gles->SetGLState();
	this->_gles->DrawWorldSpaceSpriteLine(&tinyGL->mv[0], &tinyGL->mv[1], &tinyGL->mv[2], flags ^ 0x20000);
	this->_gles->ResetGLState();
}

void Render::renderSprite(int x, int y, int z, int tileNum, int frame, int flags, int renderMode, int scaleFactor,
                          int renderFlags) {
	this->renderSprite(x, y, z, tileNum, frame, flags, renderMode, scaleFactor, renderFlags, -1);
}

void Render::renderSprite(int x, int y, int z, int tileNum, int frame, int flags, int renderMode, int scaleFactor,
                          int renderFlags, int palIndex) {

	int n10 = scaleFactor;

	if ((flags & Enums::SPRITE_FLAG_DOORLERP) != 0x0) {
		scaleFactor = 65536;
	}
	if (scaleFactor == 0) {
		return;
	}

	const SpriteRenderProps* spriteProps = SpriteDefs::getRenderProps(tileNum);
	if (spriteProps && spriteProps->multiplyShift) {
		renderFlags |= Render::RENDER_FLAG_MULTYPLYSHIFT;
	}

	this->setupTexture(tileNum, frame, renderMode, renderFlags);
	int n11 = app->tinyGL->imageBounds[1] - app->tinyGL->imageBounds[0];
	int n12 = app->tinyGL->imageBounds[3] - app->tinyGL->imageBounds[2];
	int n13 = (app->tinyGL->imageBounds[0] << 10) / app->tinyGL->sWidth;
	int n14 = (n11 << 10) / app->tinyGL->sWidth;
	int n15 = ((app->tinyGL->tHeight - app->tinyGL->imageBounds[3]) << 10) / app->tinyGL->tHeight;
	int n16 = (n12 << 10) / app->tinyGL->tHeight;
	if ((flags & (Enums::SPRITE_FLAG_TWO_SIDED | Enums::SPRITE_FLAG_DECAL)) != 0x0) {
		app->tinyGL->faceCull = TinyGL::CULL_NONE;
	} else {
		app->tinyGL->faceCull = TinyGL::CULL_CCW;
	}
	if ((flags & (Enums::SPRITE_FLAG_FLAT | Enums::SPRITE_FLAGS_ORIENTED)) == 0x0) {
		z -= 512;
		int n17 = 0;
		int n18 = 4;
		int n19;
		int n20;
		if (0x0 != (flags & Enums::SPRITE_FLAG_TILE) || app->tinyGL->textureBaseSize == app->tinyGL->sWidth * app->tinyGL->tHeight) {
			n19 = ((((n11 >> 2) << 4) + 7) * scaleFactor) / 0x10000;
			n20 = ((((n12 >> 1) << 4) + 7) * scaleFactor) / 0x10000;
		} else {
			n19 = (518 * scaleFactor) / 0x10000;
			n20 = (1036 * scaleFactor) / 0x10000;
			n15 = (n13 = 0);
			n16 = (n14 = 1024);
			n18 = 3;
			n17 = 2;
		}
		n17 += 10;
		x -= n17 * this->viewCos >> 16;
		y += n17 * this->viewSin >> 16;
		if (spriteProps && spriteProps->zOffsetFloor) {
			z += spriteProps->zOffsetFloor;
		}
		for (int i = 0; i < n18; ++i) {
			int n21 = (i & 0x2) >> 1;
			int n22 = (i & 0x1) ^ n21 ^ 0x1;
			TGLVert* tglVert = &app->tinyGL->mv[i];
			tglVert->x = x << 4;
			tglVert->y = y << 4;
			tglVert->z = z - 84;
			tglVert->s = n13 + n22 * n14;
			tglVert->t = n15 + n21 * n16;
			app->tinyGL->viewMtxMove(tglVert, 0, (n22 * 2 - 1) * n19, n21 * n20);
		}

		TGLVert* transform3DVerts = app->tinyGL->transform3DVerts(app->tinyGL->mv, n18);
		if (0x0 != (flags & Enums::SPRITE_FLAG_TILE) || app->tinyGL->textureBaseSize == app->tinyGL->sWidth * app->tinyGL->tHeight) {
			app->tinyGL->ClipQuad(&transform3DVerts[0], &transform3DVerts[1], &transform3DVerts[2],
			                      &transform3DVerts[3]);
		} else {
			TGLVert* tglVert2 = &transform3DVerts[0];
			if (tglVert2->w + tglVert2->z < 0) {
				return;
			}
			if (app->tinyGL->clipLine(transform3DVerts)) {
				app->tinyGL->projectVerts(transform3DVerts, n18);
				if (palIndex >= 0) {
					app->tinyGL->spanPalette = app->tinyGL->paletteBase[palIndex % 16];
					this->setupPalette(app->tinyGL->spanPalette, renderMode, renderFlags);
				} else {
					this->setupPalette(app->tinyGL->getFogPalette(transform3DVerts[0].z << 16), renderMode,
					                   renderFlags);
				}
				this->_gles->DrawWorldSpaceSpriteLine(&app->tinyGL->mv[0], &app->tinyGL->mv[1],
				                                       &app->tinyGL->mv[2], flags);
			}
		}
	} else {

		if (spriteProps && spriteProps->zOffsetWall) {
			z += spriteProps->zOffsetWall;
		}

		int n23 = 0;
		if ((flags & Enums::SPRITE_FLAG_EAST) != 0x0) {
			n23 = 0;
		} else if ((flags & Enums::SPRITE_FLAG_NORTH) != 0x0) {
			n23 = 2;
		} else if ((flags & Enums::SPRITE_FLAG_WEST) != 0x0) {
			n23 = 4;
		} else if ((flags & Enums::SPRITE_FLAG_SOUTH) != 0x0) {
			n23 = 6;
		}
		if ((app->tinyGL->textureBaseSize == app->tinyGL->sWidth * app->tinyGL->tHeight) ||
		    (tileNum == TILE_CLOSED_PORTAL_EYE) || (tileNum == TILE_EYE_PORTAL) ||
		    (tileNum == TILE_PORTAL_SOCKET)) {
			int n24;
			int n25;
			if (app->tinyGL->tHeight == 256 && app->tinyGL->sWidth == 256) {
				n24 = 64;
				n25 = 32;
			} else {
				n24 = n12 >> 1;
				n25 = n11 >> 2;
			}
			int n26 = n24 * scaleFactor / 65536;
			int n27 = n25 * scaleFactor / 65536;
			if ((flags & Enums::SPRITE_FLAG_FLAT) == 0x0) {
				z += app->tinyGL->tHeight - app->tinyGL->imageBounds[3] << 4;
				z -= 16 * (scaleFactor / 2048);
			} else {
				z -= 512;
			}
			const int* viewStepValues = Canvas::viewStepValues;
			int n28 = ((n23 + 4) & 0x7) << 1;
			int n29 = ((n23 + 2) & 0x7) << 1;
			int n30 = n27 << 4;
			int n31 = n26 << 4;
			x <<= 4;
			y <<= 4;
			/*if (tileNum == Enums::TILENUM_GLASS && frame == 0) { // J2ME
			    if ((flags & Enums::SPRITE_FLAGS_HORIZONTAL) != 0x0) {
			        n13 += std::abs(x - this->viewX >> 2);
			        n15 -= std::abs(y - this->viewY >> 3);
			    }
			    else {
			        n13 += std::abs(y - this->viewY >> 2);
			        n15 -= std::abs(x - this->viewX >> 3);
			    }
			}*/
			if ((flags & Enums::SPRITE_FLAG_FLIP_HORIZONTAL) != 0x0) {
				n13 += n14;
				n14 = -n14;
			}
			if ((flags & Enums::SPRITE_FLAG_FLIP_VERTICAL) != 0x0) {
				n15 += n16;
				n16 = -n16;
			}

			int n32 = n31;
			if ((flags & Enums::SPRITE_FLAG_DOORLERP) != 0x0) {
				if (tileNum >= TILE_RED_DOOR_LOCKED && tileNum <= TILE_BLUE_DOOR_UNLOCKED) {
					int n33 = n16;
					n31 = (n10 * n31) / 65536;
					n16 = (n10 * n16) / 65536;
					n15 += n33 - n16 >> 1;
				} else {
					int n34 = n14;
					n30 = (n10 * n30) / 65536;
					n14 = (n10 * n14) / 65536;
					n13 += n34 - n14;
				}
			}

			if (tileNum >= TILE_RED_DOOR_LOCKED &&
			    tileNum <= TILE_BLUE_DOOR_UNLOCKED) { // Slip Door
				int n35 = n16 >> 1;
				int n36 = n31 >> 1;
				int n37 = n32 >> 1;
				for (int j = 0; j < 2; ++j) {
					for (int k = 0; k < 4; ++k) {
						int n38 = (k & 0x2) >> 1;
						int n39 = (k & 0x1) ^ n38 ^ 0x1;
						int n40 = (n39 * 2 - 1) * n30;
						int n41 = (n38 * 2 - 1) * n36;
						TGLVert* tglVert3 = &app->tinyGL->mv[k];
						tglVert3->x = x + (viewStepValues[n29 + 0] >> 6) * n40;
						tglVert3->y = y + (viewStepValues[n29 + 1] >> 6) * n40;
						tglVert3->z = z + n38 * n41 + j * (n37 - n36 << 1);
						tglVert3->s = n13 + n39 * n14;
						tglVert3->t = n15 + n38 * n35;
					}
					z += n36;
					n15 += n35;
					app->tinyGL->swapXY = true;
					app->tinyGL->drawModelVerts(app->tinyGL->mv, 4);
				}
			} else if ((flags & Enums::SPRITE_FLAG_FLAT) == 0x0) { // Wall
				for (int l = 0; l < 4; ++l) {
					int n42 = (l & 0x2) >> 1;
					int n43 = (l & 0x1) ^ n42 ^ 0x1;
					TGLVert* tglVert4 = &app->tinyGL->mv[l];
					int n44 = (n43 * 2 - 1) * n30;
					tglVert4->x = x + (viewStepValues[n29 + 0] >> 6) * n44;
					tglVert4->y = y + (viewStepValues[n29 + 1] >> 6) * n44;
					tglVert4->z = z + n42 * n31;
					tglVert4->s = n13 + n43 * n14;
					tglVert4->t = n15 + n42 * n16;
				}
				app->tinyGL->swapXY = true;
				app->tinyGL->drawModelVerts(app->tinyGL->mv, 4);
			} else { // Plane
				for (int n45 = 0; n45 < 4; n45++) {
					int n46 = (n45 & 0x2) >> 1;
					int n47 = (n45 & 0x1) ^ n46 ^ 0x1;
					TGLVert* tglVert5 = &app->tinyGL->mv[n45];
					int n48 = (n47 * 2 - 1) * n30;
					int n49 = (n46 * 2 - 1) * n31 >> 1;
					tglVert5->x = x + (viewStepValues[n29 + 0] >> 6) * n48 + (viewStepValues[n28 + 0] >> 6) * n49;
					tglVert5->y = y + (viewStepValues[n29 + 1] >> 6) * n48 + (viewStepValues[n28 + 1] >> 6) * n49;
					tglVert5->z = z;
					tglVert5->s = n13 + n47 * n14;
					tglVert5->t = n15 + n46 * n16;
					if (spriteProps && spriteProps->texAnim.hasAnim()) {
						if (spriteProps->texAnim.sDivisor)
							tglVert5->s += (app->time / spriteProps->texAnim.sDivisor & spriteProps->texAnim.mask);
						if (spriteProps->texAnim.tDivisor)
							tglVert5->t += (app->time / spriteProps->texAnim.tDivisor & spriteProps->texAnim.mask);
					}
				}
				app->tinyGL->swapXY = false;
				app->tinyGL->drawModelVerts(app->tinyGL->mv, 4);
			}
		}
	}
}

void Render::occludeSpriteLine(int n) {


	int n2 = this->mapSpriteInfo[n];
	if (n2 < 0) {
		return;
	}

	int x = this->mapSprites[this->S_X + n] << 4;
	int y = this->mapSprites[this->S_Y + n] << 4;

	app->tinyGL->mv[0].x = x;
	app->tinyGL->mv[1].x = x;
	app->tinyGL->mv[0].y = y;
	app->tinyGL->mv[1].y = y;
	app->tinyGL->mv[0].z = 0;
	app->tinyGL->mv[1].z = 0;

	int n6 = 0;
	if ((n2 & Enums::SPRITE_FLAG_EAST) != 0x0) {
		n6 = 4;
	} else if ((n2 & Enums::SPRITE_FLAG_NORTH) != 0x0) {
		n6 = 6;
	} else if ((n2 & Enums::SPRITE_FLAG_WEST) != 0x0) {
		n6 = 4;
	} else if ((n2 & Enums::SPRITE_FLAG_SOUTH) != 0x0) {
		n6 = 6;
	}

	int n7 = (n6 + 2 & 0x7) << 1;

	app->tinyGL->mv[0].x += Canvas::viewStepValues[n7 + 0] >> 1 << 4;
	app->tinyGL->mv[1].x -= Canvas::viewStepValues[n7 + 0] >> 1 << 4;
	app->tinyGL->mv[0].y += Canvas::viewStepValues[n7 + 1] >> 1 << 4;
	app->tinyGL->mv[1].y -= Canvas::viewStepValues[n7 + 1] >> 1 << 4;

	TGLVert* transform2DVerts = app->tinyGL->transform2DVerts(app->tinyGL->mv, 2);
	if (app->tinyGL->clipLine(transform2DVerts)) {
		app->tinyGL->projectVerts(transform2DVerts, 2);
		if (transform2DVerts[0].x > transform2DVerts[1].x) {
			app->tinyGL->occludeClippedLine(&transform2DVerts[1], &transform2DVerts[0]);
		} else {
			app->tinyGL->occludeClippedLine(&transform2DVerts[0], &transform2DVerts[1]);
		}
	}
}

#include <iostream>
#include <array>
#include <string>
#include <vector>

// Subdivide Code From http://coliru.stacked-crooked.com/a/b01db8e00f50c872
using Position5 = std::array<float, 5>;
using Plane = std::array<Position5, 4>;

Position5& operator*=(Position5& pos, const float scale) {
	for (auto& x : pos)
		x *= scale;
	return pos;
}

Position5 operator*(const Position5& pos, const float scale) {
	Position5 result = pos;
	result *= scale;
	return result;
}

Position5& operator+=(Position5& left, const Position5& right) {
	for (size_t i = 0; i < 5; i++)
		left[i] += right[i];

	return left;
}

Position5 operator+(const Position5& left, const Position5& right) {
	Position5 result = left;
	result += right;
	return result;
}

Position5& operator-=(Position5& left, const Position5& right) {
	for (size_t i = 0; i < 5; i++)
		left[i] -= right[i];

	return left;
}

Position5 operator-(const Position5& left, const Position5& right) {
	Position5 result = left;
	result -= right;
	return result;
}

/* vertices by index in quad:
    3---------2
    |         |
    |         |
    |         |
    0---------1
*/
std::vector<Plane> subdivide(const Plane& plane, size_t iterations = 1) {
	if (iterations == 0) {
		return {plane};
	} else {
		Position5 e1 = plane[1] - plane[0];
		Position5 e2 = plane[2] - plane[3];
		Position5 e3 = plane[3] - plane[0];
		Position5 e4 = plane[2] - plane[1];

		Position5 p1 = e1 * 0.5f + plane[0];
		Position5 p2 = e2 * 0.5f + plane[3];
		Position5 p3 = e3 * 0.5f + plane[0];
		Position5 p4 = e4 * 0.5f + plane[1];

		Position5 e5 = p2 - p1;
		Position5 p5 = e5 * 0.5f + p1;

		std::vector<Plane> result{
		    {plane[0], p1, p5, p3}, {p1, plane[1], p4, p5}, {p5, p4, plane[2], p2}, {p3, p5, p2, plane[3]}};

		if (iterations == 1) {
			return result;
		} else {
			std::vector<Plane> result2;
			for (auto& x : result) {
				auto subplanes = subdivide(x, iterations - 1);
				result2.insert(result2.end(), subplanes.begin(), subplanes.end());
			}
			return result2;
		}
	}
}

bool Render::renderStreamSpriteGL(TGLVert* array, int n) {

	if (app->render->_gles->isInit) {
		Plane p = {{{(float)array[0].x, (float)array[0].y, (float)array[0].z, (float)array[0].s, (float)array[0].t},
		            {(float)array[1].x, (float)array[1].y, (float)array[1].z, (float)array[1].s, (float)array[1].t},
		            {(float)array[2].x, (float)array[2].y, (float)array[2].z, (float)array[2].s, (float)array[2].t},
		            {(float)array[3].x, (float)array[3].y, (float)array[3].z, (float)array[3].s, (float)array[3].t}}};

		auto subd = subdivide(p, 3);

		TGLVert quad[4];
		int quadCnt = 0;
		for (auto& plane : subd) {
			for (auto& pos : plane) {
				quad[quadCnt].x = (int)pos[0];
				quad[quadCnt].y = (int)pos[1];
				quad[quadCnt].z = (int)pos[2];
				quad[quadCnt].s = (int)pos[3];
				quad[quadCnt].t = (int)pos[4];
				if (++quadCnt >= 4) {
					app->render->_gles->DrawModelVerts(std::span(quad, 4));
					quadCnt = 0;
				}
			}
		}
		return true;
	}
	return false;
}

void Render::renderStreamSprite(int n) {


	TGLVert* mv = app->tinyGL->mv;
	int n2 = this->mapSpriteInfo[n];
	short n3 = this->mapSprites[this->S_ENT + n];
	int n4 = n2 & 0xFF;
	int n5 = (n2 & 0xFF00) >> 8;
	short n6 = this->mapSprites[this->S_RENDERMODE + n];
	int n7 = this->mapSprites[this->S_SCALEFACTOR + n] << 10;
	short n8 = this->mapSprites[this->S_X + n];
	short n9 = this->mapSprites[this->S_Y + n];
	int n10 = this->mapSprites[this->S_Z + n] << 4;
	int n11, n12, n13, n14;
	if (n3 == 1) {
		mv[0].x = mv[1].x = app->canvas->destX << 4;
		mv[0].y = mv[1].y = app->canvas->destY << 4;
		mv[0].z = mv[1].z = app->canvas->destZ << 4;
		mv[2].x = mv[3].x = n8 << 4;
		mv[2].y = mv[3].y = n9 << 4;
		mv[2].z = mv[3].z = n10;

		if (app->canvas->viewPitch <= 10) {
			if (app->canvas->viewPitch < 0) {
				n11 = 128;
				n12 = 224;
			} else {
				n11 = 176;
				n12 = Applet::IOS_HEIGHT;
			}
		} else {
			n11 = 224;
			n12 = 496;
		}

		n13 = -112;
		n14 = 0;
	} else {
		int sprite = app->game->entities[n3].getSprite();
		mv[0].x = mv[1].x = this->mapSprites[this->S_X + sprite] << 4;
		mv[0].y = mv[1].y = this->mapSprites[this->S_Y + sprite] << 4;
		mv[0].z = mv[1].z = this->mapSprites[this->S_Z + sprite] << 4;
		mv[2].x = mv[3].x = n8 << 4;
		mv[2].y = mv[3].y = n9 << 4;
		mv[2].z = mv[3].z = n10;

		n11 = 0;
		n12 = 0;
		n13 = 0;
		n14 = -Applet::IOS_HEIGHT;
	}
	app->tinyGL->faceCull = TinyGL::CULL_NONE;
	this->setupTexture(n4, n5, n6, 0);
	int n27 = app->tinyGL->imageBounds[1] - app->tinyGL->imageBounds[0];
	int n28 = app->tinyGL->imageBounds[3] - app->tinyGL->imageBounds[2];
	int n29 = (app->tinyGL->imageBounds[0] << 10) / app->tinyGL->sWidth;
	int n30 = (n27 << 10) / app->tinyGL->sWidth;
	int n31 = (app->tinyGL->tHeight - app->tinyGL->imageBounds[3] << 10) / app->tinyGL->tHeight;
	int n32 = (n28 << 10) / app->tinyGL->tHeight * 3;
	int n33 = ((n27 / 2) << 4) * n7 / 65536;
	int n34 = ((n27 / 2) << 4) * n7 / 65536;

	if (n33 < 0) {
		n34 += 3;
	}

	int n35 = 0;
	app->tinyGL->viewMtxMove(&mv[0], n12, n11 - (n34 / 4), (n35 / 4) + n13);
	app->tinyGL->viewMtxMove(&mv[1], n12, n11 + (n34 / 4), n13);
	app->tinyGL->viewMtxMove(&mv[2], 0, (n33 * 2), (n35 * 2) + n14);
	app->tinyGL->viewMtxMove(&mv[3], 0, -(n33 * 2), n14);
	int n36 = n31 - (app->time * 3 & 0x3FF);
	mv[3].s = mv[0].s = n29;
	mv[1].t = mv[0].t = n36;
	mv[2].s = mv[1].s = n29 + n30;
	mv[3].t = mv[2].t = n36 + n32;
	app->tinyGL->swapXY = false;

	if (!this->renderStreamSpriteGL(app->tinyGL->mv, 4)) { // [GEC]
		app->tinyGL->drawModelVerts(app->tinyGL->mv, 4);
	}
}

void Render::renderSpriteObject(int n) {

	// printf("renderSpriteObject %d\viewPitch", viewPitch);
	unsigned int n2 = this->mapSpriteInfo[n];
	if ((n2 & Enums::SPRITE_FLAG_HIDDEN) != 0x0) {
		return;
	}

	int n3 = n2 & 0xFF;
	int x = this->mapSprites[this->S_X + n];
	int y = this->mapSprites[this->S_Y + n];
	int z = this->mapSprites[this->S_Z + n] << 4;
	int n7 = (n2 & 0xFF00) >> 8;
	short renderMode = this->mapSprites[this->S_RENDERMODE + n];
	int scaleFactor = this->mapSprites[this->S_SCALEFACTOR + n] << 10;
	int n10 = 0;

	if ((this->mapSpriteInfo[n] & Enums::SPRITE_FLAG_TILE) != 0x0) {
		n3 += 257;
	}

	const SpriteRenderProps* spriteProps = SpriteDefs::getRenderProps(n3);

	if (spriteProps && spriteProps->skip) {
		return;
	}

	if (this->useMastermindHack) {
		if (n3 == 57) {
			if (this->delayedSpriteBuffer[0] == -1) {
				this->delayedSpriteBuffer[0] = n;
				return;
			}
			this->delayedSpriteBuffer[0] = -1;
		} else if ((n3 >= 239 && n3 <= 247) || (n3 >= 225 && n3 <= 231)) {
			if (this->delayedSpriteBuffer[1] == -1) {
				this->delayedSpriteBuffer[1] = n;
				return;
			}
			this->delayedSpriteBuffer[1] = -1;
		}
	}

	if (this->useCaldexHack && n3 == 69) {
		if (this->delayedSpriteBuffer[2] == -1) {
			this->delayedSpriteBuffer[2] = n;
			return;
		}
		this->delayedSpriteBuffer[2] = -1;
	}

	if ((n2 & Enums::SPRITE_FLAG_AUTO_ANIMATE) != 0x0) {
		n7 = (n + app->time / 100) % n7;
	}

	if (spriteProps && spriteProps->renderPath == "stream") {
		this->renderStreamSprite(n);
		return;
	}

	if (spriteProps && spriteProps->positionAtView && x == app->canvas->viewX && y == app->canvas->viewY) {
		x += (app->canvas->viewStepX >> 6) * spriteProps->viewOffsetMult;
		y += (app->canvas->viewStepY >> 6) * spriteProps->viewOffsetMult;
		z += spriteProps->viewZOffset;
		if (this->mapSprites[this->S_SCALEFACTOR + n] != 64) {
			scaleFactor = 65536;
		}
	}

	Entity* entity;
	EntityMonster* monster;
	EntityDef* def;
	if (this->mapSprites[this->S_ENT + n] != -1) {
		entity = &app->game->entities[this->mapSprites[this->S_ENT + n]];
		monster = entity->monster;
		def = entity->def;
	} else {
		entity = nullptr;
		monster = nullptr;
		def = nullptr;
	}

	if (entity != nullptr) {
		if (monster != nullptr) {
			if (!this->disableRenderActivate && app->game->activePropogators == 0 && app->game->animatingEffects == 0 &&
			    !(entity->info & 0x1050000) && !(entity->monsterFlags & Enums::MFLAG_NOACTIVATE) && !app->player->noclip &&
			    !app->game->disableAI) {
				app->game->trace(app->canvas->viewX, app->canvas->viewY, app->canvas->viewZ, x, y, z >> 4,
				                 app->player->getPlayerEnt(), 5293, 2, true);
				if (app->game->traceEntity == entity) {
					app->game->activate(entity, true, true, true, false);
				}
			}
			int monsterEffects = entity->monsterEffects;
			int n11 = n7 & 0xF0;

			if (0x0 != (monsterEffects & 0x2)) {
				n10 |= 0x20; // RENDER_FLAG_BLUESHIFT
				renderMode = 0;
			} else if (0x0 != (monsterEffects & 0x8)) {
				n10 |= 0x80; // RENDER_FLAG_REDSHIFT
				renderMode = 0;
			} else if (0x0 != (monsterEffects & 0x1)) {
				n10 |= 0x40; // RENDER_FLAG_GREENSHIFT
				renderMode = 0;
			}
			if ((n10 & 0x4) == 0x0) { // RENDER_FLAG_GREYSHIFT
				if ((entity->monsterFlags & 0x1000) == 0x0 && (n11 == 96 || n11 == 144) && app->time > entity->ai->frameTime) {
					entity->ai->frameTime = 0;
					n7 = 0;
				}
				this->mapSpriteInfo[n] = ((n2 & 0xFFFF00FF) | n7 << 8);
			}
		}
		if (entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && entity->def->eSubType == Enums::INTERACT_CRATE && entity->param != 0) {
			if (app->time > entity->param) {
				++n7;
				entity->param = app->time + 200;
			}
			if (n7 > 3) {
				entity->param = 0;
				n7 = 3;
			}
			this->mapSpriteInfo[n] = ((n2 & 0xFFFF00FF) | n7 << 8);
		}
	}

	if ((n2 & Enums::SPRITE_FLAG_TILE) != 0x0) {
		this->renderSprite(x, y, z, n3, n7, n2, renderMode, scaleFactor, n10);
	} else {
		if (monster != nullptr) {
			this->renderSpriteAnim(n, n7, x, y, z, n3, n2, renderMode, scaleFactor, n10);
			return;
		}
		if (n3 == TILE_EYE_PORTAL) {
			this->portalInView = true;
			if (this->checkPortalVisibility(x, y, z)) {
				n7 = 1;
			} else {
				n7 = 0;
				int n12 = app->time / 1536 & 0x3;
				n2 = (n2 ^ (n12 & 0x1) << 17 ^ (n12 & 0x2) >> 1 << 18);
			}
			n2 &= 0xFFF7FFFF;
			if ((n2 & Enums::SPRITE_FLAGS_ORIENTED) == 0x0) {
				z += 288;
			}
		} else if (spriteProps && spriteProps->glow.hasGlow()) {
			int zheight = ((spriteProps->glow.zMult * scaleFactor) / 65536) << 4;
			n2 ^= (n7 & 0x1) << 17;
			this->renderSprite(x, y, z + zheight, spriteProps->glow.sprite, 0, n2, spriteProps->glow.renderMode,
			                   scaleFactor, n10);
		} else {
			if (spriteProps && spriteProps->autoAnimPeriod > 0) {
				int animFrame = app->time / spriteProps->autoAnimPeriod;
				this->renderSprite(x, y, z, n3, (animFrame % spriteProps->autoAnimFrames), n2, renderMode, scaleFactor, n10);
				return;
			}
			if (spriteProps && !spriteProps->composite.empty()) {
				for (const auto& layer : spriteProps->composite) {
					int zheight = ((layer.zMult * scaleFactor) / 65536) << 4;
					this->renderSprite(x, y, z + zheight, layer.sprite, n7, n2, renderMode, scaleFactor, n10);
				}
				return;
			}
			if (def != nullptr && def->eType == Enums::ET_NPC) {
				this->renderSpriteAnim(n, n7, x, y, z, n3, n2, renderMode, scaleFactor, n10);
				if (entity->param != 0) {
					int n17 = 160;
					int n18 = 254;
					if (entity->param == 2) {
						n18 = 255;
					}
					this->renderSprite(x, y, z + n17, n18, 0, this->mapSpriteInfo[n] & ~Enums::SPRITE_FLAG_FLIP_HORIZONTAL, 0, scaleFactor, n10);
				}
				return;
			}
			if (n3 == TILE_PRACTICE_TARGET) {
				int n19 = n2 & ~Enums::SPRITE_FLAG_FLIP_HORIZONTAL;

				if (app->canvas->legShotTime) {
					if (app->canvas->legShotTime > app->time) {
						this->renderSprite(x, y, z, 149, 0, n19, renderMode, scaleFactor, n10);
					} else {
						app->canvas->legShotTime = 0;
					}
				} else {
					this->renderSprite(x, y, z, 149, 5, n19, renderMode, scaleFactor, n10);
				}

				if (app->canvas->bodyShotTime) {
					if (app->canvas->bodyShotTime > app->time) {
						this->renderSprite(x, y, z, 149, 2, n19, renderMode, scaleFactor, n10);
					} else {
						app->canvas->bodyShotTime = 0;
					}
				} else {
					this->renderSprite(x, y, z, 149, 6, n19, renderMode, scaleFactor, n10);
				}

				if (app->canvas->headShotTime) {
					if (app->canvas->headShotTime > app->time) {
						this->renderSprite(x, y, z, 149, 3, n19, renderMode, scaleFactor, n10);
					} else {
						app->canvas->headShotTime = 0;
					}
				} else {
					this->renderSprite(x, y, z, 149, 7, n19, renderMode, scaleFactor, n10);
				}

				this->renderSprite(x, y, z, n3, app->canvas->isZoomedIn ? 1 : 4, n19, renderMode, scaleFactor, n10);
				return;
			}
			if (spriteProps && spriteProps->lootAura && entity != nullptr) {
				bool showAura = false;
				if (entity->loot != nullptr && entity->param == 0 && !entity->hasEmptyLootSet()) {
					showAura = true;
				} else if (entity->isDroppedEntity() && entity->param == 0) {
					showAura = true;
				}
				if (showAura && app->canvas->state != Canvas::ST_CAMERA) {
					this->renderSprite(x, y, z, n3, n7, n2, 0,
						(spriteProps->lootAuraScale * scaleFactor) >> 4, spriteProps->lootAuraFlags);
				}
			}
		}
		this->renderSprite(x, y, z, n3, n7, n2, renderMode, scaleFactor, n10);
	}
}

void Render::renderFearEyes(Entity* entity, int frame, int x, int y, int z, int scaleFactor, bool flipH) {


	int anim = frame & Enums::MANIM_MASK;
	uint8_t eSubType = entity->def->eSubType;
	frame &= Enums::MFRAME_MASK;

	if ((anim != 0 && anim != 32) || entity->ai == nullptr ||
	    app->combat->monsterBehaviors[entity->def->monsterIdx].fearImmune || entity->ai->goalType != 4) {
		return;
	}

	const EntityDef::FearEyeData& eyes = entity->def->fearEyes;

	int eyeZ = 16;
	if (anim == 0) {
		int n10 = app->time + entity->getSprite() * 1337;
		int n11;
		if (entity->def->hasRenderFlag(EntityDef::RFLAG_FLOATER)) {
			n11 = (n10 / 512 & 0x1) << 4;
		} else {
			n11 = (n10 / 1024 & 0x1) * 26;
		}
		if ((entity->info & 0x20000000) != 0x0) {
			n11 = 0;
		}
		eyeZ += n11;
	}

	// Per-monster Z adjustments from data
	eyeZ += eyes.zAlwaysPre;
	if (anim == 0) {
		eyeZ += eyes.zIdlePre;
	}
	eyeZ += eyes.zAdd;

	// Eye lateral positions from data
	int eyeL = eyes.eyeL;
	int eyeR = eyes.eyeR;

	// Flip-dependent overrides (e.g. Revenant asymmetry)
	if (flipH && eyes.eyeLFlip != -128) {
		eyeL = eyes.eyeLFlip;
		eyeR = eyes.eyeRFlip;
	}
	if (flipH) {
		int tmp = eyeL;
		eyeL = eyeR;
		eyeR = tmp;
	}
	int scaleEyeL = eyeL * scaleFactor / 65536;
	int scaleEyeR = eyeR * scaleFactor / 65536;
	int scaleEyeZ = eyeZ * scaleFactor / 65536;
	this->renderSprite(x + (scaleEyeL * this->viewRightStepX >> 6), y + (scaleEyeL * this->viewRightStepY >> 6),
	                   z + scaleEyeZ, 251, 0, 0, 0, scaleFactor, 0);
	if (!eyes.singleEye) {
		this->renderSprite(x + (scaleEyeR * this->viewRightStepX >> 6), y + (scaleEyeR * this->viewRightStepY >> 6),
		                   z + scaleEyeZ, 251, 0, 0, 0, scaleFactor, 0);
	}
}

void Render::renderSpriteAnim(int n, int frame, int x, int y, int z, int tileNum, int flags, int renderMode,
                              int scaleFactor, int renderFlags) {


	Entity* entity = &app->game->entities[this->mapSprites[this->S_ENT + n]];

	// Scale clamping from data (Cyberdemon: reduces scale by 7/9 when close up)
	EntityDef* animDef = app->entityDefManager->lookup(tileNum);
	if (animDef && animDef->spriteAnim.clampScale && scaleFactor > 0x10000) {
		int64_t v16 = 7 * scaleFactor;
		int64_t v13 = (uint64_t)(v16 * (int64_t)0x38E38E39) >> 32;
		scaleFactor = (int)((v13 >> 1) - (v16 >> 31));
	}

	if (this->isFloater(tileNum)) {
		this->renderFloaterAnim(n, frame, x, y, z, tileNum, flags, renderMode, scaleFactor, renderFlags);
		this->renderFearEyes(entity, frame, x, y, z, scaleFactor, (flags & Enums::SPRITE_FLAG_FLIP_HORIZONTAL));
		return;
	}
	if (this->isSpecialBoss(tileNum)) {
		this->renderSpecialBossAnim(n, frame, x, y, z, tileNum, flags, renderMode, scaleFactor, renderFlags);
		this->renderFearEyes(entity, frame, x, y, z, scaleFactor, (flags & Enums::SPRITE_FLAG_FLIP_HORIZONTAL));
		return;
	}
	int anim = frame & Enums::MANIM_MASK;
	frame &= Enums::MFRAME_MASK;
	int n12 = 0;
	int n13;
	int min;

	// Look up body part offsets from EntityDef
	EntityDef* bpDef = app->entityDefManager->lookup(tileNum);
	static const EntityDef::BodyPartData defaultBody{};
	const EntityDef::BodyPartData& bp = bpDef ? bpDef->bodyParts : defaultBody;

	if ((entity->isMonster() && (entity->monsterFlags & 0x4000) != 0x0) || this->isNPC(tileNum)) {
		n13 = z;
		min = 0;
	} else {
		n13 = (this->getHeight(x, y) + 32) << 4;
		min = std::min(std::max(z - n13, 0), 256);
	}
	int n14 = scaleFactor * (256 - min) / 256;
	int n15 = 0;
	int n16 = 0;
	int n17 = 0;
	int n18 = 0;
	int n19 = 1;

	// anim = Enums::MANIM_SLAP;
	// frame = (app->time) / 1024 & 0x1;

	switch (anim) {
		case Enums::MANIM_IDLE_BACK: {
			n12 = bp.backViewOffset;
		}
		case Enums::MANIM_IDLE: {
			int n20 = (app->time + n * 1337) / 1024 & 0x1;
			if ((entity->info & 0x20000000) != 0x0) {
				n20 = 0;
			}
			int n21 = n20 * 26;

			// Shadow
			this->renderSprite(x, y, n13, TILE_SHADOW, 0, flags, renderMode, n14, renderFlags);
			// Legs
			this->renderSprite(x, y, z, tileNum, bp.legsFrame + n12, flags, renderMode, scaleFactor, renderFlags);

			if (anim == Enums::MANIM_IDLE) {
				n15 = bp.idleTorsoZ;
			}

			// Torso
			this->renderSprite(x + (n17 * this->viewRightStepX >> 6), y + (n17 * this->viewRightStepY >> 6),
			                   (z + n21) + n15, tileNum, bp.torsoFrame + n12, flags, renderMode, scaleFactor, renderFlags);

			// Head
			if (bp.sentryHeadFlip) {
				flags ^= (((app->time + n * 1337) / 2048) & 0x1) << 17; // Flip Head
				this->renderSprite(x + (n17 * this->viewRightStepX >> 6), y + (n17 * this->viewRightStepY >> 6),
				                   (z + n21) + n16, tileNum, bp.headFrame + n12, flags, renderMode, scaleFactor, renderFlags);
				break;
			}
			if (n19 != 0 && (!bp.noHeadOnBack || anim != Enums::MANIM_IDLE_BACK)) {
				if (anim == Enums::MANIM_IDLE) {
					n16 = bp.idleHeadZ;
				}
				this->renderSprite(x + (n17 * this->viewRightStepX >> 6), y + (n17 * this->viewRightStepY >> 6),
				                   (z + n21) + n16, tileNum, bp.headFrame + n12, flags, renderMode, scaleFactor, renderFlags);
				break;
			}
			break;
		}
		case Enums::MANIM_WALK_BACK: {
			n12 = bp.backViewOffset;
		}
		case Enums::MANIM_WALK_FRONT: {
			int n22 = ((frame & 0x2) >> 1 ^ 0x1) << 17;
			// Shadow
			this->renderSprite(x, y, n13, TILE_SHADOW, 0, flags, renderMode, n14, renderFlags);
			// Legs
			this->renderSprite(x, y, z, tileNum, bp.legsFrame + n12 + (frame & 0x1), flags ^ n22, renderMode, scaleFactor,
			                   renderFlags);
			int n23 = frame & 0x1;
			if ((frame & 0x2) == 0x0) {
				n23 = -n23;
			}
			x += n23 * this->viewRightStepX >> 6;
			y += n23 * this->viewRightStepY >> 6;

			n15 = bp.walkTorsoZ;
			// Torso
			this->renderSprite(x + (n17 * this->viewRightStepX >> 6), y + (n17 * this->viewRightStepY >> 6),
			                   z + n15 + ((frame & 0x1) << 4), tileNum, bp.torsoFrame + n12, bp.flipTorsoWalk ? n22 : flags,
			                   renderMode, scaleFactor, renderFlags);

			// Head
			if (bp.sentryHeadFlip) {
				flags ^= ((app->time + n * 1337) / 1024 & 0x1) << 17; // Flip Head
			}
			if (n19 != 0 && (!bp.noHeadOnBack || anim != Enums::MANIM_WALK_BACK)) {
				n16 = bp.walkHeadZ;
				this->renderSprite(x + (n18 * this->viewRightStepX >> 6), y + (n18 * this->viewRightStepY >> 6),
				                   z + n16 + ((frame & 0x1) << 4), tileNum, bp.headFrame + n12, flags, renderMode, scaleFactor,
				                   renderFlags);
				break;
			}
			break;
		}
		case Enums::MANIM_ATTACK1:
		case Enums::MANIM_ATTACK2: {
			int n24 = bp.legsFrame;
			int n25 = flags;
			if (bp.noHeadOnAttack && frame == 1) { // Zombie-like: flip torso on frame 1
				n25 ^= 0x20000;
			}
			int n26;
			if (anim == Enums::MANIM_ATTACK1) {
				n26 = bp.attack1Frame;
			} else {
				n26 = bp.attack2Frame;
			}
			// Shadow
			this->renderSprite(x, y, n13, TILE_SHADOW, 0, flags, renderMode, n14, renderFlags);
			if (this->isNPC(tileNum)) {
				this->renderSprite(x, y, z, tileNum, n26, flags, renderMode, scaleFactor, renderFlags);
				break;
			}
			if (bp.noHeadOnBack) { // Pinky-type: unique attack path (legs, torso, head/attack)
				int n27 = n26;
				int n28 = bp.torsoFrame;
				this->renderSprite(x, y, z, tileNum, bp.legsFrame, flags ^ 0x20000, renderMode, scaleFactor, renderFlags);
				this->renderSprite(x + (n17 * this->viewRightStepX >> 6), y + ((n17 * this->viewRightStepY) >> 6), z,
				                   tileNum, n28, flags, renderMode, scaleFactor, renderFlags);
				this->renderSprite(x + (n18 * this->viewRightStepX >> 6), y + ((n18 * this->viewRightStepY) >> 6), z,
				                   tileNum, n27 + (frame & 0x1), flags, renderMode, scaleFactor, renderFlags);
				break;
			}

			int n27 = 0;
			if (animDef && animDef->spriteAnim.attackLegOffset != 0) {
				if (!animDef->spriteAnim.attackLegOffsetOnFlip || n25 != 0x20000) {
					n27 = animDef->spriteAnim.attackLegOffset;
				}
			}

			// Legs
			this->renderSprite(x + ((n27 * this->viewRightStepX) >> 6), y + ((n27 * this->viewRightStepY) >> 6), z,
			                   tileNum, n24, n25 ^ 0x20000, renderMode, scaleFactor, renderFlags);

			// Attack torso Z offset from data
			if (bp.attackTorsoZF0 != 0 || bp.attackTorsoZF1 != 0) {
				if (frame == 0) {
					n15 = bp.attackTorsoZF0;
				} else if (frame == 1 && bp.attackTorsoZF1 != 0) {
					n15 = bp.attackTorsoZF1;
				}
				if (bp.attackTorsoZF0 != 0 && bp.noHeadOnAttack) {
					n19 = 0; // ArchVile-type: suppress head when attack torso used
				}
			}
			if (bp.attackRevAtk2TorsoZ != 0 && (anim == Enums::MANIM_ATTACK2)) {
				if (frame == 0) {
					n15 = bp.attackRevAtk2TorsoZ;
				}
			}

			if (frame == 1 &&
			    (!this->hasGunFlare(tileNum) || (bp.attackRevAtk2TorsoZ != 0 && anim == Enums::MANIM_ATTACK2))) {
				++n26;
			}
			// Torso
			this->renderSprite(x + (n17 * this->viewRightStepX >> 6), y + (n17 * this->viewRightStepY >> 6), z + n15,
			                   tileNum, n26, flags, renderMode, scaleFactor, renderFlags);

			bool b = (flags & Enums::SPRITE_FLAG_FLIP_HORIZONTAL) != 0x0;

			// Determine if head should render separately
			bool renderHead = true;
			if (bp.noHeadOnMancAtk) {
				renderHead = false;
			}
			if (bp.attackRevAtk2TorsoZ != 0 && (anim == Enums::MANIM_ATTACK2)) {
				renderHead = false; // Merged torso+head in attack2 (Revenant)
			}

			if (n19 != 0 && !bp.noHeadOnAttack && renderHead) {
				int n29 = bp.headFrame;
				n18 = bp.attackHeadX;
				n16 = bp.attackHeadZ;
				if (animDef && animDef->spriteAnim.hasFrameDependentHead) {
					// Frame-dependent head offsets (ChainsawGoblin)
					n16 = animDef->spriteAnim.headZFrame1;
					if (frame == 0) {
						n18 = animDef->spriteAnim.headXFrame0;
						n16 = animDef->spriteAnim.headZFrame0;
					}
				}
				if (b) {
					n18 = -n18;
				}

				// head
				this->renderSprite(x + (n18 * this->viewRightStepX >> 6), y + (n18 * this->viewRightStepY >> 6),
				                   z + n16, tileNum, n29, flags, renderMode, scaleFactor, renderFlags);
			}
			if (frame == 1 && this->hasGunFlare(tileNum) &&
			    (!this->hasNoFlareAltAttack(tileNum) || anim != Enums::MANIM_ATTACK2)) {
				EntityDef* flareDef = app->entityDefManager->lookup(tileNum);
				static const EntityDef::GunFlareData defaultFlare{};
				const EntityDef::GunFlareData& flare = flareDef ? flareDef->gunFlare : defaultFlare;
				int n30 = 0;
				int n31 = 0;
				int n32 = 0;
				if (flare.dualFlare) {
					// First flash (dual-flare monsters like Mancubus, Revenant)
					int fx = flare.flash1X * this->viewRightStepX >> 6;
					int fy = flare.flash1X * this->viewRightStepY >> 6;
					if ((flags & Enums::SPRITE_FLAG_FLIP_HORIZONTAL) != 0x0) {
						fx = -fx;
						fy = -fy;
					}
					this->renderSprite(x + fx, y + fy, z + flare.flash1Z, tileNum, n26 + 1,
					                   (app->player->totalMoves + app->combat->animLoopCount & 0x3) << 17, 4,
					                   scaleFactor / 3, renderFlags);
				}
				// Second (dual) or single flash position from data
				n30 = flare.flash2X * this->viewRightStepX >> 6;
				n31 = flare.flash2X * this->viewRightStepY >> 6;
				n32 = flare.flash2Z;
				if (b) {
					n30 = -n30;
					n31 = -n31;
				}
				x += n30;
				y += n31;
				z += n32;
				// Flash
				this->renderSprite(x, y, z, tileNum, n26 + 1,
				                   (app->player->totalMoves + app->combat->animLoopCount & 0x3) << 17, 4,
				                   scaleFactor / 3, renderFlags);
				break;
			}
			break;
		}
		case Enums::MANIM_PAIN: {
			this->renderSprite(x, y, n13, TILE_SHADOW, 0, flags, renderMode, n14, renderFlags);
			this->renderSprite(x, y, z + n15, tileNum, bp.painFrame, flags, renderMode, scaleFactor, renderFlags);
			break;
		}
		case Enums::MANIM_SLAP: {
			// Shadow
			this->renderSprite(x, y, n13, TILE_SHADOW, 0, flags, renderMode, n14, renderFlags);
			// Legs
			this->renderSprite(x, y, z, tileNum, bp.legsFrame, flags, renderMode, scaleFactor, renderFlags);
			// Slap torso
			this->renderSprite(x + (n17 * this->viewRightStepX >> 6), y + (n17 * this->viewRightStepY >> 6),
			                   z, tileNum, bp.slapTorsoFrame, flags, renderMode, scaleFactor, renderFlags);
			// Slap head (alternates between frame 15 and 16)
			if (n19 != 0) {
				this->renderSprite(x + (n18 * this->viewRightStepX >> 6), y + (n18 * this->viewRightStepY >> 6),
				                   z + n16, tileNum, bp.slapHeadFrame + (frame & 0x1), flags, renderMode, scaleFactor, renderFlags);
			}
			break;
		}
		case Enums::MANIM_DEAD: {
			if (entity->isMonster() && (entity->monsterFlags & 0x800) == 0x0 && app->canvas->state != 18 &&
			    !entity->hasEmptyLootSet()) {
				this->renderSprite(x, y, z, tileNum, bp.deadFrame, flags, 0, (18 * scaleFactor) >> 4, 512);
			}
			this->renderSprite(x, y, z, tileNum, bp.deadFrame, flags, renderMode, scaleFactor, renderFlags);
			break;
		}
		case Enums::MANIM_NPC_BACK_ACTION: {
			this->renderSprite(x, y, n13, TILE_SHADOW, 0, flags, renderMode, n14, renderFlags);
			this->renderSprite(x, y, z, tileNum, bp.attack1Frame + frame, flags, renderMode, scaleFactor, renderFlags);
			break;
		}
		case Enums::MANIM_NPC_TALK: {
			this->renderSprite(x, y, z, tileNum, frame, flags, renderMode, scaleFactor, renderFlags);
			break;
		}
	}
	this->renderFearEyes(entity, anim, x, y, z, scaleFactor, (flags & Enums::SPRITE_FLAG_FLIP_HORIZONTAL));
}

void Render::renderFloaterAnim(int n, int frame, int x, int y, int z, int tileNum, int flags, int renderMode,
                               int scaleFactor, int renderFlags) {


	// Look up floater data from EntityDef
	EntityDef* def = app->entityDefManager->lookup(tileNum);
	EntityDef::FloaterData fl;
	if (def) {
		fl = def->floater;
	}

	int anim = frame & Enums::MANIM_MASK;
	int n12 = frame;
	frame = 0;
	z += fl.zOffset;
	int n13 = 0;

	if (fl.type == EntityDef::FloaterData::FLOATER_MULTIPART) {
		// Multi-part floater (Sentinel): torso + head as separate sprites
		frame = fl.idleFrontFrame;
		switch (anim) {
			case Enums::MANIM_IDLE_BACK:
			case Enums::MANIM_WALK_BACK: {
				frame = fl.backViewFrame;
			}
			case Enums::MANIM_IDLE:
			case Enums::MANIM_WALK_FRONT:
			case Enums::MANIM_DODGE: {
				int n14 = (app->time + n * 1337) / 512 & 0x1;
				this->renderSprite(x, y, z + (n14 << 4), tileNum, frame, flags, renderMode, scaleFactor,
				                   renderFlags); // Torso
				this->renderSprite(x, y, z + (n14 << 4) + fl.headZOffset, tileNum, ++frame, flags, renderMode,
				                   scaleFactor, renderFlags); // Head
				break;
			}
			case Enums::MANIM_ATTACK2: {
				n13 = fl.attack2Offset;
			}
			case Enums::MANIM_ATTACK1: {
				this->renderSprite(x, y, z, tileNum, fl.attackFrame + n13 + (n12 & 0x1), flags, renderMode, scaleFactor,
				                   renderFlags);
				break;
			}
			case Enums::MANIM_PAIN: {
				this->renderSprite(x, y, z, tileNum, fl.painFrame, flags, renderMode, scaleFactor, renderFlags);
				break;
			}
			case Enums::MANIM_DEAD: {
				if (fl.hasDeadLoot) {
					Entity* entity = &app->game->entities[this->mapSprites[this->S_ENT + n]];
					if (entity->isMonster() && (entity->monsterFlags & 0x800) == 0x0 &&
					    app->canvas->state != 18 && !entity->hasEmptyLootSet()) {
						this->renderSprite(x, y, z, tileNum, fl.deadFrame, flags, 0, 17 * scaleFactor >> 4, 512);
					}
				}
				this->renderSprite(x, y, z, tileNum, fl.deadFrame, flags, renderMode, scaleFactor, renderFlags);
				break;
			}
		}
		return;
	}

	// Simple floater (Cacodemon/LostSoul): single sprite with optional extras
	switch (anim) {
		case Enums::MANIM_IDLE_BACK:
		case Enums::MANIM_WALK_BACK: {
			frame = fl.backViewFrame;
		}
		case Enums::MANIM_IDLE:
		case Enums::MANIM_WALK_FRONT: {
			int n15 = (app->time + n * 1337) / 512 & 0x1;
			if (n15 != 0 && fl.hasIdleFrameIncrement) {
				++frame;
			}
			this->renderSprite(x, y, z + (n15 << 4), tileNum, frame, flags, renderMode, scaleFactor, renderFlags);
			if (anim == Enums::MANIM_WALK_BACK && fl.hasBackExtraSprite) {
				this->renderSprite(x, y, z, tileNum, fl.backExtraSpriteIdx, flags & ~Enums::SPRITE_FLAG_FLIP_HORIZONTAL, renderMode,
				                   scaleFactor, renderFlags);
				break;
			}
			break;
		}
		case Enums::MANIM_ATTACK2: {
			n13 = fl.attack2Offset;
		}
		case Enums::MANIM_ATTACK1: {
			this->renderSprite(x, y, z, tileNum, fl.attackFrame + n13 + (n12 & 0x1), flags, renderMode, scaleFactor, renderFlags);
			break;
		}
		case Enums::MANIM_PAIN: {
			this->renderSprite(x, y, z, tileNum, fl.painFrame, flags, renderMode, scaleFactor, renderFlags);
			break;
		}
		case Enums::MANIM_DEAD: {
			this->renderSprite(x, y, z, tileNum, fl.deadFrame, flags, renderMode, scaleFactor, renderFlags);
			break;
		}
	}
}

void Render::renderSpecialBossAnim(int n, int frame, int x, int y, int z, int tileNum, int flags, int renderMode,
                                   int scaleFactor, int renderFlags) {


	// Look up special boss data from EntityDef
	EntityDef* def = app->entityDefManager->lookup(tileNum);
	EntityDef::SpecialBossData sb;
	if (def) {
		sb = def->specialBoss;
	}

	int anim = frame & Enums::MANIM_MASK;
	frame &= Enums::MFRAME_MASK;

	int n12 = 0;
	int n13 = (app->time + n * 1337) / 1024 & 0x1;
	int n14 = n13 * 26;
	Entity* entity = &app->game->entities[this->mapSprites[this->S_ENT + n]];
	int n15;
	int min;
	if (entity->isMonster() && (entity->monsterFlags & 0x4000) != 0x0) {
		n15 = z;
		min = 32;
	} else {
		n15 = this->getHeight(x, y) + 32 << 4;
		min = std::min(std::max(32 - (z - n15), 0), 32);
	}
	int n16 = 65536 * min / 32;

	if (sb.type == EntityDef::SpecialBossData::BOSS_MULTIPART) {
		// Multi-part boss (Boss Pinky): shadow + legs + torso + head
		if (anim == Enums::MANIM_DEAD) {
			if (entity->isMonster() && (entity->monsterFlags & 0x800) == 0x0 && app->canvas->state != 18 &&
			    !entity->hasEmptyLootSet()) {
				this->renderSprite(x, y, z, tileNum, sb.deadFrame, flags, 0, 17 * scaleFactor >> 4, 512);
			}
			this->renderSprite(x, y, z, tileNum, sb.deadFrame, flags, renderMode, scaleFactor, renderFlags);
			return;
		}
		int headSprite = sb.idleSpriteIdx;
		if (anim == Enums::MANIM_ATTACK1 || anim == Enums::MANIM_ATTACK2) {
			headSprite = sb.attackSpriteIdx;
		} else if (anim == Enums::MANIM_PAIN) {
			headSprite = sb.painSpriteIdx;
		}
		this->renderSprite(x, y, n15, 232, 0, flags, renderMode, n16, renderFlags);         // Shadow
		this->renderSprite(x, y, z, tileNum, 0 + n13, flags, renderMode, scaleFactor, renderFlags); // Legs
		this->renderSprite(x, y, z + n14 + sb.torsoZ, tileNum, 2, flags, renderMode, scaleFactor,
		                   renderFlags); // Torso
		this->renderSprite(x, y, z + n14 + sb.torsoZ, tileNum, headSprite, flags, renderMode, scaleFactor,
		                   renderFlags); // Head

	} else if (sb.type == EntityDef::SpecialBossData::BOSS_ETHEREAL) {
		// Ethereal boss (VIOS): additive multi-sprite rendering
		renderMode = sb.renderModeOverride;
		if (app->game->angryVIOS) {
			renderFlags |= 0x80;
		}
		switch (anim) {
			case Enums::MANIM_IDLE:
			case Enums::MANIM_IDLE_BACK: {
				this->renderSprite(x, y, z, tileNum, 0, flags, renderMode, scaleFactor, renderFlags);
				this->renderSprite(x, y, z + n14, tileNum, 2, flags, renderMode, scaleFactor, renderFlags);
				this->renderSprite(x, y, z + n14, tileNum, 3, flags, renderMode, scaleFactor, renderFlags);
				break;
			}
			case Enums::MANIM_WALK_FRONT:
			case Enums::MANIM_WALK_BACK: {
				this->renderSprite(x, y, z, tileNum, 0 + (frame & 0x1), flags ^ (frame & 0x2) >> 1 << 17, renderMode,
				                   scaleFactor, renderFlags);
				int n18 = frame & 0x1;
				if ((frame & 0x2) == 0x0) {
					n18 = -n18;
				}
				x += n18 * this->viewRightStepX >> 6;
				y += n18 * this->viewRightStepY >> 6;
				this->renderSprite(x, y, z, tileNum, 2, flags, renderMode, scaleFactor, renderFlags);
				this->renderSprite(x, y, z, tileNum, 3, flags, renderMode, scaleFactor, renderFlags);
				break;
			}
			case Enums::MANIM_ATTACK1:
			case Enums::MANIM_ATTACK2: {
				this->renderSprite(x, y, z, tileNum, 0, flags, renderMode, scaleFactor, renderFlags);
				this->renderSprite(x, y, z, tileNum, 8 + (frame & 0x1), flags, renderMode, scaleFactor, renderFlags);
				break;
			}
			case Enums::MANIM_PAIN: {
				renderMode = sb.painRenderMode;
				this->renderSprite(x, y, z, tileNum, 0, flags, renderMode, scaleFactor, renderFlags);
				this->renderSprite(x, y, z, tileNum, 2, flags, renderMode, scaleFactor, renderFlags);
				this->renderSprite(x, y, z, tileNum, 3, flags, renderMode, scaleFactor, renderFlags);
				break;
			}
		}

	} else if (sb.type == EntityDef::SpecialBossData::BOSS_SPIDER) {
		// Spider boss (Mastermind/Arachnotron): shadow + torso + articulated legs
		int legLat = sb.legLateral;
		int legZ = sb.legBaseZ;
		flags &= 0xFFFDFFFF;
		switch (anim) {
			case Enums::MANIM_IDLE:
			case Enums::MANIM_IDLE_BACK: {
				int torsoZ = (sb.idleBobDiv > 1 ? n14 / sb.idleBobDiv : n14) + sb.idleTorsoZBase;
				if (sb.idleBobDiv == 1) {
					torsoZ = n14 + sb.idleTorsoZ;
				}
				this->renderSprite(x, y, n15, 232, 0, flags, renderMode, n16, renderFlags); // Shadow
				this->renderSprite(x, y, z + torsoZ, tileNum, n12 + 3, flags, renderMode, scaleFactor,
				                   renderFlags); // Torso
				break;
			}
			case Enums::MANIM_WALK_FRONT:
			case Enums::MANIM_WALK_BACK: {
				this->renderSprite(x, y, n15, 232, 0, flags, renderMode, n16, renderFlags); // Shadow
				int n21 = 0;
				if (frame == 0) {
					--n21;
				} else if (frame == 2) {
					++n21;
				}

				// Torso
				this->renderSprite(x + (n21 * this->viewRightStepX >> 6), y + (n21 * this->viewRightStepY >> 6),
				                   z + sb.walkTorsoZ, tileNum, n12 + 3, flags, renderMode, scaleFactor, renderFlags);

				// Legs (frame-dependent positioning with alternating flip)
				switch (frame) {
					case 0: {
						this->renderSprite(x + (legLat * this->viewRightStepX >> 6),
						                   y + (legLat * this->viewRightStepY >> 6), z + (legZ - 1 << 4), tileNum,
						                   n12, flags, renderMode, scaleFactor, renderFlags);
						int rLeg = -legLat;
						flags ^= 0x20000;
						this->renderSprite(x + ((rLeg - 1) * this->viewRightStepX >> 6),
						                   y + ((rLeg - 1) * this->viewRightStepY >> 6), z + (legZ << 4), tileNum,
						                   n12 + 2, flags, renderMode, scaleFactor, renderFlags);
						return;
					}
					case 1: {
						this->renderSprite(x + ((legLat + 1) * this->viewRightStepX >> 6),
						                   y + ((legLat + 1) * this->viewRightStepY >> 6), z + (legZ - 1 << 4),
						                   tileNum, n12, flags, renderMode, scaleFactor, renderFlags);
						int rLeg = -legLat;
						flags ^= 0x20000;
						this->renderSprite(x + (rLeg * this->viewRightStepX >> 6),
						                   y + (rLeg * this->viewRightStepY >> 6), z + (legZ << 4), tileNum,
						                   n12 + 1, flags, renderMode, scaleFactor, renderFlags);
						return;
					}
					case 2: {
						this->renderSprite(x + ((legLat + 1) * this->viewRightStepX >> 6),
						                   y + ((legLat + 1) * this->viewRightStepY >> 6), z + (legZ << 4), tileNum,
						                   n12 + 1, flags, renderMode, scaleFactor, renderFlags);
						int rLeg = -legLat;
						flags ^= 0x20000;
						this->renderSprite(x + (rLeg * this->viewRightStepX >> 6),
						                   y + (rLeg * this->viewRightStepY >> 6), z + (legZ - 1 << 4), tileNum,
						                   n12, flags, renderMode, scaleFactor, renderFlags);
						return;
					}
					default: {
						break;
					}
				}
				break;
			}
			case Enums::MANIM_ATTACK1:
			case Enums::MANIM_ATTACK2: {
				this->renderSprite(x, y, n15, 232, 0, flags, renderMode, n16, renderFlags); // Shadow
				this->renderSprite(x, y, z + sb.attackTorsoZ, tileNum, n12 + 3, flags, renderMode, scaleFactor,
				                   renderFlags); // Torso
				this->renderSprite(x, y, z + sb.attackTorsoZ, tileNum, n12 + 8, flags, renderMode, scaleFactor,
				                   renderFlags); // Head Attack
				if (frame == 1 && sb.hasAttackFlare) {
					int n25;
					int n26;
					int pos = sb.flareLateralPos;
					int flareZ = sb.attackTorsoZ + sb.flareTorsoZExtra;
					if ((flags & Enums::SPRITE_FLAG_FLIP_HORIZONTAL) != 0x0) {
						n25 = x - (pos * this->viewRightStepX >> 6);
						n26 = y - (pos * this->viewRightStepY >> 6);
					} else {
						n25 = x + (pos * this->viewRightStepX >> 6);
						n26 = y + (pos * this->viewRightStepY >> 6);
					}
					this->renderSprite(n25, n26, z + sb.flareZOffset + flareZ, tileNum, n12 + 9,
					                   (app->player->totalMoves + app->combat->animLoopCount & 0x3) << 17, 4,
					                   scaleFactor / 3, renderFlags);
					break;
				}
				break;
			}
			case Enums::MANIM_PAIN: {
				this->renderSprite(x, y, n15, 232, 0, flags, renderMode, n16, renderFlags); // Shadow
				this->renderSprite(x, y, z + sb.painTorsoZ, tileNum, n12 + 12, flags, renderMode, scaleFactor,
				                   renderFlags); // Torso
				this->renderSprite(x + ((legLat + 3) * this->viewRightStepX >> 6),
				                   y + ((legLat + 3) * this->viewRightStepY >> 6),
				                   z + (legZ + 4 << 4) + sb.painLegsZ, tileNum, n12 + 1, flags, renderMode,
				                   scaleFactor, renderFlags);
				int rLeg = -legLat;
				flags ^= 0x20000;
				this->renderSprite(x + ((rLeg + sb.painLegPos) * this->viewRightStepX >> 6),
				                   y + ((rLeg + sb.painLegPos) * this->viewRightStepY >> 6), z + (legZ + 3 << 4),
				                   tileNum, n12, flags, renderMode, scaleFactor, renderFlags);
				return;
			}
			case Enums::MANIM_DEAD: {
				if (entity->isMonster() && (entity->monsterFlags & 0x800) == 0x0 && app->canvas->state != 18 &&
				    !entity->hasEmptyLootSet()) {
					this->renderSprite(x, y, z, tileNum, sb.deadFrame, flags, 0, 17 * scaleFactor >> 4, 512);
				}
				this->renderSprite(x, y, z, tileNum, sb.deadFrame, flags, renderMode, scaleFactor, renderFlags);
				return;
			}
		}
		// Default legs (idle fallthrough)
		this->renderSprite(x + (legLat * this->viewRightStepX >> 6), y + (legLat * this->viewRightStepY >> 6),
		                   z + (legZ << 4), tileNum, n12 + 2, flags, renderMode, scaleFactor, renderFlags);
		int rLeg = -legLat;
		flags ^= 0x20000;
		this->renderSprite(x + ((rLeg - 1) * this->viewRightStepX >> 6),
		                   y + ((rLeg - 1) * this->viewRightStepY >> 6), z + (legZ - 1 << 4), tileNum, n12, flags,
		                   renderMode, scaleFactor, renderFlags);
	}
}

void Render::handleMonsterIdleSounds(Entity* entity) {


	if ((this->monsterIdleTime[entity->def->eSubType] != 0) &&
	    (this->monsterIdleTime[entity->def->eSubType] <= app->time)) {
		if (entity->distFrom(app->canvas->viewX, app->canvas->viewY) < app->combat->tileDistances[3]) {
			int MonsterSound =
			    app->game->getMonsterSound(entity->def->monsterIdx, Enums::MSOUND_IDLE);
			app->sound->playSound(MonsterSound, 0, 1, 0);
		}
		this->monsterIdleTime[entity->def->eSubType] = ((app->nextByte() % 10) * 1000) + app->time + 6000;
	}
}

