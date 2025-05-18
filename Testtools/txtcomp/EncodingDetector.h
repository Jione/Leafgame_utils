// File: EncodingDetector.h
#pragma once
#include <string>
enum class Encoding { UTF8, UTF16LE, UTF16BE, ANSI };

// BOM 검사 및 인코딩 결정
Encoding detectEncoding(const std::string& filePath);

// 파일을 읽어 UTF-16 문자열로 반환
std::wstring readFileAsUTF16(const std::string& filePath);