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
class ModManager;

class VFS;
class SDLGL;
class Applet;
class IGameModule;
class ResourceManager;

// Per-level info populated by scanning levels/*/level.yaml at startup
struct LevelInfo {
	std::string dir;         // e.g. "levels/01_tycho"
	std::string mapFile;     // e.g. "levels/01_tycho/map.bin"
	std::string modelFile;   // e.g. "levels/01_tycho/model.bin"
	std::string configFile;  // e.g. "levels/01_tycho/level.yaml"
	std::string skyBox;      // Named sky from game.yaml skies section; empty = legacy formula
	std::string skyTexture;  // PNG file path for sky texture; empty = fallback to skyBox/tables.bin
	// Level intro config (from level.yaml intro section)
	std::string introType;   // e.g. "travel_map"
	std::string introFile;   // e.g. "travel_map.yaml" (relative to level dir)
};

// Game-level configuration loaded from game.yaml
struct GameConfig {
	std::string name = "DRPG Engine";
	std::string windowTitle = "DRPG Engine";
	std::string module = "doom2rpg";
	std::string saveDir = "Doom2rpg.app";
	std::string entryMap = "map00";
	// Optional game intro played before main menu (mirrors level.yaml intro schema).
	std::string introType;   // e.g. "travel_map"; empty = skip intro
	std::string introFile;   // path to intro scene YAML (relative to game dir)
	std::vector<int> noFogMaps;  // Map IDs where fog is disabled (e.g. outdoor maps)
	std::vector<std::string> searchDirs;  // VFS search subdirectories for asset resolution
	std::unordered_map<int, std::string> stringFiles;  // group index -> YAML file path (from game.yaml strings:)
	std::flat_map<int, LevelInfo> levelInfos;  // level id -> per-level paths
	std::vector<std::string> entityFiles = {"entities.yaml"};  // entity definition files (from game.yaml entities:)

	// Inventory/ammo capacity caps
	int capCredits = 9999;
	int capInventory = 999;
	int capAmmo = 100;
	int capBotFuel = 5;
	int capArmor = 200;                    // max armor value

	// Familiar (sentry bot) max health — used as stat2 for familiar addHealth checks
	int familiarMaxHealth = 100;

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

	// Global trinket strings (indexed by LootConfig::trinketStringIdx)
	std::vector<std::string> trinketStrings;

	// Combat timing/scale constants (from game.yaml combat section)
	int combatMaxActiveMissiles = 8;
	int combatPlacingBombTime = 750;       // ms to place a bomb
	int combatBombRecoverTime = 500;       // ms recovery after bomb placement
	int combatWeaponScale = 131072;        // fixed-point weapon rendering scale (17-bit FP)
	int combatLowerWeaponTime = 200;       // ms to lower weapon animation
	int combatLoweredWeaponY = 38;         // Y offset for lowered weapon position
	int combatExplosionOffset = 48;        // explosion sprite offset
	int combatExplosionOffset2 = 38;       // secondary explosion offset
	int combatPlacingBombZ = 18;           // Z offset for bomb placement visual
	int combatRockTimeDamage = 200;        // screen-shake duration on damage (ms)
	int combatRockTimeDodge = 800;         // screen-shake duration on dodge (ms)
	int combatRockDistCombat = 6;          // screen-shake distance in combat

	// HUD timing constants (from game.yaml hud section)
	int hudMsgDisplayTime = 700;           // base message display time (ms)
	int hudMsgFlashTime = 100;             // message flash/fade time (ms)
	int hudScrollStartDelay = 750;         // delay before text starts scrolling (ms)
	int hudMsPerChar = 64;                 // ms per character for scrolling text
	int hudBubbleTextTime = 1500;          // speech bubble display time (ms)
	int hudSentryBotIconsPadding = 15;     // padding for sentry bot icons
	int hudDamageOverlayTime = 1000;       // damage vignette overlay duration (ms)
	int hudActionIconSize = 18;            // action icon pixel size
	int hudArrowsSize = 12;               // arrow control pixel size

