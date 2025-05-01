#pragma once
#ifndef LZSS_H
#define LZSS_H

#include <string>
#include <ostream>
#include <istream>

// 입력 파일을 LZSS 방식으로 압축하여 지정된 출력 스트림에 기록
bool lzss_compress_file_to_stream(const std::string& inputFilePath, std::ostream& outputStream);

// LZSS 압축 스트림을 해제하여 출력 스트림에 기록
bool lzss_decompress_stream(std::istream& inputStream, std::ostream& outputStream);

#endif