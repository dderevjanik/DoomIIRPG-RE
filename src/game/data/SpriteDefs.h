#pragma once
#include <expected>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "StringHash.h"

// Bin   = legacy binary sprite from tables.bin (has id)
// Image = single image file, no animation frames (png/bmp)
// Sheet = sprite sheet image with frame metadata (png/bmp)
enum class SpriteSourceType { Bin, Image, Sheet };

struct SpriteSource {
	SpriteSourceType type = SpriteSourceType::Bin;
	std::string file;   // e.g. "sprites/assault_rifle.png", "sprites/door.png"
	int id = 0;         // tile index (Bin type only)
	int width = 0;      // sprite width in pixels (from size: field, 0 if unset)
	int height = 0;     // sprite height in pixels (from size: field, 0 if unset)
	int frameWidth = 0;  // frame dimensions (Sheet type only)
	int frameHeight = 0;
	int frameCount = 0;  // number of animation frames (Sheet type only)
	std::vector<std::string> tags;  // editor-facing labels (e.g. "wall", "metal")
};

// Render properties for sprites with special rendering behavior (parsed from sprites.yaml render: blocks)
struct SpriteTexAnim {
	int sDivisor = 0;   // time/sDivisor for S scroll (0 = none)
	int tDivisor = 0;   // time/tDivisor for T scroll (0 = none)
	int mask = 0x3FF;
	bool hasAnim() const { return sDivisor != 0 || tDivisor != 0; }
};

struct SpriteGlow {
	int sprite = 0;      // tile index of glow sprite
	int zMult = 0;       // height multiplier (scaled by scaleFactor)
	int renderMode = 0;  // e.g. Render::RENDER_ADD50
	bool hasGlow() const { return sprite != 0; }
};

struct SpriteCompositeLayer {
	int sprite = 0;      // tile index
	int zMult = 0;       // height multiplier
};

struct SpriteRenderProps {
	int zOffsetFloor = 0;        // Z offset in floor rendering path
	int zOffsetWall = 0;         // Z offset in wall rendering path
	bool skip = false;           // skip rendering entirely
	bool multiplyShift = false;  // RENDER_FLAG_MULTYPLYSHIFT
	std::string renderPath;      // "stream" etc., empty = default
	SpriteTexAnim texAnim;
	SpriteGlow glow;
	std::vector<SpriteCompositeLayer> composite;
	int autoAnimPeriod = 0;      // time divisor for auto-animation (0 = none)
	int autoAnimFrames = 0;      // frame count for auto-animation
	bool positionAtView = false; // nudge sprite to player viewpoint
	int viewOffsetMult = 0;
	int viewZOffset = 0;
	bool lootAura = false;       // render loot glow aura
	int lootAuraScale = 0;
	int lootAuraFlags = 0;
};

class SpriteDefs {
  public:
	static constexpr int SPRITE_INDEX_EXTERNAL = -1;

	// Name-to-index map (populated from sprites.yaml "sprites" section)
	// External sprites (Image/Sheet) are mapped to SPRITE_INDEX_EXTERNAL (-1)
	static std::unordered_map<std::string, int, StringHash, std::equal_to<>> tileNameToIndex;

	// Index-to-name reverse map (for debugging/logging)
	static std::unordered_map<int, std::string> tileIndexToName;

	// Full source metadata for each tile name
	static std::unordered_map<std::string, SpriteSource, StringHash, std::equal_to<>> tileNameToSource;

	// Tile index → PNG override path (for binary sprites with png: field)
	static std::unordered_map<int, std::string> tileIndexToPng;

	// Range boundaries (populated from sprites.yaml "ranges" section)
	static std::unordered_map<std::string, int, StringHash, std::equal_to<>> ranges;

	// Per-tile render properties (populated from sprites.yaml render: blocks)
	static std::unordered_map<int, SpriteRenderProps> renderProps;

	// Parse sprite definitions from a DataNode (called by ResourceManager)
	[[nodiscard]] static std::expected<void, std::string> parse(const class DataNode& config);

	// Get tile index by name, returns 0 if not found (returns -1 for external sprites)
	[[nodiscard]] static int getIndex(std::string_view name);

	// Get full source info by name, returns nullptr if not found
	[[nodiscard]] static const SpriteSource* getSource(std::string_view name);

	// Get full source info by tile index, returns nullptr if not found
	[[nodiscard]] static const SpriteSource* getSource(int tileIndex);

	// Get the registered name for a tile index, or empty string if unknown.
	[[nodiscard]] static const std::string& getName(int tileIndex);

	// Get tags for a tile index, or an empty vector if unknown/untagged.
	[[nodiscard]] static const std::vector<std::string>& getTags(int tileIndex);

	// Check if a sprite is a single image (not from bin, no frames)
	[[nodiscard]] static bool isImage(std::string_view name);

	// Check if a sprite is a sheet (has frame metadata)
	[[nodiscard]] static bool isSheet(std::string_view name);

	// Check if a sprite is external (Image or Sheet, not from bin)
	[[nodiscard]] static bool isExternal(std::string_view name);

	// Get PNG override path for a tile index, returns empty string if none
	[[nodiscard]] static const std::string& getPngOverride(int tileIndex);

	// Get render properties for a tile index, returns nullptr if none
	[[nodiscard]] static const SpriteRenderProps* getRenderProps(int tileIndex);

	// Every sprite layer the engine may draw when rendering `primaryTile`:
	// always the primary itself, plus each composite layer (multi-part monsters
	// like the Revenant or Cyberdemon) and any glow overlay. Each layer carries
	// the same `zMult` offset the renderer uses in `renderSpriteObject` so the
	// editor's entity preview can stack them identically, and the map compiler
	// can register every layer's media in one pass.
	//
	// Returned vector always has at least one entry (the primary), with zMult=0.
	[[nodiscard]] static std::vector<SpriteCompositeLayer> getRenderedLayers(int primaryTile);

	// Get range value by name, returns 0 if not found
	[[nodiscard]] static int getRange(std::string_view name);

	// Check if an index falls within a named range (inclusive)
	[[nodiscard]] static bool isInRange(int index, std::string_view first, std::string_view last);
};
