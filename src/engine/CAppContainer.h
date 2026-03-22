#ifndef __CAPPCONTAINER_H__
#define __CAPPCONTAINER_H__
#include <string>
#include <vector>
#include "App.h"
#include "OpcodeRegistry.h"

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
	const char* customMapFile;     // --map override
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
