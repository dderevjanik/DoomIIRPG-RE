# D2RPG Media File Formats (Textures, Palettes, Mappings)

This document describes the binary format of the D2RPG media files that define textures, palettes, and sprite/tile mappings, based on reverse engineering of the game code.

## Overview

The media system uses three file types:

| File | Description |
|------|-------------|
| `newMappings.bin` | Maps tile numbers to image indices, stores dimensions, bounds, palette refs, and texel sizes |
| `newPalettes.bin` | Color palettes for all images (16-bit RGB565 colors) |
| `newTexels000.bin` - `newTexels038.bin` | Pixel data for all images (8-bit indexed into palettes) |

**Location**: `Doom2rpg.app/Packages/`

## Constants

| Constant | Value | Description |
|----------|-------|-------------|
| MEDIA_MAX_IMAGES | 1024 | Maximum number of images/sprites |
| MEDIA_MAX_MAPPINGS | 512 | Maximum number of mapping entries |

## Data Types

All multi-byte values are stored in **little-endian** format.

| Type | Size | Description |
|------|------|-------------|
| `ubyte` | 1 | Unsigned 8-bit integer |
| `short` | 2 | Signed 16-bit integer |
| `ushort` | 2 | Unsigned 16-bit integer |
| `int` | 4 | Signed 32-bit integer |
| `marker` | 4 | Section delimiter (0xDEADBEEF or 0xCAFEBABE) |

---

## newMappings.bin

Contains all metadata needed to locate and render images. Read sequentially as five contiguous arrays:

### File Structure

```
short[512]     mediaMappings      Tile-to-image mapping indices
ubyte[1024]    mediaDimensions    Image dimensions (packed)
short[4096]    mediaBounds        Image bounding boxes (4 shorts per image)
int[1024]      mediaPalColors     Palette color counts / reference flags
int[1024]      mediaTexelSizes    Texel data sizes / reference flags
```

**Total**: 512×2 + 1024×1 + 4096×2 + 1024×4 + 1024×4 = 19,456 bytes

### mediaMappings (short[512])

Maps tile numbers to a range of image indices. For tile number `n`, its images span from `mediaMappings[n]` to `mediaMappings[n+1]` (exclusive). Each image in the range has its own palette and texel data.

### mediaDimensions (ubyte[1024])

Packed width/height for each image:

| Bits | Mask | Field | Description |
|------|------|-------|-------------|
| 0-3 | 0x0F | width | Image width (encoded) |
| 4-7 | 0xF0 | height | Image height (encoded) |

### mediaBounds (short[4096])

Bounding box data for each image, stored as 4 consecutive shorts per image (1024 × 4 = 4096 entries). Used for sprite collision and rendering offsets.

### mediaPalColors (int[1024])

Palette metadata for each image. The lower 30 bits encode the number of colors in the palette. Flag bits:

| Bit | Mask | Flag | Description |
|-----|------|------|-------------|
| 31 | 0x80000000 | MEDIA_FLAG_REFERENCE | This entry references another image's palette |
| 30 | 0x40000000 | MEDIA_PALETTE_REGISTERED | Palette is needed by the current map |

When `MEDIA_FLAG_REFERENCE` is set, the lower 10 bits (`& 0x3FF`) contain the index of the source image whose palette to share. After loading, references are resolved to `0xC0000000 | loadedIndex`.

### mediaTexelSizes (int[1024])

Texel data metadata for each image. The lower 30 bits encode the texel data size minus 1 (actual size = `(value & 0x3FFFFFFF) + 1`). Flag bits:

| Bit | Mask | Flag | Description |
|-----|------|------|-------------|
| 31 | 0x80000000 | MEDIA_FLAG_REFERENCE | This entry references another image's texel data |
| 30 | 0x40000000 | MEDIA_TEXELS_REGISTERED | Texel data is needed by the current map |

Reference resolution works the same as for palettes.

---

## newPalettes.bin

Contains palette data for all 1024 images sequentially. Each image's palette is followed by a 4-byte marker.

### Per-Image Palette

```
ushort[colorCount]    colors     RGB565 color values
marker                           Section delimiter (0xDEADBEEF)
```

- `colorCount` comes from `mediaPalColors[i] & 0x3FFFFFFF` in the mappings file
- Colors are 16-bit RGB565 format
- Each palette is terminated by a 4-byte marker
- Images that share palettes (via `MEDIA_FLAG_REFERENCE`) do not have their own palette entry in this file — the data is read sequentially and skipped for unreferenced entries

### Loading Behavior

Only palettes marked with `MEDIA_PALETTE_REGISTERED` (used by the current map's media references) are loaded into memory. Non-registered palettes are skipped over. Up to 16 brightness levels are generated from each base palette for fog/lighting effects.

---

## newTexels000.bin - newTexels038.bin

Texel (pixel) data is split across 39 files. Each file is up to 256 KB (`0x40000` bytes) of data. Images are stored sequentially across files — when the accumulated offset exceeds 256 KB, loading advances to the next file.

### Per-Image Texel Data

```
ubyte[texelSize]    pixels     8-bit palette-indexed pixel data
marker                         Section delimiter (0xDEADBEEF)
```

- `texelSize` comes from `(mediaTexelSizes[i] & 0x3FFFFFFF) + 1` in the mappings file
- Each pixel is an index into the image's color palette
- Each texel block is terminated by a 4-byte marker
- Only images marked with `MEDIA_TEXELS_REGISTERED` are loaded; others are skipped

### File Splitting

The texel data for all 1024 images is logically contiguous. The file boundary occurs when the running byte offset (data + 4-byte markers) exceeds `0x40000` (262,144 bytes), at which point the next image's data starts in the following file.

---

## Rendering Pipeline

1. A tile number is looked up in `mediaMappings` to get a range of image indices
2. Each image index provides:
   - **Dimensions** from `mediaDimensions` (width × height)
   - **Bounds** from `mediaBounds` (4 shorts for bounding box)
   - **Palette** from `mediaPalettes` (resolved via `mediaPalColors`)
   - **Texels** from `mediaTexels` (resolved via `mediaTexelSizes`)
3. The texel data (8-bit indices) is rendered using the palette (16-bit RGB565 colors)
4. Fog/lighting is applied via brightness-adjusted palette variants (16 levels)

## Media Registration

Not all 1024 images are loaded for every map. During map loading:

1. The map's media section lists which tile numbers are used
2. `RegisterMedia()` marks the palette and texel entries for those tiles
3. `FinalizeMedia()` loads only registered palettes and texels from disk

## Source References

This format was reverse-engineered from:
- `src/Render.cpp` — `beginLoadMap()`, `RegisterMedia()`, `FinalizeMedia()`
- `src/Render.h` — `MEDIA_*` constants and media array declarations
- `src/Resource.h` — Resource file name constants
