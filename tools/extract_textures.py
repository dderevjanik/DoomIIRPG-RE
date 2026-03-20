#!/usr/bin/env python3
"""
Extract textures from DoomIIRPG .ipa and export them as PNG files.

Usage:
    python3 tools/extract_textures.py ["Doom 2 RPG.ipa"] [output_dir]

The script extracts two types of textures:

1. BMP images — UI/HUD bitmaps stored directly in the .ipa (e.g. cockpit.bmp,
   ui_images.bmp, font files, backgrounds). These are 4-bit or 8-bit indexed
   color BMPs. Exported to output_dir/bmp/

2. Media textures — Sprite and world textures stored in binary format across
   newMappings.bin, newPalettes.bin, and newTexels000-038.bin. These are
   palettized 8-bit textures with RGB565 palettes. Exported to output_dir/media/

Binary format for media textures (little-endian):
    newMappings.bin:
        int16[512]  mediaMappings   — maps tile number to media index range
        uint8[1024] mediaDimensions — packed width/height as nibbles (hi=w, lo=h)
                                      actual size = 1 << nibble
        int16[4096] mediaBounds     — sprite bounds (4 shorts per media entry)
        int32[1024] mediaPalColors  — palette color counts (with flag bits)
        int32[1024] mediaTexelSizes — texel data sizes (with flag bits)

    newPalettes.bin:
        Sequential palette data, each entry is count*2 bytes of uint16 RGB565
        values, followed by a 4-byte marker.

    newTexelsNNN.bin:
        Sequential texel data (8-bit palette indices), each entry followed
        by a 4-byte marker. Files are split at 0x40000 byte boundaries.

Requires: pip install Pillow
"""

import os
import struct
import sys
import zipfile

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow")
    sys.exit(1)

# Constants matching the engine
MEDIA_MAX_MAPPINGS = 512
MEDIA_MAX_IMAGES = 1024
MEDIA_FLAG_REFERENCE = 0x80000000
MEDIA_PALETTE_REGISTERED = 0x40000000
MEDIA_TEXELS_REGISTERED = 0x40000000

IPA_PREFIX = "Payload/Doom2rpg.app/Packages/"

# All BMP files referenced in the engine source
BMP_FILES = [
    # HUD
    "cockpit.bmp",
    "damage.bmp",
    "damage_bot.bmp",
    "Hud_Actions.bmp",
    "Hud_Fill.bmp",
    "scope.bmp",
    "Hud_Player.bmp",
    "HUD_Player_Active.bmp",
    "Hud_PlayerDoom.bmp",
    "HUD_PlayerDoom_Active.bmp",
    "Hud_PlayerScientist.bmp",
    "HUD_PlayerScientist_Active.bmp",
    "Hud_Sentry.bmp",
    "HUD_sentry_active.bmp",
    "HUD_Player_frame_Normal.bmp",
    "HUD_Player_frame_Active.bmp",
    "Hud_Test.bmp",
    # UI / Canvas
    "ui_images.bmp",
    "DialogScroll.bmp",
    "Automap_Cursor.bmp",
    # Fonts
    "imgFont_16p_Light.bmp",
    "imgFont_16p_Dark.bmp",
    "imgFont_18p_Light.bmp",
    "imgWarFont.bmp",
    # Backgrounds
    "imgFabricBG.bmp",
    "imgEndOfLevelStatsBG.bmp",
    "imgGameHelpBG.bmp",
    "imgInventoryBG.bmp",
    "imgCharSelectionBG.bmp",
    # Story / Environment
    "imgStartupLogo.bmp",
    "imgProlog.bmp",
    "imgSpaceShip.bmp",
    "imgEarthCloseUp.bmp",
    "imgStarField.bmp",
    # Character selection
    "imgCharacterSelectionAssets.bmp",
    "imgMajor_legs.bmp",
    "imgMajor_torso.bmp",
    "imgRiley_legs.bmp",
    "imgRiley_torso.bmp",
    "imgSarge_legs.bmp",
    "imgSarge_torso.bmp",
    # Controls
    "imgDpad_Normal.bmp",
    "imgDpad_Up.bmp",
    "imgDpad_Down.bmp",
    "imgDpad_Left.bmp",
    "imgDpad_Right.bmp",
    "imgPageUP_Icon.bmp",
    "imgPageDOWN_Icon.bmp",
    "imgPageOK_Icon.bmp",
    # Scope
    "imgSniperScope_Dial.bmp",
    "imgSniperScope_Knob.bmp",
    # Portal
    "portal_image.bmp",
    "portal_image2.bmp",
    # Logos
    "logo.bmp",
    "logo2.bmp",
    # Sentry bot game
    "matrixSkip_button.bmp",
]

