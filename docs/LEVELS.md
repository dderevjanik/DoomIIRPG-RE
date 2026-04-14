# Level Structure

This document describes how levels are organized, configured, and loaded by the engine.

For the binary `.bin` file format, see [LEVEL_FORMAT.md](LEVEL_FORMAT.md).
For the scripting/bytecode system, see [SCRIPTING.md](SCRIPTING.md).

---

## Directory Layout

Each level lives in its own subdirectory under the game's `levels/` folder:

```
games/doom2rpg/
  game.yaml                  # Game definition — registers levels, strings, search paths
  levels.yaml                # Per-level gameplay config (loadouts, fog, joke items)
  sprites.yaml               # Global sprite/tile name -> ID mapping
  entities.yaml              # Entity definitions (monsters, NPCs, items, decorations)
  sounds.yaml                # Sound name -> WAV file mapping
  levels/
    textures/                 # Shared texture data (palettes, texels, mappings)
      newMappings.bin         #   Tile ID -> palette/texel range mapping table
      newPalettes.bin         #   16-color palettes for all tiles
      newTexels000.bin        #   Texel (pixel) data, split across multiple files
      ...
      newTexels038.bin
    01_tycho/                 # Level 1: Tycho
      level.yaml              #   Level configuration (spawn, sprites, cameras, heightmap)
      map.bin                 #   BSP geometry, lines, normals, polygons
      model.bin               #   3D model geometry for this level
      scripts.yaml            #   Script data (events, functions, disassembly)
      strings.yaml            #   Dialog and story text for this level
    02_cichus/                # Level 2: Cichus
      ...
    ...
    10_test_map/              # Level 10: Test Map
```

---

## Registration in game.yaml

Levels are registered in `game.yaml` under the `levels:` section. The key is the `map_id` (1-based integer), the value is the directory path:

```yaml
game:
  levels:
    1: levels/01_tycho
    2: levels/02_cichus
    3: levels/03_kepler
    ...
```

The engine derives file paths from the directory:

| File | Path | Purpose |
|------|------|---------|
| Map geometry | `{dir}/map.bin` | BSP tree, polygons, lines, height map |
| 3D models | `{dir}/model.bin` | Level model geometry |
| Level config | `{dir}/level.yaml` | Spawn point, sprites, cameras, media |
| Scripts | `{dir}/scripts.yaml` | Tile events, static functions, bytecode |
| Strings | `{dir}/strings.yaml` | Dialog text (registered separately in `strings:`) |

String files are separately registered in `game.yaml` under `strings:` with a group index:

```yaml
game:
  strings:
    4: levels/01_tycho/strings.yaml    # group 4 = level 1 strings
    5: levels/02_cichus/strings.yaml   # group 5 = level 2 strings
    ...
```

---

## levels.yaml — Gameplay Configuration

The top-level `levels.yaml` configures per-level gameplay settings. Each entry is keyed by level name with a `map_id` and `file` field:

```yaml
levels:
  tycho:
    map_id: 1
    file: map00.bin
    joke_items: [180, 181, 182, 183, 184]

  cichus:
    map_id: 2
    file: map01.bin
    fog: false                        # Disable fog (outdoor map)
    joke_items: [149, 150, 151, 152, 153]
    starting_loadout:                 # Equipment when starting this level
      weapons: [chainsaw, assault_rifle]
      armor: 33
      inventory:
        health_pack: 23
      ammo:
        bullets: 100
      xp: 638
      stats:
        defense: 4
        strength: 2
      vios_mallocs: 0
```

| Field | Type | Description |
|-------|------|-------------|
| `map_id` | int | Engine map number (1-based) |
| `file` | string | Binary map filename (e.g. `map00.bin`) |
| `fog` | bool | Enable/disable fog (default: true) |
| `joke_items` | int[] | Entity indices for joke items dropped by corpses |
| `starting_loadout` | map | Player equipment when entering from a previous level |

The engine reads this file at startup (`Main.cpp`) to populate fog settings, joke items, and starting loadouts.

---

## level.yaml — Level Configuration

Each level's `level.yaml` contains map-specific overrides and data that can be edited without modifying the binary `.bin` file:

