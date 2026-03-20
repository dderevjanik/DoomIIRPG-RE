#!/usr/bin/env python3
"""
Extract string tables from DoomIIRPG .ipa and generate human-readable strings.ini.

Usage:
    python3 tools/extract_strings.py ["Doom 2 RPG.ipa"] [strings.ini]

The script reads strings.idx and strings00-02.bin from the .ipa zip archive,
decodes all string groups for all available languages, and writes a single
human-readable INI file.

Binary format (little-endian):
    strings.idx:
        int16 entry_count
        Per entry (5 bytes): uint8 chunk_id, int32 offset
        chunk_id=0xFF is a sentinel (skip entry, used to compute prev group's size)
        Final sentinel entry after all entries
    stringsNN.bin:
        Raw null-terminated strings packed contiguously

Index addressing:
    (textGroup + language * MAXTEXT) * 3 -> [chunk_id, offset, size]
    MAXTEXT = 15, Languages: 0=english, 1=french, 2=german, 3=italian, 4=spanish

String counts per group (hardcoded in engine):
    0:253 1:183 2:14 3:399 4:186 5:155 6:136 7:158 8:162 9:100 10:97 11:40 12:45 13:15 14:3
"""

import struct
import sys
import zipfile

MAXTEXT = 15

LANGUAGE_NAMES = ["english", "french", "german", "italian", "spanish"]
MAX_LANGUAGES = 5

GROUP_NAMES = [
    "CodeStrings",      # 0
    "EntityStrings",    # 1
    "FileStrings",      # 2
    "MenuStrings",      # 3
    "M_01",             # 4
    "M_02",             # 5
    "M_03",             # 6
    "M_04",             # 7
    "M_05",             # 8
    "M_06",             # 9
    "M_07",             # 10
    "M_08",             # 11
    "M_09",             # 12
    "M_TEST",           # 13
    "Translations",     # 14
]

TEXT_COUNTS = [253, 183, 14, 399, 186, 155, 136, 158, 162, 100, 97, 40, 45, 15, 3]


def parse_file_index(idx_data):
    """
    Replicate Resource::loadFileIndex() to parse strings.idx.
    Returns a flat array where entry at (group + lang*MAXTEXT)*3 gives [chunk, offset, size].
    """
    total_entries = struct.unpack_from("<h", idx_data, 0)[0]
    # Over-allocate for all possible language*group combos
    array = [0] * (MAX_LANGUAGES * MAXTEXT * 3 + 10)

    off = 2
    n4 = 0
    for _ in range(total_entries):
        fb = idx_data[off]
        fi = struct.unpack_from("<i", idx_data, off + 1)[0]
        off += 5

        if fi != 0:
            array[(n4 * 3) - 1] = fi - array[(n4 * 3) - 2]
        if fb != 0xFF:
            array[(n4 * 3) + 0] = fb   # chunk file number
            array[(n4 * 3) + 1] = fi   # byte offset
            n4 += 1

    # Final sentinel
    fi = struct.unpack_from("<i", idx_data, off + 1)[0]
    array[(n4 * 3) - 1] = fi - array[(n4 * 3) - 2]

    return array, n4


def escape_string(s):
    """
    Escape special bytes for INI storage.
    Keeps text human-readable but preserves special game characters.
    """
    result = []
    for ch in s:
        code = ord(ch)
        if code == 0x0A:  # actual newline
            result.append("\\n")
        elif code == 0x0D:  # carriage return
            result.append("\\r")
        elif code == 0x09:  # tab
            result.append("\\t")
        elif 0x80 <= code <= 0x9F:
            # Game's special characters — use hex escape
            result.append(f"\\x{code:02X}")
        elif code == 0xA0:  # HARD_SPACE
            result.append("\\xA0")
        elif code == 0xAD:  # soft hyphen
            result.append("\\xAD")
        else:
            result.append(ch)
    return "".join(result)


