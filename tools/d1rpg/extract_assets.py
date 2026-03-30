#!/usr/bin/env python3
"""
DoomRPG Asset Extractor

Extracts wall textures, sprites, and palettes from DoomRPG binary files
(palettes.bin, mappings.bin, bitshapes.bin, wtexels.bin, stexels.bin)
and saves them as PNG images.

Usage:
    python extract_assets.py [--data-dir DoomRPG] [--output-dir output]

Output structure:
    output/
      textures/          - Wall textures (64x64 RGB PNGs)
      sprites/           - All sprite frames grouped by base ID
        group_NNN/       - Sprite group (e.g., a monster with all animation frames)
          frame_0.png    - idle / front
          frame_1.png    - attack
          frame_2.png    - hurt
          frame_3.png    - idle alt / walk
          frame_4.png    - pain
          frame_5.png    - death start
          frame_6.png    - death end
      palettes/          - Palette visualization
"""

import argparse
import struct
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow")
    raise SystemExit(1)

# Animation frame names for monsters (7 frames) and items (4 frames)
MONSTER_FRAME_NAMES = [
    "idle",       # 0
    "attack",     # 1
    "hurt",       # 2
    "walk",       # 3
    "pain",       # 4
    "death_1",    # 5
    "death_2",    # 6
]

ITEM_FRAME_NAMES = [
    "frame_0",    # 0
    "frame_1",    # 1
    "frame_2",    # 2
    "frame_3",    # 3
]


def read_int(data, pos):
    val = struct.unpack_from('<i', data, pos)[0]
    return val, pos + 4


def read_short(data, pos):
    val = struct.unpack_from('<h', data, pos)[0]
    return val, pos + 2


def rgb565_to_rgb(color):
    """Decode RGB565 (as stored in palettes.bin) to 8-bit RGB tuple."""
    blue5 = (color >> 11) & 0x1F
    green6 = (color >> 5) & 0x3F
    red5 = color & 0x1F
    r = (red5 << 3) | (red5 >> 2)
    g = (green6 << 2) | (green6 >> 4)
    b = (blue5 << 3) | (blue5 >> 2)
    return (r, g, b)


def load_palettes(data_dir):
    """Load palettes.bin and return list of (R, G, B) tuples."""
    data = (data_dir / "palettes.bin").read_bytes()
    pos = 0
    data_size, pos = read_int(data, pos)
    count = data_size // 2
    palettes = []
    for _ in range(count):
        color, pos = read_short(data, pos)
        palettes.append(rgb565_to_rgb(color))
    return palettes


def load_mappings(data_dir):
    """Load mappings.bin and return offset arrays and ID arrays."""
    data = (data_dir / "mappings.bin").read_bytes()
    pos = 0
    texels_cnt, pos = read_int(data, pos)
    texels_cnt *= 2
    bitshape_cnt, pos = read_int(data, pos)
    bitshape_cnt *= 2
    texture_cnt, pos = read_int(data, pos)
    sprite_cnt, pos = read_int(data, pos)

    texel_offsets = []
    for _ in range(texels_cnt):
        val, pos = read_int(data, pos)
        texel_offsets.append(val)

    bitshape_offsets = []
    for _ in range(bitshape_cnt):
        val, pos = read_int(data, pos)
        bitshape_offsets.append(val)

    texture_ids = []
    for _ in range(texture_cnt):
        val, pos = read_short(data, pos)
        texture_ids.append(val)

    sprite_ids = []
    for _ in range(sprite_cnt):
        val, pos = read_short(data, pos)
        sprite_ids.append(val)

    return texel_offsets, bitshape_offsets, texture_ids, sprite_ids


