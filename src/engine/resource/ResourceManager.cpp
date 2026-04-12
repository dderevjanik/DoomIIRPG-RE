#include <cstdlib>
#include <ranges>
#include "ResourceManager.h"
#include "DataNode.h"
#include "VFS.h"
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include "Log.h"

// --- Cache implementation (hides YAML types from header) ---

struct ResourceManager::CacheImpl {
	std::unordered_map<std::string, YAML::Node*> yamlCache;

	~CacheImpl() {
		for (auto& [_, value] : yamlCache) {
			delete value;
		}
	}

	const YAML::Node* loadYAML(const char* path, ResourceManager* rm) {
		if (auto it = yamlCache.find(path); it != yamlCache.end()) {
			return it->second;
		}

		std::string content = rm->readFileAsString(path);
		if (content.empty()) {
			LOG_ERROR("[resource] failed to read %s via VFS\n", path);
			return nullptr;
		}

		try {
			YAML::Node* node = new YAML::Node(YAML::Load(content));
			yamlCache[path] = node;
			LOG_INFO("[resource] loaded %s via VFS\n", path);
			return node;
		} catch (const YAML::Exception& e) {
			LOG_ERROR("[resource] YAML parse error in %s: %s\n", path, e.what());
			return nullptr;
		}
	}

	void invalidate(const char* path) {
		if (auto it = yamlCache.find(path); it != yamlCache.end()) {
			delete it->second;
			yamlCache.erase(it);
		}
	}

	void invalidateAll() {
		for (auto& [_, value] : yamlCache) {
			delete value;
		}
		yamlCache.clear();
	}
};

// --- ResourceManager ---

ResourceManager::ResourceManager()
	: vfs(nullptr), cache(new CacheImpl()), entriesSorted(false) {
}

ResourceManager::~ResourceManager() {
	delete cache;
}

void ResourceManager::init(VFS* vfs) {
	this->vfs = vfs;
}

uint8_t* ResourceManager::readFile(const char* path, int* sizeOut) {
	return vfs->readFile(path, sizeOut);
}

std::string ResourceManager::readFileAsString(const char* path) {
	int size = 0;
	uint8_t* data = vfs->readFile(path, &size);
	if (!data) {
		return "";
	}
	std::string result((const char*)data, size);
	std::free(data);
	return result;
}

DataNode ResourceManager::loadData(const char* path) {
	const YAML::Node* node = cache->loadYAML(path, this);
	if (!node) return DataNode();
	return DataNode::fromYAML(*node);
}

void ResourceManager::invalidateData(const char* path) {
	cache->invalidate(path);
}

void ResourceManager::invalidateAll() {
	cache->invalidateAll();
}

void ResourceManager::registerParser(const char* name, const char* dataPath,
									 std::function<bool(const DataNode&)> parser,
									 int priority, bool optional) {
	DefinitionEntry e;
	e.name = name;
	e.dataPath = dataPath;
	e.parser = parser;
	e.priority = priority;
	e.optional = optional;
	entries.push_back(std::move(e));
	entriesSorted = false;
}

void ResourceManager::registerLoader(const char* name,
									 std::function<bool(ResourceManager*)> loader,
									 int priority) {
	DefinitionEntry e;
	e.name = name;
	e.loader = loader;
	e.priority = priority;
	e.optional = false;
	entries.push_back(std::move(e));
	entriesSorted = false;
}

bool ResourceManager::loadAllDefinitions() {
	if (!entriesSorted) {
		std::ranges::sort(entries, [](const DefinitionEntry& a, const DefinitionEntry& b) {
			return a.priority < b.priority;
		});
		entriesSorted = true;
	}

	LOG_INFO("[resource] loading %d registered definitions\n", (int)entries.size());

	for (auto& entry : entries) {
		LOG_INFO("[resource] running: %s (priority %d)\n", entry.name.c_str(), entry.priority);

		bool ok;
		if (!entry.dataPath.empty()) {
			// Parser: ResourceManager loads the data, then calls the parser
			DataNode data = loadData(entry.dataPath.c_str());
			if (!data) {
				if (entry.optional) {
					LOG_WARN("[resource] %s: %s not found (optional, skipping)\n",
						   entry.name.c_str(), entry.dataPath.c_str());
					continue;
				}
				LOG_ERROR("[resource] %s: failed to load %s\n",
					   entry.name.c_str(), entry.dataPath.c_str());
				return false;
			}
			ok = entry.parser(data);
		} else {
			// Raw loader: callback handles everything
			ok = entry.loader(this);
		}

		if (!ok) {
			LOG_ERROR("[resource] failed: %s\n", entry.name.c_str());
			return false;
		}
	}

	LOG_INFO("[resource] all definitions loaded successfully\n");
	return true;
}

bool ResourceManager::fileExists(const char* path) {
	return vfs->fileExists(path);
}
