#pragma once
#include <string>
#include <memory>

// Forward declaration — DataNode wraps YAML::Node without exposing yaml-cpp.
// Only DataNode.cpp and ResourceManager.cpp include <yaml-cpp/yaml.h>.
namespace YAML { class Node; }

class DataNode {
public:
	DataNode();
	~DataNode();
	DataNode(const DataNode& other);
	DataNode& operator=(const DataNode& other);
	DataNode(DataNode&& other) noexcept;
	DataNode& operator=(DataNode&& other) noexcept;

	// Value access with defaults
	[[nodiscard]] int asInt(int def = 0) const;
	[[nodiscard]] long long asLongLong(long long def = 0) const;
	[[nodiscard]] unsigned long asULong(unsigned long def = 0) const;
	[[nodiscard]] float asFloat(float def = 0.0f) const;
	[[nodiscard]] bool asBool(bool def = false) const;
	[[nodiscard]] std::string asString(const std::string& def = "") const;

	// Subscript (returns empty/null node if key missing)
	[[nodiscard]] DataNode operator[](const char* key) const;
	[[nodiscard]] DataNode operator[](const std::string& key) const;
	[[nodiscard]] DataNode operator[](int index) const;

	// Type checks
	explicit operator bool() const;
	[[nodiscard]] bool isMap() const;
	[[nodiscard]] bool isScalar() const;
	[[nodiscard]] bool isSequence() const;
	[[nodiscard]] int size() const;

	// Deep copy
	[[nodiscard]] DataNode clone() const;

	// Merge: clone base, overlay all keys from other on top (skips a given key)
	[[nodiscard]] static DataNode merge(const DataNode& base, const DataNode& overlay, const char* skipKey = nullptr);

	// Deep merge: like merge but for map-valued keys, merges their contents instead of overwriting
	[[nodiscard]] static DataNode mergeDeep(const DataNode& base, const DataNode& overlay);

	// Iteration — works for maps and sequences
	class Iterator {
	public:
		Iterator();
		~Iterator();
		Iterator(const Iterator& other);
		Iterator& operator=(const Iterator& other);

		// Access current key and value (returns empty DataNode at end)
		DataNode key() const;
		DataNode value() const;

		Iterator& operator++();
		bool operator!=(const Iterator& other) const;

	private:
		friend class DataNode;
		struct Impl;
		std::shared_ptr<Impl> impl;
	};

	Iterator begin() const;
	Iterator end() const;

	// Load a YAML file directly (for use before ResourceManager is available)
	[[nodiscard]] static DataNode loadFile(const char* path);

	// Factory — used only by ResourceManager/DataNode internals
	[[nodiscard]] static DataNode fromYAML(const YAML::Node& node);

private:
	struct Impl;
	std::shared_ptr<Impl> impl;
};
