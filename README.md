# DRPG Engine

[![Build](https://github.com/dderevjanik/DoomIIRPG-RE/actions/workflows/build.yml/badge.svg)](https://github.com/dderevjanik/DoomIIRPG-RE/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

![logo](./logo.png)

A custom RPG turn-based engine based on the [Doom II RPG reverse engineering project by [GEC]](https://github.com/Erick194/DoomIIRPG-RE). The goal is to rebuild the original game into a moddable, open engine with YAML-driven data, a flexible asset pipeline, and an integrated map editor.

This is an early-stage project (v0.1.0). Many reverse-engineered field names still use placeholder names (e.g., `field_0x7c`). Contributions to improve naming, fix bugs, and document behavior are welcome.

## Building

### Dependencies

- CMake 3.22+
- C++23 compiler (Clang 19+ with libc++, or GCC 15+)
- SDL2
- Zlib
- OpenAL
- OpenGL

### Game Assets

You need the original `Doom 2 RPG.ipa` file placed in the project root. This file is not included in the repository.

### Linux

```bash
sudo apt-get install cmake build-essential libsdl2-dev zlib1g-dev libopenal-dev libgl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### macOS

```bash
brew install sdl2 openal-soft
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Or use the convenience script:

```bash
./build-macos.sh
```

### Windows

```bash
vcpkg install sdl2:x64-windows zlib:x64-windows openal-soft:x64-windows
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="%VCPKG_INSTALLATION_ROOT%/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release --parallel
```

## Project Structure

```
src/
├── converter/   # CLI tool: extracts assets from .ipa → YAML + binaries in games/
├── engine/      # Platform layer: SDL/GL, TinyGL renderer, input, sound, VFS, menus, scripting
└── game/        # Game logic: entities, combat, player, minigames, game states

games/doom2rpg/  # Converted game assets (YAML configs + binary data, not checked in)
tools/           # Python extraction scripts (extract_*.py) and HTML viewers
docs/            # Project documentation
├── d1-rpg/      # Doom 1 RPG binary format specs (bitshapes, BSP, palettes, etc.)
├── d2-rpg/      # Doom 2 RPG binary format specs (entities, levels, media, menus, scripting, strings, tables)
└── LEVELS.md    # Level structure documentation
Testing/         # Test scripts for automated map/minigame validation
scripts/         # Shell scripts for smoke testing
```

## Asset Pipeline

```
*.ipa  →  drpg-convert (or tools/*.py)  →  games/doom2rpg/
                                                          ├── *.yaml (entities, weapons, monsters, strings, ...)
                                                          ├── levels/maps/*.bin
                                                          ├── audio/, fonts/, hud/, ui/, ...
                                                          └── ...
```

The YAML files in `games/doom2rpg/` define game content (entities, weapons, monsters, strings, etc.) and can be edited for modding. Binary `.bin` map files should be edited through the map editor, not by hand.

## Architecture

The project is a game engine for classic mobile RPG formats, written in C++17:

- **Rendering**: SDLGL → GLES → Render → Canvas (BSP-based pipeline with TinyGL software rasterizer)
- **Game Logic**: Game, Entity, Combat, Player (entity-based game systems)
- **Scripting**: ScriptThread (bytecode execution for game scripts)
- **UI/Menu**: MenuSystem, Hud, Button (menu and HUD rendering)
- **Audio**: Sound (OpenAL-based, 10 concurrent channels)
- **Input**: Input (keyboard, mouse, and gamepad)
- **Data**: Resource, VFS, ZipFile, JavaStream (asset loading)
- **Minigames**: HackingGame, SentryBotGame, VendingMachine, ComicBook

See [docs/d2-rpg/](docs/d2-rpg/) for Doom 2 RPG binary format specs and [docs/d1-rpg/](docs/d1-rpg/) for Doom 1 RPG format specs.

## Running

```bash
./build/src/DRPGEngine --game doom2rpg
```

### Game CLI Options

| Flag                   | Description                                                    |
| ---------------------- | -------------------------------------------------------------- |
| `--game <name>`        | Game module under `games/` (e.g. `doom2rpg`)                   |
| `--gamedir <path>`     | Explicit game directory path                                   |
| `--map <path>`         | Load a specific level directory or map file (see below)        |
| `--minigame <name>`    | Launch a minigame directly (`hacking`, `sentrybot`, `vending`) |
| `--skip-intro`         | Skip the intro sequence                                        |
| `--skip-travel-map`    | Skip the travel map (auto-set when `--map` is used)            |
| `--headless`           | Run without a window (for testing)                             |
| `--ticks <n>`          | Exit after N ticks (for testing)                               |
| `--seed <n>`           | Set random seed (for reproducible runs)                        |
| `--script <path>`      | Run a test script (e.g. `Testing/test_map09.script`)           |

### Loading a Specific Level

The `--map` flag accepts a level directory registered in `game.yaml`, a path to `map.bin`, or a raw binary file:

```bash
# Level directory (resolved against game.yaml levels section)
./build/src/DRPGEngine --map levels/10_test_map

# Explicit map.bin path within a level directory
./build/src/DRPGEngine --map levels/10_test_map/map.bin

# Combined with --game
./build/src/DRPGEngine --game doom2rpg --map levels/01_tycho
```

When the path matches a registered level, the engine loads it through the full level system (textures, `level.yaml`, scripts, strings). Unrecognized paths are treated as standalone `.bin` files.

### Converter CLI Options

```bash
./build/src/converter/drpg-convert --ipa "Doom 2 RPG.ipa" --output games/doom2rpg
```

| Flag               | Description                                      |
| ------------------ | ------------------------------------------------ |
| `--ipa <path>`     | Path to `Doom 2 RPG.ipa` file                    |
| `--output <dir>`   | Output directory (default: `games/doom2rpg`)      |
| `--force`, `-f`    | Overwrite existing output                         |
| `--help`, `-h`     | Show usage                                        |

## Default Key Configuration

| Action          | Key     |
| --------------- | ------- |
| Move Forward    | W, Up   |
| Move Backward   | S, Down |
| Move Left       | A       |
| Move Right      | D       |
| Turn Left       | Left    |
| Turn Right      | Right   |
| Atk/Talk/Use    | Return  |
| Next Weapon     | Z       |
| Prev Weapon     | X       |
| Pass Turn       | C       |
| Automap         | Tab     |
| Menu Open/Back  | Escape  |
| Menu Items/Info | I       |
| Menu Drinks     | O       |
| Menu PDA        | P       |
| Bot Dis/Ret     | B       |

## Cheat Codes

J2ME/BREW version - open menu and enter these numbers:<br>
3666 -> Debug menu / Menu debug<br>
1666 -> Restart level / Reinicia el nivel<br>
4332 -> All keys, items, weapons / Todas las llaves, items y armas<br>
3366 -> Speed benchmark / Testeo de velocidad

## Editor

## Testing

### Unit Tests

No game assets required:

```bash
cmake --build build --target test-mapdata-roundtrip --parallel
ctest --test-dir build --output-on-failure
```

Verifies the MapData binary round-trip (create map → save `.bin` → reload → compare).

### Smoke Tests

Require game assets in `games/doom2rpg/` and a built binary:

```bash
scripts/test_map_loading.sh        # loads map00-map09, verifies no crash
scripts/test_minigame_loading.sh   # loads hacking, sentrybot, vending minigames
```

### Script-based Tests

Test scripts in `Testing/` can exercise specific maps with deterministic seeds:

```bash
./build/src/DRPGEngine --game doom2rpg --map levels/09_vios --script Testing/test_map09.script --seed 1337
```

## Acknowledgments

- **[GEC] (Erick194)** — Original [Doom II RPG reverse engineering project](https://github.com/Erick194/DoomIIRPG-RE) that this engine is based on
- **[TinyGL](http://www.integratedsets.com/)** — Software OpenGL rasterizer
- **[Dear ImGui](https://github.com/ocornut/imgui)** — Debug UI
- **[yaml-cpp](https://github.com/jbeder/yaml-cpp)** — YAML parsing for moddable data files
- **[hash-library](https://github.com/stbrumme/hash-library)** — Hash functions

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
