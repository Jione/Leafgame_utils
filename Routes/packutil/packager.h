#pragma once
#ifndef PACKAGER_H
#define PACKAGER_H

#include <string>

// 주어진 폴더 내 파일들을 .pak 파일로 압축 패키징
bool create_package(const std::string& folderPath);

#endif