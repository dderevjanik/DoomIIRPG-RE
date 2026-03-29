# DoomRPG Palettes Format (palettes.bin)

This document describes the binary format of `palettes.bin` used in DoomRPG.

## Overview

The palettes file contains indexed color data used for rendering textures and sprites. Colors are stored in RGB565 format (16-bit). Both wall textures and sprite textures reference palette entries to determine their final pixel colors.

## Byte Reading Functions

See [BSP_FORMAT.md](BSP_FORMAT.md) for the shared byte reading functions used across all binary formats.

---

## File Structure

```
+---------------------------+
| Data Size (int)           |
+---------------------------+
| Color Entries (N x short) |
+---------------------------+
```

---

## Format Details

### Header

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0x00 | 4 | int32 (LE) | dataSize | Total size of color data in bytes |

### Color Entries

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0x04 | 2 | int16 (LE) | color[0] | First color entry (RGB565) |
| 0x06 | 2 | int16 (LE) | color[1] | Second color entry (RGB565) |
| ... | ... | ... | ... | ... |

- **Entry count:** `dataSize / 2`
- **Entry size:** 2 bytes each (16-bit RGB565, little-endian)

---

## RGB565 Color Format

Each 16-bit color entry is packed as:

```
Bits:  15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
       [--- Blue (5) ---] [--- Green (6) ---] [- Red (5) -]
```

### Bit Layout

| Bits | Width | Component |
|------|-------|-----------|
| 15-11 | 5 bits | Blue (0-31) |
| 10-5 | 6 bits | Green (0-63) |
| 4-0 | 5 bits | Red (0-31) |

### Expansion to 8-bit RGB

The 5/6-bit components are expanded to full 8-bit range:

```c
blue  = (color >> 11) & 0x1F;
blue  = (blue << 3) | (blue >> 2);    // 5-bit -> 8-bit

green = (color >> 5) & 0x3F;
green = (green << 2) | (green >> 4);  // 6-bit -> 8-bit

red   = (color & 0x1F);
red   = (red << 3) | (red >> 2);      // 5-bit -> 8-bit
```

The expanded values are then re-packed into the engine's internal RGB565 format via `Render_make565RGB()`.

---

## Palette Organization

The palette entries are organized into groups of 16 colors. Each texture or sprite references a palette offset, and the pixel data contains 4-bit indices (0-15) into that 16-color palette group.

```
Palette Offset 0:   colors[0..15]    -> First palette group
Palette Offset 16:  colors[16..31]   -> Second palette group
...
```

## Grayscale Conversion

The engine supports converting all palettes to grayscale (e.g., for death screen effects):

```c
grayColor = (((color & 0xF800) >> 10) + ((color >> 5) & 0x3F) + ((color & 0x1F) << 1)) / 3;
```

## Loading (Pseudocode)

```c
byte* data = fileOpenRead("/palettes.bin");
int pos = 0;
int count = intAtNext(data, &pos) / 2;
short* palettes = malloc(count * sizeof(short));

for (int i = 0; i < count; i++) {
    short color = shortAtNext(data, &pos);
    // Decode RGB565 and re-encode for engine
    int blue  = ((color >> 11) & 0x1F); blue  = (blue << 3)  | (blue >> 2);
    int green = ((color >> 5)  & 0x3F); green = (green << 2) | (green >> 4);
    int red   = (color & 0x1F);         red   = (red << 3)   | (red >> 2);
    palettes[i] = make565RGB(blue, green, red);
}
```

## Source Reference

- **Loading:** `Render_loadPalettes()` in `src/Render.c`
- **Grayscale:** `Render_setGrayPalettes()` in `src/Render.c`
- **Storage:** `Render_t.mediaPalettes[]`, `Render_t.mediaPalettesLength` in `src/Render.h`
