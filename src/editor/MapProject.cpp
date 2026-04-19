#include "MapProject.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace editor {

MapProject::MapProject() {
	// All tiles solid by default (bit 0 = 1).
	blockMap.fill(1);
}

bool MapProject::isSolid(int col, int row) const {
	if (col < 0 || col >= MAP_SIZE || row < 0 || row >= MAP_SIZE) return true;
	return (blockMap[row * MAP_SIZE + col] & 1) != 0;
}

// Matches Python's `is_door_tile` tolerance: a door line counts for any tile
// whose centre is within ±TILE_SIZE of the door line's midpoint.
bool MapProject::isDoorTile(int col, int row) const {
	const int cx = col * TILE_SIZE + TILE_SIZE / 2;
	const int cy = row * TILE_SIZE + TILE_SIZE / 2;
	for (const Door& d : doors) {
		int mx, my;
		if (d.axis == 'H') {
			// E/W passage — vertical line at tile x centre, spans one tile in Y.
			mx = d.col * TILE_SIZE + TILE_SIZE / 2;
			my = d.row * TILE_SIZE + TILE_SIZE / 2;
		} else {
			// N/S passage — horizontal line at tile y centre, spans one tile in X.
			mx = d.col * TILE_SIZE + TILE_SIZE / 2;
			my = d.row * TILE_SIZE + TILE_SIZE / 2;
		}
		if (std::abs(mx - cx) <= TILE_SIZE && std::abs(my - cy) <= TILE_SIZE) {
			return true;
		}
	}
	return false;
}

int MapProject::floorByte(int col, int row) const {
	auto it = tileFloorByte.find(tileKey(col, row));
	return it == tileFloorByte.end() ? FLOOR_HEIGHT : it->second;
}

int MapProject::ceilByte(int col, int row) const {
	auto it = tileCeilByte.find(tileKey(col, row));
	return it == tileCeilByte.end() ? (FLOOR_HEIGHT + 8) : it->second;
}

int MapProject::floorTex(int col, int row) const {
	auto it = tileFloorTex.find(tileKey(col, row));
	return it == tileFloorTex.end() ? D2_FLOOR_TILE : it->second;
}

int MapProject::ceilTex(int col, int row) const {
	auto it = tileCeilTex.find(tileKey(col, row));
	return it == tileCeilTex.end() ? D2_CEIL_TILE : it->second;
}

int MapProject::wallTex(int col, int row, int dir) const {
	auto it = tileWallTex.find(wallKey(col, row, dir));
	return it == tileWallTex.end() ? D2_WALL_TILE : it->second;
}

// --- YAML I/O ---
// Schema: see docs/d2-rpg/LEVEL_AUTHORING.md §5.
namespace {

int dirFromName(const std::string& name) {
	if (name == "east")      return 0;
	if (name == "northeast") return 1;
	if (name == "north")     return 2;
	if (name == "northwest") return 3;
	if (name == "west")      return 4;
	if (name == "southwest") return 5;
	if (name == "south")     return 6;
	if (name == "southeast") return 7;
	return 0;
}

const char* dirName(int d) {
	static const char* names[] = {
		"east", "northeast", "north", "northwest",
		"west", "southwest", "south", "southeast"
	};
	return names[d & 7];
}

// "col,row" and "col,row,dir" key parsers.
bool parseTileKey(const std::string& s, int& col, int& row) {
	return std::sscanf(s.c_str(), "%d,%d", &col, &row) == 2;
}
bool parseWallKey(const std::string& s, int& col, int& row, int& dir) {
	char dc = 0;
	if (std::sscanf(s.c_str(), "%d,%d,%c", &col, &row, &dc) != 3) return false;
	switch (dc) {
		case 'N': case 'n': dir = DIR_N; return true;
		case 'E': case 'e': dir = DIR_E; return true;
		case 'S': case 's': dir = DIR_S; return true;
		case 'W': case 'w': dir = DIR_W; return true;
	}
	return false;
}

}  // namespace

