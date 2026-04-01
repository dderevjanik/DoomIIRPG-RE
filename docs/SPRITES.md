# Sprite Definitions & PNG Texture Overrides

This document describes the sprite definition system (`sprites.yaml`) and how to replace textures with custom PNG images.

**Source files:**

| File | Purpose |
|------|---------|
| `src/game/SpriteDefs.h` | Sprite source types and data structures |
| `src/game/SpriteDefs.cpp` | YAML parser, name/index maps, PNG override lookup |
| `src/engine/GLES.cpp` | PNG texture loading in `CreateTextureForMediaID()` |
| `games/doom2rpg/sprites.yaml` | Sprite definitions |

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

### Sprite Source Types

The engine infers the type from the fields present:

| Type | Detection | Description |
|------|-----------|-------------|
| **Bin** | Has `id:` and `file:` is `tables.bin` | Original binary sprite. Palette + texels loaded from `newPalettes.bin` / `newTexels*.bin` |
| **Image override** | Has `id:` and `file:` is `.png`/`.bmp`/`.jpg`/`.tga` | Binary sprite replaced by a custom image. The `id` keeps the media mapping so the engine knows *where* to use it; the image replaces *how* it looks |
| **Image** | No `id:`, `file:` is an image | Standalone image sprite (used for UI/menu elements) |
| **Sheet** | Has `frame_size:` | Sprite sheet with animation frames |

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
