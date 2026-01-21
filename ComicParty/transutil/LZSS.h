#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <cstdint>

// Alignment is crucial for binary file headers
#pragma pack(push, 1)
struct FileEntry {
    char Name[16];             // Shift-JIS Encoded
    uint32_t DataSize;
    uint32_t Flags;            // 0: Raw, 1: LZSS Compressed
    uint32_t DataOffset;
};
#pragma pack(pop)

// Constants based on the decompiled logic (N=2048)
namespace LZSS_Const {
    const int N = 2048; // Ring buffer size (modified from 4096 to match target)
    const int F = 18;   // Upper limit for match_length
    const int THRESHOLD = 2;
}

namespace LZSS {
    // Decompress from file handle to file path
    bool Decompress(HANDLE hSrcFile, const FileEntry* entry, std::vector<uint8_t>& outBuffer);

    // Decompress from memory buffer
    bool Decompress(const std::vector<uint8_t>& pakBuffer, const FileEntry* entry, std::vector<uint8_t>& outBuffer);

    // Compress memory buffer to memory buffer (Used by Archive Create)
    // Returns compressed size. If compression is worse, returns 0.
    size_t Compress(const std::vector<uint8_t>& src, std::vector<uint8_t>& dest);
}