```yaml
name: tycho
map_id: 1
joke_items: [180, 181, 182, 183, 184]

player_spawn:
  x: 4               # Tile column (0-31)
  y: 19              # Tile row (0-31)
  dir: east           # Facing: east, south, west, north

total_secrets: 2
total_loot: 60

textures:        # Textures/sprites this level needs loaded
  - assault_rifle
  - monster_zombie
  - door_unlocked
  - texture_303       # Wall/floor textures (numeric fallback names)
  ...

cameras:              # Maya camera animations (cutscenes)
  - sample_rate: 125
    keys:
      - {x: 1124, y: 98, z: 948, pitch: 996, yaw: 657, roll: 0, ms: 999}
      ...

height_map:           # 32x32 grid of tile heights
  - [0, 0, 76, 72, ...]
  ...

sprites:              # All sprites placed in the level
  - x: 96
    y: 1056
    tile: obj_fire
  - x: 160
    y: 992
    tile: one_uac_credit
  - x: 288
    y: 1376
    tile: npc_major
    flags: [animation, east]
  ...
```

### Key sections

**player_spawn** — Overrides the spawn point from the binary header. Coordinates are tile grid positions (0-31).

**textures** — The texture/sprite manifest. Lists every tile that must be loaded into texture memory for this map. Entries can be sprite names (from `sprites.yaml`) or numeric tile IDs. The engine's `RegisterMedia()` function marks each tile's palette and texel data for loading; `FinalizeMedia()` then loads only the marked data. This keeps memory usage within the original game's budget.

**height_map** — 32x32 grid of tile heights (0-255). Each value represents the floor height at that tile position. Zero means no floor (void/wall). The world height is `value << 3`.

**entities** — All entity placements in the level. Each entry has:

| Field | Type | Description |
|-------|------|-------------|
| `x`, `y` | int | World position (tile * 64 + 32 for center) |
| `tile` | string/int | Sprite name from `sprites.yaml` or numeric tile ID |
| `z` | int | Height offset (only for Z-sprites) |
| `flags` | string[] | Optional flags: `invisible`, `flip_h`, `animation`, `non_entity`, `sprite_wall`, `south`, `north`, `east`, `west` |

Entity indices in bytecode scripts refer to the position in this array (0-based, normal sprites first, then Z-sprites).

**cameras** — Maya camera animations used for cutscenes. Each camera defines keyframes with position, rotation, and timing, plus tween data for smooth interpolation.

---

## scripts.yaml — Level Scripts

Contains the event-driven scripting that controls gameplay logic — cutscenes, dialogs, puzzles, entity behavior, item drops, and dynamic events.

```yaml
static_funcs:
  init_map: 0           # Runs when map loads
  per_turn: 253         # Runs each game turn

tile_events:
  - tile: {x: 18, y: 3}
    ip: 1796            # Bytecode instruction pointer
    on: [trigger]       # Trigger types: enter, exit, trigger, face
    dir: [east, north]  # Direction filter
    flags: [disable]    # Optional: block_input, exit_goto, skip_turn, disable

disassembly: |          # Human-readable bytecode disassembly
  ; --- init_map ---
  0000  SET_FOG_COLOR          0xFF800000
  0005  EVAL                   [var[17], 0, EQ] else goto @30
  0012  CALL                   @46
  0015  NAMEENTITY             85 (level_door_unlocked) 3 "To Gehenna"
  ...
```

### static_funcs

Level-wide entry points triggered by game events. Value is the bytecode instruction pointer.

| Name | Trigger |
|------|---------|
| `init_map` | Map loads |
| `per_turn` | Each game turn |
| `boss_75` / `boss_50` / `boss_25` | Boss health thresholds |
| `boss_dead` | Boss killed |
| `attack_npc` | Player attacks NPC |
| `monster_death` | Any monster dies |
| `item_pickup` | Item picked up |

### tile_events

Script triggers bound to specific map tiles. Fields:

| Field | Description |
|-------|-------------|
| `tile: {x, y}` | Grid position (0-31) |
| `ip` | Bytecode instruction pointer to start executing |
| `on` | Trigger types: `enter` (walk onto), `exit` (walk off), `trigger` (use/interact), `face` (look at) |
| `dir` | Direction filter (optional, defaults to all) |
| `attack` | Combat filter: `melee`, `ranged`, `explosion` |
| `flags` | `block_input`, `skip_turn`, `disable` (starts disabled, toggled by scripts) |

