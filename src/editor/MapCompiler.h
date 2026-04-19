#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace editor {

struct MapProject;

struct CompiledMap {
	std::vector<uint8_t> binData;   // the .bin file bytes
	std::string levelYaml;           // level.yaml contents (rendered string)
	int numPolys = 0;
	int numLines = 0;
	int numNodes = 0;
	int numLeaves = 0;
	// Set of texture IDs referenced by any poly (includes the 3 hardcoded
	// defaults). If two consecutive compiles have the same `mediaSet`, the
	// engine can skip media tear-down + rebuild and do a geometry-only
	// hot reload.
	std::set<int> mediaSet;
};

// C++ port of tools/bsp-to-bin.py. Must produce byte-identical output for the
// same input. See docs/d2-rpg/BSP_COMPILER_NOTES.md for the algorithm.
CompiledMap compileMap(const MapProject& project);

}  // namespace editor
