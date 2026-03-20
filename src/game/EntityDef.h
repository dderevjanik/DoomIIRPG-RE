#ifndef __ENTITYDEF_H__
#define __ENTITYDEF_H__

class EntityDef;

// -----------------------
// EntityDefManager Class
// -----------------------

class EntityDefManager {
  private:
	EntityDef* list;
	int numDefs;

	bool loadFromBinary();
	bool loadFromINI(const char* path);

  public:
	// Constructor
	EntityDefManager();
	// Destructor
	~EntityDefManager();

	bool startup();
	void exportToINI(const char* path);
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
	static constexpr uint32_t RFLAG_FLOATER = 0x1;      // Floats (no legs), uses floating anim path
	static constexpr uint32_t RFLAG_GUN_FLARE = 0x2;    // Shows gun flash on attack
	static constexpr uint32_t RFLAG_SPECIAL_BOSS = 0x4; // Uses special boss rendering path

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
	};

	int16_t tileIndex;
	int16_t name;
	int16_t longName;
	int16_t description;
	uint8_t eType;
	uint8_t eSubType;
	uint8_t parm;
	uint8_t touchMe;
	uint32_t renderFlags;   // Bitmask of RFLAG_* constants
	FearEyeData fearEyes;   // Fear eye rendering offsets
	GunFlareData gunFlare;  // Gun flare rendering offsets
	BodyPartData bodyParts; // Body part rendering offsets

	// Constructor
	EntityDef();

	// Render flag helpers
	bool hasRenderFlag(uint32_t flag) const { return (renderFlags & flag) != 0; }
};

#endif
