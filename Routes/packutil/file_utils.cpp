#include "file_utils.h"
#include "string_util.h"
#include <windows.h>
#include <fstream>
#include <set>
#include <iostream>

bool is_valid_directory(const std::string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

std::string get_output_filename(const std::string& folderPath) {
    size_t pos = folderPath.find_last_of("\\/");
    std::string base = (pos == std::string::npos) ? folderPath : folderPath.substr(pos + 1);
    return base + ".pak";
}

// 실제 재귀 탐색 (UTF-16 API 사용)
static void find_files_recursive(
    const std::wstring& basePath,
    const std::wstring& rootPath,
    std::vector<FileEntry>& files,
    std::set<std::string>& uniqueNames)
{
    std::wstring searchPath = basePath + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::wstring nameW = findData.cFileName;
        if (nameW == L"." || nameW == L"..") continue;

        std::wstring fullPathW = basePath + L"\\" + nameW;
        std::wstring relativeW = fullPathW.substr(rootPath.length() + 1);

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            find_files_recursive(fullPathW, rootPath, files, uniqueNames);
        }
        else {
            std::wifstream test(fullPathW, std::ios::binary);
            if (!test) {
                std::cerr << "[Error] Cannot open file: "
                    << wstringToString(relativeW) << "\n";
                exit(1);
            }
            test.close();

            // 멀티바이트로 변환
            std::string fileOnly = wstringToString(nameW);
            std::string relative = wstringToString(relativeW);

            if (uniqueNames.find(fileOnly) != uniqueNames.end()) {
                std::cerr << "[Warning] Duplicate filename ignored: "
                    << relative << "\n";
                continue;
            }

            uniqueNames.insert(fileOnly);
            files.push_back({ relativeW, nameW });
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

std::vector<FileEntry> list_all_files(const std::string& folderPath) {
    std::vector<FileEntry> files;
    std::set<std::string> uniqueNames;

    std::wstring cleanedPath = stringToWstring(folderPath);
    if (!cleanedPath.empty() && cleanedPath.back() == '\\')
        cleanedPath.pop_back();

    find_files_recursive(cleanedPath, cleanedPath, files, uniqueNames);
    return files;
}