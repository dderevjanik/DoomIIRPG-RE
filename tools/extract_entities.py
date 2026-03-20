#!/usr/bin/env python3
"""
Extract entities.bin from DoomIIRPG .ipa and generate human-readable entities.ini.

Usage:
    python3 tools/extract_entities.py ["Doom 2 RPG.ipa"] [entities.ini]

The script reads entities.bin from the .ipa zip archive, decodes each entity
definition, and writes a human-readable INI file with entity type names and
string table comments.

Binary format (little-endian):
    Header: int16 count
    Per entity (8 bytes each):
        int16  tileIndex
        uint8  eType
        uint8  eSubType
        uint8  parm
        uint8  name        (string table index)
        uint8  longName    (string table index)
        uint8  description (string table index)

String resolution:
    The script also extracts the string index (strings.idx) and string data
    files (strings00.bin, etc.) from the .ipa to resolve entity name/longName/
    description into human-readable comments. Soft hyphens (0xAD) are removed.
"""

import re
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


def load_string_table(ipa, prefix, group_index):
    """
    Load strings from the .ipa string table files.

    Replicates the engine's Resource::loadFileIndex() logic:
        - strings.idx: int16 group_count, then per group: uint8 file#, int32 packed_offset
        - File# 0xFF is a sentinel (marks end of a file's groups)
        - String count per group is computed from offset differences
        - stringsNN.bin: raw null-terminated strings

    Returns a dict mapping string_index -> string_text for the given group.
    """
    try:
        idx_data = ipa.read(prefix + "strings.idx")
    except KeyError:
        return {}

    # Replicate loadFileIndex
    total_groups = struct.unpack_from("<h", idx_data, 0)[0]
    array = [0] * (total_groups * 3 + 10)

    off = 2
    n4 = 0
    for _ in range(total_groups):
        fb = idx_data[off]
        fi = struct.unpack_from("<i", idx_data, off + 1)[0]
        off += 5

        if fi != 0:
            array[(n4 * 3) - 1] = fi - array[(n4 * 3) - 2]
        if fb != 0xFF:
            array[(n4 * 3) + 0] = fb   # file number
            array[(n4 * 3) + 1] = fi   # byte offset
            n4 += 1

    # Final sentinel
    fb = idx_data[off]
    fi = struct.unpack_from("<i", idx_data, off + 1)[0]
    array[(n4 * 3) - 1] = fi - array[(n4 * 3) - 2]

    if group_index >= n4:
        return {}

    file_num = array[group_index * 3 + 0]
    byte_offset = array[group_index * 3 + 1]
    byte_size = array[group_index * 3 + 2]

    str_filename = f"strings{file_num:02d}.bin"
    try:
        str_data = ipa.read(prefix + str_filename)
    except KeyError:
        return {}

    # Read null-terminated strings in the byte range
    strings = {}
    pos = byte_offset
    end_pos = byte_offset + byte_size
    idx = 0
    while pos < end_pos:
        nul = str_data.index(b'\x00', pos)
        raw = str_data[pos:nul]
        # Decode and remove soft hyphens: the game stores word-break hints
        # as regular hyphens (0x2D) between letters within words.
        # Remove single hyphens between word characters, keep -- (em-dash).
        text = raw.decode('latin-1')
        text = re.sub(r'(?<=\w)-(?=\w)', '', text)
        strings[idx] = text
        pos = nul + 1
        idx += 1

    return strings


def extract_entities(ipa_path, output_path):
    """Extract entities.bin from .ipa and write entities.ini."""
    prefix = "Payload/Doom2rpg.app/Packages/"

    with zipfile.ZipFile(ipa_path) as ipa:
        data = ipa.read(prefix + "entities.bin")

        # Load entity strings (group 1 = FILE_ENTITYSTRINGS)
        entity_strings = load_string_table(ipa, prefix, 1)

    print(f"entities.bin: {len(data)} bytes")
    print(f"Entity strings loaded: {len(entity_strings)} strings")

    # Parse header (little-endian)
    count = struct.unpack_from("<h", data, 0)[0]
    print(f"Entity count: {count}")

    # Parse entities
    entities = []
    offset = 2
    for i in range(count):
        tile_index = struct.unpack_from("<h", data, offset)[0]
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

            if not name_str:
                name_str = "(none)"
            if not long_name_str:
                long_name_str = "(none)"
            if not desc_str:
                desc_str = "(none)"

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
