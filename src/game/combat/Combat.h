#pragma once
#include <string>
#include <vector>
#include "CombatEntity.h"

class Entity;
class EntityMonster;
class GameSprite;
class ScriptThread;
class EntityDef;
class Text;
class Applet;
struct GameConfig;

struct MonsterDef {
	// --- Template combat stats (cloned to each spawned instance) ---
	CombatEntity templateStats;

	// --- Behavioral rules ---
	bool isBoss = false;
	int bossMinTier = 0;        // Minimum tier (parm) to be considered a boss (0 = all tiers)
	int xp = 0;                 // XP awarded to player on kill (loaded from monsters.yaml combat.xp; bosses bake +130 into the value)
	bool fearImmune = false;
	bool evading = false;
	bool moveToAttack = false;
	bool canResurrect = false;
	bool onHitPoison = false;
	bool floats = false;            // Floating death anim (lost soul, cacodemon)
	bool isVios = false;            // Randomize param on spawn (vios bosses)
	int smallParm0Scale = -1;       // Scale override for parm==0 (-1 = no override)
	int onHitPoisonId = 13;
	int onHitPoisonDuration = 5;
	int onHitPoisonPower = 3;
	int knockbackWeaponId = -1;   // -1 = none
	int walkSoundResId = -1;      // -1 = none
	int16_t randomDeathSounds[4] = {-1, -1, -1, -1}; // Up to 4 random death sound resource IDs
	int8_t numRandomDeathSounds = 0;                   // 0 = use normal death sound
	bool boneGibs = false;         // Produce bone gib particles on death (skeleton-type monsters)

	// AI behavior parameters (loaded from monsters.yaml, configurable per-monster)
	int pathSearchDepth = 8;        // max tiles to search when pathfinding (default 8)
	int movementTimeMs = 275;       // movement interpolation duration in ms (default 275, bosses use 500)
	int chaseLosWeight = -4;        // line-of-sight weight when chasing player (-4 = prefer direct path)
	int retreatLosWeight = 0;       // line-of-sight weight when retreating
	int goalMaxTurns = 16;          // max turns before a goal expires
	int resurrectSearchRadius = 0x19000; // world-unit radius to search for raise targets (default ~1600 tiles)
	bool diagonalAttack = false;    // can attack diagonally (default false, archvile-like)
	int8_t diagonalAttackField = 0; // which attack to use for diagonal (0=attack1, 1=attack2)
	bool orthogonalAttackOnly = false; // only attack along cardinal axes, fail on diagonal (boss pinky)

	// Teleport behavior (boss vios-like): teleport to random nearby tile before attacking
	struct TeleportDef {
		bool enabled = false;           // enable teleport mechanic
		int range = 4;                  // max tile range for destination (±N tiles)
		int maxAttempts = 30;           // max random positions to try
		int cooldownMin = 3;            // min turns between teleports
		int cooldownMax = 5;            // max turns between teleports (randomized)
		int particleId = 7;             // particle effect on teleport (-1 = none)
	};
	TeleportDef teleport;

	// Per-weapon weakness modifiers (indexed by weapon index, -1=immune, 0=normal, >0=left shift)
	static constexpr int MAX_WEAKNESS_MODS = 16;
	int8_t weaknessMods[MAX_WEAKNESS_MODS] = {};  // 0 = no modifier for all weapons

	// Single drop entry. A LootConfig holds a list of these; each is rolled
	// independently against its chancePct at death time.
	struct LootDropEntry {
		int16_t base = 0;              // Packed category (bits 12-15) | parm (bits 6-11)
		int8_t modulus = 0;            // Sprite % modulus for quantity randomization (0 = trinket path)
		int8_t offset = 0;             // Added to (sprite % modulus) result
		uint8_t chancePct = 100;       // 0..100; 100 = always drops (roll skipped)
		std::string trinketText;       // Trinket display text (when drop is trinket)
		int16_t trinketStringIdx = -1; // Index into GameConfig::trinketStrings (-1 = none)
	};

