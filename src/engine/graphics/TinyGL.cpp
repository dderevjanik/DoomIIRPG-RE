#include <span>
#include <cmath>
#include <stdexcept>
#include "Log.h"

#include "SDLGL.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Graphics.h"
#include "GLES.h"
#include "Render.h"
#include "TinyGL.h"


TinyGL::TinyGL() = default;

TinyGL::~TinyGL() {
}

bool TinyGL::startup(int screenWidth) {
	this->app = CAppContainer::getInstance()->app;
	Canvas* canvas = this->app->canvas.get();

	LOG_INFO("TinyGL::startup, w [{}]\n", screenWidth);

	this->screenWidth = screenWidth;
	this->columnScale = new int[screenWidth];

	// setView/setViewport now live on Render (B3.14a).
	Render* r = this->app->render.get();
	r->setViewport(canvas->viewRect[0], canvas->viewRect[1], canvas->viewRect[2], canvas->viewRect[3]);
	r->setView(0, 0, 0, 0, 0, 0, 290, 290);
	return true;
}

void TinyGL::viewMtxMove(TGLVert* tglVert, int n, int n2, int n3) {
	// Move tglVert along the view-basis axes: forward (-n), right (+n2),
	// up (-n3). With float `view`, the legacy `>> 14` Q14 shift collapses
	// into a plain `(int)(view[i] * delta)`.
	if (n != 0) {
		const float fn = (float)(-n);
		tglVert->x += (int)(app->render->view[2]  * fn);
		tglVert->y += (int)(app->render->view[6]  * fn);
		tglVert->z += (int)(app->render->view[10] * fn);
	}
	if (n2 != 0) {
		const float fn = (float)n2;
		tglVert->x += (int)(app->render->view[0] * fn);
		tglVert->y += (int)(app->render->view[4] * fn);
		tglVert->z += (int)(app->render->view[8] * fn);
	}
	if (n3 != 0) {
		const float fn = (float)(-n3);
		tglVert->x += (int)(app->render->view[1] * fn);
		tglVert->y += (int)(app->render->view[5] * fn);
		tglVert->z += (int)(app->render->view[9] * fn);
	}
}

TGLVert* TinyGL::transform3DVerts(TGLVert* array, int n) {
	const float* mvp = app->render->mvp;
	for (int i = 0; i < n; ++i) {
		TGLVert* tglVert = &array[i];
		TGLVert* tglVert2 = &this->cv[i];
		const float x = (float)tglVert->x;
		const float y = (float)tglVert->y;
		const float z = (float)tglVert->z;
		tglVert2->x = (int)(x * mvp[0] + y * mvp[4] + z * mvp[8]  + mvp[12]);
		tglVert2->y = (int)(x * mvp[1] + y * mvp[5] + z * mvp[9]  + mvp[13]);
		tglVert2->z = (int)(x * mvp[2] + y * mvp[6] + z * mvp[10] + mvp[14]);
		tglVert2->w = (int)(x * mvp[3] + y * mvp[7] + z * mvp[11] + mvp[15]);
		tglVert2->s = tglVert->s;
		tglVert2->t = tglVert->t;
	}
	return this->cv;
}

TGLVert* TinyGL::transform2DVerts(TGLVert* array, int n) {
	const float* mvp2D = app->render->mvp2D;
	for (int i = 0; i < n; ++i) {
		TGLVert* tglVert = &array[i];
		TGLVert* tglVert2 = &this->cv[i];
		const float x = (float)tglVert->x;
		const float y = (float)tglVert->y;
		tglVert2->x = (int)(x * mvp2D[0] + y * mvp2D[4] + mvp2D[12]);
		tglVert2->z = (int)(x * mvp2D[2] + y * mvp2D[6] + mvp2D[14]);
		tglVert2->w = (int)(x * mvp2D[3] + y * mvp2D[7] + mvp2D[15]);
		tglVert2->s = tglVert->s;
		tglVert2->t = tglVert->t;
	}
	return this->cv;
}



