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
#include <algorithm>
#include <cwctype>


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

// 확장자가 wav, ogg, ogga, oggb 중 하나면 true, 아니면 false
static bool HasValidAudioExtension(const std::wstring& path) {
    // 마지막 '.' 위치 찾기
    const size_t pos = path.find_last_of(L'.');
    if (pos == std::wstring::npos || pos + 1 >= path.size()) {
        return false;
    }

    // 확장자 부분(substr) 추출
    std::wstring ext = path.substr(pos + 1);

    // 모두 소문자로 변환
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](wchar_t c) { return std::towlower(c); });

    // 허용된 확장자 목록과 비교
    return (ext == L"wav" ||
        ext == L"ogg" ||
        ext == L"ogga" ||
        ext == L"oggb");
}

static bool copy_file_to_stream(const std::wstring& inputPath, std::ofstream& ofs) {
    // 1) wstring → UTF-8 narrow path
    std::string path = wstringToString(inputPath);

    // 2) 바이너리 모드로 파일 열기
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        // 열기 실패
        return false;
    }

    // 3) 스트림 버퍼를 이용해 전체 복사
    ofs << ifs.rdbuf();

    // 4) 복사 성공 여부 반환
    return static_cast<bool>(ofs);
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

        std::streampos offset = pkgFile.tellp();
        if (HasValidAudioExtension(fileList[i].relativePath)) {
            headers[i].fileType = 0;
            std::cout << "[AudioFile] " << wstringToString(fileList[i].relativePath) << "\n";
            if (!copy_file_to_stream(fullPath, pkgFile)) {
                std::cerr << "[Error] Failed to read file: " << wstringToString(fileList[i].relativePath) << "\n";
                return false;
            }
        }
        else {
            std::cout << "[Compressing] " << wstringToString(fileList[i].relativePath) << "\n";
            if (!lzss_compress_file_to_stream(fullPath, pkgFile)) {
                std::cerr << "[Error] Failed to compress: " << wstringToString(fileList[i].relativePath) << "\n";
                return false;
            }
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