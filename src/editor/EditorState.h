#pragma once
#include "ICanvasState.h"
#include "Camera.h"
#include "MapProject.h"
#include "CAppContainer.h"  // for LevelInfo

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// Map editor as an ICanvasState plugged into the engine's Canvas state machine.
// Entered via the --editor CLI flag on startup, or via the MENU_DEBUG "Map Editor" item.
class EditorState : public ICanvasState {
public:
	enum class Tool {
		Brush,      // B — toggle solid/open on drag
		Floor,      // F — paint per-tile floor byte
		Ceil,       // C — paint per-tile ceiling byte
		Door,       // D — place/remove a door
		Spawn,      // S — set the player spawn tile
		Texture,    // T — paint texture on face under crosshair (3D click)
		Line,       // L — draw a free-form wall line (click start, click end)
		Entity,     // N — place / remove / inspect entities on tiles
	};

	enum class SnapMode {
		Off,        // 1-pixel resolution (≈ 4 world units)
		Fine,       // 8 world units (byte / engine vertex resolution)
		Coarse,     // 64 world units (tile boundary)
	};

	// Surface picker result — which face the camera crosshair is pointing at.
	struct SurfaceHit {
		bool    valid = false;
		int     col = 0;
		int     row = 0;
		// 'F' floor, 'C' ceiling, or one of N/E/S/W for a wall face.
		char    face = '\0';
		float   distance = 0.0f;
	};

	void onEnter(Canvas* canvas) override;
	void onExit(Canvas* canvas) override;
	void update(Canvas* canvas) override;
	void render(Canvas* canvas, Graphics* graphics) override;
	bool handleInput(Canvas* canvas, int key, int action) override;

private:
	editor::MapProject project;
	editor::Camera camera;
	std::string loadedProjectPath;
	std::string scratchDir;

	// Save/restore so exiting the editor leaves levelInfos untouched.
	bool levelInfoHijacked = false;
	bool hadExistingLevelInfo = false;
	LevelInfo savedLevelInfo{};

	bool projectLoaded = false;
	bool mapLoaded = false;

	// Window-size save/restore around the editor session.
	int savedResolutionIndex = -1;
	bool resolutionHijacked = false;

	// Save dialog state
	bool saveDialogRequested = false;
	char saveNameBuf[64] = {};
	int  saveMapId = 0;
	bool saveRegisterGameYaml = true;
	std::string lastSaveMsg;
	uint32_t    lastSaveMsgMs = 0;

	// Tracks the previous frame's time for dt.
	uint32_t lastUpdateMs = 0;

	// --- Authoring tools ---
	Tool    currentTool = Tool::Brush;
	uint8_t floorBrush  = editor::FLOOR_HEIGHT;         // value applied by Floor tool
	uint8_t ceilBrush   = editor::FLOOR_HEIGHT + 8;     // value applied by Ceil tool
	char    doorAxis    = 'H';                           // 'H' = EW passage, 'V' = NS
	uint8_t spawnDir    = 0;                             // 0..7 compass direction
	int     selectedTexture = editor::D2_WALL_TILE;      // for Texture tool
	// Cached list of available texture IDs (loaded once from media_texels.yaml).
	std::vector<int> availableTextures;
	// Per-ID metadata loaded from sprites.yaml. `key` is the YAML map key — the
	// canonical identifier the engine's SpriteDefs::getIndex(name) accepts.
	// `name` is the optional human-readable display label (sprites.yaml `name:`);
	// if absent it falls back to the key. Never emit `name` to level.yaml:
	// only `key` resolves via SpriteDefs.
	struct TextureMeta {
		std::string key;
		std::string name;
		std::vector<std::string> tags;
	};
	std::unordered_map<int, TextureMeta> textureMeta;
	std::string textureFilter;     // name substring search
	std::string textureTagFilter;  // tag substring filter (exact-match if non-empty)
	// All distinct tags found across available textures (sorted, for dropdown).
	std::vector<std::string> knownTags;

	// Entity catalog (sprites.yaml IDs < 257 — monsters / items / NPCs / decor).
	std::vector<int>         availableEntities;     // sorted by ID
	std::string              selectedEntityTile;    // sprite name (YAML key), set by Entity picker
	int                      selectedEntityId = 0;  // resolved sprite ID paired with selectedEntityTile
	std::string              entityFilter;
	int                      selectedEntityIdx = -1; // inspector: index into project.entities; -1 = none
	// GL texture for ImGui thumbnails, plus native image dimensions and — for
	// strip-packed sprite sheets — per-frame dimensions. Lets the previews
	// show only frame 0 instead of the whole strip, and lets composite
	// multi-layer monsters be assembled in the entity preview.
	struct PreviewTex {
		unsigned int gl = 0;    // GL texture name; 0 = load failed
		int w = 0, h = 0;       // full-image size (pixels)
		int frameW = 0;         // per-frame width; 0 ⇒ whole image is one frame
		int frameH = 0;
	};
	std::unordered_map<int, PreviewTex> texturePreviews;

