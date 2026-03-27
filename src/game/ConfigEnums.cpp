#include "ConfigEnums.h"
#include "DataNode.h"
#include <cstdio>

// Static member definitions
std::unordered_map<std::string, int> ConfigEnums::difficulty;
std::unordered_map<std::string, int> ConfigEnums::language;
std::unordered_map<std::string, int> ConfigEnums::windowMode;
std::unordered_map<std::string, int> ConfigEnums::controlLayout;
std::unordered_map<int, std::string> ConfigEnums::difficultyByValue;
std::unordered_map<int, std::string> ConfigEnums::languageByValue;
std::unordered_map<int, std::string> ConfigEnums::windowModeByValue;
std::unordered_map<int, std::string> ConfigEnums::controlLayoutByValue;

static void loadSection(const DataNode& config, const char* key,
						std::unordered_map<std::string, int>& map,
						std::unordered_map<int, std::string>& reverseMap) {
	map.clear();
	reverseMap.clear();
	DataNode section = config[key];
	if (section && section.isMap()) {
		for (auto it = section.begin(); it != section.end(); ++it) {
			std::string name = it.key().asString();
			int value = it.value().asInt(0);
			map[name] = value;
			reverseMap[value] = name;
		}
	}
}

bool ConfigEnums::parse(const DataNode& root) {
	DataNode config = root["config_enums"];
	if (!config || !config.isMap()) {
		printf("[config_enums] no config_enums section\n");
		return false;
	}

	loadSection(config, "difficulty", ConfigEnums::difficulty, ConfigEnums::difficultyByValue);
	loadSection(config, "language", ConfigEnums::language, ConfigEnums::languageByValue);
	loadSection(config, "window_mode", ConfigEnums::windowMode, ConfigEnums::windowModeByValue);
	loadSection(config, "control_layout", ConfigEnums::controlLayout, ConfigEnums::controlLayoutByValue);

	printf("[config_enums] loaded\n");
	return true;
}

static std::string lookupByValue(const std::unordered_map<int, std::string>& map,
								 int value, const char* fallback) {
	auto it = map.find(value);
	if (it != map.end()) return it->second;
	return fallback;
}

static int lookupByName(const std::unordered_map<std::string, int>& map,
						const std::string& name, int fallback) {
	auto it = map.find(name);
	if (it != map.end()) return it->second;
	return fallback;
}

int ConfigEnums::difficultyFromString(const std::string& name) {
	return lookupByName(difficulty, name, 1);
}

std::string ConfigEnums::difficultyToString(int value) {
	return lookupByValue(difficultyByValue, value, "normal");
}

int ConfigEnums::languageFromString(const std::string& name) {
	return lookupByName(language, name, 0);
}

std::string ConfigEnums::languageToString(int value) {
	return lookupByValue(languageByValue, value, "english");
}

int ConfigEnums::windowModeFromString(const std::string& name) {
	return lookupByName(windowMode, name, 0);
}

std::string ConfigEnums::windowModeToString(int value) {
	return lookupByValue(windowModeByValue, value, "windowed");
}

int ConfigEnums::controlLayoutFromString(const std::string& name) {
	return lookupByName(controlLayout, name, 0);
}

std::string ConfigEnums::controlLayoutToString(int value) {
	return lookupByValue(controlLayoutByValue, value, "chevrons");
}
