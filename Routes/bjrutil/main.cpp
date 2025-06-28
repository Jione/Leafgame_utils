#include "BmpImage.h"
#include "Crypto.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <fstream>

#if __cplusplus < 201703L
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

// 전역으로 height/stride 세팅
int g_height = 0;
int g_stride = 0;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: bjrtool <input.bmp|input.bjr>\n";
        return 1;
    }

    std::string inPath = argv[1];
    auto ext = fs::path(inPath).extension().string();
    bool isLower = ((ext.length() != 0) && ('a' <= ext.c_str()[1]));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    BmpImage img;
    if (ext == ".bmp") {
        // --- 암호화 (BMP → BJR) ---
        if (!img.load(inPath)) {
            std::cerr << "BMP load failed\n";
            return 1;
        }
        g_height = std::abs(img.infoHeader.biHeight);
        int bpp = img.infoHeader.biBitCount;
        g_stride = ((img.infoHeader.biWidth * (bpp / 8)) + 3) & ~3;
        auto repExt = isLower ? ".bjr" : ".BJR";
        auto encrypted = encryptData(img.rawData, fs::path(inPath).replace_extension(repExt).filename().string());
        auto outPath = fs::path(inPath).replace_extension(repExt).string();
        std::ofstream out(outPath, std::ios::binary);
        out.write(reinterpret_cast<char*>(&img.fileHeader), sizeof(img.fileHeader));
        out.write(reinterpret_cast<char*>(&img.infoHeader), sizeof(img.infoHeader));
        out.write(reinterpret_cast<char*>(encrypted.data()), encrypted.size());
        std::cout << "Encrypted → " << outPath << "\n";

    }
    else if (ext == ".bjr") {
        // --- 복호화 (BJR → BMP) ---
        if (!img.load(inPath)) {
            std::cerr << "BJR load failed\n";
            return 1;
        }
        g_height = std::abs(img.infoHeader.biHeight);
        int bpp = img.infoHeader.biBitCount;
        g_stride = ((img.infoHeader.biWidth * (bpp / 8)) + 3) & ~3;

        auto decrypted = decryptData(img.rawData, fs::path(inPath).filename().string());

        auto repExt = isLower ? ".bmp" : ".BMP";
        auto outPath = fs::path(inPath).replace_extension(repExt).string();
        std::ofstream out(outPath, std::ios::binary);
        out.write(reinterpret_cast<char*>(&img.fileHeader), sizeof(img.fileHeader));
        out.write(reinterpret_cast<char*>(&img.infoHeader), sizeof(img.infoHeader));
        out.write(reinterpret_cast<char*>(decrypted.data()), decrypted.size());
        std::cout << "Decrypted → " << outPath << "\n";

    }
    else {
        std::cerr << "Unsupported extension: " << ext << "\n";
        return 1;
    }

    return 0;
}