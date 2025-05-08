#pragma once
#ifndef IMAGE_BUILDER_H
#define IMAGE_BUILDER_H

#include <vector>
#include <string>

// RGBA 이미지 버퍼 생성 (배경은 완전 투명)
std::vector<unsigned char> create_empty_image(int width, int height);

// PNG 파일로 저장
bool save_image_as_png(const std::string& filename, const std::vector<unsigned char>& image, int width, int height);

#endif