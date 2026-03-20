#!/usr/bin/env python3
"""
Extract entities.bin from DoomIIRPG .ipa and generate human-readable entities.ini.

Usage:
    python3 tools/extract_entities.py ["Doom 2 RPG.ipa"] [entities.ini]

The script reads entities.bin (gzipped) from the .ipa zip archive, decodes each
entity definition, and writes a human-readable INI file with entity type names
and string table comments.

Binary format:
    Header: int16 count (big-endian)
    Per entity (8 bytes each, big-endian):
        int16  tileIndex
        uint8  eType
        uint8  eSubType
        uint8  parm
        uint8  name        (string table index)
        uint8  longName    (string table index)
        uint8  description (string table index)

String resolution:
    The script also extracts the string index and string data files from the .ipa
    to resolve entity name/longName/description into human-readable comments.
    String IDs use group << 10 | index encoding (FILE_ENTITYSTRINGS = group 1).
"""

import gzip
import struct
import sys
import zipfile

# --- Entity type names (from Enums.h) ---
ENTITY_TYPE_NAMES = {
    0: "world",
    1: "player",
    2: "monster",
    3: "npc",
    4: "playerclip",
    5: "door",
    6: "item",
    7: "decor",
    8: "env_damage",
    9: "corpse",
    10: "attack_interactive",
    11: "monsterblock_item",
    12: "spritewall",
    13: "nonobstructing_spritewall",
    14: "decor_noclip",
}


def entity_type_name(etype):
    return ENTITY_TYPE_NAMES.get(etype, str(etype))


def load_string_table(ipa, prefix, group):
    """
    Load strings from the .ipa string table files.

    String table format:
        strings.idx: packed index entries (5 bytes each: 1 byte group/flags, 2 bytes offset_hi, 2 bytes count)
        stringsNN.bin: null-terminated strings per group

    Returns a dict mapping string_index -> string_text for the given group.
    """
    try:
        idx_data = ipa.read(prefix + "strings.idx")
    except KeyError:
        return {}

    # Parse index to find the file and offset for our group
    # The index format: each entry is 5 bytes
    # But the actual format used by the game is loadFileIndex which reads:
    #   for each group: shiftByte (file#), shiftUShort (offset), shiftUShort (count)
    # Total = groups * 5 bytes
    num_groups = len(idx_data) // 5
    strings = {}

    for g in range(num_groups):
        base = g * 5
        file_num = idx_data[base]
        offset = struct.unpack_from(">H", idx_data, base + 1)[0]
        count = struct.unpack_from(">H", idx_data, base + 3)[0]

        if g != group:
            continue

        # Load the string data file
        str_filename = f"strings{file_num:02d}.bin"
        try:
            str_data = ipa.read(prefix + str_filename)
        except KeyError:
            break

        # Read null-terminated strings starting at offset
        pos = offset
        for i in range(count):
            end = str_data.index(b'\x00', pos)
            raw = str_data[pos:end]
            # Decode, replacing soft hyphens (0xAD) with empty string
            text = raw.decode('latin-1').replace('\xad', '')
            strings[i] = text
            pos = end + 1

    return strings


def extract_entities(ipa_path, output_path):
    """Extract entities.bin from .ipa and write entities.ini."""
    prefix = "Payload/Doom2rpg.app/Packages/"

    with zipfile.ZipFile(ipa_path) as ipa:
        # entities.bin is gzip-compressed in the resource pack
        raw = ipa.read(prefix + "entities.bin")
        data = gzip.decompress(raw)

        # Load entity strings (group 1 = FILE_ENTITYSTRINGS)
        entity_strings = load_string_table(ipa, prefix, 1)

    print(f"entities.bin: {len(raw)} compressed, {len(data)} decompressed")

    # Parse header (big-endian)
    count = struct.unpack_from(">h", data, 0)[0]
    print(f"Entity count: {count}")

    # Parse entities
    entities = []
    offset = 2
    for i in range(count):
        tile_index = struct.unpack_from(">h", data, offset)[0]
        etype = data[offset + 2]
        esubtype = data[offset + 3]
        parm = data[offset + 4]
        name = data[offset + 5]
        long_name = data[offset + 6]
        description = data[offset + 7]
        entities.append((tile_index, etype, esubtype, parm, name, long_name, description))
        offset += 8

    # Write INI
    with open(output_path, "w") as f:
        f.write("[Entities]\n")
        f.write(f"count = {count}\n")
        f.write("\n")

        for i, (tile_index, etype, esubtype, parm, name, long_name, desc) in enumerate(entities):
            # Resolve string names for comments
            name_str = entity_strings.get(name, "(none)")
            long_name_str = entity_strings.get(long_name, "(none)")
            desc_str = entity_strings.get(desc, "(none)")

            f.write(f"; name: {name_str}\n")
            f.write(f"; long_name: {long_name_str}\n")
            f.write(f"; description: {desc_str}\n")
            f.write(f"[Entity_{i}]\n")
            f.write(f"tile_index = {tile_index}\n")
            f.write(f"type = {entity_type_name(etype)}\n")
            f.write(f"subtype = {esubtype}\n")
            f.write(f"parm = {parm}\n")
            f.write(f"name = {name}\n")
            f.write(f"long_name = {long_name}\n")
            f.write(f"description = {desc}\n")
            f.write("\n")

    print(f"Written {output_path}: {count} entities")


if __name__ == "__main__":
    ipa_path = sys.argv[1] if len(sys.argv) > 1 else "Doom 2 RPG.ipa"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "entities.ini"
    extract_entities(ipa_path, output_path)
