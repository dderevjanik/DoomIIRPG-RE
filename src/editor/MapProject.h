#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Authoring model for the in-engine map editor. Mirrors the Python `BSPMap`
// used by tools/bsp-to-bin.py so the C++ compiler (src/editor/MapCompiler.cpp)
// can produce byte-identical .bin output.
//
// See docs/d2-rpg/LEVEL_AUTHORING.md §1 for the field semantics.

namespace editor {

constexpr int MAP_SIZE = 32;
constexpr int TILE_SIZE = 64;
constexpr int FLOOR_HEIGHT = 56;
constexpr int WALL_HEIGHT_WORLD = 64;

constexpr int D2_WALL_TILE = 258;
constexpr int D2_FLOOR_TILE = 451;
constexpr int D2_CEIL_TILE = 462;

// Door textures (0..10 in D1; we keep the same convention for markers).
constexpr bool isDoorTextureId(int t) { return t >= 0 && t <= 10; }

enum WallDir : uint8_t { DIR_N = 0, DIR_E = 1, DIR_S = 2, DIR_W = 3 };

// Packed (col, row) and (col, row, dir) keys for the unordered_maps.
inline uint16_t tileKey(int col, int row) { return uint16_t((row << 8) | col); }
inline uint32_t wallKey(int col, int row, int dir) {
	return uint32_t((uint32_t(dir) << 16) | (uint32_t(row) << 8) | uint32_t(col));
}

struct Door {
	uint8_t col = 0;
	uint8_t row = 0;
	char axis = 'H';  // 'H' = E/W passage (vertical door line), 'V' = N/S passage (horizontal line)
};

struct Spawn {
	uint8_t col = 0;
	uint8_t row = 0;
	uint8_t dir = 0;  // 0..7; 0=east, 2=north, 4=west, 6=south
};

// Free-form wall segments: endpoints in world coords (0 … MAP_SIZE*TILE_SIZE).
// Unlike tile-boundary walls, these can sit anywhere and at any angle.
// The compiler treats each line as a vertical quad spanning the owner tile's
// floor→ceil Z range, plus a collision line in byte space.
struct FreeLine {
	int x0 = 0, y0 = 0;
	int x1 = 0, y1 = 0;
	int texture = D2_WALL_TILE;   // override defaults to D2_WALL_TILE
};

// Placed entity — emitted into level.yaml `entities:` block. Position is a
// tile cell; compiler writes the tile centre as the world coord. Door
// entities still live in `doors` (separate list) because they carry collision
// semantics; this struct is for everything else (monsters / items / NPCs /
// decorations).
struct Entity {
	uint8_t col = 0;
	uint8_t row = 0;
	std::string tile;                     // sprite name — resolved via SpriteDefs at load
	int      tileId = 0;                  // resolved sprite ID (for media registration); 0 = unresolved
	int      z = -1;                      // <0 ⇒ not a Z-sprite; >=0 ⇒ emitted with `z:` field
	std::vector<std::string> flags;       // names: animation, flip_h, north, south, east, west, …
};

struct MapProject {
	std::string name;
	int mapId = 11;
	std::string skyTexture = "textures/sky_earth.png";

	// 32×32 tile grid row-major. Bit 0 = 1 solid, 0 = open.
	std::array<uint8_t, 1024> blockMap{};

	// Sparse per-tile overrides. Missing keys use defaults.
	std::unordered_map<uint16_t, uint8_t> tileFloorByte;   // default FLOOR_HEIGHT
	std::unordered_map<uint16_t, uint8_t> tileCeilByte;    // default FLOOR_HEIGHT + 8
	std::unordered_map<uint16_t, int>     tileFloorTex;    // default D2_FLOOR_TILE
	std::unordered_map<uint16_t, int>     tileCeilTex;     // default D2_CEIL_TILE
	std::unordered_map<uint32_t, int>     tileWallTex;     // default D2_WALL_TILE

	std::vector<Door> doors;
	std::vector<FreeLine> freeLines;
	std::vector<Entity> entities;
	Spawn spawn{};

	// Defaults: all tiles solid, spawn at (0,0) facing east.
	MapProject();

	// --- Lookups ---
	bool isSolid(int col, int row) const;
	bool isDoorTile(int col, int row) const;  // matches Python's ±TILE_SIZE tolerance
	int  floorByte(int col, int row) const;
	int  ceilByte(int col, int row) const;
	int  floorTex(int col, int row) const;
	int  ceilTex(int col, int row) const;
	int  wallTex(int col, int row, int dir) const;

	// --- YAML I/O (Phase 2 — defined in MapProject.cpp as stubs for now) ---
	static MapProject loadFromYaml(const std::string& path);
	void saveToYaml(const std::string& path) const;
};

}  // namespace editor
