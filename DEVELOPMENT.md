# Development Guide

## Prerequisites

- CMake 3.22+
- C++23 compiler (Clang 19+ with libc++, or GCC 15+)
- SDL2
- Zlib
- OpenAL
- OpenGL
- Game asset: original `.ipa` file placed in project root

## IDE Setup

### VSCode

The project generates `compile_commands.json` automatically during CMake configure. Point your C/C++ extension to it:

```json
{
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

### CLion

Open the root `CMakeLists.txt` as a project. CLion will auto-detect the CMake configuration.

## Project Structure

```
drpg-engine/
  src/                  # All source and header files (flat layout)
  docs/                 # Binary format documentation
    ENTITY_FORMAT.md    # Entity definition and instance formats
    LEVEL_FORMAT.md     # Map file format (BSP, tiles, entities, scripts)
    MEDIA_FORMAT.md     # Texture, palette, and sprite formats
    MENU_FORMAT.md      # Menu UI binary format
    STRING_FORMAT.md    # String table / localization format
    TABLE_FORMAT.md     # Data table format
  cmake/Modules/        # Custom CMake find modules (SDL2)
  third_party_libs/     # Vendored dependencies (hash-library)
  .github/workflows/    # CI/CD configuration
```

## Architecture Overview

The codebase is a game engine for classic mobile RPG formats. Key components:

### Core Systems
- **CAppContainer** - Singleton application container, owns all subsystems
- **App (Applet)** - Main application loop, initialization, resource loading
- **SDLGL** - SDL2/OpenGL window management and platform abstraction
- **Input** - Keyboard, mouse, gamepad input mapping

### Rendering Pipeline
- **GLES** - OpenGL ES abstraction layer
- **Render** - BSP-based world renderer (32x32 tile maps, 64 units/tile)
- **TinyGL** - Software rasterization pipeline
- **Canvas** - High-level rendering, UI drawing, sprite/texture management
- **Graphics** - Drawing primitives

### Game Systems
- **Game** - Core game state, entity management, level loading
- **Entity / EntityDef** - Entity system with definitions and instances
- **Combat / CombatEntity** - Turn-based combat system
- **Player** - Player stats, inventory, movement
- **ScriptThread** - Bytecode interpreter for game scripts

### UI
- **MenuSystem** - Full menu system (main menu, inventory, PDA, etc.)
- **Hud** - In-game HUD rendering
- **Text** - Text rendering and localization

### Data
- **Resource** - Asset loading from IPA archive
- **ZipFile** - ZIP archive reading
- **JavaStream** - Java serialization format parsing (original game was J2ME)
- **Image** - Sprite and texture management

## Known Limitations

- Many fields still have reverse-engineered placeholder names (`field_0x7c`, etc.)
- No automated tests
- Memory management uses raw `new`/`delete` (no smart pointers)
- Large monolithic files: Canvas.cpp (~8K LOC), MenuSystem.cpp (~5.8K LOC)
- Windows build may require vcpkg for dependencies
