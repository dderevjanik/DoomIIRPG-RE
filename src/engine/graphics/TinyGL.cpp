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

	this->setViewport(canvas->viewRect[0], canvas->viewRect[1], canvas->viewRect[2], canvas->viewRect[3]);
	this->setView( 0, 0, 0, 0, 0, 0, 290, 290);
	this->fogMin = 32752;
	this->fogRange = 1;
	this->fogColor = 0;
	return true;
}

void TinyGL::buildViewMatrix(int x, int y, int z, int yaw, int pitch, int roll, float* matrix) {
	// 10-bit angles (0..1023 = 0..2π). Replaces the legacy Q14 sinTable
	// lookup-and-shift gymnastics. std::sin/std::cos at this call rate is a
	// fraction of a microsecond; the FPU is plenty fast for it.
	//
	// CAREFUL: the legacy variables `cosYaw`/`sinYaw`/`cosPitch`/`sinPitch`
	// are MISNAMED — they index into the sin table at +512 / +256 offsets,
	// which gives `sin(angle + π) = -sin(angle)` and `sin(angle + π/2) =
	// cos(angle)` respectively. We preserve the legacy semantics (the matrix
	// formulas below depend on the misnaming) but compute the values from
	// honest trig.  Roll's two variables ARE named correctly.
	constexpr float kAngleToRad = 3.14159265358979323846f / 512.0f;
	const float cosYaw   = -std::sin(yaw   * kAngleToRad);  // legacy var: -sin
	const float sinYaw   =  std::cos(yaw   * kAngleToRad);  // legacy var: cos
	const float cosPitch = -std::sin(pitch * kAngleToRad);  // legacy var: -sin
	const float sinPitch =  std::cos(pitch * kAngleToRad);  // legacy var: cos
	const float sinRoll  =  std::sin(roll  * kAngleToRad);
	const float cosRoll  =  std::cos(roll  * kAngleToRad);

	const float n13 = sinRoll * cosPitch;
	const float n14 = cosRoll * cosPitch;
	matrix[0]  = n13 * sinYaw + cosRoll * -cosYaw;
	matrix[4]  = n13 * cosYaw + cosRoll *  sinYaw;
	matrix[8]  = sinRoll * sinPitch;
	matrix[1]  = -(n14 * sinYaw + -sinRoll * -cosYaw);
	matrix[5]  = -(n14 * cosYaw + -sinRoll *  sinYaw);
	matrix[9]  = -(cosRoll * sinPitch);
	matrix[2]  = -(sinPitch * sinYaw);
	matrix[6]  = -(sinPitch * cosYaw);
	matrix[10] = cosPitch;

	// Translation column: -(R · (x,y,z)) — apply rotation to the eye-position
	// and negate, the standard view-matrix translation.
	for (int i = 0; i < 3; ++i) {
		matrix[12 + i] = -((float)x * matrix[0 + i] + (float)y * matrix[4 + i] + (float)z * matrix[8 + i]);
	}

	matrix[3]  = 0.0f;
	matrix[7]  = 0.0f;
	matrix[11] = 0.0f;
	matrix[15] = 1.0f;
}

void TinyGL::buildProjectionMatrix(int fov, int aspect, float* matrix) {
	// Half-FOV for the legacy projection. `aspect` is a 10-bit half-range
	// angle so n3 is the half-FOV; the engine's convention matches the
	// original Q14 builder, just expressed in float now.
	constexpr float kAngleToRad = 3.14159265358979323846f / 512.0f;
	const int   n3 = aspect >> 1;
	const float n4 = (float)fov / (float)aspect;
	const float n5 = std::sin(n3 * kAngleToRad);
	const float n6 = std::cos(n3 * kAngleToRad);

	matrix[0]  = n6 / (n4 * n5);
	matrix[4]  = 0.0f;
	matrix[8]  = 0.0f;
	matrix[12] = 0.0f;
	matrix[1]  = 0.0f;
	matrix[5]  = n6 / n5;
	matrix[9]  = 0.0f;
	matrix[13] = 0.0f;
	matrix[2]  = 0.0f;
	matrix[6]  = 0.0f;
	matrix[10] = -1.0f;
	matrix[14] = -(float)(2 * TinyGL::NEAR_CLIP);
	matrix[3]  = 0.0f;
	matrix[7]  = 0.0f;
	matrix[11] = -1.0f;
	matrix[15] = 0.0f;
}

void TinyGL::multMatrix(const float* matrix1, const float* matrix2, float* destMtx) {
	// Same loop layout as the legacy Q14 version (column-major; computes
	// destMtx = matrix2 * matrix1 in math order, i.e. mvp = projection * view).
	// No `>> 14` since both operands are pure floats.
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			destMtx[i * 4 + j] = matrix1[i * 4 + 0] * matrix2[0 + j]
			                  + matrix1[i * 4 + 1] * matrix2[4 + j]
			                  + matrix1[i * 4 + 2] * matrix2[8 + j]
			                  + matrix1[i * 4 + 3] * matrix2[12 + j];
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

	// `view2D` and `projection` were members but only ever scratch for the
	// two multMatrix calls below; promoted to locals to drop 128 bytes of
	// per-frame zombie state.
	float view2D[16];
	float projection[16];

	this->buildViewMatrix(viewX, viewY, viewZ, this->viewYaw, this->viewPitch, viewRoll, this->view);
	this->buildViewMatrix(viewX, viewY, 0, this->viewYaw, 0, 0, view2D);
	this->buildProjectionMatrix(viewFov, viewAspect, projection);
	this->multMatrix(this->view, projection, this->mvp);

	app->render->_gles->BeginFrame(this->viewportX, this->viewportY,
	                                this->viewportWidth, this->viewportHeight,
	                                this->view, projection,
	                                this->fogColor, this->fogMin, this->fogRange);

	if (this->viewPitch > 512) {
		this->viewPitch -= 1024;
	}

	// 2D mvp uses a wider FOV (compensating for the dropped pitch) and the
	// pitch-zeroed view2D. Used only by transform2DVerts for line-visibility
	// queries against world-axis-aligned walls.
	this->buildProjectionMatrix(viewFov + std::abs(this->viewPitch), viewAspect, projection);
	this->multMatrix(view2D, projection, this->mvp2D);
}

void TinyGL::viewMtxMove(TGLVert* tglVert, int n, int n2, int n3) {
	// Move tglVert along the view-basis axes: forward (-n), right (+n2),
	// up (-n3). With float `view`, the legacy `>> 14` Q14 shift collapses
	// into a plain `(int)(view[i] * delta)`.
	if (n != 0) {
		const float fn = (float)(-n);
		tglVert->x += (int)(this->view[2]  * fn);
		tglVert->y += (int)(this->view[6]  * fn);
		tglVert->z += (int)(this->view[10] * fn);
	}
	if (n2 != 0) {
		const float fn = (float)n2;
		tglVert->x += (int)(this->view[0] * fn);
		tglVert->y += (int)(this->view[4] * fn);
		tglVert->z += (int)(this->view[8] * fn);
	}
	if (n3 != 0) {
		const float fn = (float)(-n3);
		tglVert->x += (int)(this->view[1] * fn);
		tglVert->y += (int)(this->view[5] * fn);
		tglVert->z += (int)(this->view[9] * fn);
	}
}

TGLVert* TinyGL::transform3DVerts(TGLVert* array, int n) {
	const float* mvp = this->mvp;
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
	const float* mvp2D = this->mvp2D;
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

// [GEC]

