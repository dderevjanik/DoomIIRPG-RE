# D2RPG Menu File Format (menus.bin)

This document describes the binary format of the D2RPG menu definition file based on reverse engineering of the game code.

## Overview

- **File Name**: `menus.bin`
- **Location**: `Doom2rpg.app/Packages/`
- **Purpose**: Defines all game menus and their items (main menu, in-game menus, vending machine, options, etc.)

The file contains two arrays: a menu descriptor array that maps menu IDs to their items, and a menu items array that defines each item's text, flags, action, and parameters.

## Data Types

All multi-byte values are stored in **little-endian** format.

| Type | Size | Description |
|------|------|-------------|
| `short` | 2 | Signed 16-bit integer |
| `int` | 4 | Signed 32-bit integer |

## File Structure

### Header (4 bytes)

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | short | menuDataCount | Number of entries in the menu descriptor array |
| 2 | short | menuItemsCount | Number of entries in the menu items array |

### Menu Descriptor Array (4 bytes per entry)

Immediately follows the header. Contains `menuDataCount` entries of type `int`, each packed as:

```
[  type  | itemEndIndex |  menuId  ]
  8 bits     16 bits       8 bits
```

| Bits | Mask | Field | Description |
|------|------|-------|-------------|
| 0-7 | 0x000000FF | menuId | Menu identifier (matches `Menus::MENU_*` constants) |
| 8-23 | 0x00FFFF00 | itemEndIndex | End index (exclusive) into the menu items array |
| 24-31 | 0xFF000000 | type | Menu display type (see Menu Types below) |

The item start index for a menu is derived from the previous entry's `itemEndIndex` (or 0 for the first entry). Each menu's items span from `prevItemEndIndex` to `itemEndIndex`, with 2 ints per item.

### Menu Items Array (4 bytes per entry)

Follows the menu descriptor array. Contains `menuItemsCount` entries of type `int`. Items are consumed in pairs — each menu item uses 2 consecutive entries:

**First int (n7):**

| Bits | Mask | Field | Description |
|------|------|-------|-------------|
| 0-15 | 0x0000FFFF | flags | Item flags (see Item Flags below) |
| 16-31 | 0xFFFF0000 | textIndex | String index for item label (in `FILE_MENUSTRINGS` string group) |

**Second int (n8):**

| Bits | Mask | Field | Description |
|------|------|-------|-------------|
| 0-7 | 0x000000FF | param | Action parameter |
| 8-15 | 0x0000FF00 | action | Action to execute on selection |
| 16-31 | 0xFFFF0000 | helpIndex | String index for help/description text |

## Menu Types

| Value | Constant | Description |
|-------|----------|-------------|
| 1 | MENUTYPE_LIST | Standard scrollable list |
| 2 | MENUTYPE_CONFIRM | Yes/No confirmation dialog |
| 3 | MENUTYPE_CONFIRM2 | Alternative confirmation dialog |
| 4 | MENUTYPE_MAIN | Main menu layout |
| 5 | MENUTYPE_HELP | Help screen layout |
| 6 | MENUTYPE_VCENTER | Vertically centered layout |
| 7 | MENUTYPE_NOTEBOOK | Notebook/journal layout |
| 8 | MENUTYPE_MAIN_LIST | Main menu with list |
| 9 | MENUTYPE_VENDING_MACHINE | Vending machine layout |

## Item Flags

| Value | Constant | Description |
|-------|----------|-------------|
| 0 | ITEM_NORMAL | Standard selectable item |
| 1 | ITEM_NOSELECT | Cannot be selected |
| 2 | ITEM_NODEHYPHENATE | Do not dehyphenate text |
| 4 | ITEM_DISABLED | Greyed out / disabled |
| 8 | ITEM_ALIGN_CENTER | Center-aligned text |
| 32 | ITEM_SHOWDETAILS | Shows detail panel on select |
| 64 | ITEM_DIVIDER | Visual separator line |
| 128 | ITEM_SELECTOR | Selection indicator |
| 256 | ITEM_BLOCK_TEXT | Block text rendering mode |
| 512 | ITEM_HIGHLIGHT | Highlighted text |
| 1024 | ITEM_CHECKED | Checkmark indicator |
| 8192 | ITEM_RIGHT_ARROW | Right arrow indicator |
| 16384 | ITEM_LEFT_ARROW | Left arrow indicator |
| 32768 | ITEM_HIDDEN | Hidden from display |

## Loading Behavior

Menus are loaded from the binary file at startup. At runtime, `loadMenuItems(menu, begItem, numItems)` looks up a menu by scanning the descriptor array for a matching `menuId`, then loads items starting at `begItem` for `numItems` count (-1 = all remaining items).

Items are often loaded partially or in segments — the code frequently calls `loadMenuItems` with different ranges and inserts additional dynamic items between segments (e.g., volume sliders, key bindings, inventory counts).

## Source References

This format was reverse-engineered from:
- `src/MenuSystem.cpp` — `startup()` file parsing, `loadMenuItems()` item loading
- `src/MenuSystem.h` — `MenuSystem` class with `menuData`/`menuItems` arrays
- `src/MenuItem.h` — `MenuItem` class fields
- `src/Menus.h` — Menu type constants, item flag constants, and menu ID constants