def extract_textures(data_dir, output_dir, palettes, texel_offsets, texture_ids):
    """Extract all wall textures from wtexels.bin as 64x64 PNGs."""
    tex_dir = output_dir / "textures"
    tex_dir.mkdir(parents=True, exist_ok=True)

    wtexels_data = (data_dir / "wtexels.bin").read_bytes()
    texel_data = wtexels_data[4:]  # skip 4-byte header

    count = 0
    seen = set()
    for i, tid in enumerate(texture_ids):
        texel_off = texel_offsets[tid * 2 + 0]
        pal_off = texel_offsets[tid * 2 + 1]

        key = (texel_off, pal_off)
        if key in seen:
            continue
        seen.add(key)

        file_byte_off = texel_off // 2

        if file_byte_off + 2048 > len(texel_data):
            print(f"  Warning: texture {i} (tid={tid}) offset out of range, skipping")
            continue

        img = Image.new('RGB', (64, 64))
        pixels = img.load()

        for j in range(64 * 64 // 2):
            byte = texel_data[file_byte_off + j]
            idx_lo = byte & 0x0F
            idx_hi = (byte >> 4) & 0x0F

            px = (j * 2) % 64
            py = (j * 2) // 64

            if pal_off + idx_lo < len(palettes):
                pixels[px, py] = palettes[pal_off + idx_lo]
            if pal_off + idx_hi < len(palettes):
                pixels[px + 1, py] = palettes[pal_off + idx_hi]

        img.save(tex_dir / f"texture_{i:03d}.png")
        count += 1

    print(f"  Extracted {count} unique wall textures")


def decode_sprite(bs_raw, st_raw, shape_off, pal_off, stexels_base, palettes):
    """Decode a single sprite from raw bitshape + stexel data. Returns an RGBA Image or None."""
    if shape_off + 12 > len(bs_raw):
        return None

    raw_texel_off = struct.unpack_from('<I', bs_raw, shape_off)[0]
    min_x = bs_raw[shape_off + 8]
    max_x = bs_raw[shape_off + 9]
    min_y = bs_raw[shape_off + 10]
    max_y = bs_raw[shape_off + 11]

    width = max_x - min_x + 1
    height = max_y - min_y + 1

    if width <= 0 or height <= 0 or width > 128 or height > 128:
        return None

    pitch = (height + 7) // 8

    # Parse bitmask to find visible pixels
    columns = []
    total_visible = 0

    for w in range(width):
        bitmask_off = shape_off + 12 + w * pitch
        runs = []
        h = 0
        while h < height:
            while h < height:
                byte_idx = h // 8
                bit_idx = h % 8
                if bitmask_off + byte_idx < len(bs_raw) and (bs_raw[bitmask_off + byte_idx] & (1 << bit_idx)):
                    break
                h += 1
            if h >= height:
                break
            run_start = h
            while h < height:
                byte_idx = h // 8
                bit_idx = h % 8
                if bitmask_off + byte_idx >= len(bs_raw) or not (bs_raw[bitmask_off + byte_idx] & (1 << bit_idx)):
                    break
                h += 1
            run_len = h - run_start
            runs.append((run_start, run_len))
            total_visible += run_len
        columns.append(runs)

    # Calculate stexels file position
    stexels_file_off = (raw_texel_off - stexels_base) // 2
    texel_byte_count = (total_visible + 1) // 2

    if stexels_file_off < 0 or stexels_file_off + texel_byte_count > len(st_raw):
        return None

    # Read texel nibbles
    texel_indices = []
    for b in range(texel_byte_count):
        byte = st_raw[stexels_file_off + b]
        texel_indices.append(byte & 0x0F)
        texel_indices.append((byte >> 4) & 0x0F)

    # Build RGBA image
    img = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    pixels = img.load()

    texel_pos = 0
    for w in range(width):
        for (y_start, run_len) in columns[w]:
            for dy in range(run_len):
                if texel_pos < len(texel_indices):
                    idx = texel_indices[texel_pos]
                    texel_pos += 1
                    if pal_off + idx < len(palettes):
                        r, g, b = palettes[pal_off + idx]
                        px = min_x + w
                        py = min_y + y_start + dy
                        if 0 <= px < 64 and 0 <= py < 64:
                            pixels[px, py] = (r, g, b, 255)

    bbox = img.getbbox()
    if bbox:
        img = img.crop(bbox)
    return img


def decode_sprite_uncropped(bs_raw, st_raw, shape_off, pal_off, stexels_base, palettes):
    """Same as decode_sprite but returns the full 64x64 image without cropping."""
    if shape_off + 12 > len(bs_raw):
        return None

    raw_texel_off = struct.unpack_from('<I', bs_raw, shape_off)[0]
    min_x = bs_raw[shape_off + 8]
    max_x = bs_raw[shape_off + 9]
    min_y = bs_raw[shape_off + 10]
    max_y = bs_raw[shape_off + 11]

    width = max_x - min_x + 1
    height = max_y - min_y + 1

    if width <= 0 or height <= 0 or width > 128 or height > 128:
        return None

    pitch = (height + 7) // 8
    columns = []
    total_visible = 0

    for w in range(width):
        bitmask_off = shape_off + 12 + w * pitch
        runs = []
        h = 0
        while h < height:
            while h < height:
                byte_idx = h // 8
                bit_idx = h % 8
                if bitmask_off + byte_idx < len(bs_raw) and (bs_raw[bitmask_off + byte_idx] & (1 << bit_idx)):
                    break
                h += 1
            if h >= height:
                break
            run_start = h
            while h < height:
                byte_idx = h // 8
                bit_idx = h % 8
                if bitmask_off + byte_idx >= len(bs_raw) or not (bs_raw[bitmask_off + byte_idx] & (1 << bit_idx)):
                    break
                h += 1
            run_len = h - run_start
            runs.append((run_start, run_len))
            total_visible += run_len
        columns.append(runs)

    stexels_file_off = (raw_texel_off - stexels_base) // 2
    texel_byte_count = (total_visible + 1) // 2

    if stexels_file_off < 0 or stexels_file_off + texel_byte_count > len(st_raw):
        return None

    texel_indices = []
    for b in range(texel_byte_count):
        byte = st_raw[stexels_file_off + b]
        texel_indices.append(byte & 0x0F)
        texel_indices.append((byte >> 4) & 0x0F)

    img = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    pixels = img.load()

    texel_pos = 0
    for w in range(width):
        for (y_start, run_len) in columns[w]:
            for dy in range(run_len):
                if texel_pos < len(texel_indices):
                    idx = texel_indices[texel_pos]
                    texel_pos += 1
                    if pal_off + idx < len(palettes):
                        r, g, b = palettes[pal_off + idx]
                        px = min_x + w
                        py = min_y + y_start + dy
                        if 0 <= px < 64 and 0 <= py < 64:
                            pixels[px, py] = (r, g, b, 255)

    return img


def extract_sprites(data_dir, output_dir, palettes, bitshape_offsets, sprite_ids):
    """Extract all sprites from bitshapes.bin + stexels.bin as RGBA PNGs.

    Each sprite group (e.g., a monster with all animation frames) is saved
    as a single horizontal sprite sheet PNG.
    """
    spr_dir = output_dir / "sprites"
    spr_dir.mkdir(parents=True, exist_ok=True)

    bitshapes_data = (data_dir / "bitshapes.bin").read_bytes()
    stexels_data = (data_dir / "stexels.bin").read_bytes()

    wtexels_header = (data_dir / "wtexels.bin").read_bytes()[:4]
    wtexels_data_size = struct.unpack_from('<i', wtexels_header, 0)[0]
    stexels_base = wtexels_data_size * 2

    bs_raw = bitshapes_data[4:]
    st_raw = stexels_data[4:]

    total_bitshape_entries = len(bitshape_offsets) // 2

    # Build sprite groups: find all unique base IDs from sprite_ids
    # and determine how many frames each group has
    unique_bases = sorted(set(sid for sid in sprite_ids if sid > 0))
    if 0 in sprite_ids:
        unique_bases = [0] + [b for b in unique_bases if b != 0]

    # Determine frame count per group by looking at gap to next base
    groups = []
    for idx, base in enumerate(unique_bases):
        next_base = None
        for nb in unique_bases:
            if nb > base:
                next_base = nb
                break
        if next_base is None:
            next_base = total_bitshape_entries

        frame_count = min(next_base - base, 8)

        pal_offsets = set()
        for i, sid in enumerate(sprite_ids):
            if sid == base:
                pal_offsets.add(bitshape_offsets[sid * 2 + 1])

        for pal_off in sorted(pal_offsets):
            groups.append((base, frame_count, pal_off))

    count = 0
    frame_count_total = 0

    for base, frame_count, pal_off in groups:
        # Decode all frames for this group (keep as 64x64 uncropped for uniform layout)
        frames = []
        for f in range(frame_count):
            entry_idx = base + f
            if entry_idx >= total_bitshape_entries:
                break

            shape_off = bitshape_offsets[entry_idx * 2 + 0]
            img = decode_sprite(bs_raw, st_raw, shape_off, pal_off,
                                stexels_base, palettes)
            frames.append(img)

        # Filter out None frames but keep positions
        valid_frames = [(f, img) for f, img in enumerate(frames) if img is not None]
        if not valid_frames:
            continue

        # Find uniform crop across all frames in the group
        # so they align properly in the sprite sheet
        bbox_all = [None, None, None, None]  # min_x, min_y, max_x, max_y
        full_frames = []
        for f, img in valid_frames:
            # Re-decode as full 64x64 (uncropped)
            entry_idx = base + f
            shape_off = bitshape_offsets[entry_idx * 2 + 0]
            full_img = decode_sprite_uncropped(bs_raw, st_raw, shape_off, pal_off,
                                               stexels_base, palettes)
            if full_img is None:
                continue
            full_frames.append((f, full_img))
            bb = full_img.getbbox()
            if bb:
                if bbox_all[0] is None:
                    bbox_all = list(bb)
                else:
                    bbox_all[0] = min(bbox_all[0], bb[0])
                    bbox_all[1] = min(bbox_all[1], bb[1])
                    bbox_all[2] = max(bbox_all[2], bb[2])
                    bbox_all[3] = max(bbox_all[3], bb[3])

        if not full_frames or bbox_all[0] is None:
            continue

        # Crop all frames to the same bounding box
        crop_w = bbox_all[2] - bbox_all[0]
        crop_h = bbox_all[3] - bbox_all[1]
        n = len(full_frames)

        sheet = Image.new('RGBA', (crop_w * n, crop_h), (0, 0, 0, 0))
        for i, (f, full_img) in enumerate(full_frames):
            cropped = full_img.crop(bbox_all)
            sheet.paste(cropped, (i * crop_w, 0))

        sheet.save(spr_dir / f"sprite_{base:03d}.png")
        count += 1
        frame_count_total += len(full_frames)

    print(f"  Extracted {count} sprite sheets ({frame_count_total} total frames)")


def extract_palettes(output_dir, palettes):
    """Export palette visualization as a PNG."""
    pal_dir = output_dir / "palettes"
    pal_dir.mkdir(parents=True, exist_ok=True)

    num_groups = len(palettes) // 16
    scale = 16

    img = Image.new('RGB', (16 * scale, num_groups * scale))
    pixels = img.load()

    for group in range(num_groups):
        for ci in range(16):
            idx = group * 16 + ci
            if idx < len(palettes):
                r, g, b = palettes[idx]
                for dy in range(scale):
                    for dx in range(scale):
                        pixels[ci * scale + dx, group * scale + dy] = (r, g, b)

    img.save(pal_dir / "palettes_all.png")
    print(f"  Exported {num_groups} palette groups ({len(palettes)} colors)")


def main():
    parser = argparse.ArgumentParser(description="DoomRPG Asset Extractor")
    parser.add_argument("--data-dir", default="DoomRPG",
                        help="Path to DoomRPG data directory (default: DoomRPG)")
    parser.add_argument("--output-dir", default="output",
                        help="Output directory for extracted assets (default: output)")
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    output_dir = Path(args.output_dir)

    required_files = ["palettes.bin", "mappings.bin", "bitshapes.bin",
                      "wtexels.bin", "stexels.bin"]
    for f in required_files:
        if not (data_dir / f).exists():
            print(f"Error: {data_dir / f} not found")
            return 1

    print("Loading palettes...")
    palettes = load_palettes(data_dir)

    print("Loading mappings...")
    texel_offsets, bitshape_offsets, texture_ids, sprite_ids = load_mappings(data_dir)

    print(f"Found {len(texture_ids)} textures, {len(sprite_ids)} sprites, "
          f"{len(palettes)} palette entries, "
          f"{len(bitshape_offsets)//2} total bitshape entries")

    print("\nExtracting palettes...")
    extract_palettes(output_dir, palettes)

    print("\nExtracting wall textures...")
    extract_textures(data_dir, output_dir, palettes, texel_offsets, texture_ids)

    print("\nExtracting sprites (all animation frames)...")
    extract_sprites(data_dir, output_dir, palettes, bitshape_offsets, sprite_ids)

    print(f"\nDone! Assets saved to {output_dir}/")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
