#include "ConfigEnums.h"
#include <yaml-cpp/yaml.h>
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

static void loadSection(const YAML::Node& config, const char* key,
						std::unordered_map<std::string, int>& map,
						std::unordered_map<int, std::string>& reverseMap) {
	map.clear();
	reverseMap.clear();
	YAML::Node section = config[key];
	if (section && section.IsMap()) {
		for (auto it = section.begin(); it != section.end(); ++it) {
			std::string name = it->first.as<std::string>();
			int value = it->second.as<int>(0);
			map[name] = value;
			reverseMap[value] = name;
		}
	}
}

bool ConfigEnums::loadFromYAML(const char* path) {
	try {
		YAML::Node root = YAML::LoadFile(path);
		return loadFromNode(root);
	} catch (const YAML::Exception& e) {
		printf("[config_enums] %s: %s\n", path, e.what());
		return false;
	}
}

bool ConfigEnums::loadFromNode(const YAML::Node& root) {
	YAML::Node config = root["config_enums"];
	if (!config || !config.IsMap()) {
		printf("[config_enums] no config_enums section\n");
		return false;
	}

	loadSection(config, "difficulty", difficulty, difficultyByValue);
	loadSection(config, "language", language, languageByValue);
	loadSection(config, "window_mode", windowMode, windowModeByValue);
	loadSection(config, "control_layout", controlLayout, controlLayoutByValue);

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