	// Loot drop configuration (per type, with optional per-tier overrides)
	struct LootConfig {
		std::vector<LootDropEntry> drops;
		bool noCorpseLoot = false; // Skip loot when this monster is a corpse type
	};
	LootConfig lootConfig;      // Type-level default
	static constexpr int MAX_LOOT_TIERS = 4;
	LootConfig lootTiers[MAX_LOOT_TIERS]; // Per-tier overrides (if modulus != 0)
	bool hasLootTiers = false;
};

// Backwards-compatible alias
using MonsterBehaviors = MonsterDef;

class Combat
{
private:

public:
	Applet* app = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->app
	const GameConfig* gameConfig = nullptr; // Set in startup()
	static constexpr int SEQ_ENDATTACK = 1;
	static constexpr int OUTOFCOMBAT_TURNS = 4;
	static constexpr int MAX_TILEDISTANCES = 16;
	static constexpr int EXPLOSION_OFFSET2 = 38;
	static constexpr int MAX_ACTIVE_MISSILES = 8;
	static constexpr int EXPLOSION_OFFSET = 48;
	static constexpr int DEF_PLACING_BOMB_Z = 18;
	static constexpr int PLACING_BOMB_TIME = 750;
	static constexpr int BOMB_RECOVER_TIME = 500;
	static constexpr int WEAPON_FIELD_STRMIN = 0;
	static constexpr int WEAPON_FIELD_STRMAX = 1;
	static constexpr int WEAPON_FIELD_RANGEMIN = 2;
	static constexpr int WEAPON_FIELD_RANGEMAX = 3;
	static constexpr int WEAPON_FIELD_AMMOTYPE = 4;
	static constexpr int WEAPON_FIELD_AMMOUSAGE = 5;
	static constexpr int WEAPON_FIELD_PROJTYPE = 6;
	static constexpr int WEAPON_FIELD_NUMSHOTS = 7;
	static constexpr int WEAPON_FIELD_SHOTHOLD = 8;
	static constexpr int WEAPON_MAX_FIELDS = 9;
	static constexpr int WEAPON_STRIDE = 8;
	static constexpr int MONSTER_FIELD_ATTACK1 = 0;
	static constexpr int MONSTER_FIELD_ATTACK2 = 1;
	static constexpr int MONSTER_FIELD_CHANCE = 2;
	static constexpr int PUNCH_PREP = 1;
	static constexpr int PUNCH_RIGHTHAND = 2;
	static constexpr int PUNCH_LEFTHAND = 3;
	static constexpr int ONE_FP = 65536;
	static constexpr int LOWEREDWEAPON_Y = 38;
	static constexpr int LOWERWEAPON_TIME = 200;
	static constexpr int WEAPON_SCALE = 131072;
	static constexpr int COMBAT_DONE = 0;
	static constexpr int COMBAT_CONTINUE = 1;
	static constexpr int REBOUNDOFFSET = 31;

	static constexpr int FIELD_COUNT = 6;
	static constexpr int FLD_WPIDLEX = 0;
	static constexpr int FLD_WPIDLEY = 1;
	static constexpr int FLD_WPATKX = 2;
	static constexpr int FLD_WPATKY = 3;
	static constexpr int FLD_WPFLASHX = 4;
	static constexpr int FLD_WPFLASHY = 5;

	int touchMe = 0;
	int tileDistances[Combat::MAX_TILEDISTANCES] = {};
	int16_t* monsterAttacks = nullptr;
	int unused_0x50 = 0; // never used
	int drawLogo = 0;
	int unused_0x58 = 0; // never used
	int8_t* wpinfo = nullptr;
	int8_t* wpDisplayOffsetY = nullptr;   // Per-weapon general Y offset (all renderers)
	int8_t* wpSwOffsetX = nullptr;        // Per-weapon software renderer X offset
	int8_t* wpSwOffsetY = nullptr;        // Per-weapon software renderer Y offset
	int16_t* wpAttackSound = nullptr;     // Per-weapon attack sound resource ID (-1 = none)
	int16_t* wpAttackSoundAlt = nullptr;  // Per-weapon alt attack sound (for character-dependent sounds, -1 = none)
	int16_t* wpViewTile = nullptr;        // Per-weapon tile index for first-person view sprite
	int numWeaponViewTiles = 0;
	int8_t* wpHudTexRow = nullptr;        // Per-weapon HUD texture row (-1 = use default 13)
	bool* wpHudShowAmmo = nullptr;        // Per-weapon whether to draw ammo numbers on HUD

