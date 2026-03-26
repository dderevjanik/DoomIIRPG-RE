#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <climits>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <SDL.h>
#include "SDLGL.h"
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
#include "GameScript.h"
#include "CrashHandler.h"

#include <signal.h>

static void handleSIGTERM(int) {
	// Gracefully request shutdown instead of letting the signal kill the process
	if (CAppContainer::getInstance() && CAppContainer::getInstance()->app) {
		CAppContainer::getInstance()->app->shutdown();
	} else {
		_exit(0);
	}
}

void drawView(SDLGL* sdlGL);

int main(int argc, char* args[]) {
	CrashHandler_Init();
	signal(SIGTERM, handleSIGTERM);

	int UpTime = 0;

	const char* gameDir = ".";
	const char* gameName = nullptr;
	const char* customMap = nullptr;
	bool headless = false;
	int maxTicks = 0;           // 0 = unlimited (normal mode)
	unsigned int seed = 0;      // 0 = don't seed (normal mode)
	bool hasSeed = false;
	const char* scriptFile = nullptr;

	for (int i = 1; i < argc; i++) {
		if (strcmp(args[i], "--gamedir") == 0 && i + 1 < argc) {
			gameDir = args[++i];
		} else if (strcmp(args[i], "--game") == 0 && i + 1 < argc) {
			gameName = args[++i];
		} else if (strcmp(args[i], "--map") == 0 && i + 1 < argc) {
			customMap = args[++i];
		} else if (strcmp(args[i], "--minigame") == 0 && i + 1 < argc) {
			CAppContainer::getInstance()->minigameName = args[++i];
		} else if (strcmp(args[i], "--skip-travel-map") == 0) {
			CAppContainer::getInstance()->skipTravelMap = true;
		} else if (strcmp(args[i], "--skip-intro") == 0) {
			CAppContainer::getInstance()->skipIntro = true;
		} else if (strcmp(args[i], "--headless") == 0) {
			headless = true;
		} else if (strcmp(args[i], "--ticks") == 0 && i + 1 < argc) {
			maxTicks = atoi(args[++i]);
		} else if (strcmp(args[i], "--seed") == 0 && i + 1 < argc) {
			seed = (unsigned int)atoi(args[++i]);
			hasSeed = true;
		} else if (strcmp(args[i], "--script") == 0 && i + 1 < argc) {
			scriptFile = args[++i];
		}
	}

	if (headless) {
		CAppContainer::getInstance()->headless = true;
	}
	if (customMap) {
		CAppContainer::getInstance()->skipTravelMap = true;
	}
	if (hasSeed) {
		std::srand(seed);
		printf("[main] Seed: %u\n", seed);
	}

	// --game <name> is sugar for --gamedir games/<name>
	// Default to doom2rpg if no game/gamedir specified
	std::string gameDirStr;
	if (gameName) {
		gameDirStr = std::string("games/") + gameName;
		gameDir = gameDirStr.c_str();
		printf("[main] Game: %s (directory: %s)\n", gameName, gameDir);
	} else if (strcmp(gameDir, ".") == 0) {
		// Auto-detect: if games/doom2rpg exists, use it
		if (access("games/doom2rpg/game.yaml", F_OK) == 0) {
			gameDirStr = "games/doom2rpg";
			gameDir = gameDirStr.c_str();
			printf("[main] Auto-detected game directory: %s\n", gameDir);
		} else if (access("Doom 2 RPG.ipa", F_OK) == 0) {
			// IPA found but not yet converted — run converter automatically
			printf("[main] Found 'Doom 2 RPG.ipa' but no converted assets.\n");
			printf("[main] Running doom2rpg-convert to extract game assets...\n");

			// Find converter next to engine binary, or fall back to PATH
			std::string converterCmd;
			std::string selfDir;
			{
				// Try to find converter next to this executable
				char selfPath[4096] = {};
#ifdef __APPLE__
				uint32_t selfPathSize = sizeof(selfPath);
				if (_NSGetExecutablePath(selfPath, &selfPathSize) == 0) {
					char* slash = strrchr(selfPath, '/');
					if (slash) {
						selfDir = std::string(selfPath, slash - selfPath + 1);
					}
				}
#elif defined(__linux__)
				ssize_t len = readlink("/proc/self/exe", selfPath, sizeof(selfPath) - 1);
				if (len > 0) {
					selfPath[len] = '\0';
					char* slash = strrchr(selfPath, '/');
					if (slash) {
						selfDir = std::string(selfPath, slash - selfPath + 1);
					}
				}
#endif
			}

			if (!selfDir.empty()) {
				// Check same directory, then converter/ sibling
				std::string candidates[] = {
					selfDir + "doom2rpg-convert",
					selfDir + "converter/doom2rpg-convert",
				};
				for (const auto& c : candidates) {
					if (access(c.c_str(), X_OK) == 0) {
						converterCmd = "\"" + c + "\"";
						break;
					}
				}
			}
			if (converterCmd.empty()) {
				converterCmd = "doom2rpg-convert";
			}

			std::string cmd = converterCmd + " --ipa \"Doom 2 RPG.ipa\" --output games/doom2rpg";
			printf("[main]   %s\n", cmd.c_str());
			int ret = system(cmd.c_str());
			if (ret != 0) {
				printf("[main] Error: converter failed (exit code %d).\n", ret);
				printf("[main] You can also run it manually:\n  %s\n", cmd.c_str());
				return 1;
			}

			// Re-check after conversion
			if (access("games/doom2rpg/game.yaml", F_OK) == 0) {
				gameDirStr = "games/doom2rpg";
				gameDir = gameDirStr.c_str();
				printf("[main] Auto-conversion complete. Game directory: %s\n", gameDir);
			} else {
				printf("[main] Error: conversion ran but game.yaml not found.\n");
				return 1;
			}
		}
	}

	// Resolve script path to absolute before chdir
	std::string scriptPathStr;
	if (scriptFile) {
		char resolved[PATH_MAX];
		if (realpath(scriptFile, resolved)) {
			scriptPathStr = resolved;
			scriptFile = scriptPathStr.c_str();
		}
	}

	// Change working directory to gamedir so config files are found there
	if (strcmp(gameDir, ".") != 0) {
		if (chdir(gameDir) != 0) {
			printf("[main] Error: cannot change to gamedir '%s'\n", gameDir);
			return 1;
		}
		printf("[main] Working directory: %s\n", gameDir);
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

				if (game["search_dirs"]) {
					for (const auto& dir : game["search_dirs"]) {
						gc.searchDirs.push_back(dir.as<std::string>());
					}
				}

				gc.maxEntities = game["max_entities"].as<int>(gc.maxEntities);
				gc.maxWeaponButtons = game["max_weapon_buttons"].as<int>(gc.maxWeaponButtons);

				if (YAML::Node xp = game["xp_formula"]) {
					gc.xpLinear = xp["linear"].as<int>(gc.xpLinear);
					gc.xpCubic = xp["cubic"].as<int>(gc.xpCubic);
				}

				if (YAML::Node caps = game["caps"]) {
					gc.capCredits = caps["credits"].as<int>(gc.capCredits);
					gc.capInventory = caps["inventory"].as<int>(gc.capInventory);
					gc.capAmmo = caps["ammo"].as<int>(gc.capAmmo);
					gc.capBotFuel = caps["bot_fuel"].as<int>(gc.capBotFuel);
				}

				if (YAML::Node dvd = game["damage_vignette_dirs"]) {
					gc.damageVignetteDirs.clear();
					for (const auto& val : dvd) {
						gc.damageVignetteDirs.push_back(val.as<int>());
					}
				}

				if (YAML::Node bc = game["bubble_colors"]) {
					gc.bubbleColors.clear();
					for (const auto& entry : bc) {
						unsigned int color = static_cast<unsigned int>(entry["color"].as<long long>());
						gc.bubbleColors.push_back({
							color,
							entry["offset"].as<int>(),
							entry["tail_width"].as<int>()
						});
					}
				}

				if (YAML::Node tp = game["target_practice"]) {
					gc.tpHeadPoints = tp["head_points"].as<int>(gc.tpHeadPoints);
					gc.tpBodyPoints = tp["body_points"].as<int>(gc.tpBodyPoints);
					gc.tpLegPoints = tp["leg_points"].as<int>(gc.tpLegPoints);
					gc.tpHitDisplayMs = tp["hit_display_ms"].as<int>(gc.tpHitDisplayMs);
					gc.tpWeaponIdx = tp["weapon_index"].as<int>(gc.tpWeaponIdx);
					gc.tpAmmoType = tp["ammo_type"].as<int>(gc.tpAmmoType);
					gc.tpAmmoCount = tp["ammo_count"].as<int>(gc.tpAmmoCount);
				}

				if (YAML::Node vm = game["vending_machine"]) {
					gc.vendSliderMin = vm["slider_min"].as<int>(gc.vendSliderMin);
					gc.vendSliderMax = vm["slider_max"].as<int>(gc.vendSliderMax);
					gc.vendSliderStart = vm["slider_start"].as<int>(gc.vendSliderStart);
					if (YAML::Node hints = vm["iq_hints"]) {
						gc.vendIQHints.clear();
						for (const auto& h : hints) {
							gc.vendIQHints.push_back({
								h["iq"].as<int>(),
								h["hints"].as<int>()
							});
						}
					}
				}

				if (YAML::Node am = game["automap"]) {
					if (YAML::Node dc = am["door_colors"]) {
						gc.automapDoorColors.clear();
						for (const auto& entry : dc) {
							int16_t tile = (int16_t)entry["tile"].as<int>();
							uint32_t color = (uint32_t)entry["color"].as<unsigned long>();
							gc.automapDoorColors.push_back({tile, color});
						}
					}
					gc.automapDoorDefault = (uint32_t)am["door_default_color"].as<unsigned long>(gc.automapDoorDefault);
					if (YAML::Node hd = am["hidden_decors"]) {
						gc.automapHiddenDecors.clear();
						for (const auto& entry : hd) {
							gc.automapHiddenDecors.push_back((int16_t)entry.as<int>());
						}
					}
				}

				if (YAML::Node ji = game["joke_items"]) {
					for (const auto& entry : ji) {
						int mapId = entry["map"].as<int>();
						std::vector<int> items;
						for (const auto& item : entry["items"]) {
							items.push_back(item.as<int>());
						}
						gc.jokeItems[mapId] = std::move(items);
					}
				}

				printf("[main] Game: %s (save: %s)\n", gc.name.c_str(), gc.saveDir.c_str());
			}
		} catch (const YAML::Exception& e) {
			printf("[main] Error: could not load game.yaml: %s\n", e.what());
			printf("[main] Run doom2rpg-convert to extract game assets first.\n");
			return 1;
		}
	}

	const GameConfig& gc = CAppContainer::getInstance()->gameConfig;

	// Apply save directory from game config
	setSaveDir(gc.saveDir);

	VFS vfs;
	// Mount CWD at highest priority (game directory or project root)
	vfs.mountDir(".", 200);
	// Mount basedata for shared/fallback assets
	vfs.mountDir("basedata", 100);

	// Register search subdirectories from game.yaml (e.g. ui/, hud/, fonts/, audio/)
	for (const auto& dir : gc.searchDirs) {
		vfs.addSearchDir(dir.c_str());
	}

	SDLGL sdlGL;
	if (headless) {
		sdlGL.InitializeHeadless();
	} else {
		sdlGL.Initialize();
	}

	Input input;

	// Set up the game module — custom games would provide their own IGameModule here
	DoomIIRPGGame doom2rpgModule;

	CAppContainer::getInstance()->Construct(&sdlGL, &vfs, &doom2rpgModule);

	input.init(); // [GEC] Port: set default Binds — must be after Construct() so app pointer exists

	if (customMap) {
		CAppContainer::getInstance()->customMapFile = customMap;
		printf("[main] Custom map: %s\n", customMap);
	}

	// Load script if specified (works in both headless and windowed modes)
	GameScript script;
	bool hasScript = false;
	if (scriptFile) {
		hasScript = script.loadFromFile(scriptFile);
		if (!hasScript) {
			printf("[main] Error: failed to load script '%s'\n", scriptFile);
			return 1;
		}
		// Auto-set ticks from script if not explicitly given
		if (maxTicks == 0) {
			maxTicks = script.lastTick() + 100; // run 100 ticks past last command
		}
	}

	if (headless) {
		// Headless game loop — deterministic fixed timestep, no window
		const int fixedTimestepMs = 15;
		int ticksRun = 0;

		printf("[headless] Running%s%s...\n",
			maxTicks > 0 ? (" " + std::to_string(maxTicks) + " ticks").c_str() : "",
			hasScript ? " with script" : "");

		while (!CAppContainer::getInstance()->app->closeApplet && (maxTicks == 0 || ticksRun < maxTicks)) {
			CAppContainer::getInstance()->headlessTimeMs += fixedTimestepMs;

			if (hasScript) {
				script.injectForTick(ticksRun);
			}

			CAppContainer::getInstance()->DoLoop(fixedTimestepMs);
			ticksRun++;

			if (ticksRun % 100 == 0) {
				printf("[headless] tick %d/%d, state=%d\n",
					ticksRun, maxTicks, CAppContainer::getInstance()->app->canvas->state);
			}
		}

		printf("[headless] Completed %d ticks. Final state=%d\n",
			ticksRun, CAppContainer::getInstance()->app->canvas->state);
	} else {
		// Normal windowed game loop
		sdlGL.updateVideo(); // [GEC]

		float x = 0.0f, y = 30.0f;
		int vp_cx = 480;
		int vp_cy = 320;

		Uint8 state;
		int mX, mY;                                 /* mouse location*/
		float mousePressX = 0.f, mousePressY = 0.f; /* mouse location float*/
		int winVidWidth = sdlGL.winVidWidth;
		int winVidHeight = sdlGL.winVidHeight;
		bool useMouse = false;
		int ticksRun = 0;

		while (CAppContainer::getInstance()->app->closeApplet != true) {

			int currentTimeMillis = CAppContainer::getInstance()->getTimeMS();

			if (currentTimeMillis > UpTime) {
				input.handleEvents();

				if (hasScript) {
					script.injectForTick(ticksRun);
				}

				UpTime = currentTimeMillis + 15;
				drawView(&sdlGL);
				input.consumeEvents();
				ticksRun++;

				// Auto-quit when script + ticks are done
				if (maxTicks > 0 && ticksRun >= maxTicks) {
					printf("[script] Completed %d ticks. Exiting.\n", ticksRun);
					break;
				}
			}
		}
	}

	printf("[main] APP_QUIT\n");
	CAppContainer::getInstance()->~CAppContainer();
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
