#include "EditorState.h"
#include "MapCompiler.h"
#include "GameYamlRegistrar.h"

#include "Canvas.h"
#include "CAppContainer.h"
#include "App.h"
#include "Log.h"
#include "Render.h"
#include "SDLGL.h"
#include "Game.h"

#include "imgui.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <yaml-cpp/yaml.h>
#include "stb_image.h"
#include "VFS.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

void writeTextFile(const fs::path& p, const std::string& content) {
	fs::create_directories(p.parent_path());
	std::ofstream f(p);
	f << content;
}

void writeBinFile(const fs::path& p, const std::vector<uint8_t>& data) {
	fs::create_directories(p.parent_path());
	std::ofstream f(p, std::ios::binary);
	f.write(reinterpret_cast<const char*>(data.data()), data.size());
}

}  // namespace

// =====================================================================
// Lifecycle
// =====================================================================

void EditorState::onEnter(Canvas* canvas) {
	loadedProjectPath = CAppContainer::getInstance()->editorProjectPath;
	LOG_INFO("[editor] entering — project=\"{}\"\n",
	         loadedProjectPath.empty() ? "(none)" : loadedProjectPath);

	projectLoaded = false;
	mapLoaded = false;
	levelInfoHijacked = false;
	resolutionHijacked = false;

	// Ensure the window is large enough for the editor's rich UI. The game's
	// default is 480x320 (phone-sized). Bump to 1280x800 (index 12) if we're
	// currently smaller, and restore on exit. SDLGL::updateVideo() only runs
	// once pre-loop or on menu settings changes, so we have to call it
	// ourselves for the resize to actually hit SDL.
	if (auto* sdlGL = CAppContainer::getInstance()->sdlGL) {
		if (!CAppContainer::getInstance()->headless) {
			constexpr int EDITOR_MIN_RES_INDEX = 12;   // 1280x800
			if (sdlGL->resolutionIndex < EDITOR_MIN_RES_INDEX) {
				savedResolutionIndex = sdlGL->resolutionIndex;
				sdlGL->resolutionIndex = EDITOR_MIN_RES_INDEX;
				sdlGL->updateVideo();
				resolutionHijacked = true;
				LOG_INFO("[editor] bumped resolution to index {} ({}x{})\n",
				         EDITOR_MIN_RES_INDEX,
				         sdlGL->winVidWidth, sdlGL->winVidHeight);
			}
		}
	}

	loadProject();
	if (projectLoaded) {
		compileAndLoadMap(canvas);
	}

	loadTextureList();

	// Place camera at project spawn, eye height.
	int spawnCol = project.spawn.col;
	int spawnRow = project.spawn.row;
	int floorWorld = project.floorByte(spawnCol, spawnRow) * 8;
	camera.posX = (spawnCol * 64 + 32) / 128.0f;
	camera.posZ = (spawnRow * 64 + 32) / 128.0f;
	camera.posY = (floorWorld + 36)    / 128.0f;
	camera.yaw = 0.0f;
	camera.pitch = 0.0f;

	lastUpdateMs = SDL_GetTicks();
}

void EditorState::onExit(Canvas* canvas) {
	clearPreviewTextures();
	restoreLevelInfo();
	if (resolutionHijacked) {
		if (auto* sdlGL = CAppContainer::getInstance()->sdlGL) {
			sdlGL->resolutionIndex = savedResolutionIndex;
			sdlGL->updateVideo();
		}
		resolutionHijacked = false;
	}
	LOG_INFO("[editor] leaving\n");
}

void EditorState::loadProject() {
	if (loadedProjectPath.empty()) {
		project = editor::MapProject();
		project.name = "untitled";
		projectLoaded = true;
		return;
	}
	try {
		project = editor::MapProject::loadFromYaml(loadedProjectPath);
		projectLoaded = true;
		LOG_INFO("[editor] loaded project \"{}\" (map_id={})\n",
		         project.name, project.mapId);
	} catch (const std::exception& e) {
		LOG_ERROR("[editor] failed to load project {}: {}\n",
		          loadedProjectPath, e.what());
		projectLoaded = false;
	}
}

void EditorState::compileAndLoadMap(Canvas* canvas) {
	Applet* app = canvas->app;

	editor::CompiledMap cm;
	try {
		cm = editor::compileMap(project);
	} catch (const std::exception& e) {
		LOG_ERROR("[editor] compile failed: {}\n", e.what());
		return;
	}
	LOG_INFO("[editor] compiled: polys={} lines={} nodes={} leaves={} bytes={}\n",
	         cm.numPolys, cm.numLines, cm.numNodes, cm.numLeaves, cm.binData.size());

	scratchDir = (fs::path("levels/_editor_scratch") / project.name).string();
	writeBinFile (fs::path(scratchDir) / "map.bin",    cm.binData);
	writeTextFile(fs::path(scratchDir) / "level.yaml", cm.levelYaml);
	writeTextFile(fs::path(scratchDir) / "scripts.yaml",
	              "static_funcs: {}\ntile_events: []\nbytecode: \"\"\n");
	writeTextFile(fs::path(scratchDir) / "strings.yaml", "english: []\n");

	hijackLevelInfo();

	// If a previous map is loaded, tear it down first so beginLoadMap doesn't leak.
	if (mapLoaded) {
		canvas->unloadMedia();
	}

	if (!app->render->beginLoadMap(project.mapId)) {
		LOG_ERROR("[editor] beginLoadMap({}) failed\n", project.mapId);
		mapLoaded = false;
		return;
	}
	mapLoaded = true;
	lastCompileMs = SDL_GetTicks();
	projectDirty = false;
}

void EditorState::recompileIfDirty(Canvas* canvas) {
	if (!projectDirty) return;
	uint32_t now = SDL_GetTicks();
	if (now - lastCompileMs < RECOMPILE_DEBOUNCE_MS) return;
	compileAndLoadMap(canvas);
}

