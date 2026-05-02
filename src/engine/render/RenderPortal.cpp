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
		TGLVert* transform3DVerts = app->render->transform3DVerts(app->tinyGL->mv, 3);
		int tz = transform3DVerts[0].z;
		if (transform3DVerts[0].w + transform3DVerts[0].z < 0 || transform3DVerts[1].w + transform3DVerts[1].z < 0 ||
		    transform3DVerts[2].w + transform3DVerts[2].z < 0) {
			this->portalScripted = false;
			this->portalState = Render::PORTAL_DNE;
			return false;
		}
		app->render->projectVerts(transform3DVerts, 3);
		this->portalCX = transform3DVerts[0].x + transform3DVerts[2].x >> 4;
		this->portalCY = transform3DVerts[0].y + transform3DVerts[1].y >> 4;
		this->portalScale = std::min(((transform3DVerts[1].x - transform3DVerts[2].x << 7) / 132) >> 1, 2048);

		if (app->render->_gles->isInit) { // [GEC] GL Version
			if (app->render->viewPitch != 0 && (tz < 3000)) {
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
	if (this->headless) { return; }

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

	if (!this->imgPortal) {
		this->imgPortal = app->loadImage("portal_image.bmp", true);
	}

	// Blend state for the portal effect. Color is hardcoded inside
	// DrawPortalTexture (only consumer of this state); the legacy
	// glColor4f(1,1,1,0.25) and glTexEnvi(MODULATE) calls are gone.
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
}