bool TinyGL::clipLine(TGLVert* array) {
	TGLVert* tglVert = &array[0];
	TGLVert* tglVert2 = &array[1];
	for (int i = 0; i < 3; ++i) {
		int n = 0;
		int n2 = 0;
		switch (i) {
			case 0:
			default: {
				n = tglVert->w + tglVert->x;
				n2 = tglVert2->w + tglVert2->x;
				break;
			}
			case 1: {
				n = tglVert->w - tglVert->x;
				n2 = tglVert2->w - tglVert2->x;
				break;
			}
			case 2: {
				n = tglVert->w + tglVert->z;
				n2 = tglVert2->w + tglVert2->z;
				break;
			}
		}
		int n3 = ((n < 0) ? 1 : 0) | ((n2 < 0) ? 2 : 0);
		if (n3 != 0) {
			if (n3 == 3) {
				return false;
			}
			switch (n3) {
				case 1: {
					int n4 = (n2 << 14) / (n2 - n);
					tglVert->x = tglVert2->x + (((tglVert->x - tglVert2->x) * n4) >> 14);
					tglVert->y = tglVert2->y + (((tglVert->y - tglVert2->y) * n4) >> 14);
					tglVert->z = tglVert2->z + (((tglVert->z - tglVert2->z) * n4) >> 14);
					tglVert->w = tglVert2->w + (((tglVert->w - tglVert2->w) * n4) >> 14);
					tglVert->s = tglVert2->s + (((tglVert->s - tglVert2->s) * n4) >> 14);
					tglVert->t = tglVert2->t + (((tglVert->t - tglVert2->t) * n4) >> 14);
					break;
				}
				case 2: {
					int n5 = (n << 14) / (n - n2);
					tglVert2->x = tglVert->x + (((tglVert2->x - tglVert->x) * n5) >> 14);
					tglVert2->y = tglVert->y + (((tglVert2->y - tglVert->y) * n5) >> 14);
					tglVert2->z = tglVert->z + (((tglVert2->z - tglVert->z) * n5) >> 14);
					tglVert2->w = tglVert->w + (((tglVert2->w - tglVert->w) * n5) >> 14);
					tglVert2->s = tglVert->s + (((tglVert2->s - tglVert->s) * n5) >> 14);
					tglVert2->t = tglVert->t + (((tglVert2->t - tglVert->t) * n5) >> 14);
					break;
				}
			}
		}
	}
	return true;
}


void TinyGL::projectVerts(TGLVert* array, int n) {
	// Viewport scratch lives on Render now (B3.14a).
	const Render* r = app->render.get();
	const int xBias = r->viewportXBias;
	const int yBias = r->viewportYBias;
	const int xScale = r->viewportXScale;
	const int yScale = r->viewportYScale;
	for (int i = 0; i < n; i++) {
		TGLVert* tglVert = &array[i];
		const int w = tglVert->w;
		const int z = 0x7fffff / w;
		tglVert->x = xBias + ((tglVert->x * xScale) / w);
		tglVert->y = yBias + ((tglVert->y * yScale) / w);
		tglVert->z = z;
		tglVert->s *= z;
		tglVert->t *= z;
	}
}


bool TinyGL::clippedLineVisCheck(TGLVert* tglVert, TGLVert* tglVert2, bool b) {
	int begX = tglVert->x + 7 >> 3;
	int endX = tglVert2->x + 7 >> 3;
	if (begX < 0) {
		begX = 0;
	}
	if (endX > this->screenWidth) {
		endX = this->screenWidth;
	}
	if (endX <= begX) {
		return false;
	}
	if (b) {
		int z = tglVert->z;
		for (int n = (tglVert2->z - tglVert->z) / (endX - begX), n2 = z + (n * ((begX << 3) - tglVert->x) >> 3); begX < endX; ++begX, n2 += n) {
			if (0x7fffff / n2 < this->columnScale[begX]) {
				return true;
			}
		}
		return false;
	}
	while (begX < endX) {
		if (this->columnScale[begX] == TinyGL::COLUMN_SCALE_INIT) {
			return true;
		}
		++begX;
	}
	return false;
}

bool TinyGL::occludeClippedLine(TGLVert* tglVert, TGLVert* tglVert2) {
	int begX = tglVert->x + 7 >> 3;
	int endX = tglVert2->x + 7 >> 3;
	if (begX < 0) {
		begX = 0;
	}
	if (endX > this->screenWidth) {
		endX = this->screenWidth;
	}
	if (endX <= begX) {
		return false;
	}
	int z = tglVert->z;
	int n = (tglVert2->z - tglVert->z) / (endX - begX);
	int n2 = z + (n * ((begX << 3) - tglVert->x) >> 3);
	bool b = false;
	while (begX < endX) {
		int n3 = 0x7fffff / n2;
		if (n3 < this->columnScale[begX]) {
			this->columnScale[begX] = n3;
			b = true;
		}
		++begX;
		n2 += n;
	}
	return b;
}

// [GEC]

