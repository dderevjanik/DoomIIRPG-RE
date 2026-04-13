#include "ModManager.h"
#include "VFS.h"
#include "DataNode.h"
#include "Log.h"

#include <algorithm>
#include <dirent.h>
#include <format>
#include <queue>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>

// Check if a path is a directory
static bool isDirectory(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// Check if a path is a regular file
static bool isRegularFile(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

// Extract basename without extension (e.g. "/foo/bar.zip" -> "bar")
static std::string stemName(const std::string& path) {
	size_t slash = path.find_last_of('/');
	std::string name = (slash != std::string::npos) ? path.substr(slash + 1) : path;
	size_t dot = name.find_last_of('.');
	return (dot != std::string::npos) ? name.substr(0, dot) : name;
}

// Extract basename (e.g. "/foo/bar/" -> "bar")
static std::string dirBaseName(const std::string& path) {
	std::string p = path;
	while (!p.empty() && p.back() == '/') p.pop_back();
	size_t slash = p.find_last_of('/');
	return (slash != std::string::npos) ? p.substr(slash + 1) : p;
}

bool ModManager::parseModYaml(const std::string& dirPath, ModInfo& info) {
	std::string yamlPath = dirPath + "/mod.yaml";
	DataNode config = DataNode::loadFile(yamlPath.c_str());
	if (!config) return false;

	DataNode mod = config["mod"];
	if (!mod) return false;

	info.name = mod["name"].asString(info.name);
	info.id = mod["id"].asString(info.id);
	info.version = mod["version"].asString("");
	info.author = mod["author"].asString("");
	info.requiresGame = mod["requires_game"].asString("");

	DataNode deps = mod["depends"];
	if (deps) {
		for (auto it = deps.begin(); it != deps.end(); ++it) {
			info.depends.push_back(it.value().asString());
		}
	}

	DataNode la = mod["load_after"];
	if (la) {
		for (auto it = la.begin(); it != la.end(); ++it) {
			info.loadAfter.push_back(it.value().asString());
		}
	}

	DataNode conf = mod["conflicts"];
	if (conf) {
		for (auto it = conf.begin(); it != conf.end(); ++it) {
			info.conflicts.push_back(it.value().asString());
		}
	}

	DataNode sd = mod["search_dirs"];
	if (sd) {
		for (auto it = sd.begin(); it != sd.end(); ++it) {
			info.searchDirs.push_back(it.value().asString());
		}
	}

	return true;
}

void ModManager::discoverMods(const std::string& modsDir, const std::vector<std::string>& cliModDirs) {
	discoveredMods.clear();
	warnings.clear();

	std::unordered_set<std::string> seenIds;

	// Phase 1: Auto-discover from mods/ directory
	if (isDirectory(modsDir)) {
		DIR* dir = opendir(modsDir.c_str());
		if (dir) {
			struct dirent* entry;
			// Collect entries first, then sort for deterministic order
			std::vector<std::string> entries;
			while ((entry = readdir(dir)) != nullptr) {
				if (entry->d_name[0] == '.') continue;
				entries.push_back(entry->d_name);
			}
			closedir(dir);
			std::sort(entries.begin(), entries.end());

			for (const auto& name : entries) {
				std::string fullPath = modsDir + "/" + name;

				ModInfo info;
				info.path = fullPath;

				if (isDirectory(fullPath)) {
					// Directory mod
					info.isZip = false;
					info.id = name;
					info.name = name;
					parseModYaml(fullPath, info);
				} else if (name.size() > 4 && name.substr(name.size() - 4) == ".zip") {
					// ZIP mod — id from filename stem, mod.yaml parsed after extraction
					info.isZip = true;
					info.id = stemName(name);
					info.name = stemName(name);
					// Note: mod.yaml inside zip will be parsed during mountAll
				} else {
					continue; // Skip non-directory, non-zip files
				}

				if (seenIds.count(info.id)) {
					warnings.push_back(std::format("[mod] Warning: duplicate mod id '{}' in mods/ — skipping {}", info.id, fullPath));
					continue;
				}
				seenIds.insert(info.id);
				discoveredMods.push_back(std::move(info));
			}
		}
	}

	// Phase 2: Add CLI --mod directories (these override auto-discovered mods with same id)
	for (const auto& modDir : cliModDirs) {
		ModInfo info;
		info.path = modDir;

		if (isDirectory(modDir)) {
			info.isZip = false;
			info.id = dirBaseName(modDir);
			info.name = info.id;
			parseModYaml(modDir, info);
		} else if (isRegularFile(modDir) && modDir.size() > 4 && modDir.substr(modDir.size() - 4) == ".zip") {
			info.isZip = true;
			info.id = stemName(modDir);
			info.name = info.id;
		} else {
			warnings.push_back(std::format("[mod] Warning: --mod path '{}' is not a directory or .zip — skipping", modDir));
			continue;
		}

		// CLI mods override auto-discovered mods with the same id
		if (seenIds.count(info.id)) {
			auto it = std::find_if(discoveredMods.begin(), discoveredMods.end(),
				[&](const ModInfo& m) { return m.id == info.id; });
			if (it != discoveredMods.end()) {
				*it = std::move(info);
			}
		} else {
			seenIds.insert(info.id);
			discoveredMods.push_back(std::move(info));
		}
	}
}

std::vector<ModInfo> ModManager::resolveLoadOrder(const std::vector<ModInfo>& mods) {
	// Build adjacency list and in-degree map for topological sort
	std::unordered_map<std::string, int> idToIndex;
	for (size_t i = 0; i < mods.size(); i++) {
		idToIndex[mods[i].id] = static_cast<int>(i);
	}

	int n = static_cast<int>(mods.size());
	std::vector<std::vector<int>> adj(n);     // adj[i] = list of mods that must load after i
	std::vector<int> inDegree(n, 0);

	for (int i = 0; i < n; i++) {
		const auto& mod = mods[i];

		// depends: this mod must load after its dependencies
		for (const auto& dep : mod.depends) {
			auto it = idToIndex.find(dep);
			if (it != idToIndex.end()) {
				adj[it->second].push_back(i);  // dep -> this
				inDegree[i]++;
			} else {
				warnings.push_back(std::format("[mod] Warning: '{}' requires '{}' which is not loaded", mod.id, dep));
			}
		}

		// load_after: soft ordering (no error if missing)
		for (const auto& la : mod.loadAfter) {
			auto it = idToIndex.find(la);
			if (it != idToIndex.end()) {
				adj[it->second].push_back(i);
				inDegree[i]++;
			}
		}
	}

	// Kahn's algorithm for topological sort
	std::queue<int> q;
	for (int i = 0; i < n; i++) {
		if (inDegree[i] == 0) q.push(i);
	}

	std::vector<ModInfo> sorted;
	sorted.reserve(n);

	while (!q.empty()) {
		int u = q.front();
		q.pop();
		sorted.push_back(mods[u]);

		for (int v : adj[u]) {
			if (--inDegree[v] == 0) {
				q.push(v);
			}
		}
	}

	if (static_cast<int>(sorted.size()) < n) {
		// Cycle detected — add remaining mods in original order with a warning
		warnings.push_back("[mod] Warning: circular dependency detected among mods — loading in discovery order");
		std::unordered_set<std::string> added;
		for (const auto& m : sorted) added.insert(m.id);
		for (const auto& m : mods) {
			if (!added.count(m.id)) {
				sorted.push_back(m);
			}
		}
	}

	return sorted;
}

void ModManager::checkConflicts(const std::vector<ModInfo>& mods) {
	std::unordered_set<std::string> activeIds;
	for (const auto& m : mods) {
		activeIds.insert(m.id);
	}

	for (const auto& mod : mods) {
		for (const auto& conflict : mod.conflicts) {
			if (activeIds.count(conflict)) {
				warnings.push_back(std::format("[mod] Warning: '{}' conflicts with '{}' — both are loaded", mod.id, conflict));
			}
		}
	}
}

void ModManager::mountAll(VFS& vfs, int basePriority) {
	if (discoveredMods.empty()) return;

	// Resolve load order via topological sort
	auto ordered = resolveLoadOrder(discoveredMods);

	// Check for conflicts
	checkConflicts(ordered);

	// Mount in resolved order
	loadedMods.clear();
	for (size_t i = 0; i < ordered.size(); i++) {
		ModInfo& mod = ordered[i];
		mod.priority = basePriority + static_cast<int>(i);

		if (mod.isZip) {
			vfs.mountZip(mod.path.c_str(), mod.priority);
		} else {
			vfs.mountDir(mod.path.c_str(), mod.priority);
		}

		// Register search dirs from the mod
		for (const auto& sd : mod.searchDirs) {
			vfs.addSearchDir(sd.c_str());
		}

		printf("[mod] Loaded: %s%s%s (id: %s, priority %d%s)\n",
			mod.name.c_str(),
			mod.version.empty() ? "" : (" v" + mod.version).c_str(),
			mod.author.empty() ? "" : (" by " + mod.author).c_str(),
			mod.id.c_str(),
			mod.priority,
			mod.isZip ? ", zip" : "");

		loadedMods.push_back(std::move(mod));
	}

	// Print warnings
	for (const auto& w : warnings) {
		printf("%s\n", w.c_str());
	}
}
