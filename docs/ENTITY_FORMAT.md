# Doom II RPG Entity Definition File Format (entities.bin)

This document describes the binary format of the Doom II RPG entity definition file based on reverse engineering of the game code.

## Overview

- **File Name**: `entities.bin`
- **Location**: `Doom2rpg.app/Packages/`
- **Purpose**: Defines all entity types in the game (monsters, NPCs, items, decorations, etc.)
- **Max Definitions**: 200 (`MAX_ENTITY_DEFS`)

Each entry maps a sprite tile index to an entity type, subtype, and string references for display names.

## Data Types

All multi-byte values are stored in **little-endian** format.

| Type | Size | Description |
|------|------|-------------|
| `ubyte` | 1 | Unsigned 8-bit integer |
| `short` | 2 | Signed 16-bit integer |
| `ushort` | 2 | Unsigned 16-bit integer |

## File Structure

### Header (2 bytes)

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | ushort | numDefs | Number of entity definitions |

### Entity Definition Array (8 bytes per entry)

Immediately follows the header. Repeated `numDefs` times:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | short | tileIndex | Sprite tile index that visually identifies this entity |
| 2 | ubyte | eType | Entity type (see Entity Types below) |
| 3 | ubyte | eSubType | Subtype variant within the type |
| 4 | ubyte | parm | Additional parameter (e.g. monster difficulty variant) |
| 5 | ubyte | name | String index for short name |
| 6 | ubyte | longName | String index for long/display name |
| 7 | ubyte | description | String index for full description |

**Total file size**: 2 + (numDefs × 8) bytes

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
| 10 | ET_ATTACK_INTERACTIVE | Destructible objects (crates, barrels) |
| 11 | ET_MONSTERBLOCK_ITEM | Monster-blocking items |
| 12 | ET_SPRITEWALL | Wall sprites |
| 13 | ET_NONOBSTRUCTING_SPRITEWALL | Non-solid wall sprites |
| 14 | ET_DECOR_NOCLIP | Non-collision decorations |

## Monster Subtypes

When `eType == 2` (ET_MONSTER), the `eSubType` field identifies the monster:

| Value | Constant | Monster |
|-------|----------|---------|
| 0 | MONSTER_ZOMBIE | Zombie |
| 2 | MONSTER_LOST_SOUL | Lost Soul |
| 3 | MONSTER_IMP | Imp |
| 4 | MONSTER_SAW_GOBLIN | Saw Goblin |
| 5 | MONSTER_PINKY | Pinky |
| 6 | MONSTER_CACODEMON | Cacodemon |
| 7 | MONSTER_SENTINEL | Sentinel |
| 8 | MONSTER_MANCUBUS | Mancubus |
| 9 | MONSTER_REVENANT | Revenant |
| 10 | MONSTER_ARCH_VILE | Arch-Vile |
| 11 | MONSTER_SENTRY_BOT | Sentry Bot |
| 12 | BOSS_CYBERDEMON | Boss: Cyberdemon |
| 13 | BOSS_PINKY | Boss: Pinky |
| 14 | BOSS_MASTERMIND | Boss: Mastermind |
| 15 | BOSS_VIOS | Boss: VIOS |
| 16 | BOSS_VIOS2 | Boss: VIOS (variant 2) |

The `parm` field differentiates difficulty variants of the same monster (e.g. Zombie, Zombie2, Zombie3).

## String Linking

The `name`, `longName`, and `description` fields are indices into the game's string tables (`strings00.bin`, `strings01.bin`, `strings02.bin`). These are used to display entity names and descriptions in the UI. The string index is masked with `0x3FF` (10 bits) when performing lookups.

## Lookup Methods

The game provides two ways to find entity definitions at runtime:

- **By tile index**: `EntityDefManager::lookup(tileIndex)` — used when loading map sprites to find their entity definition
- **By type/subtype**: `EntityDefManager::find(eType, eSubType, parm)` — used to look up specific entity types programmatically. When `parm` is -1, it matches any parameter value.

Both perform a linear scan over the definitions array.

## Source References

This format was reverse-engineered from:
- `src/EntityDef.cpp` — `startup()` parsing logic, `find()` and `lookup()` methods
- `src/EntityDef.h` — `EntityDef` class and `EntityDefManager` interface
- `src/Enums.h` — Entity type, monster subtype, and tile number constants
