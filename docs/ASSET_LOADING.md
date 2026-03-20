# Asset Loading Data Flow

## Overview

The engine uses a Virtual File System (VFS) with priority-based mounts. At startup, two sources are mounted:

| Mount        | Priority | Source                         |
|--------------|----------|--------------------------------|
| `basedata/`  | 100      | Loose files (modding overlay)  |
| `.ipa` zip   | 0        | Original packaged assets       |

When a file is requested, VFS checks mounts in priority order — modded files in `basedata/` override originals.

For data assets (tables, entities, menus, strings), the engine first checks for `.ini` files in the current working directory. If not found, it falls back to `.bin` files loaded through VFS.

## Startup Sequence

```
main.cpp
  │
  ├─ Parse CLI args (--ipa, --gamedir)
  ├─ Open .ipa as zip archive
  ├─ Mount VFS: basedata/ (pri 100), .ipa (pri 0)
  ├─ Init SDL / OpenGL
  │
  └─ CAppContainer::Construct()
       └─ Applet::startup()                          [App.cpp:43]
            │
            ├─ 1. Canvas::startup()                   — back buffer (480x320)
            ├─ 2. Localization::startup()              — strings
            ├─ 3. Render::startup()                    — renderer
            ├─ 4. Resource::initTableLoading()         — table init
            │     └─ Applet::loadTables()              — game data tables
            ├─ 5. TinyGL::startup()                    — OpenGL context
            ├─ 6. EntityDefManager::startup()          — entity definitions
            ├─ 7. Player::startup()                    — player data
            ├─ 8. MenuSystem::startup()                — UI menus
            ├─ 9. Sound::startup()                     — audio
            ├─10. Game::startup()                      — game logic
            ├─11. ParticleSystem::startup()            — particles
            └─12. Combat::startup()                    — combat system
```

## Asset Loading Detail

### Strings (step 2)

```
Localization::startup()                              [Text.cpp:24]
  ├─ Try: load "strings.ini" from CWD
  │    └─ Localization::loadFromINI()                [Text.cpp:116]
  │         └─ loadGroupFromINI() per lang/group     [Text.cpp:137]
  │
  └─ Fallback: load binary via VFS
       ├─ Read "strings.idx"      — index (languages, group offsets)
       └─ Read "strings00.bin"    — string data for selected language
            "strings01.bin"
            "strings02.bin"
```

**Binary files:** `strings.idx`, `strings00.bin` – `strings02.bin`
**INI file:** `strings.ini` — sections `[Lang_X_Group_Y]`
**Loaded:** 5 languages, 15 groups. Groups 0, 1, 3, 14 loaded eagerly; rest lazy.

---

### Tables (step 4)

```
Applet::loadTables()                                 [App.cpp:411]
  ├─ Try: load "tables.ini" from CWD
  │    └─ Applet::loadTablesFromINI()                [App.cpp:532]
  │
  └─ Fallback: load "tables.bin" via VFS
       └─ Binary: 20x int32 offset header, then 14 tables
```

**Binary file:** `tables.bin`
**INI file:** `tables.ini`
**14 tables loaded into subsystems:**

| # | Table            | Target subsystem |
|---|------------------|------------------|
| 0 | MonsterAttacks   | Combat           |
| 1 | WeaponInfo       | Combat           |
| 2 | Weapons          | Combat           |
| 3 | MonsterStats     | Combat           |
| 4 | CombatMasks      | Combat           |
| 5 | KeysNumeric      | Canvas           |
| 6 | OSCCycle         | Canvas           |
| 7 | LevelNames       | Game             |
| 8 | MonsterColors    | Render           |
| 9 | SinTable         | Render           |
|10 | EnergyDrinkData  | Game             |
|11 | MonsterWeakness  | Combat           |
|12 | MonsterSounds    | Combat           |
|13 | MovieEffects     | Game             |

---

### Entity Definitions (step 6)

```
EntityDefManager::startup()                          [EntityDef.cpp:63]
  ├─ Try: load "entities.ini" from CWD
  │    └─ EntityDefManager::loadFromINI()            [EntityDef.cpp:109]
  │
  └─ Fallback: load "entities.bin" via VFS
       └─ EntityDefManager::loadFromBinary()         [EntityDef.cpp:81]
            └─ int16 count + 8-byte records
```

