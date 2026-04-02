# BSP Rendering in the Doom II RPG Engine

This document describes how the engine **renders** BSP data at runtime -- what happens after a `.bin` level file is loaded and geometry appears on screen. It is the runtime counterpart to:

- [LEVEL_FORMAT.md](LEVEL_FORMAT.md) -- binary file format (how data is stored)
- [BSP_COMPILER_NOTES.md](BSP_COMPILER_NOTES.md) -- how to build a BSP tree (compilation)

Understanding the rendering pipeline is essential for creating custom maps that render correctly.

### Source Files

| File | Role |
|------|------|
| `src/engine/Render.cpp` | BSP traversal, geometry drawing, frustum culling, collision |
| `src/engine/Render.h` | Data structures, array declarations, constants |
| `src/engine/TinyGL.cpp` | Transform, clip, project, occlusion buffer |
| `src/game/Enums.h` | Polygon flag constants |

---

## 1. Coordinate Systems

The engine uses three coordinate scales. Every section in this document refers to these, so understanding them first is critical.

### Three Scales

| Scale | Name | Size | Conversion |
|-------|------|------|------------|
| Tile | Grid position | 1 tile | `col`, `row` (0-31) |
| World | Game logic | 64 units/tile | `col * 64 + 32` = tile center |
| Render | Renderer | 1024 units/tile | `(worldCoord << 4) + 8` |

The render coordinate is what gets passed to `Render::render()`. The game computes it in `Canvas.cpp` before calling the renderer.

### Byte-Packed Coordinates

Many BSP arrays store coordinates as single bytes with different shift amounts depending on context:

| Context | Shift | Space | Source |
|---------|-------|-------|--------|
| Polygon vertices | `byte << 7` | Render | `Render.cpp:1981` |
| Node bounds (culling) | `byte << 7` | Render | `Render.cpp:2044` |
| Node bounds (collision) | `byte << 3` | World | `Render.cpp:2195` |
| Line endpoints (automap) | `byte << 7` | Render | `Render.cpp:1733` |
| Line endpoints (collision) | `byte << 3` | World | `Render.cpp:2215` |
| UV coordinates | `(int8_t)byte << 6` | Texture | `Render.cpp:1984` |
| Height map | `byte << 3` | World | `Render.cpp:3401` |

The relationship: `byte = worldCoord / 8`. The engine reads `byte << 7` which equals `(worldCoord / 8) * 128 = worldCoord * 16` = render space. For collision, `byte << 3` = `(worldCoord / 8) * 8` = world space.

### Coordinate Pipeline

```
Tile (col, row)
  |  col * 64 + 32
  v
World (0..2048)
  |  << 4 + 8
  v
Render (0..32776)

Byte storage:  byte * 8 = world    byte << 7 = render
```

---

## 2. Render Pipeline Overview

### Entry Point: `render()` (`Render.cpp:3162`)

The main render function receives view position in render-space coordinates and performs:

1. Compute sky scroll position from `viewAngle`
2. Apply view effects (dizzy, rock, shake, familiar bob)
3. Nudge view position back by 160 units along view direction (third-person offset)
4. Set up the TinyGL view matrix via `tinyGL->setView()`
5. Interpolate fog parameters if a fog transition is active
6. Initialize the occlusion buffer: `columnScale[i] = INT_MAX` for all screen columns
7. Clear the color buffer (sky map texture or fog color)
8. Call **`renderBSP()`** -- the core BSP rendering

### `renderBSP()` (`Render.cpp:2692`)

This function orchestrates the two-phase rendering:

**Phase 1 -- Traversal** (front-to-back):
```
walkNode(0)  // start from BSP root, node index 0
```
This recursively walks the BSP tree, culling invisible nodes and collecting visible leaf nodes into `nodeIdxs[]` array. Leaves are added front-to-back (nearest first).

**Phase 2 -- Drawing** (back-to-front):
```
for i = numVisibleNodes-1 down to 0:
    drawNodeGeometry(nodeIdxs[i])    // render polygons
    addNodeSprites(nodeIdxs[i])      // collect sprites in this leaf
    render sprites sorted by depth   // iterate viewSprites linked list
```

The key insight: traversal collects leaves front-to-back (near side first in `walkNode`), but rendering iterates them in **reverse** order (back-to-front). This implements the **painter's algorithm** -- far geometry is drawn first, near geometry paints over it. This is why the BSP split strategy matters: it controls the draw order.

---

## 3. BSP Tree Structure

### Node Arrays (Runtime)

Each BSP node `n` is described by parallel arrays loaded from the `.bin` file:

