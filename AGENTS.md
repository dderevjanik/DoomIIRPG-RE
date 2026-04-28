# Agents

Guidelines for AI agents working on this codebase.

## About

This project is based on the Doom II RPG reverse engineering effort and aims to build a custom RPG turn-based engine. The engine is designed to be moddable, with YAML-driven data and a flexible asset pipeline.

## Project Structure

- **`src/converter/`** — CLI tool that extracts and converts assets from the original `.ipa` into `games/doom2rpg/`. Parses zip/binary formats and emits YAML + raw assets. Source YAML templates live in `src/converter/resources/`.
- **`src/engine/`** — Platform/rendering layer: SDL/GL backend, TinyGL software renderer, input, sound, resource/VFS, text, HUD, menus, scripting, and particle system.
- **`src/game/`** — Game logic: entities, combat, player, maps, minigames (hacking, sentry bot, vending), and game states (logo, credits, automap).
- **`test/`** — Unit tests (CMake/CTest).
- **`tools/`** — Python extraction scripts (`extract_*.py`, `convert_doom1rpg.py`, `generate_*.py`) and HTML viewers (`map-viewer.html`, `model-viewer.html`, `menu-editor.html`).
- **`games/doom2rpg/`** — Generated game assets (YAML + binaries). **Do not hand-edit** — see Asset Pipeline below.

## Build

**Dependencies:** CMake 3.22+, SDL2, OpenAL, OpenGL, Zlib. yaml-cpp and Dear ImGui are fetched automatically.

```bash
cmake -B build
cmake --build build --parallel
```

macOS shortcut: `./build-macos.sh`

**Targets:**
- `DRPGEngine` — main game executable
- `drpg-convert` — asset converter CLI

CI runs via `.github/workflows/build.yml`.

## Testing

**Unit tests** (no game assets required):
```bash
ctest --test-dir build --output-on-failure
```

**Smoke tests** (require game assets in `games/doom2rpg/` + built binary):
```bash
scripts/test_map_loading.sh
scripts/test_minigame_loading.sh
```

**E2E scenario tests** (step-paced scripted inputs + state assertions):
```bash
scripts/run_e2e_tests.sh              # windowed (default, more stable)
scripts/run_e2e_tests.sh --headless   # headless (faster; deeper flows still unstable)
```
Each scenario is a `.script` file in `Testing/scenarios/` using a step-paced DSL: `do <ACTION>` dispatches input only when the engine is ready (state interactive, previous input consumed); `await <STATE>` blocks until that state is reached; `expect <key> <op> <value>` is a sticky assertion that waits for the condition. Per-step default timeout is 600 ticks; on timeout the engine prints `STUCK_IN: ...` and exits 2. Engine flags go in `# args:` comment lines. Keys: `state`, `health`, `damage_dealt`, `combat_turns`, `kills`. See [README.md](README.md) for the full grammar.

Run a single scenario manually:
```bash
./build/src/DRPGEngine --game doom2rpg --script Testing/scenarios/02_combat_map10.script
```

After any change to engine, core, or converter code, verify the game still runs:
```bash
./build/src/DRPGEngine --game doom2rpg --headless --map levels/maps/map00.bin
```

After bigger refactors (engine, asset pipeline, entity system, rendering, level loading), always run `scripts/test_map_loading.sh` and `scripts/run_e2e_tests.sh` before committing.

## Asset Pipeline

`*.ipa` → `drpg-convert` (or `tools/*.py`) → YAML + binary assets in `games/doom2rpg/`

- **Do NOT edit `.yaml` files in `games/`** — they are generated output from the converter. To change game data, edit the source YAML templates in `src/converter/resources/` and re-run the converter.
- **Binary `.bin` map files** should not be hand-edited — use the converter.

## Documentation

Project documentation lives in `docs/`:

- **`docs/`** — `LEVELS.md`, `SPRITES.md` and general project docs.
- **`docs/d1-rpg/`** — Doom 1 RPG format specs and reference material.
- **`docs/d2-rpg/`** — Doom 2 RPG binary format specs (entities, levels, media, menus, BSP, scripting, strings, tables). Read these before modifying format-related code.

## Code Style

- `.clang-format` enforced: LLVM base, tabs for indentation (width 4), 120 column limit.
- C++23.

## Codebase Quirks

- Many reverse-engineered placeholder field names (`field_0x7c`) — only rename when meaning is confirmed.
- Large monolithic files: `Canvas.cpp` (~8K LOC), `MenuSystem.cpp` (~5.8K LOC) — be careful with broad changes in these.
- Raw `new`/`delete` memory management throughout (no smart pointers).
