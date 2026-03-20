#ifndef __INIREADER_H__
#define __INIREADER_H__

#include <string>
#include <map>
#include <vector>

class INIReader {
public:
    INIReader();
    ~INIReader();

    // Load INI file from disk
    bool load(const char* filename);

    // Save INI file to disk
    bool save(const char* filename);

    // Clear all data
    void clear();

    // Check if a section exists
    bool hasSection(const char* section) const;

    // Check if a key exists in a section
    bool hasKey(const char* section, const char* key) const;

    // Getters with default values
    int getInt(const char* section, const char* key, int defaultVal) const;
    bool getBool(const char* section, const char* key, bool defaultVal) const;
    std::string getString(const char* section, const char* key, const char* defaultVal) const;

    // Setters
    void setInt(const char* section, const char* key, int value);
    void setBool(const char* section, const char* key, bool value);
    void setString(const char* section, const char* key, const char* value);

    // Get comma-separated integers (for key bindings)
    std::vector<int> getIntArray(const char* section, const char* key, int count, int defaultVal) const;
    void setIntArray(const char* section, const char* key, const int* values, int count);

    // Get comma-separated strings (for human-readable key bindings)
    std::vector<std::string> getStringArray(const char* section, const char* key, int count, const char* defaultVal) const;
    void setStringArray(const char* section, const char* key, const std::vector<std::string>& values);

private:
    // Section -> (Key -> Value)
    std::map<std::string, std::map<std::string, std::string>> data;

    // Ordered list of sections (to preserve order when saving)
    std::vector<std::string> sectionOrder;

    // Helper to trim whitespace
    static std::string trim(const std::string& str);

    // Helper to get or create section
    std::map<std::string, std::string>& getOrCreateSection(const char* section);
};

#endif