| Array | Type | Internal Node | Leaf Node |
|-------|------|---------------|-----------|
| `nodeOffsets[n]` | `short` | Split plane offset | `0xFFFF` (leaf marker) |
| `nodeNormalIdxs[n]` | `ubyte` | Index into `normals[]` (multiply by 3) | Unused |
| `nodeChildOffset1[n]` | `short` | Child node index (near side) | Byte offset into `nodePolys[]` |
| `nodeChildOffset2[n]` | `short` | Child node index (far side) | Packed line reference |
| `nodeBounds[n*4..n*4+3]` | `ubyte[4]` | Bounding box (minX, minY, maxX, maxY) | Same |
| `nodeSprites[n]` | `short` | Head of split-sprite list | Head of leaf-sprite list |

### Leaf Detection

A node is a leaf when `(nodeOffsets[n] & 0xFFFF) == 0xFFFF`. For leaf nodes, the child offset fields are repurposed:

- **`nodeChildOffset1[n]`** = byte offset into the `nodePolys[]` buffer where this leaf's polygon data begins
- **`nodeChildOffset2[n]`** = packed line reference encoding both the starting line index and the line count:

```
child2 = (lineStart & 0x3FF) | ((lineCount & 0x3F) << 10)

Extract:
  lineStart = child2 & 0x3FF       // 10 bits, max 1023
  lineCount = (child2 >> 10) & 0x3F // 6 bits, max 63
```

### Sprite Linked Lists

Each node has a linked list of sprites. `nodeSprites[n]` is the head index (`-1` = empty). Sprites chain through `mapSprites[S_NODENEXT + spriteIdx]`.

- **Leaf nodes**: sprites physically located in this leaf's spatial region
- **Internal nodes**: "split sprites" that straddle the node's split plane

---

## 4. Node Classification (Split Plane Math)

### `nodeClassifyPoint()` (`Render.cpp:1920`)

Determines which side of a BSP split plane a point lies on:

```cpp
int n5 = (nodeNormalIdxs[n] & 0xFF) * 3;
return ((x * normals[n5] + y * normals[n5+1] + z * normals[n5+2]) >> 14)
       + (nodeOffsets[n] & 0xFFFF);
```

This is a standard plane equation: dot product of the point with the plane normal, plus the plane offset.

### Normal Vectors

In practice, only two normals are used (axis-aligned splits):

| Index | Normal Vector | Split Axis |
|-------|---------------|------------|
| 0 | `(-16384, 0, 0)` | X-axis (vertical split line) |
| 1 | `(0, -16384, 0)` | Y-axis (horizontal split line) |

### Worked Example: X-axis Split at Tile Column T

- Normal = `(-16384, 0, 0)`, offset = `T * 64 * 16` (tile -> world -> render)
- Result = `(viewX * -16384) >> 14 + T * 1024` = `-viewX + T * 1024`
- Result >= 0: viewer is at `viewX <= T * 1024` (left side) -> traverse **child1** first
- Result < 0: viewer is at `viewX > T * 1024` (right side) -> traverse **child2** first

### Traversal Order

`walkNode()` always recurses the **near side first** (the side containing the viewer), then the far side. This produces a front-to-back ordering of leaves in the `nodeIdxs[]` array:

```cpp
if (nodeClassifyPoint(n, viewX, viewY, viewZ) >= 0) {
    walkNode(nodeChildOffset1[n]);  // near side (child1)
    walkNode(nodeChildOffset2[n]);  // far side (child2)
} else {
    walkNode(nodeChildOffset2[n]);  // near side (child2)
    walkNode(nodeChildOffset1[n]);  // far side (child1)
}
```

---

## 5. `walkNode()` -- BSP Traversal (`Render.cpp:2039`)

### Step-by-Step

**1. Frustum Cull:**
```cpp
if (cullBoundingBox(
        nodeBounds[n*4+0] << 7, nodeBounds[n*4+1] << 7,
        nodeBounds[n*4+2] << 7, nodeBounds[n*4+3] << 7, true))
    return;  // entire subtree is invisible
```
Bounds are shifted `<< 7` from byte storage to render space.

**2. Leaf Node Processing** (when `nodeOffsets[n] == 0xFFFF`):
- Add to visible list: `nodeIdxs[numVisibleNodes++] = n` (capped at 256)
- Call `drawNodeLines(n)` to process line segments for the automap and update the occlusion buffer
- Iterate sprites linked to this leaf; for wall sprites (with `0x400000` flag and tile < 450), call `occludeSpriteLine()` to write their silhouette into the occlusion buffer

**3. Internal Node Processing:**
- Save the current `numVisibleNodes` count
- Classify viewpoint against the split plane
- Recurse near-side first, then far-side
- After both children are processed, handle split sprites: iterate sprites linked to this internal node and call `addSplitSprite()` to assign them to a visible leaf whose bounds contain them

### Split Sprites (`Render.cpp:1890`)

Entities that straddle a BSP split plane are linked to the internal node (not a leaf). During traversal, `addSplitSprite()` scans visible leaves (from the saved count onward) and assigns the sprite to the first leaf whose bounds overlap the sprite position. Maximum 8 split sprites per frame (`MAX_SPLIT_SPRITES`).

