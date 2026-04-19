#pragma once
#include "ICanvasState.h"
#include "Camera.h"
#include "MapProject.h"
#include "CAppContainer.h"  // for LevelInfo

#include <cstdint>
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
	// Per-ID metadata loaded from sprites.yaml (name = YAML key, tags = tags: [..]).
	struct TextureMeta {
		std::string name;
		std::vector<std::string> tags;
	};
	std::unordered_map<int, TextureMeta> textureMeta;
	std::string textureFilter;     // name substring search
	std::string textureTagFilter;  // tag substring filter (exact-match if non-empty)
	// All distinct tags found across available textures (sorted, for dropdown).
	std::vector<std::string> knownTags;
	// GL texture IDs for ImGui thumbnails (keyed by tex ID; 0 = failed to load).
	std::unordered_map<int, unsigned int> texturePreviews;

	// Drag-paint state: the "mode" of the current stroke is set by the first
	// tile clicked (so dragging over mixed tiles doesn't ping-pong).
	bool    brushDragActive = false;
	bool    brushDragOpenMode = false;                   // true → painting open, false → solid

	// Debounced recompile
	bool      projectDirty = false;
	uint32_t  lastCompileMs = 0;
	static constexpr uint32_t RECOMPILE_DEBOUNCE_MS = 120;

	// --- Helpers ---
	void loadProject();
	void compileAndLoadMap(Canvas* canvas);
	void recompileIfDirty(Canvas* canvas);
	void hijackLevelInfo();
	void restoreLevelInfo();

	// --- UI panels ---
	void drawToolPalette();
	void drawTileGrid2D();
	void drawValidationPanel();
	void drawCameraPanel();
	void drawTexturePicker();
	void drawCrosshairAndHit(const SurfaceHit& hit);
	void drawSaveDialog();
	void drawActionsBar();  // save / playtest buttons

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
};
