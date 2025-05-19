#include "package_header.h"
#include "string_util.h"
#include <windows.h>
#include <cstring>   // memset, strncpy

#pragma warning(disable: 4996)

std::vector<FileHeaderEntry> create_header_entries(const std::vector<FileEntry>& fileList) {
    std::vector<FileHeaderEntry> headers;

    for (const auto& f : fileList) {
        FileHeaderEntry entry;
        std::string sjisName = wstringToShiftJis(f.fileName);
        entry.fileType = 0x00000001;
        std::memset(entry.fileName, 0, sizeof(entry.fileName));
        std::strncpy(entry.fileName, sjisName.c_str(), sizeof(entry.fileName) - 1);
        entry.offset = 0;
        entry.length = 0;
        headers.push_back(entry);
    }

    return headers;
}