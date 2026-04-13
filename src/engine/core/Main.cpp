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
#include "DataNode.h"

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

#include "DevConsole.h"

#include <dirent.h>
#include <signal.h>

static DevConsole g_devConsole;

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
	std::vector<std::string> modDirs;  // --mod directories (in order specified)

	for (int i = 1; i < argc; i++) {
		if (strcmp(args[i], "--help") == 0 || strcmp(args[i], "-h") == 0) {
			printf("DRPGEngine %s\n", PROJECT_VERSION);
			printf("Usage: %s [options]\n\n", args[0]);
			printf("Options:\n");
			printf("  -h, --help              Show this help message and exit\n");
			printf("  --game <name>           Select game from games/ directory\n");
			printf("  --gamedir <path>        Set game directory (default: auto-detect)\n");
			printf("  --map <path>            Load a level directory or map file\n");
			printf("  --minigame <name>       Launch a specific minigame\n");
			printf("  --skip-intro            Skip the intro sequence\n");
			printf("  --skip-travel-map       Skip the travel map screen\n");
			printf("  --headless              Run without a window (deterministic loop)\n");
			printf("  --ticks <n>             Quit after n ticks (headless/script mode)\n");
			printf("  --seed <n>              Set random seed for deterministic runs\n");
			printf("  --script <file>         Load and replay an input script\n");
			printf("  --mod <dir>             Load a mod directory (repeatable, last wins)\n");
			return 0;
		} else if (strcmp(args[i], "--gamedir") == 0 && i + 1 < argc) {
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
		} else if (strcmp(args[i], "--mod") == 0 && i + 1 < argc) {
			modDirs.push_back(args[++i]);
		}
	}

	if (headless) {
		CAppContainer::getInstance()->headless = true;
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
		// Auto-detect: scan games/ for directories containing game.yaml
		std::vector<std::string> detectedGames;
		DIR* gamesDir = opendir("games");
		if (gamesDir) {
			struct dirent* entry;
			while ((entry = readdir(gamesDir)) != nullptr) {
				if (entry->d_name[0] == '.') continue;
				std::string yamlPath = std::string("games/") + entry->d_name + "/game.yaml";
				if (access(yamlPath.c_str(), F_OK) == 0) {
					detectedGames.push_back(entry->d_name);
				}
			}
			closedir(gamesDir);
		}

		if (detectedGames.size() == 1) {
			gameDirStr = "games/" + detectedGames[0];
			gameDir = gameDirStr.c_str();
			printf("[main] Auto-detected game: %s\n", detectedGames[0].c_str());
		} else if (detectedGames.size() > 1) {
			printf("[main] Multiple games found. Use --game <name> to select one:\n");
			for (const auto& g : detectedGames) {
				printf("  --game %s\n", g.c_str());
			}
			return 1;
		} else if (access("Doom 2 RPG.ipa", F_OK) == 0) {
			// IPA found but not yet converted — run converter automatically
			printf("[main] Found 'Doom 2 RPG.ipa' but no converted assets.\n");
			printf("[main] Running drpg-convert to extract game assets...\n");

			std::string converterCmd;
			std::string selfDir;
			{
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
				std::string candidates[] = {
					selfDir + "drpg-convert",
					selfDir + "converter/drpg-convert",
				};
				for (const auto& c : candidates) {
					if (access(c.c_str(), X_OK) == 0) {
						converterCmd = "\"" + c + "\"";
						break;
					}
				}
			}
			if (converterCmd.empty()) {
				converterCmd = "drpg-convert";
			}

			std::string cmd = converterCmd + " --ipa \"Doom 2 RPG.ipa\" --output games/doom2rpg";
			printf("[main]   %s\n", cmd.c_str());
			int ret = system(cmd.c_str());
			if (ret != 0) {
				printf("[main] Error: converter failed (exit code %d).\n", ret);
				printf("[main] You can also run it manually:\n  %s\n", cmd.c_str());
				return 1;
			}

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

	// Resolve script and mod paths to absolute before chdir
	std::string scriptPathStr;
	if (scriptFile) {
		char resolved[PATH_MAX];
		if (realpath(scriptFile, resolved)) {
			scriptPathStr = resolved;
			scriptFile = scriptPathStr.c_str();
		}
	}
	for (auto& modDir : modDirs) {
		char resolved[PATH_MAX];
		if (realpath(modDir.c_str(), resolved)) {
			modDir = resolved;
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
		DataNode config = DataNode::loadFile("game.yaml");
		if (!config) {
			printf("[main] Error: could not load game.yaml in '%s'\n", gameDir);
			printf("[main] Use --game <name> to select a game from games/ directory.\n");
			return 1;
		}
		DataNode game = config["game"];
		if (game) {
			GameConfig& gc = CAppContainer::getInstance()->gameConfig;
			if (game["module"]) gc.module = game["module"].asString();
			if (game["name"]) gc.name = game["name"].asString();
			if (game["window_title"]) gc.windowTitle = game["window_title"].asString();
			else gc.windowTitle = gc.name;
			if (game["save_dir"]) gc.saveDir = game["save_dir"].asString();
			if (game["entry_map"]) gc.entryMap = game["entry_map"].asString();

			DataNode searchDirs = game["search_dirs"];
			if (searchDirs) {
				for (auto it = searchDirs.begin(); it != searchDirs.end(); ++it) {
					gc.searchDirs.push_back(it.value().asString());
				}
			}

			DataNode entitiesNode = game["entities"];
			if (entitiesNode) {
				gc.entityFiles.clear();
				for (auto it = entitiesNode.begin(); it != entitiesNode.end(); ++it) {
					gc.entityFiles.push_back(it.value().asString());
				}
			}

			DataNode stringsNode = game["strings"];
			if (stringsNode && stringsNode.isMap()) {
				for (auto it = stringsNode.begin(); it != stringsNode.end(); ++it) {
					int groupId = std::atoi(it.key().asString().c_str());
					std::string filePath = it.value().asString("");
					if (!filePath.empty()) {
						gc.stringFiles[groupId] = filePath;
					}
				}
			}

			// Parse per-level directory mappings
			DataNode levelsNode = game["levels"];
			if (levelsNode && levelsNode.isMap()) {
				for (auto it = levelsNode.begin(); it != levelsNode.end(); ++it) {
					int mapId = std::atoi(it.key().asString().c_str());
					std::string dir = it.value().asString("");
					if (!dir.empty() && mapId > 0) {
						LevelInfo info;
						info.dir = dir;
						info.mapFile = dir + "/map.bin";
						info.modelFile = dir + "/model.bin";
						info.configFile = dir + "/level.yaml";
						gc.levelInfos[mapId] = std::move(info);
					}
				}
			}

			gc.maxEntities = game["max_entities"].asInt(gc.maxEntities);
			gc.maxWeaponButtons = game["max_weapon_buttons"].asInt(gc.maxWeaponButtons);

			// Player section
			DataNode player = game["player"];
			if (player) {
				gc.startingMaxHealth = player["starting_max_health"].asInt(gc.startingMaxHealth);
				gc.outOfCombatTurns = player["out_of_combat_turns"].asInt(gc.outOfCombatTurns);

				DataNode lu = player["level_up"];
				if (lu) {
					gc.levelUpHealth = lu["health"].asInt(gc.levelUpHealth);
					gc.levelUpDefense = lu["defense"].asInt(gc.levelUpDefense);
					gc.levelUpStrength = lu["strength"].asInt(gc.levelUpStrength);
					gc.levelUpAccuracy = lu["accuracy"].asInt(gc.levelUpAccuracy);
					gc.levelUpAgility = lu["agility"].asInt(gc.levelUpAgility);
				}

				DataNode xp = player["xp_formula"];
				if (xp) {
					gc.xpLinear = xp["linear"].asInt(gc.xpLinear);
					gc.xpCubic = xp["cubic"].asInt(gc.xpCubic);
				}

				DataNode caps = player["caps"];
				if (caps) {
					gc.capCredits = caps["credits"].asInt(gc.capCredits);
					gc.capInventory = caps["inventory"].asInt(gc.capInventory);
					gc.capAmmo = caps["ammo"].asInt(gc.capAmmo);
					gc.capBotFuel = caps["bot_fuel"].asInt(gc.capBotFuel);
				}
			}

			DataNode sc = game["scoring"];
			if (sc) {
				gc.scorePerLevel = sc["per_level"].asInt(gc.scorePerLevel);
				gc.scoreAllLevelsBonus = sc["all_levels_bonus"].asInt(gc.scoreAllLevelsBonus);
				gc.scoreNoDeathsBonus = sc["no_deaths_bonus"].asInt(gc.scoreNoDeathsBonus);
				gc.scoreDeathPenaltyBase = sc["death_penalty_base"].asInt(gc.scoreDeathPenaltyBase);
				gc.scoreDeathPenaltyMult = sc["death_penalty_mult"].asInt(gc.scoreDeathPenaltyMult);
				gc.scoreManyDeathsPenalty = sc["many_deaths_penalty"].asInt(gc.scoreManyDeathsPenalty);
				gc.scoreDeathThreshold = sc["death_threshold"].asInt(gc.scoreDeathThreshold);
				gc.scoreTimeBonusMinutes = sc["time_bonus_minutes"].asInt(gc.scoreTimeBonusMinutes);
				gc.scoreTimeBonusMult = sc["time_bonus_mult"].asInt(gc.scoreTimeBonusMult);
				gc.scoreMoveThreshold = sc["move_threshold"].asInt(gc.scoreMoveThreshold);
				gc.scoreMoveDivisor = sc["move_divisor"].asInt(gc.scoreMoveDivisor);
				gc.scorePerSecret = sc["per_secret"].asInt(gc.scorePerSecret);
				gc.scoreAllSecretsBonus = sc["all_secrets_bonus"].asInt(gc.scoreAllSecretsBonus);
			}

			DataNode dvd = game["damage_vignette_dirs"];
			if (dvd) {
				gc.damageVignetteDirs.clear();
				for (auto it = dvd.begin(); it != dvd.end(); ++it) {
					gc.damageVignetteDirs.push_back(it.value().asInt(0));
				}
			}

			DataNode bc = game["bubble_colors"];
			if (bc) {
				gc.bubbleColors.clear();
				for (auto it = bc.begin(); it != bc.end(); ++it) {
					DataNode entry = it.value();
					unsigned int color = static_cast<unsigned int>(entry["color"].asLongLong(0));
					gc.bubbleColors.push_back({
						color,
						entry["offset"].asInt(0),
						entry["tail_width"].asInt(0)
					});
				}
			}

			DataNode tp = game["target_practice"];
			if (tp) {
				gc.tpHeadPoints = tp["head_points"].asInt(gc.tpHeadPoints);
				gc.tpBodyPoints = tp["body_points"].asInt(gc.tpBodyPoints);
				gc.tpLegPoints = tp["leg_points"].asInt(gc.tpLegPoints);
				gc.tpHitDisplayMs = tp["hit_display_ms"].asInt(gc.tpHitDisplayMs);
				gc.tpWeaponIdx = tp["weapon_index"].asInt(gc.tpWeaponIdx);
				gc.tpAmmoType = tp["ammo_type"].asInt(gc.tpAmmoType);
				gc.tpAmmoCount = tp["ammo_count"].asInt(gc.tpAmmoCount);
			}

			DataNode vm = game["vending_machine"];
			if (vm) {
				gc.vendSliderMin = vm["slider_min"].asInt(gc.vendSliderMin);
				gc.vendSliderMax = vm["slider_max"].asInt(gc.vendSliderMax);
				gc.vendSliderStart = vm["slider_start"].asInt(gc.vendSliderStart);
				DataNode hints = vm["iq_hints"];
				if (hints) {
					gc.vendIQHints.clear();
					for (auto it = hints.begin(); it != hints.end(); ++it) {
						DataNode h = it.value();
						gc.vendIQHints.push_back({
							h["iq"].asInt(0),
							h["hints"].asInt(0)
						});
					}
				}
			}

			DataNode am = game["automap"];
			if (am) {
				DataNode dc = am["door_colors"];
				if (dc) {
					gc.automapDoorColors.clear();
					for (auto it = dc.begin(); it != dc.end(); ++it) {
						DataNode entry = it.value();
						int16_t tile = (int16_t)entry["tile"].asInt(0);
						uint32_t color = (uint32_t)entry["color"].asULong(0);
						gc.automapDoorColors.push_back({tile, color});
					}
				}
				gc.automapDoorDefault = (uint32_t)am["door_default_color"].asULong(gc.automapDoorDefault);
				DataNode hd = am["hidden_decors"];
				if (hd) {
					gc.automapHiddenDecors.clear();
					for (auto it = hd.begin(); it != hd.end(); ++it) {
						gc.automapHiddenDecors.push_back((int16_t)it.value().asInt(0));
					}
				}
			}

			printf("[main] Game: %s (save: %s)\n", gc.name.c_str(), gc.saveDir.c_str());
		}

		// Load per-level data from level.yaml files in each level directory
		{
			GameConfig& gc2 = CAppContainer::getInstance()->gameConfig;
			for (const auto& [mapId, info] : gc2.levelInfos) {
				DataNode levelNode = DataNode::loadFile(info.configFile.c_str());
				if (!levelNode) continue;

				// fog: false disables fog for this map (defaults to true)
				DataNode fogNode = levelNode["fog"];
				if (fogNode && !fogNode.asBool(true)) {
					gc2.noFogMaps.push_back(mapId);
				}

				DataNode jiNode = levelNode["joke_items"];
				if (jiNode && jiNode.isSequence()) {
					std::vector<int> items;
					for (auto it2 = jiNode.begin(); it2 != jiNode.end(); ++it2) {
						items.push_back(it2.value().asInt(0));
					}
					gc2.jokeItems[mapId] = std::move(items);
				}

				DataNode skyNode = levelNode["sky_box"];
				if (skyNode) {
					gc2.levelInfos[mapId].skyBox = skyNode.asString("");
				}
			}
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

	// Mount mod directories (later entries = higher priority, overriding earlier ones)
	for (size_t i = 0; i < modDirs.size(); i++) {
		int modPriority = 300 + static_cast<int>(i);
		vfs.mountDir(modDirs[i].c_str(), modPriority);

		// Try to read mod.yaml for metadata
		std::string modYamlPath = modDirs[i] + "/mod.yaml";
		DataNode modConfig = DataNode::loadFile(modYamlPath.c_str());
		if (modConfig) {
			DataNode mod = modConfig["mod"];
			if (mod) {
				std::string modName = mod["name"].asString("(unnamed)");
				std::string modVersion = mod["version"].asString("");
				std::string modAuthor = mod["author"].asString("");
				printf("[mod] Loaded: %s%s%s (priority %d)\n",
					modName.c_str(),
					modVersion.empty() ? "" : (" v" + modVersion).c_str(),
					modAuthor.empty() ? "" : (" by " + modAuthor).c_str(),
					modPriority);

				// Register additional search dirs from the mod
				DataNode modSearchDirs = mod["search_dirs"];
				if (modSearchDirs) {
					for (auto it = modSearchDirs.begin(); it != modSearchDirs.end(); ++it) {
						vfs.addSearchDir(it.value().asString().c_str());
					}
				}
			}
		} else {
			printf("[mod] Mounted: %s (priority %d, no mod.yaml)\n",
				modDirs[i].c_str(), modPriority);
		}
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

	// Initialize ImGui developer console (F12 to toggle)
	if (!headless && sdlGL.window) {
		g_devConsole.init(sdlGL.window, SDL_GL_GetCurrentContext());
		CAppContainer::getInstance()->devConsole = &g_devConsole;
	}

	input.init(); // [GEC] Port: set default Binds — must be after Construct() so app pointer exists

	if (customMap) {
		// Try to resolve --map argument against registered levels in game.yaml
		const auto& gc = CAppContainer::getInstance()->gameConfig;
		std::string mapArg(customMap);
		int resolvedID = 0;

		// Match against levelInfos directories and map files
		for (const auto& [id, info] : gc.levelInfos) {
			if (mapArg == info.dir || mapArg == info.mapFile ||
			    mapArg == info.dir + "/" || mapArg == info.dir + "/map.bin") {
				resolvedID = id;
				break;
			}
		}

		if (resolvedID > 0) {
			CAppContainer::getInstance()->customMapID = resolvedID;
			CAppContainer::getInstance()->skipTravelMap = true;
			printf("[main] Custom map: level %d (%s)\n", resolvedID,
			       gc.levelInfos.at(resolvedID).dir.c_str());
		} else {
			// Fall back to raw file path for standalone .bin files
			CAppContainer::getInstance()->customMapFile = customMap;
			CAppContainer::getInstance()->skipTravelMap = true;
			printf("[main] Custom map file: %s\n", customMap);
		}
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
		int vp_cx = Applet::IOS_WIDTH;
		int vp_cy = Applet::IOS_HEIGHT;

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
	g_devConsole.shutdown();
	CAppContainer::getInstance()->devConsole = nullptr;
	CAppContainer::getInstance()->~CAppContainer();
	_exit(0);
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

	// Render ImGui overlay on top of the game frame
	g_devConsole.newFrame();
	g_devConsole.render();

	SDL_GL_SwapWindow(sdlGL->window); // Swap the window/pBmp to display the result.
}