### disassembly

Human-readable disassembly of the level's bytecode. Generated by the converter or `tools/disasm_scripts.py`. All sprite, string, sound, and entity references are resolved to names. The engine does not read this field — it loads bytecodes from `map.bin`.

For the full opcode reference, see [SCRIPTING.md](SCRIPTING.md).

---

## strings.yaml — Level Dialog

Each level's dialog and story text, organized by language:

```yaml
english:
  - ""                          # index 0 (unused)
  - "Ty-cho"                   # index 1
  - "RE-MIND-ER|You can save..." # index 2
  ...
```

String indices in bytecode (e.g. `MESSAGE 42`) reference positions in this array. Special characters:

| Escape | Meaning |
|--------|---------|
| `\|` | Newline (in-game line break) |
| `-` | Soft hyphen (word-break hint, not displayed) |
| `\x85` | Ellipsis |
| `\x87` | Checkmark |
| `\xA0` | Hard space (non-breaking) |

---

## map.bin — Binary Geometry

The binary level file containing BSP geometry, polygon data, and raw sprite/event data. See [LEVEL_FORMAT.md](LEVEL_FORMAT.md) for the complete binary specification.

The engine always loads `map.bin` first. Values from `level.yaml` and `scripts.yaml` override the corresponding fields from the binary (spawn point, sprites, media indices, tile events, etc.).

---

## Texture Loading

Textures are loaded from the shared `levels/textures/` directory:

| File | Purpose |
|------|---------|
| `newMappings.bin` | Maps tile IDs to palette/texel ranges |
| `newPalettes.bin` | 16-color palettes (one per tile) |
| `newTexelsNNN.bin` | Pixel data, split across ~39 files |

### Loading sequence

1. **Startup**: Engine loads `newMappings.bin` to build the tile ID -> palette/texel mapping table
2. **Map load**: Engine reads `textures` from `level.yaml` (or from the binary)
3. **RegisterMedia**: For each media index, marks the tile's palette and texel entries as "needed"
4. **FinalizeMedia**: Loads only the marked palettes from `newPalettes.bin` and texels from the corresponding `newTexelsNNN.bin` files

This lazy-loading system ensures each map only loads the textures it uses, staying within the original game's memory constraints.

---

## Engine Load Sequence

When the player enters a new level (map ID N):

1. **Resolve paths** — Look up `levelInfos[N]` from `game.yaml` to get directory, map file, config file
2. **Load map.bin** — Parse BSP geometry, sprites, tile events, bytecodes, cameras from binary
3. **Load level.yaml** — Override spawn point, secrets, loot count, sprites, media indices
4. **Load scripts.yaml** — Override tile events and static functions (if present)
5. **Register media** — Mark needed textures from `textures`, then load them
6. **Load strings** — Load the level's string group for dialog text
7. **Load levels.yaml** — Apply fog settings and joke items for this map ID
8. **Initialize scripts** — Run `init_map` static function if defined
9. **Start gameplay** — Player spawns, `per_turn` begins firing each turn

---

## Modding

To add or modify a level:

1. **Create a directory** under `levels/` (e.g. `levels/11_my_level/`)
2. **Add required files**: `map.bin`, `level.yaml`, `scripts.yaml`, `strings.yaml`
3. **Register in game.yaml**: Add entry under `levels:` and `strings:`
4. **Add to levels.yaml**: Configure `map_id`, `file`, and optional loadout/fog settings
5. **Update media**: Ensure `textures` in `level.yaml` lists all textures the map uses

The `level.yaml` and `scripts.yaml` files can override most binary data, so modders can adjust spawn points, sprites, events, and media without recompiling the `.bin` file.

### Tools

| Tool | Purpose |
|------|---------|
| `tools/disasm_scripts.py` | Disassemble bytecodes with name resolution |
| `tools/extract_maps.py` | Extract geometry data from map binaries |
| `tools/extract_textures.py` | Extract texture data |
| `tools/extract_models.py` | Extract model geometry |
