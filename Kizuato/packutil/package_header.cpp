#include "package_header.h"
#include <cstring>   // memset, strncpy

#pragma warning(disable: 4996)

std::vector<FileHeaderEntry> create_header_entries(const std::vector<FileEntry>& fileList) {
    std::vector<FileHeaderEntry> headers;

    for (const auto& f : fileList) {
        FileHeaderEntry entry;
        entry.fileType = 0x00000001;
        std::memset(entry.fileName, 0, sizeof(entry.fileName));
        std::strncpy(entry.fileName, f.fileName.c_str(), sizeof(entry.fileName) - 1);
        entry.offset = 0;
        entry.length = 0;
        headers.push_back(entry);
    }

    return headers;
}