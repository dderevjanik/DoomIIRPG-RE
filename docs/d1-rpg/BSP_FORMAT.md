# DoomRPG BSP Map File Format

This document describes the binary format of `.bsp` map files used in DoomRPG.

## Overview

BSP files contain all map data including geometry (nodes, lines), sprites, events, bytecode scripts, strings, blockmap flags, and plane textures. The data is read sequentially using little-endian byte ordering.

## Byte Reading Functions

The engine uses these functions to read data from the binary file:

| Function | Size | Description |
|----------|------|-------------|
| `byteAtNext` | 1 byte | Reads unsigned byte |
| `shortAtNext` | 2 bytes | Reads signed 16-bit integer (little-endian: `[low][high]`) |
| `intAtNext` | 4 bytes | Reads signed 32-bit integer (little-endian: `[b0][b1][b2][b3]`) |
| `shiftCoordAt` | 1 byte | Reads byte and shifts left by 3 (`value << 3`) for coordinate scaling |

### Byte Order (Little-Endian)

```
short: (data[pos+1] << 8) | data[pos+0]
int:   (data[pos+3] << 24) | (data[pos+2] << 16) | (data[pos+1] << 8) | data[pos+0]
```

---

## File Structure

```
+---------------------------+
| Header                    |
+---------------------------+
| Nodes                     |
+---------------------------+
| Lines                     |
+---------------------------+
| Sprites                   |
+---------------------------+
| Tile Events               |
+---------------------------+
| ByteCodes                 |
+---------------------------+
| Strings                   |
+---------------------------+
| BlockMap                  |
+---------------------------+
| Plane Textures            |
+---------------------------+
```

---

## Section Details

### 1. Header (Offset 0)

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0x00 | 16 | char[16] | mapName | Null-terminated map name string |
| 0x10 | 1 | byte | floorColorR | Floor color - Red component |
| 0x11 | 1 | byte | floorColorG | Floor color - Green component |
| 0x12 | 1 | byte | floorColorB | Floor color - Blue component |
| 0x13 | 1 | byte | ceilingColorR | Ceiling color - Red component |
| 0x14 | 1 | byte | ceilingColorG | Ceiling color - Green component |
| 0x15 | 1 | byte | ceilingColorB | Ceiling color - Blue component |
| 0x16 | 1 | byte | floorTex | Floor texture index |
| 0x17 | 1 | byte | ceilingTex | Ceiling texture index |
| 0x18 | 1 | byte | introColorR | Intro screen color - Red |
| 0x19 | 1 | byte | introColorG | Intro screen color - Green |
| 0x1A | 1 | byte | introColorB | Intro screen color - Blue |
| 0x1B | 1 | byte | loadMapID | Map ID for loading |
| 0x1C | 2 | short | mapSpawnIndex | Player spawn tile index |
| 0x1E | 1 | byte | mapSpawnDir | Player spawn direction |
| 0x1F | 2 | short | mapCameraSpawnIndex | Camera spawn tile index |

**Total Header Size: 33 bytes (0x21)**

**Note on Colors:**
- Floor/Ceiling colors are stored as BGR and converted to RGB565 format
- Intro color is stored as RGB and converted to ARGB format: `0xFF000000 | (R << 16) | (G << 8) | B`

---

### 2. Nodes Section

BSP tree nodes for spatial partitioning and rendering.

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 2 | short | nodesLength | Number of nodes |

**For each node (nodesLength times):**

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 | shiftCoord | x1 | X1 coordinate (byte << 3) |
| 1 | shiftCoord | y1 | Y1 coordinate (byte << 3) |
| 1 | shiftCoord | x2 | X2 coordinate (byte << 3) |
| 1 | shiftCoord | y2 | Y2 coordinate (byte << 3) |
| 1 | byte | arg1_high | High byte of args1 |
| 1 | shiftCoord | arg1_low | Low portion of args1 (byte << 3) |
| 2 | short | args2_low | Low 16 bits of args2 |
| 2 | short | args2_high | High 16 bits of args2 |

**Node Structure Assembly:**
```c
args1 = (arg1_high << 16) | arg1_low
args2 = args2_low | (args2_high << 16)
```

**Node size: 10 bytes per node**

---

### 3. Lines Section

Wall/line definitions for map geometry.

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 2 | short | linesLength | Number of lines |

**For each line (linesLength times):**

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 | shiftCoord | vert1.x | Vertex 1 X (byte << 3) |
| 1 | shiftCoord | vert1.y | Vertex 1 Y (byte << 3) |
| 1 | shiftCoord | vert2.x | Vertex 2 X (byte << 3) |
| 1 | shiftCoord | vert2.y | Vertex 2 Y (byte << 3) |
| 2 | short | texture | Texture ID |
| 4 | int | flags | Line flags (see below) |

**Line size: 10 bytes per line**