	// Per-weapon behavior flags (loaded from weapons.yaml)
	struct WeaponFlags {
		bool vibrateAnim;       // Vibration animation during attack (chainsaw-like)
		bool chainsawHitEvent;  // Triggers onWeaponKill() on hit
		bool alwaysHits;        // Bypasses accuracy check (soul cube-like)
		bool doubleDamage;      // Doubles min/max damage (soul cube-like)
		bool isThrowableItem;   // Special render path with ammo check, skip in weapon cycle
		bool showFlashFrame;    // Show muzzle flash frame during attack (chaingun-like)
		bool hasFlashSprite;    // Render flash sprite overlay (assault rifle, super shotgun, chaingun)
		bool soulAmmoDisplay;   // Special ammo display format (X/5 instead of XXX)
		bool autoHit;           // Auto-hit (no accuracy roll, e.g. rocket launcher, monster weapons)
		bool isMelee;           // Melee weapon (chainsaw) — no shotsFired flag, special miss behavior
		bool noRecoil;          // No recoil/rock view on fire (chainsaw, holy water, plasma, soul cube)
		bool splashDamage;      // Deals splash/radius damage on hit (rocket launcher, BFG)
		int splashDivisor;      // Splash damage divisor (2 = half, 4 = quarter; 0 = no splash)
		bool noBloodOnHit;      // Skip blood particle spawn on hit (item weapon)
		bool drawDoubleSprite;  // Draw weapon sprite twice (scoped assault rifle overlay)
		bool consumeOnUse;      // Weapon is consumed after attack (item weapon — removes from inventory)
		bool chargeAttack;      // Monster charge: physical rush toward player (m_charge)
		bool flipSpriteOnEnd;   // Flip sprite horizontally when attack loop ends (imp fireball)
		bool poisonOnHit;       // Apply status effect on hit (arch-vile fire)
		int16_t attackSoundOverride;  // Override monster attack sound resource ID (-1 = use default)
		int16_t missSoundOverride;    // Override sound when missing (-1 = use default)
		int16_t meleeImpactAnim;      // Melee impact animation ID (0 = none)
		int interactFlags;      // Additional entity type flags when targeting with this weapon (0 = none)
		bool canLootCorpses;    // Can loot corpses at melee range (chainsaw)
		bool fountainWeapon;    // Can refill ammo at fountain/water spout entities (holy water pistol)
		bool isScoped;          // Scoped weapon — enables zoom mode and sniper hit detection (assault rifle w/ scope)
		bool fullAgility;       // Use full agility in accuracy calc (rocket launcher); if false, agility is reduced to 96/256
		bool requiresLineOfSight; // Weapon requires clear line of sight to target (melee weapons)
		bool blockableByShield; // Attack is blocked when player has shield buff active (fireball, archvile fire)
		bool outOfRangeStillFires; // Still fires when out of range (super shotgun, holy water — no early miss)
		bool requiresLosPath;     // Requires unobstructed tile path to target (soul cube — fails if visited tiles block)
		int onKillGrantKills;     // Every N kills with this weapon grants a stat bonus (0 = disabled)
		int onKillGrantStrength;  // Strength bonus granted per on_kill_grant cycle
	};
	WeaponFlags* wpFlags = nullptr;       // Array sized to numWeapons
	int numWeaponFlags = 0;
	int throwableItemAmmoType = 0;  // Cached ammo type index for the throwable item weapon (-1 = none)
	int throwableItemWeaponIdx = 0; // Cached weapon index for the throwable item weapon (-1 = none)
	int fountainWeaponIdx = 0;      // Cached weapon index for the fountain weapon (-1 = none)
	int fountainAmmoType = 0;       // Cached ammo type for the fountain weapon (-1 = none)
	int soulWeaponIdx = 0;          // Cached weapon index for the soul cube weapon (-1 = none)
	int familiarAmmoType = 0;       // Cached ammo type for familiar weapons (sentry bot fuel, -1 = none)

