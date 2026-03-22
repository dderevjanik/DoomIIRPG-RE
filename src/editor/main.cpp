#include "Camera.h"
#include "EditorUI.h"
#include "MapData.h"
#include "MapDataIO.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "SDLGL.h"
#include "VFS.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Render.h"
#include "GLES.h"
#include "TinyGL.h"
#include "Game.h"
#include "Player.h"
#include "Resource.h"
#include "DoomIIRPGGame.h"
#include "IGameModule.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

static Camera camera;
static EditorUI ui;
static MapData mapData;
static int currentMapID = 0;
static bool noclip = false;
static std::string currentMapPath;

static void editorLoadMap(int mapID) {
	if (mapID < 1 || mapID > 10) {
		std::fprintf(stderr, "Invalid map ID: %d (must be 1-10)\n", mapID);
		return;
	}

	auto* app = CAppContainer::getInstance()->app;

	// Unload previous map media if needed
	if (currentMapID > 0) {
		app->canvas->unloadMedia();
	}

	// Reset some state before loading
	app->tinyGL->resetViewPort();

	if (!app->render->beginLoadMap(mapID)) {
		std::fprintf(stderr, "Failed to load map %d\n", mapID);
		return;
	}

	// Skip loadMapEntities() — it requires full gameplay state (loot tables, etc.)
	// The BSP geometry and textures are already loaded by beginLoadMap()
	currentMapID = mapID;

	// Load MapData from the same .bin file for editing
	{
		char mapFile[64];
		std::snprintf(mapFile, sizeof(mapFile), "levels/maps/map%02d.bin", mapID - 1);
		currentMapPath = mapFile;
		std::string err;
		MapData newData;
		if (MapDataIO::loadFromBin(mapFile, newData, err)) {
			mapData = std::move(newData);
			std::fprintf(stderr, "MapData loaded for editing\n");
		} else {
			std::fprintf(stderr, "Warning: could not load MapData: %s\n", err.c_str());
		}
	}

	// Place camera at player spawn point
	int spawnTileX = app->render->mapSpawnIndex % 32;
	int spawnTileY = app->render->mapSpawnIndex / 32;
	int spawnDir = app->render->mapSpawnDir;
	int spawnWorldX = spawnTileX * 64 + 32;
	int spawnWorldY = spawnTileY * 64 + 32;
	int tileHeight = app->render->getHeight(spawnWorldX, spawnWorldY);
	camera.posX = spawnWorldX / 128.0f;
	camera.posZ = spawnWorldY / 128.0f;
	camera.posY = (tileHeight + 36) / 128.0f; // tile height + eye height
	// spawnDir: 0=east, 1=north, 2=west, 3=south → engine angle = dir * 128
	// Convert to editor yaw (degrees)
	camera.yaw = (spawnDir * 128) * (360.0f / 1024.0f);
	camera.pitch = 0.0f;

	std::fprintf(stderr, "Loaded map %d (%d nodes, %d sprites)\n", mapID, app->render->numNodes,
	             app->render->numMapSprites);
}

static void editorLoadMapByID(int mapID) {
	editorLoadMap(mapID);
}

static void editorSaveMap() {
	if (currentMapID <= 0 || currentMapPath.empty()) return;

	std::string err;
	if (MapDataIO::saveToBin(mapData, currentMapPath, err)) {
		std::fprintf(stderr, "Saved map to %s\n", currentMapPath.c_str());
		mapData.dirty = false;

		// Reload map in engine to see changes
		editorLoadMap(currentMapID);
	} else {
		std::fprintf(stderr, "Error saving map: %s\n", err.c_str());
	}
}

