#pragma once
#include <stdexcept>
#include <stdint.h>

class TinyGL;

using SpanFunc = void (*)(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
using SpanFuncStretch = void (*)(uint16_t*, int32_t, int32_t, int32_t, int32_t, TinyGL*);

struct SpanMode {
    SpanFunc Normal;
    SpanFunc DT;
    SpanFunc DS;
    SpanFuncStretch Stretch;
};

struct SpanType {
    SpanMode* Span;
};


extern void spanNoDraw(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanNoDrawStretch(uint16_t*, int32_t, int32_t, int32_t, int32_t, TinyGL*);

extern void spanTransparent(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanTransparentDT(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanTransparentDS(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanTransparentStretch(uint16_t*, int32_t, int32_t, int32_t, int32_t, TinyGL*);

extern void spanBlend50Transparent(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanBlend50TransparentDT(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanBlend50TransparentDS(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanBlend50TransparentStretch(uint16_t*, int32_t, int32_t, int32_t, int32_t, TinyGL*);

extern void spanAddTransparent(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanAddTransparentDT(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanAddTransparentDS(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanAddTransparentStretch(uint16_t*, int32_t, int32_t, int32_t, int32_t, TinyGL*);

extern void spanSubTransparent(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanSubTransparentDT(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanSubTransparentDS(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanSubTransparentStretch(uint16_t*, int32_t, int32_t, int32_t, int32_t, TinyGL*);

extern void spanPerfTexture(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanPerfTextureStretch(uint16_t*, int32_t, int32_t, int32_t, int32_t, TinyGL*);

extern void spanTexture(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanTextureDT(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanTextureDS(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);

extern void spanBlend25Texture(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanBlend25TextureDT(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanBlend25TextureDS(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);

extern void spanAddTexture(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanAddTextureDT(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanAddTextureDS(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);

extern void spanSubTexture(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanSubTextureDT(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
extern void spanSubTextureDS(uint16_t*, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, TinyGL*);
