#include "Sounds.h"
#include "INIReader.h"
#include <cstdio>

// Static member definition
std::vector<std::string> Sounds::soundFileNames;

bool Sounds::loadFromINI(const char* path) {
	INIReader ini;

	if (!ini.load(path)) {
		return false;
	}

	int count = ini.getInt("Sounds", "count", 0);
	if (count <= 0) {
		printf("sounds.ini: invalid count\n");
		return false;
	}

	soundFileNames.clear();
	soundFileNames.reserve(count + 1);

	for (int i = 0; i <= count; i++) {
		char section[32];
		snprintf(section, sizeof(section), "Sound_%d", i);

		std::string file = ini.getString(section, "file", "");
		if (file.empty()) {
			printf("sounds.ini: missing file for %s\n", section);
			soundFileNames.push_back("");
		} else {
			soundFileNames.push_back(file);
		}
	}

	printf("Sounds: loaded %d sound definitions from %s\n", (int)soundFileNames.size(), path);
	return true;
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
