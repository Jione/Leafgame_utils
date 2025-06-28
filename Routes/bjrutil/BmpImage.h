#pragma once
#include <cstdint>
#include <vector>
#include <string>

#pragma pack(push,1)
struct BitmapFileHeader {
    uint16_t bfType;      // �׻� 'BM' = 0x4D42
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;   // �ȼ� ������ ������
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
    std::vector<uint8_t> rawData;  // bfOffBits ������ �ȼ����ȷ�Ʈ ��

    // BMP ���� �б�
    bool load(const std::string& path);

    // BMP ���� ����
    bool save(const std::string& path) const;
};