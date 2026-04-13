#pragma once
#include <cstdint>
#include <expected>
#include <string>
class EntityDef;

// -----------------------
// EntityDefManager Class
// -----------------------

class EntityDefManager {
  private:
	EntityDef* list = nullptr;
	int numDefs = 0;
	int numMonsterDefs = 0;  // Count of monster entities (each gets a monsterIdx)

  public:
	int getNumMonsterDefs() const { return numMonsterDefs; }
	// Constructor
	EntityDefManager() = default;
	// Destructor
	~EntityDefManager();

	bool startup();

	// Parse entity definitions from a DataNode (called by ResourceManager)
	static std::expected<void, std::string> parse(EntityDefManager* mgr, const class DataNode& config);
	EntityDef* find(int eType, int eSubType);
	EntityDef* find(int eType, int eSubType, int parm);
	EntityDef* lookup(int tileIndex);
};

// ----------------
// EntityDef Class
// ----------------

class EntityDef {
  private:
  public:
	// Render flag constants (bitmask)
	static constexpr uint32_t RFLAG_NONE = 0;
	static constexpr uint32_t RFLAG_FLOATER = 0x1;       // Floats (no legs), uses floating anim path
	static constexpr uint32_t RFLAG_GUN_FLARE = 0x2;     // Shows gun flash on attack
	static constexpr uint32_t RFLAG_SPECIAL_BOSS = 0x4;  // Uses special boss rendering path
	static constexpr uint32_t RFLAG_NPC = 0x8;           // Is an NPC (shadow placement, body composition)
	static constexpr uint32_t RFLAG_NO_FLARE_ALT_ATTACK = 0x10; // Skip gun flare on ATTACK2 animation
	static constexpr uint32_t RFLAG_TALL_HITBOX = 0x20;         // Extended head hitbox for scoped weapons

	// Fear eye rendering offsets (used by Render::renderFearEyes)
	struct FearEyeData {
		int8_t eyeL = -1;       // lateral left-eye position
		int8_t eyeR = -1;       // lateral right-eye position
		int16_t zAdd = 0;       // Z offset added in per-monster switch
		int16_t zAlwaysPre = 0; // Z added before switch, all anims (e.g. LostSoul +192)
		int16_t zIdlePre = 0;   // Z added before switch, idle only (e.g. ArchVile +48)
		bool singleEye = false; // Only render left eye (Cacodemon)
		int8_t eyeLFlip = -128; // Override eyeL when flipped (-128 = no override)
		int8_t eyeRFlip = -128; // Override eyeR when flipped
	};

	// Gun flare rendering offsets (used in renderSpriteAnim attack flash)
	struct GunFlareData {
		bool dualFlare = false; // Two separate flashes (Mancubus, Revenant)
		int8_t flash1X = 0;     // Dual: first flash lateral offset
		int16_t flash1Z = 0;    // Dual: first flash Z offset
		int8_t flash2X = 0;     // Second (dual) or single flash lateral offset
		int16_t flash2Z = 0;    // Second (dual) or single flash Z offset
	};

	// Floater rendering data (used by Render::renderFloaterAnim)
	struct FloaterData {
		// Type discriminator: 0=simple (Cacodemon/LostSoul), 1=multipart (Sentinel)
		enum Type : uint8_t { FLOATER_SIMPLE = 0, FLOATER_MULTIPART = 1 };
		Type type = FLOATER_SIMPLE;
		int16_t zOffset = 0;               // Z offset added always (LostSoul: 192)
		bool hasIdleFrameIncrement = false; // Alternate frame on idle bob (LostSoul)
		bool hasBackExtraSprite = false;    // Render extra sprite on back walk (Cacodemon)
		int8_t backExtraSpriteIdx = 7;     // Which sprite for back extra render
		int8_t backViewFrame = 4;          // Start frame for back views (4 for simple, 6 for multipart)
		// Multipart (Sentinel) specific:
		int8_t idleFrontFrame = 2;         // Start frame for idle/walk front
		int16_t headZOffset = -11;         // Head Z relative to torso
		bool hasDeadLoot = false;          // Show loot indicator on dead
		// Sub-sprite frame indices
		int8_t attackFrame = 8;            // Attack base frame
		int8_t attack2Offset = 2;          // Offset added for ATTACK2 vs ATTACK1
		int8_t painFrame = 12;             // Pain frame
		int8_t deadFrame = 13;             // Dead frame
	};