---

## 6. Frustum Culling

### Simple Overload (`Render.cpp:1753`)

For single-tile visibility checks. Takes tile-aligned world coordinates, adds 64-unit padding, shifts to render space:

```cpp
bool cullBoundingBox(int worldX, int worldY, bool frontOnly) {
    return cullBoundingBox(
        (worldX & 0xFFFFFFC0) << 4, (worldY & 0xFFFFFFC0) << 4,
        (worldX | 0x3F) << 4,       (worldY | 0x3F) << 4, frontOnly);
}
```

### Full AABB Overload (`Render.cpp:1757`)

The main culling function. Takes a bounding box in render space and returns `true` if completely invisible:

1. **Inside check**: If the viewer is inside the AABB (with `CULL_EXTRA` = 272 units padding), return `false` (always visible)
2. **Silhouette edge selection**: Based on the viewer's position relative to the box (9 quadrants), select the two corners that form the widest visible angular extent of the box
3. **Transform**: Convert the two corner points from world space to view space via `transform2DVerts()`
4. **Clip**: Clip the line segment against the near plane via `clipLine()`
5. **Project**: Project to screen columns via `projectVerts()`
6. **Occlusion test**: Check against the per-column occlusion buffer (`columnScale[]`) via `clippedLineVisCheck()`. If the box is fully behind already-drawn geometry at every screen column it spans, it is culled

### Occlusion Buffer

`TinyGL::columnScale[]` is a per-screen-column array initialized to `INT_MAX` each frame. As wall lines and sprite silhouettes are drawn during the front-to-back traversal phase, they write depth values into this buffer. The frustum cull check compares the bounding box's projected depth against the stored values -- if occluded at every column, the node (and its entire subtree) is skipped.

This is why front-to-back traversal matters: near walls fill the occlusion buffer early, allowing far nodes to be culled without ever processing their geometry.

---

## 7. Polygon Data Format

### `drawNodeGeometry()` (`Render.cpp:1926`)

This is the core geometry renderer. It only processes leaf nodes and reads polygon data from the `nodePolys[]` byte buffer.

### Data Layout

```
offset = nodeChildOffset1[n] & 0xFFFF    // byte offset into nodePolys[]

[1 byte]  meshCount                       // number of meshes in this leaf

For each mesh:
  [4 bytes]  reserved                     // unused
  [2 bytes]  packed (little-endian)       // tileNumber << 7 | polyCount
  
  tileNumber = packed >> 7                // texture/media index (9 bits)
  polyCount  = packed & 0x7F             // polygon count (7 bits, max 127)
  
  For each polygon:
    [1 byte]  polyFlags                   // vertex count + rendering flags
    
    numVerts = (polyFlags & 0x07) + 2     // 2 to 9 vertices
    
    For each vertex (numVerts times):
      [1 byte]  x                         // render X = byte << 7
      [1 byte]  y                         // render Y = byte << 7
      [1 byte]  z                         // render Z = byte << 7
      [1 byte]  s                         // texture U = (int8_t)byte << 6
      [1 byte]  t                         // texture V = (int8_t)byte << 6
```

Total per vertex: **5 bytes**. Total per polygon: **1 + (numVerts * 5) bytes**.

### Polygon Flags Byte

| Bits | Mask | Constant | Values |
|------|------|----------|--------|
| 0-2 | `0x07` | `POLY_FLAG_VERTS_MASK` | numVerts - 2 (0=2 verts ... 7=9 verts) |
| 3-4 | `0x18` | `POLY_FLAG_AXIS_MASK` | 0=AXIS_X, 8=AXIS_Y, 16=AXIS_Z, 24=AXIS_NONE |
| 5 | `0x20` | `POLY_FLAG_WALL_TEXTURE` | Not used in practice |
| 6 | `0x40` | `POLY_FLAG_SWAPXY` | Swap X/Y in texture mapping |
| 7 | `0x80` | `POLY_FLAG_UV_DELTAX` | UV expansion axis for 2-vert polys |

### Standard Flag Values

| Surface | Flags | Hex | Meaning |
|---------|-------|-----|---------|
| Floor | 4 verts + AXIS_NONE | `0x1A` | (2 in bits 0-2) + (24 in bits 3-4) = 26 |
| Ceiling | 4 verts + AXIS_NONE | `0x1A` | Same as floor |
| Wall | 4 verts + AXIS_NONE + SWAPXY | `0x5A` | 26 + 64 = 90 |

### 2-Vertex Polygon Expansion (`Render.cpp:1988`)

When `numVerts == 2`, the engine expands the two stored vertices into a 4-vertex quad. The two vertices define diagonally opposite corners. The `AXIS_MASK` bits control which axis is used to construct the other two corners:

