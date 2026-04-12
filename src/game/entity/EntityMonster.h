#ifndef __ENTITYMONSTER_H__
#define __ENTITYMONSTER_H__

#include <cstring>
#include <stdint.h>

#include "CombatEntity.h"
class OutputStream;
class InputStream;
class CombatEntity;
class Entity;

class EntityMonster
{
private:

public:
	int touchMe;
	CombatEntity ce;

	// Constructor
	EntityMonster();
};

#endif