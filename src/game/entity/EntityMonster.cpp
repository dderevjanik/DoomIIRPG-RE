#include <stdexcept>

#include "EntityMonster.h"

EntityMonster::EntityMonster() {
	std::memset(this, 0, sizeof(EntityMonster));
}
