# D2RPG Table File Format (tables.bin)

This document describes the binary format of the D2RPG resource table file based on reverse engineering of the game code.

## Overview

- **File Name**: `tables.bin`
- **Location**: `Doom2rpg.app/Packages/`
- **Purpose**: Contains all game data tables (combat stats, weapon info, monster data, sine tables, sky textures, etc.) packed into a single indexed binary file.
- **Total Tables**: 20

## Data Types

All multi-byte values are stored in **little-endian** format.

| Type | Size | Description |
|------|------|-------------|
| `byte` | 1 | Signed 8-bit integer |
| `ubyte` | 1 | Unsigned 8-bit integer |
| `short` | 2 | Signed 16-bit integer |
| `ushort` | 2 | Unsigned 16-bit integer |
| `int` | 4 | Signed 32-bit integer |

## File Structure

### Header (80 bytes)

The header contains 20 end-offset values, one per table:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | int[20] | tableOffsets | End byte offset (relative to data start) for each table |

### Table Data

Immediately after the 80-byte header, all table data follows contiguously. Each table consists of:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | int | size | Element count (for byte tables: byte count; for short/int tables: element count) |
| 4 | varies | data | Table data (size × element width bytes) |

### Offset Addressing

Table boundaries are derived from the header offsets:

- **Table 0** starts at byte 0 (after the header), ends at `tableOffsets[0]`
- **Table N** (N > 0) starts at `tableOffsets[N-1]`, ends at `tableOffsets[N]`

The data size for computing element counts (before reading the 4-byte size prefix) is:
- Table 0: `tableOffsets[0] - 4`
- Table N: `tableOffsets[N] - 4 - tableOffsets[N-1]`

## Table Index

| Index | Constant | Element Type | Description |
|-------|----------|-------------|-------------|
| 0 | TBL_COMBAT_MONSTERATTACKS | short | Monster attack definitions |
| 1 | TBL_COMBAT_WEAPONINFO | byte | Weapon info parameters |
| 2 | TBL_COMBAT_WEAPONDATA | byte | Weapon data values |
| 3 | TBL_COMBAT_MONSTERSTATS | byte | Monster stat values |
| 4 | TBL_COMBAT_COMBATMASK | int | Combat bitmask data |
| 5 | TBL_CANVAS_KEYSNUMERIC | byte | Numeric key mappings |
| 6 | TBL_ENUMS_OSC_CYCLE | byte | Oscillation cycle data |
| 7 | TBL_GAME_LEVELNAMES | short | Level name string IDs |
| 8 | (monster colors) | byte | Particle system monster colors |
| 9 | TBL_RENDER_SINETABLE | int | Sine lookup table |
| 10 | TBL_ENERGY_DRINK_DATA | short | Vending machine energy drink stats |
| 11 | TBL_MONSTER_WEAKNESS | byte | Monster weakness values |
| 12 | (movie effects) | int | Movie/cutscene effect data |
| 13 | (monster sounds) | byte | Monster sound indices |
| 14 | (camera keys) | short | Table camera keyframe data |
| 15 | (camera tweens) | byte | Table camera tween data |
| 16 | TBL_SKYMAP1_PALETTE | ushort | Sky map 1 color palette |
| 17 | TBL_SKYMAP1_TEXELS | ubyte | Sky map 1 texture pixels |
| 18 | TBL_SKYMAP2_PALETTE | ushort | Sky map 2 color palette |
| 19 | TBL_SKYMAP2_TEXELS | ubyte | Sky map 2 texture pixels |

## Loading Behavior

Tables are loaded in two phases:

1. **Startup** (`initTableLoading`): Reads the 80-byte header to cache all 20 offsets.
2. **Data loading** (`beginTableLoading` / `finishTableLoading`): Reopens the file and reads table data sequentially. Tables must be loaded in ascending index order — seeking backwards is not supported.

### Loading Groups

- **Tables 0-13**: Loaded at startup in `Applet::loadTables()`
- **Tables 14-15**: Camera data, loaded on demand via `Game::loadTableCamera(14, 15)`
- **Tables 16-19**: Sky map data, loaded during map initialization. Sky map selection is based on the current map: `skyIndex = ((mapNameID - 1) / 5 % 2) * 2`, then palette = `skyIndex + 16`, texels = `skyIndex + 17`

## Source References

This format was reverse-engineered from:
- `src/Resource.cpp` — `initTableLoading()`, `seekTable()`, `loadByteTable()`, `loadShortTable()`, `loadIntTable()`
- `src/Resource.h` — Resource file name constants
- `src/App.cpp` — `loadTables()` table allocation and loading
- `src/Render.cpp` — Sky map table loading
- `src/Game.cpp` — Camera table loading
- `src/Enums.h` — `TBL_*` table index constants
