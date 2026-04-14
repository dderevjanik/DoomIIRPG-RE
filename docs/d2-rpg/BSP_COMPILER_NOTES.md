# BSP Compiler Notes

## Working Algorithm (Proven 2026-03-31)

### Geometry Generation — Verified Correct

Proven by roundtrip test: original test_map BSP tree + our generated polyData = perfect rendering.

**Floor quad** (per open tile):
- Winding: CW from above `(x0,y1) → (x1,y1) → (x1,y0) → (x0,y0)`
- Flags: `0x1a` (4 verts + AXIS_NONE, no SWAPXY)
- Z = heightMap value (e.g., 0 for flat, 7 for D1 maps at height 56)

**Ceiling quad** (per open tile):
- Winding: CCW from above `(x0,y0) → (x1,y0) → (x1,y1) → (x0,y1)`
- Flags: `0x1a` (same as floor)
- Z = heightMap + 8 (= floor + 64 world units = 1 tile height)

**Wall quad** (per solid/open tile boundary):
- Winding: top-start → top-end → bottom-end → bottom-start
- Flags: `0x5a` (4 verts + AXIS_NONE + SWAPXY)
- Z spans floor to ceiling
- **Assigned to the leaf containing the adjacent OPEN tile** (not the solid tile)

**Vertex coordinates:**
- Byte = worldCoord / 8 (engine reads `byte << 7` for render space)
- Tile boundary at col C = `C * 8` bytes
- Example: col 1 boundary = 8, col 16 = 128

**Mesh ordering within leaf:**
1. Floor meshes first (max 127 polys each)
2. Ceiling meshes next
3. Wall meshes last (draws on top)

**Mesh header:** `(tileNumber << 7) | (polyCount & 0x7F)`. Max 127 polys per mesh — split into multiple meshes if more.

### BSP Tree Generation — Working Algorithm

**Key insight:** The BSP must ensure that adjacent leaves with open tiles ALWAYS have a wall between them. If they don't, far floors/ceilings bleed through near walls (painter's algorithm failure).

**Split strategy — minimize open-tile crossings:**

For each candidate split position, count how many open tiles span the boundary (open on BOTH sides with no wall between). Choose the split with the FEWEST crossings.

```
def count_crossings_x(col, r0, r1):
    """Count open tiles that span column boundary at `col`"""
    return sum(1 for r in range(r0,r1)
               if not is_solid(col-1,r) and not is_solid(col,r))

def count_crossings_y(row, c0, c1):
    return sum(1 for c in range(c0,c1)
               if not is_solid(c,row-1) and not is_solid(c,row))
```

**Split selection:**
1. Alternate axis by depth (`depth % 2`: even=X, odd=Y)
2. Try preferred axis first, then other axis
3. For each axis, scan ALL positions in the region
4. Pick the position with fewest crossings (prefer positions near midpoint as tiebreaker)
5. Split at tile boundaries (col or row number × 64 world units)
6. If no split possible (region too small), make leaf

**Leaf termination:** `(c1-c0 <= 1 AND r1-r0 <= 1)` or no open tiles in region.

**Result for D1 intro map:** ~248 leaves, ~495 nodes, most splits have 0-2 crossings.

### BSP Node Format

**Normals:** `(-16384, 0, 0)` for X-axis, `(0, -16384, 0)` for Y-axis

**Node offset for split at tile boundary T:**
- `offset = T * 64 * 16` (tile → world → render space)
- Engine classify: `(-viewCoord * 16384 >> 14) + offset = -viewCoord + offset`
- Result ≥ 0 → child1 (left/near side), < 0 → child2 (right/far side)

**Leaf node:**
- `nodeOffset = 0xFFFF` (leaf marker)
- `child1 = polyData byte offset` (uint16)
- `child2 = (lineStart & 0x3FF) | ((lineCount & 0x3F) << 10)` (uint16)

**Bounds:** `(min_col*8, min_row*8, max_col*8, max_row*8)` clamped to 0-255