- **AXIS_X (0)**: vertices 1 and 3 get their X swapped from the opposite corner
- **AXIS_Y (8)**: vertices 1 and 3 get their Y swapped
- **AXIS_Z (16)**: vertices 1 and 3 get their Z swapped

The `UV_DELTAX` bit controls whether the S (U) or T (V) texture coordinates are expanded along the same axis. This is a compression technique used by the original game tools. For custom maps, using explicit 4-vertex polygons (AXIS_NONE = 24) is simpler and recommended.

### Mesh Ordering Within a Leaf

The order meshes appear in the poly data controls draw order within a leaf. Recommended order:

1. **Floor meshes** first (drawn first, behind everything)
2. **Ceiling meshes** next
3. **Wall meshes** last (drawn on top, in front)

If a single surface type exceeds 127 polygons, split into multiple consecutive meshes with the same tile number.

### Special Tile Handling

- **Lava tiles** (`TILE_FLAT_LAVA`, `TILE_FLAT_LAVA2`): Z offset wobbles with a sine wave; UV coordinates scroll over time. Face culling is disabled for lava.
- **Hell Hands tile**: Rendered with 50% alpha blending
- **Fade/Scorch Mark tiles**: Subtractive blending on TinyGL renderer

---

## 8. Line Segments

Lines serve two purposes: automap rendering and collision detection. They share the same data arrays but interpret coordinates differently.

### Data Storage

- `lineFlags[(numLines+1)/2]` -- packed nibbles, 4 bits per line (2 lines per byte)
- `lineXs[numLines * 2]` -- X coordinates for each line's two endpoints
- `lineYs[numLines * 2]` -- Y coordinates for each line's two endpoints

Line `i` has endpoints at:
- Start: `(lineXs[i*2], lineYs[i*2])`
- End: `(lineXs[i*2+1], lineYs[i*2+1])`

Flag extraction: `lineFlags[i >> 1] >> ((i & 1) << 2) & 0xF`

### Line Types

| Type | Value | Automap | Collision (`traceWorld`) |
|------|-------|---------|--------------------------|
| 0 | Blocking wall | Drawn | Full collision |
| 4 | Passable (door) | Drawn | Skipped (pass through) |
| 5 | One-way | Not drawn | Blocks only with specific flags |
| 6 | Skip | Marked seen silently | Always skipped |
| 7 | Directional | Not drawn | Blocks from one side only (dot product test) |

The upper nibble bit (bit 3, value 8) is the "discovered" flag -- set by the automap system when the line has been seen.

### Automap Rendering: `drawNodeLines()` (`Render.cpp:1724`)

During the `walkNode()` traversal (front-to-back), each leaf's lines are processed:

1. Extract line range from `nodeChildOffset2[n]`
2. For each line, extract the 4-bit type
3. Lines of type 0 and 4 are transformed to screen space (`byte << 7` = render coords), clipped, and tested against the occlusion buffer
4. If the line passes the occlusion test and automap updating is enabled, bit 3 is set in the line flags to mark it as "discovered"
5. Type 6 lines are silently marked as discovered without drawing

This occlusion pass is important: it fills the `columnScale[]` buffer with wall depths, enabling frustum culling of far nodes.

### Collision Detection: `traceWorld()` (`Render.cpp:2194`)

Physics collision uses the BSP tree for spatial acceleration:

1. Check capsule bounds against node bounds (`byte << 3` = **world space**, not render space)
2. If leaf: iterate all lines, skip types 4 and 6, perform `CapsuleToLineTrace()` for blocking types
3. If internal: classify capsule center, recurse near-side first. If no hit, try far-side
4. Type 5 lines block only when specific trace flags are set (`0x10` or `0x800`)
5. Type 7 lines use a dot product test to block from one side only

Returns `16384` for no hit, or a smaller distance value for the closest hit.

---

## 9. Sprite Rendering

### Sprite-to-Node Assignment

Each sprite is assigned to a BSP leaf node via `getNodeForPoint()` (`Render.cpp:3355`). This function walks the BSP tree from root, classifying the sprite's render-space position against each split plane until reaching a leaf. The sprite is linked into that leaf's sprite linked list.

Sprites that land exactly on a split plane or have directional flags may be linked to an internal node as "split sprites."

### Collection and Sorting

During the back-to-front rendering phase (`renderBSP`), for each visible leaf:

1. **`addNodeSprites()`**: Iterates the leaf's sprite linked list, calls `addSprite()` for each
2. **`addSprite()`** (`Render.cpp:1828`): Computes a sort-Z depth using the view matrix projection, then inserts the sprite into a depth-sorted linked list (`viewSprites`)
3. **Sprite rendering**: The sorted linked list is iterated, rendering sprites far-to-near

### Depth Sort Adjustments

