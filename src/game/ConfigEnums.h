#ifndef __CONFIGENUMS_H__
#define __CONFIGENUMS_H__

#include <string>
#include <unordered_map>

class ConfigEnums {
  public:
	static std::unordered_map<std::string, int> difficulty;
	static std::unordered_map<std::string, int> language;
	static std::unordered_map<std::string, int> windowMode;
	static std::unordered_map<std::string, int> controlLayout;

	// Reverse maps (value → name)
	static std::unordered_map<int, std::string> difficultyByValue;
	static std::unordered_map<int, std::string> languageByValue;
	static std::unordered_map<int, std::string> windowModeByValue;
	static std::unordered_map<int, std::string> controlLayoutByValue;

	static bool loadFromYAML(const char* path);

	static int difficultyFromString(const std::string& name);
	static std::string difficultyToString(int value);

	static int languageFromString(const std::string& name);
	static std::string languageToString(int value);

	static int windowModeFromString(const std::string& name);
	static std::string windowModeToString(int value);

	static int controlLayoutFromString(const std::string& name);
	static std::string controlLayoutToString(int value);
};

#endif
