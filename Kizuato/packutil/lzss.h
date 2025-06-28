#pragma once
#ifndef LZSS_H
#define LZSS_H

#include <string>
#include <ostream>
#include <istream>

// LZSS 압축 / 압축 해제 전역 설정
const char initFillChar = 0x20;
const bool isPrefillCompress = false;
const bool isXorData = false;
const bool isRotateFlag = false;
const bool isShiftFlagData = false;
const char initShiftChar[5] = "DACB";

// 입력 파일을 LZSS 방식으로 압축하여 지정된 출력 스트림에 기록
bool lzss_compress_file_to_stream(const std::wstring& inputFilePath, std::ostream& outputStream);

// LZSS 압축 스트림을 해제하여 출력 스트림에 기록
bool lzss_decompress_stream(std::istream& inputStream, std::ostream& outputStream);

#endif