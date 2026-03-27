#include "Sounds.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

// Static member definitions
std::vector<std::string> Sounds::soundFileNames;
std::unordered_map<std::string, int> Sounds::soundNameToIndex;

bool Sounds::loadFromYAML(const char* path) {
	try {
		YAML::Node config = YAML::LoadFile(path);
		return loadFromNode(config);
	} catch (const YAML::Exception& e) {
		printf("[sounds] %s: %s\n", path, e.what());
		return false;
	}
}

bool Sounds::loadFromNode(const YAML::Node& config) {
	YAML::Node sounds = config["sounds"];
	if (!sounds || !sounds.IsMap()) {
		printf("[sounds] missing or invalid 'sounds' map\n");
		return false;
	}

	soundFileNames.clear();
	soundFileNames.reserve(sounds.size());
	soundNameToIndex.clear();

	int i = 0;
	for (auto it = sounds.begin(); it != sounds.end(); ++it, ++i) {
		std::string name = it->first.as<std::string>();
		std::string file;

		if (it->second.IsMap()) {
			file = it->second["file"].as<std::string>("");
		} else if (it->second.IsScalar()) {
			file = it->second.as<std::string>("");
		}

		soundFileNames.push_back(file);
		if (!name.empty()) {
			soundNameToIndex[name] = i;
		}
	}

	printf("[sounds] loaded %d sound definitions\n", (int)soundFileNames.size());
	return true;
}


const char* Sounds::getFileName(int index) {
	if (index < 0 || index >= (int)soundFileNames.size()) {
		return nullptr;
	}
	return soundFileNames[index].c_str();
}

int Sounds::getIndexByName(const std::string& name) {
	auto it = soundNameToIndex.find(name);
	if (it != soundNameToIndex.end()) {
		return it->second;
	}
	return -1;
}

int Sounds::getResIDByName(const std::string& name) {
	int index = getIndexByName(name);
	if (index >= 0) {
		return index + 1000;
	}
	return -1;
}

int Sounds::getCount() {
	return (int)soundFileNames.size();
}
