#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <stdint.h>
#include <zlib.h>

static constexpr uint32_t ZIP_LOCAL_FILE_SIG = 0x04034b50;
static constexpr uint32_t ZIP_CENTRAL_DIRECTORY_SIG = 0x02014b50;
static constexpr uint32_t ZIP_END_OF_CENTRAL_DIRECTORY_SIG = 0x06054b50;
static constexpr uint32_t ZIP_ENCRYPTED_FLAG = 0x1;

struct zip_entry_t {
	char* name;
	int offset;
	int csize, usize;
};

class ZipFile
{
private:
	FILE* file = nullptr;
	int entry_count = 0;
	zip_entry_t* entry = nullptr;
	int page_count = 0;
	int* page = nullptr;
public:

	// Constructor
	ZipFile();
	// Destructor
	~ZipFile();

	void findAndReadZipDir(int startoffset);
	void openZipFile(const char* name);
	void closeZipFile();
	uint8_t* readZipFileEntry(const char* name, int* sizep);
	bool hasEntry(const char* name);
	int getEntryCount() const { return entry_count; }
	const char* getEntryName(int i) const { return (i >= 0 && i < entry_count) ? entry[i].name : nullptr; }
};
