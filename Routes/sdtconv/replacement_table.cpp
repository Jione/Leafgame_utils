#include "replacement_table.h"
#include <unordered_map>
#include <string>
#include <windows.h>

// 공통 테이블
static const struct {
    uint16_t hexCode;
    const char* replacement; // ANSI 인코딩 문자열 리터럴 (컴파일 시 기준)
} rawReplacementTable[] = {
    { 0xF040, "[!?]" },
    { 0xF041, "[!!]" },
    { 0xF042, "♥" },
    { 0xF043, "[~]" },
    { 0xF044, "[;;]" },
    { 0xF045, "[;]" },
    { 0xF046, "[~~]" },
    { 0xF047, "[~~~]" },
    { static_cast<uint16_t>(-1), "" } // 종료용
};

// CSV -> SDT 변환 시에만 적용되는 특수 테이블
static const struct {
    uint16_t hexCode;
    const char* replacement;
} rawReverseReplacementTable[] = {
    //{ 0x8148, "?" },
    //{ 0x8149, "!" },
    //{ 0x8160, "~" },
    { static_cast<uint16_t>(-1), "" } // 종료용
};

static std::unordered_map<uint16_t, std::string> hexToUtf8;
static std::unordered_map<std::string, uint16_t> utf8ToHex;

void initialize_replacement_table(const bool isToCSV) {
    hexToUtf8.clear();
    utf8ToHex.clear();

    for (int i = 0; rawReplacementTable[i].hexCode != static_cast<uint16_t>(-1); ++i) {
        uint16_t hex = rawReplacementTable[i].hexCode;

        // Step 1: ANSI → WideChar (유니코드)
        int wideLen = MultiByteToWideChar(CP_ACP, 0, rawReplacementTable[i].replacement, -1, nullptr, 0);
        if (wideLen <= 0) continue;

        std::wstring wideStr(wideLen, 0);
        MultiByteToWideChar(CP_ACP, 0, rawReplacementTable[i].replacement, -1, &wideStr[0], wideLen);

        // Step 2: WideChar → UTF-8
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len <= 0) continue;

        std::string utf8(utf8Len - 1, 0); // 마지막 널 문자 제외
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8[0], utf8Len, nullptr, nullptr);

        // 등록
        hexToUtf8[hex] = utf8;
        utf8ToHex[utf8] = hex;
    }

    if (!isToCSV) {
        for (int i = 0; rawReverseReplacementTable[i].hexCode != static_cast<uint16_t>(-1); ++i) {
            uint16_t hex = rawReverseReplacementTable[i].hexCode;

            // Step 1: ANSI → WideChar (유니코드)
            int wideLen = MultiByteToWideChar(CP_ACP, 0, rawReverseReplacementTable[i].replacement, -1, nullptr, 0);
            if (wideLen <= 0) continue;

            std::wstring wideStr(wideLen, 0);
            MultiByteToWideChar(CP_ACP, 0, rawReverseReplacementTable[i].replacement, -1, &wideStr[0], wideLen);

            // Step 2: WideChar → UTF-8
            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (utf8Len <= 0) continue;

            std::string utf8(utf8Len - 1, 0); // 마지막 널 문자 제외
            WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8[0], utf8Len, nullptr, nullptr);

            // 등록
            hexToUtf8[hex] = utf8;
            utf8ToHex[utf8] = hex;
        }
    }
}

const std::string* find_replacement_string(uint16_t hex) {
    auto it = hexToUtf8.find(hex);
    return (it != hexToUtf8.end()) ? &it->second : nullptr;
}

int find_hexcode_from_text(const std::string& utf8text) {
    auto it = utf8ToHex.find(utf8text);
    return (it != utf8ToHex.end()) ? it->second : -1;
}