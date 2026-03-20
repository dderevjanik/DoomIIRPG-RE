#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <limits.h>
#include <unistd.h>

#include <SDL.h>
#include "SDLGL.h"
#include "ZipFile.h"
#include "VFS.h"
#include "INIReader.h"

#include "CAppContainer.h"
#include "App.h"
#include "Image.h"
#include "Resource.h"
#include "Render.h"
#include "GLES.h"
#include "JavaStream.h"

#include "Canvas.h"
#include "Graphics.h"
#include "Player.h"
#include "Game.h"
#include "Graphics.h"
#include "Utils.h"
#include "TinyGL.h"
#include "Input.h"

void drawView(SDLGL* sdlGL);

int main(int argc, char* args[]) {

	int UpTime = 0;

	const char* ipaPath = "Doom 2 RPG.ipa";
	const char* gameDir = ".";
	const char* gameName = nullptr;
	const char* customMap = nullptr;

	for (int i = 1; i < argc; i++) {
		if (strcmp(args[i], "--ipa") == 0 && i + 1 < argc) {
			ipaPath = args[++i];
		} else if (strcmp(args[i], "--gamedir") == 0 && i + 1 < argc) {
			gameDir = args[++i];
		} else if (strcmp(args[i], "--game") == 0 && i + 1 < argc) {
			gameName = args[++i];
		} else if (strcmp(args[i], "--map") == 0 && i + 1 < argc) {
			customMap = args[++i];
		}
	}

	// --game <name> is sugar for --gamedir games/<name>
	std::string gameDirStr;
	if (gameName) {
		gameDirStr = std::string("games/") + gameName;
		gameDir = gameDirStr.c_str();
		printf("Game: %s (directory: %s)\n", gameName, gameDir);
	}

	// Resolve IPA path to absolute before any chdir
	char ipaAbsPath[PATH_MAX];
	if (ipaPath[0] != '/') {
		if (realpath(ipaPath, ipaAbsPath)) {
			ipaPath = ipaAbsPath;
		}
	}

	// Change working directory to gamedir so .ini files are found there
	if (strcmp(gameDir, ".") != 0) {
		if (chdir(gameDir) != 0) {
			printf("Error: cannot change to gamedir '%s'\n", gameDir);
			return 1;
		}
		printf("Working directory: %s\n", gameDir);
	}

	// Load game.ini early (before VFS) for game name, save dir, and IPA prefix
	{
		INIReader gameIni;
		if (gameIni.load("game.ini")) {
			GameConfig& gc = CAppContainer::getInstance()->gameConfig;
			gc.name = gameIni.getString("Game", "name", gc.name.c_str());
			gc.windowTitle = gameIni.getString("Game", "window_title", gc.name.c_str());
			gc.saveDir = gameIni.getString("Game", "save_dir", gc.saveDir.c_str());
			gc.ipaPrefix = gameIni.getString("Game", "ipa_prefix", gc.ipaPrefix.c_str());
			gc.entryMap = gameIni.getString("Game", "entry_map", gc.entryMap.c_str());

			// Parse no_fog_maps: comma-separated list of map IDs (e.g. "2,5")
			std::string noFogStr = gameIni.getString("Game", "no_fog_maps", "");
			if (!noFogStr.empty()) {
				size_t pos = 0;
				while (pos < noFogStr.size()) {
					size_t comma = noFogStr.find(',', pos);
					if (comma == std::string::npos) comma = noFogStr.size();
					std::string token = noFogStr.substr(pos, comma - pos);
					if (!token.empty()) {
						gc.noFogMaps.push_back(std::stoi(token));
					}
					pos = comma + 1;
				}
			}

			printf("Game: %s (save: %s)\n", gc.name.c_str(), gc.saveDir.c_str());
		}
	}

	const GameConfig& gc = CAppContainer::getInstance()->gameConfig;

	// Apply save directory from game config
	setSaveDir(gc.saveDir);

	ZipFile zipFile;
	zipFile.openZipFile(ipaPath);

	VFS vfs;
	// Mount CWD at highest priority (game directory or project root)
	vfs.mountDir(".", 200);
	// Mount basedata for shared/fallback assets
	vfs.mountDir("basedata", 100);
	// Mount the .ipa zip with the internal package prefix
	vfs.mountZip(&zipFile, gc.ipaPrefix.c_str(), 0);

	SDLGL sdlGL;
	sdlGL.Initialize();

	Input input;
	input.init(); // [GEC] Port: set default Binds

	CAppContainer::getInstance()->Construct(&sdlGL, &zipFile, &vfs);
	if (customMap) {
		CAppContainer::getInstance()->customMapFile = customMap;
		printf("Custom map: %s\n", customMap);
	}
	sdlGL.updateVideo(); // [GEC]

	SDL_Event ev;

	float x = 0.0f, y = 30.0f;
	int vp_cx = 480;
	int vp_cy = 320;

	Uint8 state;
	int mX, mY;                                 /* mouse location*/
	float mousePressX = 0.f, mousePressY = 0.f; /* mouse location float*/
	int winVidWidth = sdlGL.winVidWidth;
	int winVidHeight = sdlGL.winVidHeight;
	bool useMouse = false;

	while (CAppContainer::getInstance()->app->closeApplet != true) {

		int currentTimeMillis = CAppContainer::getInstance()->getTimeMS();

		// if (!useMouse) {
		// float ax = SDL_GameControllerGetAxis(sdlGL.accelerometer, SDL_CONTROLLER_AXIS_LEFTX);
		// float ay = SDL_GameControllerGetAxis(sdlGL.accelerometer, SDL_CONTROLLER_AXIS_LEFTY);
		// CAppContainer::getInstance()->UpdateAccelerometer((ax* (1.0f / 32767))/2, (ay* (1.0f / 32767))/2, 0.f,
		// false);
		//}

		if (currentTimeMillis > UpTime) {
			input.handleEvents();
			UpTime = currentTimeMillis + 15;
			drawView(&sdlGL);
			input.consumeEvents();
		}
	}

	printf("APP_QUIT\n");
	CAppContainer::getInstance()->~CAppContainer();
	zipFile.closeZipFile();
	sdlGL.~SDLGL();
	input.~Input();
	return 0;
}

static uint32_t lastTimems = 0;

void drawView(SDLGL* sdlGL) {

	int cx, cy;
	int w = sdlGL->vidWidth;
	int h = sdlGL->vidHeight;

	if (lastTimems == 0) {
		lastTimems = CAppContainer::getInstance()->getTimeMS();
	}

	SDL_GL_GetDrawableSize(sdlGL->window, &cx, &cy);
	if (w != cx || h != cy) {
		w = cx;
		h = cy;
	}

	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, -1.0, 1.0);
	// glRotatef(90.0, 0.0, 0.0, 1.0);
	// glTranslatef(0.0, -320.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	uint32_t startTime = CAppContainer::getInstance()->getTimeMS();
	uint32_t passedTime = startTime - lastTimems;
	lastTimems = startTime;

	if (passedTime >= 125) {
		passedTime = 125;
	}
	// printf("passedTime %d\n", passedTime);

	CAppContainer::getInstance()->DoLoop(passedTime);

	SDL_GL_SwapWindow(sdlGL->window); // Swap the window/pBmp to display the result.
}
