#ifndef __RESOURCEMANAGER_H__
#define __RESOURCEMANAGER_H__

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <cstdint>

namespace YAML { class Node; }

class VFS;

class ResourceManager {
public:
	ResourceManager();
	~ResourceManager();

	void init(VFS* vfs);

	// --- VFS-backed file access ---

	// Read a raw file through VFS. Returns malloc'd buffer, caller frees.
	uint8_t* readFile(const char* path, int* sizeOut);

	// Read a file as a string through VFS.
	std::string readFileAsString(const char* path);

	// --- YAML loading with caching ---

	// Load and cache a YAML document by logical path.
	// Returns pointer to cached Node, or nullptr on failure.
	const YAML::Node* loadYAML(const char* path);

	// Invalidate a cached YAML document (for future hot-reload).
	void invalidateYAML(const char* path);

	// Invalidate all cached resources.
	void invalidateAll();

	// --- Registered loader pattern ---

	// Register a definition loader. Called during initialization.
	// name: human-readable name for logging
	// loader: function that performs the actual loading
	// priority: lower runs first (0 = highest priority)
	void registerLoader(const char* name,
						std::function<bool(ResourceManager*)> loader,
						int priority = 100);

	// Run all registered loaders in priority order. Returns false if any fail.
	bool loadAllDefinitions();

	// --- Utility ---

	bool fileExists(const char* path);
	VFS* getVFS() { return vfs; }

private:
	VFS* vfs;

	// Cached YAML documents
	std::unordered_map<std::string, YAML::Node*> yamlCache;

	// Registered definition loaders, sorted by priority on load
	struct LoaderEntry {
		std::string name;
		std::function<bool(ResourceManager*)> loader;
		int priority;
	};
	std::vector<LoaderEntry> loaders;
	bool loadersSorted;
};

#endif
