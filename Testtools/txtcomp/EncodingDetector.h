// File: EncodingDetector.h
#pragma once
#include <string>
enum class Encoding { UTF8, UTF16LE, UTF16BE, ANSI };

// BOM �˻� �� ���ڵ� ����
Encoding detectEncoding(const std::string& filePath);

// ������ �о� UTF-16 ���ڿ��� ��ȯ
std::wstring readFileAsUTF16(const std::string& filePath);