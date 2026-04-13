#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <ranges>
#include <sys/stat.h>
#include "Log.h"

#include "VFS.h"
#include "ZipFile.h"

VFS::VFS() {
}

VFS::~VFS() {
	// Clean up owned ZipFile instances
	for (auto& mount : mounts) {
		if (mount.zip) {
			mount.zip->closeZipFile();
			delete mount.zip;
			mount.zip = nullptr;
		}
	}
}

void VFS::sortMounts() {
	std::ranges::sort(mounts, [](const VFSMount& a, const VFSMount& b) {
		return a.priority > b.priority;
	});
}

void VFS::mountDir(const char* dirPath, int priority) {
	VFSMount mount;
	mount.basePath = dirPath ? dirPath : "";
	mount.priority = priority;
	mount.zip = nullptr;
	mounts.push_back(mount);

	sortMounts();

	// Invalidate file index when mounts change
	fileIndexBuilt = false;
	fileIndex.clear();
}

void VFS::mountZip(const char* zipPath, int priority) {
	ZipFile* zip = new ZipFile();
	zip->openZipFile(zipPath);
	if (zip->getEntryCount() == 0) {
		LOG_WARN("VFS: failed to open zip or empty archive: {}\n", zipPath);
		delete zip;
		return;
	}

	VFSMount mount;
	mount.basePath = zipPath ? zipPath : "";
	mount.priority = priority;
	mount.zip = zip;
	mounts.push_back(mount);

	sortMounts();

	LOG_INFO("VFS: mounted zip '{}' with {} entries at priority {}\n",
		zipPath, zip->getEntryCount(), priority);

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

static void scanZipForSearchDir(ZipFile* zip, const std::string& subdir,
                                std::unordered_map<std::string, std::string>& index) {
	std::string prefix = subdir + "/";
	for (int i = 0; i < zip->getEntryCount(); i++) {
		const char* entryName = zip->getEntryName(i);
		if (!entryName) continue;

		std::string name(entryName);
		// Check if entry is under the search subdir
		if (name.size() > prefix.size() && name.substr(0, prefix.size()) == prefix) {
			// Only include direct children (no deeper subdirs)
			std::string rest = name.substr(prefix.size());
			if (rest.find('/') == std::string::npos) {
				if (index.find(rest) == index.end()) {
					index[rest] = name;
				}
			}
		}
	}
}

void VFS::buildFileIndex() {
	fileIndex.clear();
	for (const auto& mount : mounts) {
		for (const auto& subdir : searchDirs) {
			if (mount.zip) {
				scanZipForSearchDir(mount.zip, subdir, fileIndex);
			} else {
				scanDirectory(mount.basePath, subdir, fileIndex);
			}
		}
	}
	fileIndexBuilt = true;
	LOG_INFO("VFS: file index built with {} entries\n", fileIndex.size());
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

uint8_t* VFS::readFromZip(const VFSMount& mount, const char* path, int* sizeOut) {
	if (!mount.zip) return nullptr;
	return mount.zip->readZipFileEntry(path, sizeOut);
}

uint8_t* VFS::readFile(const char* path, int* sizeOut) {
	// First, try the exact path across all mounts
	for (const auto& mount : mounts) {
		uint8_t* data = mount.zip
			? readFromZip(mount, path, sizeOut)
			: readFromDir(mount, path, sizeOut);
		if (data) {
			return data;
		}
	}

	// If not found and we have search dirs, check the file index
	if (!searchDirs.empty()) {
		if (!fileIndexBuilt) {
			buildFileIndex();
		}

		if (auto it = fileIndex.find(path); it != fileIndex.end()) {
			for (const auto& mount : mounts) {
				uint8_t* data = mount.zip
					? readFromZip(mount, it->second.c_str(), sizeOut)
					: readFromDir(mount, it->second.c_str(), sizeOut);
				if (data) {
					return data;
				}
			}
		} else {
			LOG_WARN("VFS: '{}' not in file index\n", path);
		}
	}

	LOG_WARN("VFS: file not found: {}\n", path);
	return nullptr;
}

bool VFS::fileExists(const char* path) {
	// Try exact path first
	for (const auto& mount : mounts) {
		if (mount.zip) {
			if (mount.zip->hasEntry(path)) return true;
		} else {
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
	}

	// Check file index for search dir resolution
	if (!searchDirs.empty()) {
		if (!fileIndexBuilt) {
			buildFileIndex();
		}

		if (auto it = fileIndex.find(path); it != fileIndex.end()) {
			for (const auto& mount : mounts) {
				if (mount.zip) {
					if (mount.zip->hasEntry(it->second.c_str())) return true;
				} else {
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
	}

	return false;
}
