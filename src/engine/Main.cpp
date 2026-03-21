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
#include <yaml-cpp/yaml.h>

#include "CAppContainer.h"
#include "App.h"
#include "IGameModule.h"
#include "DoomIIRPGGame.h"
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
	// Default to doom2rpg if no game/gamedir specified
	std::string gameDirStr;
	if (gameName) {
		gameDirStr = std::string("games/") + gameName;
		gameDir = gameDirStr.c_str();
		printf("Game: %s (directory: %s)\n", gameName, gameDir);
	} else if (strcmp(gameDir, ".") == 0) {
		// Auto-detect: if games/doom2rpg exists, use it
		if (access("games/doom2rpg/game.yaml", F_OK) == 0) {
			gameDirStr = "games/doom2rpg";
			gameDir = gameDirStr.c_str();
			printf("Auto-detected game directory: %s\n", gameDir);
		}
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

	// Load game.yaml early (before VFS) for game name, save dir, etc.
	{
		try {
			YAML::Node config = YAML::LoadFile("game.yaml");
			if (YAML::Node game = config["game"]) {
				GameConfig& gc = CAppContainer::getInstance()->gameConfig;
				if (game["name"]) gc.name = game["name"].as<std::string>();
				if (game["window_title"]) gc.windowTitle = game["window_title"].as<std::string>();
				else gc.windowTitle = gc.name;
				if (game["save_dir"]) gc.saveDir = game["save_dir"].as<std::string>();
				if (game["entry_map"]) gc.entryMap = game["entry_map"].as<std::string>();

				if (game["no_fog_maps"]) {
					for (const auto& id : game["no_fog_maps"]) {
						gc.noFogMaps.push_back(id.as<int>());
					}
				}

				printf("Game: %s (save: %s)\n", gc.name.c_str(), gc.saveDir.c_str());
			}
		} catch (const YAML::Exception& e) {
			printf("Warning: could not load game.yaml: %s\n", e.what());
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

	// Set up the game module — custom games would provide their own IGameModule here
	DoomIIRPGGame doom2rpgModule;

	CAppContainer::getInstance()->Construct(&sdlGL, &zipFile, &vfs, &doom2rpgModule);

	input.init(); // [GEC] Port: set default Binds — must be after Construct() so app pointer exists

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
