#include "font_exporter.h"
#include "lodepng.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype>
#include <cstdlib>

static bool parse_filename(const std::string& filename, int& fontSize, bool& isFD0) {
    size_t base = filename.find_last_of("/\\");
    std::string name = (base == std::string::npos) ? filename : filename.substr(base + 1);

    if (name.length() < 12 || name.substr(0, 4) != "font")
        return false;

    fontSize = std::atoi(name.substr(4, 2).c_str());
    std::string ext = name.substr(6, 3); // ".fd" or ".fk"
    isFD0 = (ext == ".fd");
    return isFD0 || ext == ".fk";
}

static int get_block_width(bool isFD0, int fontSize, bool isHalf) {
    int baseW = fontSize;
    int offset = (baseW < 20) ? 2 : 4;
    baseW = isFD0 ? (isHalf ? (baseW / 2) : baseW) : (isHalf ? (baseW / 2 + offset) : (baseW + offset));
    return baseW + (baseW % 2);
}

bool export_png_to_font_binary(const std::string& inputPngPath) {
    int fontSize = 0;
    bool isFD0 = true;
    if (!parse_filename(inputPngPath, fontSize, isFD0)) {
        std::cerr << "[Error] Invalid filename format: " << inputPngPath << "\n";
        return false;
    }

    // PNG decode
    std::vector<unsigned char> image;
    unsigned width, height;
    unsigned error = lodepng::decode(image, width, height, inputPngPath);
    if (error) {
        std::cerr << "[Error] Failed to decode PNG: " << lodepng_error_text(error) << "\n";
        return false;
    }
    const int TILE = 48;
    const int blocksPerRow = width / TILE;
    const int rows = height / TILE;
    const bool isOrigin = (rows >= 169) && (rows <= 171) && (blocksPerRow == 42);
    const int totalBlocks = isOrigin ? 7023 + 157 : blocksPerRow * rows;
    const int fk0_offset = (fontSize < 20) ? 2 : 4;

    std::vector<unsigned char> outputBytes;

    // fk1일 경우 예약영역 4바이트 삽입
    if (!isFD0) {
        outputBytes.push_back((fontSize < 20) ? 0x01 : 0x02);
        outputBytes.push_back(0x00);
        outputBytes.push_back(0x00);
        outputBytes.push_back(0x00);
    }

    for (int i = 0; i < totalBlocks; ++i) {
        bool isHalf = isOrigin && (i > 7022);
        int blockW = get_block_width(isFD0, fontSize, isHalf);
        int blockH = isFD0 ? fontSize : fontSize + fk0_offset;

        int col = isHalf ? ((i - 7023) % (blocksPerRow * 2)) : (i % blocksPerRow);
        int row = isHalf ? (168 + ((i - 7023) / (blocksPerRow * 2))) : (i / blocksPerRow);
        int ox = isHalf ? (col * (TILE / 2) + ((TILE / 2) - blockW) / 2) : (col * TILE + (TILE - blockW) / 2);
        int oy = row * TILE + (TILE - blockH) / 2;

        std::vector<unsigned char> packed((blockW * blockH + 1) / 2, 0);

        for (int y = 0; y < blockH; ++y) {
            for (int x = 0; x < blockW; ++x) {
                int px = ox + x;
                int py = oy + y;
                int offset = (py * width + px) * 4;
                unsigned char alpha8 = image[offset + 3];
                unsigned char alpha4 = (alpha8 + 8) / 17;
                if (alpha4 > 15) alpha4 = 15;

                int index = y * blockW + x;
                if (index % 2 != 0) {
                    packed[index / 2] |= (alpha4 << 4); // high
                }
                else {
                    packed[index / 2] |= (alpha4 & 0x0F); // low
                }
            }
        }

        outputBytes.insert(outputBytes.end(), packed.begin(), packed.end());
    }

    // 출력 경로
    std::string outPath = inputPngPath.substr(0, inputPngPath.length() - 5); // remove 0.png
    int i = stoi(inputPngPath.substr(inputPngPath.length() - 5, inputPngPath.length() - 4));
    outPath += std::to_string(i + 1); // .fd1 or .fk1
    
    std::ofstream out(outPath, std::ios::binary);
    if (!out) {
        std::cerr << "[Error] Failed to create output: " << outPath << "\n";
        return false;
    }
    out.write(reinterpret_cast<const char*>(outputBytes.data()), outputBytes.size());
    out.close();

    std::cout << "[Success] Saved font binary: " << outPath << "\n\n";
    return true;
}