Different sprite types receive depth nudges to control draw priority:

| Sprite Type | Adjustment | Effect |
|-------------|------------|--------|
| Wall sprites (`0x10000000` flag) | `z = INT_MAX` | Always drawn behind everything |
| Entity sprites (extended tile) | `z += 6` | Slightly behind same-depth geometry |
| Monsters | `z -= 1` | Slightly in front |
| Doors | `z -= 3` | In front of walls |

### Sprite Occlusion

Wall sprites (those with directional flags in `mapSpriteInfo` and tile < 450) have their silhouette written to the occlusion buffer during the `walkNode()` front-to-back pass. This prevents geometry behind a wall sprite from rendering through it.

### Split Sprites

Entities that straddle a BSP split plane are linked to the internal node above the split. During `walkNode()`, after both children are processed, `addSplitSprite()` checks if any split sprites overlap with newly visible leaves and adds them to the appropriate leaf for rendering.

---

## 10. Height Map

### Storage

1024 bytes (32x32 grid), one byte per tile. Stored as section 9 of the `.bin` file.

### `getHeight()` (`Render.cpp:3395`)

```cpp
int getHeight(int x, int y) {
    x &= 0x7FF;  // clamp to 0-2047 (32 tiles * 64 units)
    y &= 0x7FF;
    return heightMap[(y >> 6) * 32 + (x >> 6)] << 3;
}
```

Takes world-space coordinates, returns world-space height.

### Usage

- **Sprite Z positioning**: After loading, `postProcessSprites()` adds `getHeight(spriteX, spriteY)` to each sprite's Z coordinate
- **Player floor height**: `getHeight(playerX, playerY)` gives the floor Z in world units
- **Polygon Z**: Floor polygons use `heightMap[tile]` directly as the Z byte value. Ceiling polygons use `heightMap[tile] + 8` (since `8 * 8 = 64` world units = 1 tile height)

A flat map uses height 0 for all tiles. For elevated terrain, set per-tile height values.

---

## 11. Diagonal and Non-Axis-Aligned Geometry

The engine fully supports diagonal walls and arbitrary-angle BSP splits. Real D2 levels (e.g., Gehenna with 62 normals, Abaddon with 49) use diagonal split planes extensively. This section documents the techniques required to render diagonal geometry correctly.

### Arbitrary Normals — Verified in Production Levels

The `nodeClassifyPoint()` dot product supports any normal vector, not just axis-aligned. Real D2 level normals include:

| Normal | Meaning | Example Level |
|--------|---------|---------------|
| `(-16384, 0, 0)` | X-axis split (vertical line) | All levels |
| `(0, -16384, 0)` | Y-axis split (horizontal line) | All levels |
| `(0, 0, -16384)` | Z-axis split (height split) | Gehenna, Abaddon |
| `(-11585, -11585, 0)` | 45° NE-SW diagonal | Gehenna node 547 |
| `(-11585, 11585, 0)` | 45° NW-SE diagonal | Gehenna node 593 |
| `(-14654, 0, 7327)` | Angled ramp/slope | Gehenna, Abaddon |
| `(-13107, -9830, 0)` | Non-45° wall angle | Gehenna |

The value `-11585` is `-16384 / √2`, representing a unit vector at 45°. Arbitrary angles use other ratios. The normal magnitude must be `16384` (= `1 << 14`) for the `>> 14` shift in `nodeClassifyPoint()` to work correctly.

### Offset Calculation for Diagonal Splits

For a split plane with normal `(nx, ny, nz)` passing through world-space point `(wx, wy, wz)`:

```
renderX = wx * 16 + 8
renderY = wy * 16 + 8
renderZ = wz * 16 + 8
offset = -((renderX * nx + renderY * ny + renderZ * nz) >> 14)
```

Store as `uint16`. The engine computes `classify = dot(viewPos, normal) >> 14 + offset`. Result `>= 0` means child1 (near), `< 0` means child2 (far).

### Triangle Floor/Ceiling Splits

When a diagonal wall crosses a tile, the tile's floor and ceiling quads must be split into **two triangles** along the diagonal. Without this, the rectangular quad extends past the diagonal boundary and causes painter's algorithm bleed-through.

**Triangle poly format**: `flags = 0x19` (1 in bits 0-2 = 3 vertices, + AXIS_NONE). Verified in production D2 levels (Gehenna leaf 550).

**NE-SW diagonal** (line from tile corner `(x0,y1)` to `(x1,y0)`):
```
Triangle A (top-left):  (x0,y0), (x1,y0), (x0,y1)  — centroid at (col*64+21, row*64+21)
Triangle B (bot-right): (x1,y0), (x1,y1), (x0,y1)  — centroid at (col*64+43, row*64+43)
```

