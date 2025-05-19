#include "packager.h"
#include "file_utils.h"
#include "package_header.h"
#include "string_util.h"
#include "lzss.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>


// 헤더 전체를 출력 스트림에 기록
static bool write_package_header(std::ofstream& out, const std::vector<FileHeaderEntry>& headers) {
    uint32_t fileCount = static_cast<uint32_t>(headers.size());

    out.seekp(0, std::ios::beg);
    out.write(reinterpret_cast<char*>(&kPackageHeader.signature), sizeof(PackageHeader));
    out.write(reinterpret_cast<const char*>(&fileCount), 4);

    for (const auto& entry : headers) {
        out.write(reinterpret_cast<const char*>(&entry), sizeof(FileHeaderEntry));
    }

    return out.good();
}

// 메인 패키징 함수
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
    const std::streamoff headerSize = sizeof(PackageHeader) + 4 + headers.size() * sizeof(FileHeaderEntry);

    std::string outputPath = get_output_filename(folderPath);
    std::ofstream pkgFile(outputPath, std::ios::binary);
    if (!pkgFile) {
        std::cerr << "[Error] Failed to create package file.\n";
        return false;
    }

    pkgFile.seekp(headerSize, std::ios::beg);

    for (size_t i = 0; i < fileList.size(); ++i) {
        std::wstring fullPath = stringToWstring(folderPath);
        if (!folderPath.empty() && folderPath.back() != '\\')
            fullPath += L"\\";
        fullPath += fileList[i].relativePath;

        std::cout << "[Compressing] " << wstringToString(fileList[i].relativePath) << "\n";

        std::streampos offset = pkgFile.tellp();
        if (!lzss_compress_file_to_stream(fullPath, pkgFile)) {
            std::cerr << "[Error] Failed to compress: " << wstringToString(fileList[i].relativePath) << "\n";
            return false;
        }
        std::streampos after = pkgFile.tellp();

        headers[i].offset = static_cast<uint32_t>(offset);
        headers[i].length = static_cast<uint32_t>(after - offset);
    }

    int count = 0;
    for (int i = fileList.size() - 1; i >= 0; --i) {
        if (headers[i].length == 0) {
            headers[i].fileType = (count++ << 24) + 0x73006E;
        }
    }
    if (!write_package_header(pkgFile, headers)) {
        std::cerr << "[Error] Failed to write header.\n";
        return false;
    }

    pkgFile.close();
    std::cout << "[Success] Package created: " << outputPath << "\n\n";
    return true;
}