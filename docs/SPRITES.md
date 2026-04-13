# Sprite Definitions & PNG Texture Overrides

This document describes the sprite definition system (`sprites.yaml`) and how to replace textures with custom PNG images.

**Source files:**

| File | Purpose |
|------|---------|
| `src/game/data/SpriteDefs.h` | Sprite source types, `SpriteRenderProps`, data structures |
| `src/game/data/SpriteDefs.cpp` | YAML parser, name/index maps, render props, PNG override lookup |
| `src/engine/graphics/GLES.cpp` | PNG texture loading in `CreateTextureForMediaID()` |
| `src/engine/render/RenderSprite.cpp` | Sprite rendering — uses `SpriteRenderProps` for data-driven behavior |
| `games/doom2rpg/sprites.yaml` | Sprite definitions (runtime) |
| `src/converter/resources/d2-rpg/sprites.yaml` | Sprite definitions (converter template) |

---

## sprites.yaml Format

Each entry maps a sprite name to its source data:

```yaml
sprites:
  # Binary sprite from the original game data
  assault_rifle: {file: tables.bin, id: 1}

  # PNG override — replaces the binary texture with a custom image
  blue_door_unlocked: {file: sprites/door_texture.png, id: 274}

  # Pure image sprite (no binary fallback, used for UI)
  menu_btn_bg: {file: menu_button_background.bmp}
```

### Fields

| Field | Required | Description |
|-------|----------|-------------|
| `file` | yes | Source file path. `tables.bin` = original binary data. An image path (`.png`, `.bmp`, `.jpg`, `.tga`) = load from that file instead |
| `id` | for binary/override | Tile index in the media mapping table (0-511). Required for sprites that appear in the 3D world |
| `frame_size` | for sheets | `[w, h]` frame dimensions for sprite sheet animations |
| `frames` | for sheets | Number of animation frames |
| `render` | no | Render properties block — controls special rendering behavior (see below) |

### Sprite Source Types

The engine infers the type from the fields present:

| Type | Detection | Description |
|------|-----------|-------------|
| **Bin** | Has `id:` and `file:` is `tables.bin` | Original binary sprite. Palette + texels loaded from `newPalettes.bin` / `newTexels*.bin` |
| **Image override** | Has `id:` and `file:` is `.png`/`.bmp`/`.jpg`/`.tga` | Binary sprite replaced by a custom image. The `id` keeps the media mapping so the engine knows *where* to use it; the image replaces *how* it looks |
| **Image** | No `id:`, `file:` is an image | Standalone image sprite (used for UI/menu elements) |
| **Sheet** | Has `frame_size:` | Sprite sheet with animation frames |

---

## Sprite Render Properties

The optional `render:` block controls special rendering behavior. This allows modders to define Z-offsets, texture animation, composite sprites, glow overlays, and more — all via YAML, with no code changes.

```yaml
sprites:
  # Z-offset applied during floor/wall rendering
  obj_crate: {file: tables.bin, id: 152, render: {z_offset_floor: -224}}
  terminal_target: {file: tables.bin, id: 179, render: {z_offset_wall: -256}}

  # Scrolling texture animation
  flat_lava: {file: tables.bin, id: 479, render: {tex_anim: {s_div: 8, t_div: 16, mask: 0x3FF}}}

  # Glow sprite overlay (rendered above the base sprite)
  obj_torchiere: {file: tables.bin, id: 136, render: {glow: {sprite: sfx_lightglow1, z_mult: 10, mode: add50}}}

  # Multi-part composite sprite (rendered as stacked layers)
  tree_top: {file: tables.bin, id: 188, render: {composite: [{sprite: tree_trunk, z_mult: 0}, {sprite: tree_top, z_mult: 36}]}}

  # Auto-animation (cycles through frames over time)
  water_spout: {file: tables.bin, id: 134, render: {auto_anim: {period: 128, frames: 2}}}

  # Skip rendering entirely (invisible sprite)
  treadmill_side: {file: tables.bin, id: 158, render: {skip: true}}

  # Multiplicative color shift
  boss_vios: {file: tables.bin, id: 58, render: {multiply_shift: true}}

  # Custom render path (routes to a special renderer)
  water_stream: {file: tables.bin, id: 240, render: {path: stream}}

  # Nudge sprite to player viewpoint position
  obj_fire: {file: tables.bin, id: 130, render: {position_at_view: {offset_mult: 18, z_offset: -512}}}

  # Loot aura glow on corpses/dropped items
  obj_corpse: {file: tables.bin, id: 138, render: {loot_aura: {scale: 18, flags: 512}}}
```

