# BSP Compiler Notes

Algorithm reference for the BSP tree builder in [tools/bsp-to-bin.py](../../tools/bsp-to-bin.py). For the authoring-side view (what an editor produces as input) see [LEVEL_AUTHORING.md](LEVEL_AUTHORING.md); for the binary section layout see [LEVEL_FORMAT.md](LEVEL_FORMAT.md).

## 1. Purpose of the BSP

The engine renders via painter's algorithm. For every frame it walks the BSP tree front-to-back (populating `nodeIdxs[]`) and then draws leaves in reverse order (back-to-front). Correct rendering requires that **adjacent leaves at a split boundary either (a) don't share open airspace or (b) are separated by a wall in one of the leaves**. Split planes without walls between the two halves cause the far half's floor / ceiling to bleed through the near half's walls.

The BSP is also used by `traceWorld` ([src/engine/render/RenderBSP.cpp:502](../../src/engine/render/RenderBSP.cpp#L502)) to localise line queries for collision / LOS — but that's O(N) in a leaf regardless, and gameplay works fine on a 32×32 grid even with a flat BSP. Visual correctness is what actually drives the split strategy.

## 2. Algorithm — tile-centric crossings-minimisation

Implemented at [tools/bsp-to-bin.py:297](../../tools/bsp-to-bin.py#L297). Operates on the **set of open tiles** within a region, not on polys.

```
build_bsp(open_tiles, c0, r0, c1, r1, depth, nodes):
    # c0/r0 inclusive, c1/r1 exclusive — tile indices
    tile_set = open_tiles ∩ region

    # Leaf termination
    if (c1 - c0 ≤ 1 and r1 - r0 ≤ 1) or not tile_set:
        return leaf with tile_set

    # Pick the best split
    preferred_axis = 'x' if depth % 2 == 0 else 'y'
    best = None

    for axis in (preferred_axis, other_axis):
        for each candidate split-line k (tile boundary inside region):
            if one side has no open tiles:
                skip
            crossings = count of open-open tile pairs straddling k
            dist_from_mid = |k - region_midpoint|
            score = (crossings, dist_from_mid)    # lexicographic
            keep the best (axis, k) on this axis
        if preferred axis produced a candidate, commit it and stop

    if no valid split found:
        return leaf with tile_set

    recurse on left [c0..k), right [k..c1)
    return internal node (axis, split_coord = k * TILE_SIZE)
```

Key properties:

- **Splits fall on tile boundaries only.** A split at `col = k` means the boundary between tile columns `k-1` and `k`.
- **Crossings** = number of rows `r` where both `(k-1, r)` and `(k, r)` are open — these are the places where the split has no wall, and the painter's-algorithm rule above is violated. Minimising crossings is the whole point.
- **Preferred-axis commitment.** If the preferred axis yields any valid split we commit to it, even if the other axis would score lower. This produces the alternating pattern that keeps the tree balanced; falling through to the other axis only happens when the preferred axis is degenerate (e.g., a 1-wide region).
- **Balance-minimisation is the tiebreaker**, not the primary objective. Two splits with the same crossing count are broken by proximity to the region midpoint.

### Result scale

For a dense D1-style map (~400 open tiles) the algorithm produces ~250 leaves and ~500 total nodes. For a sparse hand-authored map (e.g. 33 open tiles in the two-room test) it produces 33 leaves / 65 total nodes — one leaf per tile. Both shapes are within the 256-leaf engine cap as long as open-tile counts stay under ~256.

## 3. Poly / line attachment

After `build_bsp` returns, `attach_polys_lines_to_leaves` ([tools/bsp-to-bin.py:~400](../../tools/bsp-to-bin.py)) walks the tree and, for every leaf, scans polys / lines and keeps those whose `owner_tile` is in the leaf's `tile_set`. Ownership is assigned at geometry-generation time:

- Floor + ceiling quads of open tile `(c, r)` → owner `(c, r)`.
- Wall quad between solid tile and open neighbour `(nc, nr)` → owner `(nc, nr)` (the open side).
- Collision line for that wall → owner `(nc, nr)`.
- Ceiling step wall between open tiles with mismatched ceilings → owner = **tile with the lower ceiling** (the side from which the step is visible).
- Floor step wall → owner = **tile with the lower floor**.

See [LEVEL_AUTHORING.md §2.1](LEVEL_AUTHORING.md) for the geometry-emit rules.

## 4. Engine format — node encoding

| Field | Leaf | Internal |
|---|---|---|
| `nodeOffsets[n]` (u16) | `0xFFFF` (marker) | `splitCoord_world * 16` (render space) |
| `nodeNormalIdxs[n]` (u8) | `0` | `0` for X-split, `1` for Y-split |
| `nodeChildOffset1[n]` (u16) | Byte offset into `nodePolys[]` | Index of left / near child |
| `nodeChildOffset2[n]` (u16) | `(lineStart & 0x3FF) \| ((lineCount & 0x3F) << 10)` | Index of right / far child |
| `nodeBounds[n]` (4× u8) | `(min_x, min_y, max_x, max_y)` in byte space (clamped 0..255) | same |

**Normals array is always exactly two entries:** `(-16384, 0, 0)` for X-axis splits, `(0, -16384, 0)` for Y. The engine's classify-point equation with these normals reduces to:

```
result = (-viewCoord * 16384 >> 14) + offset
       = -viewCoord + splitCoord_render
result ≥ 0  → viewCoord ≤ splitCoord  → child1 (near / left / lower)
```

So `child1` is the lower-coordinate half of the split.

## 5. Geometry format — poly flags

Four-vertex quads, flag byte encoded as:

| Bits | Field | Values used |
|---|---|---|
| 0-2 | `numVerts - 2` | `2` for a quad |
| 3-4 | axis hint | `AXIS_NONE = 0x18` for all emitted polys |
| 5 | `UV_DELTAX` | unset |
| 6 | `SWAPXY` | **set (`0x40`) for walls, cleared for floors / ceilings / step walls' matrix-compatible faces** |
| 7 | unused | unset |

Concrete emitted values:

| Poly kind | Flag byte |
|---|---|
| Floor quad | `0x1a` |
| Ceiling quad | `0x1a` |
| Wall quad (solid ↔ open) | `0x5a` |
| Ceiling-step or floor-step wall | `0x5a` |

> **`0x20` is NOT a wall flag.** Earlier versions of the Python converter defined `POLY_FLAG_WALL_TEXTURE = 0x20` and used it instead of `SWAPXY`. That produces stretched / wrongly-oriented wall textures and does not match any real D2 map. Don't reintroduce it.

## 6. Hard limits

| Constraint | Limit | Source |
|---|---|---|
| Total BSP leaves | 256 | visible-leaf cap in `Render.cpp` |
| Polys per mesh (texture group within a leaf) | 127 | 7-bit count in mesh header |
| Collision lines per leaf | 63 | 6-bit count in `nodeChildOffset2` |
| `lineStart` | 1023 | 10-bit field in `nodeChildOffset2` |
| Total `dataSizePolys` | 65535 bytes | u16 header field |
| Tile grid | 32×32 | engine-wide |
| Byte coord range | 0..255 | u8 vertex / heightMap fields |

The reference converter asserts leaves, line count, lineStart, and dataSizePolys in `convert_bsp_to_bin`. An editor should pre-check open-tile count before compiling to give authors useful feedback.

## 7. Doors

Door tiles are **open** tiles whose three non-passage neighbours are solid. They're marked by a single entry in `bsp.lines` with a texture in `0..10` whose midpoint falls inside the tile — `is_door_tile` ([tools/bsp-to-bin.py:171](../../tools/bsp-to-bin.py#L171)) detects it. Consequences for the compiler:

- Collision lines generated for walls *adjacent to* door tiles get `flags = 4` (passable) instead of `0`.
- Wall quads around the door tile are emitted normally; the door sprite (registered in `level.yaml` `entities:`) renders over them when closed, revealing them when open.
- The tile's solid bit in `mapFlags` is cleared so `traceWorld` treats the tile as walkable — already handled by the converter (it scans `bsp.lines` and clears the nibble for each door's tile).

## 8. History — what was wrong before

For the record (so no-one re-introduces these):

1. **`src/editor/BSPCompiler.cpp`** — original C++ compiler used `>> 7` for coords instead of `>> 3`, giving geometry 16× too small, and wrote more polys than the mesh header declared. Replaced by the Python converter.
2. **Midpoint-of-bounds splits** — the old `build_bsp` split at the bounds midpoint, not at tile boundaries, and had no crossings heuristic. On sparse maps this reliably collapsed to a single leaf, producing see-through walls.
3. **`POLY_FLAG_WALL_TEXTURE = 0x20`** — wrong bit; walls need `SWAPXY = 0x40` (final flag `0x5a`).
4. **Wall "extension" of ±1 byte past each endpoint** — added to paper over corner gaps caused by walls being placed in the wrong leaves. Unnecessary with correct BSP + correct ownership; stretches wall textures.
5. **UV span 0..1** — 64 texels per tile, visibly low-res. Real D2 maps use 0..16 = 1024 texels per tile.
6. **Inverted wall `t` coordinate** — had `t=0` at the ceiling and `t=16` at the floor, flipping wall textures vertically.
7. **Singleton-leaf fallback** — proposed as a workaround for the broken BSP; kept the idea of "ship a trivial 1-leaf tree and sort polys at draw time" in the back pocket but never needed once the real algorithm was implemented.
