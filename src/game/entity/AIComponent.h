#pragma once
#include <cstring>
#include <stdint.h>

class Entity;
class OutputStream;
class InputStream;

struct AIComponent
{
	Entity* target = nullptr;
	int frameTime = 0;
	uint8_t goalType = 0;
	uint8_t goalFlags = 0;
	uint8_t goalTurns = 0;
	int goalX = 0;
	int goalY = 0;
	int goalParam = 0;

	AIComponent() = default;
	void reset();
	void resetGoal();
	void saveGoalState(OutputStream* OS);
	void loadGoalState(InputStream* IS);
};