# Comic book pages
for i in range(17):
    name = "Cover" if i == 0 else f"page_{i:02d}"
    BMP_FILES.append(f"ComicBook/{name}.bmp")

IPHONE_COMIC_PAGES = [
    "iPhone cover",
    "iPhone page 1a", "iPhone page 1b",
    "iPhone page 2a", "iPhone page 2b", "iPhone page 2c",
    "iPhone page 3a", "iPhone page 3b",
    "iPhone page 4a", "iPhone page 4b", "iPhone page 4c",
    "iPhone page 5a", "iPhone page 5b", "iPhone page 5c",
    "iPhone page 6a", "iPhone page 6b", "iPhone page 6c",
    "iPhone page 7a", "iPhone page 7b",
    "iPhone page 8a", "iPhone page 8b", "iPhone page 8c",
    "iPhone page 9",
    "iPhone page 10a", "iPhone page 10b", "iPhone page 10c",
    "iPhone page 11a", "iPhone page 11b",
    "iPhone page 12a", "iPhone page 12b",
    "iPhone page 13a", "iPhone page 13b",
    "iPhone page 14a", "iPhone page 14b",
    "iPhone page 15a", "iPhone page 15b", "iPhone page 15c",
    "iPhone page 16a", "iPhone page 16b",
]
for page in IPHONE_COMIC_PAGES:
    BMP_FILES.append(f"ComicBook/frames/{page}.bmp")

