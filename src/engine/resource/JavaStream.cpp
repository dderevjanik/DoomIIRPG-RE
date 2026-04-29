#include <format>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <cstdio>
#include "Log.h"

#include "JavaStream.h"
#include "CAppContainer.h"
#include "App.h"
#include "VFS.h"

// Save directory (defaults to "Doom2rpg.app", overridden from GameConfig at startup)
static std::string s_saveDir = "Doom2rpg.app";

const std::string& getSaveDir() {
	return s_saveDir;
}

void setSaveDir(const std::string& dir) {
	s_saveDir = dir;
}

namespace {
template <typename T> T readData(const uint8_t* data, uint32_t& cursor) {
	T value;
	std::memcpy(&value, data, sizeof(T));
	cursor += sizeof(T);
	return value;
}

template <typename T> void writeData(T data, uint8_t* stream, uint32_t& cursor) {
	std::memcpy(stream, &data, sizeof(T));
	cursor += sizeof(T);
}
} // namespace

// ------------------
// InputStream Class
// ------------------

InputStream::InputStream() {
	// printf("InputStream::init\n");
	this->data = nullptr;
	this->unused_0x4 = 0;
	this->cursor = 0;
	this->unused_0x0 = 0;
	this->file = nullptr;
	this->unused_0x28 = 0;
}

InputStream::~InputStream() {
	if (this->file) {
		std::fclose(this->file);
		this->file = nullptr;
	}

	if (this->data) {
		std::free(this->data);
		this->data = nullptr;
	}
}

void InputStream::offsetCursor(int offset) {
	this->cursor += offset;
}

const uint8_t* InputStream::getData() {
	return this->data;
}

int InputStream::getFileSize() {
	return this->fileSize;
}

bool InputStream::openForStreaming(const char* fileName, int loadType) {
	this->cursor = 0;
	this->data = nullptr;

	if (loadType == LT_RESOURCE) {
		VFS* vfs = CAppContainer::getInstance()->vfs;
		this->file = vfs->openFileForReading(fileName, &this->fileSize);
		return this->file != nullptr;
	}
	if (loadType == LT_SOUND_RESOURCE) {
		VFS* vfs = CAppContainer::getInstance()->vfs;
		auto namePath = std::format("audio/{}", fileName);
		this->file = vfs->openFileForReading(namePath.c_str(), &this->fileSize);
		return this->file != nullptr;
	}
	return false; // streaming not supported for LT_FILE / save-dir paths
}

bool InputStream::readChunk(uint8_t* dst, int offset, int len) {
	if (offset < 0 || len < 0 || offset + len > this->fileSize) {
		return false;
	}
	if (this->file) {
		if (std::fseek(this->file, offset, SEEK_SET) != 0) return false;
		size_t got = std::fread(dst, 1, (size_t)len, this->file);
		return (int)got == len;
	}
	if (this->data) {
		std::memcpy(dst, this->data + offset, (size_t)len);
		return true;
	}
	return false;
}

bool InputStream::loadResource(const char* fileName) {
	return loadFile(fileName, LT_RESOURCE);
}

bool InputStream::loadFile(const char* fileName, int loadType) {
	this->cursor = 0;

	if (loadType == LT_RESOURCE) {
		VFS* vfs = CAppContainer::getInstance()->vfs;
		this->data = vfs->readFile(fileName, &this->fileSize);
		if (this->data) {
			return true;
		}
	} else if (loadType == LT_SOUND_RESOURCE) { // [GEC]
		VFS* vfs = CAppContainer::getInstance()->vfs;
		auto namePath = std::format("audio/{}", fileName);
		this->data = vfs->readFile(namePath.c_str(), &this->fileSize);
		if (this->data) {
			return true;
		}
	} else if (loadType == LT_FILE) {
		auto namePath = std::format("{}/{}", getSaveDir(), fileName);

		this->file = std::fopen(namePath.c_str(), "rb");

		if (this->file != nullptr) {
			std::fseek(this->file, 0, SEEK_END);
			this->fileSize = ftell(this->file);
			std::fseek(this->file, 0, SEEK_SET);
			this->data = (uint8_t*)std::malloc(this->fileSize);
			std::fread(this->data, sizeof(uint8_t), this->fileSize, this->file);

			if (std::ferror(this->file)) {
				CAppContainer::getInstance()->app->Error("Error Reading File!\n");
				return false;
			}

			std::fclose(this->file);
			this->file = nullptr;
			return true;
		} else {
			// CAppContainer::getInstance()->app->Error("Error Openeing File: %s\n", namePath);
		}
	} else {
		CAppContainer::getInstance()->app->Error("File does not exist\n");
	}

	return false;
}

bool InputStream::loadFromBuffer(const uint8_t* src, int size) {
	// Malloc a copy so the destructor can std::free it like other load paths.
	// For the editor's hot-reload .bin (typically <10 KB) this is sub-ms.
	this->cursor = 0;
	if (this->data) {
		std::free(this->data);
		this->data = nullptr;
	}
	if (this->file) {
		std::fclose(this->file);
		this->file = nullptr;
	}
	if (!src || size <= 0) {
		this->fileSize = 0;
		return false;
	}
	this->fileSize = size;
	this->data = (uint8_t*)std::malloc(size);
	if (!this->data) {
		this->fileSize = 0;
		return false;
	}
	std::memcpy(this->data, src, size);
	return true;
}