void EditorState::hijackLevelInfo() {
	// Ensure app->game->levelNames has a slot for project.mapId, otherwise
	// the engine's Canvas::loadMap (line ~1547) indexes out of bounds when
	// we enter Playtest with an unregistered mapId.
	auto* game = CAppContainer::getInstance()->app->game.get();
	if (game && project.mapId > game->levelNamesCount) {
		int newCount = project.mapId;
		int16_t* fresh = new int16_t[newCount]();
		if (game->levelNames) {
			std::memcpy(fresh, game->levelNames, game->levelNamesCount * sizeof(int16_t));
			delete[] game->levelNames;
		}
		game->levelNames = fresh;
		game->levelNamesCount = newCount;
	}

	auto& gc = const_cast<GameConfig&>(CAppContainer::getInstance()->gameConfig);
	auto it = gc.levelInfos.find(project.mapId);
	if (it != gc.levelInfos.end()) {
		savedLevelInfo = it->second;
		hadExistingLevelInfo = true;
	} else {
		hadExistingLevelInfo = false;
	}
	LevelInfo scratch;
	scratch.dir        = scratchDir;
	scratch.mapFile    = scratchDir + "/map.bin";
	scratch.modelFile  = scratchDir + "/model.bin";
	scratch.configFile = scratchDir + "/level.yaml";
	scratch.skyTexture = project.skyTexture;
	gc.levelInfos[project.mapId] = scratch;
	levelInfoHijacked = true;
}

void EditorState::restoreLevelInfo() {
	if (!levelInfoHijacked) return;
	auto& gc = const_cast<GameConfig&>(CAppContainer::getInstance()->gameConfig);
	if (hadExistingLevelInfo) {
		gc.levelInfos[project.mapId] = savedLevelInfo;
	} else {
		gc.levelInfos.erase(project.mapId);
	}
	levelInfoHijacked = false;
}

// =====================================================================
// Per-frame update (input + recompile)
// =====================================================================

void EditorState::update(Canvas* canvas) {
	uint32_t now = SDL_GetTicks();
	float dt = (now - lastUpdateMs) / 1000.0f;
	if (dt > 0.1f) dt = 0.1f;
	lastUpdateMs = now;

	if (ImGui::GetCurrentContext() != nullptr) {
		ImGuiIO& io = ImGui::GetIO();
		bool imguiHasKbd = io.WantCaptureKeyboard;
		bool imguiHasMouse = io.WantCaptureMouse;

		if (!imguiHasKbd) {
			const Uint8* ks = SDL_GetKeyboardState(nullptr);
			bool fwd   = ks[SDL_SCANCODE_W];
			bool back  = ks[SDL_SCANCODE_S];
			bool left  = ks[SDL_SCANCODE_A];
			bool right = ks[SDL_SCANCODE_D];
			bool up    = ks[SDL_SCANCODE_SPACE] || ks[SDL_SCANCODE_E];
			bool dn    = ks[SDL_SCANCODE_LCTRL] || ks[SDL_SCANCODE_Q];
			bool shift = ks[SDL_SCANCODE_LSHIFT] || ks[SDL_SCANCODE_RSHIFT];
			camera.handleKeyboard(fwd, back, left, right, up, dn, shift, dt);
		}

		if (!imguiHasMouse && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
			ImVec2 d = io.MouseDelta;
			camera.handleMouseMotion(int(d.x), int(d.y));
		}
		if (!imguiHasMouse && io.MouseWheel != 0.0f) {
			camera.handleMouseWheel(io.MouseWheel);
		}

		// LMB click in the 3D area with Texture tool → paint on hit surface.
		if (!imguiHasMouse && currentTool == Tool::Texture
		    && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			SurfaceHit h = raycastFromCamera();
			if (applyTextureAt(h)) projectDirty = true;
		}

		// Tool hotkeys (edge-triggered so they don't override palette clicks).
		// B/F/C for Brush/Floor/Ceil, G for door (avoids D = strafe-right),
		// T for texture. Spawn has no hotkey (S = strafe-back collision).
		if (!imguiHasKbd) {
			if (ImGui::IsKeyPressed(ImGuiKey_B, false)) currentTool = Tool::Brush;
			if (ImGui::IsKeyPressed(ImGuiKey_F, false)) currentTool = Tool::Floor;
			if (ImGui::IsKeyPressed(ImGuiKey_C, false)) currentTool = Tool::Ceil;
			if (ImGui::IsKeyPressed(ImGuiKey_G, false)) currentTool = Tool::Door;
			if (ImGui::IsKeyPressed(ImGuiKey_T, false)) currentTool = Tool::Texture;
			// Ctrl+S → save, F5 → playtest.
			bool ctrl = io.KeyCtrl || io.KeySuper;
			if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
				openSaveDialog();
			}
			if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
				playtest(canvas);
			}
		}
	}

	// End-of-stroke: release drag state when the mouse comes up.
	if (ImGui::GetCurrentContext() != nullptr && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		brushDragActive = false;
	}

	recompileIfDirty(canvas);
}

// =====================================================================
// Render: 3D viewport + ImGui panels
// =====================================================================

void EditorState::render(Canvas* canvas, Graphics* graphics) {
	if (mapLoaded && !CAppContainer::getInstance()->headless) {
		int viewX, viewY, viewZ, viewAngle, viewPitch, viewFov;
		camera.getEngineView(viewX, viewY, viewZ, viewAngle, viewPitch, viewFov);
		canvas->renderScene(viewX, viewY, viewZ, viewAngle, viewPitch, 0, viewFov);
	}

	if (ImGui::GetCurrentContext() == nullptr) return;

	drawActionsBar();
	drawToolPalette();
	drawTileGrid2D();
	drawValidationPanel();
	drawCameraPanel();
	drawTexturePicker();
	drawSaveDialog();

	// Crosshair + target HUD (only useful when 3D view is live)
	if (mapLoaded && !CAppContainer::getInstance()->headless) {
		SurfaceHit hit = raycastFromCamera();
		drawCrosshairAndHit(hit);
	}
}

bool EditorState::handleInput(Canvas* canvas, int key, int action) {
	if (key == 18 && action != 0) {
		canvas->setState(Canvas::ST_MENU);
		return true;
	}
	return false;
}

// =====================================================================
// UI panels
// =====================================================================

