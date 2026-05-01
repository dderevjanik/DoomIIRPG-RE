#include <span>
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

bool TinyGL::startup(int screenWidth, int screenHeight) {
	this->app = CAppContainer::getInstance()->app;
	Canvas* canvas = this->app->canvas.get();

	LOG_INFO("TinyGL::startup, w [{}], h [{}]\n", screenWidth, screenHeight);

	this->screenWidth = screenWidth;
	this->screenHeight = screenHeight;
	this->columnScale = new int[screenWidth];

	this->setViewport(canvas->viewRect[0], canvas->viewRect[1], canvas->viewRect[2], canvas->viewRect[3]);
	this->setView( 0, 0, 0, 0, 0, 0, 290, 290);
	this->fogMin = 32752;
	this->fogRange = 1;
	this->fogColor = 0;
	this->countDrawn = 0;
	this->spanPixels = 0;
	return true;
}

void TinyGL::buildViewMatrix(int x, int y, int z, int yaw, int pitch, int roll, int* matrix) {

	int *sinTable = app->render->sinTable;

	int cosYaw = sinTable[yaw + 512 & 0x3ff] >> 2;
	int sinYaw = sinTable[yaw + 256 & 0x3ff] >> 2;
	int cosPitch = sinTable[pitch + 512 & 0x3ff] >> 2;
	int sinPitch = sinTable[pitch + 256 & 0x3ff] >> 2;
	int sinRoll = sinTable[roll & 0x3ff] >> 2;
	int cosRoll = sinTable[roll + 256 & 0x3ff] >> 2;
	int n13 = sinRoll * cosPitch >> 14;
	int n14 = cosRoll * cosPitch >> 14;
	matrix[0] = n13 * sinYaw + cosRoll * -cosYaw >> 14;
	matrix[4] = n13 * cosYaw + cosRoll * sinYaw >> 14;
	matrix[8] = sinRoll * sinPitch >> 14;
	matrix[1] = -(n14 * sinYaw + -sinRoll * -cosYaw) >> 14;
	matrix[5] = -(n14 * cosYaw + -sinRoll * sinYaw) >> 14;
	matrix[9] = -(cosRoll * sinPitch) >> 14;
	matrix[2] = -(sinPitch * sinYaw) >> 14;
	matrix[6] = -(sinPitch * cosYaw) >> 14;
	matrix[10] = -(-cosPitch);

	for (int i = 0; i < 3; ++i) {
		matrix[12 + i] = -(x * matrix[0 + i] + y * matrix[4 + i] + z * matrix[8 + i]) >> 14;
	}

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 16384;
}

void TinyGL::buildProjectionMatrix(int fov, int aspect, int* matrix) {

	int* sinTable = app->render->sinTable;

	int n3 = aspect >> 1;
	int n4 = (fov << 14) / aspect;
	int n5 = sinTable[n3 & 0x3FF] >> 2;
	int n6 = sinTable[n3 + 256 & 0x3FF] >> 2;
	matrix[0] = (n6 << 14) / (n4 * n5 >> 14);
	matrix[8] = (matrix[4] = 0);
	matrix[1] = (matrix[12] = 0);
	matrix[5] = (n6 << 14) / n5;
	matrix[13] = (matrix[9] = 0);
	matrix[6] = (matrix[2] = 0);
	matrix[10] = -TinyGL::MATRIX_ONE;
	matrix[14] = -(2 * TinyGL::NEAR_CLIP);
	matrix[7] = (matrix[3] = 0);
	matrix[11] = -TinyGL::MATRIX_ONE;
	matrix[15] = 0;
}

void TinyGL::multMatrix(int* matrix1, int* matrix2, int* destMtx) {
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			destMtx[i * 4 + j] = (matrix1[i * 4 + 0] * matrix2[0 + j] + matrix1[i * 4 + 1] * matrix2[4 + j] + matrix1[i * 4 + 2] * matrix2[8 + j] + matrix1[i * 4 + 3] * matrix2[12 + j]) >> 14;
		}
	}
}

void TinyGL::_setViewport(int viewportX, int viewportY, int viewportWidth, int viewportHeight)
{
	this->viewportX = viewportX;
	this->viewportY = viewportY;
	this->viewportWidth = viewportWidth;
	this->viewportHeight = viewportHeight;
	this->viewportX2 = viewportX + viewportWidth;
	this->viewportClampX1 = viewportX << TinyGL::SCREEN_SHIFT;
	this->viewportClampX2 = (this->viewportX2 << TinyGL::SCREEN_SHIFT) + TinyGL::SCREEN_ONE - 1;
	this->viewportXScale = viewportWidth << 2;
	this->viewportYScale = viewportHeight << 2;
	this->viewportXBias = ((viewportX + viewportWidth / 2) << 3) - 4;
	this->viewportYBias = ((viewportY + viewportHeight / 2) << 3) - 4;
}

