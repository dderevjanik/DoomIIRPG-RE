# Doom II RPG Scripting System

The game uses a stack-based bytecode virtual machine to drive level scripting — cutscenes, dialogs, puzzles, entity behavior, item drops, and all dynamic gameplay events. Each level embeds its own bytecode program in the map file.

**Source files:**

| File | Purpose |
|------|---------|
| `src/game/ScriptThread.h` | VM thread state |
| `src/game/ScriptThread.cpp` | VM interpreter and all 98 opcode handlers |
| `src/game/Enums.h` | Opcode constants, eval operators, event flags |
| `src/engine/OpcodeRegistry.h` | Extension opcode registry (128-254) |
| `src/game/Game.cpp` | Thread pool management, state variable updates |

---

## Architecture Overview

- **VM type**: Stack-based bytecode interpreter
- **Thread pool**: Up to 20 concurrent `ScriptThread` instances (`Game::scriptThreads[20]`)
- **Bytecode storage**: `Render::mapByteCode` (section 17 of the level file, see [LEVEL_FORMAT.md](LEVEL_FORMAT.md))
- **Entry points**: Tile events (section 16) and static functions (section 15)
- **Opcodes**: 98 built-in (0-97), 127 extension slots (128-254), 255 reserved for end marker

---

## Thread Lifecycle

### State

| Field | Type | Description |
|-------|------|-------------|
| `IP` | int | Instruction pointer — current bytecode offset |
| `FP` | int | Frame pointer — stack base of current call frame |
| `stackPtr` | int | Next free stack slot |
| `scriptStack[16]` | int[16] | Operand/call stack |
| `state` | int | 0 = inactive, 2 = initialized/active |
| `unpauseTime` | int | Game time to resume (-1 = waiting for external event, 0 = ready) |
| `inuse` | bool | Slot is allocated |
| `type` | int | Trigger type flags (copied from tile event) |
| `flags` | int | Bit 0: block input |

### Execution flow

1. **Allocation** — `alloc(eventIndex, type, blockInput)` extracts the bytecode IP from the tile event's upper 16 bits, initializes the stack with a sentinel frame: `push(-1)` (return address) then `push(0)` (old FP).
2. **Run** — `run()` calls `updateScriptVars()`, then loops: read opcode at `mapByteCode[IP]`, dispatch via switch, advance IP. Returns:
   - `0` — error/fatal
   - `1` — script completed
   - `2` — paused (waiting for time, dialog, animation, etc.)
3. **Resume** — `attemptResume(gameTime)` checks `unpauseTime`: if `-1` stays paused (external trigger needed); if `gameTime >= unpauseTime`, calls `run()` to continue.
4. **Completion** — When `evReturn()` pops a return address of `-1`, the script is done. Thread is freed back to the pool.

### Serialization

`saveState`/`loadState` persist: `unpauseTime` (relative to gameTime), `state`, `IP`, `FP`, and `scriptStack[0..FP-1]`.

---

## Bytecode Encoding

Bytecode is stored as a `uint8_t[]` array. Multi-byte arguments are **big-endian** (note: differs from the level file header which uses little-endian).

### Instruction format

```
[opcode:1B] [arguments:variable]
```

After the switch dispatches on the opcode byte, the handler reads arguments which advance IP. After the handler returns, IP is incremented by 1 (to skip past the opcode byte itself).

### Argument readers

| Function | Bytes | Result | Encoding |
|----------|-------|--------|----------|
| `getByteArg()` | 1 | uint8_t | `mapByteCode[++IP]` |
| `getUByteArg()` | 1 | unsigned short | `mapByteCode[++IP] & 0xFF` |
| `getShortArg()` | 2 | signed short | `(mapByteCode[IP+1] << 8) \| (mapByteCode[IP+2] & 0xFF)`, IP += 2 |
| `getUShortArg()` | 2 | unsigned int | `((mapByteCode[IP+1] & 0xFF) << 8) \| (mapByteCode[IP+2] & 0xFF)`, IP += 2 |
| `getIntArg()` | 4 | signed int | Big-endian 32-bit from IP+1..IP+4, IP += 4 |

### Stack operations

- `push(n)` — writes `n` to `scriptStack[stackPtr++]`
- `pop()` — returns `scriptStack[--stackPtr]`
- Stack depth is 16 elements, limiting call nesting

---

## Call Stack

`EV_CALL_FUNC` (opcode 7) implements function calls:

1. Read target IP (ushort)
2. Save current FP: `oldFP = FP`
3. Set `FP = stackPtr`
4. `push(currentIP)` — return address
5. `push(oldFP)` — caller's frame pointer
6. Set `IP = targetIP - 1` (will be incremented to targetIP after dispatch)

`EV_RETURN` (opcode 2) unwinds:

1. Pop stack down to `FP + 2`
2. `FP = pop()` — restore caller's frame pointer
3. `IP = pop()` — restore return address
4. If IP == -1, script has completed (top-level return)

---

## Triggering: Tile Events

Tiles with map flag bit 6 (`0x40`) have script events. Events are stored as pairs of 32-bit ints in `tileEvents[]` (section 16 of the level file).

### First int (packed)

| Bits | Mask | Description |
|------|------|-------------|
| 0-9 | `0x3FF` | Tile index in 32x32 grid |
| 16-31 | `0xFFFF0000` >> 16 | Bytecode IP (instruction pointer into `mapByteCode`) |

### Second int (event flags)

**Trigger type** (bits 0-3):

| Bit | Constant | Description |
|-----|----------|-------------|
| 0 | `EVFL_EXEC_ENTER` (1) | Player enters tile |
| 1 | `EVFL_EXEC_EXIT` (2) | Player exits tile |
| 2 | `EVFL_EXEC_TRIGGER` (4) | Player interacts/uses |
| 3 | `EVFL_EXEC_FACE` (8) | Player faces tile |

**Direction modifiers** (bits 4-11):

| Bit | Value | Constant | Direction |
|-----|-------|----------|-----------|
| 4 | 0x010 | `EVFL_MOD_EAST` | East (+x) |
| 5 | 0x020 | `EVFL_MOD_NORTHEAST` | Northeast |
| 6 | 0x040 | `EVFL_MOD_NORTH` | North (-y) |
| 7 | 0x080 | `EVFL_MOD_NORTHWEST` | Northwest |
| 8 | 0x100 | `EVFL_MOD_WEST` | West (-x) |
| 9 | 0x200 | `EVFL_MOD_SOUTHWEST` | Southwest |
| 10 | 0x400 | `EVFL_MOD_SOUTH` | South (+y) |
| 11 | 0x800 | `EVFL_MOD_SOUTHEAST` | Southeast |

**Attack type** (bits 12-14):

| Bit | Value | Constant | Description |
|-----|-------|----------|-------------|
| 12 | 0x1000 | `EVFL_MOD_MELEE` | Melee attack |
| 13 | 0x2000 | `EVFL_MOD_RANGED` | Ranged attack |
| 14 | 0x4000 | `EVFL_MOD_EXPLOSION` | Explosion |

**Flags** (bits 16+):

| Bit | Value | Constant | Description |
|-----|-------|----------|-------------|
| 16 | 0x10000 | `EVFL_FLAG_BLOCKINPUT` | Block player input during execution |
| 17 | 0x20000 | `EVFL_FLAG_EXIT_GOTO` | — |
| 18 | 0x40000 | `EVFL_FLAG_SKIP_TURN` | Skip turn advancement |
| 19 | 0x80000 | `EVFL_FLAG_DISABLE` | Event starts disabled (toggled by `EV_EVENTOP`) |

### Matching logic

An event fires when ALL conditions are met:
1. Not disabled (bit 19 clear)
2. Trigger type matches (bits 0-3 AND with current action)
3. Direction matches (bits 4-11 AND with player direction)
4. Attack type matches (bits 12-14 AND), OR both are zero (non-combat)

Multiple events can exist for the same tile; all matching events execute sequentially.

---

## Triggering: Static Functions

12 level-wide entry points stored as `ushort[12]` (section 15 of the level file). Value `0xFFFF` (`SCR_NOT_DEFINED`) means not defined for this level.

| Index | Constant | Trigger |
|-------|----------|---------|
| 0 | `SCR_INIT_MAP` | Map load |
| 1 | `SCR_END_GAME` | End-game sequence |
| 2 | `SCR_BOSS_75` | Boss health < 75% |
| 3 | `SCR_BOSS_50` | Boss health < 50% |
| 4 | `SCR_BOSS_25` | Boss health < 25% |
| 5 | `SCR_BOSS_DEAD` | Boss killed |
| 6 | `SCR_PER_TURN` | Each game turn |
| 7 | `SCR_ATTACK_NPC` | Player attacks NPC |
| 8 | `SCR_MONSTER_DEATH` | Any monster dies |
| 9 | `SCR_MONSTER_ACTIVATE` | Monster activates |
| 10 | `SCR_CHICKEN_KICKED` | Chicken entity kicked |
| 11 | `SCR_ITEM_PICKUP` | Item picked up |