	// Safe accessor for weapon flags (returns all-false flags for out-of-range indices)
	const WeaponFlags& getWeaponFlags(int weaponIdx) const {
		static const WeaponFlags empty{};
		if (weaponIdx >= 0 && weaponIdx < numWeaponFlags) return wpFlags[weaponIdx];
		return empty;
	}

	// Familiar data loaded from weapons.yaml (per sentry bot weapon)
	struct FamiliarDef {
		int weaponIndex;        // weapon index this familiar is tied to
		int16_t familiarType;   // familiar type ID (1-4)
		int8_t postProcess;     // post-process render mode (0-2)
		bool explodes;          // explodes on death
		int explodeWeaponIndex; // weapon index used for explosion damage (-1 = none)
		int deathRemainsWeapon; // weapon index for death remains drop
		int16_t helpId;         // help text ID shown when activating
		bool canShoot;          // familiar can fire weapons (shooting sentry bots)
		bool selfDestructs;     // familiar self-destructs on attack (exploding sentry bots)
		int8_t hudFaceRow;      // Y offset row in sentry bot face sprite (0 or 17)
	};
	FamiliarDef* familiarDefs = nullptr;  // array, familiarDefCount entries
	int familiarDefCount = 0;
	int defaultFamiliarType = 0;    // familiar type for non-sentry weapons (default: 1)

	// Lookup FamiliarDef by familiarType; returns nullptr if not found
	const FamiliarDef* getFamiliarDefByType(int famType) const {
		for (int i = 0; i < familiarDefCount; i++) {
			if (familiarDefs[i].familiarType == famType) return &familiarDefs[i];
		}
		return nullptr;
	}

	// Projectile visual data (per projectile type)
	struct ProjVisual {
		int8_t launchRenderMode;     // Render mode for missile sprite
		int16_t launchAnim;          // Sprite anim ID (0 = no missile)
		int16_t launchAnimMonster;   // Alt anim when monster fires (0 = use launchAnim)
		int16_t launchSpeed;         // Override speed (0 = default)
		int16_t launchSpeedAdd;      // Added to default speed (0 = none)
		int8_t launchOffsetXR;       // Right-step X offset divisor
		int8_t launchOffsetYR;       // Right-step Y offset divisor
		int8_t launchOffsetZ;        // Z launch offset
		int16_t launchZOffset;       // Additional Z offset for start/end
		bool launchAnimFromWeapon;   // Use weapon tile as anim
		int16_t impactAnim;          // Impact explosion anim ID
		int8_t impactRenderMode;     // Impact render mode
		int16_t impactSound;         // Impact sound resource ID (-1 = none)
		bool impactScreenShake;      // Trigger screen shake on impact
		int16_t shakeDuration;
		int8_t shakeIntensity;
		int16_t shakeFade;
		// Impact behaviors
		bool causesFear;             // Causes fear on target monster (holy water)
		bool reflectsWithBuff;       // Reflects when player has anti-fire buff
		int8_t reflectBuffId;        // Which buff index blocks this projectile (-1 = none)
		int16_t reflectAnim;         // Anim override when reflected
		int8_t reflectRenderMode;    // Render mode when reflected
		bool impactParticles;        // Spawn particles at impact point
		uint32_t particleColor;      // Particle color (0 = default white)
		int16_t particleZOffset;     // Z offset for particle spawn
		int16_t impactZOffset;       // Z offset added at impact position
		bool soulCubeReturn;         // Soul cube special return behavior
		bool impactParticlesOnSprite; // Spawn particles on sprite (not position)
		// Launch behaviors
		bool closeRangeZAdjust;      // Adjust target Z when target is close (plasma)
		int16_t closeRangeZAmount;   // Z amount to add (default: 16)
		bool monsterDamageBoost;     // Monster fires with 1.5x speed/damage (BFG)
		bool resetThornParticles;    // Reset thorn particle counter on launch
	};
	ProjVisual* projVisuals = nullptr;
	int numProjTypes = 0;

