# DoomRPG entities.db File Format

This document describes the binary format of `entities.db` used in DoomRPG. This file defines all entity types in the game — weapons, monsters, keys, ammo, pickups, NPCs, decorations, doors, and special objects.

## Overview

`entities.db` is a flat binary database of fixed-size entity definitions. It is loaded at startup by `EntityDef_startup()` in `EntityDef.c`. All multi-byte values are little-endian.

---

## File Structure

```
+---------------------------+
| Header (2 bytes)          |
+---------------------------+
| EntityDef[0] (24 bytes)   |
+---------------------------+
| EntityDef[1] (24 bytes)   |
+---------------------------+
| ...                       |
+---------------------------+
| EntityDef[N-1] (24 bytes) |
+---------------------------+
```

**Total file size:** `2 + (numDefs * 24)` bytes

---

## Header

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0x00 | 2 | short | numDefs | Number of entity definitions |

---

## Entity Definition Record (24 bytes each)

Each record is read in the following order:

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0x00 | 2 | short | tileIndex | Sprite tile index (used to match entities to map sprites) |
| 0x02 | 1 | byte | eType | Entity type category (see Entity Types below) |
| 0x03 | 1 | byte | eSubType | Subtype within the category |
| 0x04 | 4 | int | parm | Entity parameter (usage varies by type, e.g. ammo count, health value) |
| 0x08 | 16 | char[16] | name | Null-terminated display name (padded with zeros) |

**Note:** The struct in `EntityDef.h` declares `name` before `parm`, but the binary file stores `parm` before `name`. The reading code in `EntityDef_startup()` reads them in file order.

---

## Entity Types (eType)

| eType | Name | Description |
|-------|------|-------------|
| 0 | Line/Wall | Map structure elements |
| 1 | Monster | Enemy entities (3 tiers per subtype) |
| 2 | NPC | Friendly human characters |
| 3 | Pickup | Keys, health vials, armor shards, credits |
| 4 | Consumable | Medical items (medkits, soulsphere, berserker) |
| 5 | Weapon | Collectible weapons |
| 6 | Ammo (Small) | Single ammunition pickup |
| 7 | Special | Portals, computers, item vendors, lamps |
| 8 | Obstacle | Physical blocking objects |
| 9 | Unknown | (Seen in data, e.g. tileIndex 0x0201) |
| 10 | Fire | Environmental fire hazard |
| 11 | Lava | Extreme environmental hazard |
| 12 | Destructible | Barrels, crates, jammed doors, power couplings |
| 13 | Furniture | Non-destructible props (sink, toilet, chair) |
| 14 | Sprite A | Special sprite rendering entity |
| 15 | Sprite B | Special sprite rendering entity |
| 16 | Ammo (Large) | Bulk ammunition pickup |

---

## Monster SubTypes (eType = 1)

Each monster subtype has 3 tiers (weak, medium, strong), distinguished by the `parm` field:

| eSubType | Tier 1 (parm=0x0126) | Tier 2 (parm=0x01D3) | Tier 3 (parm=0x0280) |
|----------|----------------------|----------------------|----------------------|
| 0 | Zombie Pvt | Zombie Lt | Zombie Cpt |
| 1 | Hellhound | Cerberus | Demon Wolf |
| 2 | Troop | Commando | Assassin |
| 3 | Impling | Imp | Imp Lord |
| 4 | Phantom | Lost Soul | Nightmare |
| 5 | Bull Demon | Pinky | Belphegor |
| 6 | Malwrath | Cacodemon | Wretched |
| 7 | Beholder | Rahovart | Pain Elemental |
| 8 | Ghoul | Fiend | Revenant |
| 9 | Behemoth | Mancubus | Druj |
| 10 | Infernis | Archvile | Apollyon |
| 11 | Ogre | Hell Knight | Baron |
| 12 | Cyberdemon | — | — |
| 13 | Kronos | — | — |

**Monster parm values:** The `parm` field for monsters appears to encode difficulty/stat scaling. The three common values are `0x0126` (294), `0x01D3` (467), and `0x0280` (640).

---

## Weapon SubTypes (eType = 5)

| eSubType | Name | parm |
|----------|------|------|
| 0 | Axe | 0 |
| 1 | Fire Ext | 10 |
| 2 | Pistol | 0 |
| 3 | Shotgun | 8 |
| 4 | Chaingun | 12 |
| 5 | Super Shotgun | 10 |
| 6 | Plasma Gun | 12 |
| 7 | Rocket Lnchr | 4 |
| 8 | BFG | 15 |
| 9 | Hellhound | 0 |
| 10 | Cerberus | 0 |
| 11 | Demon Wolf | 0 |

---

## Key SubTypes (eType = 3, eSubType = 24)

| tileIndex | Name | parm |
|-----------|------|------|
| 0x41 (65) | Red Key | 3 |
| 0x42 (66) | Blue Key | 2 |
| 0x43 (67) | Green Key | 0 |
| 0x49 (73) | Yellow Key | 1 |