void TinyGL::setViewport(int x, int y, int w, int h) {


	int posX = x - app->canvas->viewRect[0];
	int posY = y - app->canvas->viewRect[1];

	if (!app->render->_gles->isInit) { // [GEC] Adjusted like this to match the Y position on the GL version
		posY = 3;  // posY = app->canvas->viewRect[1];
	}

	if (this->viewportX == posX && this->viewportY == posY && this->viewportWidth == w && this->viewportHeight == h) {
		return;
	}

	this->_setViewport(posX + 1, posY + 1, w - 2, h - 2);

	if (app->render->_gles->isInit) { // [GEC] <- adding new line code
		app->canvas->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
	}
}

void TinyGL::resetViewPort() {

	this->setViewport(app->canvas->viewRect[0], app->canvas->viewRect[1], app->canvas->viewRect[2], app->canvas->viewRect[3]);
}

void TinyGL::setView(int viewX, int viewY, int viewZ, int viewYaw, int viewPitch, int viewRoll, int viewFov, int viewAspect) {
	


	this->viewX = viewX;
	this->viewY = viewY;
	this->viewZ = viewZ;
	this->viewPitch = viewPitch & 0x3ff;
	this->viewYaw = viewYaw & 0x3ff;
	// Mirror viewYaw onto gles (used by RasterizeConvexPolygon's sky path
	// for UV scrolling). Single-writer decoupling toward dropping the gles
	// → tinyGL pointer.
	app->render->_gles->viewYaw = this->viewYaw;

	this->buildViewMatrix(viewX, viewY, viewZ, this->viewYaw, this->viewPitch, viewRoll, this->view);
	this->buildViewMatrix(viewX, viewY, 0, this->viewYaw, 0, 0, this->view2D);
	this->buildProjectionMatrix(viewFov, viewAspect, this->projection);
	this->multMatrix(this->view, this->projection, this->mvp);

	app->render->_gles->BeginFrame(this->viewportX, this->viewportY,
	                                this->viewportWidth, this->viewportHeight,
	                                this->view, this->projection,
	                                this->fogColor, this->fogMin, this->fogRange);

	if (this->viewPitch > 512) {
		this->viewPitch -= 1024;
	}

	this->buildProjectionMatrix(viewFov + std::abs(this->viewPitch), viewAspect, this->projection);
	this->multMatrix(this->view2D, this->projection, this->mvp2D);
}

void TinyGL::viewMtxMove(TGLVert* tglVert, int n, int n2, int n3) {
	if (n != 0) {
		n = -n;
		tglVert->x += this->view[2] * n >> 14;
		tglVert->y += this->view[6] * n >> 14;
		tglVert->z += this->view[10] * n >> 14;
	}
	if (n2 != 0) {
		tglVert->x += this->view[0] * n2 >> 14;
		tglVert->y += this->view[4] * n2 >> 14;
		tglVert->z += this->view[8] * n2 >> 14;
	}
	if (n3 != 0) {
		n3 = -n3;
		tglVert->x += this->view[1] * n3 >> 14;
		tglVert->y += this->view[5] * n3 >> 14;
		tglVert->z += this->view[9] * n3 >> 14;
	}
}

void TinyGL::drawModelVerts(TGLVert* array, int n) {
	if ((app->render->renderMode & 0x1) == 0x0) {
		return;
	}
	app->render->_gles->DrawModelVerts(std::span(array, n));
}

TGLVert* TinyGL::transform3DVerts(TGLVert* array, int n) {
	int* mvp = this->mvp;
	for (int i = 0; i < n; ++i) {
		TGLVert* tglVert = &array[i];
		TGLVert* tglVert2 = &this->cv[i];
		int x = tglVert->x;
		int y = tglVert->y;
		int z = tglVert->z;
		tglVert2->x = (x * mvp[0] + y * mvp[4] + z * mvp[8] >> 14) + mvp[12];
		tglVert2->y = (x * mvp[1] + y * mvp[5] + z * mvp[9] >> 14) + mvp[13];
		tglVert2->z = (x * mvp[2] + y * mvp[6] + z * mvp[10] >> 14) + mvp[14];
		tglVert2->w = (x * mvp[3] + y * mvp[7] + z * mvp[11] >> 14) + mvp[15];
		tglVert2->s = tglVert->s;
		tglVert2->t = tglVert->t;
	}
	return this->cv;
}

TGLVert* TinyGL::transform2DVerts(TGLVert* array, int n) {
	int* mvp2D = this->mvp2D;
	for (int i = 0; i < n; ++i) {
		TGLVert* tglVert = &array[i];
		TGLVert* tglVert2 = &this->cv[i];
		int x = tglVert->x;
		int y = tglVert->y;
		tglVert2->x = (x * mvp2D[0] + y * mvp2D[4] >> 14) + mvp2D[12];
		tglVert2->z = (x * mvp2D[2] + y * mvp2D[6] >> 14) + mvp2D[14];
		tglVert2->w = (x * mvp2D[3] + y * mvp2D[7] >> 14) + mvp2D[15];
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
	const int xBias = this->viewportXBias;
	const int yBias = this->viewportYBias;
	const int xScale = this->viewportXScale;
	const int yScale = this->viewportYScale;
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

void TinyGL::resetCounters() {
	this->countDrawn = 0;
	this->spanPixels = 0;
}


// [GEC]

