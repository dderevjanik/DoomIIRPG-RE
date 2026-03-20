#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "VFS.h"
#include "ZipFile.h"

VFS::VFS() {
}

VFS::~VFS() {
}

void VFS::mountZip(ZipFile* zipFile, const char* basePath, int priority) {
	VFSMount mount;
	mount.type = VFSMount::MOUNT_ZIP;
	mount.basePath = basePath ? basePath : "";
	mount.zipFile = zipFile;
	mount.priority = priority;
	mounts.push_back(mount);

	// Keep sorted by priority descending (highest priority first)
	std::sort(mounts.begin(), mounts.end(), [](const VFSMount& a, const VFSMount& b) {
		return a.priority > b.priority;
	});
}

void VFS::mountDir(const char* dirPath, int priority) {
	VFSMount mount;
	mount.type = VFSMount::MOUNT_DIR;
	mount.basePath = dirPath ? dirPath : "";
	mount.zipFile = nullptr;
	mount.priority = priority;
	mounts.push_back(mount);

	std::sort(mounts.begin(), mounts.end(), [](const VFSMount& a, const VFSMount& b) {
		return a.priority > b.priority;
	});
}

uint8_t* VFS::readFromZip(const VFSMount& mount, const char* path, int* sizeOut) {
	std::string fullPath = mount.basePath + path;
	return mount.zipFile->readZipFileEntry(fullPath.c_str(), sizeOut);
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
	for (const auto& mount : mounts) {
		uint8_t* data = nullptr;

		if (mount.type == VFSMount::MOUNT_DIR) {
			data = readFromDir(mount, path, sizeOut);
		} else if (mount.type == VFSMount::MOUNT_ZIP) {
			std::string fullPath = mount.basePath + path;
			if (mount.zipFile->hasEntry(fullPath.c_str())) {
				data = readFromZip(mount, path, sizeOut);
			}
		}

		if (data) {
			return data;
		}
	}

	return nullptr;
}

bool VFS::fileExists(const char* path) {
	for (const auto& mount : mounts) {
		if (mount.type == VFSMount::MOUNT_DIR) {
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
		} else if (mount.type == VFSMount::MOUNT_ZIP) {
			std::string fullPath = mount.basePath + path;
			if (mount.zipFile->hasEntry(fullPath.c_str())) {
				return true;
			}
		}
	}

	return false;
}
