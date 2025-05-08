#pragma once
#ifndef LZSS_H
#define LZSS_H

#include <string>
#include <ostream>
#include <istream>

// �Է� ������ LZSS ������� �����Ͽ� ������ ��� ��Ʈ���� ���
bool lzss_compress_file_to_stream(const std::string& inputFilePath, std::ostream& outputStream);

// LZSS ���� ��Ʈ���� �����Ͽ� ��� ��Ʈ���� ���
bool lzss_decompress_stream(std::istream& inputStream, std::ostream& outputStream);

#endif