**NW-SE diagonal** (line from tile corner `(x0,y0)` to `(x1,y1)`):
```
Triangle A (top-right): (x0,y0), (x1,y0), (x1,y1)  — centroid at (col*64+43, row*64+21)
Triangle B (bot-left):  (x0,y0), (x1,y1), (x0,y1)  — centroid at (col*64+21, row*64+43)
```

The offset centroids (21 and 43 instead of 32) ensure each triangle classifies to the correct side of the diagonal split plane.

### Diagonal Wall Poly Classification — The Nudge Rule

**Critical**: A diagonal wall poly's midpoint lies exactly on the diagonal split plane (`classify ≈ 0`). It may land on either side randomly, causing painter's algorithm failure if it ends up separated from its adjacent floor triangles.

**Fix**: Nudge the wall poly's BSP classification point (tileCenterX/Y) toward the **open/walkable side** by ~24 world units along the wall's open-side normal:

```
// After determining wall facing (swapping endpoints so front faces open side):
swappedDx = v2x - v1x;  swappedDy = v2y - v1y;
// Right-normal of swapped direction = toward open side
openNudgeX = sign(swappedDy) * 24;
openNudgeY = sign(-swappedDx) * 24;
tileCenterX = midX + openNudgeX;
tileCenterY = midY + openNudgeY;
```

This ensures the wall poly classifies to the **same BSP leaf** as the floor triangles on the open side, which is required for correct painter's algorithm ordering (wall draws on top of floor).

### Post-Split Strategy for Converting External Levels

When converting levels from external sources (e.g., Doom 1 RPG .bsp) that contain diagonal walls:

1. **Build the main BSP tree** using axis-aligned tile-boundary splits with crossing-minimization. This produces a proven glitch-free tree for all axis-aligned geometry.

2. **Post-process leaves containing diagonal polys**: For each leaf that has a diagonal wall poly, replace it with an interior node using the diagonal's normal, creating two child leaves. Classify all polys/lines in the original leaf to the correct child.

3. **Use triangle-split floors** at tiles crossed by diagonals. Each triangle's centroid is offset to one side of the diagonal, ensuring correct classification.

4. **Apply the nudge rule** to diagonal wall polys so they end up with their floor triangles.

This two-phase approach (axis-aligned BSP + diagonal post-splits) avoids disrupting the proven axis-aligned rendering while correctly handling diagonal geometry.

### Wall Facing Determination

For a wall line from `(v1x,v1y)` to `(v2x,v2y)`:

1. Compute the right-normal direction: `(sign(dy), sign(-dx))` scaled by half a tile (32 units)
2. Check which tile the right-normal side points to
3. If that tile is **solid**, the open side is to the **left** — swap `v1` and `v2`
4. After swap, the winding `v1→v2` goes along the wall with the open side on the right

The wall quad vertices are then:
```
top-start:    (v1x/8, v1y/8, z_ceil,  s=0, t=1)
top-end:      (v2x/8, v2y/8, z_ceil,  s=1, t=1)
bottom-end:   (v2x/8, v2y/8, z_floor, s=1, t=0)
bottom-start: (v1x/8, v1y/8, z_floor, s=0, t=0)
```

### Collision Lines for Diagonal Walls

The D2 collision system (`CapsuleToLineTrace` in `Render.cpp:2119`) performs full parametric line-segment tests with no axis-alignment assumption. Diagonal collision lines work identically to axis-aligned ones:

```
x1 = v1x / 8,  x2 = v2x / 8
y1 = v1y / 8,  y2 = v2y / 8
flags = 0 (blocking) or 4 (passable/door)
```

The automap renderer also handles diagonal lines natively (`drawLine` in `Canvas.cpp`).

---

## 12. Practical Guide: Creating a Custom Map

This section summarizes the complete workflow for creating a renderable BSP map.

### Step 1: Define the Tile Grid

Create a 32x32 grid marking each tile as solid (wall) or open (walkable). This determines the base map geometry.

### Step 2: Build the BSP Tree

Follow the algorithm in [BSP_COMPILER_NOTES.md](BSP_COMPILER_NOTES.md). Produce:

- `normals[]` -- at minimum two entries: `(-16384, 0, 0)` and `(0, -16384, 0)`. Add diagonal normals if needed (see Section 11).
- `nodeOffsets[]` -- split plane offsets; `0xFFFF` for leaves
- `nodeNormalIdxs[]` -- index into `normals[]` for each node
- `nodeChildOffset1[]` -- child indices for internal nodes, poly data offsets for leaves
- `nodeChildOffset2[]` -- child indices for internal nodes, packed line refs for leaves
- `nodeBounds[]` -- AABB per node: `(min_col*8, min_row*8, max_col*8, max_row*8)`

If the map contains diagonal walls, apply the post-split refinement described in Section 11.

### Step 3: Generate Polygon Data

For each leaf node, build the `nodePolys[]` data:

