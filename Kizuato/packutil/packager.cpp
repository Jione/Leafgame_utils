#include "packager.h"
#include "file_utils.h"
#include "package_header.h"
#include "lzss.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>

// �ñ״�ó �� ���ప ����
static const uint32_t kSignature = 0x5041434B; // "KCAP"
static const uint32_t kReserved = 0x00000009;

// ��� ��ü�� ��� ��Ʈ���� ���
static bool write_package_header(std::ofstream& out, const std::vector<FileHeaderEntry>& headers) {
    uint32_t fileCount = static_cast<uint32_t>(headers.size());

    out.seekp(0, std::ios::beg);
    out.write(reinterpret_cast<const char*>(&kSignature), 4);
    out.write(reinterpret_cast<const char*>(&kReserved), 4);
    out.write(reinterpret_cast<const char*>(&fileCount), 4);

    for (const auto& entry : headers) {
        out.write(reinterpret_cast<const char*>(&entry), sizeof(FileHeaderEntry));
    }

    return out.good();
}

// ���� ��Ű¡ �Լ�
bool create_package(const std::string& folderPath) {
    if (!is_valid_directory(folderPath)) {
        std::cerr << "[Error] Invalid folder path.\n";
        return false;
    }

    std::vector<FileEntry> fileList = list_all_files(folderPath);
    if (fileList.empty()) {
        std::cerr << "[Error] No files found.\n";
        return false;
    }

    std::vector<FileHeaderEntry> headers = create_header_entries(fileList);
    const std::streamoff headerSize = 12 + headers.size() * sizeof(FileHeaderEntry);

    std::string outputPath = get_output_filename(folderPath);
    std::ofstream pkgFile(outputPath, std::ios::binary);
    if (!pkgFile) {
        std::cerr << "[Error] Failed to create package file.\n";
        return false;
    }

    pkgFile.seekp(headerSize, std::ios::beg);

    for (size_t i = 0; i < fileList.size(); ++i) {
        std::string fullPath = folderPath;
        if (!folderPath.empty() && folderPath.back() != '\\')
            fullPath += "\\";
        fullPath += fileList[i].relativePath;

        std::cout << "[Compressing] " << fileList[i].relativePath << "\n";

        std::streampos offset = pkgFile.tellp();
        if (!lzss_compress_file_to_stream(fullPath, pkgFile)) {
            std::cerr << "[Error] Failed to compress: " << fileList[i].relativePath << "\n";
            return false;
        }
        std::streampos after = pkgFile.tellp();

        headers[i].offset = static_cast<uint32_t>(offset);
        headers[i].length = static_cast<uint32_t>(after - offset);
    }

    if (!write_package_header(pkgFile, headers)) {
        std::cerr << "[Error] Failed to write header.\n";
        return false;
    }

    pkgFile.close();
    std::cout << "[Success] Package created: " << outputPath << "\n\n";
    return true;
}