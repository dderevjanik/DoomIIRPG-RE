#pragma once
#include <flat_map>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>
#include "App.h"
#include "OpcodeRegistry.h"
#include "MinigameRegistry.h"

class DevConsole;

class VFS;
class SDLGL;
class Applet;
class IGameModule;
class ResourceManager;

// Per-level info populated from game.yaml levels section
struct LevelInfo {
	std::string dir;         // e.g. "levels/01_tycho"
	std::string mapFile;     // e.g. "levels/01_tycho/map.bin"
	std::string modelFile;   // e.g. "levels/01_tycho/model.bin"
	std::string configFile;  // e.g. "levels/01_tycho/level.yaml"
	std::string skyBox;      // Named sky from game.yaml skies section; empty = legacy formula
};

// Game-level configuration loaded from game.yaml
struct GameConfig {
	std::string name = "DRPG Engine";
	std::string windowTitle = "DRPG Engine";
	std::string module = "doom2rpg";
	std::string saveDir = "Doom2rpg.app";
	std::string entryMap = "map00";
	std::vector<int> noFogMaps;  // Map IDs where fog is disabled (e.g. outdoor maps)
	std::vector<std::string> searchDirs;  // VFS search subdirectories for asset resolution
	std::unordered_map<int, std::string> stringFiles;  // group index -> YAML file path (from game.yaml strings:)
	std::flat_map<int, LevelInfo> levelInfos;  // map_id -> per-level paths
	std::vector<std::string> entityFiles = {"entities.yaml"};  // entity definition files (from game.yaml entities:)

	// Inventory/ammo capacity caps
	int capCredits = 9999;
	int capInventory = 999;
	int capAmmo = 100;
	int capBotFuel = 5;

	// XP formula coefficients: linear*n + cubic*((n-1)^3 + (n-1))
	int xpLinear = 500;
	int xpCubic = 100;

	// Player starting stats
	int startingMaxHealth = 100;

	// Level-up stat grants
	int levelUpHealth = 10;
	int levelUpDefense = 1;
	int levelUpStrength = 2;
	int levelUpAccuracy = 1;
	int levelUpAgility = 3;


	// Out-of-combat cooldown (turns since last combat before "inCombat" clears)
	int outOfCombatTurns = 4;

	// Scoring formula constants
	int scorePerLevel = 1000;
	int scoreAllLevelsBonus = 1000;
	int scoreNoDeathsBonus = 1000;
	int scoreDeathPenaltyBase = 5;
	int scoreDeathPenaltyMult = 50;
	int scoreManyDeathsPenalty = 250;
	int scoreDeathThreshold = 10;
	int scoreTimeBonusMinutes = 120;
	int scoreTimeBonusMult = 15;
	int scoreMoveThreshold = 5000;
	int scoreMoveDivisor = 2;
	int scorePerSecret = 1000;
	int scoreAllSecretsBonus = 1000;

	// Entity limits
	int maxEntities = 275;

	// HUD limits
	int maxWeaponButtons = 15;

	// Damage vignette direction masks (indexed by damageDir 0-7)
	std::vector<int> damageVignetteDirs = {8, 2, 3, 15, 5, 4, 8, 8};

	// Dialog bubble colors
	struct BubbleColor {
		unsigned int color;
		int offset;
		int tailWidth;
	};
	std::vector<BubbleColor> bubbleColors = {
		{0xFF800000, 0, 6},
		{0xFF002864, 10, 6},
		{0xFF2E0854, 20, 6},
		{0xFF005617, 30, 12},
		{0xFFFF9600, 45, 12},
	};

	// Target practice scoring
	int tpHeadPoints = 30;
	int tpBodyPoints = 20;
	int tpLegPoints = 10;
	int tpHitDisplayMs = 500;
	int tpWeaponIdx = 9;      // Weapon given during target practice (assault_rifle_with_scope)
	int tpAmmoType = 1;       // Ammo type for target practice (bullets)
	int tpAmmoCount = 8;      // Amount of ammo given for target practice

	// Vending machine minigame
	struct IQHint {
		int iq;
		int hints;
	};
	int vendSliderMin = 0;
	int vendSliderMax = 9;
	int vendSliderStart = 5;
	std::vector<IQHint> vendIQHints = { {80, 3}, {50, 2}, {20, 1} };

	// Per-map joke item tables: mapID → list of item IDs (selected by sprite % count)
	std::flat_map<int, std::vector<int>> jokeItems;

	// Automap color configuration
	struct AutomapDoorColor {
		int16_t tileIndex;
		uint32_t color;
	};
	std::vector<AutomapDoorColor> automapDoorColors = {
		{271, 0xFFFF8400}, {272, 0xFFFF8400},   // orange key doors
		{273, 0xFF00C0FF}, {274, 0xFF00C0FF},   // blue key doors
	};
	uint32_t automapDoorDefault = 0xFF3D68E3;         // default door color
	std::vector<int16_t> automapHiddenDecors = {173, 180}; // decor tiles invisible on automap

	bool isFogDisabled(int mapID) const {
		for (int id : noFogMaps) {
			if (id == mapID) return true;
		}
		return false;
	}

	// Get map file path for a given map ID (falls back to legacy mapNN.bin naming)
	std::string getMapFile(int mapId) const {
		if (auto it = levelInfos.find(mapId); it != levelInfos.end()) return it->second.mapFile;
		return std::format("map{:02d}.bin", mapId - 1);
	}
};

class CAppContainer {
  private:
  public:
	Applet* app;
	SDLGL* sdlGL;
	VFS* vfs;
	ResourceManager* resourceManager;
	GameConfig gameConfig;         // Loaded from game.yaml
	OpcodeRegistry opcodeRegistry; // Extension script opcodes (128-254)
	MinigameRegistry minigameRegistry; // Minigame dispatch registry
	DevConsole* devConsole = nullptr;  // ImGui developer console overlay (set by Main)
	const char* customMapFile;     // --map override (raw file path, used when no level ID match)
	int customMapID;               // --map override resolved to a level ID (0 = not set)
	const char* minigameName;     // --minigame override
	int pendingEquipLevel;         // deferred equipForLevel after --map load (0 = none)
	bool skipTravelMap;            // --skip-travel-map
	bool skipIntro;                // --skip-intro
	bool headless;                 // --headless mode (no rendering, no audio)
	uint32_t headlessTimeMs;       // deterministic tick counter for headless mode
	static CAppContainer* getInstance();
	static int m_cheatEntry;
	float MoveX, MoveY, MoveAng;

	// Constructor
	CAppContainer();
	// Destructor
	~CAppContainer();

	short* GetBackBuffer();
	void DoLoop(int time);
	void suspendOpenAL();
	void resumeOpenAL();
	void userPressed(float pressX, float pressY);
	void userMoved(float pressX, float pressY);
	void userReleased(float pressX, float pressY);
	void unHighlightButtons();
	uint32_t getTimeMS();
	void saveGame(int i, int* i2);
	void TestCheatEntry(float pressX, float pressY);
	bool testCheatCode(int code);
	void UpdateAccelerometer(float x, float y, float z, bool useMouse);
	void Construct(SDLGL* sdlGL, VFS* vfs, IGameModule* gameModule = nullptr);
};
