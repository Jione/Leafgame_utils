#pragma once
#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>

struct FileEntry {
    std::string relativePath;
    std::string fileName;
};

// ���� ��ΰ� ��ȿ���� Ȯ��
bool is_valid_directory(const std::string& path);

// �������� base �̸� ���� �� .pak Ȯ���� ����
std::string get_output_filename(const std::string& folderPath);

// ���� �� ��� ���ϸ��� name.ext �������� ��ȯ
std::vector<FileEntry> list_all_files(const std::string& folderPath);

#endif