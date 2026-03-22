#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

#include "VFS.h"

VFS::VFS() {
}

VFS::~VFS() {
}

void VFS::mountDir(const char* dirPath, int priority) {
	VFSMount mount;
	mount.basePath = dirPath ? dirPath : "";
	mount.priority = priority;
	mounts.push_back(mount);

	std::sort(mounts.begin(), mounts.end(), [](const VFSMount& a, const VFSMount& b) {
		return a.priority > b.priority;
	});

	// Invalidate file index when mounts change
	fileIndexBuilt = false;
	fileIndex.clear();
}

void VFS::addSearchDir(const char* subdir) {
	searchDirs.push_back(subdir);

	// Invalidate file index when search dirs change
	fileIndexBuilt = false;
	fileIndex.clear();
}

static void scanDirectory(const std::string& basePath, const std::string& subdir,
                          std::unordered_map<std::string, std::string>& index) {
	std::string fullDir = basePath;
	if (!fullDir.empty() && fullDir.back() != '/') {
		fullDir += '/';
	}
	fullDir += subdir;

	DIR* dir = opendir(fullDir.c_str());
	if (!dir) {
		return;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		if (entry->d_name[0] == '.') continue;

		std::string entryPath = fullDir + "/" + entry->d_name;
		struct stat st;
		if (stat(entryPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
			std::string name(entry->d_name);
			std::string mappedPath = subdir + "/" + name;
			// First mapping wins (higher-priority search dirs registered first)
			if (index.find(name) == index.end()) {
				index[name] = mappedPath;
			}
		}
	}
	closedir(dir);
}

void VFS::buildFileIndex() {
	fileIndex.clear();
	for (const auto& mount : mounts) {
		for (const auto& subdir : searchDirs) {
			scanDirectory(mount.basePath, subdir, fileIndex);
		}
	}
	fileIndexBuilt = true;
	printf("VFS: file index built with %zu entries\n", fileIndex.size());
}

uint8_t* VFS::readFromDir(const VFSMount& mount, const char* path, int* sizeOut) {
	std::string fullPath = mount.basePath;
	if (!fullPath.empty() && fullPath.back() != '/') {
		fullPath += '/';
	}
	fullPath += path;

	FILE* f = std::fopen(fullPath.c_str(), "rb");
	if (!f) {
		return nullptr;
	}

	std::fseek(f, 0, SEEK_END);
	int fileSize = (int)std::ftell(f);
	std::fseek(f, 0, SEEK_SET);

	uint8_t* data = (uint8_t*)std::malloc(fileSize);
	if (!data) {
		std::fclose(f);
		return nullptr;
	}

	std::fread(data, 1, fileSize, f);
	std::fclose(f);

	*sizeOut = fileSize;
	return data;
}

uint8_t* VFS::readFile(const char* path, int* sizeOut) {
	// First, try the exact path across all mounts
	for (const auto& mount : mounts) {
		uint8_t* data = readFromDir(mount, path, sizeOut);
		if (data) {
			return data;
		}
	}

	// If not found and we have search dirs, check the file index
	if (!searchDirs.empty()) {
		if (!fileIndexBuilt) {
			buildFileIndex();
		}

		auto it = fileIndex.find(path);
		if (it != fileIndex.end()) {
			for (const auto& mount : mounts) {
				uint8_t* data = readFromDir(mount, it->second.c_str(), sizeOut);
				if (data) {
					return data;
				}
			}
		} else {
			printf("VFS: '%s' not in file index\n", path);
		}
	}

	printf("VFS: file not found: %s\n", path);
	return nullptr;
}

bool VFS::fileExists(const char* path) {
	// Try exact path first
	for (const auto& mount : mounts) {
		std::string fullPath = mount.basePath;
		if (!fullPath.empty() && fullPath.back() != '/') {
			fullPath += '/';
		}
		fullPath += path;

		FILE* f = std::fopen(fullPath.c_str(), "rb");
		if (f) {
			std::fclose(f);
			return true;
		}
	}

	// Check file index for search dir resolution
	if (!searchDirs.empty()) {
		if (!fileIndexBuilt) {
			buildFileIndex();
		}

		auto it = fileIndex.find(path);
		if (it != fileIndex.end()) {
			for (const auto& mount : mounts) {
				std::string fullPath = mount.basePath;
				if (!fullPath.empty() && fullPath.back() != '/') {
					fullPath += '/';
				}
				fullPath += it->second;

				FILE* f = std::fopen(fullPath.c_str(), "rb");
				if (f) {
					std::fclose(f);
					return true;
				}
			}
		}
	}

	return false;
}