void EditorState::drawToolPalette() {
	ImGui::SetNextWindowPos(ImVec2(12, 32), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(260, 280), ImGuiCond_FirstUseEver);
	ImGui::Begin("Tools");

	struct Row { const char* label; Tool tool; const char* key; };
	const Row rows[] = {
		{ "Brush  (solid/open)",    Tool::Brush,   "B" },
		{ "Floor byte",             Tool::Floor,   "F" },
		{ "Ceiling byte",           Tool::Ceil,    "C" },
		{ "Door",                   Tool::Door,    "G" },   // 'D' collides with strafe-right
		{ "Spawn",                  Tool::Spawn,   "—" },   // no hotkey (S=strafe-back)
		{ "Texture (3D click)",     Tool::Texture, "T" },
	};
	for (const Row& r : rows) {
		bool active = (currentTool == r.tool);
		if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.85f, 1.0f));
		if (ImGui::Button(r.label, ImVec2(170, 0))) currentTool = r.tool;
		if (active) ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::TextDisabled("(%s)", r.key);
	}

	ImGui::Separator();
	ImGui::Text("Options for: %s",
		currentTool == Tool::Brush ? "Brush" :
		currentTool == Tool::Floor ? "Floor" :
		currentTool == Tool::Ceil  ? "Ceil"  :
		currentTool == Tool::Door  ? "Door"  :
		"Spawn");

	switch (currentTool) {
		case Tool::Floor: {
			int v = floorBrush;
			if (ImGui::SliderInt("Floor byte", &v, 0, 255))
				floorBrush = uint8_t(v);
			ImGui::TextDisabled("Default = %d.  Each unit = 8 world units.", editor::FLOOR_HEIGHT);
			break;
		}
		case Tool::Ceil: {
			int v = ceilBrush;
			if (ImGui::SliderInt("Ceil byte", &v, 0, 255))
				ceilBrush = uint8_t(v);
			ImGui::TextDisabled("Default = %d.  Step walls auto-emitted at mismatches.",
			                    editor::FLOOR_HEIGHT + 8);
			break;
		}
		case Tool::Door: {
			int axis = (doorAxis == 'H') ? 0 : 1;
			if (ImGui::RadioButton("E/W passage", &axis, 0)) doorAxis = 'H';
			ImGui::SameLine();
			if (ImGui::RadioButton("N/S passage", &axis, 1)) doorAxis = 'V';
			ImGui::TextDisabled("Click a tile to place.  Click an existing door to remove.");
			break;
		}
		case Tool::Spawn: {
			const char* names[] = {
				"east", "northeast", "north", "northwest",
				"west", "southwest", "south", "southeast"
			};
			int d = spawnDir;
			if (ImGui::Combo("Facing", &d, names, 8)) spawnDir = uint8_t(d);
			break;
		}
		case Tool::Texture: {
			ImGui::Text("Selected: %d", selectedTexture);
			ImGui::TextDisabled("Click a face in the 3D view to apply.");
			ImGui::TextDisabled("See the Textures panel to pick a different ID.");
			break;
		}
		default: break;
	}

	ImGui::End();
}

