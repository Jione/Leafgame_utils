#include "extractor.h"
#include "package_header.h"
#include "lzss.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>


#if __cplusplus < 201703L
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

static const uint32_t kExpectedSignature = 0x5041434B; // "KCAP"
static const uint32_t kExpectedReserved = 0x00000009;

bool extract_package(const std::string& pakFilePath) {
    std::ifstream inFile(pakFilePath, std::ios::binary);
    if (!inFile) {
        std::cerr << "[Error] Cannot open pak file: " << pakFilePath << "\n";
        return false;
    }

    uint32_t signature = 0, reserved = 0, fileCount = 0;
    inFile.read(reinterpret_cast<char*>(&signature), 4);
    inFile.read(reinterpret_cast<char*>(&reserved), 4);
    inFile.read(reinterpret_cast<char*>(&fileCount), 4);

    if (signature != kExpectedSignature) {
        std::cerr << "[Error] Invalid PAK file header.\n";
        return false;
    }

    std::vector<FileHeaderEntry> entries(fileCount);
    inFile.read(reinterpret_cast<char*>(entries.data()), fileCount * sizeof(FileHeaderEntry));

    // Create output folder: same as pakFilePath without .pak
    std::string folderName = pakFilePath;
    size_t pos = folderName.find_last_of("/\\");
    std::string baseName = (pos != std::string::npos) ? folderName.substr(pos + 1) : folderName;
    size_t ext = baseName.find_last_of('.');
    if (ext != std::string::npos)
        baseName = baseName.substr(0, ext);

    fs::create_directory(baseName);

    for (const auto& entry : entries) {
        if (entry.fileType != 0x00000001) {
            std::cout << "[Skip] Unsupported file type for: " << entry.fileName << "\n";
            continue;
        }

        std::string outputPath = baseName + "/" + std::string(entry.fileName);
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile) {
            std::cerr << "[Error] Failed to create: " << outputPath << "\n";
            return false;
        }

        inFile.seekg(entry.offset, std::ios::beg);
        std::vector<char> compressed(entry.length);
        inFile.read(compressed.data(), entry.length);

        std::istringstream compressedStream(std::string(compressed.begin(), compressed.end()));
        if (!lzss_decompress_stream(compressedStream, outFile)) {
            std::cerr << "[Error] Failed to decompress: " << entry.fileName << "\n";
            return false;
        }

        std::cout << "[Extracted] " << entry.fileName << "\n";
    }

    std::cout << "[Success] Package extracted to folder: " << baseName << "\n\n";
    return true;
}