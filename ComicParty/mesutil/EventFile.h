#pragma once
#include <cstdint>
#include <cstddef>

struct EventBlocks {
    uint32_t index;
    const uint32_t* base; // points into file buffer (DWORD*)
    const uint32_t* end;  // one-past-last (DWORD*)
    size_t word_count;
};

struct EventFile {
    // Owned file bytes
    uint8_t* file_bytes;
    size_t file_size_bytes;

    // Parsed header info
    uint32_t block_count;
    uint32_t* offsets;        // owned copy of offsets table (DWORD offsets from code_base)
    const uint32_t* code_base; // points into file_bytes
    size_t code_words;

    EventFile();
    ~EventFile();

    bool Load(const wchar_t* path);
    void Unload();

    bool IsValid() const;

    bool GetBlock(uint32_t block_index, EventBlocks& out_block) const;

    // Convert a DWORD pointer within file buffer to file byte offset (for error reporting).
    bool PtrToFileByteOffset(const uint32_t* p, size_t& out_off) const;

private:
    bool ParseAndValidate();
};
