#include "INIReader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

INIReader::INIReader() {
}

INIReader::~INIReader() {
}

std::string INIReader::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

bool INIReader::load(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    clear();

    std::string currentSection;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        // Check for section header
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            currentSection = trim(currentSection);

            // Add to section order if not already present
            if (data.find(currentSection) == data.end()) {
                sectionOrder.push_back(currentSection);
                data[currentSection] = std::map<std::string, std::string>();
            }
            continue;
        }

        // Parse key=value
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos && !currentSection.empty()) {
            std::string key = trim(line.substr(0, eqPos));
            std::string value = trim(line.substr(eqPos + 1));
            data[currentSection][key] = value;
        }
    }

    file.close();
    return true;
}

bool INIReader::save(const char* filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // Write sections in order
    for (const auto& sectionName : sectionOrder) {
        auto sectionIt = data.find(sectionName);
        if (sectionIt == data.end()) {
            continue;
        }

        file << "[" << sectionName << "]\n";

        for (const auto& pair : sectionIt->second) {
            file << pair.first << " = " << pair.second << "\n";
        }

        file << "\n";
    }

    file.close();
    return true;
}

void INIReader::clear() {
    data.clear();
    sectionOrder.clear();
}

bool INIReader::hasSection(const char* section) const {
    return data.find(section) != data.end();
}

bool INIReader::hasKey(const char* section, const char* key) const {
    auto sectionIt = data.find(section);
    if (sectionIt == data.end()) {
        return false;
    }
    return sectionIt->second.find(key) != sectionIt->second.end();
}

int INIReader::getInt(const char* section, const char* key, int defaultVal) const {
    auto sectionIt = data.find(section);
    if (sectionIt == data.end()) {
        return defaultVal;
    }

    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) {
        return defaultVal;
    }

    return std::atoi(keyIt->second.c_str());
}

bool INIReader::getBool(const char* section, const char* key, bool defaultVal) const {
    auto sectionIt = data.find(section);
    if (sectionIt == data.end()) {
        return defaultVal;
    }

    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) {
        return defaultVal;
    }

    std::string val = keyIt->second;
    // Convert to lowercase for comparison
    std::transform(val.begin(), val.end(), val.begin(), ::tolower);

    return (val == "true" || val == "1" || val == "yes" || val == "on");
}

std::string INIReader::getString(const char* section, const char* key, const char* defaultVal) const {
    auto sectionIt = data.find(section);
    if (sectionIt == data.end()) {
        return defaultVal;
    }

    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) {
        return defaultVal;
    }

    return keyIt->second;
}

std::map<std::string, std::string>& INIReader::getOrCreateSection(const char* section) {
    if (data.find(section) == data.end()) {
        sectionOrder.push_back(section);
        data[section] = std::map<std::string, std::string>();
    }
    return data[section];
}

void INIReader::setInt(const char* section, const char* key, int value) {
    getOrCreateSection(section)[key] = std::to_string(value);
}

void INIReader::setBool(const char* section, const char* key, bool value) {
    getOrCreateSection(section)[key] = value ? "true" : "false";
}

void INIReader::setString(const char* section, const char* key, const char* value) {
    getOrCreateSection(section)[key] = value;
}

std::vector<int> INIReader::getIntArray(const char* section, const char* key, int count, int defaultVal) const {
    std::vector<int> result(count, defaultVal);

    auto sectionIt = data.find(section);
    if (sectionIt == data.end()) {
        return result;
    }

    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) {
        return result;
    }

    std::stringstream ss(keyIt->second);
    std::string item;
    int index = 0;

    while (std::getline(ss, item, ',') && index < count) {
        result[index++] = std::atoi(trim(item).c_str());
    }

    return result;
}

void INIReader::setIntArray(const char* section, const char* key, const int* values, int count) {
    std::stringstream ss;
    for (int i = 0; i < count; i++) {
        if (i > 0) {
            ss << ",";
        }
        ss << values[i];
    }
    getOrCreateSection(section)[key] = ss.str();
}
