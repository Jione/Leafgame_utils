#pragma once
#ifndef PACKAGE_HEADER_H
#define PACKAGE_HEADER_H

#include <string>
#include <vector>
#include <cstdint>
#include "file_utils.h"

// ���� ��� ����ü (�� 0x24 = 36����Ʈ)
struct FileHeaderEntry {
    uint32_t fileType;          // 0x00000001 ����
    char fileName[24];          // zero-padded name.ext
    uint32_t offset;            // ���� ���� ��ġ (����Ʈ)
    uint32_t length;            // ����� ����
};

// ��ü ��� ������ �ʱ�ȭ �Լ�
std::vector<FileHeaderEntry> create_header_entries(const std::vector<FileEntry>& fileList);

#endif