1. **Floor quad** per open tile (no diagonal): flags `0x1A`, CW winding from above `(x0,y1) -> (x1,y1) -> (x1,y0) -> (x0,y0)`, Z = height map value
2. **Floor triangles** per diagonal tile: flags `0x19`, split along the diagonal (see Section 11)
3. **Ceiling quad/triangles**: same as floor but CCW winding, Z = height + 8
4. **Wall quad** per solid/open boundary: flags `0x5A`, assigned to the leaf containing the open tile
5. **Diagonal wall quad**: flags `0x5A`, classification point nudged toward open side (see Section 11)
6. Vertex byte = `worldCoord / 8`. Tile boundary at column C = `C * 8`.
7. Pack with mesh headers: `(tileNumber << 7) | (polyCount & 0x7F)`, preceded by 4 zero bytes

### Step 4: Generate Collision Lines

For each leaf, create line segments for its walls:

- Blocking walls: line type `0`
- Door walls: line type `4`
- Diagonal walls: same format, endpoints at arbitrary angles
- Pack coordinates as `worldCoord / 8`
- Set `nodeChildOffset2[leaf] = (lineStart & 0x3FF) | ((lineCount & 0x3F) << 10)`

### Step 5: Set Up Height Map

Fill 1024 bytes. Use `0` for flat maps. Value `N` means floor at `N * 8` world units. Player viewZ = `getHeight(x,y) + 36` world units.

### Step 6: Register Media

Include all tile numbers used in mesh headers in the media section of the `.bin` file.

### Step 7: Verify Against Engine Constraints

| Constraint | Limit | Consequence of Violation |
|------------|-------|--------------------------|
| Visible leaves per frame | 256 | Geometry silently dropped |
| Polygons per mesh | 127 | Data corruption (7-bit overflow) |
| Lines per leaf | 63 | Packed reference overflow (6-bit) |
| Line start index | 1023 | Packed reference overflow (10-bit) |
| Poly data total size | 65535 bytes | Exceeds `uint16` header field |
| Coordinate byte range | 0-255 | Clamped / wrapping |
| Normal vector magnitude | 16384 | Required for `>> 14` shift in classify |

### Common Rendering Issues

| Symptom | Likely Cause |
|---------|--------------|
| Far floors visible through near walls | BSP splits don't separate them; painter's algorithm failure. Improve split strategy to minimize open-tile crossings. |
| See-through diagonal walls | Diagonal wall poly in wrong BSP leaf. Apply the nudge rule (Section 11) to place it with its floor triangles. |
| Floor bleeds through diagonal wall | Floor quad not split into triangles at diagonal boundary. Use triangle-split floors (Section 11). |
| Glitches everywhere (not just diagonals) | Wrong BSP tree structure. The D1 2D BSP tree does NOT work for D2's 3D painter's algorithm. Use tile-boundary crossing-minimization splits. |
| Missing geometry (chunks of map vanish) | More than 256 visible leaves. Reduce leaf count or improve split strategy. |
| Texture appears mirrored or rotated | Wrong `SWAPXY` flag. Walls need `0x5A`, floors/ceilings need `0x1A`. |
| Player walks through walls | Line type set to `4` (passable) or `6` (skip) instead of `0` (blocking). |
| Sprites floating above floor | Height map values don't match sprite tile positions. Check `getHeight()` at sprite coordinates. |
| Walls render on wrong side of room | Wall polygon assigned to wrong leaf. Walls must be assigned to the leaf containing the adjacent **open** tile, not the solid tile. |
| Automap doesn't show walls | Line type is not `0` or `4`, or lines are missing from the leaf entirely. |
| Player spawns below/above floor | Height map value doesn't match floor polygon Z byte. Both must use the same value. |

---

## 13. Lessons Learned from D1→D2 Conversion

These findings were discovered during the Doom 1 RPG .bsp to Doom 2 RPG .bin conversion project and apply to any level conversion targeting this engine.

### D1 RPG BSP Tree Cannot Be Used Directly

The D1 RPG uses a 2D line-based BSP tree optimized for a 2D renderer. Using it directly for D2's 3D painter's algorithm causes rendering glitches everywhere — not just at diagonals. The D1 tree produces correct spatial partitioning for 2D line visibility, but incorrect draw ordering for 3D polygon rendering. Always build a fresh BSP using the tile-boundary crossing-minimization algorithm for the main tree.

### Real D2 Levels Use Arbitrary-Angle Normals

Production D2 levels (exported from Maya) use many non-axis-aligned split planes:

| Level | Normal Count | Diagonal/3D Normals |
|-------|-------------|---------------------|
| 01_tycho | 2 | 0 (flat, simple) |
| 02_cichus | 19 | 7 |
| 07_gehenna | 62 | 40 |
| 08_abaddon | 49 | 30 |

This proves the engine handles arbitrary BSP splits correctly. The limiting factor is generating the right split planes and polygon assignments, not the engine itself.

