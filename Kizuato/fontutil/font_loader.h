#pragma once
#ifndef FONT_LOADER_H
#define FONT_LOADER_H

#include <string>
#include <vector>

// fd0/fk0 파일을 읽어 RGBA 이미지 버퍼로 변환
// outputBuffer: RGBA 이미지 (width * height * 4)
// imageWidth: 고정 2016
// imageHeight: 블록 수에 따라 결정됨
bool load_font_file_to_image(const std::string& filepath, std::vector<unsigned char>& outputBuffer, int& imageWidth, int& imageHeight);

#endif