**Line Flags:**

| Bit | Hex | Description |
|-----|-----|-------------|
| 3 | 0x0008 | Offset +3 direction |
| 4 | 0x0010 | Offset -3 direction |
| 8 | 0x0100 | Vertical line (Y-axis aligned) |
| 9 | 0x0200 | Horizontal line (X-axis aligned) |
| 15 | 0x8000 | Reverse Z coordinate direction |

**Z Coordinate Calculation:**
```c
if (!(flags & 0x8000)) {
    vert1.z = 0;
    vert2.z = MAX(ABS(vert2.x - vert1.x), ABS(vert2.y - vert1.y));
} else {
    vert1.z = MAX(ABS(vert2.x - vert1.x), ABS(vert2.y - vert1.y));
    vert2.z = 0;
}
```

---

### 4. Sprites Section

Map sprite/entity definitions.

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 2 | short | numMapSprites | Number of sprites |

**For each sprite (numMapSprites times):**

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 | shiftCoord | x | X position (byte << 3) |
| 1 | shiftCoord | y | Y position (byte << 3) |
| 1 | byte | info_low | Low 8 bits of info (sprite type) |
| 2 | short | info_high | High 16 bits of info (sprite flags) |

**Sprite size: 5 bytes per sprite**

**Info Field Assembly:**
```c
info = info_low | (info_high << 16)
```

**Sprite Info Bits:**

| Bits | Mask | Description |
|------|------|-------------|
| 0-8 | 0x1FF | Sprite ID (0-511) |
| 18 | 0x40000 | Is texture (not sprite) |
| 19 | 0x80000 | Y offset -1 |
| 20 | 0x100000 | Y offset +1 |
| 21 | 0x200000 | X offset +1 |
| 22 | 0x400000 | X offset -1 |
| 23 | 0x800000 | Has direction offset |
| 29 | 0x20000000 | Skip rendering |

**Render Mode:**
- Sprites 136, 137, 144: `renderMode = 7`
- All other sprites: `renderMode = 0`

#### Sprite Type Definitions

The following table lists all sprite types defined in `entities.db`. The Sprite ID corresponds to the `tileIndex` field in entity definitions.

