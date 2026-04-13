#pragma once
class Applet;
struct AIComponent;
class CombatEntity;
class EntityDef;
class EntityMonster;
class LerpSprite;
class OutputStream;
class InputStream;
struct LootComponent;

struct GameConfig;

class Entity
{
private:

public:
	Applet* app = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->app
	const GameConfig* gameConfig = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->gameConfig
	int touchMe = 0;
	EntityDef* def = nullptr;
	EntityMonster* monster = nullptr;
	CombatEntity* combat = nullptr;
	AIComponent* ai = nullptr;
	Entity* nextOnTile = nullptr;
	Entity* prevOnTile = nullptr;
	short linkIndex = 0;
	short name = 0;
	int info = 0;
	int param = 0;
	LootComponent* loot = nullptr;
	short monsterFlags = 0;
	int monsterEffects = 0;
	int32_t knockbackDelta[2] = {};
	Entity* raiseTargets[4] = {};
	Entity* nextOnList = nullptr;
	Entity* prevOnList = nullptr;
	Entity* nextAttacker = nullptr;
	int pos[2] = {};
	int tempSaveBuf[2] = {};

	// Constructor
	Entity();
	// Destructor
	~Entity();

	bool isMonster() const { return combat != nullptr && ai != nullptr; }
	void clearMonsterEffects() { monsterEffects = 0; }

	bool startup();
	void reset();
	void initspawn();
	int getSprite();
	bool touched();
	bool touchedItem();
	bool pain(int n, Entity* entity);
	int checkMonsterDeath(bool b, bool b2);
	void died(bool b, Entity* entity);
	bool deathByExplosion(Entity* entity);
	void aiCalcSimpleGoal(bool b);
	void aiMoveToGoal();
	void aiChooseNewGoal(bool b);
	bool aiIsValidGoal();
	bool aiIsAttackValid();
	void aiThink(bool b);
	static bool CheckWeaponMask(char n1, int n2) {
		return (1 << n1 & n2);
	}
	int aiWeaponForTarget(Entity* entity);
	LerpSprite* aiInitLerp(int travelTime);
	void aiFinishLerp();
	bool checkLineOfSight(int n, int n2, int n3, int n4, int n5);
	bool calcPath(int n, int n2, int n3, int n4, int n5, bool b);
	bool aiGoal_MOVE();
	void aiReachedGoal_MOVE();
	int distFrom(int n, int n2);
	void attack();
	void undoAttack();
	void trimCorpsePile(int n, int n2);
	void knockback(int n, int n2, int n3);
	int getFarthestKnockbackDist(int n, int n2, int n3, int n4, Entity* entity, int n5, int n6, int n7);
	int findRaiseTarget(int n, int n2, int n3);
	void raiseTarget(int n);
	void resurrect(int n, int n2, int n3);
	int* calcPosition();
	bool isBoss();
	bool isHasteResistant();
	bool isDroppedEntity();
	bool isBinaryEntity(int* array);
	bool isNamedEntity(int* array);
	void saveState(OutputStream* OS, int n);
	void loadState(InputStream* IS, int n);
	int getSaveHandle(bool b);
	void restoreBinaryState(int n);
	short getIndex();
	void updateMonsterFX();
	void populateDefaultLootSet();
	int findRandomJokeItem();
	void addToLootSet(int n, int n2, int n3);
	bool hasEmptyLootSet();
	
	

	
	
};