	// Drag-paint state: the "mode" of the current stroke is set by the first
	// tile clicked (so dragging over mixed tiles doesn't ping-pong).
	bool    brushDragActive = false;
	bool    brushDragOpenMode = false;                   // true → painting open, false → solid

	// Line tool state: awaiting second click to commit (x0,y0)→cursor.
	bool    linePending = false;
	int     linePendingX = 0;
	int     linePendingY = 0;
	SnapMode snapMode = SnapMode::Fine;                  // default: engine resolution

	// Debounced recompile. `strokeInProgress` is set while the user is holding
	// LMB on a drag-capable tool (Brush/Floor/Ceil); recompile is deferred
	// until the mouse is released so the engine reload runs at most once per
	// stroke, not mid-drag at 120 ms cadence.
	bool      projectDirty = false;
	bool      strokeInProgress = false;
	uint32_t  lastCompileMs = 0;
	static constexpr uint32_t RECOMPILE_DEBOUNCE_MS = 120;

	// Set of texture IDs compiled into the last engine reload. If the next
	// compile's set is identical we can take the fast geometry-only hot path
	// (≈5 ms) instead of the full beginLoadMap (≈200 ms).
	std::set<int> lastCompiledMediaSet;

	// --- Helpers ---
	void loadProject();
	void compileAndLoadMap(Canvas* canvas);
	void recompileIfDirty(Canvas* canvas);
	void hijackLevelInfo();
	void restoreLevelInfo();

	// --- UI panels (top-level windows) ---
	// Layout: top actions bar · left tool strip · centered 2D map overlay ·
	// right contextual inspector sidebar · bottom status bar. Each `draw*Body`
	// below is a self-contained fragment rendered inline inside its owner
	// window (e.g. the inspector sidebar picks one body based on currentTool).
	void drawActionsBar();      // top strip: save / playtest / dirty / fade msg
	void drawToolStrip();       // left strip: one button per Tool enum value
	void drawTileGrid2D();      // 2D minimap overlay (own window; hover tooltip)
	void drawInspectorSidebar();// right sidebar: dispatches on currentTool
	void drawStatusBar();       // bottom strip: validation + camera summary
	void drawCrosshairAndHit(const SurfaceHit& hit);
	void drawSaveDialog();

	// --- Inspector sidebar bodies (no Begin/End — rendered inside the sidebar) ---
	void drawToolOptionsBody();   // Brush / Floor / Ceil / Door / Spawn / Line
	void drawTexturePickerBody(); // Texture tool
	void drawEntityPickerBody();  // Entity tool: top section (sprite palette)
	void drawEntityInspectorBody();// Entity tool: bottom section (selected inst)

	// --- Save / Playtest ---
	void openSaveDialog();
	bool performSave();      // writes project yaml + level dir + registers
	void playtest(Canvas* canvas);

	// --- Surface picker ---
	SurfaceHit raycastFromCamera() const;
	bool applyTextureAt(const SurfaceHit& hit);

	// --- One-time setup ---
	void loadTextureList();

	// --- Texture preview helpers ---
	unsigned int getPreviewTexture(int id);   // lazy-loads + caches GL texture
	// Returns the full PreviewTex entry (GL + dims + per-frame dims) so
	// callers can compute UVs for a single frame of a horizontal strip.
	const PreviewTex& getPreviewTexInfo(int id);
	// Draws a single sprite frame (frame 0 by default) as an ImGui image in
	// the current window at the cursor. `pxSize` is the edge length of the
	// preview square. Composite multi-layer sprites draw every layer stacked
	// with z-offsets derived from each layer's zMult.
	void drawSpritePreview(int id, int pxSize);
	void clearPreviewTextures();

	// --- Tool application at (col, row). Returns true if anything mutated. ---
	bool applyBrushOnClick(int col, int row);
	bool applyBrushOnDrag (int col, int row);
	bool applyFloor       (int col, int row);
	bool applyCeil        (int col, int row);
	bool applyDoor        (int col, int row);
	bool applySpawn       (int col, int row);

	// Remove a door at (col,row) if one exists; return true if mutated.
	bool removeDoorAt(int col, int row);

	// Entity tool — place at (col, row), or pick / remove if one exists.
	bool applyEntity(int col, int row);
	int  findEntityAt(int col, int row) const;   // returns index or -1
};
