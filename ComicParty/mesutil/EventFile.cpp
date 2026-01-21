#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <new>
#include <cstdio>
#include "EventFile.h"

static bool ReadFileAll(const wchar_t* path, uint8_t** out_buf, size_t* out_size) {
    *out_buf = nullptr;
    *out_size = 0;

    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        printf("Error: CreateFileW failed gle=%lu\n", (unsigned long)GetLastError());
        return false;
    }

    LARGE_INTEGER li{};
    if (!GetFileSizeEx(h, &li)) {
        printf("Error: GetFileSizeEx failed gle=%lu\n", (unsigned long)GetLastError());
        CloseHandle(h);
        return false;
    }
    if (li.QuadPart <= 0 || li.QuadPart > (LONGLONG)SIZE_MAX) {
        printf("Error: invalid file size\n");
        CloseHandle(h);
        return false;
    }

    size_t size = (size_t)li.QuadPart;
    uint8_t* buf = new (std::nothrow) uint8_t[size];
    if (!buf) {
        printf("Error: OOM size=%zu\n", size);
        CloseHandle(h);
        return false;
    }

    DWORD total = 0;
    while (total < size) {
        DWORD to_read = (DWORD)((size - total) > 0x7fffffff ? 0x7fffffff : (size - total));
        DWORD now = 0;
        if (!ReadFile(h, buf + total, to_read, &now, nullptr)) {
            printf("Error: ReadFile failed gle=%lu\n", (unsigned long)GetLastError());
            delete[] buf;
            CloseHandle(h);
            return false;
        }
        if (now == 0) break;
        total += now;
    }
    CloseHandle(h);

    if (total != size) {
        printf("Error: incomplete read read=%lu expected=%zu\n", (unsigned long)total, size);
        delete[] buf;
        return false;
    }

    *out_buf = buf;
    *out_size = size;
    return true;
}

EventFile::EventFile()
    : file_bytes(nullptr),
    file_size_bytes(0),
    block_count(0),
    offsets(nullptr),
    code_base(nullptr),
    code_words(0)
{
}

EventFile::~EventFile() {
    Unload();
}

void EventFile::Unload() {
    delete[] offsets;
    offsets = nullptr;

    delete[] file_bytes;
    file_bytes = nullptr;

    file_size_bytes = 0;
    block_count = 0;
    code_base = nullptr;
    code_words = 0;
}

bool EventFile::Load(const wchar_t* path) {
    Unload();
    if (!ReadFileAll(path, &file_bytes, &file_size_bytes)) return false;
    return ParseAndValidate();
}

bool EventFile::IsValid() const {
    return file_bytes != nullptr && offsets != nullptr && code_base != nullptr && block_count > 0;
}

bool EventFile::ParseAndValidate() {
    // Must be DWORD aligned size for clean parsing; allow tail but do not include it in code_words.
    if (!file_bytes || file_size_bytes < 4) {
        printf("Error: file too small\n");
        return false;
    }

    size_t aligned_bytes = (file_size_bytes / 4) * 4;
    if (aligned_bytes < 4) {
        printf("Error: aligned_bytes too small\n");
        return false;
    }

    const uint32_t* u32 = (const uint32_t*)file_bytes;
    size_t total_words = aligned_bytes / 4;

    uint32_t n = u32[0];
    if (n == 0) {
        printf("Error: block_count is zero\n");
        return false;
    }

    size_t header_words = 1ull + (size_t)n;
    if (header_words > total_words) {
        printf("Error: header out of range header_words=%zu total_words=%zu\n", header_words, total_words);
        return false;
    }

    offsets = new (std::nothrow) uint32_t[n];
    if (!offsets) {
        printf("Error: OOM offsets n=%u\n", n);
        return false;
    }

    for (uint32_t i = 0; i < n; ++i) offsets[i] = u32[1 + i];

    const uint32_t* code = u32 + header_words;
    size_t cw = total_words - header_words;

    // Validate monotonic offsets within code_words.
    for (uint32_t i = 0; i < n; ++i) {
        size_t off = (size_t)offsets[i];
        if (off > cw) {
            printf("Error: offsets[%u]=%u out of code_words=%zu\n", i, offsets[i], cw);
            return false;
        }
        if (i > 0 && offsets[i - 1] > offsets[i]) {
            printf("Error: offsets not non-decreasing at i=%u prev=%u cur=%u\n",
                i, offsets[i - 1], offsets[i]);
            return false;
        }
    }

    block_count = n;
    code_base = code;
    code_words = cw;
    return true;
}

bool EventFile::GetBlock(uint32_t block_index, EventBlocks& out_block) const {
    out_block.index = block_index;
    out_block.base = nullptr;
    out_block.end = nullptr;
    out_block.word_count = 0;

    if (!IsValid()) return false;
    if (block_index >= block_count) return false;

    size_t start_w = (size_t)offsets[block_index];
    size_t end_w = (block_index + 1 < block_count) ? (size_t)offsets[block_index + 1] : code_words;

    if (end_w < start_w || end_w > code_words) return false;

    out_block.base = code_base + start_w;
    out_block.end = code_base + end_w;
    out_block.word_count = end_w - start_w;
    return true;
}

bool EventFile::PtrToFileByteOffset(const uint32_t* p, size_t& out_off) const {
    out_off = 0;
    if (!file_bytes || !p) return false;

    const uint8_t* pb = (const uint8_t*)p;
    const uint8_t* fb = (const uint8_t*)file_bytes;

    if (pb < fb || pb > fb + file_size_bytes) return false;
    out_off = (size_t)(pb - fb);
    return true;
}
