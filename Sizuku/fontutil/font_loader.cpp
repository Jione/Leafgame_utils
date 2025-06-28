#include "font_loader.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <cctype>
#include <cstring>

static int parse_block_size(const std::string& filename, bool& isFD0, int& fontNumber) {
    isFD0 = false;
    size_t base = filename.find_last_of("/\\");
    std::string name = (base == std::string::npos) ? filename : filename.substr(base + 1);

    if (name.length() < 7 || name.substr(0, 4) != "font")
        return -1;

    fontNumber = std::atoi(name.substr(4, 2).c_str());
    int size = fontNumber;
    std::string ext = name.substr(name.length() - 4, 3);
    if (ext == ".fd") {
        isFD0 = true;
        return size;
    }
    else if (ext == ".fk") {
        return (size < 20) ? (size + 2) : (size + 4);
    }
    return -1;
}

static void write_block_to_buffer(const unsigned char* blockData, int blockW, int blockH,
    std::vector<unsigned char>& image, int imageW, int index, bool isFD0, bool isSmall) {
    int tileW = isSmall ? 24 : 48;
    int tileH = 48;
    int blocksPerRow = (2016 / tileW);
    int localIndex = isSmall ? (index - 7023) : index;
    int row = localIndex / blocksPerRow;
    int col = localIndex % blocksPerRow;

    int ox = col * tileW + (tileW - blockW) / 2;
    int oy = isSmall ? ((7023 + 41) / 42) * 48 + row * tileH : row * tileH;
    oy += (tileH - blockH) / 2;

    for (int y = 0; y < blockH; ++y) {
        for (int x = 0; x < blockW; ++x) {
            int bitIndex = y * blockW + x;
            int byteIndex = bitIndex / 2;
            unsigned char packed = blockData[byteIndex];

            bool useHighNibble = (x % 2 == 1);
            unsigned char alpha4 = useHighNibble ? (packed >> 4) : (packed & 0x0F);
            unsigned char alpha = static_cast<unsigned char>(alpha4 * 17);

            int px = ox + x;
            int py = oy + y;
            int offset = (py * imageW + px) * 4;

            image[offset + 0] = isFD0 ? 255 : 0;
            image[offset + 1] = isFD0 ? 255 : 0;
            image[offset + 2] = isFD0 ? 255 : 0;
            image[offset + 3] = alpha;
        }
    }
}

bool load_font_file_to_image(const std::string& filepath, std::vector<unsigned char>& outputBuffer, int& imageWidth, int& imageHeight) {
    bool isFD0 = false;
    int fontNum = 0;
    int fullBlockSize = parse_block_size(filepath, isFD0, fontNum);
    int smallBlockSize = (fontNum / 2) + ((fontNum / 2) % 2) + (isFD0 ? 0 : (fontNum < 20) ? 2 : 4);
    if (fullBlockSize <= 0) {
        std::cerr << "[Error] Invalid filename or extension: " << filepath << "\n";
        return false;
    }

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "[Error] Failed to open file: " << filepath << "\n";
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (!isFD0) {
        file.seekg(4, std::ios::beg);
        fileSize -= 4;
    }

    int fullBlockBytes = (fullBlockSize * fullBlockSize) / 2;
    int smallBlockBytes = (smallBlockSize * fullBlockSize) / 2;

    int maxFullBlocks = static_cast<int>(fileSize) / fullBlockBytes;
    int smallBlockCount = 0;
    bool isOrigin = false;
    if (maxFullBlocks < 7168) {
        maxFullBlocks = std::min<int>(7023, maxFullBlocks);
        int remainingBytes = static_cast<int>(fileSize) - (maxFullBlocks * fullBlockBytes);
        smallBlockCount = remainingBytes / smallBlockBytes;
        isOrigin = true;
    }

    int totalBlocks = maxFullBlocks + smallBlockCount;

    int imageW = 2016;
    int fullRows = (maxFullBlocks + 41) / 42;
    int smallRows = (smallBlockCount + 83) / 84;
    int imageH = (fullRows + smallRows) * 48;
    std::cout << "[Info] Count blocks: Full-width " << maxFullBlocks << ", Half-width " << smallBlockCount << "\n";

    imageWidth = imageW;
    imageHeight = imageH;
    outputBuffer.assign(imageW * imageH * 4, 0);

    std::vector<unsigned char> blockBuf;

    for (int i = 0; i < totalBlocks; ++i) {
        bool isSmall = isOrigin && (i >= 7023);
        int blockW = isSmall ? smallBlockSize : fullBlockSize;
        int blockH = fullBlockSize;
        int blockBytes = isSmall ? smallBlockBytes : fullBlockBytes;

        blockBuf.resize(blockBytes);
        if (!file.read(reinterpret_cast<char*>(blockBuf.data()), blockBytes)) {
            std::cerr << "[Error] Unexpected EOF at block " << i << "\n";
            return false;
        }

        write_block_to_buffer(blockBuf.data(), blockW, blockH, outputBuffer, imageW, i, isFD0, isSmall);
    }

    return true;
}