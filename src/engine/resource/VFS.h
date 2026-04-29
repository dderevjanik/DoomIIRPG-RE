#pragma once
#include <cstdio>
#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>

class ZipFile;

// A mounted filesystem source for the VFS (directory or ZIP archive)
struct VFSMount {
	std::string basePath;  // filesystem directory or zip file path
	int priority;          // higher priority = checked first
	ZipFile* zip = nullptr; // non-null = zip archive mount (owned by VFS)
};

class VFS {
private:
	std::vector<VFSMount> mounts;
	std::vector<std::string> searchDirs;

	// Lazily-built cache: bare filename -> subdir/filename (e.g. "logo.bmp" -> "ui/logo.bmp")
	std::unordered_map<std::string, std::string> fileIndex;
	bool fileIndexBuilt = false;

	uint8_t* readFromDir(const VFSMount& mount, const char* path, int* sizeOut);
	uint8_t* readFromZip(const VFSMount& mount, const char* path, int* sizeOut);
	void buildFileIndex();
	void sortMounts();

public:
	VFS();
	~VFS();

	// Mount a filesystem directory (e.g. "basedata/" or "mods/mymod/")
	void mountDir(const char* dirPath, int priority);

	// Mount a ZIP archive as a read-only filesystem
	void mountZip(const char* zipPath, int priority);

	// Register a subdirectory to search when a file isn't found at the mount root.
	// Search dirs are tried in registration order after the exact path fails.
	void addSearchDir(const char* subdir);

	// Read a file by logical name (e.g. "logo.bmp", "audio/hit.wav")
	// Returns malloc'd buffer, caller must free(). Sets *sizeOut to file size.
	// Returns nullptr if not found in any mount.
	[[nodiscard]] uint8_t* readFile(const char* path, int* sizeOut);

	// Open a file for streaming (random-access reads without loading the entire
	// file into RAM). Returns a FILE* opened "rb" for the first directory mount
	// that contains this path, or nullptr if the file is only in a zip mount or
	// doesn't exist. Sets *sizeOut to file size on success. Caller must fclose().
	// Used by Sound for music streaming via OpenAL streaming buffers.
	[[nodiscard]] FILE* openFileForReading(const char* path, int* sizeOut);

	// Check if a file exists in any mount
	[[nodiscard]] bool fileExists(const char* path);
};
