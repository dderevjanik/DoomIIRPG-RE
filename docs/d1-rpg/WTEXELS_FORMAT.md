# DoomRPG Wall Texels Format (wtexels.bin)

This document describes the binary format of `wtexels.bin` used in DoomRPG.

## Overview

The wall texels file contains pixel data for wall and environment textures. Each texture is 64x64 pixels stored as 4-bit palette-indexed values (two pixels per byte). The textures are used for rendering walls, doors, and other vertical surfaces in the game world.

## Byte Reading Functions

See [BSP_FORMAT.md](BSP_FORMAT.md) for the shared byte reading functions used across all binary formats.

---

## File Structure

```
+---------------------------+
| Data Size (int)           |
+---------------------------+
| Texture Data              |
| (concatenated textures)   |
+---------------------------+
```

---

## Format Details

### Header

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0x00 | 4 | int32 (LE) | dataSize | Total data size in half-words (shorts) |

### Texture Data

Starting at offset 0x04, texture pixel data is stored sequentially. Each texture occupies 2048 bytes (64 * 64 / 2, since pixels are packed as 4-bit nibbles).

| Field | Value | Description |
|-------|-------|-------------|
| Texture width | 64 pixels | Fixed |
| Texture height | 64 pixels | Fixed |
| Bytes per texture | 2048 | 4096 pixels / 2 (4-bit packed) |
| Pixel format | 4-bit indexed | Each byte holds 2 pixel indices |

### Pixel Packing

Each byte contains two 4-bit palette indices:

```
Byte: [high nibble][low nibble]
       pixel[1]     pixel[0]

Low nibble:  byte & 0x0F        -> palette index for first pixel
High nibble: (byte >> 4) & 0x0F -> palette index for second pixel
```

### Pixel-to-Color Resolution

To resolve a pixel to its final RGB565 color:

```c
paletteOffset = mediaTexelOffsets[textureId * 2 + 1];  // from mappings.bin
texelOffset   = mediaTexelOffsets[textureId * 2 + 0];  // from mappings.bin

// For pixel pair at position j (even index):
byte texelByte = mediaTexels[texelOffset / 2 + j / 2];
color0 = mediaPalettes[paletteOffset + (texelByte & 0x0F)];
color1 = mediaPalettes[paletteOffset + ((texelByte >> 4) & 0x0F)];
```

---

## Texture Addressing

Textures are not necessarily stored in sequential order. The byte offset of each texture within the file is specified by `mediaTexelOffsets[]` (loaded from `mappings.bin`):

```
mediaTexelOffsets[textureId * 2 + 0] = byte offset into wtexels.bin data (after header)
mediaTexelOffsets[textureId * 2 + 1] = palette offset into mediaPalettes[]
```

Only textures referenced by the current map are loaded into memory. During loading, textures are sorted by their file offset to enable sequential reading.

---

## Loading Process

1. Open `wtexels.bin` and read the 4-byte header
2. Sort the map's required textures by their file offset (ascending)
3. For each required texture:
   - Seek forward to the texture's offset (skip unused data)
   - Read 2048 bytes of pixel data
   - Store in the shared `mediaTexels[]` buffer
   - Update `mediaTexelOffsets[]` to point to the new in-memory location

### Loading (Pseudocode)

```c
byte* data = fileOpenRead("/wtexels.bin");
int dataSize = intAt(data, 0);
data += 4;  // skip header

// Sort mapTextureTexels by offset (ascending)
bubbleSort(mapTextureTexels, by: mediaTexelOffsets[id * 2]);

int memPos = 0;
int streamPos = 0;

for (int i = 0; i < mapTextureTexelsCount; i++) {
    int texId = mapTextureTexels[i] * 2;
    int fileOffset = mediaTexelOffsets[texId];

    // Skip to offset
    if (fileOffset > streamPos) {
        data += (fileOffset - streamPos) / 2;
        streamPos = fileOffset;
    }

    // Read 2048 bytes (64x64 texture, 4-bit packed)
    memcpy(&mediaTexels[memPos], data, 2048);
    data += 2048;
    streamPos += 4096;

    // Update offset to point to in-memory location
    mediaTexelOffsets[texId] = memPos * 2;
    memPos += 2048;
}
```

## Source Reference

- **Loading:** `Render_loadTexels()` in `src/Render.c` (wall texel section)
- **Color resolve:** `Render_finishLoadMap()` in `src/Render.c`
- **Storage:** `Render_t.mediaTexels[]` (shared with sprite texels) in `src/Render.h`
- **Offsets from:** `Render_t.mediaTexelOffsets[]` (loaded from `mappings.bin`)