void InputStream::close() {
	if (this->file) {
		std::fclose(this->file);
		this->file = nullptr;
	}
}

uint8_t* InputStream::getTop() {
	return &this->data[this->cursor];
};

int InputStream::readInt() {
	return readData<int32_t>(this->getTop(), this->cursor);
}

int InputStream::readShort() {
	return readData<int16_t>(this->getTop(), this->cursor);
}

bool InputStream::readBoolean() {
	return readData<uint8_t>(this->getTop(), this->cursor) != 0;
}

uint8_t InputStream::readByte() {
	return readData<uint8_t>(this->getTop(), this->cursor);
}

uint8_t InputStream::readUnsignedByte() {
	return readData<uint8_t>(this->getTop(), this->cursor);
}

int InputStream::readSignedByte() {
	return (int)readData<int8_t>(this->getTop(), this->cursor);
}

void InputStream::read(uint8_t* dest, int off, int size) {
	for (int i = 0; i < size; i++) {
		if (this->cursor >= this->fileSize) {
			break;
		}
		dest[off + i] = this->data[this->cursor];
		this->cursor++;
	}
}

// -------------------
// OutputStream Class
// -------------------

OutputStream::OutputStream() {
	// printf("OutputStream::init\n");
	this->buffer = nullptr;
	this->writeBuff = nullptr;
	this->file = nullptr;
	this->App = CAppContainer::getInstance()->app;
	;
	this->unused_0x24 = -1;
	this->written = 0;
	this->flushCount = 0;
	this->isOpen = false;
	this->noWrite = false;
}

OutputStream::~OutputStream() {
	this->close();

	if (this->writeBuff) {
		std::free(this->writeBuff);
		this->writeBuff = nullptr;
	}

	if (this->buffer != nullptr) {
		std::free(this->buffer);
		this->buffer = nullptr;
	}
}

#include <sys/stat.h>

int OutputStream::openFile(const char* fileName, int openMode) {
	struct stat sb;
	if (stat(getSaveDir().c_str(), &sb)) {
#ifdef _WIN32
		_mkdir(getSaveDir().c_str());
#else
		mkdir(getSaveDir().c_str(), 0755);
#endif
	}

	auto namePath = std::format("{}/{}", getSaveDir(), fileName);

	// printf("output file: %s\n", namePath);
	this->buffer = (uint8_t*)std::malloc(512);
	std::memset(this->buffer, 0, 512);

	if (this->file != nullptr) {
		this->isOpen = true;
		return 0;
	}

	this->isOpen = false;
	if (this->noWrite) {
		return 1;
	}

	switch (openMode) {
		case 1:
			this->file = std::fopen(namePath.c_str(), "wb");
			break;
		case 2:
			this->file = std::fopen(namePath.c_str(), "rb");
			break;
		case 3:
			this->file = std::fopen(namePath.c_str(), "w+b");
			break;
		default:
			return 0;
	}

	if (this->file != nullptr) {
		std::fseek(this->file, 0, SEEK_END);
		this->fileSize = std::ftell(this->file);
		std::fseek(this->file, 0, SEEK_SET);
		this->writeBuff = (uint8_t*)std::malloc(this->fileSize);
		return 1;
	}

	return 0;
}

uint8_t* OutputStream::getTop() {
	return &this->buffer[this->written];
}

bool OutputStream::canWrite(int typeSizeof) {
	if ((256U - this->written) < typeSizeof) {
		if (this->flush() == 0) {
			return false;
		}
	}
	return true;
}

void OutputStream::close() {
	this->flush();
	if (this->file != nullptr) {
		std::fclose(this->file);
		this->file = nullptr;
	}

	if (this->buffer != nullptr) {
		std::free(this->buffer);
		this->buffer = nullptr;
	}
}

int OutputStream::flush() {
	if (this->written == 0) {
		return 1;
	}

	this->flushCount += this->written;

	if (!this->noWrite) {
		if (std::fwrite(this->buffer, sizeof(uint8_t), this->written, this->file) <= 0) {
			return 0;
		}
	}

	this->written = 0;
	return 1;
}

void OutputStream::writeInt(int i) {
	if (!this->canWrite(sizeof(i))) {
		return;
	}

	writeData(i, this->getTop(), this->written);
}

void OutputStream::writeShort(int16_t i) {
	if (!this->canWrite(sizeof(i))) {
		return;
	}

	writeData(i, this->getTop(), this->written);
}

void OutputStream::writeByte(uint8_t i) {
	if (!this->canWrite(sizeof(i))) {
		return;
	}

	writeData(i, this->getTop(), this->written);
}

void OutputStream::writeBoolean(bool b) {
	if (!this->canWrite(sizeof(uint8_t))) {
		return;
	}
	writeData((uint8_t)b, this->getTop(), this->written);
}

void OutputStream::write(uint8_t* buff, int off, int size) {
	const int& _written = this->written;
	if ((uint32_t)(_written + size) >= 256) {
		int count = 256 - _written;
		std::memcpy(&this->buffer[_written], &buff[off], count);
		this->written += count;
		size -= count;
		off += count;
		if (this->flush() == 0) {
			return;
		}
	}

	std::memcpy(&this->buffer[_written], &buff[off], size);
	this->written += size;
}
