# Level Authoring Guide

How to build new levels for the DRPG engine — a practical guide aimed at tool and map-editor authors. This document describes the **authoring model** (what the editor manipulates) and the **emit pipeline** (how that model becomes the on-disk files the engine loads). For the raw binary layout, see [LEVEL_FORMAT.md](LEVEL_FORMAT.md). For the BSP compiler algorithm, see [BSP_COMPILER_NOTES.md](BSP_COMPILER_NOTES.md).

Reference implementation: [tools/bsp-to-bin.py](../../tools/bsp-to-bin.py) + [tools/gen-two-room-level.py](../../tools/gen-two-room-level.py).

---

## 1. The authoring model

A level is fully described by a handful of per-tile tables plus a short list of global fields. The in-memory `BSPMap` class in [tools/bsp-to-bin.py](../../tools/bsp-to-bin.py) is the reference schema:

| Field | Shape | Meaning |
|---|---|---|
| `block_map` | `list[int]` length 1024 (row-major 32×32) | Tile solidity. Bit 0 = 1 means solid (no geometry generated inside), bit 0 = 0 means open (floor + ceiling + walls derived from neighbors). Higher bits are nibble-packed tile flags written verbatim into the `.bin` `mapFlags` section. |
| `tile_floor_byte` | `dict[(col,row) → int]` | Per-tile floor Z in byte space (default `FLOOR_HEIGHT = 56` → 448 world units). Engine computes player camera elevation from this. |
| `tile_ceil_byte` | `dict[(col,row) → int]` | Per-tile ceiling Z in byte space (default `FLOOR_HEIGHT + 8 = 64`). Only affects rendering; no collision. |
| `spawn_index` | `int 0..1023` | Player spawn tile, packed as `row*32 + col`. |
| `spawn_dir` | `int 0..255` | Spawn facing angle. `d2_dir = (spawn_dir // 32) & 7` → 0=east, 2=north, 4=west, 6=south. |
| `lines` | `list[(x0, y0, x1, y1, texture, flags)]` | Special lines. **For a map editor, the only use today is door markers:** any line with texture in `0..10` that sits inside an open tile marks that tile as a door. Midpoint of the line determines the tile; axis determines direction. See §4. |
| `sprites` / `tile_events` / `bytecodes` / `strings` | Reserved for future editor features; leave empty. |

Everything else — wall quads, collision lines, step walls, BSP tree, media indices, section markers — is **derived** by the pipeline below. The editor only needs to expose the six fields above.

### Tile grid coordinates

- Grid is **32×32** (indices 0–31 on each axis).
- Tile `(col, row)` spans world coords `[col*64, (col+1)*64) × [row*64, (row+1)*64)`.
- Tile centre = `(col*64+32, row*64+32)`.
- World → byte: `byte = world >> 3` (i.e., world / 8). The engine renders at `byte << 7` = `world * 16`.

### Height conventions

All Z values are **byte-space**. World Z = `byte * 8`.

- Default floor byte = `FLOOR_HEIGHT = 56` (world 448).
- Default ceiling byte = `FLOOR_HEIGHT + 8 = 64` (world 512). This is a pure convention carried over from the engine's `getHeight() + 8` pattern — the renderer doesn't enforce it.
- Tiles without an entry in `tile_floor_byte` / `tile_ceil_byte` use these defaults.
- Valid byte range is 0–255 (256 byte units = 2048 world units = 32 tiles of Z).

### What the editor must *forbid*

- Two adjacent open tiles where `ceil_byte(A) < floor_byte(B)` (or vice versa): the "room" intersects itself with negative space. Geometry emits but rendering is undefined.
- Step heights that exceed the engine's walkable-step threshold (the engine snaps the camera to `getHeight(x,y) + 36` on every tile transition; unreasonable jumps cause camera pop). A practical ceiling for a walkable step is ~2–4 bytes (16–32 world units).
- Door tiles not sandwiched between solid tiles on the non-passage axes. The door sprite is drawn in place of the tile's walls, so both passage-perpendicular neighbours must be solid for the door to look like a door.

---

## 2. Emit pipeline

The pipeline is one function call: `convert_bsp_to_bin(bsp)` in [tools/bsp-to-bin.py:407](../../tools/bsp-to-bin.py). Its phases:

