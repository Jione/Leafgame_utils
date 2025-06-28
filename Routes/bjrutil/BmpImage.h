#pragma once
#include <cstdint>
#include <vector>
#include <string>

#pragma pack(push,1)
struct BitmapFileHeader {
    uint16_t bfType;      // 항상 'BM' = 0x4D42
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;   // 픽셀 데이터 오프셋
};

struct BitmapInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

class BmpImage {
public:
    BitmapFileHeader fileHeader;
    BitmapInfoHeader infoHeader;
    std::vector<uint8_t> rawData;  // bfOffBits 이후의 픽셀·팔레트 등

    // BMP 파일 읽기
    bool load(const std::string& path);

    // BMP 파일 쓰기
    bool save(const std::string& path) const;
};