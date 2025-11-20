#pragma once

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

// External libraries
#include "lodepng.h"
#include "libimagequant.h"

#ifdef _AMD64_
#ifdef _DEBUG
#pragma comment(lib,"libimagequant-x64-dbg.lib")
#else
#pragma comment(lib,"libimagequant-x64-rel.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib,"libimagequant-x86-dbg.lib")
#else
#pragma comment(lib,"libimagequant-x86-rel.lib")
#endif
#endif

namespace fs = std::filesystem;

// Ensure 1-byte packing for binary file structures
#pragma pack(push, 1)

// C16 File Structure
struct C16Header {
    uint16_t uCount; // Index counter
};

struct ColorIndex {
    uint8_t IndexNum;
    uint8_t Blue;
    uint8_t Green;
    uint8_t Red;
};

// GRP File Structure (Header only, data follows)
struct GRPHeader {
    uint16_t posX;
    uint16_t posY;
    uint16_t sizeX;
    uint16_t sizeY;
};

// Font GRP Structure (Header only, data follows)
struct FontGRPHeader {
    uint16_t sizeX;
    uint16_t sizeY;
};

#pragma pack(pop)

enum class ConversionMode {
    ToPng_NoMsk = 1,
    ToPng_WithMsk,
    Update_Grp
};
