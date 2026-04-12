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
	static constexpr int BUFF_AMOUNT = 15;
	static constexpr int DEF_STATUS_TURNS = 30;
	static constexpr int ANTI_FIRE_TURNS = 10;
	static constexpr int AGILITY_TURNS = 20;
	static constexpr int PURIFY_TURNS = 10;
	static constexpr int FEAR_TURNS = 6;
	static constexpr int COLD_TURNS = 5;

	// Buff data loaded from effects.yaml
	int8_t buffMaxStacks[15] = {};
	int8_t buffBlockedBy[15] = {};     // -1 = none
	int16_t buffApplySound[15] = {};   // -1 = none
	int8_t buffPerTurnDamage[15] = {}; // 0 = none, >0 = flat damage per turn
	bool buffPerTurnHealByAmount[15] = {}; // true = heals by buff amount per turn
	int buffNoAmountMask = 0;
	int buffAmtNotDrawnMask = 0;
	int buffWarningTime = 0;

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
	void selectWeapon(int i);
	void selectPrevWeapon();
	void selectNextWeapon();
	int getHealth();
	int modifyStat(int n, int n2);
	bool requireStat(int n, int n2);
	bool requireItem(int n, int n2, int n3, int n4);
	void addXP(int xp);
	void addLevel();
	int calcLevelXP(int n);
	int calcScore();
	bool addHealth(int i);
	bool addHealth(int i, bool b);
	void setStatsAccordingToCharacterChoice();
	void reset();
	int calcDamageDir(int x1, int y1, int angle, int x2, int y2);
	void painEvent(Entity* entity, bool b);
	void pain(int n, Entity* entity, bool b);
	void died();
	void familiarDying(bool familiarSelfDestructed);
	bool fireWeapon(Entity* entity, int n, int n2);
	bool useItem(int n);
	bool give(int n, int n2, int n3);
	bool give(int n, int n2, int n3, bool b);
	bool give(int n, int n2, int n3, bool b, bool b2);
	void giveAmmoWeapon(int n, bool b);
	void updateQuests(short n, int n2);
	void setQuestTile(int n, int n2, int n3);
	bool isQuestDone(int n);
	bool isQuestFailed(int n);
	void formatTime(int n, Text* text);
	void showInvHelp(int n, bool b);
	void showAmmoHelp(int n, bool b);
	bool showHelp(short n, bool b);
	void showWeaponHelp(int n, bool b);
	void drawBuffs(Graphics* graphics);
	void setCharacterChoice(short i);
	bool loadState(InputStream* IS);
	bool saveState(OutputStream* OS);
	void unpause(int n);
	void relink();
	void unlink();
	void link();
	void updateStats();
	void updateStatusEffects();
	void translateStatusEffects();
	void removeStatusEffect(int n);
	bool addStatusEffect(int n, int n2, int n3);
	void drawStatusEffectIcon(Graphics* graphics, int n, int n2, int n3, int n4, int n5);
	void resetCounters();
	Entity* getPlayerEnt();
	void setPickUpWeapon(int n);
	void giveAll();
	void equipForLevel(int highestMap);
	bool addArmor(int n);
	int distFrom(Entity* entity);
	void showAchievementMessage(int n);
	short gradeToString(int n);
	int levelGrade(bool b);
	int finalCurrentGrade();
	int finalBestGrade();
	int getCurrentGrade(int n);
	void setCurrentGrade(int n, int n2);
	int getBestGrade(int n);
	void setBestGrade(int n, int n2);
	bool hasPurifyEffect();
	void setFamiliar(short familiarType);
	short unsetFamiliar(bool b);
	void clearOutFamiliarsStatusEffects();
	void swapStatusEffects();
	void familiarDied();
	void explodeFamiliar(int n, int n2, int n3);
	void familiarReturnsToPlayer(bool b);
	bool stealFamiliarsInventory();
	void handleBotRemains(int n, int n2, int n3);
	void forceFamiliarReturnDueToMonster();
	void attemptToDeploySentryBot();
	void attemptToDiscardFamiliar(int n);
	void startSelfDestructDialog();
	bool vendingMachineIsHacked(int n);
	void setVendingMachineHack(int n);
	int getVendingMachineTriesLeft(int max);
	void removeOneVendingMachineTry(int max);
	bool weaponIsASentryBot(int n);
	bool hasASentryBot();
	void setFamiliarType(short familiarType);
	void calcViewMode();
	void enterTargetPractice(int n, int n2, int n3, ScriptThread* targetPracticeThread);
	void assessTargetPracticeShot(Entity* entity);
	void exitTargetPractice();
	void onWeaponKill(int weaponIdx, bool bonusHit);
	bool hasANanoDrink();
	void stripInventoryForViosBattle();
	void stripInventoryForTargetPractice();
	void restoreInventory();
	void forceRemoveFromScopeZoom();
};
