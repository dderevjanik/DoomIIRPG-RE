#ifndef __SPRITEDEFS_H__
#define __SPRITEDEFS_H__

#include <string>
#include <unordered_map>

class SpriteDefs {
  public:
	// Name-to-index map (populated from sprites.yaml "tiles" section)
	static std::unordered_map<std::string, int> tileNameToIndex;

	// Index-to-name reverse map (for debugging/logging)
	static std::unordered_map<int, std::string> tileIndexToName;

	// Range boundaries (populated from sprites.yaml "ranges" section)
	static std::unordered_map<std::string, int> ranges;

	// Load sprite definitions from sprites.yaml
	// Returns true if loaded successfully, false on error
	static bool loadFromYAML(const char* path);

	// Get tile index by name, returns 0 if not found
	static int getIndex(const std::string& name);

	// Get range value by name, returns 0 if not found
	static int getRange(const std::string& name);

	// Check if an index falls within a named range (inclusive)
	static bool isInRange(int index, const std::string& first, const std::string& last);
};

#endif
