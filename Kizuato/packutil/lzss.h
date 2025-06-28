#pragma once
#ifndef LZSS_H
#define LZSS_H

#include <string>
#include <ostream>
#include <istream>

// LZSS ���� / ���� ���� ���� ����
const char initFillChar = 0x20;
const bool isPrefillCompress = false;
const bool isXorData = false;
const bool isRotateFlag = false;
const bool isShiftFlagData = false;
const char initShiftChar[5] = "DACB";

// �Է� ������ LZSS ������� �����Ͽ� ������ ��� ��Ʈ���� ���
bool lzss_compress_file_to_stream(const std::wstring& inputFilePath, std::ostream& outputStream);

// LZSS ���� ��Ʈ���� �����Ͽ� ��� ��Ʈ���� ���
bool lzss_decompress_stream(std::istream& inputStream, std::ostream& outputStream);

#endif