### The Painter's Algorithm Is Extremely Sensitive

Within a single BSP leaf, polygons render in mesh order (floors first, then ceilings, then walls). Between leaves, the back-to-front traversal determines order. A single polygon in the wrong leaf causes visible bleed-through. Key rules:

1. Every wall poly must be in the **same leaf** as the floor directly in front of it
2. Floor/ceiling polys that cross a split boundary must be **split into triangles**
3. Wall polys at split boundaries need their classification point **nudged** to the correct side
4. The tile-center classification works for axis-aligned walls; diagonal walls need explicit nudging

### Conversion Pipeline Summary

For converting levels from other games to the D2 .bin format:

```
Source level data
  |
  v
1. Extract tile grid (solid/open) → 32x32 blockmap
2. Extract wall lines (with endpoints, textures, flags)
3. Identify diagonal lines (non-axis-aligned)
  |
  v
4. Generate floor/ceiling quads (from blockmap)
   - Split into triangles at tiles with diagonals
5. Generate axis-aligned wall quads (from blockmap solid/open boundaries)
6. Generate diagonal wall quads (from source line data)
   - Determine facing (solid side check)
   - Nudge classification point toward open side
  |
  v
7. Build tile-boundary BSP (crossing-minimization)
8. Post-split leaves containing diagonals
9. Assign all polys to leaves by classification point
10. Pack into .bin format
  |
  v
11. Generate level.yaml (spawn point, media, door sprites)
12. Generate scripts.yaml (empty for geometry-only)
```

---

## 14. Key Constants Reference

| Constant | Value | Source | Description |
|----------|-------|--------|-------------|
| `MAP_SIZE` | 32 | Various | Tiles per side |
| `TILE_SIZE` | 64 | Various | World units per tile |
| `MAX_VISIBLE_NODES` | 256 | `Render.h:71` | Max visible leaves per frame |
| `MAX_SPLIT_SPRITES` | 8 | `Render.h:124` | Max sprites on split boundaries |
| `NEAR_CLIP` | 256 | `TinyGL.h:39` | Near clip plane distance |
| `CULL_EXTRA` | 272 | `TinyGL.h:40` | Frustum cull padding (NEAR_CLIP + 16) |
| `COLUMN_SCALE_INIT` | `INT_MAX` | `TinyGL.h:42` | Occlusion buffer initial value |
| `POLY_FLAG_VERTS_MASK` | `0x07` | `Enums.h:1066` | Bits 0-2: vertex count - 2 |
| `POLY_FLAG_AXIS_X` | `0` | `Enums.h:1069` | Expand along X axis |
| `POLY_FLAG_AXIS_Y` | `8` | `Enums.h:1070` | Expand along Y axis |
| `POLY_FLAG_AXIS_Z` | `16` | `Enums.h:1071` | Expand along Z axis |
| `POLY_FLAG_AXIS_NONE` | `24` | `Enums.h:1072` | No axis expansion (explicit verts) |
| `POLY_FLAG_AXIS_MASK` | `24` | `Enums.h:1073` | Bits 3-4 mask |
| `POLY_FLAG_WALL_TEXTURE` | `32` | `Enums.h:1074` | Not used in practice |
| `POLY_FLAG_SWAPXY` | `64` | `Enums.h:1075` | Swap texture X/Y mapping |
| `POLY_FLAG_UV_DELTAX` | `128` | `Enums.h:1076` | UV expansion direction |

## 15. Source References

This document was derived from reverse-engineered source code and verified by converting Doom 1 RPG levels to the D2 format:
- `Render::render()` -- `Render.cpp:3162-3305`
- `Render::renderBSP()` -- `Render.cpp:2692-2733`
- `Render::walkNode()` -- `Render.cpp:2039-2088`
- `Render::nodeClassifyPoint()` -- `Render.cpp:1920-1924`
- `Render::drawNodeGeometry()` -- `Render.cpp:1926-2037`
- `Render::drawNodeLines()` -- `Render.cpp:1724-1751`
- `Render::cullBoundingBox()` -- `Render.cpp:1753-1826`
- `Render::traceWorld()` -- `Render.cpp:2194-2266`
- `Render::getNodeForPoint()` -- `Render.cpp:3355-3393`
- `Render::getHeight()` -- `Render.cpp:3395-3402`
- `Render::addSprite()` -- `Render.cpp:1828-1888`
- `Render::CapsuleToLineTrace()` -- `Render.cpp:2119` (diagonal collision)

### Conversion Tools
- `tools/bsp2bin.cpp` -- D1 RPG .bsp to D2 RPG .bin converter (C++, standalone)
- `tools/bsp-to-bin.py` -- Python reference implementation
- `tools/extract_models.py` -- Extract polygon geometry from .bin files to OBJ/JSON