void EditorState::drawTileGrid2D() {
	ImGui::SetNextWindowPos(ImVec2(284, 32), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(560, 560), ImGuiCond_FirstUseEver);
	ImGui::Begin("Map (32x32)");

	constexpr float TILE_PX = 16.0f;
	constexpr int   N = editor::MAP_SIZE;

	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImVec2(TILE_PX * N, TILE_PX * N);
	ImGui::InvisibleButton("map_canvas", canvasSize);
	bool canvasHovered = ImGui::IsItemHovered();

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->PushClipRect(canvasPos,
	                 ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

	ImVec2 mouse = ImGui::GetMousePos();
	int hoverCol = int((mouse.x - canvasPos.x) / TILE_PX);
	int hoverRow = int((mouse.y - canvasPos.y) / TILE_PX);
	if (hoverCol < 0 || hoverCol >= N || hoverRow < 0 || hoverRow >= N) {
		hoverCol = hoverRow = -1;
	}

	// --- Tile fills ---
	for (int r = 0; r < N; ++r) {
		for (int c = 0; c < N; ++c) {
			ImVec2 a{ canvasPos.x + c * TILE_PX,     canvasPos.y + r * TILE_PX };
			ImVec2 b{ a.x + TILE_PX,                  a.y + TILE_PX };
			ImU32 col;
			if (project.isSolid(c, r)) {
				col = IM_COL32(48, 48, 52, 255);
			} else {
				// Tint by floor byte: higher floor = warmer.
				int fb = project.floorByte(c, r);
				int t = std::clamp((fb - editor::FLOOR_HEIGHT) * 6, -64, 96);
				int base = 110;
				col = IM_COL32(base + t, base - std::max(0, t)/2,
				               base - std::max(0, t), 255);
			}
			dl->AddRectFilled(a, b, col);
		}
	}

	// --- Ceiling overlay text (non-default only, for readability) ---
	for (int r = 0; r < N; ++r) {
		for (int c = 0; c < N; ++c) {
			if (project.isSolid(c, r)) continue;
			int cb = project.ceilByte(c, r);
			if (cb == editor::FLOOR_HEIGHT + 8) continue;
			char buf[6];
			std::snprintf(buf, sizeof(buf), "%d", cb);
			ImVec2 p{ canvasPos.x + c * TILE_PX + 1, canvasPos.y + r * TILE_PX };
			dl->AddText(p, IM_COL32(220, 220, 120, 255), buf);
		}
	}

	// --- Door markers ---
	for (const editor::Door& d : project.doors) {
		ImVec2 a{ canvasPos.x + d.col * TILE_PX, canvasPos.y + d.row * TILE_PX };
		ImVec2 b{ a.x + TILE_PX, a.y + TILE_PX };
		// Draw a rectangle across the passage perpendicular to its axis.
		if (d.axis == 'H') {  // E/W passage — vertical red bar in tile centre
			ImVec2 p0{ a.x + TILE_PX*0.42f, a.y + TILE_PX*0.10f };
			ImVec2 p1{ a.x + TILE_PX*0.58f, a.y + TILE_PX*0.90f };
			dl->AddRectFilled(p0, p1, IM_COL32(210, 70, 70, 255));
		} else {              // N/S passage — horizontal red bar
			ImVec2 p0{ a.x + TILE_PX*0.10f, a.y + TILE_PX*0.42f };
			ImVec2 p1{ a.x + TILE_PX*0.90f, a.y + TILE_PX*0.58f };
			dl->AddRectFilled(p0, p1, IM_COL32(210, 70, 70, 255));
		}
		dl->AddRect(a, b, IM_COL32(255, 40, 40, 255));
	}

	// --- Spawn marker ---
	{
		float cx = canvasPos.x + project.spawn.col * TILE_PX + TILE_PX * 0.5f;
		float cy = canvasPos.y + project.spawn.row * TILE_PX + TILE_PX * 0.5f;
		float r  = TILE_PX * 0.4f;
		// Spawn dir → angle. Engine: 0=east, 2=north, 4=west, 6=south.
		// Screen: +x=east, +y=south, so east→0°, north→-90°, west→180°, south→90°.
		static const float dirAngleDeg[8] = {
			0, -45, -90, -135, 180, 135, 90, 45
		};
		float ang = dirAngleDeg[project.spawn.dir & 7] * 3.14159265f / 180.0f;
		float tipX = cx + r * std::cos(ang);
		float tipY = cy + r * std::sin(ang);
		float baseL_x = cx + r * std::cos(ang + 2.4f);
		float baseL_y = cy + r * std::sin(ang + 2.4f);
		float baseR_x = cx + r * std::cos(ang - 2.4f);
		float baseR_y = cy + r * std::sin(ang - 2.4f);
		dl->AddTriangleFilled({tipX, tipY}, {baseL_x, baseL_y}, {baseR_x, baseR_y},
		                      IM_COL32(60, 210, 90, 255));
		dl->AddTriangle     ({tipX, tipY}, {baseL_x, baseL_y}, {baseR_x, baseR_y},
		                      IM_COL32(20, 120, 40, 255));
	}

	// --- Grid lines ---
	ImU32 gridCol = IM_COL32(70, 70, 75, 120);
	for (int i = 0; i <= N; ++i) {
		float x = canvasPos.x + i * TILE_PX;
		float y = canvasPos.y + i * TILE_PX;
		dl->AddLine({x, canvasPos.y}, {x, canvasPos.y + canvasSize.y}, gridCol);
		dl->AddLine({canvasPos.x, y}, {canvasPos.x + canvasSize.x, y}, gridCol);
	}

	// --- Hover highlight ---
	if (hoverCol >= 0) {
		ImVec2 a{ canvasPos.x + hoverCol * TILE_PX, canvasPos.y + hoverRow * TILE_PX };
		ImVec2 b{ a.x + TILE_PX, a.y + TILE_PX };
		dl->AddRect(a, b, IM_COL32(80, 180, 255, 255), 0, 0, 2.0f);
	}

	dl->PopClipRect();

	// --- Interaction ---
	if (canvasHovered && hoverCol >= 0) {
		bool leftDown  = ImGui::IsMouseDown(ImGuiMouseButton_Left);
		bool leftClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

		if (leftClick) {
			bool mutated = false;
			switch (currentTool) {
				case Tool::Brush: mutated = applyBrushOnClick(hoverCol, hoverRow); break;
				case Tool::Floor: mutated = applyFloor       (hoverCol, hoverRow); break;
				case Tool::Ceil:  mutated = applyCeil        (hoverCol, hoverRow); break;
				case Tool::Door:  mutated = applyDoor        (hoverCol, hoverRow); break;
				case Tool::Spawn: mutated = applySpawn       (hoverCol, hoverRow); break;
			}
			if (mutated) projectDirty = true;
		} else if (leftDown) {
			bool mutated = false;
			switch (currentTool) {
				case Tool::Brush: mutated = applyBrushOnDrag(hoverCol, hoverRow); break;
				case Tool::Floor: mutated = applyFloor      (hoverCol, hoverRow); break;
				case Tool::Ceil:  mutated = applyCeil       (hoverCol, hoverRow); break;
				default: break;  // no drag semantics for Door/Spawn
			}
			if (mutated) projectDirty = true;
		}
	}

	// --- Status line ---
	if (hoverCol >= 0) {
		ImGui::Text("Tile (%d, %d) — %s  floor=%d  ceil=%d",
		            hoverCol, hoverRow,
		            project.isSolid(hoverCol, hoverRow) ? "solid" : "open",
		            project.floorByte(hoverCol, hoverRow),
		            project.ceilByte(hoverCol, hoverRow));
	} else {
		ImGui::TextDisabled("Hover a tile to inspect.");
	}

	ImGui::End();
}

void EditorState::drawValidationPanel() {
	ImGui::SetNextWindowPos(ImVec2(860, 32), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(280, 180), ImGuiCond_FirstUseEver);
	ImGui::Begin("Validation");

	int openTiles = 0;
	for (int r = 0; r < editor::MAP_SIZE; ++r)
		for (int c = 0; c < editor::MAP_SIZE; ++c)
			if (!project.isSolid(c, r)) ++openTiles;

	// Rough approximation: one leaf per open tile (the typical case).
	int estLeaves = openTiles;

	ImGui::Text("Open tiles: %d", openTiles);
	ImGui::Text("Doors: %zu", project.doors.size());
	ImGui::Text("Estimated leaves: %d", estLeaves);

	auto flag = [](const char* label, int cur, int limit) {
		bool over = cur > limit;
		ImU32 col = over ? IM_COL32(255, 80, 80, 255) : IM_COL32(120, 220, 120, 255);
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::Text("%s: %d / %d %s", label, cur, limit, over ? "(OVER)" : "");
		ImGui::PopStyleColor();
	};
	flag("Leaves",    estLeaves,      256);
	flag("Door count", int(project.doors.size()), 16);   // soft cap for sanity

	ImGui::Separator();
	ImGui::Text(projectDirty ? "● dirty — recompiling soon" : "saved");
	if (mapLoaded) {
		ImGui::TextDisabled("Last compile: %u ms ago",
		                    SDL_GetTicks() - lastCompileMs);
	}

	ImGui::End();
}

void EditorState::drawCameraPanel() {
	ImGui::SetNextWindowPos(ImVec2(860, 220), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(280, 200), ImGuiCond_FirstUseEver);
	ImGui::Begin("Camera / Project");
	ImGui::Text("Project: %s", loadedProjectPath.empty() ? "(blank)"
	                                                     : fs::path(loadedProjectPath).filename().string().c_str());
	if (projectLoaded) {
		ImGui::Text("Name: %s", project.name.c_str());
		ImGui::Text("Map ID: %d", project.mapId);
	}
	ImGui::Separator();
	ImGui::Text("Pos (%.1f, %.1f, %.1f)", camera.posX, camera.posY, camera.posZ);
	ImGui::Text("Yaw %.0f  Pitch %.0f  Spd %.1f", camera.yaw, camera.pitch, camera.moveSpeed);
	ImGui::Separator();
	ImGui::TextDisabled("WASD move | Space/Ctrl up/down | Shift sprint");
	ImGui::TextDisabled("RMB-drag look | Wheel speed | ESC menu");
	ImGui::End();
}

// =====================================================================
// Tool application
// =====================================================================

bool EditorState::applyBrushOnClick(int col, int row) {
	// Decide the stroke mode from the starting tile.
	bool startingWasSolid = project.isSolid(col, row);
	brushDragOpenMode = startingWasSolid;   // if started on solid → paint open
	brushDragActive = true;
	uint8_t newVal = brushDragOpenMode ? 0 : 1;
	if ((project.blockMap[row * editor::MAP_SIZE + col] & 1) == newVal) return false;
	project.blockMap[row * editor::MAP_SIZE + col] = newVal;
	return true;
}

bool EditorState::applyBrushOnDrag(int col, int row) {
	if (!brushDragActive) return false;
	uint8_t newVal = brushDragOpenMode ? 0 : 1;
	if ((project.blockMap[row * editor::MAP_SIZE + col] & 1) == newVal) return false;
	project.blockMap[row * editor::MAP_SIZE + col] = newVal;
	return true;
}

bool EditorState::applyFloor(int col, int row) {
	if (project.isSolid(col, row)) return false;
	uint16_t k = editor::tileKey(col, row);
	uint8_t cur = uint8_t(project.floorByte(col, row));
	if (cur == floorBrush) return false;
	project.tileFloorByte[k] = floorBrush;
	return true;
}

bool EditorState::applyCeil(int col, int row) {
	if (project.isSolid(col, row)) return false;
	uint16_t k = editor::tileKey(col, row);
	uint8_t cur = uint8_t(project.ceilByte(col, row));
	if (cur == ceilBrush) return false;
	project.tileCeilByte[k] = ceilBrush;
	return true;
}

bool EditorState::removeDoorAt(int col, int row) {
	auto it = std::find_if(project.doors.begin(), project.doors.end(),
		[col, row](const editor::Door& d){ return d.col == col && d.row == row; });
	if (it == project.doors.end()) return false;
	project.doors.erase(it);
	return true;
}

bool EditorState::applyDoor(int col, int row) {
	// Click on an existing door → remove it; otherwise place one.
	if (removeDoorAt(col, row)) return true;

	if (project.isSolid(col, row)) {
		// Auto-convert to open so the door tile makes sense.
		project.blockMap[row * editor::MAP_SIZE + col] = 0;
	}
	editor::Door d;
	d.col = uint8_t(col);
	d.row = uint8_t(row);
	d.axis = doorAxis;
	project.doors.push_back(d);
	return true;
}

bool EditorState::applySpawn(int col, int row) {
	editor::Spawn s{ uint8_t(col), uint8_t(row), spawnDir };
	if (s.col == project.spawn.col && s.row == project.spawn.row && s.dir == project.spawn.dir) {
		return false;
	}
	project.spawn = s;
	return true;
}

// =====================================================================
// Texture picker + 3D surface picker (Phase 4)
// =====================================================================

unsigned int EditorState::getPreviewTexture(int id) {
	auto it = texturePreviews.find(id);
	if (it != texturePreviews.end()) return it->second;

	// Load PNG via VFS (searches game dir + mods). Resource path is
	// relative to the game dir — `textures/` is on the asset search path.
	std::string path = std::string("textures/texture_") + std::to_string(id) + ".png";
	auto* vfs = CAppContainer::getInstance()->vfs;
	int fileSize = 0;
	uint8_t* data = vfs ? vfs->readFile(path.c_str(), &fileSize) : nullptr;
	if (!data) {
		texturePreviews[id] = 0;
		return 0;
	}
	int w = 0, h = 0, ch = 0;
	uint8_t* rgba = stbi_load_from_memory(data, fileSize, &w, &h, &ch, 4);
	std::free(data);
	if (!rgba) {
		texturePreviews[id] = 0;
		return 0;
	}

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	stbi_image_free(rgba);

	texturePreviews[id] = tex;
	return tex;
}

void EditorState::clearPreviewTextures() {
	if (texturePreviews.empty()) return;
	for (auto& [id, tex] : texturePreviews) {
		if (tex) {
			GLuint t = tex;
			glDeleteTextures(1, &t);
		}
	}
	texturePreviews.clear();
}

void EditorState::loadTextureList() {
	if (!availableTextures.empty()) return;
	try {
		YAML::Node doc = YAML::LoadFile("levels/textures/media_texels.yaml");
		for (auto it = doc.begin(); it != doc.end(); ++it) {
			// Keys are integer strings; skip non-integer keys.
			try {
				int id = it->first.as<int>();
				// IDs < 257 are sprite/transparent media (monsters, items,
				// etc.). The engine routes them through the sprite rasteriser
				// when referenced as a poly tileNum, which segfaults. Only
				// keep world-texture IDs in the palette.
				if (id < 257) continue;
				availableTextures.push_back(id);
			} catch (...) { continue; }
		}
		std::sort(availableTextures.begin(), availableTextures.end());
	} catch (const std::exception& e) {
		LOG_WARN("[editor] failed to load texture list: {}\n", e.what());
	}

	// Cross-reference sprites.yaml for human-readable names and tags. Keyed
	// by the entry's `id:` value so the editor can display `#<id> <name>`
	// and filter by name substring or tag.
	try {
		YAML::Node root = YAML::LoadFile("sprites.yaml");
		YAML::Node sprites = root["sprites"];
		if (sprites && sprites.IsMap()) {
			for (auto it = sprites.begin(); it != sprites.end(); ++it) {
				if (!it->second.IsMap()) continue;
				YAML::Node idNode = it->second["id"];
				if (!idNode) continue;
				int id = idNode.as<int>(0);
				if (id <= 0) continue;
				TextureMeta meta;
				// Prefer explicit name: field; fall back to the YAML key.
				if (YAML::Node nameNode = it->second["name"]; nameNode && nameNode.IsScalar()) {
					meta.name = nameNode.as<std::string>();
				}
				if (meta.name.empty()) meta.name = it->first.as<std::string>();
				if (YAML::Node tagsNode = it->second["tags"]; tagsNode && tagsNode.IsSequence()) {
					for (auto tit = tagsNode.begin(); tit != tagsNode.end(); ++tit) {
						meta.tags.push_back(tit->as<std::string>());
					}
				}
				textureMeta.emplace(id, std::move(meta));
			}
		}
	} catch (const std::exception& e) {
		LOG_WARN("[editor] failed to load sprites.yaml metadata: {}\n", e.what());
	}

	// Collect the distinct tag set across the available world textures so the
	// picker's tag-filter dropdown has something to offer.
	std::set<std::string> tagSet;
	for (int id : availableTextures) {
		auto it = textureMeta.find(id);
		if (it == textureMeta.end()) continue;
		for (const auto& tag : it->second.tags) tagSet.insert(tag);
	}
	knownTags.assign(tagSet.begin(), tagSet.end());
	LOG_INFO("[editor] texture palette: {} entries, {} unique tags, {} w/ metadata\n",
	         availableTextures.size(), knownTags.size(), textureMeta.size());
}

void EditorState::drawTexturePicker() {
	ImGui::SetNextWindowPos(ImVec2(860, 430), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(280, 320), ImGuiCond_FirstUseEver);
	ImGui::Begin("Textures");

	ImGui::Text("Selected: %d", selectedTexture);
	ImGui::InputInt("Texture ID", &selectedTexture);
	// Sprite-range IDs (< 257) crash the renderer when used as wall/floor/
	// ceiling tileNums — clamp to the world-texture range.
	if (selectedTexture < 257) selectedTexture = 257;

	// Thumbnail of the selected texture.
	unsigned int previewGL = getPreviewTexture(selectedTexture);
	if (previewGL) {
		ImGui::Image(ImTextureID(intptr_t(previewGL)), ImVec2(128, 128));
	} else {
		ImGui::TextDisabled("(no preview — textures/texture_%d.png not found)",
		                    selectedTexture);
	}

	// Show name + tags of the currently selected texture.
	if (auto mit = textureMeta.find(selectedTexture); mit != textureMeta.end()) {
		ImGui::Text("Name: %s", mit->second.name.c_str());
		if (!mit->second.tags.empty()) {
			std::string joined;
			for (size_t i = 0; i < mit->second.tags.size(); ++i) {
				if (i) joined += ", ";
				joined += mit->second.tags[i];
			}
			ImGui::Text("Tags: %s", joined.c_str());
		} else {
			ImGui::TextDisabled("Tags: (none)");
		}
	}

	ImGui::TextDisabled("World textures only (IDs ≥ 257).");
	ImGui::Separator();
	char buf[64];
	std::strncpy(buf, textureFilter.c_str(), sizeof(buf));
	buf[sizeof(buf) - 1] = 0;
	if (ImGui::InputText("Filter", buf, sizeof(buf))) {
		textureFilter = buf;
	}
	ImGui::TextDisabled("(ID, name substring, or #tag)");

	// Tag dropdown — clicking a tag fills the filter with `#<tag>` so users
	// don't have to remember tag names. "Any tag" clears.
	if (!knownTags.empty()) {
		// Build a combo with "Any tag" at index 0.
		std::vector<const char*> items;
		items.push_back("Any tag");
		for (const auto& t : knownTags) items.push_back(t.c_str());
		int current = 0;
		if (!textureFilter.empty() && textureFilter[0] == '#') {
			std::string want = textureFilter.substr(1);
			for (size_t i = 0; i < knownTags.size(); ++i) {
				if (knownTags[i] == want) { current = int(i + 1); break; }
			}
		}
		if (ImGui::Combo("Tag", &current, items.data(), int(items.size()))) {
			if (current == 0) textureFilter.clear();
			else              textureFilter = "#" + knownTags[current - 1];
		}
	}

	if (!textureFilter.empty()) {
		ImGui::SameLine();
		if (ImGui::SmallButton("Clear")) textureFilter.clear();
	}

	ImGui::TextDisabled("%zu textures available", availableTextures.size());

	ImGui::BeginChild("texlist", ImVec2(0, 0), true);
	// Filter semantics: empty = show all; starts with '#' = tag exact match;
	// pure integer = exact ID; otherwise = case-insensitive substring of name.
	std::string filter = textureFilter;
	int filterId = -1;
	std::string tagQuery, nameQuery;
	if (!filter.empty()) {
		if (filter[0] == '#') {
			tagQuery = filter.substr(1);
		} else {
			try {
				size_t pos = 0;
				filterId = std::stoi(filter, &pos);
				if (pos != filter.size()) filterId = -1;
			} catch (...) { filterId = -1; }
			if (filterId < 0) {
				nameQuery.reserve(filter.size());
				for (char c : filter) nameQuery.push_back(std::tolower(static_cast<unsigned char>(c)));
			}
		}
	}
	auto matchesFilter = [&](int id) {
		if (filter.empty()) return true;
		if (filterId >= 0) return id == filterId;
		auto mit = textureMeta.find(id);
		if (mit == textureMeta.end()) return false;
		if (!tagQuery.empty()) {
			for (const auto& t : mit->second.tags) if (t == tagQuery) return true;
			return false;
		}
		std::string lname = mit->second.name;
		for (char& c : lname) c = std::tolower(static_cast<unsigned char>(c));
		return lname.find(nameQuery) != std::string::npos;
	};
	for (int id : availableTextures) {
		if (!matchesFilter(id)) continue;
		char label[96];
		if (auto mit = textureMeta.find(id); mit != textureMeta.end()) {
			std::snprintf(label, sizeof(label), "#%d  %s", id, mit->second.name.c_str());
		} else {
			std::snprintf(label, sizeof(label), "#%d", id);
		}
		bool sel = (id == selectedTexture);
		if (ImGui::Selectable(label, sel)) selectedTexture = id;
	}
	ImGui::EndChild();

	ImGui::End();
}

// Raymarch from the camera forward direction, step 2 world units, max 512.
// Returns the first hit — floor, ceiling, or wall face — or invalid if none.
EditorState::SurfaceHit EditorState::raycastFromCamera() const {
	SurfaceHit hit;

	// Camera in world coords. Camera stores pos in editor units (world/128).
	float pxW = camera.posX * 128.0f;
	float pyW = camera.posZ * 128.0f;   // editor Z → engine Y (ground plane)
	float pzW = camera.posY * 128.0f;   // editor Y → engine Z (up)

	// Forward vector from yaw/pitch. Yaw=0 → +X (east), pitch=0 → horizontal.
	const float deg = 3.14159265f / 180.0f;
	float yaw = camera.yaw   * deg;
	float pit = camera.pitch * deg;
	float fx  = std::cos(yaw) * std::cos(pit);
	float fy  = -std::sin(yaw) * std::cos(pit);
	float fz  = std::sin(pit);

	const float stepLen = 2.0f;
	const int   maxSteps = 600;
	int prevCol = int(pxW / 64);
	int prevRow = int(pyW / 64);

	for (int i = 1; i <= maxSteps; ++i) {
		float x = pxW + fx * stepLen * i;
		float y = pyW + fy * stepLen * i;
		float z = pzW + fz * stepLen * i;
		int col = int(x / 64);
		int row = int(y / 64);
		if (col < 0 || col >= editor::MAP_SIZE || row < 0 || row >= editor::MAP_SIZE) break;

		// Stepped into a new tile?
		if (col != prevCol || row != prevRow) {
			if (project.isSolid(col, row)) {
				// Wall hit — the wall lies on the OPEN neighbour (prev) side.
				if (project.isSolid(prevCol, prevRow)) {
					// Both solid — can happen if ray grazed through a solid block.
					break;
				}
				hit.valid = true;
				hit.col = prevCol;
				hit.row = prevRow;
				if      (col > prevCol) hit.face = 'E';
				else if (col < prevCol) hit.face = 'W';
				else if (row > prevRow) hit.face = 'S';
				else                    hit.face = 'N';
				hit.distance = stepLen * i;
				return hit;
			}
			prevCol = col;
			prevRow = row;
		}

		// Inside an open tile — test floor/ceiling.
		if (!project.isSolid(col, row)) {
			int floorWorld = project.floorByte(col, row) * 8;
			int ceilWorld  = project.ceilByte(col, row)  * 8;
			if (z < floorWorld) {
				hit.valid = true;
				hit.col = col;
				hit.row = row;
				hit.face = 'F';
				hit.distance = stepLen * i;
				return hit;
			}
			if (z > ceilWorld) {
				hit.valid = true;
				hit.col = col;
				hit.row = row;
				hit.face = 'C';
				hit.distance = stepLen * i;
				return hit;
			}
		}
	}
	return hit;
}

bool EditorState::applyTextureAt(const SurfaceHit& h) {
	if (!h.valid) return false;
	uint16_t tk = editor::tileKey(h.col, h.row);
	if (h.face == 'F') {
		if (project.floorTex(h.col, h.row) == selectedTexture) return false;
		project.tileFloorTex[tk] = selectedTexture;
		return true;
	} else if (h.face == 'C') {
		if (project.ceilTex(h.col, h.row) == selectedTexture) return false;
		project.tileCeilTex[tk] = selectedTexture;
		return true;
	} else {
		int dir = (h.face == 'N') ? editor::DIR_N
		        : (h.face == 'E') ? editor::DIR_E
		        : (h.face == 'S') ? editor::DIR_S
		        : editor::DIR_W;
		uint32_t wk = editor::wallKey(h.col, h.row, dir);
		if (project.wallTex(h.col, h.row, dir) == selectedTexture) return false;
		project.tileWallTex[wk] = selectedTexture;
		return true;
	}
}

void EditorState::drawCrosshairAndHit(const SurfaceHit& hit) {
	// Draw crosshair at screen centre via the foreground draw list so it
	// overlays both the 3D viewport and any ImGui panels behind it.
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 c{ io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f };
	ImDrawList* fg = ImGui::GetForegroundDrawList();
	fg->AddCircle(c, 4.0f, IM_COL32(255, 255, 255, 180), 0, 1.5f);
	fg->AddLine({c.x - 8, c.y}, {c.x - 3, c.y}, IM_COL32(255, 255, 255, 200));
	fg->AddLine({c.x + 3, c.y}, {c.x + 8, c.y}, IM_COL32(255, 255, 255, 200));
	fg->AddLine({c.x, c.y - 8}, {c.x, c.y - 3}, IM_COL32(255, 255, 255, 200));
	fg->AddLine({c.x, c.y + 3}, {c.x, c.y + 8}, IM_COL32(255, 255, 255, 200));

	// Small HUD line bottom-left of the crosshair.
	if (hit.valid) {
		const char* faceName =
			hit.face == 'F' ? "floor" :
			hit.face == 'C' ? "ceiling" :
			hit.face == 'N' ? "wall N" :
			hit.face == 'E' ? "wall E" :
			hit.face == 'S' ? "wall S" :
			                  "wall W";
		int tex = (hit.face == 'F') ? project.floorTex(hit.col, hit.row)
		        : (hit.face == 'C') ? project.ceilTex(hit.col, hit.row)
		        : project.wallTex(hit.col, hit.row,
		                          hit.face == 'N' ? editor::DIR_N :
		                          hit.face == 'E' ? editor::DIR_E :
		                          hit.face == 'S' ? editor::DIR_S :
		                                            editor::DIR_W);
		char buf[96];
		std::snprintf(buf, sizeof(buf), "target: (%d,%d) %s  tex=%d",
		              hit.col, hit.row, faceName, tex);
		fg->AddText({c.x + 14, c.y + 14}, IM_COL32(255, 255, 255, 230), buf);
	}
}

// =====================================================================
// Save / Playtest (Phase 5)
// =====================================================================

namespace {

// Pad an int to 2 digits: 11 → "11", 5 → "05".
std::string pad2(int n) {
	char buf[8];
	std::snprintf(buf, sizeof(buf), "%02d", n);
	return buf;
}

// Normalise a user-entered project name to a filename-safe slug.
std::string slugify(const std::string& in) {
	std::string out;
	for (char c : in) {
		if (std::isalnum(static_cast<unsigned char>(c))) out += char(std::tolower(c));
		else if (c == ' ' || c == '_' || c == '-') out += '_';
	}
	if (out.empty()) out = "untitled";
	return out;
}

}  // namespace

void EditorState::openSaveDialog() {
	// Seed the dialog inputs from the current project.
	std::strncpy(saveNameBuf, project.name.c_str(), sizeof(saveNameBuf) - 1);
	saveNameBuf[sizeof(saveNameBuf) - 1] = 0;
	saveMapId = project.mapId;
	saveDialogRequested = true;
}

void EditorState::drawActionsBar() {
	// A thin top-anchored window holding Save / Playtest buttons + last-save msg.
	ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(1100, 24), ImGuiCond_Always);
	ImGui::Begin("actions", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
	             ImGuiWindowFlags_NoSavedSettings);

	if (ImGui::Button("Save… (Ctrl+S)")) openSaveDialog();
	ImGui::SameLine();
	if (ImGui::Button("Playtest (F5)")) {
		// We need the Canvas pointer; it's the owner of this state — obtain from
		// CAppContainer to avoid plumbing it through.
		playtest(CAppContainer::getInstance()->app->canvas.get());
	}
	ImGui::SameLine();
	ImGui::TextDisabled(" | ");
	ImGui::SameLine();
	ImGui::Text("%s — map %d %s",
		project.name.c_str(), project.mapId,
		projectDirty ? "●" : "");
	if (!lastSaveMsg.empty() && SDL_GetTicks() - lastSaveMsgMs < 4000) {
		ImGui::SameLine();
		ImGui::TextDisabled(" | ");
		ImGui::SameLine();
		ImGui::TextUnformatted(lastSaveMsg.c_str());
	}
	ImGui::End();
}

