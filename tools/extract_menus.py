#!/usr/bin/env python3
"""
Extract menus.bin from DoomIIRPG .ipa and generate human-readable menus.ini.

Usage:
    python3 tools/extract_menus.py ["Doom 2 RPG.ipa"] [menus.ini]

The script reads menus.bin from the .ipa zip archive, decodes the packed
menuData and menuItems arrays, and writes a human-readable INI file.

Binary format:
    Header: int16 menuDataCount, int16 menuItemsCount
    menuData[menuDataCount]: uint32 each
        bits [7:0]   = menu_id
        bits [23:8]  = cumulative item offset (end)
        bits [31:24] = menu_type
    menuItems[menuItemsCount]: uint32 each, consumed in pairs per item
        word0 bits [31:16] = string_id
        word0 bits [15:0]  = flags
        word1 bits [7:0]   = param
        word1 bits [15:8]  = action
        word1 bits [31:16] = help_string_id
"""

import struct
import sys
import zipfile

# --- Menu ID to name mapping (from Menus.h enum menus) ---
MENU_NAMES = {
    -6: "MENU_MAIN_CONTROLLER",
    -5: "MENU_MAIN_BINDINGS",
    -4: "MENU_MAIN_CONTROLS",
    -3: "MENU_MAIN_OPTIONS_SOUND",
    -2: "MENU_MAIN_OPTIONS_VIDEO",
    -1: "MENU_MAIN_OPTIONS_INPUT",
    0: "MENU_NONE",
    1: "MENU_LEVEL_STATS",
    2: "MENU_DRAWSWORLD",
    3: "MENU_MAIN",
    4: "MENU_MAIN_HELP",
    5: "MENU_MAIN_ARMORHELP",
    6: "MENU_MAIN_EFFECTHELP",
    7: "MENU_MAIN_ITEMHELP",
    8: "MENU_MAIN_ABOUT",
    9: "MENU_MAIN_GENERAL",
    10: "MENU_MAIN_MOVE",
    11: "MENU_MAIN_ATTACK",
    12: "MENU_MAIN_SNIPER",
    13: "MENU_MAIN_EXIT",
    14: "MENU_MAIN_CONFIRMNEW",
    15: "MENU_MAIN_CONFIRMNEW2",
    16: "MENU_MAIN_DIFFICULTY",
    17: "MENU_MAIN_OPTIONS",
    18: "MENU_MAIN_MINIGAME",
    19: "MENU_MAIN_MORE_GAMES",
    20: "MENU_MAIN_HACKER_HELP",
    21: "MENU_MAIN_MATRIX_SKIP_HELP",
    22: "MENU_MAIN_POWER_UP_HELP",
    23: "MENU_SELECT_LANGUAGE",
    24: "MENU_END_RANKING",
    25: "MENU_ENABLE_SOUNDS",
    26: "MENU_END_",
    27: "MENU_END_FINALQUIT",
    28: "MENU_INHERIT_BACKMENU",
    29: "MENU_INGAME",
    30: "MENU_INGAME_STATUS",
    31: "MENU_INGAME_PLAYER",
    32: "MENU_INGAME_LEVEL",
    33: "MENU_INGAME_GRADES",
    34: "MENU_NULL01",
    35: "MENU_INGAME_OPTIONS",
    36: "MENU_INGAME_LANGUAGE",
    37: "MENU_INGAME_HELP",
    38: "MENU_INGAME_GENERAL",
    39: "MENU_INGAME_MOVE",
    40: "MENU_INGAME_ATTACK",
    41: "MENU_INGAME_SNIPER",
    42: "MENU_INGAME_EXIT",
    43: "MENU_INGAME_ARMORHELP",
    44: "MENU_INGAME_EFFECTHELP",
    45: "MENU_INGAME_ITEMHELP",
    46: "MENU_INGAME_QUESTLOG",
    47: "MENU_INGAME_RECIPES",
    48: "MENU_INGAME_SAVE",
    49: "MENU_INGAME_LOAD",
    50: "MENU_INGAME_LOADNOSAVE",
    51: "MENU_INGAME_DEAD",
    52: "MENU_INGAME_RESTARTLVL",
    53: "MENU_INGAME_SAVEQUIT",
    54: "MENU_INGAME_BINDINGS",
    55: "MENU_INGAME_OPTIONS_SOUND",
    56: "MENU_INGAME_OPTIONS_VIDEO",
    57: "MENU_INGAME_KICKING",
    58: "MENU_INGAME_SPECIAL_EXIT",
    59: "MENU_INGAME_HACKER_HELP",
    60: "MENU_INGAME_MATRIX_SKIP_HELP",
    61: "MENU_INGAME_POWER_UP_HELP",
    62: "MENU_INGAME_CONTROLS",
    63: "MENU_INGAME_OPTIONS_INPUT",
    64: "MENU_INGAME_CONTROLLER",
    65: "MENU_DEBUG",
    66: "MENU_DEBUG_MAPS",
    67: "MENU_DEBUG_STATS",
    68: "MENU_DEBUG_CHEATS",
    69: "MENU_DEVELOPER_VARS",
    70: "MENU_DEBUG_SYS",
    71: "MENU_SHOWDETAILS",
    72: "MENU_ITEMS",
    73: "MENU_ITEMS_WEAPONS",
    74: "MENU_NULL07",
    75: "MENU_ITEMS_DRINKS",
    76: "MENU_NULL08",
    77: "MENU_ITEMS_CONFIRM",
    78: "MENU_NULL09",
    79: "MENU_ITEMS_HEALTHMSG",
    80: "MENU_ITEMS_ARMORMSG",
    81: "MENU_ITEMS_SYRINGEMSG",
    82: "MENU_ITEMS_HOLY_WATER_MAX",
    83: "MENU_VENDING_MACHINE",
    84: "MENU_VENDING_MACHINE_DRINKS",
    85: "MENU_VENDING_MACHINE_SNACKS",
    86: "MENU_VENDING_MACHINE_CONFIRM",
    87: "MENU_VENDING_MACHINE_CANT_BUY",
    88: "MENU_VENDING_MACHINE_DETAILS",
    89: "MENU_COMIC_BOOK",
}

