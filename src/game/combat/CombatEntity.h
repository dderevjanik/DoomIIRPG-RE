#pragma once
class InputStream;
class OutputStream;
class Entity;

class Applet;

class CombatEntity
{
private:

public:
	Applet* app = nullptr;  // Set lazily, replaces CAppContainer::getInstance()->app
	int touchMe = 0;
	int stats[8] = {};
	int weapon = -1;

	// Constructor
	CombatEntity();
	CombatEntity(int health, int armor, int defense, int strength, int accuracy, int agility);
	// Destructor
	~CombatEntity();

	void clone(CombatEntity* ce);
	int getStat(int i);
	int getStatPercent(int i);
	int getIQPercent();
	int addStat(int i, int i2);
	int setStat(int i, int i2);
	void loadState(InputStream* inputStream, bool b);
	void saveState(OutputStream* outputStream, bool b);
	void calcCombat(CombatEntity* combatEntity, Entity* entity, bool b, int n, int n2);
	int calcHit(CombatEntity* ce, CombatEntity* ce2, bool b, int i, bool b2);
	int calcDamage(CombatEntity* ce, Entity* entity, CombatEntity* ce2, bool b, int n);
};
