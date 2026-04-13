#include <stdexcept>
#include <cstdlib>

#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Image.h"
#include "SDLGL.h"

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

    if (this->isTransparentMask == false) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
    }
    else {
        uint32_t i = 0;
        uint16_t* dat = data;
        while (i < height * width) {
            uint32_t pix = *dat;
            if (pix == 0xf81f) {
                pix = 0x0000;
            }
            else {
                pix = pix & 0xffc0 | (uint16_t)((pix & 0x1f) << 1) | 1;
            }
            *dat++ = pix;
            i++;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, data);
    }
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
    glVertexPointer(3, GL_FLOAT, 0, vp);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, st);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
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
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
    glVertexPointer(3, GL_FLOAT, 0, vp);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, st);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    if (rotated) {
        glTranslatef((float)(hh + (float)posX) + (240.0f - hh), hw + (float)posY, 0.0f);
        glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
    } else {
        glTranslatef(hw + (float)posX, hh + (float)posY, 0.0f);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glPopMatrix();
}

void Image::setRenderMode(int renderMode) {
    if (!this->app) {
        this->app = CAppContainer::getInstance()->app;
        this->headless = this->headless;
    }
    Applet* app = this->app;
    int color;

    switch (renderMode) {
    case 0:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        if (this->isTransparentMask == false) {
            glDisable(GL_ALPHA_TEST);
            glDisable(GL_BLEND);
            return;
        }
        glEnable(GL_ALPHA_TEST);
        break;
    case 1:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
        glDisable(GL_ALPHA_TEST);
        break;
    case 2:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        glDisable(GL_ALPHA_TEST);
        break;
    case 3:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        return;
    case 8:
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_BLEND);
        color = Graphics::charColors[app->canvas->graphics.currentCharColor];
        glColor4ub(color >> 0x10 & 0xff, color >> 8 & 0xff, color & 0xff, color >> 0x18);
        return;
    case 12:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
        glDisable(GL_ALPHA_TEST);
        break;
    case 13:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
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