### Engine Constraints

| Constraint | Limit | Source |
|-----------|-------|--------|
| Max visible leaves | 256 | `Render.cpp:2050` |
| Max polys per mesh | 127 | 7-bit polyCount in packed header |
| Max lines per leaf | 63 | 6-bit lineCount |
| Max line start index | 1023 | 10-bit lineStart |
| Max polyData size | 65535 | uint16 in header |
| Coordinate byte | worldCoord / 8 | Engine reads byte << 7 |
| POLY_FLAG_WALL_TEXTURE | DO NOT USE | Test_map doesn't use it (0x20) |
| POLY_FLAG_SWAPXY | Walls only | Flags = 0x5a for walls, 0x1a for floors/ceilings |

### Doors

Doors are handled via **`level.yaml` entities** + **passable collision lines**:

**Door detection:** D1 lines with texture 0-10 are doors. For each door line, find the midpoint tile `(mx//64, my//64)` and the direction (horizontal = N/S, vertical = E/W).

**Door entities in `level.yaml`:**
```yaml
entities:
  - x: 1632         # D1 world coord (midpoint of door line)
    y: 1504
    tile: door_unlocked
    flags: [animation, north, south]   # or [animation, east, west]
```
- Horizontal D1 lines → `north, south` direction flags
- Vertical D1 lines → `east, west` direction flags

**Wall polys:** Keep ALL walls including door tiles — do NOT exclude door wall polys. The door sprite renders on top of the wall texture when closed. When opened, the wall poly remains visible behind the opened door (acceptable visual).

**Collision lines:** Door tile walls get line type 4 (passable in `traceWorld`). All other walls get type 0 (blocking). This is set per-tile: if an open tile `(c,r)` is a door tile, ALL its adjacent walls are passable.

**Door media:** Register tile 276 (`door_unlocked`) in the binary's media section.

**`scripts.yaml`:** Must be empty (`static_funcs: {}`, `tile_events: []`) to prevent D2 tile event scripts from executing on D1 map data.

**`level.yaml` textures:** Must include `door_unlocked` and `sky_box`:
```yaml
textures:
  - texture_258
  - texture_451
  - texture_462
  - door_unlocked
  - sky_box
```

### What Was Wrong in Previous Attempts

1. **BSPCompiler.cpp** — Used `>> 7` instead of `>> 3` for coordinates (16× too small). Also had broken `packPolygons` that wrote more polys than the mesh header declared (>127 overflow).

2. **Grid-based splits** — Splitting at regular intervals (every 2 or 4 tiles) puts far floors and near walls in different leaves, causing painter's algorithm failures.

3. **Single-leaf approach** — Works for small rooms (4×4) but fails for large maps because the engine has no depth ordering within a single leaf.

4. **1×1 tile leaves** — Exceeds 256 visible node limit, causing geometry to be silently dropped.

5. **Wall Z values** — Originally used wrong Z range. Correct: floor Z=0, ceiling Z=8 (for height 0 maps). Wall height = 8 byte units = 64 world units.

6. **Door wall exclusion by tile** — Excluding ALL walls for a door tile removed side walls, exposing skybox. Fix: keep all wall polys, let the door sprite render on top.

7. **Door edge matching** — D1 door lines are placed at tile midpoints (not tile boundaries), so they can't be matched to tile-boundary wall quads by coordinate. Match by tile + direction instead, or just keep all walls.

### Verification Method

**Roundtrip test:** Extract tile grid from a known-working D2 .bin → regenerate geometry → inject into original BSP tree → render. If it matches the original, geometry is correct. Used with `10_test_map/map.bin`.

**Incremental tests:**
1. Single 4×4 room (1 leaf) — verifies geometry format
2. Split 4×4 room (2 leaves) — verifies BSP math
3. Split 4×4 room (4 leaves) — verifies multi-level BSP
4. Full D1 map with smart splits — verifies at scale
5. Full D1 map with doors — verifies door rendering and passability