# Known tile number names for labeling media textures
TILENUM_NAMES = {
    1: "ASSAULT_RIFLE", 2: "CHAINSAW", 3: "HOLY_WATER_PISTOL",
    4: "SENTRY_BOT_SHOOTING", 5: "SENTRY_BOT_EXPLODING",
    6: "SUPER_SHOTGUN", 7: "CHAINGUN", 8: "ASSAULT_RIFLE_WITH_SCOPE",
    9: "PLASMA_GUN", 10: "ROCKET_LAUNCHER", 11: "BFG", 12: "SOUL_CUBE",
    13: "SENTRY_BOT_RED_SHOOTING", 14: "SENTRY_BOT_RED_EXPLODING",
    15: "WORLD_WEAPON",
    18: "MONSTER_RED_SENTRY_BOT", 19: "MONSTER_SENTRY_BOT",
    20: "MONSTER_ZOMBIE", 21: "MONSTER_ZOMBIE2", 22: "MONSTER_ZOMBIE3",
    23: "MONSTER_IMP", 24: "MONSTER_IMP2", 25: "MONSTER_IMP3",
    26: "MONSTER_SAW_GOBLIN", 27: "MONSTER_SAW_GOBLIN2", 28: "MONSTER_SAW_GOBLIN3",
    29: "MONSTER_LOST_SOUL", 30: "MONSTER_LOST_SOUL2", 31: "MONSTER_LOST_SOUL3",
    32: "MONSTER_PINKY", 33: "MONSTER_PINKY2", 34: "MONSTER_PINKY3",
    35: "MONSTER_REVENANT", 36: "MONSTER_REVENANT2", 37: "MONSTER_REVENANT3",
    38: "MONSTER_MANCUBUS", 39: "MONSTER_MANCUBUS2", 40: "MONSTER_MANCUBUS3",
    41: "MONSTER_CACODEMON", 42: "MONSTER_CACODEMON2", 43: "MONSTER_CACODEMON3",
    44: "MONSTER_SENTINEL", 45: "MONSTER_SENTINEL2", 46: "MONSTER_SENTINEL3",
    50: "MONSTER_ARCH_VILE", 51: "MONSTER_ARCH_VILE2", 52: "MONSTER_ARCH_VILE3",
    53: "MONSTER_ARACHNOTRON",
    54: "BOSS_CYBERDEMON", 56: "BOSS_PINKY", 57: "BOSS_MASTERMIND",
    58: "BOSS_VIOS", 59: "BOSS_VIOS2", 60: "BOSS_VIOS3",
    61: "BOSS_VIOS4", 62: "BOSS_VIOS5",
    66: "NPC_RILEY_OCONNOR", 68: "NPC_MAJOR", 69: "NPC_BOB",
    71: "NPC_CIVILIAN", 72: "NPC_SARGE", 73: "NPC_FEMALE",
    74: "NPC_EVIL_SCIENTIST", 75: "NPC_SCIENTIST", 76: "NPC_RESEARCHER",
    77: "NPC_CIVILIAN2",
    85: "AMMO_BULLETS", 86: "AMMO_SHELLS", 88: "AMMO_ROCKETS",
    89: "AMMO_CELLS", 90: "AMMO_HOLY_WATER",
    107: "ONE_UAC_CREDIT", 110: "KEY_RED", 111: "KEY_BLUE",
    112: "ARMOR_JACKET", 113: "SATCHEL", 114: "PACK_ITEM",
    115: "WORKER_PACK", 116: "HEALTH_PACK", 117: "FOOD_PLATE",
    119: "ARMOR_SHARD", 122: "HELL_SEAL", 123: "TOILET",
    124: "DIRT_DECAL", 125: "LADDER", 126: "BLOOD_SPLATTER",
    127: "SINK", 128: "BARRED_WINDOW", 129: "HAZARD_BAR",
    130: "OBJ_FIRE", 131: "TREADMILL_MONITOR", 133: "TECH_STATION",
    134: "WATER_SPOUT", 135: "OBJ_CHAIR", 136: "OBJ_TORCHIERE",
    137: "OBJ_SCIENTIST_CORPSE", 138: "OBJ_CORPSE", 139: "OBJ_OTHER_CORPSE",
    147: "SEPTIC_STATION", 149: "PRACTICE_TARGET", 150: "OBJ_PRINTER",
    152: "OBJ_CRATE", 153: "VENDING_MACHINE", 154: "ARMOR_REPAIR",
    155: "CLOSED_PORTAL_EYE", 156: "EYE_PORTAL", 157: "PORTAL_SOCKET",
    158: "TREADMILL_SIDE", 159: "TREADMILL_FRONT",
    161: "HELL_HANDS", 162: "DOORJAMB_DECAL",
    164: "STONES_AND_SKULLS", 168: "FENCE",
    170: "SENTINEL_SPIKES", 171: "SENTINEL_SPIKES_DUMMY",
    173: "SWITCH", 175: "TECH_DETAIL", 177: "HELL_SKULLS",
    178: "GLASS", 179: "TERMINAL_TARGET", 180: "TERMINAL_GENERAL",
    181: "TERMINAL_VIOS", 182: "TERMINAL_BOT", 183: "TERMINAL_HACKING",
    184: "ELEVATOR_NUMS", 187: "BUSH", 188: "TREE_TOP", 189: "TREE_TRUNK",
    193: "SFX_LIGHTGLOW1", 197: "WINDOW3", 201: "GLAEVENSCOPE",
    208: "FOG_GRAY", 212: "SCORCH_MARK", 223: "STATIC_FLAME",
    225: "MISSILE_PLAYER_ROCKET", 226: "MISSILE_ROCKET",
    227: "FLESH", 232: "SHADOW",
    234: "ANIM_FIRE", 235: "ANIM_WATER", 236: "AIR_VENT",
    239: "SOUL_CUBE_ATTACK", 240: "WATER_STREAM",
    241: "CACO_PLASMA", 242: "FIRE_BALL", 243: "PLASMA_BALL", 244: "BFG_BALL",
    245: "MONSTER_CLAW", 246: "MONSTER_BITE", 247: "MONSTER_BLUNT_TRAUMA",
    248: "ELECTRIC_SLIDE", 251: "FEAR_EYE", 252: "ACID_SPIT",
    254: "NPC_CHAT", 255: "ALERT",
    257: "DOORJAMB",
    271: "RED_DOOR_LOCKED", 272: "RED_DOOR_UNLOCKED",
    273: "BLUE_DOOR_LOCKED", 274: "BLUE_DOOR_UNLOCKED",
    275: "DOOR_LOCKED", 276: "DOOR_UNLOCKED",
    277: "LEVEL_DOOR_LOCKED", 278: "LEVEL_DOOR_UNLOCKED",
    301: "SKY_BOX", 302: "FADE",
    479: "FLAT_LAVA", 480: "FLAT_LAVA2",
}