| Sprite ID | Name | Entity Type | Description |
|-----------|------|-------------|-------------|
| 1 | Axe | Weapon | Melee weapon |
| 2 | Fire Extinguisher | Weapon | Fire extinguisher weapon |
| 3 | Shotgun | Weapon | Shotgun |
| 4 | Super Shotgun | Weapon | Double-barrel shotgun |
| 5 | Assault Rifle | Weapon | Automatic rifle |
| 6 | Chaingun | Weapon | Rapid-fire chaingun |
| 7 | Plasma Gun | Weapon | Plasma rifle |
| 8 | BFG | Weapon | BFG 9000 |
| 9 | Rocket Launcher | Weapon | Rocket launcher |
| 10 | Fire Wall | Weapon | Fire wall attack |
| 11 | Soul Cube | Weapon | Soul Cube |
| 12 | Dog Collar | Weapon | Dog collar item |
| 17 | Zombie Private | Monster | Basic zombie enemy |
| 18 | Zombie Lieutenant | Monster | Stronger zombie variant |
| 19 | Zombie Captain | Monster | Elite zombie variant |
| 20 | Zombie Commando | Monster | Commando zombie |
| 21 | Zombie Commando 2 | Monster | Commando zombie variant |
| 22 | Zombie Sergant | Monster | Sergeant zombie |
| 23 | Imp | Monster | Standard imp demon |
| 24 | Imp Leader | Monster | Stronger imp variant |
| 25 | Imp Lord | Monster | Elite imp |
| 26 | Phantasm | Monster | Ghost enemy |
| 27 | Beholder | Monster | Floating eye demon |
| 28 | Rahovart | Monster | Demon variant |
| 29 | Infernis | Monster | Fire demon |
| 30 | Ogre | Monster | Large demon |
| 31 | Wretched | Monster | Twisted demon |
| 32 | Bull Demon | Monster | Charging demon (Pinky) |
| 33 | Belphegor | Monster | Demon lord |
| 34 | Assassin | Monster | Stealthy demon |
| 35 | Lost Soul | Monster | Flying skull |
| 36 | Nightmare | Monster | Nightmare creature |
| 37 | Revenant | Monster | Skeleton demon |
| 38 | Ghoul | Monster | Undead creature |
| 39 | Malwrath | Monster | Demon enemy |
| 40 | Hellhound | Monster | Hell dog |
| 41 | Mancubus | Monster | Fat demon |
| 42 | Druj | Monster | Demon variant |
| 43 | Behemoth | Monster | Large demon |
| 44 | Cacodemon | Monster | Floating demon |
| 45 | Watcher | Monster | Observer demon |
| 46 | Pinkinator | Monster | Pinky variant |
| 47 | Archvile | Monster | Fire demon |
| 48 | Infernotaur | Monster | Fire bull demon |
| 49 | Baron | Monster | Baron of Hell |
| 50 | Cyberdemon | Monster | Cyber demon boss |
| 51 | Pain Elemental | Monster | Soul spawner |
| 52 | Kronos | Monster | Boss demon |
| 53 | Guardian | Monster | Guardian demon |
| 54 | Spider Mastermind | Monster | Spider boss |
| 65 | Red Key | Key | Red keycard |
| 66 | Blue Key | Key | Blue keycard |
| 67 | Green Key | Key | Green keycard |
| 68 | Yellow Key | Key | Yellow keycard |
| 69 | Red Axe Key | Key | Red axe key |
| 70 | Blue Axe Key | Key | Blue axe key |
| 71 | Green Axe Key | Key | Green axe key |
| 72 | Yellow Axe Key | Key | Yellow axe key |
| 73 | ID Card | Key | Security ID card |
| 81 | Halon Canister | Ammo | Fire extinguisher ammo |
| 82 | Bullets Small | Ammo | Small bullet pack |
| 83 | Bullets Large | Ammo | Large bullet pack |
| 84 | Shells Small | Ammo | Small shell box |
| 85 | Shells Large | Ammo | Large shell box |
| 86 | Rockets Small | Ammo | Small rocket pack |
| 87 | Rockets Large | Ammo | Large rocket pack |
| 88 | Cells Small | Ammo | Small cell pack |
| 89 | Cells Large | Ammo | Large cell pack |
| 90 | BFG Cells | Ammo | BFG cell ammo |
| 91 | Armor Bonus | Pickup | Small armor bonus |
| 92 | Armor Shard | Pickup | Armor shard |
| 93 | Security Armor | Pickup | Security armor vest |
| 94 | Combat Armor | Pickup | Combat armor |
| 95 | Berserker | Pickup | Berserk powerup |
| 96 | Medkit 10 | Pickup | Small medkit (10 HP) |
| 97 | Medkit 25 | Pickup | Medium medkit (25 HP) |
| 98 | Medkit 100 | Pickup | Large medkit (100 HP) |
| 99 | Soulsphere | Pickup | Soul sphere (+100 HP) |
| 100 | Credits | Pickup | Credit pickup |
| 101 | Dog Food | Pickup | Dog food item |
| 102 | Dog Toy | Pickup | Dog toy item |
| 128 | Fire A | Decoration | Fire effect A |
| 129 | Fire B | Decoration | Fire effect B |
| 130 | Fire Pit | Decoration | Fire pit |
| 131 | Red Candle | Decoration | Red candle |
| 132 | Green Candle | Decoration | Green candle |
| 133 | Blue Candle | Decoration | Blue candle |
| 134 | Blue Torch | Decoration | Blue wall torch |
| 135 | Red Torch | Decoration | Red wall torch |
| 136 | Light 1 | Light | Light source (renderMode=7) |
| 137 | Light 2 | Light | Light source (renderMode=7) |
| 138 | Barrel | Decoration | Explosive barrel |
| 139 | Toxic Barrel | Decoration | Toxic waste barrel |
| 140 | Gore Corpse | Decoration | Gore decoration |
| 141 | Bones | Decoration | Skeleton remains |
| 142 | Table | Decoration | Table furniture |
| 143 | Chair | Decoration | Chair furniture |
| 144 | Light 3 | Light | Light source (renderMode=7) |
| 145 | Scientist 1 | NPC | Female scientist |
| 146 | Scientist 2 | NPC | Male scientist |
| 147 | Marine 1 | NPC | Marine NPC |
| 148 | Marine 2 | NPC | Marine NPC variant |
| 149 | Civilian 1 | NPC | Female civilian |
| 150 | Civilian 2 | NPC | Male civilian |
| 151 | Dr. Jensen | NPC | Dr. Jensen NPC |
| 152 | Dr. Guerard | NPC | Dr. Guerard NPC |
| 153 | Crate Small | Decoration | Small crate |
| 154 | Crate Large | Decoration | Large crate |
| 155 | Box | Decoration | Box prop |
| 156 | Flower | Decoration | Flower pot |
| 157 | Trash | Decoration | Trash pile |
| 158 | Bucket | Decoration | Bucket |
| 159 | Vent A | Decoration | Vent grate A |
| 160 | Vent B | Decoration | Vent grate B |
| 161 | Screen | Decoration | Computer screen |
| 162 | Terminal | Decoration | Computer terminal |
| 305 | Door 1 | Door | Standard door |
| 306 | Door 2 | Door | Door variant |
| 307 | Door Locked | Door | Locked door |
| 308 | Door Red | Door | Red key door |
| 309 | Door Blue | Door | Blue key door |
| 310 | Door Yellow | Door | Yellow key door |
| 311 | Door Green | Door | Green key door |
| 312 | Secret Door | Door | Hidden secret door |
| 313 | Exit Door | Door | Level exit door |
| 314 | Elevator Door | Door | Elevator door |
| 315 | Airlock | Door | Airlock door |
| 338 | Portal | Special | Teleporter portal |
| 339 | Save Station | Special | Save game station |
| 340 | Heal Station | Special | Health station |
| 341 | Armor Station | Special | Armor station |
| 342 | Dog | Special | Hellhound pet/NPC |
| 343 | Computer | Special | Interactive computer |
| 344 | Item Vendor | Special | Item vending machine |
| 345 | Weapon Vendor | Special | Weapon vending machine |
| 346 | Upgrade Station | Special | Upgrade station |
| 347 | Teleporter Pad | Special | Teleporter destination |
| 348 | Exit Sign | Special | Exit marker |
| 349 | Vent Exit | Special | Ventilation exit |
| 350 | Corpse Marine | Decoration | Dead marine |
| 351 | Corpse Scientist | Decoration | Dead scientist |
| 352 | Corpse Civilian | Decoration | Dead civilian |
| 353 | Blood Pool | Decoration | Blood splatter |
| 354 | Gibs | Decoration | Gibbed remains |
| 355 | Hanging Body | Decoration | Hanging corpse |
| 356 | Impaled Body | Decoration | Impaled corpse |
| 357 | Sparks | Effect | Spark effect |
| 358 | Steam | Effect | Steam vent |
| 359 | Electricity | Effect | Electric sparks |
| 360 | Smoke | Effect | Smoke effect |

