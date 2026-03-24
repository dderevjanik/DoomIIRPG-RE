#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <chrono>

#include <SDL.h>
#include "SDLGL.h"
#include "VFS.h"
#include <yaml-cpp/yaml.h>

#include "CAppContainer.h"
#include "App.h"
#include "IGameModule.h"
#include "DoomIIRPGGame.h"
#include "Canvas.h"
#include "Game.h"
#include "Input.h"
#include "JavaStream.h"

int main(int argc, char* args[]) {

	int maxTicks = 100;
	unsigned int seed = 42; // Default deterministic seed
	const char* gameDir = ".";
	const char* gameName = nullptr;
	const char* customMap = nullptr;

	for (int i = 1; i < argc; i++) {
		if (strcmp(args[i], "--gamedir") == 0 && i + 1 < argc) {
			gameDir = args[++i];
		} else if (strcmp(args[i], "--game") == 0 && i + 1 < argc) {
			gameName = args[++i];
		} else if (strcmp(args[i], "--map") == 0 && i + 1 < argc) {
			customMap = args[++i];
		} else if (strcmp(args[i], "--ticks") == 0 && i + 1 < argc) {
			maxTicks = atoi(args[++i]);
		} else if (strcmp(args[i], "--seed") == 0 && i + 1 < argc) {
			seed = (unsigned int)atoi(args[++i]);
		} else if (strcmp(args[i], "--minigame") == 0 && i + 1 < argc) {
			CAppContainer::getInstance()->minigameName = args[++i];
		} else if (strcmp(args[i], "--skip-travel-map") == 0) {
			CAppContainer::getInstance()->skipTravelMap = true;
		}
	}

	// Mark headless mode before anything else
	CAppContainer::getInstance()->headless = true;

	// Seed RNG for deterministic execution
	std::srand(seed);
	printf("[headless] Seed: %u\n", seed);

	// --game <name> is sugar for --gamedir games/<name>
	std::string gameDirStr;
	if (gameName) {
		gameDirStr = std::string("games/") + gameName;
		gameDir = gameDirStr.c_str();
		printf("[headless] Game: %s (directory: %s)\n", gameName, gameDir);
	} else if (strcmp(gameDir, ".") == 0) {
		if (access("games/doom2rpg/game.yaml", F_OK) == 0) {
			gameDirStr = "games/doom2rpg";
			gameDir = gameDirStr.c_str();
			printf("[headless] Auto-detected game directory: %s\n", gameDir);
		}
	}

	// Change working directory to gamedir
	if (strcmp(gameDir, ".") != 0) {
		if (chdir(gameDir) != 0) {
			printf("Error: cannot change to gamedir '%s'\n", gameDir);
			return 1;
		}
		printf("[headless] Working directory: %s\n", gameDir);
	}

	// Load game.yaml (same as Main.cpp)
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

				printf("[headless] Game: %s\n", gc.name.c_str());
			}
		} catch (const YAML::Exception& e) {
			printf("Error: could not load game.yaml: %s\n", e.what());
			return 1;
		}
	}

	const GameConfig& gc = CAppContainer::getInstance()->gameConfig;
	setSaveDir(gc.saveDir);

	VFS vfs;
	vfs.mountDir(".", 200);
	vfs.mountDir("basedata", 100);
	for (const auto& dir : gc.searchDirs) {
		vfs.addSearchDir(dir.c_str());
	}

	// Headless SDLGL — sets dimensions without creating window/GL context
	SDLGL sdlGL;
	sdlGL.InitializeHeadless();

	DoomIIRPGGame doom2rpgModule;
	CAppContainer::getInstance()->Construct(&sdlGL, &vfs, &doom2rpgModule);

	if (customMap) {
		CAppContainer::getInstance()->customMapFile = customMap;
		printf("[headless] Custom map: %s\n", customMap);
	}

	printf("[headless] Running %d ticks...\n", maxTicks);

	int ticksRun = 0;
	const int fixedTimestepMs = 15; // ~67 FPS, same as normal game loop

	while (!CAppContainer::getInstance()->app->closeApplet && ticksRun < maxTicks) {
		CAppContainer::getInstance()->headlessTimeMs += fixedTimestepMs;
		CAppContainer::getInstance()->DoLoop(fixedTimestepMs);
		ticksRun++;

		if (ticksRun % 100 == 0) {
			printf("[headless] tick %d/%d, state=%d\n",
				ticksRun, maxTicks, CAppContainer::getInstance()->app->canvas->state);
		}
	}

	printf("[headless] Completed %d ticks. Final state=%d\n",
		ticksRun, CAppContainer::getInstance()->app->canvas->state);

	CAppContainer::getInstance()->~CAppContainer();
	return 0;
}
