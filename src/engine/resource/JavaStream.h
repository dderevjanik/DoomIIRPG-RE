#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string>
static constexpr int LT_RESOURCE = 5;
static constexpr int LT_FILE = 6;
static constexpr int LT_SOUND_RESOURCE = 7; // [GEC]

// Save directory - set at startup from GameConfig.saveDir
const std::string& getSaveDir();
void setSaveDir(const std::string& dir);

class Applet;

// ------------------
// InputStream Class
// ------------------

class InputStream {
  private:
	int unused_0x0; // never used
	int unused_0x4; // never used

	uint32_t cursor;
	FILE* file;
	int fileSize;
	int unused_0x28; // never used

  public:
	static constexpr int LOADTYPE_RESOURCE = 5;
	static constexpr int LOADTYPE_FILE = 6;
	uint8_t* data;

	// Constructor
	InputStream();
	// Destructor
	~InputStream();
	uint8_t* getTop();
	const uint8_t* getData();
	int getFileSize();
	void offsetCursor(int offset);
	bool startup();
	bool loadResource(const char* fileName);
	bool loadFile(const char* fileName, int loadType);
	// Point this stream at a caller-owned memory buffer instead of loading
	// from the VFS. The stream does NOT take ownership — caller keeps `data`
	// alive until `close()`. Used by the map editor for fast hot-reload of a
	// compiled .bin without round-tripping through disk.
	bool loadFromBuffer(const uint8_t* data, int size);
	void close();
	int readInt();
	int readShort();
	bool readBoolean();
	uint8_t readByte();
	uint8_t readUnsignedByte();
	int readSignedByte();
	void read(uint8_t* dest, int off, int size);
};

// -------------------
// OutputStream Class
// -------------------

class OutputStream {
  private:
	uint8_t* getTop();
	bool canWrite(int typeSizeof);
	bool isOpen;
	FILE* file;
	uint8_t* buffer;
	uint32_t written;
	uint8_t* writeBuff;
	int unused_0x24; // never used
	int fileSize;
	int flushCount;
	bool noWrite;
	Applet* App;

  public:
	// Constructor
	OutputStream();
	// Destructor
	~OutputStream();

	int openFile(const char* fileName, int openMode);
	void close();
	int flush();
	void writeInt(int i);
	void writeShort(int16_t i);
	void writeByte(uint8_t i);
	void writeBoolean(bool b);
	void write(uint8_t* buff, int off, int size);
};