# --- Menu type names (from Menus.h) ---
MENU_TYPE_NAMES = {
    0: "default",
    1: "list",
    2: "confirm",
    3: "confirm2",
    4: "main",
    5: "help",
    6: "vcenter",
    7: "notebook",
    8: "main_list",
    9: "vending_machine",
}

# --- Action names (from Menus.h) ---
ACTION_NAMES = {
    0: "none",
    1: "goto",
    2: "back",
    3: "load",
    4: "save",
    5: "backtomain",
    6: "togsound",
    7: "newgame",
    8: "exit",
    9: "changestate",
    10: "difficulty",
    11: "returntogame",
    12: "restartlevel",
    13: "savequit",
    14: "offersuccess",
    15: "changesfxvolume",
    16: "showdetails",
    17: "changemap",
    18: "useitemweapon",
    19: "select_language",
    20: "useitemsyring",
    21: "useitemother",
    22: "continue",
    23: "main_special",
    24: "confirmuse",
    25: "saveexit",
    26: "backtwo",
    27: "minigame",
    28: "confirmbuy",
    29: "buydrink",
    30: "buysnack",
    33: "return_to_player",
    35: "flip_controls",
    36: "control_layout",
    37: "changemusicvolume",
    38: "changealpha",
    39: "change_vid_mode",
    40: "tog_vsync",
    41: "change_resolution",
    42: "apply_changes",
    43: "set_binding",
    44: "default_bindings",
    45: "tog_vibration",
    46: "change_vibration_intensity",
    47: "change_deadzone",
    48: "tog_tinygl",
    100: "debug",
    102: "giveall",
    103: "givemap",
    104: "noclip",
    105: "disableai",
    106: "nohelp",
    107: "godmode",
    108: "showlocation",
    109: "rframes",
    110: "rspeeds",
    111: "rskipflats",
    112: "rskipcull",
    114: "rskipbsp",
    115: "rskiplines",
    116: "rskipsprites",
    117: "ronlyrender",
    118: "rskipdecals",
    119: "rskip2dstretch",
    120: "driving_mode",
    121: "render_mode",
    122: "equipformap",
    123: "oneshot",
    124: "debug_font",
    125: "sys_test",
    126: "skip_minigames",
    127: "show_heap",
}

# --- Item flag names (from Menus.h) ---
FLAG_BITS = [
    (0x0001, "noselect"),
    (0x0002, "nodehyphenate"),
    (0x0004, "disabled"),
    (0x0008, "align_center"),
    (0x0020, "showdetails"),
    (0x0040, "divider"),
    (0x0080, "selector"),
    (0x0100, "block_text"),
    (0x0200, "highlight"),
    (0x0400, "checked"),
    (0x2000, "right_arrow"),
    (0x4000, "left_arrow"),
    (0x8000, "hidden"),
]


def flags_to_names(flags):
    """Convert flags bitmask to comma-separated flag names."""
    if flags == 0:
        return "normal"
    names = []
    for bit, name in FLAG_BITS:
        if flags & bit:
            names.append(name)
    return ",".join(names) if names else f"0x{flags:04x}"


