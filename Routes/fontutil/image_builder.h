#pragma once
#ifndef IMAGE_BUILDER_H
#define IMAGE_BUILDER_H

#include <vector>
#include <string>

// RGBA �̹��� ���� ���� (����� ���� ����)
std::vector<unsigned char> create_empty_image(int width, int height);

// PNG ���Ϸ� ����
bool save_image_as_png(const std::string& filename, const std::vector<unsigned char>& image, int width, int height);

#endif