Called via `Game::executeStaticFunc(index)` which allocates a thread and sets IP to `staticFuncs[index]`.

---

## Expression Evaluator (EV_EVAL)

Opcode 0 implements a stack-based conditional expression evaluator.

### Bytecode format

```
[0x00] [count:ubyte] [token...] [skip_offset:ubyte]
```

- **count**: Number of tokens to process
- **tokens**: Evaluated in sequence, manipulating the stack
- **skip_offset**: If the final stack result is 0 (false), IP advances by this offset (conditional skip)

### Token encoding

Each token is a single byte with these interpretations:

| Condition | Meaning | Stack effect |
|-----------|---------|--------------|
| Bit 7 set (`& 0x80`) | Variable reference: index = `byte & 0x7F` | Push `scriptStateVars[index]` |
| Bit 6 set (`& 0x40`) | Constant: reads 1 more byte; 14-bit signed = `((byte & 0x3F) << 8 \| nextByte) << 18 >> 18` | Push constant value |
| Neither bit set | Operator (see table below) | Pop operand(s), push result |

### Operators

| Value | Constant | Operation | Stack |
|-------|----------|-----------|-------|
| 0 | `EVAL_AND` | Logical AND | Pop a, pop b → push `(a==1 && b==1)` |
| 1 | `EVAL_OR` | Logical OR | Pop a, pop b → push `(a==1 \|\| b==1)` |
| 2 | `EVAL_LTE` | Less or equal | Pop a, pop b → push `(b <= a)` |
| 3 | `EVAL_LT` | Less than | Pop a, pop b → push `(b < a)` |
| 4 | `EVAL_EQ` | Equal | Pop a, pop b → push `(a == b)` |
| 5 | `EVAL_NEQ` | Not equal | Pop a, pop b → push `(a != b)` |
| 6 | `EVAL_NOT` | Negation | Pop a → push `(a == 0 ? 1 : 0)` |

### Example

To test `scriptStateVars[5] == 3`:
```
EV_EVAL, 3,            // 3 tokens follow
  0x85,                // var[5]  (0x80 | 5)
  0x40, 0x03,          // const 3 (0x40 | 0, then 0x03)
  0x04,                // EVAL_EQ
  skip_offset           // bytes to skip if false
```

---

## State Variables

`Game::scriptStateVars[128]` is a shared array of `short` values accessible to all script threads.

### Auto-updated variables

These are refreshed by `updateScriptVars()` before each `run()` call:

| Index | Source | Description |
|-------|--------|-------------|
| 0 | `player->statusEffects[33]` | Active status effects bitfield |
| 1 | `player->ce->getStat(0)` | Player health |
| 2 | `canvas->viewX >> 6` | Player tile X |
| 3 | `canvas->viewY >> 6` | Player tile Y |
| 8 | `player->inventory[24]` | Currency (UAC Credits) |
| 14 | `player->characterChoice` | Character selection |
| 16 | `player->isFamiliar ? 1 : 0` | Controlling sentry bot |

### Special variables

| Index | Description |
|-------|-------------|
| 7 | Last opcode result value (`n2`), auto-set after each non-control-flow opcode when `b` flag is true |

### General purpose

Indices 4-6, 9-13, 15, 17-127 are general purpose. Set via `EV_SETSTATE`, `EV_PREVSTATE`, `EV_NEXTSTATE`, and read via `EV_EVAL` variable references.

---

## Opcode Reference

All 98 built-in opcodes (0-97). Arguments listed in read order. Types: `B` = getByteArg (1B signed), `UB` = getUByteArg (1B unsigned), `S` = getShortArg (2B signed), `US` = getUShortArg (2B unsigned), `I` = getIntArg (4B signed).

> **Note**: Opcodes 63-64 are undefined (gap in the enum). Stubs (no-op): 39, 44, 46, 54, 55, 62, 65.

### Control Flow

