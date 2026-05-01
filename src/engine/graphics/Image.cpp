// Expose GL 2.0+ extension prototypes (glUniform*, glVertexAttribPointer, …)
// for the B2 programmable-pipeline migration. macOS's <OpenGL/gl.h> ships
// up to GL 1.4 by default and gates the rest behind this macro.
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <stdexcept>
#include <cstdlib>
#include <cmath>

#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Image.h"
#include "SDLGL.h"
#include "GLES.h"
// Multiply two column-major 4x4 matrices: dst = a * b. Used by the shader
// migration (B2) to combine the fixed-function projection and modelview
// matrices into a single u_mvp uniform.
static void mat4Mul(float dst[16], const float a[16], const float b[16]) {
	for (int c = 0; c < 4; c++) {
		for (int r = 0; r < 4; r++) {
			dst[c * 4 + r] = a[0 * 4 + r] * b[c * 4 + 0]
			              + a[1 * 4 + r] * b[c * 4 + 1]
			              + a[2 * 4 + r] * b[c * 4 + 2]
			              + a[3 * 4 + r] * b[c * 4 + 3];
		}
	}
}

// Build the 2D modelview matrix that the legacy fixed-function path used to
// produce via glLoadIdentity / glTranslatef / glRotatef / glScalef. Replaces
// the matrix-stack readback and lets us delete the GL_PROJECTION/GL_MODELVIEW
// matrix-mode plumbing entirely (B2.9). Column-major to match GL convention.
//
// rotateMode mirrors Image::DrawTexture's switch:
//   0=identity, 1=90°, 2=180°, 3=270°, 4=flipX, 5..7=rot+flipX, 8=flipY.
static void buildModelView2D(float out[16], float tx, float ty, int rotateMode) {
	// Decompose the cases into (rotation degrees, mirror-x flag, mirror-y flag).
	int rotDeg = 0;
	bool mirrorX = false;
	bool mirrorY = false;
	switch (rotateMode) {
	case 1: rotDeg = 90;  break;
	case 2: rotDeg = 180; break;
	case 3: rotDeg = 270; break;
	case 4: mirrorX = true; break;
	case 5: rotDeg = 90;  mirrorX = true; break;
	case 6: rotDeg = 180; mirrorX = true; break;
	case 7: rotDeg = 270; mirrorX = true; break;
	case 8: mirrorY = true; break;
	default: break;
	}

	// translate(tx, ty, 0) — column-major
	float t[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		tx, ty, 0, 1,
	};

	// Rotation around Z by rotDeg degrees, exact integer values for sin/cos.
	float c, s;
	switch (rotDeg) {
	case 0:   c = 1;  s = 0;  break;
	case 90:  c = 0;  s = 1;  break;
	case 180: c = -1; s = 0;  break;
	case 270: c = 0;  s = -1; break;
	default:  c = 1;  s = 0;  break;
	}
	float r[16] = {
		 c,  s, 0, 0,
		-s,  c, 0, 0,
		 0,  0, 1, 0,
		 0,  0, 0, 1,
	};

	// Combined scale for mirror flags. Both mirrors never apply simultaneously
	// in the original switch, but the matrix handles them independently.
	float sx = mirrorX ? -1.0f : 1.0f;
	float sy = mirrorY ? -1.0f : 1.0f;
	float sm[16] = {
		sx, 0, 0, 0,
		0, sy, 0, 0,
		0,  0, 1, 0,
		0,  0, 0, 1,
	};

	// out = t * r * sm   (translate first, then rotate, then mirror — matching
	// glTranslatef -> glRotatef -> glScalef order with column-major math).
	float tr[16];
	mat4Mul(tr, t, r);
	mat4Mul(out, tr, sm);
}