	int8_t* monsterStats = nullptr;
	Entity* curAttacker = nullptr;
	Entity* curTarget = nullptr;
	Entity* lastTarget = nullptr;
	int dodgeDir = 0;
	EntityMonster* targetMonster = nullptr;
	EntityMonster* attackerMonster = nullptr;
	int targetSubType = 0;
	int targetType = 0;
	int stage = 0;
	int nextStageTime = 0;
	int nextStage = 0;
	GameSprite* activeMissiles[8] = {};
	int numActiveMissiles = 0;
	int missileAnim = 0;
	bool targetKilled = false;
	bool exploded = false;
	ScriptThread* explodeThread = nullptr;
	int damage = 0;
	int totalDamage = 0;
	int deathAmt = 0;
	int accumRoundDamage = 0;
	int totalArmorDamage = 0;
	int hitType = 0;
	int animStartTime = 0;
	int animTime = 0;
	int animEndTime = 0;
	int attackerWeaponId = 0;
	int attackerWeaponProj = 0;
	int attackerWeapon = 0;
	int weaponDistance = 0;
	bool lerpingWeapon = false;
	bool weaponDown = false;
	bool lerpWpDown = false;
	int lerpWpStartTime = 0;
	int lerpWpDur = 0;
	int combatAnimState = 0; // guessed — reset to -1 at stage transitions
	bool flashDone = false;
	int flashDoneTime = 0;
	int flashTime = 0;
	bool settingDynamite = false;
	bool dynamitePlaced = false;
	int settingDynamiteTime = 0;
	int currentBombIndex = 0;
	int placingBombZ = 0;
	int attackFrame = 0;
	int animLoopCount = 0;
	bool gotCrit = false;
	bool gotHit = false;
	bool isGibbed = false;
	int reflectionDmg = 0;
	int playerMissRepetition = 0;
	int monsterMissRepetition = 0;
	int renderTime = 0;
	int attackX = 0;
	int attackY = 0;
	int8_t* weapons = nullptr;
	int8_t* monsterWeakness = nullptr;
	MonsterDef* monsterDefs = nullptr;    // Unified monster definitions (indexed by monsterIdx)
	MonsterDef* monsterBehaviors = nullptr; // Alias for monsterDefs (backwards compat)
	int numMonsterDefs = 0;         // Number of monster entity defs (from EntityDefManager)
	int worldDist = 0;
	int tileDist = 0;
	int crFlags = 0;
	int crDamage = 0;
	int crArmorDamage = 0;
	int crCritChance = 0;
	int crHitChance = 0;
	int punchingMonster = 0;
	bool punchMissed = false;
	bool oneShotCheat = false;
	int numThornParticleSystems = 0;
	int32_t* tableCombatMasks = nullptr;
	bool soulCubeIsAttacking = false;

	// Constructor
	Combat();
	// Destructor
	~Combat();

	short getWeaponWeakness(int weaponIdx, int monsterIdx);
	bool startup();
	void performAttack(Entity* curAttacker, Entity* curTarget, int attackX, int attackY, bool b);
	void checkMonsterFX();
	int playerSeq();
	int monsterSeq();
	void drawEffects();
	void drawWeapon(int sx, int sy);
	void shiftWeapon(bool lerpWpDown);
	int runFrame();
	int calcHit(Entity* entity);
	void explodeOnMonster();
	void explodeOnPlayer();
	int getMonsterField(EntityDef* entityDef, int n);
	void checkForBFGDeaths(int x, int y);
	void radiusHurtEntities(int n, int n2, int n3, int n4, Entity* entity, Entity* entity2);
	void hurtEntityAt(int n, int n2, int n3, int n4, int n5, int n6, Entity* entity, Entity* entity2);
	void hurtEntityAt(int n, int n2, int n3, int n4, int n5, int n6, Entity* entity, bool b, Entity* entity2);
	Text* getWeaponStatStr(int n);
	Text* getArmorStatStr(int n);
	int WorldDistToTileDist(int n);
	void cleanUpAttack();
	void updateProjectile();
	void launchProjectile();
	GameSprite* allocMissile(int n, int n2, int n3, int n4, int n5, int n6, int duration, int n7);
	void launchSoulCube();
	int getWeaponTileNum(int n);
	const FamiliarDef* getFamiliarDefByWeapon(int weaponIndex) const;

};
