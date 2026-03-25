\_# Agents

Guidelines for AI agents working on this codebase.

## Overview

- Don't directly edit games/doom2rpg/ assets. They are converted from original game files and should remain unchanged for reference. Instead
edit `src/converter` scripts to modify how assets are processed.

## After bigger refactors

When completing a significant refactor (engine changes, asset pipeline, entity system, rendering, level loading), always run the map loading smoke test to verify nothing is broken:

```bash
scripts/test_map_loading.sh
```

This launches the game with each map file (`map00.bin` - `map09.bin`) and verifies it doesn't crash during startup. It requires proprietary game assets in `games/doom2rpg/` and a built binary (`cmake --build build`).

If any map fails, investigate and fix before committing.
