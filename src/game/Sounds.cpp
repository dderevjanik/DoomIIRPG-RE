#include "Sounds.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

// Static member definition
std::vector<std::string> Sounds::soundFileNames;

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

		for (const auto& entry : sounds) {
			soundFileNames.push_back(entry.as<std::string>(""));
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

int Sounds::getCount() {
	return (int)soundFileNames.size();
}
