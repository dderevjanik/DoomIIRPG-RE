#ifndef __SOUNDS_H__
#define __SOUNDS_H__

#include <string>
#include <unordered_map>
#include <vector>

namespace YAML { class Node; }

class Sounds {
  public:
	// Runtime-loaded sound filenames (populated from sounds.yaml)
	static std::vector<std::string> soundFileNames;

	// Name-to-index lookup map (populated from sounds.yaml)
	static std::unordered_map<std::string, int> soundNameToIndex;

	// Load sound definitions from sounds.yaml
	// Returns true if loaded successfully, false on error
	static bool loadFromYAML(const char* path);

	// Load sound definitions from a pre-parsed YAML node
	static bool loadFromNode(const YAML::Node& config);

	// Get filename for a sound index, with bounds checking
	static const char* getFileName(int index);

	// Get sound index by name, returns -1 if not found
	static int getIndexByName(const std::string& name);

	// Get resource ID (index + 1000) by name, returns -1 if not found
	static int getResIDByName(const std::string& name);

	// Get total number of loaded sounds
	static int getCount();

};

#endif
