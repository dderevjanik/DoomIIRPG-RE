#pragma once
#include <memory>
#include <typeindex>
#include <unordered_map>
#include "Component.h"

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
	// Generic component storage for mod-defined components
	std::unordered_map<std::type_index, std::unique_ptr<Component>> components;

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

	// Generic component system — for mod-defined components
	template<typename T, typename... Args>
	T* addComponent(Args&&... args) {
		auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
		T* raw = ptr.get();
		components[componentTypeId<T>()] = std::move(ptr);
		return raw;
	}

	template<typename T>
	T* getComponent() {
		auto it = components.find(componentTypeId<T>());
		return it != components.end() ? static_cast<T*>(it->second.get()) : nullptr;
	}

	template<typename T>
	const T* getComponent() const {
		auto it = components.find(componentTypeId<T>());
		return it != components.end() ? static_cast<const T*>(it->second.get()) : nullptr;
	}

	template<typename T>
	bool hasComponent() const {
		return components.count(componentTypeId<T>()) > 0;
	}

	template<typename T>
	void removeComponent() {
		components.erase(componentTypeId<T>());
	}

	void clearComponents() { components.clear(); }

	bool startup();
	void reset();
	void initspawn();
	int getSprite();
	bool touched();
	bool touchedItem();
	bool pain(int damage, Entity* entity);
	int checkMonsterDeath(bool awardXP, bool playDeathSound);
	void died(bool awardXP, Entity* entity);
	bool deathByExplosion(Entity* entity);
	void aiCalcSimpleGoal(bool forceNewGoal);
	void aiMoveToGoal();
	void aiChooseNewGoal(bool forceNewGoal);
	bool aiIsValidGoal();
	bool aiIsAttackValid();
	void aiThink(bool forceNewGoal);
	static bool CheckWeaponMask(char n1, int n2) {
		return (1 << n1 & n2);
	}
	int aiWeaponForTarget(Entity* entity);
	LerpSprite* aiInitLerp(int travelTime);
	void aiFinishLerp();
	bool checkLineOfSight(int startX, int startY, int endX, int endY, int typeMask);
	bool calcPath(int curTileX, int curTileY, int goalTileX, int goalTileY, int traceMask, bool retreat);
	bool aiGoal_MOVE();
	void aiReachedGoal_MOVE();
	int distFrom(int x, int y);
	void attack();
	void undoAttack();
	void trimCorpsePile(int x, int y);
	void knockback(int srcX, int srcY, int distance);
	int getFarthestKnockbackDist(int srcX, int srcY, int dstX, int dstY, Entity* entity, int traceMask, int radius, int maxDist);
	int findRaiseTarget(int maxRange, int requireFlags, int excludeFlags);
	void raiseTarget(int entityIdx);
	void resurrect(int x, int y, int z);
	int* calcPosition();
	bool isBoss();
	bool isHasteResistant();
	bool isDroppedEntity();
	bool isBinaryEntity(int* outFlags);
	bool isNamedEntity(int* outName);
	void saveState(OutputStream* OS, int saveFlags);
	void loadState(InputStream* IS, int saveFlags);
	int getSaveHandle(bool briefSave);
	void restoreBinaryState(int saveFlags);
	short getIndex();
	void updateMonsterFX();
	void populateDefaultLootSet();
	void addToLootSet(int n, int n2, int n3);
	bool hasEmptyLootSet();
	
	

	
	
};
