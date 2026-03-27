#ifndef __SPRITEDEFS_H__
#define __SPRITEDEFS_H__

#include <string>
#include <unordered_map>

enum class SpriteSourceType { Bin, Png };

struct SpriteSource {
	SpriteSourceType type = SpriteSourceType::Bin;
	std::string file;   // e.g. "tables.bin" or "chainsaw.png"
	int id = 0;         // tile index (bin type only)
};

class SpriteDefs {
  public:
	static constexpr int SPRITE_INDEX_EXTERNAL = -1;

	// Name-to-index map (populated from sprites.yaml "tiles" section)
	// PNG sprites are mapped to SPRITE_INDEX_EXTERNAL (-1)
	static std::unordered_map<std::string, int> tileNameToIndex;

	// Index-to-name reverse map (for debugging/logging)
	static std::unordered_map<int, std::string> tileIndexToName;

	// Full source metadata for each tile name
	static std::unordered_map<std::string, SpriteSource> tileNameToSource;

	// Range boundaries (populated from sprites.yaml "ranges" section)
	static std::unordered_map<std::string, int> ranges;

	// Parse sprite definitions from a DataNode (called by ResourceManager)
	static bool parse(const class DataNode& config);

	// Get tile index by name, returns 0 if not found (returns -1 for PNG sprites)
	static int getIndex(const std::string& name);

	// Get full source info by name, returns nullptr if not found
	static const SpriteSource* getSource(const std::string& name);

	// Check if a sprite is a PNG source
	static bool isPng(const std::string& name);

	// Get range value by name, returns 0 if not found
	static int getRange(const std::string& name);

	// Check if an index falls within a named range (inclusive)
	static bool isInRange(int index, const std::string& first, const std::string& last);
};

#endif
