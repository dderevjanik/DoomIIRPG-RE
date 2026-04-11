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

bool Render::checkPortalVisibility(int x, int y, int z) {

	int n4 = 64 * this->sinTable[this->viewAngle & 0x3FF] >> 12;
	int n5 = 64 * this->sinTable[this->viewAngle + 256 & 0x3FF] >> 12;
	if (app->player->isFamiliar) {
		this->portalState = Render::PORTAL_DNE;
		return false;
	}
	int n6 = ((app->canvas->viewX >> 6) - (x >> 6)) * ((app->canvas->viewX >> 6) - (x >> 6));
	int n7 = ((app->canvas->viewY >> 6) - (y >> 6)) * ((app->canvas->viewY >> 6) - (y >> 6));
	if (this->closestPortalDist <= n6 + n7) {
		return false;
	}
	this->closestPortalDist = n6 + n7;
	if (n6 + n7 > 15) {
		if (!this->portalScripted) {
			if (this->portalState != Render::PORTAL_DNE) {
				this->portalState = Render::PORTAL_SPINNING_DOWN;
			}
			return false;
		}
		return true;
	} else {
		app->game->trace(app->canvas->viewX, app->canvas->viewY, app->canvas->viewZ, x, y, z >> 4,
		                 app->player->getPlayerEnt(), 5293, 25, true);
		if (app->game->traceFracs[0] != 0x3FFF) {
			this->portalScripted = false;
			this->portalState = Render::PORTAL_DNE;
			return false;
		}
		x <<= 4;
		y <<= 4;
		TGLVert* tglVert = &app->tinyGL->mv[0];
		tglVert->x = x + n4;
		tglVert->y = y + n5;
		tglVert->z = z + 1056;
		TGLVert* tglVert2 = &app->tinyGL->mv[1];
		tglVert2->x = tglVert->x;
		tglVert2->y = tglVert->y;
		tglVert2->z = tglVert->z - (1056 * 2);
		TGLVert* tglVert3 = &app->tinyGL->mv[2];
		tglVert3->x = tglVert->x - (n4 << 1);
		tglVert3->y = tglVert->y - (n5 << 1);
		tglVert3->z = tglVert->z;
		TGLVert* transform3DVerts = app->tinyGL->transform3DVerts(app->tinyGL->mv, 3);
		int tz = transform3DVerts[0].z;
		if (transform3DVerts[0].w + transform3DVerts[0].z < 0 || transform3DVerts[1].w + transform3DVerts[1].z < 0 ||
		    transform3DVerts[2].w + transform3DVerts[2].z < 0) {
			this->portalScripted = false;
			this->portalState = Render::PORTAL_DNE;
			return false;
		}
		app->tinyGL->projectVerts(transform3DVerts, 3);
		this->portalCX = transform3DVerts[0].x + transform3DVerts[2].x >> 4;
		this->portalCY = transform3DVerts[0].y + transform3DVerts[1].y >> 4;
		this->portalScale = std::min(((transform3DVerts[1].x - transform3DVerts[2].x << 7) / 132) >> 1, 2048);

		if (app->render->_gles->isInit) { // [GEC] GL Version
			if (app->tinyGL->viewPitch != 0 && (tz < 3000)) {
				this->portalCY += 10;
			}
			this->portalCY += tz / 300;
		}

		if (this->portalScripted) {
			return this->portalState != Render::PORTAL_DNE;
		}
		if (this->portalState != Render::PORTAL_SPINNING) {
			this->portalState = Render::PORTAL_SPINNING_UP;
		}
		return true;
	}
}