void EditorState::drawSaveDialog() {
	if (saveDialogRequested) {
		ImGui::OpenPopup("Save Project");
		saveDialogRequested = false;
	}
	if (ImGui::BeginPopupModal("Save Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::InputText("Name", saveNameBuf, sizeof(saveNameBuf));
		ImGui::InputInt ("Map ID", &saveMapId);
		if (saveMapId < 1) saveMapId = 1;
		ImGui::Checkbox("Register in games/<game>/game.yaml", &saveRegisterGameYaml);

		ImGui::Separator();
		std::string slug = slugify(saveNameBuf);
		std::string relDir = "levels/" + pad2(saveMapId) + "_" + slug;
		ImGui::TextDisabled("Will write:");
		ImGui::TextDisabled("  %s/map.bin + level.yaml + stubs", relDir.c_str());
		ImGui::TextDisabled("  src/editor/projects/%s.yaml", slug.c_str());

		if (ImGui::Button("Save", ImVec2(120, 0))) {
			project.name  = slug;
			project.mapId = saveMapId;
			bool ok = performSave();
			lastSaveMsg   = ok ? "saved" : "save failed (see log)";
			lastSaveMsgMs = SDL_GetTicks();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

bool EditorState::performSave() {
	// Compile a fresh .bin + level.yaml.
	editor::CompiledMap cm;
	try {
		cm = editor::compileMap(project);
	} catch (const std::exception& e) {
		LOG_ERROR("[editor] save: compile failed: {}\n", e.what());
		return false;
	}

	// Output paths.
	std::string relDir = "levels/" + pad2(project.mapId) + "_" + project.name;
	fs::path levelDir = relDir;
	writeBinFile (levelDir / "map.bin",      cm.binData);
	writeTextFile(levelDir / "level.yaml",   cm.levelYaml);
	writeTextFile(levelDir / "scripts.yaml",
	              "static_funcs: {}\ntile_events: []\nbytecode: \"\"\n");
	writeTextFile(levelDir / "strings.yaml", "english: []\n");

	// Project YAML path — live under src/editor/projects/ in the repo root
	// (resolved via the originalCwd we stashed before the engine chdir'd).
	std::string origCwd = CAppContainer::getInstance()->originalCwd;
	fs::path projPath;
	if (!loadedProjectPath.empty() && fs::path(loadedProjectPath).is_absolute()) {
		projPath = loadedProjectPath;
	} else {
		fs::path base = origCwd.empty() ? fs::current_path() : fs::path(origCwd);
		projPath = base / "src/editor/projects" / (project.name + ".yaml");
	}
	try {
		fs::create_directories(projPath.parent_path());
		project.saveToYaml(projPath.string());
		loadedProjectPath = projPath.string();
	} catch (const std::exception& e) {
		LOG_ERROR("[editor] save: project yaml failed: {}\n", e.what());
		return false;
	}

	// Update in-memory LevelInfo so future (re)compiles and Playtest hit the
	// new real directory. We leave the scratch dir populated but stop using it.
	{
		auto& gc = const_cast<GameConfig&>(CAppContainer::getInstance()->gameConfig);
		LevelInfo info;
		info.dir        = relDir;
		info.mapFile    = relDir + "/map.bin";
		info.configFile = relDir + "/level.yaml";
		info.modelFile  = relDir + "/model.bin";
		info.skyTexture = project.skyTexture;
		gc.levelInfos[project.mapId] = info;
		// The hijack is now permanent; don't restore on exit.
		levelInfoHijacked = false;
	}

	// Register in game.yaml if requested.
	if (saveRegisterGameYaml) {
		std::string stringsPath = relDir + "/strings.yaml";
		auto r = editor::registerLevelInGameYaml("game.yaml", project.mapId,
		                                         relDir, stringsPath);
		if (!r.ok) {
			LOG_WARN("[editor] game.yaml update failed: {}\n", r.error);
		} else {
			LOG_INFO("[editor] game.yaml: level {} stringsGroup {} appended(level={} strings={})\n",
			         project.mapId, r.stringsGroupIndex,
			         r.levelAppended, r.stringsAppended);
		}
	}

	LOG_INFO("[editor] saved \"{}\" → {} (map_id={})\n",
	         project.name, relDir, project.mapId);
	projectDirty = false;
	return true;
}

void EditorState::playtest(Canvas* canvas) {
	if (!canvas) return;
	// Fresh compile if dirty.
	if (projectDirty) compileAndLoadMap(canvas);
	if (!mapLoaded) {
		LOG_WARN("[editor] playtest: no map loaded\n");
		return;
	}

	LOG_INFO("[editor] playtest → ST_PLAYING (map {})\n", project.mapId);
	// Restore window size so the game isn't stuck at editor dimensions.
	if (resolutionHijacked) {
		if (auto* sdlGL = CAppContainer::getInstance()->sdlGL) {
			sdlGL->resolutionIndex = savedResolutionIndex;
			sdlGL->updateVideo();
		}
		resolutionHijacked = false;
	}
	// CRITICAL: canvas->loadMap triggers ST_EDITOR → ST_TRAVELMAP which runs
	// our onExit → restoreLevelInfo, erasing levelInfos[mapId]. The subsequent
	// beginLoadMap then can't find the map file. Clear the hijack flag so the
	// scratch LevelInfo stays resident for the playtest session.
	levelInfoHijacked = false;

	CAppContainer::getInstance()->customMapID = project.mapId;
	CAppContainer::getInstance()->pendingEquipLevel = project.mapId;
	CAppContainer::getInstance()->skipTravelMap = true;
	canvas->startupMap = project.mapId;
	canvas->loadMap(project.mapId, /*newGame=*/true, /*reload=*/true);
}
