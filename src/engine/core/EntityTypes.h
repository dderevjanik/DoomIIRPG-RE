#pragma once

// Engine-level entity classification and collision-mask constants.
// Referenced by engine subsystems (renderer, canvas, particles) to categorize
// entities without pulling in game-specific constants. Game-specific subtypes
// (monster IDs, item IDs, etc.) live in src/game/data/Enums.h.

namespace EntityTypes {

// Maximum number of distinct entity definitions supported
constexpr int MAX_ENTITY_DEFS = 200;

// Entity type classification (EntityDef::eType)
constexpr int ET_WORLD = 0;
constexpr int ET_PLAYER = 1;
constexpr int ET_MONSTER = 2;
constexpr int ET_NPC = 3;
constexpr int ET_PLAYERCLIP = 4;
constexpr int ET_DOOR = 5;
constexpr int ET_ITEM = 6;
constexpr int ET_DECOR = 7;
constexpr int ET_ENV_DAMAGE = 8;
constexpr int ET_CORPSE = 9;
constexpr int ET_ATTACK_INTERACTIVE = 10;
constexpr int ET_MONSTERBLOCK_ITEM = 11;
constexpr int ET_SPRITEWALL = 12;
constexpr int ET_NONOBSTRUCTING_SPRITEWALL = 13;
constexpr int ET_DECOR_NOCLIP = 14;
constexpr int ET_MAX = 15;

// Collision/trace mask values used by Trace() and solidity checks
constexpr int CONTENTS_ANY = -1;
constexpr int CONTENTS_PICKUP = 64;
constexpr int CONTENTS_INTERACTIVE = 1068;
constexpr int CONTENTS_PLAYERSOLID = 13501;
constexpr int CONTENTS_MONSTERSOLID = 15535;
constexpr int CONTENTS_WEAPONSOLID = 13997;
constexpr int CONTENTS_VIEWSOLID = 5293;
constexpr int CONTENTS_MONSTERWPSOLID = 5295;
constexpr int CONTENTS_WORLD = 1;
constexpr int CONTENTS_SPRITEWALL = 12288;
constexpr int CONTENTS_NOFLOAT = 12424;
constexpr int CONTENTS_DYNAMITE_SOLID = 13349;
constexpr int CONTENTS_ISEMPTY_SCRIPT = 25152;
constexpr int CONTENTS_SPLASH_SOLID = 4129;
constexpr int CONTENTS_LINE_O_SIGHT = 4131;

} // namespace EntityTypes
