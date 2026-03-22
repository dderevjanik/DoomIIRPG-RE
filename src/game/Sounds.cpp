#include "Sounds.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

// Static member definitions
std::vector<std::string> Sounds::soundFileNames;
std::unordered_map<std::string, int> Sounds::soundNameToIndex;

bool Sounds::loadFromYAML(const char* path) {
	try {
		YAML::Node config = YAML::LoadFile(path);
		YAML::Node sounds = config["sounds"];
		if (!sounds || !sounds.IsSequence()) {
			printf("sounds.yaml: missing or invalid 'sounds' array\n");
			return false;
		}

		soundFileNames.clear();
		soundFileNames.reserve(sounds.size());
		soundNameToIndex.clear();

		for (int i = 0; i < (int)sounds.size(); i++) {
			const auto& entry = sounds[i];
			std::string file;
			std::string name;

			if (entry.IsMap()) {
				file = entry["file"].as<std::string>("");
				name = entry["name"].as<std::string>("");
			} else {
				// Backward compat: plain string entry
				file = entry.as<std::string>("");
			}

			soundFileNames.push_back(file);
			if (!name.empty()) {
				soundNameToIndex[name] = i;
			}
		}

		printf("Sounds: loaded %d sound definitions from %s\n", (int)soundFileNames.size(), path);
		return true;
	} catch (const YAML::Exception& e) {
		printf("sounds.yaml: %s\n", e.what());
		return false;
	}
}

void Sounds::loadDefaults() {
	soundFileNames.clear();
	soundFileNames.reserve(DEFAULT_COUNT);
	soundNameToIndex.clear();
	for (int i = 0; i < DEFAULT_COUNT; i++) {
		soundFileNames.push_back(DEFAULT_FILE_NAMES[i]);
	}
	printf("Sounds: loaded %d default sound definitions\n", DEFAULT_COUNT);
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
