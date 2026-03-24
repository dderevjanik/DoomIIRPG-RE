#ifndef __CAPPCONTAINER_H__
#define __CAPPCONTAINER_H__
#include <string>
#include <vector>
#include "App.h"
#include "OpcodeRegistry.h"
#include "MinigameRegistry.h"

class VFS;
class SDLGL;
class Applet;
class IGameModule;

// Game-level configuration loaded from game.yaml
struct GameConfig {
	std::string name = "Doom II RPG";
	std::string windowTitle = "Doom II RPG";
	std::string saveDir = "Doom2rpg.app";
	std::string entryMap = "map00";
	std::vector<int> noFogMaps;  // Map IDs where fog is disabled (e.g. outdoor maps)
	std::vector<std::string> searchDirs;  // VFS search subdirectories for asset resolution

	// Inventory/ammo capacity caps
	int capCredits = 9999;
	int capInventory = 999;
	int capAmmo = 100;
	int capBotFuel = 5;

	// XP formula coefficients: linear*n + cubic*((n-1)^3 + (n-1))
	int xpLinear = 500;
	int xpCubic = 100;

	// Entity limits
	int maxEntities = 275;

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

	// Vending machine minigame
	struct IQHint {
		int iq;
		int hints;
	};
	int vendSliderMin = 0;
	int vendSliderMax = 9;
	int vendSliderStart = 5;
	std::vector<IQHint> vendIQHints = { {80, 3}, {50, 2}, {20, 1} };

	bool isFogDisabled(int mapID) const {
		for (int id : noFogMaps) {
			if (id == mapID) return true;
		}
		return false;
	}
};

class CAppContainer {
  private:
  public:
	Applet* app;
	SDLGL* sdlGL;
	VFS* vfs;
	GameConfig gameConfig;         // Loaded from game.ini
	OpcodeRegistry opcodeRegistry; // Extension script opcodes (128-254)
	MinigameRegistry minigameRegistry; // Minigame dispatch registry
	const char* customMapFile;     // --map override
	const char* minigameName;     // --minigame override
	bool skipTravelMap;            // --skip-travel-map
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

#endif