TEXEL_FILES = [f"newTexels{i:03d}.bin" for i in range(39)]


def rgb565_to_rgba(c):
    """Convert a 16-bit RGB565 color to (R, G, B, A) tuple with full alpha."""
    r = (c >> 11) & 0x1F
    g = (c >> 5) & 0x3F
    b = c & 0x1F
    r8 = (r << 3) | (r >> 2)
    g8 = (g << 2) | (g >> 4)
    b8 = (b << 3) | (b >> 2)
    return (r8, g8, b8, 255)


def is_transparent_color(r, g, b):
    """Check if a color is near-magenta (the engine's transparent marker).

    The engine uses: (R >= 250 && G == 0 && B >= 250) as the transparency test,
    plus a special case for Arachnotron: (R >= 250 && G == 4 && B >= 250).
    """
    return (r >= 250 and b >= 250 and g <= 4)


def read_le_short(data, offset):
    return struct.unpack_from("<h", data, offset)[0]


def read_le_ushort(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def read_le_int(data, offset):
    return struct.unpack_from("<i", data, offset)[0]


def extract_bmp_files(ipa, output_dir):
    """Extract all BMP image files from the .ipa and save as PNG."""
    bmp_dir = os.path.join(output_dir, "bmp")
    os.makedirs(bmp_dir, exist_ok=True)

    extracted = 0
    skipped = 0

    namelist = set(ipa.namelist())

    for bmp_name in BMP_FILES:
        ipa_path = IPA_PREFIX + bmp_name
        if ipa_path not in namelist:
            # Try case-insensitive match
            found = False
            for n in namelist:
                if n.lower() == ipa_path.lower():
                    ipa_path = n
                    found = True
                    break
            if not found:
                skipped += 1
                continue

        try:
            bmp_data = ipa.read(ipa_path)
        except KeyError:
            skipped += 1
            continue

        # Parse BMP header
        if len(bmp_data) < 54:
            print(f"  {bmp_name}: too small, skipping")
            skipped += 1
            continue

        magic = bmp_data[0:2]
        if magic != b"BM":
            print(f"  {bmp_name}: not a BMP file, skipping")
            skipped += 1
            continue

        off_bits = struct.unpack_from("<I", bmp_data, 10)[0]
        width = struct.unpack_from("<i", bmp_data, 18)[0]
        height = struct.unpack_from("<i", bmp_data, 22)[0]
        bpp = struct.unpack_from("<H", bmp_data, 28)[0]
        colors_used = struct.unpack_from("<I", bmp_data, 46)[0]

        if bpp not in (4, 8):
            print(f"  {bmp_name}: unsupported bpp={bpp}, skipping")
            skipped += 1
            continue

        if colors_used == 0:
            colors_used = 1 << bpp

        # Read palette (BGRA format)
        pal_offset = 54
        palette = []
        for i in range(colors_used):
            b = bmp_data[pal_offset + i * 4]
            g = bmp_data[pal_offset + i * 4 + 1]
            r = bmp_data[pal_offset + i * 4 + 2]
            palette.append((r, g, b))

        # Check for transparent color (magenta 0xFF00FF)
        transparent_idx = None
        for i, (r, g, b) in enumerate(palette):
            if r == 0xFF and g == 0x00 and b == 0xFF:
                transparent_idx = i
                break

        # Read pixel data
        abs_height = abs(height)
        top_down = height < 0

        # Calculate row stride (rows are padded to 4-byte boundaries)
        if bpp == 8:
            row_bytes = width
        else:  # bpp == 4
            row_bytes = (width + 1) // 2
        row_stride = (row_bytes + 3) & ~3

        pixel_data = bmp_data[off_bits:]

        has_alpha = transparent_idx is not None
        img = Image.new("RGBA" if has_alpha else "RGB", (width, abs_height))

        for y in range(abs_height):
            if top_down:
                src_y = y
            else:
                src_y = abs_height - 1 - y

            row_start = src_y * row_stride
            for x in range(width):
                if bpp == 8:
                    idx = pixel_data[row_start + x]
                else:  # bpp == 4
                    byte_val = pixel_data[row_start + x // 2]
                    if x % 2 == 0:
                        idx = (byte_val >> 4) & 0xF
                    else:
                        idx = byte_val & 0xF

                if idx < len(palette):
                    r, g, b = palette[idx]
                else:
                    r, g, b = (0, 0, 0)

                if has_alpha:
                    a = 0 if idx == transparent_idx else 255
                    img.putpixel((x, y), (r, g, b, a))
                else:
                    img.putpixel((x, y), (r, g, b))

        # Create subdirectories if needed
        png_name = bmp_name.replace(".bmp", ".png")
        png_path = os.path.join(bmp_dir, png_name)
        os.makedirs(os.path.dirname(png_path), exist_ok=True)
        img.save(png_path)
        extracted += 1

    print(f"BMP textures: {extracted} extracted, {skipped} not found")
    return extracted


def extract_media_textures(ipa, output_dir):
    """Extract media textures (sprites/walls/flats) from binary data."""
    media_dir = os.path.join(output_dir, "media")
    os.makedirs(media_dir, exist_ok=True)

    prefix = IPA_PREFIX

    # Load newMappings.bin
    mappings_data = ipa.read(prefix + "newMappings.bin")
    pos = 0

    # Read mediaMappings: int16[512]
    media_mappings = []
    for i in range(MEDIA_MAX_MAPPINGS):
        media_mappings.append(read_le_short(mappings_data, pos))
        pos += 2

    # Read mediaDimensions: uint8[1024]
    media_dimensions = list(mappings_data[pos:pos + MEDIA_MAX_IMAGES])
    pos += MEDIA_MAX_IMAGES

    # Read mediaBounds: int16[4096]
    media_bounds = []
    for i in range(MEDIA_MAX_IMAGES * 4):
        media_bounds.append(read_le_short(mappings_data, pos))
        pos += 2

    # Read mediaPalColors: int32[1024]
    media_pal_colors = []
    for i in range(MEDIA_MAX_IMAGES):
        media_pal_colors.append(read_le_int(mappings_data, pos))
        pos += 4

    # Read mediaTexelSizes: int32[1024]
    media_texel_sizes = []
    for i in range(MEDIA_MAX_IMAGES):
        media_texel_sizes.append(read_le_int(mappings_data, pos))
        pos += 4

    # Load newPalettes.bin
    palettes_data = ipa.read(prefix + "newPalettes.bin")

    # Load all texel files
    texel_files = {}
    for i, fname in enumerate(TEXEL_FILES):
        try:
            texel_files[i] = ipa.read(prefix + fname)
        except KeyError:
            pass

    # Parse palettes — sequential read with markers
    palettes = {}  # media_id -> list of uint16 RGB565 values
    pal_pos = 0
    for i in range(MEDIA_MAX_IMAGES):
        num_colors = media_pal_colors[i] & 0x3FFFFFFF
        is_ref = (media_pal_colors[i] & MEDIA_FLAG_REFERENCE) != 0

        if is_ref:
            # Reference to another palette — will resolve later
            continue

        if num_colors > 0 and pal_pos + num_colors * 2 <= len(palettes_data):
            pal = []
            for c in range(num_colors):
                color = read_le_ushort(palettes_data, pal_pos + c * 2)
                pal.append(color)
            palettes[i] = pal

        pal_pos += num_colors * 2 + 4  # +4 for marker

    # Resolve palette references
    for i in range(MEDIA_MAX_IMAGES):
        if (media_pal_colors[i] & MEDIA_FLAG_REFERENCE) != 0:
            ref_id = media_pal_colors[i] & 0x3FF
            if ref_id in palettes:
                palettes[i] = palettes[ref_id]

    # Parse texels — sequential read across texel files
    texels = {}  # media_id -> bytes
    file_idx = 0
    file_offset = 0
    accumulated = 0

    for i in range(MEDIA_MAX_IMAGES):
        is_ref = (media_texel_sizes[i] & MEDIA_FLAG_REFERENCE) != 0
        if is_ref:
            continue

        size = (media_texel_sizes[i] & 0x3FFFFFFF) + 1

        if file_idx in texel_files:
            data = texel_files[file_idx]
            if file_offset + size <= len(data):
                texels[i] = data[file_offset:file_offset + size]

        file_offset += size + 4  # +4 for marker
        accumulated += size + 4

        if file_offset > 0x40000:
            file_idx += 1
            file_offset = 0
            accumulated = 0

    # Resolve texel references
    for i in range(MEDIA_MAX_IMAGES):
        if (media_texel_sizes[i] & MEDIA_FLAG_REFERENCE) != 0:
            ref_id = media_texel_sizes[i] & 0x3FF
            if ref_id in texels:
                texels[i] = texels[ref_id]

    # Build tile-to-media mapping for naming
    tile_to_media = {}
    for tile_num in range(MEDIA_MAX_MAPPINGS - 1):
        start = media_mappings[tile_num]
        end = media_mappings[tile_num + 1]
        if start < end:
            for media_id in range(start, end):
                if media_id not in tile_to_media:
                    tile_to_media[media_id] = tile_num

    # Build RGBA palette lookup for each media ID
    # The engine converts RGB565 palette to RGBA, then checks for near-magenta
    # transparency. If any palette entry is near-magenta, that color gets alpha=0.
    def build_rgba_palette(pal_rgb565):
        """Convert RGB565 palette to RGBA, marking near-magenta as transparent."""
        rgba_pal = []
        has_trans = False
        for c in pal_rgb565:
            r, g, b, a = rgb565_to_rgba(c)
            if is_transparent_color(r, g, b):
                has_trans = True
        for i, c in enumerate(pal_rgb565):
            r, g, b, a = rgb565_to_rgba(c)
            if is_transparent_color(r, g, b):
                rgba_pal.append((0, 0, 0, 0))
            else:
                rgba_pal.append((r, g, b, 255))
        return rgba_pal, has_trans

    # Build tile-to-media mapping for naming
    tile_to_media = {}
    for tile_num in range(MEDIA_MAX_MAPPINGS - 1):
        start = media_mappings[tile_num]
        end = media_mappings[tile_num + 1]
        if start < end:
            for media_id in range(start, end):
                if media_id not in tile_to_media:
                    tile_to_media[media_id] = tile_num

    # Export each media texture as PNG
    exported = 0
    skipped_sprite = 0
    for media_id in sorted(set(list(texels.keys()) + list(palettes.keys()))):
        if media_id not in texels or media_id not in palettes:
            continue

        texel_data = texels[media_id]
        palette_rgb565 = palettes[media_id]

        dim = media_dimensions[media_id] if media_id < len(media_dimensions) else 0
        w_shift = (dim >> 4) & 0xF
        h_shift = dim & 0xF
        width = 1 << w_shift
        height = 1 << h_shift

        if width == 0 or height == 0:
            continue

        texel_size = len(texel_data)

        # Skip unused media slots: 1x1 dimension with trivial data means
        # the slot was never populated with a real texture.
        if dim == 0 and texel_size <= 1:
            continue

        is_full_texture = (texel_size == width * height)

        rgba_pal, has_trans = build_rgba_palette(palette_rgb565)

        # Build RGBA image
        img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
        pixels = img.load()

        if is_full_texture:
            # Full texture (walls, flats) — simple width*height grid
            for y in range(height):
                for x in range(width):
                    idx = texel_data[y * width + x]
                    if idx < len(rgba_pal):
                        pixels[x, y] = rgba_pal[idx]
        else:
            # Sprite texture — column-based RLE compression
            # Format (from engine's CreateTextureForMediaID):
            #   Last 2 bytes of texel_data encode the shape table offset:
            #     shape_offset = texel_data[-1] << 8 | texel_data[-2]
            #   pixel_end = texel_size - shape_offset - 2
            #   Bounds from mediaBounds[media_id*4]: shapeMin, shapeMax, minY, maxY
            #   Shape table has nibbles (4-bit) per column: number of runs
            #   Pixel data is sequential: for each run, a (y_start, run_height)
            #   pair followed by run_height palette indices.
            if texel_size < 2:
                continue

            shape_offset = (texel_data[-1] << 8) | texel_data[-2]
            pixel_end = texel_size - shape_offset - 2

            shape_min = media_bounds[4 * media_id + 0] & 0xFF
            shape_max = media_bounds[4 * media_id + 1] & 0xFF
            min_y = media_bounds[4 * media_id + 2] & 0xFF
            max_y = media_bounds[4 * media_id + 3] & 0xFF

            if shape_max > width or shape_min > shape_max:
                skipped_sprite += 1
                continue
            if max_y > height or min_y > max_y:
                skipped_sprite += 1
                continue

            # Fill background: if has_trans, use average non-transparent color
            # with alpha=0 (matching engine behavior for sprite bg).
            # Otherwise leave transparent.
            if has_trans:
                # Engine computes average of non-transparent palette entries
                avg_r, avg_g, avg_b = 0, 0, 0
                count = 0
                for i, c in enumerate(palette_rgb565):
                    if i == 0:
                        continue
                    r, g, b, a = rgb565_to_rgba(c)
                    if not is_transparent_color(r, g, b):
                        avg_r += r
                        avg_g += g
                        avg_b += b
                        count += 1
                if count > 0:
                    bg = (avg_r // count, avg_g // count, avg_b // count, 0)
                else:
                    bg = (0, 0, 0, 0)
                for y in range(height):
                    for x in range(width):
                        pixels[x, y] = bg

            # Decode column runs
            shape_table_start = pixel_end + ((shape_max - shape_min + 1) >> 1)
            pixel_pos = 0  # position in the pixel data portion

            for col_idx in range(shape_max - shape_min):
                x = shape_min + col_idx
                # Read number of runs from shape table (4-bit nibbles)
                nibble_offset = pixel_end + (col_idx >> 1)
                if nibble_offset >= texel_size:
                    break
                if col_idx & 1:
                    num_runs = (texel_data[nibble_offset] >> 4) & 0xF
                else:
                    num_runs = texel_data[nibble_offset] & 0xF

                for _ in range(num_runs):
                    if shape_table_start >= texel_size - 1:
                        break
                    y_start = texel_data[shape_table_start]
                    run_height = texel_data[shape_table_start + 1]
                    shape_table_start += 2

                    for dy in range(run_height):
                        if pixel_pos >= pixel_end:
                            break
                        idx = texel_data[pixel_pos]
                        pixel_pos += 1
                        y = y_start + dy
                        if 0 <= x < width and 0 <= y < height and idx < len(rgba_pal):
                            pixels[x, y] = rgba_pal[idx]

        # Generate filename
        tile_num = tile_to_media.get(media_id)
        tile_name = TILENUM_NAMES.get(tile_num, "") if tile_num is not None else ""

        if tile_name:
            frame = media_id - media_mappings[tile_num]
            if frame > 0:
                filename = f"{media_id:04d}_{tile_name}_f{frame}.png"
            else:
                filename = f"{media_id:04d}_{tile_name}.png"
        else:
            filename = f"{media_id:04d}.png"

        img.save(os.path.join(media_dir, filename))
        exported += 1

    print(f"Media textures: {exported} extracted ({skipped_sprite} sprites skipped due to invalid bounds)")
    return exported


def main():
    ipa_path = sys.argv[1] if len(sys.argv) > 1 else "Doom 2 RPG.ipa"
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "textures"

    print(f"Extracting textures from: {ipa_path}")
    print(f"Output directory: {output_dir}")
    os.makedirs(output_dir, exist_ok=True)

    with zipfile.ZipFile(ipa_path) as ipa:
        bmp_count = extract_bmp_files(ipa, output_dir)
        media_count = extract_media_textures(ipa, output_dir)

    print(f"\nDone. Total: {bmp_count + media_count} textures extracted to {output_dir}/")


if __name__ == "__main__":
    main()
