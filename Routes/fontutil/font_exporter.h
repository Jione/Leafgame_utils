#pragma once
#ifndef FONT_EXPORTER_H
#define FONT_EXPORTER_H

#include <string>

// PNG 파일을 읽어 .fd1 또는 .fk1로 저장
bool export_png_to_font_binary(const std::string& inputPngPath);

#endif