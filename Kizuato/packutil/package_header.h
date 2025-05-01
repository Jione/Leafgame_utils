#pragma once
#ifndef PACKAGE_HEADER_H
#define PACKAGE_HEADER_H

#include <string>
#include <vector>
#include <cstdint>
#include "file_utils.h"

// 파일 헤더 구조체 (총 0x24 = 36바이트)
struct FileHeaderEntry {
    uint32_t fileType;          // 0x00000001 고정
    char fileName[24];          // zero-padded name.ext
    uint32_t offset;            // 파일 시작 위치 (바이트)
    uint32_t length;            // 압축된 길이
};

// 전체 헤더 데이터 초기화 함수
std::vector<FileHeaderEntry> create_header_entries(const std::vector<FileEntry>& fileList);

#endif