# DoomRPG Sine Table Format (sintable.bin)

This document describes the binary format of `sintable.bin` used in DoomRPG.

## Overview

The sine table file contains 256 precomputed sine values stored as 32-bit integers. These are used for fast trigonometric calculations during rendering (raycasting, sprite positioning, etc.) without needing floating-point math.

## Byte Reading Functions

See [BSP_FORMAT.md](BSP_FORMAT.md) for the shared byte reading functions used across all binary formats.

---

## File Structure

```
+---------------------------+
| Sine Values (256 x int)   |
+---------------------------+
```

**Total File Size: 1024 bytes**

---

## Format Details

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x000 | 4 | int32 (LE) | sinTable[0] |
| 0x004 | 4 | int32 (LE) | sinTable[1] |
| ... | ... | ... | ... |
| 0x3FC | 4 | int32 (LE) | sinTable[255] |

- **Entry count:** 256 (fixed)
- **Entry size:** 4 bytes each (signed 32-bit integer, little-endian)
- **Total size:** 256 * 4 = 1024 bytes

## Usage

The table is indexed by angle (0-255, representing 0-360 degrees). The sine values are stored as fixed-point integers. Cosine is derived by offsetting the index by 64 (90 degrees).

## Loading (Pseudocode)

```c
byte* data = fileOpenRead("/sintable.bin");
memcpy(sinTable, data, 256 * sizeof(int));
for (int i = 0; i < 256; i++) {
    sinTable[i] = swapLE32(sinTable[i]);
}
```

## Source Reference

- **Loading:** `Render_startup()` in `src/Render.c`
- **Storage:** `Render_t.sinTable[256]` in `src/Render.h`
