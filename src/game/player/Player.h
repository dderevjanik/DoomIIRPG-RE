#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include "Entity.h"
#include "EntityDef.h"
#include "CombatEntity.h"

class Entity;
class EntityDef;
class CombatEntity;
class Text;
class InputStream;
class OutputStream;
class Graphics;
class Applet;
class ScriptThread;
struct GameConfig;

struct ItemEffect {
	enum Type { HEALTH, ARMOR, STATUS_EFFECT };
	Type type;
	int buffIndex;   // for STATUS_EFFECT: resolved buff index
	int amount;
	int duration;    // for STATUS_EFFECT: turn duration
};

struct ItemDef {
	int index;
	bool requireAll;      // true = all effects must succeed, false = any
	bool advanceTurn;
	bool consume;
	bool skipEmptyCheck;
	int requiresNoBuff;   // -1 = none, otherwise buff index
	std::vector<ItemEffect> effects;
	std::vector<ItemEffect> bonusEffects;
	std::vector<int> removeBuffs;
	int ammoCostType;     // -1 = none
	int ammoCostAmount;
};

class Player
{
private:

public:
	Applet* app = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->app
	const GameConfig* gameConfig = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->gameConfig

	static constexpr int EXPIRE_DURATION = 5;
	static constexpr int MAX_DISPLAY_BUFFS = 6;
	static constexpr int ICE_FOG_DIST = 1024;
	static constexpr int MAX_NOTEBOOK_INDEXES = 8;
	static constexpr int NUMBER_OF_TARGET_PRACTICE_SHOTS = 8;
	static constexpr int HEAD_SHOT_POINTS = 30;
	static constexpr int BODY_SHOT_POINTS = 20;
	static constexpr int LEG_SHOT_POINTS = 10;
	static constexpr int NUMBER_OF_BITS_PER_VM_TRY = 3;
	static constexpr int BUFF_TURNS = 0;
	static constexpr int MAX_BUFF_IDS = 18; // statusEffects supports buff IDs 0-17

	// Buff data loaded from effects.yaml (not in save files — safe to use plain int)
	int buffMaxStacks[15] = {};
	int buffBlockedBy[15] = {};        // -1 = none
	int buffApplySound[15] = {};       // -1 = none
	int buffPerTurnDamage[15] = {};    // 0 = none, >0 = flat damage per turn
	bool buffPerTurnHealByAmount[15] = {}; // true = heals by buff amount per turn
	int buffDuration[MAX_BUFF_IDS] = {}; // default duration in turns
	int buffNoAmountMask = 0;
	int buffAmtNotDrawnMask = 0;
	int buffWarningTime = 0;

	// Cached buff indices resolved by name from effects.yaml. -1 if not present in yaml.
	// Used by call sites that previously referenced Enums::BUFF_* constants directly.
	int reflectBuffIdx = 0;   // "reflect"
	int purifyBuffIdx = 1;    // "purify"
	int hasteBuffIdx = 2;     // "haste"
	int regenBuffIdx = 3;     // "regen"
	int defenseBuffIdx = 4;   // "defense"
	int strengthBuffIdx = 5;  // "strength"
	int agilityBuffIdx = 6;   // "agility"
	int focusBuffIdx = 7;     // "focus"
	int angerBuffIdx = 8;     // "anger"
	int antifireBuffIdx = 9;  // "antifire"
	int fortitudeBuffIdx = 10; // "fortitude"
	int fearBuffIdx = 11;     // "fear"
	int wpPoisonBuffIdx = 12; // "wp_poison"
	int fireBuffIdx = 13;     // "fire"
	int diseaseBuffIdx = 14;  // "disease"

	// Item data loaded from items.yaml
	std::vector<ItemDef>* itemDefs = nullptr;
	static constexpr int BOX_X1 = 17;
	static constexpr int BOX_X2 = 31;
	static constexpr int BOX_X3 = 43;
	static constexpr int BOX_X4 = 55;

	Entity* facingEntity = nullptr;
	short inventory[26] = {};
	std::vector<short> ammo;
	int weapons = 0;
	short inventoryCopy[26] = {};
	std::vector<short> ammoCopy;
	int weaponsCopy = 0;
	int goldCopy = 0;
	int currentWeaponCopy = 0;
	bool tookBotsInventory = false;
	bool botReturnedDueToMonster = false;
	bool unsetFamiliarOnceOutOfCinematic = false;
	int disabledWeapons = 0;
	int level = 0;
	int currentXP = 0;
	int nextLevelXP = 0;
	CombatEntity* baseCe = nullptr;
	CombatEntity* ce = nullptr;
	EntityDef* activeWeaponDef = nullptr;
	bool noclip = false;
	bool god = false;
	short characterChoice = 0;
	bool isFamiliar = false;
	short familiarType = 0;
	bool attemptingToSelfDestructFamiliar = false;
	std::unordered_map<int, int> killGrantCounts; // per-weapon kill counters for on_kill_grant
	uint8_t lastSkipCode = 0;
	bool inTargetPractice = false;
	int targetPracticeScore = 0;
	int playTime = 0;
	int totalTime = 0;
	int moves = 0;
	int totalMoves = 0;
	int completedLevels = 0;
	int killedMonstersLevels = 0;
	int foundSecretsLevels = 0;
	int xpGained = 0;
	int currentLevelDeaths = 0;
	int totalDeaths = 0;
	int currentGrades = 0;
	int bestGrades = 0;
	short notebookIndexes[Player::MAX_NOTEBOOK_INDEXES] = {};
	short notebookPositions[Player::MAX_NOTEBOOK_INDEXES] = {};
	uint8_t questComplete = 0;
	uint8_t questFailed = 0;
	int hackedVendingMachines = 0;
	int vendingMachineHackTriesLeft1 = 0;
	int vendingMachineHackTriesLeft2 = 0;
	int numNotebookIndexes = 0;
	int helpBitmask = 0;
	int invHelpBitmask = 0;
	int ammoHelpBitmask = 0;
	int weaponHelpBitmask = 0;
	int armorHelpBitmask = 0;
	int gamePlayedMask = 0;
	int lastCombatTurn = 0;
	bool inCombat = false;
	bool enableHelp = false;
	int turnTime = 0;
	int highestMap = 0;
	int prevWeapon = 0;
	bool noDeathFlag = false;
	bool noFamiliarRemains = false;
	int numStatusEffects = 0;
	int numStatusEffectsCopy = 0;
	int statusEffects[54] = {};
	int statusEffectsCopy[54] = {};
	short buffs[30] = {};
	short buffsCopy[30] = {};
	int numbuffs = 0;
	int numbuffsCopy = 0;
	bool gameCompleted = false;
	int playerEntityCopyIndex = 0;
	int counters[8] = {};
	int monsterStats[2] = {};

