#pragma once
#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>

struct FileEntry {
    std::string relativePath;
    std::string fileName;
};

// 폴더 경로가 유효한지 확인
bool is_valid_directory(const std::string& path);

// 폴더명에서 base 이름 추출 후 .pak 확장자 생성
std::string get_output_filename(const std::string& folderPath);

// 폴더 내 모든 파일명을 name.ext 형식으로 반환
std::vector<FileEntry> list_all_files(const std::string& folderPath);

#endif