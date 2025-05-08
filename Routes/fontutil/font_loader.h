#pragma once
#ifndef FONT_LOADER_H
#define FONT_LOADER_H

#include <string>
#include <vector>

// fd0/fk0 ������ �о� RGBA �̹��� ���۷� ��ȯ
// outputBuffer: RGBA �̹��� (width * height * 4)
// imageWidth: ���� 2016
// imageHeight: ��� ���� ���� ������
bool load_font_file_to_image(const std::string& filepath, std::vector<unsigned char>& outputBuffer, int& imageWidth, int& imageHeight);

#endif