def menu_name(menu_id):
    """Get human-readable menu name from ID."""
    return MENU_NAMES.get(menu_id, f"MENU_UNKNOWN_{menu_id}")


def action_name(action_id):
    """Get human-readable action name from ID."""
    return ACTION_NAMES.get(action_id, str(action_id))


def menu_type_name(type_id):
    """Get human-readable menu type name from ID."""
    return MENU_TYPE_NAMES.get(type_id, str(type_id))


def extract_menus(ipa_path, output_path):
    """Extract menus.bin from .ipa and write menus.ini."""
    prefix = "Payload/Doom2rpg.app/Packages/"

    with zipfile.ZipFile(ipa_path) as ipa:
        data = ipa.read(prefix + "menus.bin")

    print(f"menus.bin: {len(data)} bytes")

    # Parse header
    menu_data_count, menu_items_count = struct.unpack_from("<HH", data, 0)
    print(f"menuDataCount: {menu_data_count}, menuItemsCount: {menu_items_count}")

    # Parse menuData array
    offset = 4
    menu_data = []
    for i in range(menu_data_count):
        val = struct.unpack_from("<I", data, offset)[0]
        menu_data.append(val)
        offset += 4

    # Parse menuItems array
    menu_items = []
    for i in range(menu_items_count):
        val = struct.unpack_from("<I", data, offset)[0]
        menu_items.append(val)
        offset += 4

    print(f"Parsed {menu_data_count} menu entries, {menu_items_count} item words")

    # Write INI
    with open(output_path, "w") as f:
        f.write("; menus.ini - Generated from menus.bin\n")
        f.write("; Human-readable menu definitions for DoomIIRPG\n")
        f.write(";\n")
        f.write("; Menu types: default=0, list=1, confirm=2, confirm2=3, main=4,\n")
        f.write(";             help=5, vcenter=6, notebook=7, main_list=8, vending_machine=9\n")
        f.write(";\n")
        f.write("; Item flags: normal=0, noselect=1, nodehyphenate=2, disabled=4,\n")
        f.write(";             align_center=8, showdetails=32, divider=64, selector=128,\n")
        f.write(";             block_text=256, highlight=512, checked=1024,\n")
        f.write(";             right_arrow=8192, left_arrow=16384, hidden=32768\n")
        f.write(";\n")
        f.write("; String IDs reference FILE_MENUSTRINGS group (Strings::FILE_MENUSTRINGS = 3)\n")
        f.write("\n")

        f.write("[Menus]\n")
        f.write(f"count = {menu_data_count}\n")
        f.write("\n")

        for i, md in enumerate(menu_data):
            mid = md & 0xFF
            # menu_id is stored as unsigned byte but some IDs in the enum are negative
            # In the binary, negative IDs would wrap: -1 = 255, -2 = 254, etc.
            # Check if this is a high value that maps to a negative enum
            if mid >= 250:
                signed_id = mid - 256
            else:
                signed_id = mid

            item_end = (md & 0xFFFF00) >> 8
            mtype = (md & 0xFF000000) >> 24
            item_start = (menu_data[i - 1] & 0xFFFF00) >> 8 if i > 0 else 0
            num_items = (item_end - item_start) // 2

            mname = menu_name(signed_id)
            f.write(f"; {mname}\n")
            f.write(f"[Menu_{i}]\n")
            f.write(f"menu_id = {signed_id}\n")
            f.write(f"type = {menu_type_name(mtype)}\n")
            f.write(f"item_count = {num_items}\n")

            for j in range(num_items):
                idx = item_start + j * 2
                w0 = menu_items[idx]
                w1 = menu_items[idx + 1]

                string_id = (w0 >> 16) & 0xFFFF
                flags = w0 & 0xFFFF
                action = (w1 >> 8) & 0xFF
                param = w1 & 0xFF
                help_string = (w1 >> 16) & 0xFFFF

                f.write(f"\n")
                f.write(f"[Menu_{i}_Item_{j}]\n")
                f.write(f"string_id = {string_id}\n")
                f.write(f"flags = {flags_to_names(flags)}\n")
                f.write(f"action = {action_name(action)}\n")
                f.write(f"param = {param}\n")
                if help_string != 0:
                    f.write(f"help_string = {help_string}\n")

            f.write("\n")

    print(f"Written {output_path}: {menu_data_count} menus")


if __name__ == "__main__":
    ipa_path = sys.argv[1] if len(sys.argv) > 1 else "Doom 2 RPG.ipa"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "menus.ini"
    extract_menus(ipa_path, output_path)
