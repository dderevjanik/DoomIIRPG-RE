#include "AIComponent.h"
#include "JavaStream.h"

AIComponent::AIComponent() {
	std::memset(this, 0, sizeof(AIComponent));
}

void AIComponent::reset() {
	this->target = nullptr;
	this->frameTime = 0;
	this->resetGoal();
}

void AIComponent::resetGoal() {
	this->goalType = 0;
	this->goalFlags = 0;
	this->goalTurns = 0;
	this->goalY = 0;
	this->goalX = 0;
	this->goalParam = 0;
}

void AIComponent::saveGoalState(OutputStream* OS) {
	OS->writeInt(this->goalType | this->goalFlags << 4 | this->goalTurns << 8 | this->goalX << 12 | this->goalY << 17 | this->goalParam << 22);
}

void AIComponent::loadGoalState(InputStream* IS) {
	int args = IS->readInt();
	this->goalType = (uint8_t)(args & 0xF);
	this->goalFlags = (uint8_t)(args >> 4 & 0xF);
	this->goalTurns = (uint8_t)(args >> 8 & 0xF);
	this->goalX = (args >> 12 & 0x1F);
	this->goalY = (args >> 17 & 0x1F);
	this->goalParam = (args >> 22 & 0x3FF);
}
