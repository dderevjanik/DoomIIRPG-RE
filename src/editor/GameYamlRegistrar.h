#pragma once

#include <string>

namespace editor {

struct RegisterResult {
	bool ok = false;
	bool levelAppended = false;
	bool stringsAppended = false;
	int stringsGroupIndex = -1;
	std::string error;
};

// Append `<mapId>: <levelDir>` under game.yaml `levels:` and `<nextFreeIdx>:
// <stringsPath>` under `strings:`. Line-preserving: the rest of the file is
// passed through verbatim. Skips entries that already exist.
RegisterResult registerLevelInGameYaml(const std::string& gameYamlPath,
                                       int mapId,
                                       const std::string& levelDir,
                                       const std::string& stringsPath);

}  // namespace editor
