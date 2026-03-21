#ifndef __VFS_H__
#define __VFS_H__

#include <stdint.h>
#include <vector>
#include <string>

// A mounted filesystem directory for the VFS
struct VFSMount {
	std::string basePath;  // filesystem directory
	int priority;          // higher priority = checked first
};

class VFS {
private:
	std::vector<VFSMount> mounts;

	uint8_t* readFromDir(const VFSMount& mount, const char* path, int* sizeOut);

public:
	VFS();
	~VFS();

	// Mount a filesystem directory (e.g. "basedata/" or "mods/mymod/")
	void mountDir(const char* dirPath, int priority);

	// Read a file by logical name (e.g. "logo.bmp", "sounds2/hit.wav")
	// Returns malloc'd buffer, caller must free(). Sets *sizeOut to file size.
	// Returns nullptr if not found in any mount.
	uint8_t* readFile(const char* path, int* sizeOut);

	// Check if a file exists in any mount
	bool fileExists(const char* path);
};

#endif
