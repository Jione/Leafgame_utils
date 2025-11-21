#pragma once
#include <Windows.h>
#include <string>
#include <vector>

namespace Util {
    // Enumeration for encoding types
    enum class EncodingType {
        ACP,        // CP_ACP
        ShiftJIS,   // 932
        UTF8,       // 65001
        CustomUTF8  // User defined logic
    };

    // Open file dialog to select multiple files
    std::vector<std::wstring> GetMesFiles(LPWSTR lpExt);

    // Convert string encoding
    std::string StringConvert(const std::string& src, EncodingType srcEnc, EncodingType targetEnc);

    // Custom UTF-8 logic: XOR 0x40 on first byte if it matches specific ranges
    void EncodeCustomUTF8(std::string& text);

    // Helper: Get raw codepage ID
    UINT GetCodePageID(EncodingType type);

    bool GetSjisException(unsigned char lead, unsigned char trail, wchar_t& outUnicode);

    bool GetUnicodeException(wchar_t unicode, unsigned char& outLead, unsigned char& outTrail);
}
