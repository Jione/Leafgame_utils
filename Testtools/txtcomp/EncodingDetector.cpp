#include "EncodingDetector.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <windows.h>

Encoding detectEncoding(const std::string& filePath) {
    std::ifstream fs(filePath, std::ios::binary);
    if (!fs) throw std::runtime_error("Cannot open file: " + filePath);

    unsigned char bom[3] = { 0 };
    fs.read(reinterpret_cast<char*>(bom), 3);
    if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)
        return Encoding::UTF8;
    if (bom[0] == 0xFF && bom[1] == 0xFE)
        return Encoding::UTF16LE;
    if (bom[0] == 0xFE && bom[1] == 0xFF)
        return Encoding::UTF16BE;
    return Encoding::ANSI; // BOM 없으면 ANSI
}

std::wstring readFileAsUTF16(const std::string& filePath) {
    Encoding enc = detectEncoding(filePath);
    std::ifstream fs(filePath, std::ios::binary);
    if (!fs) throw std::runtime_error("Cannot open file: " + filePath);

    std::vector<char> buffer((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    size_t offset = 0;
    if (enc == Encoding::UTF8 && buffer.size() >= 3) {
        offset = 3; // skip BOM
    }
    else if ((enc == Encoding::UTF16LE || enc == Encoding::UTF16BE) && buffer.size() >= 2) {
        offset = 2; // skip BOM
    }

    if (enc == Encoding::UTF16LE || enc == Encoding::UTF16BE) {
        std::wstring result;
        result.reserve((buffer.size() - offset) / 2);
        for (size_t i = offset; i + 1 < buffer.size(); i += 2) {
            wchar_t wch = static_cast<unsigned char>(buffer[i]) |
                (static_cast<unsigned char>(buffer[i + 1]) << 8);
            if (enc == Encoding::UTF16BE) {
                // swap bytes for BE
                wch = (wch >> 8) | (wch << 8);
            }
            result.push_back(wch);
        }
        return result;
    }

    // 멀티바이트 -> UTF-16 변환 (UTF-8 또는 ANSI)
    UINT codePage = (enc == Encoding::UTF8) ? CP_UTF8 : CP_ACP;
    std::cout << "Codepage:" << ((codePage == CP_UTF8) ? "CP_UTF8" : "CP_ACP") << "\n";
    int wideLen = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS,
        buffer.data() + offset,
        static_cast<int>(buffer.size() - offset),
        nullptr, 0);
    if (wideLen == 0) throw std::runtime_error("Conversion to UTF-16 failed for: " + filePath);
    std::wstring result(wideLen, L'\0');
    MultiByteToWideChar(codePage, 0,
        buffer.data() + offset,
        static_cast<int>(buffer.size() - offset),
        &result[0], wideLen);
    return result;
}