int main(int argc, char* argv[]) {
	const char* gameDir = ".";
	const char* gameName = nullptr;
	int initialMap = 1;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--gamedir") == 0 && i + 1 < argc) {
			gameDir = argv[++i];
		} else if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
			gameName = argv[++i];
		} else if (strcmp(argv[i], "--map") == 0 && i + 1 < argc) {
			initialMap = std::atoi(argv[++i]);
		}
	}

	// --game <name> is sugar for --gamedir games/<name>
	std::string gameDirStr;
	if (gameName) {
		gameDirStr = std::string("games/") + gameName;
		gameDir = gameDirStr.c_str();
	} else if (strcmp(gameDir, ".") == 0) {
		if (access("games/doom2rpg/game.yaml", F_OK) == 0) {
			gameDirStr = "games/doom2rpg";
			gameDir = gameDirStr.c_str();
		}
	}

	// Change to game directory
	if (strcmp(gameDir, ".") != 0) {
		if (chdir(gameDir) != 0) {
			std::fprintf(stderr, "Error: cannot change to gamedir '%s'\n", gameDir);
			return 1;
		}
		std::fprintf(stderr, "Working directory: %s\n", gameDir);
	}

	// Load game.yaml
	{
		try {
			YAML::Node config = YAML::LoadFile("game.yaml");
			if (YAML::Node game = config["game"]) {
				GameConfig& gc = CAppContainer::getInstance()->gameConfig;
				if (game["name"])
					gc.name = game["name"].as<std::string>();
				if (game["window_title"])
					gc.windowTitle = "DoomIIRPG Map Editor";
				else
					gc.windowTitle = "DoomIIRPG Map Editor";
				if (game["save_dir"])
					gc.saveDir = game["save_dir"].as<std::string>();
				if (game["entry_map"])
					gc.entryMap = game["entry_map"].as<std::string>();
				if (game["no_fog_maps"]) {
					for (const auto& id : game["no_fog_maps"])
						gc.noFogMaps.push_back(id.as<int>());
				}
				if (game["search_dirs"]) {
					for (const auto& dir : game["search_dirs"])
						gc.searchDirs.push_back(dir.as<std::string>());
				}
			}
		} catch (const YAML::Exception& e) {
			std::fprintf(stderr, "Error: could not load game.yaml: %s\n", e.what());
			return 1;
		}
	}

	const GameConfig& gc = CAppContainer::getInstance()->gameConfig;

	// Set up VFS
	VFS vfs;
	vfs.mountDir(".", 200);
	vfs.mountDir("basedata", 100);
	for (const auto& dir : gc.searchDirs) {
		vfs.addSearchDir(dir.c_str());
	}

	// Initialize engine (creates SDL window + GL context)
	SDLGL sdlGL;
	sdlGL.Initialize();

	// Set up game module and construct engine
	DoomIIRPGGame doom2rpgModule;
	CAppContainer::getInstance()->Construct(&sdlGL, &vfs, &doom2rpgModule);

	// Make window resizable and resize for editor
	SDL_SetWindowResizable(sdlGL.window, SDL_TRUE);
	SDL_SetWindowSize(sdlGL.window, 1280, 800);
	SDL_SetWindowPosition(sdlGL.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_SetWindowTitle(sdlGL.window, "DoomIIRPG Map Editor");

	// Init ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();

	SDL_GLContext glContext = SDL_GL_GetCurrentContext();
	ImGui_ImplSDL2_InitForOpenGL(sdlGL.window, glContext);
	ImGui_ImplOpenGL3_Init("#version 120");

	// Init editor UI
	ui.init(nullptr);
	ui.setLoadCallback([](int mapID) { editorLoadMapByID(mapID); });
	ui.setSaveCallback([]() { editorSaveMap(); });
	ui.setMapData(&mapData);

	// Load initial map
	editorLoadMap(initialMap);

	// Main loop
	bool running = true;
	bool mouseCaptured = false;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

			switch (event.type) {
				case SDL_QUIT:
					running = false;
					break;

				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE && mouseCaptured) {
						SDL_SetRelativeMouseMode(SDL_FALSE);
						mouseCaptured = false;
					} else if (event.key.keysym.sym == SDLK_TAB) {
						ui.showAutomap = !ui.showAutomap;
					} else if (event.key.keysym.sym == SDLK_n) {
						noclip = !noclip;
					} else if (event.key.keysym.sym == SDLK_s && (event.key.keysym.mod & KMOD_CTRL)) {
						editorSaveMap();
					} else if (event.key.keysym.sym == SDLK_q && (event.key.keysym.mod & KMOD_CTRL)) {
						running = false;
					}
					break;

				case SDL_MOUSEBUTTONDOWN:
					if (!io.WantCaptureMouse && !mouseCaptured) {
						SDL_SetRelativeMouseMode(SDL_TRUE);
						mouseCaptured = true;
					}
					break;

				case SDL_MOUSEMOTION:
					if (mouseCaptured) {
						camera.handleMouseMotion(event.motion.xrel, event.motion.yrel);
					}
					break;

				case SDL_MOUSEWHEEL:
					if (!io.WantCaptureMouse) {
						camera.handleMouseWheel(static_cast<float>(event.wheel.y));
					}
					break;
			}
		}

		// WASD keyboard movement
		if (!io.WantCaptureKeyboard) {
			const uint8_t* keys = SDL_GetKeyboardState(nullptr);
			static uint32_t lastTicks = SDL_GetTicks();
			uint32_t now = SDL_GetTicks();
			float dt = (now - lastTicks) / 1000.0f;
			lastTicks = now;
			dt = std::min(dt, 0.05f);

			camera.handleKeyboard(keys[SDL_SCANCODE_W], keys[SDL_SCANCODE_S], keys[SDL_SCANCODE_A],
			                      keys[SDL_SCANCODE_D], keys[SDL_SCANCODE_E], keys[SDL_SCANCODE_Q],
			                      keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT], dt);
		}

		// ImGui new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// Draw editor UI
		ui.draw(currentMapID, camera, noclip);

		// Render 3D scene using game engine
		int drawW, drawH;
		SDL_GL_GetDrawableSize(sdlGL.window, &drawW, &drawH);

		if (currentMapID > 0) {
			auto* app = CAppContainer::getInstance()->app;

			// Set up GL state the same way drawView() does in Main.cpp
			int w = sdlGL.vidWidth;
			int h = sdlGL.vidHeight;
			int cx, cy;
			SDL_GL_GetDrawableSize(sdlGL.window, &cx, &cy);

			glViewport(0, 0, cx, cy);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_ALPHA_TEST);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0.0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, -1.0, 1.0);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// Convert editor camera to engine coordinates
			int viewX, viewY, viewZ, viewAngle, viewPitch, viewFov;
			camera.getEngineView(viewX, viewY, viewZ, viewAngle, viewPitch, viewFov);

			// Update engine time
			app->time = app->upTimeMs;
			app->canvas->staleView = true;

			// Enable textures in rendering
			app->game->scriptStateVars[5] = 1;

			// Push fog far out for maximum render distance
			app->tinyGL->fogMin = 32752;
			app->tinyGL->fogRange = 1;

			// Skip sprites — entities aren't loaded in editor mode
			app->render->skipSprites = true;

			// Disable BSP culling when noclip is on or camera is outside map bounds
			{
				constexpr int mapExtent = 32 * 64 * 16;
				bool outside = (viewX < 0 || viewY < 0 || viewX > mapExtent || viewY > mapExtent);
				app->render->skipCull = noclip || outside;
			}

			// Unbind VBO/EBO so engine's client-side arrays work correctly
			// (ImGui creates buffer objects that would cause glVertexPointer
			//  and glDrawElements to misinterpret pointers as offsets)
			auto glBindBufferFn = (void (*)(GLenum, GLuint))SDL_GL_GetProcAddress("glBindBuffer");
			glBindBufferFn(0x8892 /*GL_ARRAY_BUFFER*/, 0);
			glBindBufferFn(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, 0);

			// Render using game engine
			app->render->render(viewX, viewY, viewZ, viewAngle, viewPitch, 0, viewFov);
			app->render->_gles->ResetGLState();
		} else {
			glViewport(0, 0, drawW, drawH);
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		// ImGui render on top
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(sdlGL.window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	return 0;
}