### Render Property Reference

| Property | Type | Description |
|----------|------|-------------|
| `z_offset_floor` | int | Z offset applied when sprite renders in the floor path |
| `z_offset_wall` | int | Z offset applied when sprite renders in the wall path |
| `skip` | bool | If `true`, sprite is never rendered |
| `multiply_shift` | bool | Applies multiplicative color shift rendering |
| `path` | string | Routes to a named render path (e.g. `stream` for water stream wireframe) |
| `tex_anim` | object | Texture coordinate scrolling: `s_div` (S-axis time divisor), `t_div` (T-axis), `mask` (bitwise AND mask) |
| `glow` | object | Overlay glow sprite: `sprite` (name), `z_mult` (height multiplier), `mode` (render mode name from `game.yaml render_modes`) |
| `composite` | list | Multi-layer sprite: each entry has `sprite` (name) and `z_mult` (height multiplier). Layers render bottom to top |
| `auto_anim` | object | Time-based frame cycling: `period` (time divisor), `frames` (frame count) |
| `position_at_view` | object | Nudge to player position: `offset_mult` (lateral nudge), `z_offset` (vertical offset) |
| `loot_aura` | object | Pulsating loot glow: `scale` (size multiplier), `flags` (render flags bitmask) |

---

## PNG Texture Overrides

Any binary sprite can be replaced with a PNG by changing its `file:` field from `tables.bin` to an image path.

### How it works

1. At startup, `SpriteDefs::parse()` reads `sprites.yaml`
2. For entries with `id:` and an image file extension, it registers the image path in `tileIndexToPng[id]`
3. At runtime, when the renderer needs a texture for a tile, `CreateTextureForMediaID()` checks `SpriteDefs::getPngOverride(tileNum)`
4. If a PNG path is found, the image is loaded via `stb_image` and uploaded directly as a GL RGBA texture
5. If no PNG override exists, the original binary palette+texel path is used (unchanged)

### Example: Replacing a door texture

**Before** (original binary):
```yaml
  blue_door_unlocked: {file: tables.bin, id: 274}
```

**After** (custom PNG):
```yaml
  blue_door_unlocked: {file: sprites/my_door.png, id: 274}
```

Place `my_door.png` in `games/doom2rpg/sprites/`. The `id: 274` is kept so the engine knows this sprite occupies tile slot 274 in the media mapping table.

### Requirements for PNG files

- **Format**: PNG with RGBA (recommended) or RGB. Alpha channel is used for transparency
- **Dimensions**: Should match the original texture's power-of-two size (e.g. 64x64, 128x128, 256x256). Check `media_indices` or `sprites.yaml` for the original dimensions
- **Location**: Place in the game directory (e.g. `games/doom2rpg/sprites/`). The VFS resolves paths relative to the game root

### What can be replaced

Any sprite with a tile `id` can be overridden:

```yaml
  # Wall/floor textures
  texture_303: {file: sprites/custom_wall.png, id: 303}

  # Door sprites
  blue_door_unlocked: {file: sprites/custom_door.png, id: 274}

  # Monster sprites
  monster_zombie: {file: sprites/custom_zombie.png, id: 20}

  # Item sprites
  health_pack: {file: sprites/custom_healthpack.png, id: 107}

  # Weapon sprites
  assault_rifle: {file: sprites/custom_rifle.png, id: 1}
```

### Reverting to original

Change `file:` back to `tables.bin`:

```yaml
  blue_door_unlocked: {file: tables.bin, id: 274}
```

---

## Tile Index and Media Mapping

Each sprite's `id` is a tile index (0-511) that maps to one or more **media entries** (0-1023) via the `mediaMappings` table from `newMappings.bin`.

```
Tile Index (id: 274)
  |
  v
mediaMappings[274] -> base media ID (e.g. 867)
mediaMappings[275] -> end of range (e.g. 870)
  |
  v
Media IDs 867, 868, 869 = animation frames 0, 1, 2
```

A PNG override replaces frame 0 (the base media ID). Animated sprites with multiple frames will show the PNG for the first frame and original binary textures for subsequent frames.

---

## Level Integration

Levels reference sprites by name in `level.yaml`:

```yaml
media_indices:
  - assault_rifle
  - blue_door_unlocked    # If this has a PNG override, it loads the PNG
  - texture_303
  - monster_zombie
```

No changes to `level.yaml` are needed when overriding textures. The `media_indices` list just names which sprites the level uses; the actual texture source (binary or PNG) is determined by `sprites.yaml`.
