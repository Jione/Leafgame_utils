#pragma once
#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <string>

// .pak 파일을 풀어서 폴더에 저장
bool extract_package(const std::string& pakFilePath);

#endif