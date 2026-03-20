#ifndef __VFS_H__
#define __VFS_H__

#include <stdint.h>
#include <vector>
#include <string>

class ZipFile;

// A mounted source for the VFS (zip archive or filesystem directory)
struct VFSMount {
	enum Type { MOUNT_ZIP, MOUNT_DIR };

	Type type;
	std::string basePath;  // zip: internal prefix, dir: filesystem directory
	ZipFile* zipFile;      // only used for MOUNT_ZIP
	int priority;          // higher priority = checked first
};

class VFS {
private:
	std::vector<VFSMount> mounts;

	uint8_t* readFromZip(const VFSMount& mount, const char* path, int* sizeOut);
	uint8_t* readFromDir(const VFSMount& mount, const char* path, int* sizeOut);

public:
	VFS();
	~VFS();

	// Mount a zip file with an internal base path prefix (e.g. "Payload/Doom2rpg.app/Packages/")
	void mountZip(ZipFile* zipFile, const char* basePath, int priority);

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