**Entity Types:**
- Weapon (5): Collectible weapons
- Monster (1): Enemy entities
- Key (4): Key items for locked doors
- Ammo (6): Ammunition pickups
- Pickup (7): Health, armor, and other items
- Decoration (8): Non-interactive props
- NPC (2): Friendly characters
- Door (3): Door entities
- Light: Special lighting sprites
- Special: Interactive objects
- Effect: Visual effect sprites

---

### 5. Tile Events Section

Event triggers for map tiles. Each tile event associates a tile position with a sequence of bytecode commands.

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 2 | short | numTileEvents | Number of tile events |

**For each event (numTileEvents times):**

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 4 | int | event | Event data (packed format) |

**Event size: 4 bytes per event**

**Event Data Format (32-bit packed integer):**

| Bits | Mask | Field | Description |
|------|------|-------|-------------|
| 0-9 | 0x3FF | tileIndex | Tile position (0-1023, calculated as `y*32 + x`) |
| 10-18 | 0x7FC00 | commandIndex | Starting index in bytecode array |
| 19-24 | 0x1F80000 | commandCount | Number of bytecode commands |
| 25-28 | 0x1E000000 | eventState | Current state (0-9) for state machine events |
| 29-31 | 0xE0000000 | eventFlags | Event flags (see below) |

**Event Flags:**

| Bit | Value | Description |
|-----|-------|-------------|
| 29 | 0x20000000 | EVENT_FLAG_BLOCKINPUT - Blocks input during execution |

**Extracting Event Data (C code):**
```c
int tileIndex = event & 0x3FF;
int commandIndex = (event & 0x7FC00) >> 10;
int commandCount = (event & 0x1F80000) >> 19;
int eventState = (event & 0x1E000000) >> 25;
int eventFlags = (event & 0xE0000000) >> 29;
```

**Note:** If bits 19-24 (commandCount) are set, the tile is marked with `BIT_AM_EVENTS` on the automap.

---

### 6. ByteCodes Section

Script bytecode for map logic. Bytecodes are commands executed when tile events are triggered.

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 2 | short | numByteCodes | Number of bytecode entries |

**For each bytecode (numByteCodes times):**

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 | byte | id | Bytecode instruction ID (event type) |
| 4 | int | arg1 | First argument (instruction-specific) |
| 4 | int | arg2 | Second argument (condition/state flags) |

**ByteCode size: 9 bytes per entry**

**Bytecode Array Structure:**
```c
// Each bytecode entry is stored as 3 ints in the array
mapByteCode[(index * 3) + 0] = id;    // BYTE_CODE_ID
mapByteCode[(index * 3) + 1] = arg1;  // BYTE_CODE_ARG1
mapByteCode[(index * 3) + 2] = arg2;  // BYTE_CODE_ARG2
```

**arg2 Condition Flags:**

| Bits | Mask | Description |
|------|------|-------------|
| 0-8 | 0x1FF | Trigger condition flags |
| 9 | 0x200 | ARG2_FLAG_MODIFYWORLD - Clear this command after execution (one-time event) |
| 12-15 | 0xF000 | Key requirement flags |
| 16-24 | 0x1FF0000 | State requirement flags |

