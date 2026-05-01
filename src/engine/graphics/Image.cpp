// Expose GL 2.0+ extension prototypes (glUniform*, glVertexAttribPointer, …)
// for the B2 programmable-pipeline migration. macOS's <OpenGL/gl.h> ships
// up to GL 1.4 by default and gates the rest behind this macro.
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <stdexcept>
#include <cstdlib>

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
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
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
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(scaleW + (float)(scaleX), scaleH + (float)(scaleY), 0.0);
    switch (rotateMode)
    {
    case 1:
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        break;
    case 2:
        glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
        break;
    case 3:
        glRotatef(270.0f, 0.0f, 0.0f, 1.0f);
        break;
    case 4:
        glScalef(-1.0f, 1.0f, 1.0f);
        break;
    case 5:
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        glScalef(-1.0f, 1.0f, 1.0f);
        break;
    case 6:
        glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
        glScalef(-1.0f, 1.0f, 1.0f);
        break;
    case 7:
        glRotatef(270.0f, 0.0f, 0.0f, 1.0f);
        glScalef(-1.0f, 1.0f, 1.0f);
        break;

    case 8: // New
        glScalef(1.0f, -1.0f, 1.0f);
        break;
    default:
        break;
    }

    // Migration to programmable pipeline (B2): if the shader is ready, use it.
    // Hybrid for now — fixed-function still computes the matrix stack above;
    // we read it back via glGetFloatv and pass as u_mvp. The modulator color
    // comes from this->modColor (set by setRenderMode); reading back via
    // glGetFloatv(GL_CURRENT_COLOR) is unsafe because REPLACE-mode branches
    // don't touch glColor and we'd inherit stale state from previous draws.
    if (_glesObj && _glesObj->isShaderReady) {
        float proj[16], mv[16], mvp[16];
        glGetFloatv(GL_PROJECTION_MATRIX, proj);
        glGetFloatv(GL_MODELVIEW_MATRIX, mv);
        mat4Mul(mvp, proj, mv);

        const float* color = this->modColor;

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
    } else {
        glVertexPointer(3, GL_FLOAT, 0, vp);
        glEnableClientState(GL_VERTEX_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, st);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glPopMatrix();
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

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
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
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    if (rotated) {
        glTranslatef((float)(hh + (float)posX) + (240.0f - hh), hw + (float)posY, 0.0f);
        glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
    } else {
        glTranslatef(hw + (float)posX, hh + (float)posY, 0.0f);
    }

    // B2.4: shader path for fade-overlay quads. The matrix stack is still set
    // up by glPushMatrix/glTranslatef above; we read the result via
    // glGetFloatv. u_color is built directly from `alpha` (the only modulator
    // this path takes) instead of going through GL_CURRENT_COLOR.
    if (_glesObj && _glesObj->isShaderReady) {
        float proj[16], mv[16], mvp[16];
        glGetFloatv(GL_PROJECTION_MATRIX, proj);
        glGetFloatv(GL_MODELVIEW_MATRIX, mv);
        mat4Mul(mvp, proj, mv);

        const float color[4] = {1.0f, 1.0f, 1.0f, alpha};

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
    } else {
        glVertexPointer(3, GL_FLOAT, 0, vp);
        glEnableClientState(GL_VERTEX_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, st);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glPopMatrix();
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
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        if (!this->isTransparentMask) {
            glDisable(GL_ALPHA_TEST);
            glDisable(GL_BLEND);
            return;
        }
        glEnable(GL_ALPHA_TEST);
        break;
    case 1:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        this->modColor[3] = 0.25f;
        glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
        glDisable(GL_ALPHA_TEST);
        break;
    case 2:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        this->modColor[3] = 0.5f;
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        glDisable(GL_ALPHA_TEST);
        break;
    case 3:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        return;
    case 8:
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_BLEND);
        color = Graphics::charColors[app->canvas->graphics.currentCharColor];
        this->modColor[0] = ((color >> 16) & 0xff) / 255.0f;
        this->modColor[1] = ((color >>  8) & 0xff) / 255.0f;
        this->modColor[2] = ( color        & 0xff) / 255.0f;
        this->modColor[3] = ((color >> 24) & 0xff) / 255.0f;
        glColor4ub(color >> 0x10 & 0xff, color >> 8 & 0xff, color & 0xff, color >> 0x18);
        return;
    case 12:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        this->modColor[3] = 0.75f;
        glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
        glDisable(GL_ALPHA_TEST);
        break;
    case 13:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        this->modColor[3] = app->canvas->blendSpecialAlpha;
        glColor4f(1.0f, 1.0f, 1.0f, app->canvas->blendSpecialAlpha);
        glDisable(GL_ALPHA_TEST);
        break;
    default:
        return;
    }
    glAlphaFunc(GL_GREATER, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
