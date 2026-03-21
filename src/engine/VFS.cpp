#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

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
		uint8_t* data = readFromDir(mount, path, sizeOut);
		if (data) {
			return data;
		}
	}

	return nullptr;
}

bool VFS::fileExists(const char* path) {
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

	return false;
}
