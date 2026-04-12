#pragma once
#include <cstdint>
#include <expected>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class DataNode;
class VFS;

class ResourceManager {
public:
	ResourceManager();
	~ResourceManager();

	void init(VFS* vfs);

	// --- VFS-backed file access ---

	// Read a raw file through VFS. Returns malloc'd buffer, caller frees.
	[[nodiscard]] uint8_t* readFile(const char* path, int* sizeOut);

	// Read a file as a string through VFS.
	[[nodiscard]] std::string readFileAsString(const char* path);

	// --- Data loading with caching (YAML-free public API) ---

	// Load and cache a data document by logical path.
	// Returns a DataNode wrapping the parsed content.
	// Returns an empty (falsy) DataNode on failure.
	[[nodiscard]] DataNode loadData(const char* path);

	// Invalidate a cached document (for future hot-reload).
	void invalidateData(const char* path);

	// Invalidate all cached resources.
	void invalidateAll();

	// --- Registered loader/parser pattern ---

	// Parse result: void on success, error message on failure.
	using ParseResult = std::expected<void, std::string>;

	// Register a parser for a data file. ResourceManager handles loadData()
	// and calls the parser with the resulting DataNode.
	// If optional=true, a missing file is not an error (parser is skipped).
	void registerParser(const char* name, const char* dataPath,
						std::function<ParseResult(const DataNode&)> parser,
						int priority = 100, bool optional = false);

	// Register a loader callback for multi-file or custom loading scenarios.
	// The loader receives ResourceManager* and handles loadData() itself.
	void registerLoader(const char* name,
						std::function<ParseResult(ResourceManager*)> loader,
						int priority = 100);

	// Run all registered parsers and loaders in priority order.
	[[nodiscard]] ParseResult loadAllDefinitions();

	// --- Utility ---

	[[nodiscard]] bool fileExists(const char* path);
	[[nodiscard]] VFS* getVFS() { return vfs; }

private:
	VFS* vfs;

	// Opaque cache — implementation in ResourceManager.cpp
	struct CacheImpl;
	CacheImpl* cache;

	// Unified entry for both parsers and loaders, sorted by priority on load
	struct DefinitionEntry {
		std::string name;
		std::string dataPath;                              // empty for raw loaders
		std::function<ParseResult(const DataNode&)> parser; // used when dataPath is set
		std::function<ParseResult(ResourceManager*)> loader; // used when dataPath is empty
		int priority;
		bool optional;
	};
	std::vector<DefinitionEntry> entries;
	bool entriesSorted;
};
