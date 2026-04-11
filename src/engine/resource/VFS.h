#ifndef __VFS_H__
#define __VFS_H__

#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>

// A mounted filesystem directory for the VFS
struct VFSMount {
	std::string basePath;  // filesystem directory
	int priority;          // higher priority = checked first
};

class VFS {
private:
	std::vector<VFSMount> mounts;
	std::vector<std::string> searchDirs;

	// Lazily-built cache: bare filename -> subdir/filename (e.g. "logo.bmp" -> "ui/logo.bmp")
	std::unordered_map<std::string, std::string> fileIndex;
	bool fileIndexBuilt = false;

	uint8_t* readFromDir(const VFSMount& mount, const char* path, int* sizeOut);
	void buildFileIndex();

public:
	VFS();
	~VFS();

	// Mount a filesystem directory (e.g. "basedata/" or "mods/mymod/")
	void mountDir(const char* dirPath, int priority);

	// Register a subdirectory to search when a file isn't found at the mount root.
	// Search dirs are tried in registration order after the exact path fails.
	void addSearchDir(const char* subdir);

	// Read a file by logical name (e.g. "logo.bmp", "audio/hit.wav")
	// Returns malloc'd buffer, caller must free(). Sets *sizeOut to file size.
	// Returns nullptr if not found in any mount.
	uint8_t* readFile(const char* path, int* sizeOut);

	// Check if a file exists in any mount
	bool fileExists(const char* path);
};

#endif