void Render::renderPortal() {
	static float angle = 0.f;


	if (this->portalState == Render::PORTAL_DNE) {
		this->previousPortalState = Render::PORTAL_DNE;
		return;
	}
	if (this->portalState == Render::PORTAL_SPINNING_UP) {
		if (this->previousPortalState != Render::PORTAL_SPINNING_UP) {
			this->portalSpinTime = app->time + 500;
			this->previousPortalState = Render::PORTAL_SPINNING_UP;
		}
	} else if (this->portalState == Render::PORTAL_SPINNING_DOWN &&
	           this->previousPortalState != Render::PORTAL_SPINNING_DOWN) {
		this->portalSpinTime = app->time + 500;
		this->previousPortalState = Render::PORTAL_SPINNING_DOWN;
	}
	if (this->portalState != Render::PORTAL_SPINNING) {
		int n2 = this->portalSpinTime - app->time;
		if (n2 < 0) {
			if (this->portalState == Render::PORTAL_SPINNING_DOWN) {
				this->portalState = Render::PORTAL_DNE;
				return;
			}
			if (this->portalState == Render::PORTAL_SPINNING_UP) {
				this->portalState = Render::PORTAL_SPINNING;
			}
		} else if (this->portalState == Render::PORTAL_SPINNING_UP) {
			this->portalScale = (this->portalScale * (500 - n2)) / 500;
		} else {
			this->portalScale = (this->portalScale * n2) / 500;
		}
	}

	if (app->render->_gles->isInit) { // [GEC] GL Version
		if (!this->imgPortal) {
			this->imgPortal = app->loadImage("portal_image.bmp", true);
		}

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
		glDisable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		angle += 1.0f;

		float pScale = (float)this->portalScale / 600.0f;
		float sinAng = sinf((float)((float)(angle * 5.0f) * 3.14f) / 360.0f);
		float scale = pScale * (float)((float)(sinAng * 0.1f) + 1.0f);

		this->_gles->DrawPortalTexture(
		    this->imgPortal, 0, 0, imgPortal->width, imgPortal->height,
		    (float)((float)this->portalCX + (float)((float)(scale * (float)imgPortal->width) * -0.5f)),
		    (float)((float)this->portalCY + (float)((float)(scale * (float)imgPortal->height) * -0.5f)), scale, angle,
		    0);

		this->_gles->DrawPortalTexture(
		    this->imgPortal, 0, 0, imgPortal->width, imgPortal->height,
		    (float)((float)this->portalCX +
		            (float)((float)((float)((float)(pScale + pScale) - scale) * (float)imgPortal->width) * -0.5f)),
		    (float)((float)this->portalCY +
		            (float)((float)((float)((float)(pScale + pScale) - scale) * (float)imgPortal->height) * -0.5f)),
		    (float)(pScale + pScale) - scale, -angle, 1);
	} else { // TnyGL Version restored from J2ME/BREW
		int n = 10;
		int max = std::max(Render::portalMaxRadius * this->portalScale >> n, 1);
		int portalCX = this->portalCX;
		int portalCY = this->portalCY;
		int n3 = (Render::portalMaxRadius - 14) * this->portalScale >> n;
		int n4 = 11 * this->portalScale >> n;
		int n5 = 9 * this->portalScale >> n;
		int n6 = 6 * this->portalScale >> n;
		int max2 = std::max(((8 * this->portalScale) >> n), 1);
		int n7 =
		    100 *
		    (16 /
		     max2); // Old (8 / max2); // [GEC] Prevents it from spinning too fast / Evita que gire demasiado rapido
		int n8 = this->currentPortalMod >> 1;
		for (int i = portalCY - n3 - n8; i < portalCY + n3 + n8; ++i) {
			for (int j = portalCX - n3 - n8; j < portalCX + n3 + n8; ++j) {
				if ((portalCX - j) * (portalCX - j) + (portalCY - i) * (portalCY - i) <= (n3 + n8) * (n3 + n8)) {
					if (j >= 1 && j < screenWidth - 1 && i >= 1 && i < screenHeight - 1) {
						app->canvas->graphics.drawPixelPortal(app->canvas->viewRect, j, i, 0x6f000000);
					}
				}
			}
		}
		for (int k = -max - this->currentPortalMod; k < max + this->currentPortalMod; ++k) {
			if (k <= -n4 || k >= n4) {
				int n11 = (max >> 1) * this->sinTable[(k - max << 16) / max * 512 >> 16 & 0x3FF] >> 16;
				int n12 = (max - std::abs(k) << 16) / max;
				int n13;
				if (k < -max || k > max) {
					n13 = 0xAFB06E00;
				} else {
					n13 = ((160 * (65536 - n12) >> 16) + 15 << 24 | 0xB00000 | 55 * n12 >> 16 << 8);
				}
				for (int l = 0; l < 8; ++l) {
					int n14 = l * 512 / 8 - this->currentPortalTheta;
					int n15 = this->sinTable[n14 & 0x3FF];
					int n16 = this->sinTable[n14 + 256 & 0x3FF];
					int n17 = (k * n16 - n11 * n15 >> 16) + portalCX;
					int n18 = (k * n15 + n11 * n16 >> 16) + portalCY;
					int n19 = (n13 >> 24 & 0xFF) << 16;
					if (n17 >= 1 && n17 < screenWidth - 1 && n18 >= 1 && n18 < screenHeight - 1) {
						app->canvas->graphics.drawPixelPortal(app->canvas->viewRect, n17, n18, n13);
					}
					for (int n22 = 0; n22 < n6; ++n22) {
						int n23 = n17 + app->nextByte() % n5 - (n5 >> 1);
						int n24 = n18 + app->nextByte() % n5 - (n5 >> 1);
						if (n23 >= 1 && n23 < screenWidth - 1 && n24 >= 1 && n24 < screenHeight - 1) {
							app->canvas->graphics.drawPixelPortal(app->canvas->viewRect, n23, n24, n13);
						}
					}
				}
			}
		}
		if (app->time > this->nextPortalFrame) {
			this->currentPortalTheta = (this->currentPortalTheta + 20 & 0x3FF);
			if (this->portalModIncreasing) {
				if (this->currentPortalMod >= max2) {
					this->portalModIncreasing = false;
					this->currentPortalMod = max2 - 1;
				} else {
					++this->currentPortalMod;
				}
			} else if (this->currentPortalMod <= 0) {
				this->portalModIncreasing = true;
				++this->currentPortalMod;
			} else {
				--this->currentPortalMod;
			}
			this->nextPortalFrame = app->time + n7;
		}
	}
}