	// Special boss rendering data (used by Render::renderSpecialBossAnim)
	struct SpecialBossData {
		// Type discriminator
		enum Type : uint8_t { BOSS_MULTIPART = 0, BOSS_ETHEREAL = 1, BOSS_SPIDER = 2 };
		Type type = BOSS_MULTIPART;
		// Multipart (Boss Pinky): shadow + legs + torso + head
		int16_t torsoZ = 384;             // Torso/head Z offset above legs
		int8_t idleSpriteIdx = 3;         // Sprite index for idle head
		int8_t attackSpriteIdx = 8;       // Sprite index for attack
		int8_t painSpriteIdx = 12;        // Sprite index for pain
		// Ethereal (VIOS): additive multi-sprite rendering
		int8_t renderModeOverride = 3;    // Render mode (3=additive)
		int8_t painRenderMode = 5;        // Render mode during pain
		// Spider (Mastermind/Arachnotron): shadow + torso + articulated legs
		int16_t legLateral = -44;         // Leg lateral offset from center
		int16_t legBaseZ = 6;             // Base leg Z (shifted <<4)
		int16_t idleTorsoZ = 35;          // Torso Z during idle
		int8_t idleBobDiv = 1;            // Bob divisor (1=full, 2=half bob amplitude)
		int16_t idleTorsoZBase = 0;       // Added to bob result (Arachnotron: 50)
		int16_t walkTorsoZ = 35;          // Torso Z during walk
		int16_t attackTorsoZ = 40;        // Torso Z during attack
		int16_t painTorsoZ = 70;          // Torso Z during pain
		int16_t painLegsZ = 100;          // Legs Z during pain
		int8_t painLegPos = 2;            // Leg positioning offset during pain
		bool hasAttackFlare = false;      // Has gun flare on attack frame 1
		int16_t flareZOffset = 288;       // Gun flare Z offset
		int8_t flareLateralPos = 0;       // Gun flare lateral position
		int16_t flareTorsoZExtra = 10;    // Extra Z added to torso for flare positioning
		// Sub-sprite frame index
		int8_t deadFrame = 13;            // Dead frame (used by all boss types)
	};

	// Render data for Cyberdemon/ChainsawGoblin specific checks in renderSpriteAnim
	struct SpriteAnimData {
		bool clampScale = false;          // Clamp scale factor (Cyberdemon: scaleFactor * 7/9)
		int8_t attackLegOffset = 0;       // Leg lateral offset during attack (Cyberdemon: -3)
		bool attackLegOffsetOnFlip = false; // Only apply leg offset when not flipped (Cyberdemon)
		// ChainsawGoblin frame-dependent head offsets:
		bool hasFrameDependentHead = false;
		int16_t headZFrame0 = 17;         // Head Z on attack frame 0
		int8_t headXFrame0 = 2;           // Head lateral on attack frame 0
		int16_t headZFrame1 = 16;         // Head Z on attack frame 1+
	};

