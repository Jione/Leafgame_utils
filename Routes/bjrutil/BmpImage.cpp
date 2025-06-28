#include "BmpImage.h"
#include <fstream>

bool BmpImage::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    in.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    if (fileHeader.bfType != 0x4D42) return false; // "BM"
    in.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    // 전체 파일 크기에서 픽셀 시작 위치까지 건너뛰기
    in.seekg(fileHeader.bfOffBits, std::ios::beg);
    // 남은 바이트를 rawData에 로드
    rawData.assign(std::istreambuf_iterator<char>(in), {});
    return true;
}

bool BmpImage::save(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    out.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    out.write(reinterpret_cast<const char*>(rawData.data()), rawData.size());
    return true;
}