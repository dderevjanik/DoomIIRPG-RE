#pragma once
#include <span>
#include "JavaStream.h"

class InputStream;

class Resources
{
public:
    static constexpr char* RES_LOGO_BMP_GZ = "logo.bmp";
    static constexpr char* RES_LOGO2_BMP_GZ = "logo2.bmp";
    static constexpr char* RES_TABLES_BIN_GZ = "tables.bin";
    static constexpr char* RES_NEWMAPPINGS_BIN_GZ = "newMappings.bin";
    static constexpr char* RES_NEWPALETTES_BIN_GZ = "newPalettes.bin";
    static constexpr char* RES_NEWTEXEL_FILE_ARRAY[] = {
        "newTexels000.bin", "newTexels001.bin", "newTexels002.bin", "newTexels003.bin", "newTexels004.bin",
        "newTexels005.bin", "newTexels006.bin", "newTexels007.bin", "newTexels008.bin", "newTexels009.bin",
        "newTexels010.bin", "newTexels011.bin", "newTexels012.bin", "newTexels013.bin", "newTexels014.bin", 
        "newTexels015.bin", "newTexels016.bin", "newTexels017.bin", "newTexels018.bin", "newTexels019.bin",
        "newTexels020.bin", "newTexels021.bin", "newTexels022.bin", "newTexels023.bin", "newTexels024.bin",
        "newTexels025.bin", "newTexels026.bin", "newTexels027.bin", "newTexels028.bin", "newTexels029.bin",
        "newTexels030.bin", "newTexels031.bin", "newTexels032.bin", "newTexels033.bin", "newTexels034.bin",
        "newTexels035.bin", "newTexels036.bin", "newTexels037.bin", "newTexels038.bin" };
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
    int tableOffsets[20] = {};
    int prevOffset = 0;
    InputStream prevIS{};

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
    void initTableLoading();
    void beginTableLoading();
    void seekTable(int index);
    void finishTableLoading();
    int getNumTableBytes(int index);
    int getNumTableShorts(int index);
    int getNumTableInts(int index);
    void loadByteTable(int8_t* array, int index);
    int loadShortTable(short* array, int index);
    void loadIntTable(int32_t* array, int index);
    void loadUByteTable(uint8_t* array, int index);
    void loadUShortTable(uint16_t* array, int index);
};
