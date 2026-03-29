# DoomRPG Sprite Texels Format (stexels.bin)

This document describes the binary format of `stexels.bin` used in DoomRPG.

## Overview

The sprite texels file contains pixel data for sprites and objects (monsters, items, weapons, decorations, etc.). Unlike wall textures which are fixed 64x64, sprite textures are variable-sized — their dimensions are determined by the corresponding shape data in `bitshapes.bin`. Pixels are stored as 4-bit palette-indexed values (two pixels per byte).

## Byte Reading Functions

See [BSP_FORMAT.md](BSP_FORMAT.md) for the shared byte reading functions used across all binary formats.

---

## File Structure

```
+---------------------------+
| Data Size (int)           |
+---------------------------+
| Sprite Texel Data         |
| (variable-size sprites)   |
+---------------------------+
```

---

## Format Details

### Header

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0x00 | 4 | int32 (LE) | dataSize | Total data size in half-words (shorts) |

### Sprite Texel Data

Starting at offset 0x04, sprite pixel data is stored sequentially. Each sprite's size varies based on its shape (defined in `bitshapes.bin`).

| Field | Description |
|-------|-------------|
| Pixel format | 4-bit indexed (same as wtexels.bin) |
| Sprite size | Variable — determined by bitshape bounds |
| Bytes per sprite | `getSTexelBufferSize() / 2` |

### Pixel Packing

Same as wall texels — each byte contains two 4-bit palette indices:

```
Byte: [high nibble][low nibble]
       pixel[1]     pixel[0]

Low nibble:  byte & 0x0F        -> palette index for first pixel
High nibble: (byte >> 4) & 0x0F -> palette index for second pixel
```

---

## Sprite Size Calculation

The number of texel bytes for a sprite is calculated from its decoded shape data:

```c
int getSTexelBufferSize(int spriteId) {
    int offset = mediaBitShapeOffsets[spriteId * 2];
    int minX = shapeData[offset + 2];
    int maxX = shapeData[offset + 3];
    int buffSize = 0;

    for (int col = minX; col <= maxX; col++) {
        int colPtr = offset + shapeData[offset + 4 + (col - minX)] + 2;
        // Sum all run lengths in this column
        while ((shapeData[colPtr] & 0xFF) != 0x7F) {
            buffSize += shapeData[colPtr] >> 8;  // run length
            colPtr += 2;
        }
    }
    return (buffSize + 1) & ~1;  // round up to even
}
```

---

## Sprite Addressing

Sprite texel offsets are stored within the decoded `shapeData[]` (from `bitshapes.bin`). The first two shorts of each shape record are overwritten with the texel offset during loading:

```
shapeData[offset + 0] = texelOffset & 0xFFFF        (low 16 bits)
shapeData[offset + 1] = (texelOffset >> 16) & 0xFFFF (high 16 bits)
```

To retrieve the offset:
```c
int texelOffset = (shapeData[offset] & 0xFFFF) | (shapeData[offset + 1] << 16);
```

---

## Relationship to bitshapes.bin

The bitshapes define *which* pixels are visible (the sprite mask), and the stexels provide *what color* those pixels are. Only visible pixels (as determined by the shape's RLE runs) have corresponding texel data stored.

```
bitshapes.bin:  Shape mask  -> column runs [(yStart, length), ...]
stexels.bin:    Pixel data  -> sequential color indices for visible pixels only
palettes.bin:   Color table -> resolves indices to RGB565 colors
```

---

## Loading Process

1. Open `stexels.bin` and read the 4-byte header
2. Sort the map's required sprites by their file offset (ascending)
3. For each required sprite:
   - Seek forward to the sprite's texel offset
   - Read `bufferSize / 2` bytes of pixel data
   - Store in the shared `mediaTexels[]` buffer (appended after wall texels)
   - Update `shapeData[offset + 0..1]` to point to the new in-memory location

### Loading (Pseudocode)

```c
byte* data = fileOpenRead("/stexels.bin");
int dataSize = intAt(data, 0);
data += 4;  // skip header

// Sort mapSpriteTexels by offset (ascending)
bubbleSort(mapSpriteTexels, by: getSTexelOffsets(id));

int memPos = currentTexelPos;  // continues after wall texels
int streamPos = dataSize * 2;

for (int i = 0; i < mapSpriteTexelsCount; i++) {
    int shapeOffset = mediaBitShapeOffsets[mapSpriteTexels[i] * 2];
    int fileOffset = (shapeData[shapeOffset] & 0xFFFF) | (shapeData[shapeOffset + 1] << 16);
    int bufSize = spriteBufferSizes[i];

    // Skip to offset
    if (fileOffset > streamPos) {
        data += (fileOffset - streamPos) / 2;
        streamPos += (fileOffset - streamPos);
    }

    int readSize = bufSize / 2;
    memcpy(&mediaTexels[memPos], data, readSize);
    data += readSize;
    streamPos += readSize * 2;

    // Update shape data to point to in-memory texels
    shapeData[shapeOffset + 0] = (memPos * 2) & 0xFFFF;
    shapeData[shapeOffset + 1] = ((memPos * 2) >> 16) & 0xFFFF;
    memPos += readSize;
}
```

## Source Reference

- **Loading:** `Render_loadTexels()` in `src/Render.c` (sprite texel section)
- **Buffer size:** `Render_getSTexelBufferSize()` in `src/Render.c`
- **Offset access:** `Render_getSTexelOffsets()` in `src/Render.c`
- **Storage:** `Render_t.mediaTexels[]` (shared with wall texels) in `src/Render.h`
- **Shape data from:** `Render_t.shapeData[]` (loaded from `bitshapes.bin`)