---

## Ammo Types (eType = 6 small, eType = 16 large)

| eSubType | Small Name | Large Name | Small parm | Large parm |
|----------|-----------|------------|------------|------------|
| 0 | Halon Can | Halon Cans | 3 | 10 |
| 1 | Bullet Clip | Bullet Clips | 4 | 10 |
| 2 | Shell Clip | Shell Clips | 4 | 10 |
| 3 | Rocket | Rockets | 1 | 4 |
| 4 | Cell Clip | Cell Clips | 4 | 10 |

---

## Complete Entity Listing

All 115 entity definitions in the file:

| # | tileIndex | eType | eSubType | parm | Name |
|---|-----------|-------|----------|------|------|
| 0 | 1 | 5 | 0 | 0 | Axe |
| 1 | 2 | 5 | 1 | 10 | Fire Ext |
| 2 | 3 | 5 | 3 | 8 | Shotgun |
| 3 | 4 | 5 | 5 | 10 | Super Shotgn |
| 4 | 5 | 5 | 4 | 12 | Chaingun |
| 5 | 6 | 5 | 7 | 4 | Rocket Lnchr |
| 6 | 7 | 5 | 6 | 12 | Plasma Gun |
| 7 | 8 | 5 | 8 | 15 | BFG |
| 8 | 9 | 5 | 2 | 0 | Pistol |
| 9 | 10 | 5 | 9 | 0 | Hellhound |
| 10 | 11 | 5 | 10 | 0 | Cerberus |
| 11 | 12 | 5 | 11 | 0 | Demon Wolf |
| 12 | 17 | 1 | 0 | 294 | Zombie Pvt |
| 13 | 18 | 1 | 0 | 467 | Zombie Lt |
| 14 | 19 | 1 | 0 | 640 | Zombie Cpt |
| 15 | 20 | 1 | 1 | 294 | Hellhound |
| 16 | 21 | 1 | 1 | 467 | Cerberus |
| 17 | 22 | 1 | 1 | 640 | Demon Wolf |
| 18 | 23 | 1 | 2 | 294 | Troop |
| 19 | 24 | 1 | 2 | 467 | Commando |
| 20 | 25 | 1 | 2 | 640 | Assassin |
| 21 | 26 | 1 | 3 | 294 | Impling |
| 22 | 27 | 1 | 3 | 467 | Imp |
| 23 | 28 | 1 | 3 | 640 | Imp Lord |
| 24 | 29 | 1 | 4 | 294 | Phantom |
| 25 | 30 | 1 | 4 | 467 | Lost Soul |
| 26 | 31 | 1 | 4 | 640 | Nightmare |
| 27 | 32 | 1 | 5 | 294 | Bull Demon |
| 28 | 33 | 1 | 5 | 467 | Pinky |
| 29 | 34 | 1 | 5 | 640 | Belphegor |
| 30 | 35 | 1 | 6 | 294 | Malwrath |
| 31 | 36 | 1 | 6 | 467 | Cacodemon |
| 32 | 37 | 1 | 6 | 640 | Wretched |
| 33 | 38 | 1 | 7 | 294 | Beholder |
| 34 | 39 | 1 | 7 | 467 | Rahovart |
| 35 | 40 | 1 | 7 | 640 | Pain Elemental |
| 36 | 41 | 1 | 8 | 294 | Ghoul |
| 37 | 42 | 1 | 8 | 467 | Fiend |
| 38 | 43 | 1 | 8 | 640 | Revenant |
| 39 | 44 | 1 | 9 | 294 | Behemoth |
| 40 | 45 | 1 | 9 | 467 | Mancubus |
| 41 | 46 | 1 | 9 | 640 | Druj |
| 42 | 47 | 1 | 10 | 294 | Infernis |
| 43 | 48 | 1 | 10 | 467 | Archvile |
| 44 | 49 | 1 | 10 | 640 | Apollyon |
| 45 | 50 | 1 | 11 | 294 | Ogre |
| 46 | 51 | 1 | 11 | 467 | Hell Knight |
| 47 | 52 | 1 | 11 | 640 | Baron |
| 48 | 53 | 1 | 12 | 294 | Cyberdemon |
| 49 | 54 | 1 | 13 | 294 | Kronos |
| 50 | 65 | 3 | 24 | 3 | Red Key |
| 51 | 66 | 3 | 24 | 2 | Blue Key |
| 52 | 67 | 3 | 24 | 0 | Green Key |
| 53 | 73 | 3 | 24 | 1 | Yellow Key |
| 54 | 81 | 6 | 0 | 3 | Halon Can |
| 55 | 82 | 16 | 0 | 10 | Halon Cans |
| 56 | 83 | 6 | 1 | 4 | Bullet Clip |
| 57 | 84 | 16 | 1 | 10 | Bullet Clips |
| 58 | 85 | 6 | 2 | 4 | Shell Clip |
| 59 | 86 | 16 | 2 | 10 | Shell Clips |
| 60 | 87 | 6 | 3 | 1 | Rocket |
| 61 | 88 | 16 | 3 | 4 | Rockets |
| 62 | 89 | 6 | 4 | 4 | Cell Clip |
| 63 | 90 | 16 | 4 | 10 | Cell Clips |
| 64 | 91 | 3 | 20 | 4 | Health Vial |
| 65 | 92 | 3 | 21 | 4 | Armor Shard |
| 66 | 93 | 3 | 21 | 25 | Flak Jacket |
| 67 | 94 | 3 | 21 | 50 | Combat Suit |
| 68 | 95 | 3 | 22 | 1 | 1 credit |
| 69 | 96 | 3 | 23 | 5 | 5 credits |
| 70 | 99 | 4 | 25 | 25 | Sm Medkit |
| 71 | 100 | 4 | 26 | 75 | Lg Medkit |
| 72 | 101 | 4 | 27 | 0 | Soul Sphere |
| 73 | 102 | 4 | 28 | 0 | Berserker |
| 74 | 110 | 4 | 29 | 0 | Dog Collar |
| 75 | 145 | 2 | 0 | 0 | Civilian |
| 76 | 146 | 2 | 0 | 0 | Scientist |
| 77 | 147 | 2 | 0 | 0 | Marine |
| 78 | 148 | 2 | 0 | 0 | Scientist |
| 79 | 149 | 2 | 0 | 0 | Dr. Guerard |
| 80 | 150 | 2 | 0 | 0 | Civilian |
| 81 | 151 | 2 | 0 | 0 | Civilian |
| 82 | 152 | 2 | 0 | 0 | Marine |
| 83 | 128 | 11 | 0 | 0 | Lava Pool |
| 84 | 129 | 10 | 0 | 0 | Fire |
| 85 | 130 | 12 | 1 | 4095 | Barrel |
| 86 | 131 | 12 | 4 | 4095 | Power Coupling |
| 87 | 132 | 13 | 0 | 0 | Sink |
| 88 | 133 | 13 | 0 | 0 | Toilet |
| 89 | 134 | 13 | 0 | 0 | Chair |
| 90 | 135 | 7 | 0 | 0 | Yellow Lamp |
| 91 | 139 | 7 | 0 | 0 | Cabinet |
| 92 | 140 | 7 | 0 | 0 | Blue Lamp |
| 93 | 141 | 13 | 0 | 0 | Table |
| 94 | 142 | 7 | 0 | 0 | Bunk Beds |
| 95 | 161 | 12 | 2 | 4095 | Crate |
| 96 | 162 | 12 | 2 | 4095 | Crate |
| 97 | 306 | 12 | 3 | 1 | Jammed Door |
| 98 | 332 | 12 | 3 | 1 | Weak Wall |
| 99 | 305 | 0 | 0 | 0 | Door |
| 100 | 308 | 0 | 0 | 0 | Red Door |
| 101 | 309 | 0 | 0 | 0 | Blue Door |
| 102 | 310 | 0 | 0 | 0 | Green Door |
| 103 | 311 | 0 | 0 | 0 | Yellow Door |
| 104 | 312 | 0 | 0 | 0 | Exit Door |
| 105 | 314 | 0 | 1 | 0 | Locked Door |
| 106 | 315 | 0 | 2 | 0 | Unlocked Door |
| 107 | 338 | 7 | 4 | 0 | (unnamed) |
| 108 | 339 | 7 | 3 | 0 | Portal |
| 109 | 359 | 7 | 5 | 0 | Computer |
| 110 | 360 | 7 | 0 | 0 | Item Vendor |
| 111 | 513 | 9 | 0 | 0 | (unnamed) |
| 112 | 514 | 14 | 0 | 0 | (unnamed) |
| 113 | 515 | 15 | 0 | 0 | (unnamed) |
| 114 | 666 | 8 | 0 | 0 | (unnamed) |

---

## Lookup Functions

### EntityDef_lookup(tileIndex)

Searches for an entity definition by its `tileIndex`. Used when the map loader encounters a sprite and needs to determine what entity it represents.

### EntityDef_find(eType, eSubType)

Searches for the first entity definition matching the given `eType` and `eSubType`. Returns the first match found.

---

## Code References

- Loading: [EntityDef.c:59-101](src/EntityDef.c#L59-L101) — `EntityDef_startup()`
- Struct: [EntityDef.h:9-16](src/EntityDef.h#L9-L16) — `EntityDef_t`
- Byte reading: [DoomRPG.c](src/DoomRPG.c) — `DoomRPG_shortAtNext()`, `DoomRPG_byteAtNext()`, `DoomRPG_intAtNext()`
