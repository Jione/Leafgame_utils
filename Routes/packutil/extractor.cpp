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

bool extract_package(const std::string& pakFilePath) {
    std::ifstream inFile(pakFilePath, std::ios::binary);
    if (!inFile) {
        std::cerr << "[Error] Cannot open pak file: " << pakFilePath << "\n";
        return false;
    }

    uint32_t fileCount = 0;
    PackageHeader packHeader;
    inFile.read(reinterpret_cast<char*>(&packHeader.signature), sizeof(PackageHeader));
    if (packHeader.signature != kPackageHeader.signature) {
        std::cerr << "[Error] Invalid PAK file header.\n";
        return false;
    }

    inFile.read(reinterpret_cast<char*>(&fileCount), 4);
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
        if (entry.fileType == 0xCCCCCCCC) {
            std::cout << "[Skip] Unsupported file type for: " << entry.fileName << "\n";
            continue;
        }

        if (entry.length == 0) {
            bool isAscii = true;
            for (const auto& chr : entry.fileName) {
                isAscii = (isAscii && ((unsigned char)chr < 128));
            }
            if (!isAscii) {
                std::cout << "[Skip] Unsupported file type for: " << entry.fileName << "\n";
                continue;
            }
        }

        std::string outputPath = baseName + "/" + std::string(entry.fileName);
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile) {
            std::cerr << "[Error] Failed to create: " << outputPath << "\n";
            //return false;
            continue;
        }

        if (entry.length == 0) {
            std::cout << "[Extracted] Empty file created: " << entry.fileName << "\n";
            continue;
        }

        inFile.seekg(entry.offset, std::ios::beg);
        std::vector<char> compressed(entry.length);
        inFile.read(compressed.data(), entry.length);
        std::istringstream compressedStream(std::string(compressed.begin(), compressed.end()));

        if (entry.fileType == 0x00000000) {
            outFile << compressedStream.str();
        }
        else if (entry.fileType == 0x00000001) {
            if (!lzss_decompress_stream(compressedStream, outFile)) {
                std::cerr << "[Error] Failed to decompress: " << entry.fileName << "\n";
                return false;
            }
        }
        outFile.close();
        std::cout << "[Extracted] " << entry.fileName << "\n";
    }

    std::cout << "[Success] Package extracted to folder: " << baseName << "\n\n";
    return true;
}