### 2.1 `generate_geometry(bsp)` → `(polys, lines)`

Visits every tile in the 32×32 grid. For each tile:

- **Solid tile** — for each of its four neighbours that is *open*, emit:
  - One **wall quad** (flag `0x5a`) spanning `floor_byte(neighbor) .. ceil_byte(neighbor)` at the shared edge. Winding: floor-start → floor-end → ceil-end → ceil-start. `owner_tile = neighbor`.
  - One **collision line** at the same edge, in byte space. Flag = `4` if the neighbour is a door tile (passable), else `0` (blocking). `owner_tile = neighbor`.
- **Open tile** — emit:
  - One **floor quad** (flag `0x1a`) at `floor_byte(col,row)`. Winding: CW from above. `owner_tile = (col, row)`.
  - One **ceiling quad** (flag `0x1a`) at `ceil_byte(col,row)`. Winding: CCW from above. `owner_tile = (col, row)`.
  - **Step walls** for height mismatches with its +X and +Y neighbours (only two directions checked to avoid double-emission):
    - Ceiling step: if `ceil_byte(this) ≠ ceil_byte(neighbour)`, emit a wall quad spanning `[min, max]` Z at the shared edge. `owner_tile = tile with the LOWER ceiling` (the side from which the step is visible).
    - Floor step: same pattern, `owner_tile = tile with the LOWER floor`.

Each record carries `owner_tile` — used to place the poly/line in the correct BSP leaf.

### 2.2 `build_bsp(open_tiles, bounds, depth, nodes)` — tile-centric BSP

See [BSP_COMPILER_NOTES.md](BSP_COMPILER_NOTES.md) for full details. Summary:

- Operates on the **set of open tiles** within a region, not on polys directly.
- Splits at **tile-boundary axis-aligned planes** only.
- Scores each candidate split by **open-tile crossings** (open both sides of the boundary = 1 crossing). Minimises crossings; tiebreaks by distance from the region midpoint. Preferred axis alternates with depth.
- Terminates when bounds are 1×1 or the region has no open tiles.
- Typical output: one leaf per open tile, ~2× that many total nodes.

### 2.3 `attach_polys_lines_to_leaves(nodes, polys, lines)`

Walks the tree; for each leaf, populates `node.poly_indices` / `node.line_indices` by looking up each record's `owner_tile` in the leaf's `tile_set`. Zero geometry computation — it's just a join.

### 2.4 Serialisation

Pack per-leaf polys grouped by texture (one "mesh" per texture, 127-poly cap per mesh). Collect leaf lines into a contiguous array with per-leaf `(lineStart, lineCount)` in `nodeChildOffset2`. Write the header, normals, node arrays, poly data, line arrays, height map, sprite arrays, static funcs, tile events, bytecode, maya cameras, and map flags, each followed by a section marker (`0xCAFEBABE` or `0xDEADBEEF`). See [LEVEL_FORMAT.md](LEVEL_FORMAT.md) for exact offsets.

---

## 3. Engine format facts (ground truth)

Independent of our generator — these are the conventions the engine decoder expects.

### 3.1 Vertex format

Each poly vertex is 5 bytes: `(x, y, z, s, t)`.

| Byte | Decoding | Semantics |
|---|---|---|
| x | `uint8 << 7` | Render-space X = world × 16 |
| y | `uint8 << 7` | Render-space Y = world × 16 |
| z | `uint8 << 7` | Render-space Z = world × 16 (no heightMap offset) |
| s | `int8 << 6` | Texel U (signed) |
| t | `int8 << 6` | Texel V (signed) |