def extract_strings(ipa_path, output_path):
    """Extract string tables from .ipa and write strings.ini."""
    prefix = "Payload/Doom2rpg.app/Packages/"

    with zipfile.ZipFile(ipa_path) as ipa:
        idx_data = ipa.read(prefix + "strings.idx")

        # Load all chunk files
        chunks = {}
        for i in range(3):
            fname = f"strings{i:02d}.bin"
            try:
                chunks[i] = ipa.read(prefix + fname)
                print(f"{fname}: {len(chunks[i])} bytes")
            except KeyError:
                print(f"{fname}: not found")

    print(f"strings.idx: {len(idx_data)} bytes")

    array, num_real_groups = parse_file_index(idx_data)
    print(f"Index entries: {num_real_groups}")

    # Discover which languages have data
    languages_with_data = []
    for lang in range(MAX_LANGUAGES):
        has_data = False
        for grp in range(MAXTEXT):
            idx_pos = (grp + lang * MAXTEXT) * 3
            size = array[idx_pos + 2]
            if size > 0:
                has_data = True
                break
        if has_data:
            languages_with_data.append(lang)

    print(f"Languages with data: {[LANGUAGE_NAMES[l] for l in languages_with_data]}")

    # Extract all strings
    with open(output_path, "w", encoding="utf-8") as f:
        f.write("; strings.ini - Generated from strings.idx + strings00-02.bin\n")
        f.write("; Human-readable string tables for DoomIIRPG\n")
        f.write(";\n")
        f.write("; Special character escapes:\n")
        f.write(";   \\x80 = line separator    \\x84 = cursor2      \\x85 = ellipsis\n")
        f.write(";   \\x87 = checkmark         \\x88 = mini-dash    \\x89 = grey line\n")
        f.write(";   \\x8A = cursor            \\x8B = shield       \\x8D = heart\n")
        f.write(";   \\x90 = pointer           \\xA0 = hard space\n")
        f.write(";   | = newline (in-game)     - = soft hyphen (word-break hint)\n")
        f.write(";\n")
        f.write("; String IDs in code: STRINGID(group, index) = (group << 10) | index\n")
        f.write("\n")
        f.write("[Strings]\n")
        f.write(f"languages = {len(languages_with_data)}\n")
        f.write(f"groups = {MAXTEXT}\n")
        f.write("\n")

        # Write group names as reference
        for i, name in enumerate(GROUP_NAMES):
            f.write(f"; group {i} = {name} (count={TEXT_COUNTS[i]})\n")
        f.write("\n")

        total_strings = 0
        for lang in languages_with_data:
            for grp in range(MAXTEXT):
                idx_pos = (grp + lang * MAXTEXT) * 3
                chunk_id = array[idx_pos + 0]
                offset = array[idx_pos + 1]
                size = array[idx_pos + 2]

                if size <= 0:
                    continue

                if chunk_id not in chunks:
                    continue

                data = chunks[chunk_id][offset:offset + size]

                # Count null terminators to get the true string count
                # Each string (including empty ones) ends with \x00
                num_strings = data.count(b"\x00")
                raw_strings = data.split(b"\x00")[:num_strings]

                section = f"Lang_{lang}_Group_{grp}"
                lang_name = LANGUAGE_NAMES[lang]
                grp_name = GROUP_NAMES[grp]

                f.write(f"; {lang_name} / {grp_name}\n")
                f.write(f"[{section}]\n")
                f.write(f"count = {len(raw_strings)}\n")

                for i, raw in enumerate(raw_strings):
                    text = raw.decode("latin-1")
                    escaped = escape_string(text)
                    f.write(f"{i} = {escaped}\n")

                f.write("\n")
                total_strings += len(raw_strings)

    print(f"Written {output_path}: {total_strings} strings across {len(languages_with_data)} language(s)")


if __name__ == "__main__":
    ipa_path = sys.argv[1] if len(sys.argv) > 1 else "Doom 2 RPG.ipa"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "strings.ini"
    extract_strings(ipa_path, output_path)
