# DoomRPG Mappings Format (mappings.bin)

This document describes the binary format of `mappings.bin` used in DoomRPG.

## Overview

The mappings file acts as a global index/lookup table that connects texture and sprite IDs to their data offsets in other binary files (`wtexels.bin`, `stexels.bin`, `bitshapes.bin`). It is loaded at the start of every map load.

## Byte Reading Functions

See [BSP_FORMAT.md](BSP_FORMAT.md) for the shared byte reading functions used across all binary formats.

---

## File Structure

```
+---------------------------+
| Header (4 x int)          |
+---------------------------+
| Texel Offsets (N1 x int)  |
+---------------------------+
| BitShape Offsets (N2 x int)|
+---------------------------+
| Texture IDs (N3 x short)  |
+---------------------------+
| Sprite IDs (N4 x short)   |
+---------------------------+
```

---

## Format Details

### Header

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0x00 | 4 | int32 (LE) | texelOffsetCount | Number of texel offset pairs (actual count = value * 2) |
| 0x04 | 4 | int32 (LE) | bitShapeOffsetCount | Number of bitshape offset pairs (actual count = value * 2) |
| 0x08 | 4 | int32 (LE) | textureCnt | Number of texture ID entries |
| 0x0C | 4 | int32 (LE) | spriteCnt | Number of sprite ID entries |

**Note:** The texel and bitshape counts stored in the header are halved. Multiply by 2 to get the actual array length. Each logical entry consists of a pair of two int values.

### Texel Offsets Array

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x10 | N1 * 4 | int32[] (LE) | Byte offsets into `wtexels.bin` / `stexels.bin` data |

Where N1 = `texelOffsetCount * 2`

Each texture is referenced by a pair of values at index `[id * 2]` and `[id * 2 + 1]`:
- `[id * 2 + 0]` — Byte offset to the texture data in `wtexels.bin`
- `[id * 2 + 1]` — Palette offset for the texture

### BitShape Offsets Array

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| varies | N2 * 4 | int32[] (LE) | Byte offsets into `bitshapes.bin` data |

Where N2 = `bitShapeOffsetCount * 2`

Each sprite is referenced by a pair of values at index `[id * 2]` and `[id * 2 + 1]`:
- `[id * 2 + 0]` — Byte offset to the shape data in `bitshapes.bin`
- `[id * 2 + 1]` — Associated data offset

### Texture IDs Array

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| varies | N3 * 2 | int16[] (LE) | Texture ID mapping table |

Where N3 = `textureCnt`

### Sprite IDs Array

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| varies | N4 * 2 | int16[] (LE) | Sprite ID mapping table |

Where N4 = `spriteCnt`

---

## Data Flow

The mappings file connects the rendering pipeline:

```
BSP map file
  └─ references texture/sprite by index
      └─ mediaTexturesIds[index] / mediaSpriteIds[index]
          └─ looks up offsets in mediaTexelOffsets[] / mediaBitShapeOffsets[]
              └─ reads pixel data from wtexels.bin / stexels.bin / bitshapes.bin
```

## Loading (Pseudocode)

```c
byte* data = fileOpenRead("/mappings.bin");
int pos = 0;

int texelsCnt     = intAtNext(data, &pos) * 2;
int bitShapeCnt   = intAtNext(data, &pos) * 2;
int textureCnt    = intAtNext(data, &pos);
int spriteCnt     = intAtNext(data, &pos);

int* texelOffsets    = malloc(texelsCnt * sizeof(int));
int* bitShapeOffsets = malloc(bitShapeCnt * sizeof(int));
short* textureIds    = malloc(textureCnt * sizeof(short));
short* spriteIds     = malloc(spriteCnt * sizeof(short));

for (i = 0; i < texelsCnt; i++)    texelOffsets[i]    = intAtNext(data, &pos);
for (i = 0; i < bitShapeCnt; i++)  bitShapeOffsets[i]  = intAtNext(data, &pos);
for (i = 0; i < textureCnt; i++)   textureIds[i]       = shortAtNext(data, &pos);
for (i = 0; i < spriteCnt; i++)    spriteIds[i]        = shortAtNext(data, &pos);
```

## Source Reference

- **Loading:** `Render_loadMappings()` in `src/Render.c`
- **Storage:** `Render_t.mediaTexelOffsets[]`, `Render_t.mediaBitShapeOffsets[]`, `Render_t.mediaTexturesIds[]`, `Render_t.mediaSpriteIds[]` in `src/Render.h`