	// Constructor
	Player();
	// Destructor
	~Player();

	bool startup();
	bool modifyCollision(Entity* entity);
	void advanceTurn();
	void levelInit();
	void fillMonsterStats();
	void readyWeapon();
	void selectWeapon(int weaponIdx);
	void selectPrevWeapon();
	void selectNextWeapon();
	int getHealth();
	int modifyStat(int statId, int delta);
	bool requireStat(int statId, int minValue);
	bool requireItem(int category, int index, int minCount, int maxCount);
	void addXP(int xp);
	void addLevel();
	int calcLevelXP(int level);
	int calcScore();
	bool addHealth(int amount);
	bool addHealth(int amount, bool announce);
	void setStatsAccordingToCharacterChoice();
	void reset();
	int calcDamageDir(int x1, int y1, int angle, int x2, int y2);
	void painEvent(Entity* entity, bool fromMonster);
	void pain(int damage, Entity* entity, bool announce);
	void died();
	void familiarDying(bool familiarSelfDestructed);
	bool fireWeapon(Entity* entity, int attackX, int attackY);
	bool useItem(int itemIdx);
	bool give(int category, int index, int count);
	bool give(int category, int index, int count, bool silent);
	bool give(int category, int index, int count, bool silent, bool selfPickup);
	void giveAmmoWeapon(int weaponIdx, bool silent);
	void updateQuests(short questId, int status);
	void setQuestTile(int questId, int tileX, int tileY);
	bool isQuestDone(int questIdx);
	bool isQuestFailed(int questIdx);
	void formatTime(int millis, Text* text);
	void showInvHelp(int itemIdx, bool force);
	void showAmmoHelp(int ammoType, bool force);
	bool showHelp(short helpId, bool force);
	void showWeaponHelp(int weaponIdx, bool force);
	void drawBuffs(Graphics* graphics);
	void setCharacterChoice(short choice);
	bool loadState(InputStream* IS);
	bool saveState(OutputStream* OS);
	void unpause(int delay);
	void relink();
	void unlink();
	void link();
	void updateStats();
	void updateStatusEffects();
	void translateStatusEffects();
	void removeStatusEffect(int effectIdx);
	bool addStatusEffect(int effectIdx, int amount, int duration);
	void drawStatusEffectIcon(Graphics* graphics, int effectIdx, int amount, int duration, int x, int y);
	void resetCounters();
	Entity* getPlayerEnt();
	void setPickUpWeapon(int tileIdx);
	void giveAll();
	void equipForLevel(int highestMap);
	bool addArmor(int amount);
	int distFrom(Entity* entity);
	void showAchievementMessage(int achievementIdx);
	short gradeToString(int grade);
	int levelGrade(bool record);
	int finalCurrentGrade();
	int finalBestGrade();
	int getCurrentGrade(int mapId);
	void setCurrentGrade(int mapId, int grade);
	int getBestGrade(int mapId);
	void setBestGrade(int mapId, int grade);
	bool hasPurifyEffect();
	void setFamiliar(short familiarType);
	short unsetFamiliar(bool keepCurrentPos);
	void clearOutFamiliarsStatusEffects();
	void swapStatusEffects();
	void familiarDied();
	void explodeFamiliar(int tileX, int tileY, int familiarType);
	void familiarReturnsToPlayer(bool keepCurrentPos);
	bool stealFamiliarsInventory();
	void handleBotRemains(int x, int y, int corpseType);
	void forceFamiliarReturnDueToMonster();
	void attemptToDeploySentryBot();
	void attemptToDiscardFamiliar(int weaponIdx);
	void startSelfDestructDialog();
	bool vendingMachineIsHacked(int vmId);
	void setVendingMachineHack(int vmId);
	int getVendingMachineTriesLeft(int max);
	void removeOneVendingMachineTry(int max);
	bool weaponIsASentryBot(int weaponIdx);
	bool hasASentryBot();
	void setFamiliarType(short familiarType);
	void calcViewMode();
	void enterTargetPractice(int tileX, int tileY, int direction, ScriptThread* targetPracticeThread);
	void assessTargetPracticeShot(Entity* entity);
	void exitTargetPractice();
	void onWeaponKill(int weaponIdx, bool bonusHit);
	bool hasANanoDrink();
	void stripInventoryForViosBattle();
	void stripInventoryForTargetPractice();
	void restoreInventory();
	void forceRemoveFromScopeZoom();
};
