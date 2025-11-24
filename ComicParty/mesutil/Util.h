#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <map>

namespace Util {

    // Enum not strictly used for logic anymore, but kept for compatibility if needed
    enum class EncodingType {
        Auto,
        Utf8
    };

    // Open file dialog
    std::vector<std::wstring> GetMesFiles(LPWSTR lpExt);

    // Helper: Convert between Wide and MultiByte using specific CodePage
    std::string WideToMultiByteStr(const std::wstring& wstr, UINT codePage);
    std::wstring MultiByteToWideStr(const std::string& str, UINT codePage);

    // --- Core Parsing Logic ---

    // Converts raw MES binary data (with mixed encodings/control codes) to UTF-8 string for Excel
    std::string MesBytesToUtf8(const std::vector<char>& data, uint32_t offset, uint32_t len);

    // Converts UTF-8 string from Excel back to MES binary data
    std::vector<char> Utf8ToMesBytes(const std::string& utf8Str);
}