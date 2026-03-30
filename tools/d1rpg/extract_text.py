#!/usr/bin/env python3
"""
DoomRPG Text Extractor

Extracts all game text from DoomRPG binary data files:
  - BSP map strings (NPC dialogue, event messages, passwords, etc.)
  - entities.db (entity/item/monster names)
  - help.txt (game help text)

Usage:
    python extract_text.py [--data-dir DoomRPG] [--output-dir output]
"""

import argparse
import struct
from pathlib import Path

ENTITY_TYPES = {
    0: "Nothing",
    1: "Item",
    2: "Door",
    3: "NPC",
    4: "Monster",
    5: "Weapon",
    6: "Key",
}

# BSP file section sizes (per entry)
NODE_ENTRY_SIZE = 10    # 4 shiftCoords(4) + 1 byte + 1 shiftCoord + 2 shorts(4)
LINE_ENTRY_SIZE = 10    # 4 shiftCoords(4) + 1 short(2) + 1 int(4)
SPRITE_ENTRY_SIZE = 5   # 2 shiftCoords(2) + 1 byte + 1 short(2)
EVENT_ENTRY_SIZE = 4    # 1 int
BYTECODE_ENTRY_SIZE = 9 # 1 byte + 2 ints(8)


def read_byte(data, pos):
    return data[pos], pos + 1


def read_short(data, pos):
    return struct.unpack_from('<h', data, pos)[0], pos + 2


def read_int(data, pos):
    return struct.unpack_from('<i', data, pos)[0], pos + 4


def extract_bsp_strings(bsp_path):
    """Extract all strings from a BSP map file."""
    data = bsp_path.read_bytes()
    pos = 0

    # Header: 16-byte map name + 17 bytes of other header data = 33 bytes
    map_name = data[0:16].split(b'\x00')[0].decode('ascii', errors='replace')
    pos = 33  # skip full header

    # Nodes
    num_nodes, pos = read_short(data, pos)
    pos += num_nodes * NODE_ENTRY_SIZE

    # Lines
    num_lines, pos = read_short(data, pos)
    pos += num_lines * LINE_ENTRY_SIZE

    # Sprites
    num_sprites, pos = read_short(data, pos)
    pos += num_sprites * SPRITE_ENTRY_SIZE

    # Tile Events
    num_events, pos = read_short(data, pos)
    pos += num_events * EVENT_ENTRY_SIZE

    # ByteCodes
    num_bytecodes, pos = read_short(data, pos)
    pos += num_bytecodes * BYTECODE_ENTRY_SIZE

    # Strings
    num_strings, pos = read_short(data, pos)
    strings = []
    for _ in range(num_strings):
        str_size, pos = read_short(data, pos)
        text = data[pos:pos + str_size].decode('ascii', errors='replace')
        strings.append(text)
        pos += str_size

    return map_name, strings


def extract_entities_db(data_dir):
    """Extract entity names from entities.db."""
    data = (data_dir / "entities.db").read_bytes()
    pos = 0

    num_defs, pos = read_short(data, pos)
    entities = []

    for _ in range(num_defs):
        tile_index, pos = read_short(data, pos)
        e_type, pos = read_byte(data, pos)
        e_sub_type, pos = read_byte(data, pos)
        parm, pos = read_int(data, pos)
        name = data[pos:pos + 16].split(b'\x00')[0].decode('ascii', errors='replace')
        pos += 16

        type_name = ENTITY_TYPES.get(e_type, f"Type_{e_type}")
        entities.append({
            "tile_index": tile_index,
            "type": type_name,
            "sub_type": e_sub_type,
            "parm": parm,
            "name": name,
        })

    return entities


def main():
    parser = argparse.ArgumentParser(description="DoomRPG Text Extractor")
    parser.add_argument("--data-dir", default="DoomRPG",
                        help="Path to DoomRPG data directory (default: DoomRPG)")
    parser.add_argument("--output-dir", default="output",
                        help="Output directory (default: output)")
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    output_dir = Path(args.output_dir) / "text"
    output_dir.mkdir(parents=True, exist_ok=True)

    # --- BSP Map Strings ---
    bsp_files = sorted(data_dir.glob("*.bsp"))
    if bsp_files:
        print(f"Extracting strings from {len(bsp_files)} BSP maps...")
        all_map_text = []

        for bsp_path in bsp_files:
            map_name, strings = extract_bsp_strings(bsp_path)
            if not strings:
                continue

            label = bsp_path.stem
            all_map_text.append(f"{'=' * 60}")
            all_map_text.append(f"  {label}.bsp  —  \"{map_name}\"")
            all_map_text.append(f"  {len(strings)} strings")
            all_map_text.append(f"{'=' * 60}")
            all_map_text.append("")

            for i, s in enumerate(strings):
                # Replace | with newline for display (game uses | as line break)
                display = s.replace('|', '\n    ')
                all_map_text.append(f"  [{i:3d}] {display}")
            all_map_text.append("")

        out_path = output_dir / "map_strings.txt"
        out_path.write_text('\n'.join(all_map_text), encoding='utf-8')
        print(f"  Saved to {out_path}")
    else:
        print("  No BSP files found")

    # --- entities.db ---
    if (data_dir / "entities.db").exists():
        print("Extracting entities.db...")
        entities = extract_entities_db(data_dir)

        lines = []
        lines.append(f"{'Name':<20} {'Type':<10} {'SubType':>7}  {'Tile':>5}  {'Parm':>8}")
        lines.append("-" * 60)
        for e in entities:
            lines.append(f"{e['name']:<20} {e['type']:<10} {e['sub_type']:>7}  {e['tile_index']:>5}  {e['parm']:>8}")

        out_path = output_dir / "entities.txt"
        out_path.write_text('\n'.join(lines), encoding='utf-8')
        print(f"  Saved {len(entities)} entities to {out_path}")
    else:
        print("  entities.db not found")

    # --- help.txt ---
    if (data_dir / "help.txt").exists():
        print("Copying help.txt...")
        text = (data_dir / "help.txt").read_text(encoding='ascii', errors='replace')
        out_path = output_dir / "help.txt"
        out_path.write_text(text, encoding='utf-8')
        print(f"  Saved to {out_path}")

    print(f"\nDone! Text saved to {output_dir}/")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