#### Complete Event Type List

| ID | Name | Description | arg1 Format |
|----|------|-------------|-------------|
| 1 | EV_GOTO | Teleport player to coordinates | `x \| (y << 8) \| (angle << 16)` |
| 2 | EV_CHANGEMAP | Change to another map | Map ID |
| 3 | EV_TRIGGER | Trigger event at coordinates | `x \| (y << 8)` |
| 4 | EV_MESSAGE | Show HUD message | String index |
| 5 | EV_PAIN | Damage the player | Damage amount |
| 6 | EV_MOVELINE | Move/animate a line (door) | Line index |
| 7 | EV_SHOW | Show a hidden sprite | `spriteIndex \| (flags << 16)` |
| 8 | EV_DIALOG | Show dialog with back option | String index |
| 9 | EV_GIVEMAP | Reveal the automap | - |
| 10 | EV_PASSWORD | Prompt for password | `passCodeID \| (stringID << 8)` |
| 11 | EV_CHANGESTATE | Set event state | `x \| (y << 8) \| (state << 16)` |
| 12 | EV_LOCK | Lock a line (door) | Line index |
| 13 | EV_UNLOCK | Unlock a line (door) | Line index |
| 14 | EV_TOGGLELOCK | Toggle line lock state | Line index |
| 15 | EV_OPENLINE | Open a door line | Line index |
| 16 | EV_CLOSELINE | Close a door line | Line index |
| 17 | EV_MOVELINE2 | Alternate line movement | Line index |
| 18 | EV_HIDE | Hide entities at coordinates | `x \| (y << 8)` |
| 19 | EV_NEXTSTATE | Increment event state | `x \| (y << 8)` |
| 20 | EV_PREVSTATE | Decrement event state | `x \| (y << 8)` |
| 21 | EV_INCSTAT | Increase player stat | `statType \| (amount << 8)` |
| 22 | EV_DECSTAT | Decrease player stat | `statType \| (amount << 8)` |
| 23 | EV_REQSTAT | Require player stat | `statType \| (amount << 8) \| (msgID << 16)` |
| 24 | EV_FORCEMESSAGE | Force status bar message | String index |
| 25 | EV_ANIM | Spawn animation | `x \| (y << 8) \| (animID << 16)` |
| 26 | EV_cF | Show dialog without back | String index |
| 27 | EV_SAVEGAME | Trigger save game | `strID \| (x << 8) \| (y << 16) \| (angle << 24)` |
| 28 | EV_ABORTMOVE | Cancel player movement | - |
| 29 | EV_SCREENSHAKE | Shake the screen | `duration \| (intensity << 12) \| (speed << 24)` |
| 30 | EV_CHANGEFLOORCOLOR | Change floor color | RGB color value |
| 31 | EV_CHANGECEILCOLOR | Change ceiling color | RGB color value |
| 32 | EV_ENABLEWEAPONS | Enable/disable weapons | 0 = disable, 1 = enable |
| 33 | EV_OPENSTORE | Open the store menu | Store parameter |
| 34 | EV_CHANGESPRITE | Change sprite appearance | `x \| (y << 5) \| (flags << 10) \| (spriteID << 13)` |
| 35 | EV_SPAWNPARTICLES | Spawn particle effects | `r \| (g << 8) \| (b << 16) \| (type << 24) \| (count << 29)` |
| 36 | EV_REFRESHVIEW | Force view refresh | - |
| 37 | EV_WAIT | Pause execution | Wait time in milliseconds |
| 38 | EV_ACTIVE_PORTAL | Activate monster portal | - |
| 39 | EV_CHECK_COMPLETED_LEVEL | Check level completion | `stringID \| (levelRange << 16)` |
| 40 | EV_NOTE | Add notebook entry | String index |
| 41 | EV_CHECK_KEY | Check for key item | Key type (0=green, 1=yellow, 2=blue, 3=red) |
| 42 | EV_PLAYSOUND | Play sound effect | Sound ID |

**Event Execution Flow:**
1. Player steps on a tile or triggers an action
2. Engine finds tile event by tile index
3. Extracts commandIndex and commandCount from event data
4. Iterates through bytecode commands at that index
5. For each command, checks state/condition flags in arg2
6. Executes matching commands via `Game_executeEvent()`

---

### 7. Strings Section

Map text strings for dialogs, messages, etc.

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 2 | short | numStrings | Number of strings |

**For each string (numStrings times):**

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 2 | short | strSize | Length of string |
| strSize | char[] | data | String data (NOT null-terminated in file) |

---

### 8. BlockMap Section

Collision/traversal flags for 32x32 tile grid (1024 tiles total).

**Format:** 256 bytes, each byte contains flags for 4 tiles (2 bits each).