// Same as above but rotate by an arbitrary angle (used by DrawTextureAlpha
// rotated mode and DrawPortalTexture). angleDeg in degrees.
static void buildModelView2DAngle(float out[16], float tx, float ty, float angleDeg) {
	const float pi = 3.14159265358979323846f;
	float a = angleDeg * pi / 180.0f;
	float c = std::cos(a);
	float s = std::sin(a);
	float t[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		tx, ty, 0, 1,
	};
	float r[16] = {
		 c,  s, 0, 0,
		-s,  c, 0, 0,
		 0,  0, 1, 0,
		 0,  0, 0, 1,
	};
	mat4Mul(out, t, r);
}

Image::Image() {
    this->app = nullptr;
}

Image::~Image() {
    if (this) {
        if (this->piDIB) {
            this->piDIB->~IDIB();
            std::free(this->piDIB);
        }
        this->piDIB = nullptr;
        if (!this->headless) {
            glDeleteTextures(1, &this->texture);
        }
        this->texture = -1;
        std::free(this);
    }
}

void Image::CreateTexture(uint16_t* data, uint32_t width, uint32_t height) {
    if (this->headless) { return; }
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &this->texture);
    glBindTexture(GL_TEXTURE_2D, this->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Convert RGB565 to RGBA8 for modern OpenGL compatibility
    // (GL_UNSIGNED_SHORT_5_6_5 / GL_UNSIGNED_SHORT_5_5_5_1 are unreliable on macOS Metal-backed OpenGL)
    uint8_t* rgba = (uint8_t*)std::malloc(width * height * 4);
    for (uint32_t i = 0; i < width * height; i++) {
        uint16_t rgb = data[i];
        uint8_t r5 = (rgb >> 11) & 0x1F;
        uint8_t g6 = (rgb >> 5) & 0x3F;
        uint8_t b5 = rgb & 0x1F;
        rgba[i * 4 + 0] = (r5 << 3) | (r5 >> 2);
        rgba[i * 4 + 1] = (g6 << 2) | (g6 >> 4);
        rgba[i * 4 + 2] = (b5 << 3) | (b5 >> 2);
        if (this->isTransparentMask && rgb == 0xF81F) {
            rgba[i * 4 + 0] = 0;
            rgba[i * 4 + 1] = 0;
            rgba[i * 4 + 2] = 0;
            rgba[i * 4 + 3] = 0;
        } else {
            rgba[i * 4 + 3] = 0xFF;
        }
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    std::free(rgba);
}

void Image::DrawTexture(int texX, int texY, int texW, int texH, int posX, int posY, int rotateMode, int renderMode) {
    float scaleW, scaleH;
    float scaleTexW, scaleTexH;
    float scaleX, scaleY;
    float vp[12];
    float st[8];

    PFNGLACTIVETEXTUREPROC glActiveTexture = (PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");

    this->setRenderMode(renderMode);
    scaleW = (float)texW * 0.5f;
    scaleH = (float)texH * 0.5f;
    scaleX = (float)posX;
    scaleY = (float)posY;

    //CAppContainer::getInstance()->sdlGL->transformCoord2f(&scaleW, &scaleH);
    //CAppContainer::getInstance()->sdlGL->transformCoord2f(&scaleX, &scaleY);

    vp[2] = 0.5f;
    vp[5] = 0.5f;
    vp[0] = scaleW;
    vp[8] = 0.5f;
    vp[11] = 0.5f;
    vp[3] = -scaleW;
    vp[6] = scaleW;
    vp[7] = scaleH;
    vp[1] = -scaleH;
    vp[4] = -scaleH;
    vp[9] = -scaleW;
    vp[10] = scaleH;

    st[0] = 0.0f;
    st[1] = 0.0f;
    st[2] = 0.0f;
    st[3] = 0.0f;
    st[4] = 0.0f;
    st[5] = 0.0f;
    st[6] = 0.0f;
    st[7] = 0.0f;

    scaleTexW = 1.0f / (float)this->texWidth;
    scaleTexH = 1.0f / (float)this->texHeight;
    st[0] = scaleTexW * (float)(texW + texX);
    st[4] = st[0];
    st[1] = scaleTexH * (float)texY;
    st[3] = st[1];
    st[2] = scaleTexW * (float)texX;
    st[6] = st[2];
    st[5] = scaleTexH * (float)(texH + texY);
    st[7] = st[5];

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Compose the MVP explicitly. Previously this was done via glPushMatrix /
    // glLoadIdentity / glTranslatef / glRotatef / glScalef and read back with
    // glGetFloatv — kept the legacy matrix mode functional. With B2.9 the
    // shader path computes its own matrix from the gles projection member,
    // and the GL_PROJECTION/GL_MODELVIEW state is no longer consulted.
    float mv[16];
    buildModelView2D(mv, scaleW + (float)scaleX, scaleH + (float)scaleY, rotateMode);
    float mvp[16];
    // ortho2D is the permanent 2D screen-space projection; projectionMatrix
    // would be the 3D camera (overwritten by BeginFrame on every world render).
    mat4Mul(mvp, _glesObj->ortho2D, mv);

    const float* color = this->modColor;

    // ImGui's OpenGL3 backend leaves VBOs bound after RenderDrawData; unbind
    // so our client-side glVertexAttribPointer isn't reinterpreted as an
    // offset into ImGui's buffer.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    _glesObj->textureShader.use();
    const GLint uMvp = _glesObj->textureShader.uniform("u_mvp");
    const GLint uColor = _glesObj->textureShader.uniform("u_color");
    const GLint uAddColor = _glesObj->textureShader.uniform("u_addColor");
    const GLint uTex = _glesObj->textureShader.uniform("u_tex");
    const GLint aPos = _glesObj->textureShader.attribute("a_pos");
    const GLint aUv = _glesObj->textureShader.attribute("a_uv");
    static constexpr float kZeroAdd[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (uMvp >= 0)   glUniformMatrix4fv(uMvp, 1, GL_FALSE, mvp);
    if (uColor >= 0) glUniform4fv(uColor, 1, color);
    if (uAddColor >= 0) glUniform4fv(uAddColor, 1, kZeroAdd);
    if (uTex >= 0)   glUniform1i(uTex, 0);
    if (aPos >= 0) {
        glEnableVertexAttribArray(aPos);
        glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 0, vp);
    }
    if (aUv >= 0) {
        glEnableVertexAttribArray(aUv);
        glVertexAttribPointer(aUv, 2, GL_FLOAT, GL_FALSE, 0, st);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    if (aPos >= 0) glDisableVertexAttribArray(aPos);
    if (aUv >= 0)  glDisableVertexAttribArray(aUv);
    Shader::useNone();
}

void Image::DrawTextureAlpha(int posX, int posY, float alpha, bool rotated, bool flipUV) {
    if (this->headless) return;

    PFNGLACTIVETEXTUREPROC glActiveTexture = (PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");

    float w = (float)this->width;
    float h = (float)this->height;
    float hw = w * 0.5f;
    float hh = h * 0.5f;

    float vp[12];
    vp[0] =  hw; vp[1] = -hh; vp[2]  = 0.5f;
    vp[3] = -hw; vp[4] = -hh; vp[5]  = 0.5f;
    vp[6] =  hw; vp[7] =  hh; vp[8]  = 0.5f;
    vp[9] = -hw; vp[10] = hh; vp[11] = 0.5f;

    float scaleTexW = 1.0f / (float)this->texWidth;
    float scaleTexH = 1.0f / (float)this->texHeight;
    float u1 = w * scaleTexW;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float v1 = h * scaleTexH;

    float st[8];
    if (flipUV) {
        st[0] = u0; st[1] = v1;
        st[2] = u1; st[3] = v1;
        st[4] = u0; st[5] = v0;
        st[6] = u1; st[7] = v0;
    } else {
        st[0] = u1; st[1] = v0;
        st[2] = u0; st[3] = v0;
        st[4] = u1; st[5] = v1;
        st[6] = u0; st[7] = v1;
    }

    glDisable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Compose modelview explicitly. `rotated` mode rotates -90° around Z and
    // shifts to compose against the rotated basis; non-rotated is just a
    // translate. Replaces the legacy glPushMatrix / glTranslatef / glRotatef
    // chain plus GL_MODELVIEW_MATRIX readback.
    float mv[16];
    if (rotated) {
        buildModelView2DAngle(mv,
                              (float)(hh + (float)posX) + (240.0f - hh),
                              hw + (float)posY,
                              -90.0f);
    } else {
        buildModelView2D(mv, hw + (float)posX, hh + (float)posY, 0);
    }
    float mvp[16];
    // ortho2D is the permanent 2D screen-space projection; projectionMatrix
    // would be the 3D camera (overwritten by BeginFrame on every world render).
    mat4Mul(mvp, _glesObj->ortho2D, mv);

    const float color[4] = {1.0f, 1.0f, 1.0f, alpha};

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    _glesObj->textureShader.use();
    const GLint uMvp = _glesObj->textureShader.uniform("u_mvp");
    const GLint uColor = _glesObj->textureShader.uniform("u_color");
    const GLint uAddColor = _glesObj->textureShader.uniform("u_addColor");
    const GLint uTex = _glesObj->textureShader.uniform("u_tex");
    const GLint aPos = _glesObj->textureShader.attribute("a_pos");
    const GLint aUv = _glesObj->textureShader.attribute("a_uv");
    static constexpr float kZeroAdd[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (uMvp >= 0)   glUniformMatrix4fv(uMvp, 1, GL_FALSE, mvp);
    if (uColor >= 0) glUniform4fv(uColor, 1, color);
    if (uAddColor >= 0) glUniform4fv(uAddColor, 1, kZeroAdd);
    if (uTex >= 0)   glUniform1i(uTex, 0);
    if (aPos >= 0) {
        glEnableVertexAttribArray(aPos);
        glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 0, vp);
    }
    if (aUv >= 0) {
        glEnableVertexAttribArray(aUv);
        glVertexAttribPointer(aUv, 2, GL_FLOAT, GL_FALSE, 0, st);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    if (aPos >= 0) glDisableVertexAttribArray(aPos);
    if (aUv >= 0)  glDisableVertexAttribArray(aUv);
    Shader::useNone();
}

void Image::setRenderMode(int renderMode) {
    if (!this->app) {
        this->app = CAppContainer::getInstance()->app;
        this->headless = this->headless;
    }
    Applet* app = this->app;
    int color;

    // Default for REPLACE modes — texture passes through unmodulated.
    this->modColor[0] = 1.0f;
    this->modColor[1] = 1.0f;
    this->modColor[2] = 1.0f;
    this->modColor[3] = 1.0f;

    switch (renderMode) {
    case 0:
        if (!this->isTransparentMask) {
            glDisable(GL_ALPHA_TEST);
            glDisable(GL_BLEND);
            return;
        }
        glEnable(GL_ALPHA_TEST);
        break;
    case 1:
        this->modColor[3] = 0.25f;
        glDisable(GL_ALPHA_TEST);
        break;
    case 2:
        this->modColor[3] = 0.5f;
        glDisable(GL_ALPHA_TEST);
        break;
    case 3:
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        return;
    case 8:
        glEnable(GL_BLEND);
        color = Graphics::charColors[app->canvas->graphics.currentCharColor];
        this->modColor[0] = ((color >> 16) & 0xff) / 255.0f;
        this->modColor[1] = ((color >>  8) & 0xff) / 255.0f;
        this->modColor[2] = ( color        & 0xff) / 255.0f;
        this->modColor[3] = ((color >> 24) & 0xff) / 255.0f;
        return;
    case 12:
        this->modColor[3] = 0.75f;
        glDisable(GL_ALPHA_TEST);
        break;
    case 13:
        this->modColor[3] = app->canvas->blendSpecialAlpha;
        glDisable(GL_ALPHA_TEST);
        break;
    default:
        return;
    }
    glAlphaFunc(GL_GREATER, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
