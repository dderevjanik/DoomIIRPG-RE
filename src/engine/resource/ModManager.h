#pragma once
#include <string>
#include <vector>

class VFS;

// Metadata for a loaded mod, parsed from mod.yaml
struct ModInfo {
	std::string id;           // Unique mod identifier (defaults to directory/zip name)
	std::string name;         // Display name
	std::string version;      // Version string
	std::string author;       // Author name
	std::string path;         // Filesystem path (directory or .zip file)
	bool isZip = false;       // True if mod was loaded from a .zip archive
	int priority = 0;         // VFS mount priority assigned at load time

	// Dependency/conflict metadata
	std::string requiresGame;         // Required game module (e.g. "doom2rpg")
	std::vector<std::string> depends; // Mod IDs this mod requires
	std::vector<std::string> loadAfter;  // Mod IDs this should load after (soft dependency)
	std::vector<std::string> conflicts;  // Mod IDs that conflict with this mod
	std::vector<std::string> searchDirs; // Additional VFS search directories
};

// Manages mod discovery, dependency resolution, and loading.
// Created during startup, stored in CAppContainer for DevConsole access.
class ModManager {
public:
	ModManager() = default;

	// Discover mods from the mods/ directory (auto-discovery) and CLI --mod flags.
	// Returns the resolved, ordered list of ModInfo structs ready for mounting.
	// Logs warnings for missing dependencies and conflicts.
	void discoverMods(const std::string& modsDir, const std::vector<std::string>& cliModDirs);

	// Mount all discovered mods onto the VFS, starting from basePriority.
	// Parses mod.yaml, resolves load order, and mounts dirs/zips.
	void mountAll(VFS& vfs, int basePriority = 300);

	// Get the list of loaded mods (in load order)
	const std::vector<ModInfo>& getLoadedMods() const { return loadedMods; }

	// Get warnings generated during discovery/resolution
	const std::vector<std::string>& getWarnings() const { return warnings; }

private:
	std::vector<ModInfo> discoveredMods;  // All discovered mods (before resolution)
	std::vector<ModInfo> loadedMods;      // Mods in resolved load order (after mounting)
	std::vector<std::string> warnings;

	// Parse mod.yaml from a directory path, populate ModInfo fields
	bool parseModYaml(const std::string& dirPath, ModInfo& info);

	// Topological sort on depends/loadAfter to determine load order.
	// Returns sorted list. Adds warnings for cycles or missing deps.
	std::vector<ModInfo> resolveLoadOrder(const std::vector<ModInfo>& mods);

	// Check for conflicts between mods. Adds warnings.
	void checkConflicts(const std::vector<ModInfo>& mods);
};