```
For j = 0 to 255:
    flags = read_byte()
    mapFlags[i+0] |= (flags >> 0) & 3   // Bits 0-1
    mapFlags[i+1] |= (flags >> 2) & 3   // Bits 2-3
    mapFlags[i+2] |= (flags >> 4) & 3   // Bits 4-5
    mapFlags[i+3] |= (flags >> 6) & 3   // Bits 6-7
    i += 4
```

**Total BlockMap Size: 256 bytes**

**Flag Values:**
- `0`: Passable
- `1`: Blocked (wall)
- `2`: Special (door, etc.)
- `3`: Reserved

---

### 9. Plane Textures Section

Floor and ceiling texture indices per tile.

**Format:** 2 planes × 1024 tiles = 2048 bytes

```
For plane = 0 to 1:    // 0 = floor, 1 = ceiling
    For tile = 0 to 1023:
        textureId = read_byte()
        // Store in planeTextures[(plane * 1024) + tile]
```

**Maximum unique textures per map: 24**

---

## Complete File Layout Example

```
Offset    Size      Section
------    ----      -------
0x0000    33        Header
0x0021    2         Nodes count
0x0023    N*10      Nodes data
...       2         Lines count
...       N*10      Lines data
...       2         Sprites count
...       N*5       Sprites data
...       2         Tile events count
...       N*4       Tile events data
...       2         ByteCodes count
...       N*9       ByteCodes data
...       2         Strings count
...       Variable  Strings data
...       256       BlockMap
...       2048      Plane textures
```

---

## Rendering Pipeline

This section describes how the engine renders BSP data into pixels on screen.

### Coordinate System

- **Tile grid:** 32x32 tiles, each tile is 64 world units
- **shiftCoordAt:** reads one byte, shifts left by 3 → `value << 3` (range 0-2040, 8 units per step)
- **Fixed-point:** 16-bit fractional part (`FRACBITS = 16`, `FRACUNIT = 65536`)
- **View angle:** 0-255 maps to 0-360 degrees, with the sine table providing 256 precomputed values
- **View Z:** vertical position (default 32 = half of 64-unit tile height)

### Render Loop Overview

Each frame follows this sequence:

```
Render_render(viewX, viewY, viewZ, viewAngle)
  │
  ├─ 1. Compute view vectors from angle
  │     sin_ = sinTable[angle & 255]
  │     cos_ = sinTable[(angle + 64) & 255]
  │
  ├─ 2. Apply view nudge (push camera 16 units back)
  │     vx = viewX - ((16 * cos_) >> 16)
  │     vy = viewY + ((16 * sin_) >> 16)
  │
  ├─ 3. Set up view transform matrix
  │     viewTransX = -(vx * viewCos_ + vy * viewSin_)
  │     viewTransY = -(vx * viewSin  + vy * viewCos)
  │
  ├─ 4. Reset columnScale[] to COLUMN_SCALE_INIT
  │
  ├─ 5. Draw floor/ceiling background
  │     (solid color fill or textured plane rendering)
  │
  ├─ 6. Traverse BSP tree (Render_walkNode)
  │     ├─ Collect visible nodes and lines
  │     └─ Build depth-sorted sprite list
  │
  ├─ 7. Draw wall lines (Render_drawNodeLines)
  │
  └─ 8. Draw sprites back-to-front (Render_renderSpriteObject)
```

### View Transform

All world coordinates are transformed to view space before clipping and projection:

```c
// Render_transform2DVerts
view_x = (world_x * viewCos_) + (world_y * viewSin_) + viewTransX;
view_y = (world_x * viewSin)  + (world_y * viewCos)  + viewTransY;
```

After transform: `view_x` is depth (distance from camera), `view_y` is horizontal offset.

---

### BSP Traversal — Render_walkNode()

The BSP tree is traversed recursively with front-to-back ordering:

```
walkNode(nodeIndex):
    node = nodes[nodeIndex]

    if cullBoundingBox(node):
        return                          // fully occluded, skip

    if (node.args1 & 0x30000) == 0:     // LEAF NODE
        add node to viewNodes list
        render all lines in this leaf
        collect sprites into depth-sorted list
        return

    // INTERNAL NODE — determine split axis and position
    splitPos = node.args1 & 0xFFFF
    frontChild = (node.args2 >> 16) & 0xFFFF
    backChild  = node.args2 & 0xFFFF

    if args1 & 0x20000:  split on Y axis
    if args1 & 0x10000:  split on X axis

    // Traverse near side first for front-to-back ordering
    if viewer is on the "less than" side of split plane:
        walkNode(frontChild)
        walkNode(backChild)
    else:
        walkNode(backChild)
        walkNode(frontChild)
```

**Node args1 bit layout (extended for traversal):**

| Bits | Mask | Description |
|------|------|-------------|
| 0-15 | 0xFFFF | Split plane position (leaf: unused) |
| 16 | 0x10000 | Split on X axis |
| 17 | 0x20000 | Split on Y axis |

