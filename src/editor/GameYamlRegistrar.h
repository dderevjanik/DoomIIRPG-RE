#pragma once

#include <string>

namespace editor {

struct RegisterResult {
	bool ok = false;
	bool stringsAppended = false;
	int stringsGroupIndex = -1;
	std::string error;
};

// Append `<nextFreeIdx>: <stringsPath>` under game.yaml `strings:`.
// Line-preserving: the rest of the file is passed through verbatim. Skips
// entries that already exist. Levels themselves are auto-discovered from
// levels/*/level.yaml at engine startup, so no levels-section update is
// needed; the mapId/levelDir parameters are retained for log context only.
RegisterResult registerLevelInGameYaml(const std::string& gameYamlPath,
                                       int mapId,
                                       const std::string& levelDir,
                                       const std::string& stringsPath);

}  // namespace editor