**Binary file:** `entities.bin` (1522 bytes)
**INI file:** `entities.ini`
**Record:** tileIndex (int16), eType (uint8), eSubType (uint8), parm (uint8), name/longName/description (uint8 string refs)

---

### Menus (step 8)

```
MenuSystem::startup()                                [MenuSystem.cpp:44]
  ├─ Try: load "menus.ini" from CWD
  │    └─ MenuSystem::loadFromINI()
  │
  └─ Fallback: load "menus.bin" via VFS
       └─ Binary: header (menuDataCount, menuItemsCount)
                  + packed uint32 menu entries
                  + packed uint32 item pairs
```

**Binary file:** `menus.bin` (1996 bytes)
**INI file:** `menus.ini`

---

### Textures (step 3 — Render::startup)

```
Render::startup()
  └─ Loaded on demand via Resource system
       ├─ "newMappings.bin"    — tile mappings (18432 bytes)
       ├─ "newPalettes.bin"    — RGB565 palettes (335932 bytes)
       └─ "newTexels000.bin"   — texture pixel data
          through
          "newTexels038.bin"      (39 files)
```

**No INI equivalent** — these are raw binary image data.

---

### Maps (on demand — Game::startup / level load)

```
Game loads maps on level transition:
  └─ "map00.bin" through "map09.bin"   — 10 levels
```

---

### 3D Models (on demand)

```
"model_0000.bin" through "model_0009.bin"   — 10 models
```

---

### BMP Images (on demand)

```
Applet::loadImage()                                  [App.cpp:333]
  └─ Applet::createImage()                           [App.cpp:169]
       └─ BMP parser (4-bit/8-bit indexed → RGB565)
```

340+ `.bmp` files for UI, fonts, backgrounds, sprites. Loaded through VFS as needed.

---

### Sounds (step 9)

```
Sound::startup()
  └─ Loaded via VFS with LOADTYPE_SOUND_RESOURCE
       └─ Prefixed with "sounds2/" path
```

## File I/O Layer

```
InputStream::loadFile()                              [JavaStream.cpp:67]
  ├─ LOADTYPE_RESOURCE (5)       → VFS::readFile()
  ├─ LOADTYPE_FILE (6)           → direct fopen from Doom2rpg.app/
  └─ LOADTYPE_SOUND_RESOURCE (7) → VFS::readFile() with "sounds2/" prefix

VFS::readFile()                                      [VFS.cpp:76]
  └─ iterate mounts by priority (high → low)
       ├─ Directory mount → readFromDir()   (fopen/fread)
       └─ Zip mount      → readFromZip()   (ZipFile)
```

## Extraction Pipeline (tools/)

These Python scripts extract `.bin` → `.ini` from the original `.ipa`:

| Script                | Input                          | Output          |
|-----------------------|--------------------------------|-----------------|
| `extract_tables.py`   | `tables.bin`                   | `tables.ini`    |
| `extract_strings.py`  | `strings.idx` + `stringsXX.bin`| `strings.ini`   |
| `extract_entities.py` | `entities.bin`                 | `entities.ini`  |
| `extract_menus.py`    | `menus.bin`                    | `menus.ini`     |
| `extract_textures.py` | `newMappings/Palettes/Texels`  | PNG files       |

Usage: `python3 tools/extract_<type>.py [path/to/ipa] [output.ini]`

## Summary Diagram

```
  .ipa (original)          basedata/ (mods)         CWD (ini overrides)
       │                        │                         │
       └──── VFS (pri 0) ──────┘── VFS (pri 100) ───────┘
                    │                                     │
                    ▼                                     ▼
              .bin loading                         .ini loading
              (binary format)                      (human-readable)
                    │                                     │
                    └────────────┬─────────────────────────┘
                                 ▼
                          Engine subsystems
                    ┌────────────────────────┐
                    │ Localization (strings)  │
                    │ Combat (tables 0-4,11-12)│
                    │ Canvas (tables 5-6)     │
                    │ Game (tables 7,10,13)   │
                    │ Render (tables 8-9)     │
                    │ EntityDefManager        │
                    │ MenuSystem              │
                    │ Sound                   │
                    └────────────────────────┘
```
