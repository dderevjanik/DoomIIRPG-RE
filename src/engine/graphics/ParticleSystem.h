#pragma once
#include <cstdint>
#include <vector>

class ParticleSystem;
class Image;
class Graphics;
class Entity;
class DataNode;

// ----------------------
// ParticleEmitter Class
// ----------------------

class ParticleEmitter
{
private:

public:
	ParticleSystem* systems = nullptr;
	short pos[3] = {};
	int gravity = 0;
	uint8_t gibType = 0;
	int color = 0;
	int startTime = 0;

	// Constructor
	ParticleEmitter();
	// Destructor
	~ParticleEmitter();

	bool startup();
	void render(Graphics* graphics, int n);
};

// ---------------------
// ParticleSystem Class
// ---------------------

class Applet;

class ParticleSystem
{
private:

public:
	Applet* app;  // Set in startup(), replaces CAppContainer::getInstance()->app

	// Default palette matches the original 7 hardcoded level theme colors.
	// Loaded from particles.yaml at startup; per-level level.yaml may override via setLevelPalette().
	static constexpr uint32_t kDefaultPalette[7] = { 0xFFFFFFFF, 0xFF3A332E, 0xFF626262, 0xFF7F7F7F, 0xFFD1CF11, 0xFFFD8918, 0xFF20B00D };
	std::vector<uint32_t> levelColors{ kDefaultPalette, kDefaultPalette + 7 };

	static constexpr int32_t rotationSequence[] = {0, 4, 2, 6};
	static constexpr int GIB_BONE_MASK = 0x619;

	// Replace the active palette. If `colors` is empty, falls back to the default palette.
	void setLevelPalette(const std::vector<uint32_t>& colors);
	// Get a palette color by index, clamped to the active palette (returns white if empty).
	uint32_t getPaletteColor(int idx) const;

	int systemIdx = 0;
	ParticleEmitter particleEmitter[4] = {};
	Image* imgGibs[3] = {};
	uint8_t* monsterColors = nullptr;
	uint8_t particleNext[64] = {};
	uint8_t particleStartX[64] = {};
	uint8_t particleStartY[64] = {};
	int16_t particleVelX[64] = {};
	int16_t particleVelY[64] = {};
	uint8_t particleSize[32] = {};
	int clipRect[4] = {};

	// Constructor
	ParticleSystem();
	// Destructor
	~ParticleSystem();

	bool startup();
	void freeAllParticles();
	int unlinkParticle(int i);
	void linkParticle(int i, int i2);
	void freeSystem(int n);
	void renderSystems(Graphics* graphics);
	void spawnMonsterBlood(Entity* entity, bool b);
	void spawnParticles(int n, int n2, int n3);
	void spawnParticles(int n, int color, int n2, int n3, int n4);
	
};