| # | Name | Args | Description |
|---|------|------|-------------|
| 0 | `EV_EVAL` | B count, [tokens...], UB skip | Expression evaluator (see [above](#expression-evaluator-ev_eval)) |
| 1 | `EV_JUMP` | US offset | Unconditional relative jump: `IP += offset` |
| 2 | `EV_RETURN` | — | Return from function call (or complete script) |
| 7 | `EV_CALL_FUNC` | US ip | Call function at bytecode address `ip` |
| 14 | `EV_WAIT` | UB time | Pause script for `time * 100` ms |
| 52 | `EV_ADVANCETURN` | — | End current combat turn, advance game turn |

### State Variables

| # | Name | Args | Description |
|---|------|------|-------------|
| 6 | `EV_SETSTATE` | B index, S value | Set `scriptStateVars[index] = value` |
| 26 | `EV_PREVSTATE` | B index | Decrement `scriptStateVars[index]` |
| 27 | `EV_NEXTSTATE` | B index | Increment `scriptStateVars[index]` |
| 8 | `EV_ITEM_COUNT` | US packed, B stateIndex | Query inventory. Packed: bits 0-4 = type (0=item, 1=weapon, 2=ammo), bits 5-9 = subtype. Stores count in `scriptStateVars[stateIndex]` |
| 9 | `EV_TILE_EMPTY` | US packed, B stateIndex | Check tile for blocking entities. Packed: bits 0-4 = X, bits 5-9 = Y. Stores 1 (empty) or 0 in `scriptStateVars[stateIndex]` |
| 10 | `EV_WEAPON_EQUIPPED` | B stateIndex | Store current weapon ID in `scriptStateVars[stateIndex & 0x7F]` |
| 23 | `EV_EVENTOP` | US packed | Enable/disable tile event. Bit 15: 1=disable, 0=enable. Bits 0-14: event index |

### Messages & Dialog

| # | Name | Args | Description |
|---|------|------|-------------|
| 3 | `EV_MESSAGE` | US packed | Show HUD message. Bits 0-14: string ID. Bit 15: 1=priority message |
| 12 | `EV_CAMERA_STR` | US packed, US time | Show cinematic subtitle. Packed bits 0-13: string ID, bit 14: show player, bit 15: 0=subtitle/1=title. Time in ms |
| 13 | `EV_DIALOG` | UB stringID, UB packed | Start dialog. Packed: high nibble = dialog count, low nibble = dialog type (1=combat end, 2=help, 4=weapon select, 6/7=combat end). Pauses script |
| 38 | `EV_NPCCHAT` | US packed | Set NPC chat mode. Bits 14-15: param, bits 0-13: sprite index |
| 49 | `EV_SPEECHBUBBLE` | US texID, UB colorIndex | Show speech bubble with texture and color |
| 66 | `EV_DEBUGPRINT` | B mode, ... | Debug output. Mode 0: reads null-terminated string (UB chars). Mode 1: reads UB stateVar index. Chains if next opcode is also 66 |

### Map & Navigation

| # | Name | Args | Description |
|---|------|------|-------------|
| 11 | `EV_CHANGE_MAP` | UB packed, US spawnData | Level transition. Packed: bits 0-3 = target map, bits 4-6 = spawn facing, bit 7 = fade. SpawnData: bits 0-9 = spawn tile index |
| 15 | `EV_GOTO` | US packed | Teleport player. Bits 0-4: Y tile, bits 5-9: X tile, bits 10-13: facing (15=keep), bit 14: animate, bit 15: advance turn |
| 16 | `EV_ABORT_MOVE` | — | Cancel current player movement |
| 42 | `EV_MARKTILE` | US packed | Mark tile special. Bits 0-4: Y, bits 5-9: X, bits 10-15: flags (bit 0=monster clip, bit 1=ladder, bit 2-3=map flags, bit 4=exit, bit 5=entrance) |
| 82 | `EV_UNMARKTILE` | US packed | Remove tile marks. Same bit layout as `EV_MARKTILE` |
| 69 | `EV_TURN_PLAYER` | B packed | Rotate player. Bits 0-2: direction (×128° angle), bit 3+: 1=animate rotation, 0=instant |
| 71 | `EV_JOURNAL_TILE` | UB questID, UB tileX, UB tileY | Set quest objective tile position (tileX/Y masked to 5 bits) |
| 85 | `EV_GIVE_AUTOMAP` | — | Reveal entire automap |
| 87 | `EV_UNHIDE_AUTOMAP` | B x1, B y1, B x2, B y2 | Clear automap hidden flag for rectangle (x1,y1)-(x2,y2) |
| 88 | `EV_HIDE_AUTOMAP` | B x1, B y1, B x2, B y2 | Set automap hidden flag for rectangle (x1,y1)-(x2,y2) |
| 89 | `EV_PORTAL_EVENT` | B packed | Control portal. Low nibble: state (4=unscripted), high nibble: previous state |
| 90 | `EV_PITCH_CONTROL` | US packed | Add/remove pitch-lock tile. Bits 0-2: flags, bits 3-7: Y, bits 8-12: X, bit 13: 0=add, 1=remove |
| 67 | `EV_GOTO_MENU` | UB menuID | Navigate to menu screen |

### Entity Manipulation

| # | Name | Args | Description |
|---|------|------|-------------|
| 17 | `EV_ENTITY_FRAME` | UB sprite, UB frame, UB time | Set sprite animation frame. Time = `time * 100` ms wait |
| 24 | `EV_HIDE` | UB sprite | Hide sprite and unlink entity. Handles corpse creation for monsters |
| 34 | `EV_NAMEENTITY` | UB sprite, UB stringID | Set entity display name from map string table |
| 22 | `EV_MONSTERFLAGOP` | UB sprite, UB packed | Modify monster flags. Packed: bits 6-7 = operation (0=add, 1=remove, 2=set), bits 0-5 = flag bit index |
| 28 | `EV_WAKEMONSTER` | UB sprite | Activate sleeping monster, reset animation frame, trigger AI |
| 47 | `EV_RESPAWN_MONSTER` | US sprite, UB tileX, UB tileY | Resurrect dead monster at new position. Sets n2=-1 on failure |
| 51 | `EV_AIGOAL` | US packed, B goalParam | Set AI goal. Packed: bits 12-15 = goal type, bits 0-11 = sprite index |
| 72 | `EV_MAKE_CORPSE` | US sprite, UB tileX, UB tileY | Convert living monster to corpse at position |
| 92 | `EV_ENTITY_BREATHES` | UB sprite, UB mode | Set breathing animation: 0=enable idle breathing, 1=disable |
| 36 | `EV_SETDEATHFUNC` | UB sprite, S funcIP | Attach death callback script (-1 to remove) |
| 83 | `EV_ASSIGN_LOOTSET` | US sprite (& 0xFFF), UB count, [US lootEntry]×count | Assign loot table entries to entity |

### Items & Inventory

| # | Name | Args | Description |
|---|------|------|-------------|
| 25 | `EV_DROPITEM` | US packed, UB entityDefIndex | Spawn item on ground. Packed: bits 0-4 = X, bits 5-9 = Y, bits 10-14 = quantity |
| 33 | `EV_GIVEITEM` | UB a, UB b, B mode | Give item. Mode 0: sprite index = `(a<<8)\|b`, touch entity. Mode != 0: a=entityDef index, b=quantity |
| 35 | `EV_DROPMONSTERITEM` | US packed, UB entityDef/sprite, [UB extra], UB quantity | Drop item from monster position. Bit 15 of packed: extended sprite (reads extra byte) |
| 41 | `EV_GIVELOOT` | (inline via composeLootDialog) | Show loot pickup dialog. Reads: UB count, then per-item: US packed (bits 12-15=type, bits 6-11=subtype, bits 0-5=quantity). Pauses script |
| 73 | `EV_INVENTORY_OP` | B operation | 0=strip for VIOS battle, 1=strip for target practice, 2=restore inventory |
| 60 | `EV_DISABLED_WEAPONS` | S bitmask | Disable weapons by bitmask. Auto-switches if current weapon is disabled |

### Combat

| # | Name | Args | Description |
|---|------|------|-------------|
| 19 | `EV_DAMAGEMONSTER` | UB sprite, B damage | Deal damage to monster by sprite index. Triggers death if health <= 0 |
| 20 | `EV_DAMAGEPLAYER` | B damage, B armorDmg, B direction | Damage player. Negative damage = heal. Direction: 0-7 for HUD indicator, -1 for none |
| 29 | `EV_SHOW_PLAYERATTACK` | B weaponID | Show weapon attack animation during cinematic |
| 56 | `EV_PLAYERATTACK` | US packed | Execute player attack. Bits 12-15: weapon, bits 0-11: target sprite. Pauses 1 tick |
| 93 | `EV_DESTROY_PLAYER` | B mode | Kill player instantly (full health damage). Mode 0: no familiar remains |
| 91 | `EV_USED_CHAINSAW` | — | Award chainsaw achievement |

### Sprite Animation

| # | Name | Args | Description |
|---|------|------|-------------|
| 4 | `EV_LERPSPRITE` | 3×UB packed, [UB z], [UB time] | Move sprite to tile. Packed (24 bits): bits 14-21 = sprite, bits 9-13 = X, bits 4-8 = Y, bits 0-3 = flags. Optional Z (if not DEFAULT_Z flag), optional time in 100ms units (if not NO_TIME flag). Flags: 1=async, 2=block, 4=no time, 8=default Z |
| 59 | `EV_LERPSPRITEOFFSET` | UB sprite, UB time, I packed | Move sprite by world-coordinate offset. Packed: bits 0-10 = dstY, bits 11-21 = dstX, bits 22-23 = flags, bits 24-31 = Z offset (-48) |
| 61 | `EV_LERPSCALE` | US packed1, US travelTime, UB scale | Scale sprite. Packed1: bits 4+ = sprite, bits 0-3 = flags. Scale = `value << 1` |
| 75 | `EV_LERPSPRITEPARABOLA` | I packed, US travelTime | Parabolic arc movement. Packed: bits 22-31 = sprite, bits 17-21 = X, bits 12-16 = Y, bits 4-11 = height(-48), bits 0-3 = flags |
| 96 | `EV_LERPSPRITEPARABOLA_SCALE` | I packed, US travelTime, UB scale | Parabolic arc with scale change. Same packed layout as opcode 75, plus destination scale = `value << 1` |
| 40 | `EV_LERPFLAT` | UB (unused), US (unused) | Stub — reads and discards args |

### Cinematics & Camera

| # | Name | Args | Description |
|---|------|------|-------------|
| 5 | `EV_STARTCINEMATIC` | B cameraIndex | Start Maya camera animation. Sets state to ST_CAMERA |
| 18 | `EV_ADV_CAMERAKEY` | UB resumeCount | Advance to next camera keyframe. Pauses until animation reaches next key |
| 68 | `EV_START_INTERCINEMATIC` | UB packed | Start non-blocking camera. Uses `packed & 0x7F` as camera index. Sets ST_INTER_CAMERA |

### Visual Effects

| # | Name | Args | Description |
|---|------|------|-------------|
| 30 | `EV_MONSTER_PARTICLES` | UB sprite | Spawn blood particles on monster |
| 31 | `EV_SPAWN_PARTICLES` | UB packed, US posData, UB z | Spawn particles. Packed: bit 7 = world coords mode, bits 3-6 = count, bits 0-2 = color index. World coords: posData bits 11-15 = X tile, bits 6-10 = Y tile. Sprite mode: posData = sprite index |
| 32 | `EV_FADEOP` | US packed | Screen fade. Bit 15: 1=fade-in, 0=fade-out. Bits 0-14: duration in ms |
| 48 | `EV_SCREEN_SHAKE` | US packed | Screen shake. Bits 14-15: intensity, bits 7-13: X amplitude, bits 0-6: Y amplitude |
| 57 | `EV_SET_FOG_COLOR` | I color | Set fog color (ARGB 32-bit) |
| 58 | `EV_LERP_FOG` | I packed | Interpolate fog. Bits 0-10: start distance, bits 11-21: end distance, bits 22-29: duration in 100ms units |
| 76 | `EV_TOGGLE_OVERLAY` | — | Toggle cockpit HUD overlay |
| 77 | `EV_FOG_AFFECTS_SKYMAP` | B enabled | Set whether fog affects skybox rendering (0/1) |
| 79 | `EV_SET_MM_RENDER_HACK` | B enabled | Mastermind rendering workaround (0/1) |
| 97 | `EV_SET_CALDEX_RENDER_HACK` | B enabled | Caldex rendering workaround (0/1) |

### Sound

| # | Name | Args | Description |
|---|------|------|-------------|
| 37 | `EV_PLAYSOUND` | UB resID, UB packed | Play sound. Resource = `resID + 1000`. Packed: high nibble = flags, low nibble = priority |
| 95 | `EV_STOPSOUND` | UB resID | Stop sound. Resource = `resID + 1000` |

### Door Operations

| # | Name | Args | Description |
|---|------|------|-------------|
| 21 | `EV_DOOROP` | US packed | Door operation. Bits 0-9: sprite index, bits 10-11: operation (0=open, 1=close, 2=lock, 3=unlock), bit 12: silent mode |

### Game Mechanics

| # | Name | Args | Description |
|---|------|------|-------------|
| 43 | `EV_UPDATEJOURNAL` | UB questID, UB state | Update quest journal. State 0 = new quest (shows help notification) |
| 45 | `EV_PLAYER_ADD_STAT` | UB packed | Modify player stat. Bits 5-7: stat type (strength, IQ, etc.), bits 0-4: signed delta (5-bit, sign-extended) |
| 50 | `EV_AWARDSECRET` | — | Award secret found achievement |
| 70 | `EV_STATUS_EFFECT` | UB packed, [B intensity] | Apply/remove status effect. Bit 7: 1=remove (bits 0-6 = effect ID). Bit 7=0: apply effect, reads intensity byte |
| 74 | `EV_END_GAME` | — | Trigger game completion and epilogue |
| 78 | `EV_ENABLE_HELP` | B enabled | Enable/disable help system (0/1) |
| 81 | `EV_FORCE_BOT_RETURN` | — | Force sentry bot to return to player |
| 86 | `EV_ANGER_VIOS` | B enabled | Make VIOS hostile (0/1) |

### Minigames

| # | Name | Args | Description |
|---|------|------|-------------|
| 53 | `EV_MINIGAME` | B type, B param1, B param2, B param3 | Start minigame. Type: 0=sentry bot, 2=hacking, 4=vending machine. Pauses script |
| 80 | `EV_START_ARMORREPAIR` | — | Start armor repair minigame. Pauses script |
| 84 | `EV_START_TARGETPRACTICE` | US packed | Start target practice. Bits 8-12: X, bits 3-7: Y, bits 0-2: facing. Pauses script |
| 94 | `EV_START_TREADMILL` | — | Start treadmill minigame |

### Unimplemented / Stubs

| # | Name | Notes |
|---|------|-------|
| 39 | `EV_STOCKSTATION` | No-op |
| 44 | `EV_BRIBE_ENTITY` | No-op |
| 46 | `EV_PLAYER_ADD_RECIPE` | No-op |
| 54 | `EV_ENDMINIGAME` | No-op |
| 55 | `EV_ENDROUND` | No-op |
| 62 | `EV_GIVEAWARD` | No-op |
| 63-64 | — | Not defined in enum (gap) |
| 65 | `EV_STARTMIXING` | No-op |

---

## Extension Opcode System (128-254)

Game modules can register custom opcodes via `OpcodeRegistry` without modifying the core interpreter.

### Range

- **128-254**: Available for extension opcodes (127 slots)
- **255**: Reserved for `EV_END` (script termination marker)

### Handler signature

```cpp
typedef int (*OpcodeHandler)(ScriptThread* thread);
// Returns: 0 = continue, 1 = done, 2 = paused
```

### Registration

```cpp
OpcodeRegistry registry;
registry.registerOpcode(128, myHandler, "MY_OPCODE");
```

### Dispatch

In the `run()` switch default case, if the opcode byte is >= 128, the VM looks up the handler:

```
OpcodeHandler handler = opcodeRegistry.getHandler(opcode);
if (handler) {
    int result = handler(this);
    // 0 = continue loop, 1 = return done, 2 = pause
} else {
    Error("Cannot handle event: %d", opcode);
}
```

Handlers read arguments via the same `getByteArg()` / `getShortArg()` / etc. methods available through the `ScriptThread*` pointer.

---

## Lerp Sprite Flags

Shared flag constants used by `EV_LERPSPRITE`, `EV_LERPSPRITEOFFSET`, `EV_LERPSCALE`, `EV_LERPSPRITEPARABOLA`, and `EV_LERPSPRITEPARABOLA_SCALE`:

| Value | Constant | Description |
|-------|----------|-------------|
| 1 | `SCRIPT_LS_FLAG_ASYNC` | Non-blocking — script continues while animation plays |
| 2 | `SCRIPT_LS_FLAG_BLOCK` | Blocking — script waits for animation to complete |
| 3 | `SCRIPT_LS_FLAG_ASYNC_BLOCK` | Both flags (async start, waits for completion) |
| 4 | `SCRIPT_LS_NO_TIME` | No duration specified (instant move) |
| 8 | `SCRIPT_LS_DEFAULT_Z` | Use default Z height (32) instead of reading Z argument |