If both bits 16-17 are 0, the node is a **leaf** containing geometry.

**Node args2 bit layout:**

| Bits | Mask | Description (leaf node) | Description (internal node) |
|------|------|------------------------|----------------------------|
| 0-15 | 0xFFFF | First line index | Back child node index |
| 16-31 | 0xFFFF0000 | Line count | Front child node index |

---

### Frustum Culling — Render_cullBoundingBox()

Before traversing a node, the engine checks if its bounding box is fully behind already-rendered geometry:

```
1. If viewer is inside the bounding box (within ±5 units):
   → Not culled (always visible)

2. Select the bounding box edge closest to the viewer:
   → Creates a test line from the two corners of that edge

3. Transform the test line to view space

4. Clip the test line to the view frustum (4 planes)

5. Project the clipped line to screen columns

6. For each screen column in the projected range:
   if columnScale[column] == COLUMN_SCALE_INIT:
       → At least one column is unoccluded
       → Not culled (node is visible)

7. If all columns are already filled:
   → Fully culled (skip this node)
```

---

### Column Occlusion Tracking

The `columnScale[]` array (one entry per screen column) tracks rendering depth:

| Value | Constant | Meaning |
|-------|----------|---------|
| `0x7FFFFFFF` | `COLUMN_SCALE_INIT` | Column has no geometry yet |
| `0x7FFFFFFE` | `COLUMN_SCALE_OCCLUDED` | Column fully blocked by opaque BSP wall |
| Other | Scale value | Distance to closest wall (larger = closer) |

**Updated by:**
- **BSP walls** set columns to `COLUMN_SCALE_OCCLUDED` via `Render_occludeClippedLine()`
- **Visible walls/sprites** set columns to their scale value, only if closer than current value

---

### Line Rendering — Render_drawLines()

```
1. Backface cull:
   cross = (vert1 - viewPos) × (vert2 - vert1)
   if cross <= 0 and line is not double-sided (flag 1):
       skip this line
   if cross <= 0 and line IS double-sided:
       swap vert1 ↔ vert2 (render back face)

2. Transform both vertices to view space

3. Render_drawLine():
   a. Clip line to 4 frustum planes
   b. Project vertices to screen coordinates
   c. Route to renderer based on flags:
      - flag 0x20000000 (BSP wall):  mark columns as occluded
      - flag 2 (sprite line):        Render_drawSpriteSpan()
      - otherwise (textured wall):   Render_drawWallSpans()
```

**Line flags (extended for rendering):**

| Bit | Hex | Description |
|-----|-----|-------------|
| 0 | 0x01 | Double-sided (render both faces) |
| 1 | 0x02 | Sprite line (render as sprite, not wall) |
| 7 | 0x80 | Line has been rendered this frame |
| 29 | 0x20000000 | From BSP traversal (occlude columns) |

---

### Clipping — Render_clipLine()

Lines are clipped against 4 planes that form a view pyramid:

```
Plane 1: x + y >= 0    (left edge)
Plane 2: x - y >= 0    (right edge)
Plane 3: x <= 0x40000  (far plane)
```

If a line is fully behind any plane, it is discarded. Otherwise, endpoints are interpolated to the clip boundary using fixed-point linear interpolation:

```c
t = -dist1 / (dist2 - dist1);
clipped = vert1 * (1 - t) + vert2 * t;
```

---

### Projection — Render_projectVertex()

Perspective projection from view space to screen coordinates:

```c
// view_x = depth, view_y = horizontal offset
scale = screenWidth / view_x                    // perspective divide
screen_x = (view_y * scale) / 2 + halfScreenWidth
screen_z = view_z * scale                       // scaled height
```

All arithmetic uses 64-bit intermediates to avoid overflow.

---

### Wall Span Rendering — Render_drawWallSpans()

Walls are rendered column-by-column with perspective-correct texture mapping:

```
For each screen column from x1 to x2:

    1. Calculate wall scale:
       scale = 0x40000000 / interpolated_depth

    2. Skip if column already has closer geometry:
       if columnScale[col] < scale: skip
       columnScale[col] = scale

    3. Calculate texture U coordinate:
       u = (z_interpolated * scale) >> 32   // 0-63 range

    4. Calculate wall height on screen:
       wallHeight = (64 * depth) >> 17

    5. Calculate vertical screen position:
       yCenter = halfScreenHeight - ((64 - viewZ) * depth) >> 17

    6. Clamp to screen bounds

    7. Draw vertical span:
       texelOffset = (texelBase + u * 64) << 12
       spanFunction(column, yStart, texelOffset, vStep, height)
```

### Texture Animation

Wall texture indices are animated during rendering:

```c
animatedTexture = baseTexture + ((animFrameTime + textureIndex * 3) * 0x400000) >> 30
```