	// Body part offsets for idle/walk/attack rendering
	struct BodyPartData {
		int16_t idleTorsoZ = 0;          // Torso Z offset during idle (e.g. Revenant -30, ArchVile -36)
		int16_t idleHeadZ = 0;           // Head Z offset during idle (e.g. Revenant 140, ArchVile 109)
		int16_t walkTorsoZ = 0;          // Torso Z offset during walk (e.g. ArchVile -36)
		int16_t walkHeadZ = 0;           // Head Z offset during walk (e.g. Revenant 160, ArchVile 109)
		int16_t attackTorsoZF0 = 0;      // Torso Z on attack frame 0 (e.g. ArchVile 288, ChainsawGoblin 96)
		int16_t attackTorsoZF1 = 0;      // Torso Z on attack frame 1 (e.g. ChainsawGoblin -100)
		int16_t attackHeadZ = 0;         // Head Z during attack (e.g. Revenant 20, Mancubus -112)
		int8_t attackHeadX = 0;          // Head lateral offset during attack (e.g. Revenant 2, Mancubus 7)
		bool noHeadOnBack = false;       // Suppress head on back views (Pinky)
		bool sentryHeadFlip = false;     // Flip head periodically (SentryBot)
		bool flipTorsoWalk = false;      // Use walk flip for torso (NPC/ArchVile)
		bool noHeadOnAttack = false;     // No separate head in attack (Zombie, Imp)
		bool noHeadOnMancAtk = false;    // No head on attack (Mancubus merged torso/head)
		int16_t attackRevAtk2TorsoZ = 0; // Special: Revenant attack2 torso Z offset
		// Sub-sprite frame indices (match default sprite sheet layout)
		int8_t legsFrame = 0;            // Idle/walk legs
		int8_t torsoFrame = 2;           // Idle/walk torso
		int8_t headFrame = 3;            // Idle/walk head
		int8_t backViewOffset = 4;       // Added to frame indices for back-facing views
		int8_t attack1Frame = 8;         // Attack1 torso base frame
		int8_t attack2Frame = 10;        // Attack2 torso base frame
		int8_t painFrame = 12;           // Pain single-sprite frame
		int8_t deadFrame = 13;           // Dead single-sprite frame
		int8_t slapTorsoFrame = 14;      // Slap melee torso frame
		int8_t slapHeadFrame = 15;       // Slap melee head base (alternates +0/+1)
	};

	// Destruction effects (for ET_ATTACK_INTERACTIVE entities: barricades, crates, furniture, pickups)
	struct DestroyData {
		int8_t particleId = -1;         // particle effect on destroy (-1 = none, 1 = small, 2 = wood)
		int32_t particleColor = -1;     // particle color override (-1 = default)
		bool unlinkOnly = false;        // true = unlink (barricade), false = remove from world
		int8_t destroyFrame = -1;       // sprite frame to set on destroy (-1 = don't change, 1 = broken)
		bool convertToWaterSpout = false; // convert entity to water spout on destroy (pickup)
	};

	// Environmental damage (for ET_ENV_DAMAGE entities: fire, spikes)
	struct EnvDamageData {
		int16_t damage = 0;             // damage amount (0 = no env damage)
		int8_t statusEffectId = -1;     // status effect to apply (-1 = none, 13 = burn)
		int8_t statusDuration = 0;      // status effect duration in turns
		int8_t statusPower = 0;         // status effect power/amount
		int8_t resistBuffIdx = -1;      // buff index that grants resistance (-1 = none, 9 = antifire)
	};

	int16_t tileIndex = 0;
	int16_t name = 0;
	int16_t longName = 0;
	int16_t description = 0;
	uint8_t eType = 0;
	uint8_t eSubType = 0;
	uint8_t parm = 0;
	uint8_t touchMe = 0;
	int16_t monsterIdx = -1;    // Sequential index for monster entities (-1 = not a monster)
	uint32_t renderFlags = 0;   // Bitmask of RFLAG_* constants
	FearEyeData fearEyes;       // Fear eye rendering offsets
	GunFlareData gunFlare;      // Gun flare rendering offsets
	BodyPartData bodyParts;     // Body part rendering offsets
	FloaterData floater;        // Floater rendering offsets
	SpecialBossData specialBoss; // Special boss rendering offsets
	SpriteAnimData spriteAnim;  // Sprite anim overrides (Cyberdemon/ChainsawGoblin)
	DestroyData destroy;        // Destruction effects (ET_ATTACK_INTERACTIVE)
	EnvDamageData envDamage;    // Environmental damage (ET_ENV_DAMAGE)

	// Constructor
	EntityDef() = default;

	// Render flag helpers
	bool hasRenderFlag(uint32_t flag) const { return (renderFlags & flag) != 0; }
};
