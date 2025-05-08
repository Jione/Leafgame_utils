#include "file_utils.h"
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

static void find_files_recursive(const std::string& basePath, const std::string& rootPath,
    std::vector<FileEntry>& files, std::set<std::string>& uniqueNames) {
    std::string searchPath = basePath + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string name = findData.cFileName;
        if (name == "." || name == "..") continue;

        std::string fullPath = basePath + "\\" + name;
        std::string relative = fullPath.substr(rootPath.length() + 1); // ex: sub\abc.txt

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            find_files_recursive(fullPath, rootPath, files, uniqueNames);
        }
        else {
            std::ifstream test(fullPath, std::ios::binary);
            if (!test) {
                std::cerr << "[Error] Cannot open file for reading: " << relative << "\n";
                exit(1);
            }
            test.close();

            std::string fileOnly;
            size_t pos = name.find_last_of("\\/");
            fileOnly = (pos == std::string::npos) ? name : name.substr(pos + 1);

            if (uniqueNames.find(fileOnly) != uniqueNames.end()) {
                std::cerr << "[Warning] Duplicate filename ignored: " << relative << "\n";
                continue;
            }

            uniqueNames.insert(fileOnly);
            files.push_back({ relative, fileOnly });
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

std::vector<FileEntry> list_all_files(const std::string& folderPath) {
    std::vector<FileEntry> files;
    std::set<std::string> uniqueNames;

    std::string cleanedPath = folderPath;
    if (!cleanedPath.empty() && cleanedPath.back() == '\\')
        cleanedPath.pop_back();

    find_files_recursive(cleanedPath, cleanedPath, files, uniqueNames);
    return files;
}