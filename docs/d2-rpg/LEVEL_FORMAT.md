# Doom II RPG Level File Format (.bin)

This document describes the binary format of Doom II RPG level files based on reverse engineering of the game code.

## Overview

- **File Names**: `map00.bin` through `map09.bin`
- **Location**: `Doom2rpg.app/Packages/`
- **Map Size**: 32x32 tiles (1024 tiles total)
- **Tile Size**: 64 world units per tile

Each level file contains all geometry, sprites, entities, scripting bytecode, and camera animation data for a single map.

## Data Types

All multi-byte values are stored in **little-endian** format.

| Type | Size | Description |
|------|------|-------------|
| `byte` | 1 | Signed 8-bit integer |
| `ubyte` | 1 | Unsigned 8-bit integer |
| `short` | 2 | Signed 16-bit integer |
| `ushort` | 2 | Unsigned 16-bit integer |
| `int` | 4 | Signed 32-bit integer |
| `coord` | 1 | Compressed coordinate (value × 8) |
| `marker` | 4 | Section delimiter (0xDEADBEEF or 0xCAFEBABE) |

## File Structure

### Header (42 bytes)

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | ubyte | version | Must equal 3 |
| 1 | int | mapCompileDate | Compilation timestamp |
| 5 | ushort | mapSpawnIndex | Player spawn tile index (0-1023), col=index%32, row=index/32 |
| 7 | ubyte | mapSpawnDir | Player spawn direction (0-7), angle=dir*128. See [Angle System](#angle-system) |
| 8 | byte | mapFlagsBitmask | Map-wide flags |
| 9 | byte | totalSecrets | Number of secrets in level |
| 10 | ubyte | totalLoot | Number of loot items |
| 11 | ushort | numNodes | Number of BSP nodes |
| 13 | ushort | dataSizePolys | Size of polygon data in bytes |
| 15 | ushort | numLines | Number of line segments |
| 17 | ushort | numNormals | Number of surface normals |
| 19 | ushort | numNormalSprites | Number of normal sprites |
| 21 | short | numZSprites | Number of Z-sprites (height-varied) |
| 23 | short | numTileEvents | Number of tile event triggers |
| 25 | short | mapByteCodeSize | Size of script bytecode |
| 27 | byte | totalMayaCameras | Number of Maya camera animations |
| 28 | short | totalMayaCameraKeys | Total camera keyframes |
| 30 | short[6] | mayaTweenOffsets | Camera tween data offsets (-1 if unused) |

**Total**: 42 bytes

### Section Markers

Sections are delimited by 4-byte markers:
- `0xDEADBEEF` - Primary section delimiter
- `0xCAFEBABE` - Alternative section delimiter

### Data Sections

After the header, data sections follow in this order:

---

#### 1. Media Section

```
[MARKER: 0xDEADBEEF]
ushort    mediaCount          Number of media references
ushort[]  mediaIndices        Array of texture/sprite indices (mediaCount entries)
[MARKER: 0xDEADBEEF]
```

---

#### 2. Normals

```
short[numNormals * 3]  normals    Surface normal vectors (X, Y, Z triplets)
[MARKER: 0xCAFEBABE]
```

---

#### 3. BSP Node Offsets

```
short[numNodes]  nodeOffsets    Offset into polygon data for each node
[MARKER: 0xCAFEBABE]
```

---

#### 4. Node Normal Indices

```
ubyte[numNodes]  nodeNormalIdxs    Index into normals array for each node
[MARKER: 0xCAFEBABE]
```

---

#### 5. Node Child Offsets

```
short[numNodes]  nodeChildOffset1    First child node offset
short[numNodes]  nodeChildOffset2    Second child node offset
[MARKER: 0xCAFEBABE]
```

---

#### 6. Node Bounds

```
ubyte[numNodes * 4]  nodeBounds    Bounding box data (4 bytes per node)
[MARKER: 0xCAFEBABE]
```

---

#### 7. Node Polygons

```
ubyte[dataSizePolys]  nodePolys    Polygon reference data
[MARKER: 0xCAFEBABE]
```

---

#### 8. Line Data

```
ubyte[(numLines+1)/2]  lineFlags    Line properties (packed as nibbles)
ubyte[numLines * 2]    lineXs       Line X coordinates
ubyte[numLines * 2]    lineYs       Line Y coordinates
[MARKER: 0xCAFEBABE]
```

---

#### 9. Height Map

```
ubyte[1024]  heightMap    Height value for each tile (32x32 grid)
[MARKER: 0xCAFEBABE]
```

Indexed as `heightMap[row * 32 + col]`. The stored value must be shifted left by 3 to get the actual world height: `worldZ = heightMap[index] << 3`. See [Coordinate System](#coordinate-system) for details.

---

#### 10. Sprite Coordinates

Uses compressed coordinate format (each byte × 8 = world units):

```
coord[numMapSprites]  spriteX    X positions
coord[numMapSprites]  spriteY    Y positions
```

---

#### 11. Sprite Info (Lower 8 bits)

```
ubyte[numMapSprites]  spriteInfoLow    Tile number / sprite index
[MARKER: 0xCAFEBABE]
```

---

#### 12. Sprite Info (Upper 16 bits)

```
ushort[numMapSprites]  spriteInfoHigh    Render flags (shifted left 16)
[MARKER: 0xCAFEBABE]
```

---

#### 13. Z-Sprite Heights

```
ubyte[numZSprites]  spriteZ    Z-position for height-varied sprites
[MARKER: 0xCAFEBABE]
```

---

#### 14. Sprite Info (Middle 8 bits)

For Z-sprites only:

```
ubyte[numZSprites]  spriteInfoMid    Additional flags (shifted left 8)
[MARKER: 0xCAFEBABE]
```

---

#### 15. Static Functions

```
ushort[12]  staticFuncs    Static function entry points
[MARKER: 0xCAFEBABE]
```

12 level-wide script entry points (e.g., map init, boss health thresholds, per-turn logic). Value `0xFFFF` means not defined. See [SCRIPTING.md — Static Functions](SCRIPTING.md#triggering-static-functions) for the full table.

---

#### 16. Tile Events

```
int[numTileEvents * 2]  tileEvents    Tile event data (2 ints per event)
[MARKER: 0xCAFEBABE]
```

Tile event format:
- First int: Tile index in lower 10 bits (& 0x3FF), bytecode IP in upper 16 bits (>> 16)
- Second int: Event flags — trigger type, direction, attack type, disable/block flags

See [SCRIPTING.md — Tile Events](SCRIPTING.md#triggering-tile-events) for the complete flag bit layout and matching logic.

---

#### 17. Map Bytecode

```
ubyte[mapByteCodeSize]  mapByteCode    Script VM bytecode
[MARKER: 0xCAFEBABE]
```

Big-endian bytecode for the stack-based script VM. See [SCRIPTING.md](SCRIPTING.md) for the instruction format, complete opcode reference, and VM architecture.

---

#### 18. Maya Cameras

For each camera (totalMayaCameras iterations):

```
ubyte   numKeys        Number of keyframes
short   sampleRate     Animation sample rate

short[numKeys * 7]     Camera keyframe data (7 values per key)
short[numKeys * 6]     Tween indices
short[6]               Tween counts per axis

[MARKER: 0xDEADBEEF]
byte[sum of tweenCounts]  Tween data
```

---

#### 19. Map Flags

```
ubyte[512]  mapFlags    Navigation/collision flags (2 tiles per byte)
[MARKER: 0xCAFEBABE]
```

Each byte contains flags for 2 tiles:
- Lower nibble (bits 0-3): First tile flags
- Upper nibble (bits 4-7): Second tile flags

## Sprite Info Bitfield

The `mapSpriteInfo` array contains 32-bit values with the following bit layout:

| Bits | Mask | Description |
|------|------|-------------|
| 0-7 | 0x000000FF | Tile number (sprite index) |
| 8-15 | 0x0000FF00 | Frame/animation data |
| 16 | 0x00010000 | Invisible flag |
| 17 | 0x00020000 | Flip horizontal |
| 20 | 0x00100000 | Animation flag |
| 21 | 0x00200000 | Non-entity flag |
| 22 | 0x00400000 | Extended tile number (+257) |
| 23 | 0x00800000 | Sprite wall flag |
| 24-27 | 0x0F000000 | Direction flags |

Direction flags in bits 24-27:
- Bit 24 (0x01000000): South facing
- Bit 25 (0x02000000): North facing
- Bit 26 (0x04000000): East facing
- Bit 27 (0x08000000): West facing

## Entity Types

| Value | Constant | Description |
|-------|----------|-------------|
| 0 | ET_WORLD | World geometry |
| 1 | ET_PLAYER | Player spawn point |
| 2 | ET_MONSTER | Enemy creatures |
| 3 | ET_NPC | Non-player characters |
| 4 | ET_PLAYERCLIP | Player collision |
| 5 | ET_DOOR | Doors |
| 6 | ET_ITEM | Pickups/items |
| 7 | ET_DECOR | Decorative objects |
| 8 | ET_ENV_DAMAGE | Environmental hazards |
| 9 | ET_CORPSE | Dead bodies |
| 10 | ET_ATTACK_INTERACTIVE | Destructible objects |
| 11 | ET_MONSTERBLOCK_ITEM | Monster-blocking items |
| 12 | ET_SPRITEWALL | Wall sprites |
| 13 | ET_NONOBSTRUCTING_SPRITEWALL | Non-solid wall sprites |
| 14 | ET_DECOR_NOCLIP | Non-collision decorations |

## Tile Number Ranges

Tile numbers identify sprite types in the game:

| Range | Category | Examples |
|-------|----------|----------|
| 1-14 | View Weapons | Assault Rifle, Chainsaw, BFG, etc. |
| 15 | World Weapon | Generic weapon pickup |
| 18-63 | Monsters | Zombie, Imp, Cacodemon, Bosses, etc. |
| 65-80 | NPCs | Scientists, Civilians, etc. |
| 81-118 | Items | Ammo, Keys, Armor, Health, etc. |
| 119+ | Decorations | Hell Seal, Toilet, Ladder, etc. |

### Monster Tile Numbers

| Tile | Monster |
|------|---------|
| 18 | Red Sentry Bot |
| 19 | Sentry Bot |
| 20-22 | Zombie (variants) |
| 23-25 | Imp (variants) |
| 26-28 | Saw Goblin (variants) |
| 29-31 | Lost Soul (variants) |
| 32-34 | Pinky (variants) |
| 35-37 | Revenant (variants) |
| 38-40 | Mancubus (variants) |
| 41-43 | Cacodemon (variants) |
| 44-46 | Sentinel (variants) |
| 50-52 | Arch-Vile (variants) |
| 53 | Arachnotron |
| 54 | Boss: Cyberdemon |
| 56 | Boss: Pinky |
| 57 | Boss: Mastermind |
| 58-62 | Boss: VIOS (variants) |

### Item Tile Numbers

| Tile | Item |
|------|------|
| 85 | Bullets |
| 86 | Shells |
| 88 | Rockets |
| 89 | Cells |
| 90 | Holy Water |
| 107 | UAC Credit |
| 110 | Red Key |
| 111 | Blue Key |
| 112 | Armor Jacket |
| 113 | Satchel |
| 116 | Health Pack |
| 117 | Food Plate |

## Map Flag Bits

Per-tile navigation flags (4 bits per tile):

| Bit | Description |
|-----|-------------|
| 0 | Solid/blocking |
| 6 | Has tile event (0x40) |

## Coordinate System

The game uses multiple coordinate scales. Understanding how they relate is critical for correctly positioning the camera and interpreting map data.

### Tile Coordinates

Tiles are addressed by column (X) and row (Y) in a 32x32 grid (indices 0-31).

- **Tile index** (0-1023): `index = row * 32 + col`
- **Tile to column/row**: `col = index % 32`, `row = index / 32`

### World Coordinates (Canvas Scale)

Each tile is 64 world units wide. The center of a tile is at an offset of +32:

- `worldX = col * 64 + 32` (tile center)
- `worldY = row * 64 + 32` (tile center)

This is the coordinate system used by `mapFlags`, `heightMap`, `getHeight()`, and sprite positions.

### Render Coordinates

The renderer (`Render::render()`) expects coordinates at a 16x higher scale with a +8 offset:

- `renderX = (worldX << 4) + 8`
- `renderY = (worldY << 4) + 8`
- `renderZ = (worldZ << 4) + 8`

This scaling is applied by the game in `Canvas.cpp` before calling `render()`. The full range for a 32-tile map is 0 to 32×64×16 = 32768 render units.

### Height Map

The `heightMap` array stores one byte per tile (1024 entries). The actual world height is obtained by shifting left by 3:

- `worldZ = heightMap[tileIndex] << 3`

Use `Render::getHeight(worldX, worldY)` which performs the lookup and shift internally.

### Player Spawn

The `mapSpawnIndex` (0-1023) and `mapSpawnDir` (0-7) fields in the header define the player start:

- **Position**: Convert spawn index to tile col/row, then to world coordinates (center of tile), then use `getHeight()` to get the tile's floor height. Player eye height is approximately +36 world units above the floor.
- **Direction**: `mapSpawnDir` maps to engine angles: `engineAngle = spawnDir * 128` (in a 0-1023 angle system where 256 = 90°). Direction values: 0=East, 1=NE, 2=North, 3=NW, 4=West, 5=SW, 6=South, 7=SE.

### Angle System

The engine uses a 10-bit angle system (0-1023):

| Angle | Direction | Degrees |
|-------|-----------|---------|
| 0 | East (+X) | 0° |
| 256 | North | 90° |
| 512 | West (-X) | 180° |
| 768 | South | 270° |

Conversion: `degrees = angle * (360.0 / 1024.0)`, `angle = degrees * (1024.0 / 360.0)`

## Constants

| Constant | Value | Description |
|----------|-------|-------------|
| MAP_SIZE | 32 | Tiles per side |
| TILE_SIZE | 64 | World units per tile |
| MAX_ENTITIES | 275 | Maximum entities per map |
| MAX_MONSTERS | 80 | Maximum monsters per map |
| MAX_LADDERS_PER_MAP | 10 | Maximum ladders |
| MAX_CUSTOM_SPRITES | 48 | Custom sprite slots |
| MAX_DROP_SPRITES | 16 | Dropped item sprite slots |

## Source References

This format was reverse-engineered from:
- `src/Render.cpp` - `beginLoadMap()` function
- `src/Game.cpp` - `loadMapEntities()` and `loadMayaCameras()` functions
- `src/Resource.cpp` - Binary reading utilities
- `src/Enums.h` - Entity types and tile number constants