Source: [src/engine/render/RenderBSP.cpp:289-293](../../src/engine/render/RenderBSP.cpp#L289-L293).

### 3.2 UV convention

- **One full tile (8 byte units = 64 world units) = UV span 16**, i.e., s=0 at one edge, s=16 at the opposite edge. Texel span = `16 << 6 = 1024`, which the engine wraps against the texture's native dimensions.
- Floor quad: `s,t ∈ {(0,0), (16,0), (16,16), (0,16)}`.
- Ceiling quad: CCW variant of the same — `{(0,16), (16,16), (16,0), (0,0)}`.
- Wall quad: s is along the horizontal edge (0 at floor-start to 16 at floor-end); t is along Z (0 at floor to 16 at ceiling).
- Step walls: 0..16 regardless of actual Z span. Thin steps stretch the texture vertically; that's the accepted trade for simple authoring.

### 3.3 Poly flag byte

```
bit 0-2:  numVerts - 2        (3-vert → 1, 4-vert → 2, …, 9-vert → 7)
bit 3-4:  axis hint           (POLY_FLAG_AXIS_NONE = 0x18)
bit 5:    UV_DELTAX           (auxiliary, normally unset)
bit 6:    SWAPXY              (POLY_FLAG_SWAPXY = 0x40; set on walls, cleared on floor/ceil)
bit 7:    (unused)
```

Concrete values emitted by the reference pipeline:
- Floor / ceiling / step wall: `0x1a` and `0x5a` respectively (always 4-vert).
- Wall: `0x5a`.
- **`0x20` is NOT a wall flag.** The old Python code called it `POLY_FLAG_WALL_TEXTURE` and used it instead of SWAPXY; setting it produces stretched / wrongly-oriented wall textures. Don't use it.

### 3.4 Node encoding

| Field | Width | Leaf case (nodeOffsets[n] == 0xFFFF) | Internal case |
|---|---|---|---|
| `nodeOffsets` | u16 | `0xFFFF` (leaf marker) | `splitCoord_world * 16` (render-space) |
| `nodeNormalIdxs` | u8 | `0` | `0` for X-axis split, `1` for Y-axis |
| `nodeChildOffset1` | u16 | Byte offset into `nodePolys` | Left / near child index |
| `nodeChildOffset2` | u16 | `(lineStart & 0x3FF) \| ((lineCount & 0x3F) << 10)` | Right / far child index |
| `nodeBounds` | 4× u8 | `(min_x, min_y, max_x, max_y)` in byte space, clamped 0..255 |

Normals: always exactly two — `(-16384, 0, 0)` and `(0, -16384, 0)`. The classify equation is `(-viewCoord * 16384 >> 14) + offset ≥ 0 → child1`.

### 3.5 Hard limits

| Constraint | Limit | Source |
|---|---|---|
| Total BSP leaves | 256 | `Render.cpp` visible-leaf cap |
| Polys per mesh (per-texture group within a leaf) | 127 | 7-bit count in mesh header |
| Collision lines per leaf | 63 | 6-bit count in `nodeChildOffset2` |
| `lineStart` across all leaves | 1023 | 10-bit field in `nodeChildOffset2` |
| Total `dataSizePolys` | 65535 bytes | u16 header field |
| Tile grid | 32×32 | Engine-wide |
| Byte coord range | 0..255 | Vertex and heightMap byte width |

The reference generator asserts these in `convert_bsp_to_bin`. An editor should pre-check them before invoking the generator to give authors useful feedback.

---

## 4. Doors

Doors are three things at once: a passable collision line, a wall-texture hole that the door sprite is drawn over, and an animated entity in `level.yaml`.

**Authoring representation** — append one entry to `bsp.lines`:

| For passage direction | Line shape | Converter assigns |
|---|---|---|
| East/west (wall runs N-S) | Vertical: `(cx, row*64, cx, (row+1)*64, tex, 0)` where `cx = col*64 + 32` | `flags: [animation, east, west]` |
| North/south (wall runs E-W) | Horizontal: `(col*64, cy, (col+1)*64, cy, tex, 0)` where `cy = row*64 + 32` | `flags: [animation, north, south]` |

- `tex` must be in `0..10` (matches `D1_DOOR_TEXTURES` in the converter). `0` is fine; the slot is not used for texture lookup — only to flag the tile.
- The door tile itself must be **open**. Its non-passage neighbours must be **solid** for visual correctness.
- The converter's `is_door_tile` detects which tiles are doors by testing each `bsp.lines` entry's midpoint against the tile centre (±`TILE_SIZE` tolerance). Collision lines generated for walls adjacent to door tiles get `flags = 4` (passable).
- A door entity is emitted into `level.yaml`: `{x: mid_x, y: mid_y, tile: door_unlocked, flags: [animation, <dir>]}`. Mid-point direction detection uses `dx > dy ? "north, south" : "east, west"`.

---

## 5. Output files

A level is a directory with four files, registered in the per-game `game.yaml`.

### 5.1 `map.bin`

Produced by `convert_bsp_to_bin` + `open(path, 'wb')`. Binary format is fixed; see [LEVEL_FORMAT.md](LEVEL_FORMAT.md).

### 5.2 `level.yaml`

Human-written overrides and per-level data. Minimal valid contents:

```yaml
name: my_level
player_spawn:
  x: 3        # spawn tile col (redundant with map.bin spawn, but engine reads it)
  y: 4        # spawn tile row
  dir: east   # east | northeast | north | northwest | west | southwest | south | southeast

textures:
  - texture_258       # wall (D2_WALL_TILE)
  - texture_451       # floor (D2_FLOOR_TILE)
  - texture_462       # ceiling (D2_CEIL_TILE)
  - door_unlocked     # required if any doors

sky_texture: textures/sky_earth.png   # REQUIRED — renderer segfaults without one

height_map:    # 32 rows × 32 cols of per-tile floor byte values
  - [56, 56, 56, …]   # row 0
  - …
  - [56, 56, 56, …]   # row 31

entities:      # doors + any objects; MUST be `entities:`, NOT `sprites:`
  - x: 416
    y: 352
    tile: door_unlocked
    flags: [animation, east, west]
```

Key gotchas:
- Use key **`entities:`**, not `sprites:`. The bsp-to-bin `generate_level_yaml` helper emits `sprites:` — the engine ignores it. The reference `gen-two-room-level.py` writes `entities:` directly for this reason.
- `height_map` must match the per-tile `floor_byte` values. Mismatch → player camera floats above / sinks into the floor.
- `sky_texture` is effectively required. With no sky set, `gles::DrawSkyMap` dereferences a null texture pointer and segfaults.

### 5.3 `scripts.yaml`

Minimum:
```yaml
static_funcs: {}
tile_events: []
bytecode: ""
```

Non-empty scripts (tile events, bytecode) are out of scope for geometry authoring.

### 5.4 `strings.yaml`

Minimum:
```yaml
english:
  []
```

### 5.5 Registration in `game.yaml`

Append two entries under the game's `game.yaml`, typically at `games/<game>/game.yaml`:

```yaml
  levels:
    …
    11: levels/11_my_level
  strings:
    …
    15: levels/11_my_level/strings.yaml     # next free string-group index
```

### 5.6 Required engine patch for map IDs > 10

The stock `Game::GetSaveFile(FILE_NAME_BRIEFWORLD, i2)` in [src/game/core/GameSaveLoad.cpp:1135](../../src/game/core/GameSaveLoad.cpp#L1135) returned `nullptr` for `i2 > 9` (map IDs > 10), crashing in `saveLevelSnapshot`. The fix — already applied upstream — generates `SB_<N>` dynamically via `thread_local char sbBuf`. If you fork or port to another engine build, re-apply this patch or the first level load will segfault.

---

## 6. Editor-oriented summary

A map editor only needs to maintain the **six BSPMap fields** from §1 as the source-of-truth project file. Everything else is a pure function of that state:

```
Editor state ─── (your choice of on-disk format) ─┐
                                                  │
                                                  ▼
                   ┌──────────────────────┐
                   │ build BSPMap object  │
                   └──────────┬───────────┘
                              │
                              ▼
         convert_bsp_to_bin(bsp) → map.bin
         write_level_yaml(bsp)   → level.yaml
         + static scripts.yaml/strings.yaml stubs
                              │
                              ▼
              Append entry in games/<game>/game.yaml
                              │
                              ▼
              ./DRPGEngine --game <game> --map levels/<id>_<name>
```

For per-tile wall/floor/ceiling **texture** overrides (the thing currently hardcoded to `D2_WALL_TILE=258`, `D2_FLOOR_TILE=451`, `D2_CEIL_TILE=462`), the authoring model needs three more dicts — `tile_wall_tex`, `tile_floor_tex`, `tile_ceil_tex`, all keyed by `(col, row)` — and `generate_geometry` needs to read them instead of the global constants. That's the main thing not yet in the reference generator.
