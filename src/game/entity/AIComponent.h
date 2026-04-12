#pragma once
#include <cstring>
#include <stdint.h>

class Entity;
class OutputStream;
class InputStream;

struct AIComponent
{
	Entity* target;
	int frameTime;
	uint8_t goalType;
	uint8_t goalFlags;
	uint8_t goalTurns;
	int goalX;
	int goalY;
	int goalParam;

	AIComponent();
	void reset();
	void resetGoal();
	void saveGoalState(OutputStream* OS);
	void loadGoalState(InputStream* IS);
};