This produces a 2-bit animation frame (0-3) cycling based on time.

---

### Sprite Span Rendering — Render_drawSpriteSpan()

Similar to wall rendering but uses shape data from `bitshapes.bin` for variable-height columns:

```
For each screen column:
    1. Calculate column index in sprite texture from shape data
    2. Look up column pointer in shapeData[offset + 4 + columnIndex]
    3. For each run in the column (until 0x7F terminator):
       - yStart = shapeData[ptr] & 0xFF
       - runLength = shapeData[ptr] >> 8
       - texelOffset = shapeData[ptr + 1]
       - Calculate screen position and scale
       - Call spanFunction() for this segment
```

---

### Pixel Drawing Modes — SpanMode Functions

Each span function draws a vertical column of pixels from texture data. The texture is read as packed 4-bit nibbles:

```c
// Extract palette index from packed texel data
paletteIndex = mediaTexels[texOffset >> 13] >> ((texOffset >> 10) & 4) & 0xF;
color = spanPalettes[paletteIndex];
```

Available span modes:

| Mode | Name | Description |
|------|------|-------------|
| 0 | Normal | Direct palette lookup |
| 1 | XOR | XOR blend with framebuffer |
| 2 | OR | OR blend with framebuffer |
| 3 | AND | AND blend with framebuffer |
| 4 | 25% Alpha | 25% texture, 75% framebuffer |
| 5 | 50% Alpha | 50% transparency blend |
| 6 | 75% Alpha | 75% texture, 25% framebuffer |
| 7 | Additive | Additive glow effect (used by lights) |
| 8 | Subtractive | Darken effect |
| 9 | Spectre | Random noise dither (invisible enemy effect) |

---

### Sprite Sorting and Rendering

Sprites are collected during BSP traversal and sorted by depth (furthest first) for back-to-front rendering:

```
1. Calculate depth:
   sortZ = sprite.x * viewCos_ + sprite.y * viewSin_ + viewTransX

2. Adjust priority:
   - Background sprites (flag 0x800000):  sortZ = MAX_INT (always behind)
   - Foreground sprites (flag 0x1000000): sortZ++ (slightly in front)
   - Certain decorations (ID 180-191):    sortZ -= 2 (pushed back)

3. Insert into sorted linked list (furthest first)

4. Render all sprites in order (back-to-front):
   For each sprite: Render_renderSpriteObject()
```

### Sprite Orientation

Each sprite in the BSP file has orientation flags that determine how it faces the camera:

| Flag | Hex | Orientation |
|------|-----|-------------|
| Bit 19 | 0x80000 | Horizontal, facing East/West |
| Bit 20 | 0x100000 | Horizontal, facing opposite |
| Bit 21 | 0x200000 | Vertical, facing opposite |
| Bit 22 | 0x400000 | Vertical, facing North/South |

The sprite is converted into a line segment (two vertices) based on its orientation, then rendered through the same `Render_drawLine()` path as walls.

---

### Floor and Ceiling Rendering

Floor and ceiling are drawn before BSP geometry using one of two methods:

**Solid Color Mode:**
- Top half of screen filled with `ceilingColor` (from header)
- Bottom half filled with `floorColor` (from header)

**Textured Plane Mode:**
- For each scanline above center: sample ceiling texture
- For each scanline below center: sample floor texture
- Texture coordinates are computed using view position and angle:

```c
distance = (viewZ * 8 * screenWidth) / (scanlineOffset + 1)
u = viewX + viewCos * distance + viewSin * columnOffset
v = viewY - viewSin * distance + viewCos * columnOffset
```

The plane texture data comes from the **Plane Textures** section, providing per-tile texture indices across the 32x32 grid.

---

### Sprite Relinking

When a sprite moves (e.g., a door opening or entity walking), it must be re-inserted into the correct BSP leaf node:

```
1. Remove sprite from current node's sprite list
2. Traverse BSP tree from root:
   - At each internal node, compare sprite position to split plane
   - Navigate to the appropriate child
3. Insert sprite into the new leaf node's sprite list
```

This ensures sprites are always rendered with the correct occlusion relative to walls.

---

## Related Files

| File | Description |
|------|-------------|
| `/mappings.bin` | Texture/sprite ID mappings and offsets |
| `/bitshapes.bin` | Sprite shape data |
| `/texels.bin` | Texture pixel data |
| `/palettes.bin` | Color palette data |
| `/menu.bsp` | Menu screen map |

---

## Code References

Main loading functions in `Render.c`:
- `Render_beginLoadMap()` - Loads header
- `Render_beginLoadMapData()` - Loads all other sections

Byte reading utilities in `DoomRPG.c`:
- `DoomRPG_byteAtNext()`
- `DoomRPG_shortAtNext()`
- `DoomRPG_intAtNext()`
- `DoomRPG_shiftCoordAt()`
