// View / viewport / matrix machinery moved from TinyGL.cpp in B3.14a.
// Each function is the identical body — TinyGL:: → Render::, internal
// TinyGL state (mvp/mvp2D/viewport scratch) now lives on Render.

#include <cmath>
#include <cstdlib>

#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "GLES.h"
#include "Render.h"
#include "TGLVert.h"
#include "TinyGL.h"

void Render::buildViewMatrix(int x, int y, int z, int yaw, int pitch, int roll, float* matrix) {
	// 10-bit angles (0..1023 = 0..2π).
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

	// Translation column: -(R · (x,y,z)).
	for (int i = 0; i < 3; ++i) {
		matrix[12 + i] = -((float)x * matrix[0 + i] + (float)y * matrix[4 + i] + (float)z * matrix[8 + i]);
	}

	matrix[3]  = 0.0f;
	matrix[7]  = 0.0f;
	matrix[11] = 0.0f;
	matrix[15] = 1.0f;
}

void Render::buildProjectionMatrix(int fov, int aspect, float* matrix) {
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

void Render::multMatrix(const float* matrix1, const float* matrix2, float* destMtx) {
	// Column-major: computes destMtx = matrix2 * matrix1 in math order
	// (i.e. mvp = projection * view).
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			destMtx[i * 4 + j] = matrix1[i * 4 + 0] * matrix2[0 + j]
			                  + matrix1[i * 4 + 1] * matrix2[4 + j]
			                  + matrix1[i * 4 + 2] * matrix2[8 + j]
			                  + matrix1[i * 4 + 3] * matrix2[12 + j];
		}
	}
}

void Render::_setViewport(int viewportX, int viewportY, int viewportWidth, int viewportHeight) {
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

void Render::setViewport(int x, int y, int w, int h) {
	int posX = x - app->canvas->viewRect[0];
	int posY = y - app->canvas->viewRect[1];

	if (!this->_gles->isInit) { // [GEC] Adjusted like this to match the Y position on the GL version
		posY = 3;
	}

	if (this->viewportX == posX && this->viewportY == posY && this->viewportWidth == w && this->viewportHeight == h) {
		return;
	}

	this->_setViewport(posX + 1, posY + 1, w - 2, h - 2);

	if (this->_gles->isInit) {
		app->canvas->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
	}
}

void Render::resetViewPort() {
	this->setViewport(app->canvas->viewRect[0], app->canvas->viewRect[1], app->canvas->viewRect[2], app->canvas->viewRect[3]);
}

void Render::setView(int viewX, int viewY, int viewZ, int viewYaw, int viewPitch, int viewRoll, int viewFov, int viewAspect) {
	const int normYaw   = viewYaw   & 0x3ff;
	int       sgnPitch  = viewPitch & 0x3ff;
	this->_gles->viewYaw = normYaw;

	// view2D and projection are scratch for the two multMatrix calls.
	float view2D[16];
	float projection[16];

	this->buildViewMatrix(viewX, viewY, viewZ, normYaw, sgnPitch, viewRoll, this->view);
	this->buildViewMatrix(viewX, viewY, 0, normYaw, 0, 0, view2D);
	this->buildProjectionMatrix(viewFov, viewAspect, projection);
	this->multMatrix(this->view, projection, this->mvp);

	this->_gles->BeginFrame(this->viewportX, this->viewportY,
	                        this->viewportWidth, this->viewportHeight,
	                        this->view, projection,
	                        this->fogColor, this->fogMin, this->fogRange);

	if (sgnPitch > 512) {
		sgnPitch -= 1024;
	}

	// 2D mvp uses a wider FOV (compensating for the dropped pitch) and the
	// pitch-zeroed view2D. Used only by transform2DVerts for line-visibility
	// queries against world-axis-aligned walls.
	this->buildProjectionMatrix(viewFov + std::abs(sgnPitch), viewAspect, projection);
	this->multMatrix(view2D, projection, this->mvp2D);

	this->viewPitch = sgnPitch;
}
