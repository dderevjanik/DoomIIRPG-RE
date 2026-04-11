#include "Sounds.h"
#include "DataNode.h"
#include "Log.h"

// Static member definitions
std::vector<std::string> Sounds::soundFileNames;
std::unordered_map<std::string, int> Sounds::soundNameToIndex;

bool Sounds::parse(const DataNode& config) {
	DataNode sounds = config["sounds"];
	if (!sounds || !sounds.isMap()) {
		LOG_ERROR("[sounds] missing or invalid 'sounds' map\n");
		return false;
	}

	Sounds::soundFileNames.clear();
	Sounds::soundFileNames.reserve(sounds.size());
	Sounds::soundNameToIndex.clear();

	int i = 0;
	for (auto it = sounds.begin(); it != sounds.end(); ++it, ++i) {
		std::string name = it.key().asString();
		std::string file;

		if (it.value().isMap()) {
			file = it.value()["file"].asString("");
		} else if (it.value().isScalar()) {
			file = it.value().asString("");
		}

		Sounds::soundFileNames.push_back(file);
		if (!name.empty()) {
			Sounds::soundNameToIndex[name] = i;
		}
	}

	LOG_INFO("[sounds] loaded %d sound definitions\n", (int)Sounds::soundFileNames.size());
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