MapProject MapProject::loadFromYaml(const std::string& path) {
	YAML::Node doc = YAML::LoadFile(path);
	MapProject p;

	if (doc["name"])        p.name        = doc["name"].as<std::string>();
	if (doc["map_id"])      p.mapId       = doc["map_id"].as<int>();
	if (doc["sky_texture"]) p.skyTexture  = doc["sky_texture"].as<std::string>();

	if (doc["spawn"]) {
		auto s = doc["spawn"];
		p.spawn.col = uint8_t(s["col"].as<int>(0));
		p.spawn.row = uint8_t(s["row"].as<int>(0));
		if (s["dir"].IsScalar()) {
			std::string d = s["dir"].as<std::string>();
			// Numeric direction (0..7) or name.
			if (!d.empty() && std::isdigit(static_cast<unsigned char>(d[0]))) {
				p.spawn.dir = uint8_t(std::stoi(d));
			} else {
				p.spawn.dir = uint8_t(dirFromName(d));
			}
		}
	}

	if (doc["block_map"]) {
		auto bm = doc["block_map"];
		for (int r = 0; r < MAP_SIZE && r < int(bm.size()); ++r) {
			const std::string row = bm[r].as<std::string>();
			for (int c = 0; c < MAP_SIZE && c < int(row.size()); ++c) {
				p.blockMap[r * MAP_SIZE + c] = (row[c] == '#') ? 1 : 0;
			}
		}
	}

	auto loadTileDict = [](const YAML::Node& node, std::unordered_map<uint16_t, uint8_t>& dst) {
		if (!node) return;
		for (auto it = node.begin(); it != node.end(); ++it) {
			int col, row;
			if (!parseTileKey(it->first.as<std::string>(), col, row)) continue;
			dst[tileKey(col, row)] = uint8_t(it->second.as<int>());
		}
	};
	auto loadTileTexDict = [](const YAML::Node& node, std::unordered_map<uint16_t, int>& dst) {
		if (!node) return;
		for (auto it = node.begin(); it != node.end(); ++it) {
			int col, row;
			if (!parseTileKey(it->first.as<std::string>(), col, row)) continue;
			dst[tileKey(col, row)] = it->second.as<int>();
		}
	};

	loadTileDict(doc["floor_byte"], p.tileFloorByte);
	loadTileDict(doc["ceil_byte"],  p.tileCeilByte);
	loadTileTexDict(doc["floor_tex"], p.tileFloorTex);
	loadTileTexDict(doc["ceil_tex"],  p.tileCeilTex);

	if (doc["wall_tex"]) {
		auto n = doc["wall_tex"];
		for (auto it = n.begin(); it != n.end(); ++it) {
			int col, row, dir;
			if (!parseWallKey(it->first.as<std::string>(), col, row, dir)) continue;
			p.tileWallTex[wallKey(col, row, dir)] = it->second.as<int>();
		}
	}

	if (doc["doors"]) {
		for (const auto& d : doc["doors"]) {
			Door door;
			door.col = uint8_t(d["col"].as<int>(0));
			door.row = uint8_t(d["row"].as<int>(0));
			std::string ax = d["axis"].as<std::string>("EW");
			if (ax == "EW" || ax == "H") door.axis = 'H';
			else                         door.axis = 'V';
			p.doors.push_back(door);
		}
	}

	return p;
}

void MapProject::saveToYaml(const std::string& path) const {
	std::ostringstream os;
	os << "# Level project for DRPG editor\n";
	os << "name: " << name << "\n";
	os << "map_id: " << mapId << "\n";
	os << "sky_texture: " << skyTexture << "\n\n";
	os << "spawn: { col: " << int(spawn.col)
	   << ", row: " << int(spawn.row)
	   << ", dir: " << dirName(spawn.dir) << " }\n\n";

	// Compact block_map: 32 strings of 32 chars.
	os << "block_map:\n";
	for (int r = 0; r < MAP_SIZE; ++r) {
		os << "  - \"";
		for (int c = 0; c < MAP_SIZE; ++c) {
			os << ((blockMap[r * MAP_SIZE + c] & 1) ? '#' : '.');
		}
		os << "\"\n";
	}
	os << "\n";

	auto writeTileDict = [&](const char* key, const std::unordered_map<uint16_t, uint8_t>& m) {
		os << key << ":";
		if (m.empty()) { os << " {}\n"; return; }
		os << "\n";
		// Sort for deterministic output
		std::vector<std::pair<int,int>> keys;
		for (auto& kv : m) keys.emplace_back(kv.first & 0xFF, (kv.first >> 8) & 0xFF);
		std::sort(keys.begin(), keys.end(), [](auto& a, auto& b){
			return a.second == b.second ? a.first < b.first : a.second < b.second;
		});
		for (auto& [c, r] : keys) {
			os << "  \"" << c << "," << r << "\": " << int(m.at(tileKey(c, r))) << "\n";
		}
	};
	auto writeTileTexDict = [&](const char* key, const std::unordered_map<uint16_t, int>& m) {
		os << key << ":";
		if (m.empty()) { os << " {}\n"; return; }
		os << "\n";
		std::vector<std::pair<int,int>> keys;
		for (auto& kv : m) keys.emplace_back(kv.first & 0xFF, (kv.first >> 8) & 0xFF);
		std::sort(keys.begin(), keys.end(), [](auto& a, auto& b){
			return a.second == b.second ? a.first < b.first : a.second < b.second;
		});
		for (auto& [c, r] : keys) {
			os << "  \"" << c << "," << r << "\": " << m.at(tileKey(c, r)) << "\n";
		}
	};

	writeTileDict("floor_byte", tileFloorByte);
	writeTileDict("ceil_byte",  tileCeilByte);
	writeTileTexDict("floor_tex", tileFloorTex);
	writeTileTexDict("ceil_tex",  tileCeilTex);

	os << "wall_tex:";
	if (tileWallTex.empty()) { os << " {}\n"; }
	else {
		os << "\n";
		struct WK { int col, row, dir; };
		std::vector<WK> keys;
		for (auto& kv : tileWallTex) {
			keys.push_back({ int(kv.first & 0xFF),
			                 int((kv.first >> 8) & 0xFF),
			                 int((kv.first >> 16) & 0xFF) });
		}
		std::sort(keys.begin(), keys.end(), [](auto& a, auto& b){
			if (a.row != b.row) return a.row < b.row;
			if (a.col != b.col) return a.col < b.col;
			return a.dir < b.dir;
		});
		const char dirc[] = {'N','E','S','W'};
		for (auto& k : keys) {
			os << "  \"" << k.col << "," << k.row << "," << dirc[k.dir] << "\": "
			   << tileWallTex.at(wallKey(k.col, k.row, k.dir)) << "\n";
		}
	}

	os << "\ndoors:";
	if (doors.empty()) { os << " []\n"; }
	else {
		os << "\n";
		for (const Door& d : doors) {
			os << "  - { col: " << int(d.col)
			   << ", row: " << int(d.row)
			   << ", axis: " << (d.axis == 'V' ? "NS" : "EW") << " }\n";
		}
	}

	std::ofstream f(path);
	if (!f) throw std::runtime_error("cannot write: " + path);
	f << os.str();
}

}  // namespace editor
