#include "Crypto.h"
#include <windows.h>      // MultiByteToWideChar, WideCharToMultiByte
#include <vector>

// UTF-8 std::string → ShiftJIS(std::string)
static std::string toShiftJIS(const std::string& utf8) {
    // 1) UTF-8 → UTF-16
    int wcLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> wbuf(wcLen);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wbuf.data(), wcLen);
    // 2) UTF-16 → CP932(ShiftJIS)
    int sjLen = WideCharToMultiByte(932, 0, wbuf.data(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> sbuf(sjLen);
    WideCharToMultiByte(932, 0, wbuf.data(), -1, sbuf.data(), sjLen, nullptr, nullptr);
    return std::string(sbuf.data());
}

static void computeKeys(const std::string& name, int& lineKey, int& evenKey, int& oddKey) {
    auto sj = toShiftJIS(name);
    lineKey = 0;
    evenKey = 0xFF;
    oddKey = 0;
    for (uint8_t b : std::vector<uint8_t>(sj.begin(), sj.end())) {
        lineKey ^= b;
        evenKey += b;
        oddKey -= b;
    }
}

std::vector<uint8_t> decryptData(const std::vector<uint8_t>& data, const std::string& filename) {
    // filename: 원본 .bjr 파일명 (확장자 포함)
    int lineKey, evenKey, oddKey;
    computeKeys(filename, lineKey, evenKey, oddKey);

    // height, stride 값은 BmpImage.infoHeader로 미리 전달받아야 하지만
    // 여기서는 호출 전에 알맞게 rawData 크기/형식을 설정했다고 가정
    // 예) height = abs(biHeight), stride = ((width * bpp/8)+3)&~3
    extern int g_height, g_stride;
    std::vector<uint8_t> dst(data.size());
    size_t dstOff = 0;

    for (int y = 0; y < g_height; ++y) {
        lineKey += 7;
        int srcRow = (lineKey % g_height) * g_stride;
        for (int x = 0; x < g_stride; ++x) {
            uint8_t v = data[srcRow + x];
            if ((x & 1) == 0)
                dst[dstOff + x] = static_cast<uint8_t>(evenKey - v);
            else
                dst[dstOff + x] = static_cast<uint8_t>(oddKey + v);
        }
        dstOff += g_stride;
    }
    return dst;
}

std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data, const std::string& filename) {
    int lineKey, evenKey, oddKey;
    computeKeys(filename, lineKey, evenKey, oddKey);

    extern int g_height, g_stride;
    std::vector<uint8_t> dst(data.size());
    size_t dstOff = 0;

    for (int y = 0; y < g_height; ++y) {
        lineKey += 7;
        int srcRow = (lineKey % g_height) * g_stride;
        for (int x = 0; x < g_stride; ++x) {
            uint8_t p = data[dstOff + x];
            if ((x & 1) == 0)
                dst[srcRow + x] = static_cast<uint8_t>(evenKey - p);
            else
                dst[srcRow + x] = static_cast<uint8_t>(p - oddKey);
        }
        dstOff += g_stride;
    }
    return dst;
}