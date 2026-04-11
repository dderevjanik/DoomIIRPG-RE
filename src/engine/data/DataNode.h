#ifndef __DATANODE_H__
#define __DATANODE_H__

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
	int asInt(int def = 0) const;
	long long asLongLong(long long def = 0) const;
	unsigned long asULong(unsigned long def = 0) const;
	float asFloat(float def = 0.0f) const;
	bool asBool(bool def = false) const;
	std::string asString(const std::string& def = "") const;

	// Subscript (returns empty/null node if key missing)
	DataNode operator[](const char* key) const;
	DataNode operator[](const std::string& key) const;
	DataNode operator[](int index) const;

	// Type checks
	explicit operator bool() const;
	bool isMap() const;
	bool isScalar() const;
	bool isSequence() const;
	int size() const;

	// Deep copy
	DataNode clone() const;

	// Merge: clone base, overlay all keys from other on top (skips a given key)
	static DataNode merge(const DataNode& base, const DataNode& overlay, const char* skipKey = nullptr);

	// Deep merge: like merge but for map-valued keys, merges their contents instead of overwriting
	static DataNode mergeDeep(const DataNode& base, const DataNode& overlay);

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
	static DataNode loadFile(const char* path);

	// Factory — used only by ResourceManager/DataNode internals
	static DataNode fromYAML(const YAML::Node& node);

private:
	struct Impl;
	std::shared_ptr<Impl> impl;
};

#endif
