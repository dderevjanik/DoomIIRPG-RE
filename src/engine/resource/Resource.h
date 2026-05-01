#pragma once
#include <span>
#include "BinaryStream.h"

class InputStream;

class Resources
{
public:
    static constexpr char* RES_LOGO_BMP_GZ = "logo.bmp";
    static constexpr char* RES_LOGO2_BMP_GZ = "logo2.bmp";
};

class Applet;

class Resource
{
private:

public:
    Applet* app = nullptr;  // Set in startup(), replaces CAppContainer::getInstance()->app

    static constexpr int IO_SIZE = 20480;

    int touchMe = 0;
    int cursor = 0;
    uint8_t* ioBuffer = nullptr;

	// Constructor
	Resource();
	// Destructor
	~Resource();

	bool startup();
    void readByteArray(InputStream* IS, std::span<uint8_t> dest);
    void readUByteArray(InputStream* IS, std::span<short> dest);
    void readCoordArray(InputStream* IS, std::span<short> dest);
    void readShortArray(InputStream* IS, std::span<short> dest);
    void readUShortArray(InputStream* IS, std::span<int> dest);
    void readIntArray(InputStream* IS, std::span<int> dest);
    void readMarker(InputStream* IS, int i);
    void readMarker(InputStream* IS);
    void writeMarker(OutputStream* OS, int i);
    void writeMarker(OutputStream* OS);
    void read(InputStream* IS, int i);
    void bufSkip(InputStream* IS, int offset, bool updateLB);
    uint8_t byteAt(int i);
    uint8_t shiftByte();
    short UByteAt(int i);
    short shiftUByte();
    short shortAt(int i);
    short shiftShort();
    int shiftUShort();
    int shiftInt();
    short shiftCoord();
};
