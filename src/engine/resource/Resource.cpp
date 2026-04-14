#include <span>
#include <stdexcept>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "Resource.h"
#include "Canvas.h"

Resource::Resource() {
    this->ioBuffer = new uint8_t[Resource::IO_SIZE]();
}

Resource::~Resource() {
}

bool Resource::startup() {
	this->app = CAppContainer::getInstance()->app;
	//printf("Resource::startup\n");

	return false;
}

void Resource::readByteArray(InputStream* IS, std::span<uint8_t> dest) {
	IS->read(dest.data(), 0, static_cast<int>(dest.size()));
}

void Resource::readUByteArray(InputStream* IS, std::span<short> dest) {
    int off = 0;
    int size = static_cast<int>(dest.size());
    while (size > 0) {
        int n2 = (Resource::IO_SIZE > size) ? size : Resource::IO_SIZE;
        size -= n2;
        this->read(IS, n2);
        while (--n2 >= 0) {
            dest[off++] = this->shiftUByte();
        }
    }
}

void Resource::readCoordArray(InputStream* IS, std::span<short> dest) {
    int off = 0;
    int size = static_cast<int>(dest.size());
    while (size > 0) {
        int n2 = (Resource::IO_SIZE > size) ? size : Resource::IO_SIZE;
        size -= n2;
        this->read(IS, n2);
        while (--n2 >= 0) {
            dest[off++] = this->shiftCoord();
        }
    }
}

void Resource::readShortArray(InputStream* IS, std::span<short> dest) {
    int off = 0;
    int size = static_cast<int>(dest.size());
    while (size > 0) {
        int n2 = (Resource::IO_SIZE > size) ? size : Resource::IO_SIZE;
        size -= n2;
        this->read(IS, n2 * sizeof(short));
        while (--n2 >= 0) {
            dest[off++] = this->shiftShort();
        }
    }
}

void Resource::readUShortArray(InputStream* IS, std::span<int> dest) {
    int off = 0;
    int size = static_cast<int>(dest.size());
    while (size > 0) {
        int n2 = (Resource::IO_SIZE > size) ? size : Resource::IO_SIZE;
        size -= n2;
        this->read(IS, n2 * sizeof(uint16_t));
        while (--n2 >= 0) {
            dest[off++] = this->shiftUShort();
        }
    }
}

void Resource::readIntArray(InputStream* IS, std::span<int> dest) {
    int off = 0;
    int size = static_cast<int>(dest.size());
    while (size > 0) {
        int n2 = (Resource::IO_SIZE > size) ? size : Resource::IO_SIZE;
        size -= n2;
        this->read(IS, n2 * sizeof(int));
        while (--n2 >= 0) {
            dest[off++] = this->shiftInt();
        }
    }
}

void Resource::readMarker(InputStream* IS, int i) {
    this->read(IS, sizeof(uint32_t));
}

void Resource::readMarker(InputStream* IS) {
    this->readMarker(IS, 0xCAFEBABE);
}

void Resource::writeMarker(OutputStream* OS, int i) {
    this->ioBuffer[0] = (uint8_t)(i & 0xFF);
    this->ioBuffer[1] = (uint8_t)(i >> 8 & 0xFF);
    this->ioBuffer[2] = (uint8_t)(i >> 16 & 0xFF);
    this->ioBuffer[3] = (uint8_t)(i >> 24 & 0xFF);
    OS->write(this->ioBuffer, 0, sizeof(uint32_t));
}

void Resource::writeMarker(OutputStream* OS) {
    this->writeMarker(OS, 0xCAFEBABE);
}

void Resource::read(InputStream* IS, int i) {
    this->cursor = 0;
    IS->read(this->ioBuffer, 0, i);
}

void Resource::bufSkip(InputStream* IS, int offset, bool updateLB) {
    IS->offsetCursor(offset);
    if (updateLB) {
        this->app->canvas->updateLoadingBar(false);
    }
}

uint8_t Resource::byteAt(int i) {
    return this->ioBuffer[i];
}

uint8_t Resource::shiftByte() {
    return this->ioBuffer[this->cursor++];
}

short Resource::UByteAt(int i) {
    return (short)(this->ioBuffer[i] & 0xFF);
}

short Resource::shiftUByte() {
    return (short)(this->ioBuffer[this->cursor++] & 0xFF);
}

short Resource::shortAt(int i) {
    return (short)((this->ioBuffer[i + 0] & 0xFF) + (this->ioBuffer[i + 1] << 8 & 0xFF00));
}

short Resource::shiftShort() {
    short dat = (short)((this->ioBuffer[this->cursor + 0] & 0xFF) + (this->ioBuffer[this->cursor + 1] << 8 & 0xFF00));
    this->cursor += sizeof(short);
    return dat;
}

int Resource::shiftUShort() {
    int dat = (int)((this->ioBuffer[this->cursor + 0] & 0xFF) + (this->ioBuffer[this->cursor + 1] << 8 & 0xFF00));
    this->cursor += sizeof(uint16_t);
    return dat;
}

int Resource::shiftInt() {
    int dat = (int)(this->ioBuffer[this->cursor + 3] << 24 & 0xFF000000) | (this->ioBuffer[this->cursor + 2] << 16 & 0xFF0000) | (this->ioBuffer[this->cursor + 1] << 8 & 0xFF00) | (this->ioBuffer[this->cursor + 0] & 0xFF);
    this->cursor += sizeof(int);
    return dat;
}

short Resource::shiftCoord() {
    return (short)((this->ioBuffer[this->cursor++] & 0xFF) * 8);
}

