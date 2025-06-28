#include "encoding_converter.h"
#include <windows.h>
#include <vector>

// 내부 유틸: MultiByte -> WideChar 변환
static std::wstring multibyte_to_wide(const std::string& str, UINT codePage) {
    int wideLen = MultiByteToWideChar(codePage, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0);
    if (wideLen == 0) return L"";

    std::wstring wideStr(wideLen, 0);
    MultiByteToWideChar(codePage, 0, str.c_str(), static_cast<int>(str.size()), &wideStr[0], wideLen);
    return wideStr;
}

// 내부 유틸: WideChar -> MultiByte 변환
static std::string wide_to_multibyte(const std::wstring& wstr, UINT codePage) {
    int mbLen = WideCharToMultiByte(codePage, 0, wstr.c_str(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
    if (mbLen == 0) return "";

    std::string mbStr(mbLen, 0);
    WideCharToMultiByte(codePage, 0, wstr.c_str(), static_cast<int>(wstr.size()), &mbStr[0], mbLen, NULL, NULL);
    return mbStr;
}

// Shift-JIS → UTF-8
std::string shiftjis_to_utf8(const std::string& shiftJisText) {
    std::wstring wideStr = multibyte_to_wide(shiftJisText, 932); // 932 = Shift-JIS 코드페이지
    return wide_to_multibyte(wideStr, CP_UTF8);
}

// UTF-8 → Shift-JIS
std::string utf8_to_shiftjis(const std::string& utf8Text) {
    std::wstring wideStr = multibyte_to_wide(utf8Text, CP_UTF8);
    return wide_to_multibyte(wideStr, 932);
}

// EUC-KR → UTF-8
std::string euckr_to_utf8(const std::string& eucKrText) {
    std::wstring wideStr = multibyte_to_wide(eucKrText, 949); // 949 = EUC-KR (Korean) 코드페이지
    return wide_to_multibyte(wideStr, CP_UTF8);
}

// UTF-8 → EUC-KR
std::string utf8_to_euckr(const std::string& utf8Text) {
    std::wstring wideStr = multibyte_to_wide(utf8Text, CP_UTF8);
    return wide_to_multibyte(wideStr, 949);
}