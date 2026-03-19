# DoomIIRPG-RE

![image](https://github.com/Erick194/DoomIIRPG-RE/assets/41172072/6249f7bd-18e6-4838-b1ec-8654d18cc1b4)<br>
https://www.doomworld.com/forum/topic/135602

Doom II RPG Reverse Engineering by [GEC] / Ingenieria inversa por [GEC]<br />
Created by / Creado por Erick Vasquez Garcia.

Current version / Version actual: **0.1.0**

License: GPL v3

## Building

### Dependencies

- CMake 3.22+
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

## Architecture

The project reconstructs the original Doom II RPG iOS game engine in C++17:

- **Rendering**: SDLGL -> GLES -> Render -> Canvas (BSP-based rendering pipeline with TinyGL software rasterizer)
- **Game Logic**: Game, Entity, Combat, Player (entity-based game systems)
- **Scripting**: ScriptThread (bytecode execution for game scripts)
- **UI/Menu**: MenuSystem, Hud, Button (menu and HUD rendering)
- **Audio**: Sound (OpenAL-based audio with 10 concurrent channels)
- **Input**: Input (keyboard, mouse, and gamepad support)
- **Data**: Resource, ZipFile, JavaStream (asset loading from IPA archive)
- **Minigames**: HackingGame, SentryBotGame, VendingMachine, ComicBook

See [docs/](docs/) for detailed binary format specifications.

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

## Status

This is an early-stage reverse engineering project (v0.1.0). Many reverse-engineered field names still use placeholder names (e.g., `field_0x7c`). Contributions to improve naming, fix bugs, and document behavior are welcome.
