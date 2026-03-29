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
