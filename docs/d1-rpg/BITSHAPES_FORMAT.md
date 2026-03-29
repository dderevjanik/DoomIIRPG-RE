# DoomRPG BitShapes Format (bitshapes.bin)

This document describes the binary format of `bitshapes.bin` used in DoomRPG.

## Overview

The bitshapes file contains sprite shape/mask data. Each shape defines the outline of a sprite using a column-based bitmask format. The engine uses this data to determine which pixels of a sprite are visible (non-transparent) and to build run-length encoded scanline data for efficient rendering.

## Byte Reading Functions

See [BSP_FORMAT.md](BSP_FORMAT.md) for the shared byte reading functions used across all binary formats.

---

## File Structure

```
+---------------------------+
| Data Size Header (int)    |
+---------------------------+
| Shape Record 0            |
+---------------------------+
| Shape Record 1            |
+---------------------------+
| ...                       |
+---------------------------+
| Shape Record N            |
+---------------------------+
```

---

## Format Details

### File Header

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x00 | 4 | int32 (LE) | Data size header (skipped during parsing) |

### Shape Record (Raw on-disk format)

Each shape record starts at the byte offset specified by `mediaBitShapeOffsets[spriteId * 2]` (from `mappings.bin`), relative to byte 4 of the file.

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| +0x00 | 2 | int16 (LE) | originX | X coordinate origin (texel offset, stored as low + high byte) |
| +0x02 | 2 | int16 (LE) | originY | Y coordinate origin (texel offset, stored as low + high byte) |
| +0x04 | 2 | int16 (LE) | field1 | Reserved / unknown |
| +0x06 | 2 | int16 (LE) | shapeDataSize | Total size of decoded shape data (in shorts) |
| +0x08 | 1 | byte | minX | Minimum X bound |
| +0x09 | 1 | byte | maxX | Maximum X bound |
| +0x0A | 1 | byte | minY | Minimum Y bound |
| +0x0B | 1 | byte | maxY | Maximum Y bound |
| +0x0C | N | byte[] | bitmask | Column bitmask data |

### Derived Values

```
width  = (maxX - minX) + 1
height = (maxY - minY) + 1
pitch  = (height % 8 != 0) ? (height / 8 + 1) : (height / 8)
```

### Bitmask Layout

Starting at offset +0x0C, the bitmask data is organized by columns:

```
For each column w (0 to width-1):
    bitmask bytes at: offset + 0x0C + (w * pitch)
    Each byte contains 8 rows of visibility data
    Bit N of byte B represents row (B * 8 + N)
    Bit set = pixel visible, Bit clear = pixel transparent
```

---

## Decoded Shape Data (In-memory format)

After loading, the engine decodes the bitmask into run-length encoded (RLE) shape data stored as an array of shorts:

```
+------------------------------------------+
| originX (short)                          |  [offset + 0]
+------------------------------------------+
| originY (short)                          |  [offset + 1]
+------------------------------------------+
| minX (short)                             |  [offset + 2]
+------------------------------------------+
| maxX (short)                             |  [offset + 3]
+------------------------------------------+
| Column Pointer 0 (short)                 |  [offset + 4]
| Column Pointer 1 (short)                 |  [offset + 5]
| ...                                      |
| Column Pointer (width-1) (short)         |  [offset + 4 + width - 1]
+------------------------------------------+
| Column 0 Scanline Runs                   |
+------------------------------------------+
| Column 1 Scanline Runs                   |
+------------------------------------------+
| ...                                      |
+------------------------------------------+
```

### Column Pointers

Each column pointer is a relative offset (in shorts) from `(shapeOffset + 2)` to the start of that column's scanline run data.

### Scanline Runs

For each column, the scanline data consists of one or more run entries followed by a terminator:

| Size | Type | Description |
|------|------|-------------|
| 2 | short | `(yStart & 0xFF) \| (runLength << 8)` — Y start position and run length packed together |
| 2 | short | Texel data offset — index into the sprite's pixel data |
| ... | ... | Additional runs... |
| 2 | short | `0x007F` — Column terminator (value 127 in low byte) |

Where:
- **yStart** = `(value & 0xFF)` — Absolute Y position where this run starts (includes minY offset)
- **runLength** = `(value >> 8)` — Number of consecutive visible pixels in this run

---

## Decoding Algorithm (Pseudocode)

```c
// For each sprite shape:
ioOffset = mediaBitShapeOffsets[spriteId * 2]; // from mappings.bin

width  = ioBuffer[ioOffset + 9] - ioBuffer[ioOffset + 8] + 1;
height = ioBuffer[ioOffset + 11] - ioBuffer[ioOffset + 10] + 1;
pitch  = (height & 7) ? (height / 8 + 1) : (height / 8);

// Write header
shapeData[pos++] = originX;  // bytes [0..1]
shapeData[pos++] = originY;  // bytes [2..3]
shapeData[pos++] = minX;     // byte [8]
shapeData[pos++] = maxX;     // byte [9]

// Reserve space for column pointers
int columnsStart = pos;
pos += width;

int texelIndex = 0;
for (w = 0; w < width; w++) {
    // Store column pointer (relative offset)
    shapeData[columnsStart + w] = (pos - shapeOffset - 2);

    bitmaskOffset = ioOffset + 12 + (w * pitch);
    h = 0;
    while (h < height) {
        // Skip transparent pixels
        while (h < height && (ioBuffer[bitmaskOffset + h/8] & (1 << (h & 7))) == 0)
            h++;
        if (h == height) break;

        int runStart = h;
        int texelStart = texelIndex;

        // Count visible pixels
        while (h < height && (ioBuffer[bitmaskOffset + h/8] & (1 << (h & 7))) != 0) {
            texelIndex++;
            h++;
        }

        // Write run: yStart | (runLength << 8)
        shapeData[pos++] = ((runStart + minY) & 0xFF) | ((h - runStart) << 8);
        shapeData[pos++] = texelStart;
    }
    shapeData[pos++] = 0x7F; // Column terminator
}
```

## Source Reference

- **Loading:** `Render_loadBitShapes()` in `src/Render.c`
- **Buffer size calc:** `Render_getSTexelBufferSize()` in `src/Render.c`
- **Storage:** `Render_t.shapeData[]` in `src/Render.h`
- **Offsets from:** `Render_t.mediaBitShapeOffsets[]` (loaded from `mappings.bin`)
