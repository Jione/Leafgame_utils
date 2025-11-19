#pragma once
#include "Common.h"

namespace LZSS {
    // Decompress from file handle to file path
    bool Decompress(HANDLE hSrcFile, const FileEntry* entry, LPCWSTR lpOutPath);

    // Compress memory buffer to memory buffer (Used by Archive Create)
    // Returns compressed size. If compression is worse, returns 0.
    size_t Compress(const std::vector<uint8_t>& src, std::vector<uint8_t>& dest);
}
