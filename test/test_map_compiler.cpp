// Byte-compares MapCompiler output against the Python reference
// (tools/bsp-to-bin.py via tools/gen-test-level.py).
//
// Test cases are paired: each builds the same logical BSPMap in C++ and Python
// and asserts that compileMap() produces the same bytes as the Python pipeline.

#include "MapCompiler.h"
#include "MapProject.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const char* PYTHON = "python3";

std::vector<uint8_t> readFile(const std::string& path) {
	std::ifstream f(path, std::ios::binary);
	if (!f) throw std::runtime_error("cannot open: " + path);
	return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

std::string runPythonRef(const std::string& caseName) {
	fs::path tmp = fs::temp_directory_path() / ("drpg-editor-test-" + caseName + ".bin");
	// Build command: python3 <repo>/tools/gen-test-level.py --case <name> --out <tmp>
	// Assume cwd is the build dir or repo root; test is typically run from repo root.
	std::string cmd = std::string(PYTHON)
	                  + " tools/gen-test-level.py"
	                  + " --case " + caseName
	                  + " --out \"" + tmp.string() + "\"";
	int rc = std::system(cmd.c_str());
	if (rc != 0) throw std::runtime_error("python reference failed for: " + caseName);
	return tmp.string();
}

bool compareBytes(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, const char* label) {
	if (a.size() != b.size()) {
		std::cerr << "[" << label << "] SIZE MISMATCH — cpp=" << a.size()
		          << " py=" << b.size() << "\n";
		// Dump first few differences.
		size_t lim = std::min(a.size(), b.size());
		for (size_t i = 0, ndiff = 0; i < lim && ndiff < 8; ++i) {
			if (a[i] != b[i]) {
				std::cerr << "  first diff @ " << i
				          << ": cpp=0x" << std::hex << int(a[i])
				          << " py=0x"   << int(b[i]) << std::dec << "\n";
				++ndiff;
			}
		}
		return false;
	}
	for (size_t i = 0; i < a.size(); ++i) {
		if (a[i] != b[i]) {
			std::cerr << "[" << label << "] BYTE DIFF @ " << i
			          << ": cpp=0x" << std::hex << int(a[i])
			          << " py=0x"   << int(b[i]) << std::dec << "\n";
			// Dump a few more to help triage.
			size_t dumps = 0;
			for (size_t j = i + 1; j < a.size() && dumps < 7; ++j) {
				if (a[j] != b[j]) {
					std::cerr << "  also @ " << j
					          << ": cpp=0x" << std::hex << int(a[j])
					          << " py=0x"   << int(b[j]) << std::dec << "\n";
					++dumps;
				}
			}
			return false;
		}
	}
	return true;
}

// --- Test cases (must match tools/gen-test-level.py) ---

editor::MapProject makeEmpty() {
	editor::MapProject p;
	p.name = "empty";
	p.spawn = {0, 0, 0};
	return p;
}

editor::MapProject makeSingleRoom() {
	editor::MapProject p;
	p.name = "single_room";
	for (int r = 2; r < 6; ++r)
		for (int c = 2; c < 6; ++c)
			p.blockMap[r * 32 + c] = 0;
	p.spawn = {3, 3, 0};
	return p;
}

editor::MapProject makeTwoRooms() {
	editor::MapProject p;
	p.name = "two_rooms";
	for (int r = 3; r < 7; ++r) {
		for (int c = 2; c < 6; ++c) p.blockMap[r * 32 + c] = 0;
		for (int c = 7; c < 11; ++c) p.blockMap[r * 32 + c] = 0;
	}
	p.blockMap[5 * 32 + 6] = 0;  // door tile
	p.doors.push_back({6, 5, 'H'});
	p.spawn = {3, 4, 0};
	return p;
}

struct Case {
	const char* name;
	editor::MapProject (*make)();
};

editor::MapProject makeMultiRoom() {
	editor::MapProject p;
	p.name = "multi_room";
	auto openRect = [&](int c0, int r0, int c1, int r1) {
		for (int r = r0; r <= r1; ++r)
			for (int c = c0; c <= c1; ++c)
				p.blockMap[r * 32 + c] = 0;
	};
	openRect(2, 3, 5, 6);
	openRect(11, 3, 14, 6);
	openRect(10, 12, 14, 15);
	for (int c = 6; c < 11; ++c) p.blockMap[4 * 32 + c] = 0;
	for (int r = 7; r < 12; ++r) p.blockMap[r * 32 + 12] = 0;

	p.doors.push_back({6,  4, 'H'});
	p.doors.push_back({10, 4, 'H'});
	p.doors.push_back({12, 7, 'V'});
	p.doors.push_back({12, 11, 'V'});

	constexpr int TALL = 56 + 16;
	constexpr int HIGH = 56 + 24;
	for (int r = 3; r < 7; ++r)
		for (int c = 11; c < 15; ++c)
			p.tileCeilByte[editor::tileKey(c, r)] = TALL;
	for (int r = 12; r < 16; ++r)
		for (int c = 10; c < 15; ++c) {
			p.tileFloorByte[editor::tileKey(c, r)] = 64;
			p.tileCeilByte [editor::tileKey(c, r)] = HIGH;
		}
	int i = 0;
	for (int r = 7; r < 12; ++r, ++i) {
		int f = 56 + 2 * i;
		p.tileFloorByte[editor::tileKey(12, r)] = uint8_t(f);
		p.tileCeilByte [editor::tileKey(12, r)] = uint8_t(f + 8);
	}

	p.spawn = {3, 4, 0};
	return p;
}

const Case CASES[] = {
	{ "empty",       makeEmpty       },
	{ "single_room", makeSingleRoom  },
	{ "two_rooms",   makeTwoRooms    },
	{ "multi_room",  makeMultiRoom   },
};

}  // namespace

int main() {
	int passed = 0;
	int failed = 0;

	std::cout << "=== Map Compiler byte-parity tests ===\n\n";

	for (const Case& tc : CASES) {
		std::cout << "[ RUN  ] " << tc.name << "\n";

		try {
			editor::MapProject proj = tc.make();
			editor::CompiledMap compiled = editor::compileMap(proj);

			std::string refPath = runPythonRef(tc.name);
			std::vector<uint8_t> refBytes = readFile(refPath);

			if (compareBytes(compiled.binData, refBytes, tc.name)) {
				std::cout << "[  OK  ] " << tc.name
				          << "  (bytes=" << compiled.binData.size()
				          << " polys=" << compiled.numPolys
				          << " lines=" << compiled.numLines
				          << " nodes=" << compiled.numNodes << ")\n\n";
				++passed;
			} else {
				++failed;
				std::cout << "\n";
			}
		} catch (const std::exception& e) {
			std::cerr << "[ FAIL ] " << tc.name << " threw: " << e.what() << "\n\n";
			++failed;
		}
	}

	std::cout << "=== Results: " << passed << " passed, " << failed << " failed ===\n";
	return failed == 0 ? 0 : 1;
}
