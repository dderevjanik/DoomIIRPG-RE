#include "ResourceManager.h"
#include "VFS.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

ResourceManager::ResourceManager()
	: vfs(nullptr), loadersSorted(false) {
}

ResourceManager::~ResourceManager() {
	invalidateAll();
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

const YAML::Node* ResourceManager::loadYAML(const char* path) {
	// Check cache first
	auto it = yamlCache.find(path);
	if (it != yamlCache.end()) {
		return it->second;
	}

	// Read through VFS
	std::string content = readFileAsString(path);
	if (content.empty()) {
		printf("[resource] failed to read %s via VFS\n", path);
		return nullptr;
	}

	try {
		YAML::Node* node = new YAML::Node(YAML::Load(content));
		yamlCache[path] = node;
		printf("[resource] loaded %s via VFS\n", path);
		return node;
	} catch (const YAML::Exception& e) {
		printf("[resource] YAML parse error in %s: %s\n", path, e.what());
		return nullptr;
	}
}

void ResourceManager::invalidateYAML(const char* path) {
	auto it = yamlCache.find(path);
	if (it != yamlCache.end()) {
		delete it->second;
		yamlCache.erase(it);
	}
}

void ResourceManager::invalidateAll() {
	for (auto& kv : yamlCache) {
		delete kv.second;
	}
	yamlCache.clear();
}

void ResourceManager::registerLoader(const char* name,
									 std::function<bool(ResourceManager*)> loader,
									 int priority) {
	loaders.push_back({name, loader, priority});
	loadersSorted = false;
}

bool ResourceManager::loadAllDefinitions() {
	if (!loadersSorted) {
		std::sort(loaders.begin(), loaders.end(),
				  [](const LoaderEntry& a, const LoaderEntry& b) {
					  return a.priority < b.priority;
				  });
		loadersSorted = true;
	}

	printf("[resource] loading %d registered definitions\n", (int)loaders.size());

	for (auto& entry : loaders) {
		printf("[resource] running loader: %s (priority %d)\n", entry.name.c_str(), entry.priority);
		if (!entry.loader(this)) {
			printf("[resource] loader failed: %s\n", entry.name.c_str());
			return false;
		}
	}

	printf("[resource] all definitions loaded successfully\n");
	return true;
}

bool ResourceManager::fileExists(const char* path) {
	return vfs->fileExists(path);
}
