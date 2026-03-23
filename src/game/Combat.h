#ifndef __COMBAT_H__
#define __COMBAT_H__

class Entity;
class EntityMonster;
class GameSprite;
class ScriptThread;
class CombatEntity;
class EntityDef;
class Text;
class Applet;

struct MonsterBehaviors {
	bool isBoss = false;
	bool fearImmune = false;
	bool evading = false;
	bool moveToAttack = false;
	bool canResurrect = false;
	bool onHitPoison = false;
	int onHitPoisonId = 13;
	int onHitPoisonDuration = 5;
	int onHitPoisonPower = 3;
	int knockbackWeaponId = -1;   // -1 = none
	int walkSoundResId = -1;      // -1 = none
};

class Combat
{
private:

public:
	Applet* app; // Set in startup(), replaces CAppContainer::getInstance()->app
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
	static constexpr int MAX_WRAITH_DRAIN = 45;
	static constexpr int MAP_03_BOOST = 9;
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

	int touchMe;
	int tileDistances[Combat::MAX_TILEDISTANCES];
	int16_t*monsterAttacks;
	int field_0x50_;
	int drawLogo;
	int field_0x58_;
	int8_t* wpinfo;
	int8_t* wpDisplayOffsetY;   // Per-weapon general Y offset (all renderers)
	int8_t* wpSwOffsetX;        // Per-weapon software renderer X offset
	int8_t* wpSwOffsetY;        // Per-weapon software renderer Y offset
	int16_t* wpAttackSound;     // Per-weapon attack sound resource ID (-1 = none)
	int16_t* wpAttackSoundAlt;  // Per-weapon alt attack sound (for character-dependent sounds, -1 = none)

	// Familiar data loaded from weapons.yaml (per sentry bot weapon)
	struct FamiliarDef {
		int weaponIndex;        // weapon index this familiar is tied to
		int16_t familiarType;   // familiar type ID (1-4)
		int8_t postProcess;     // post-process render mode (0-2)
		bool explodes;          // explodes on death
		int explodeWeaponIndex; // weapon index used for explosion damage (-1 = none)
		int deathRemainsWeapon; // weapon index for death remains drop
		int16_t helpId;         // help text ID shown when activating
	};
	FamiliarDef* familiarDefs;  // array, familiarDefCount entries
	int familiarDefCount;
	int defaultFamiliarType;    // familiar type for non-sentry weapons (default: 1)

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
	};
	ProjVisual* projVisuals;
	int numProjTypes;

	int8_t* monsterStats;
	Entity* curAttacker;
	Entity* curTarget;
	Entity* lastTarget;
	int dodgeDir;
	EntityMonster* targetMonster;
	EntityMonster* attackerMonster;
	int targetSubType;
	int targetType;
	int stage;
	int nextStageTime;
	int nextStage;
	GameSprite* activeMissiles[8];
	int numActiveMissiles;
	int missileAnim;
	bool targetKilled;
	bool exploded;
	ScriptThread* explodeThread;
	int damage;
	int totalDamage;
	int deathAmt;
	int accumRoundDamage;
	int totalArmorDamage;
	int hitType;
	int animStartTime;
	int animTime;
	int animEndTime;
	int attackerWeaponId;
	int attackerWeaponProj;
	int attackerWeapon;
	int weaponDistance;
	bool lerpingWeapon;
	bool weaponDown;
	bool lerpWpDown;
	int lerpWpStartTime;
	int lerpWpDur;
	int field_0x110_;
	bool flashDone;
	int flashDoneTime;
	int flashTime;
	bool settingDynamite;
	bool dynamitePlaced;
	int settingDynamiteTime;
	int currentBombIndex;
	int placingBombZ;
	int attackFrame;
	int animLoopCount;
	bool gotCrit;
	bool gotHit;
	bool isGibbed;
	int reflectionDmg;
	int playerMissRepetition;
	int monsterMissRepetition;
	int renderTime;
	int attackX;
	int attackY;
	int8_t* weapons;
	int8_t* monsterWeakness;
	MonsterBehaviors* monsterBehaviors;
	CombatEntity* monsters[51];
	int worldDist;
	int tileDist;
	int crFlags;
	int crDamage;
	int crArmorDamage;
	int crCritChance;
	int crHitChance;
	int punchingMonster;
	bool punchMissed;
	bool oneShotCheat;
	int numThornParticleSystems;
	int32_t* tableCombatMasks;
	bool soulCubeIsAttacking;

	// Constructor
	Combat();
	// Destructor
	~Combat();

	short getWeaponWeakness(int n, int n2, int n3);
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
	const FamiliarDef* getFamiliarDefByType(int familiarType) const;

};

#endif