	// Render timing/visual constants (from game.yaml render section)
	int renderChangemapFadeTime = 1000;    // fade duration for map transitions (ms)
	int renderHoldBrightnessTime = 500;    // hold at max brightness (ms)
	int renderFadeBrightnessTime = 500;    // fade from max brightness (ms)
	int renderMaxVScrollVelocity = 30;     // max vertical scroll velocity
	int renderMaxInitialVScrollVelocity = 90;  // max initial scroll velocity
	int renderPortalMaxRadius = 66;        // portal visual max radius
	int renderAnimIdleTime = 8192;         // idle animation cycle time
	int renderAnimIdleSwitchTime = 256;    // idle animation switch interval
	int renderMaxDizzy = 30;               // max dizzy effect intensity
	int renderLatencyAdjust = 50;          // latency adjustment for animations (ms)
	int renderFearEyeSize = 8;            // fear-eye sprite size

	// Player gameplay constants (from game.yaml player section)
	int playerExpireDuration = 5;          // buff expiration turns
	int playerMaxDisplayBuffs = 6;         // max buffs shown on HUD
	int playerIceFogDist = 1024;           // ice fog visibility distance
	int playerMaxNotebookIndexes = 8;      // notebook capacity
	int playerBitsPerVmTry = 3;            // bits per vending machine attempt

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

	// Automap entity colors (from game.yaml automap.entity_colors section)
	struct AutomapColors {
		uint32_t entrance = 0xFF00FF00;      // green — map entrance marker
		uint32_t exit = 0xFFFF0000;          // red — map exit marker
		uint32_t visited = 0xFF90B9E7;       // light blue — visited tile
		uint32_t background = 0xFF2A3657;    // dark blue — unvisited/background
		uint32_t ladder = 0xFFFFFF00;        // yellow — ladder marker
		uint32_t ladderStripe = 0xFF000000;  // black — ladder stripe overlay
		uint32_t npc = 0xFF0000FF;           // blue — NPC marker
		uint32_t interactive = 0xFF8000FF;   // purple — interactive entity
		uint32_t monster = 0xFFFF8000;       // orange — monster marker
		uint32_t wall = 0xFF8D8068;          // brown — wall/decor
		uint32_t godModeItem = 0xFF00FFEA;   // cyan — item (god mode only)
		uint32_t questBlinkA = 0xFFFF0000;   // red — quest objective blink A
		uint32_t questBlinkB = 0xFF00FF00;   // green — quest objective blink B
	};
	AutomapColors automapColors;

	// Automap layout constants (from game.yaml automap section)
	int automapCellSize = 8;               // pixel size of each map cell
	int automapOffsetX = 112;              // X offset of map grid on screen
	int automapOffsetY = 32;               // Y offset of map grid on screen
	int automapBlinkInterval = 333;        // quest marker blink interval (ms)

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
	ModManager* modManager = nullptr;  // Mod discovery, dependency resolution, load tracking
	const char* customMapFile;     // --map override (raw file path, used when no level ID match)
	int customMapID;               // --map override resolved to a level ID (0 = not set)
	const char* minigameName;     // --minigame override
	int pendingEquipLevel;         // deferred equipForLevel after --map load (0 = none)
	bool skipTravelMap;            // --skip-travel-map
	bool skipIntro;                // --skip-intro
	bool headless;                 // --headless mode (no rendering, no audio)
	bool editorMode = false;       // --editor flag: jump to ST_EDITOR after startup
	std::string editorProjectPath; // --editor <path>: project YAML to open; empty = blank
	std::string originalCwd;       // absolute path to the CWD at launch (pre-chdir-to-gamedir)
	uint32_t headlessTimeMs;       // deterministic tick counter for headless mode
	static CAppContainer* getInstance();
	static int m_cheatEntry;
	float MoveX, MoveY, MoveAng;

	// Constructor
	CAppContainer();
	// Destructor
	~CAppContainer();

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
