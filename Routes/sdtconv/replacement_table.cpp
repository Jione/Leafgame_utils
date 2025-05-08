#include "replacement_table.h"
#include <unordered_map>
#include <string>
#include <windows.h>

// ���� ���̺�
static const struct {
    uint16_t hexCode;
    const char* replacement; // ANSI ���ڵ� ���ڿ� ���ͷ� (������ �� ����)
} rawReplacementTable[] = {
    { 0xF040, "[!?]" },
    { 0xF041, "[!!]" },
    { 0xF042, "��" },
    { 0xF043, "[~]" },
    { 0xF044, "[;;]" },
    { 0xF045, "[;]" },
    { 0xF046, "[~~]" },
    { 0xF047, "[~~~]" },
    { static_cast<uint16_t>(-1), "" } // �����
};

// CSV -> SDT ��ȯ �ÿ��� ����Ǵ� Ư�� ���̺�
static const struct {
    uint16_t hexCode;
    const char* replacement;
} rawReverseReplacementTable[] = {
    //{ 0x8148, "?" },
    //{ 0x8149, "!" },
    //{ 0x8160, "~" },
    { static_cast<uint16_t>(-1), "" } // �����
};

static std::unordered_map<uint16_t, std::string> hexToUtf8;
static std::unordered_map<std::string, uint16_t> utf8ToHex;

void initialize_replacement_table(const bool isToCSV) {
    hexToUtf8.clear();
    utf8ToHex.clear();

    for (int i = 0; rawReplacementTable[i].hexCode != static_cast<uint16_t>(-1); ++i) {
        uint16_t hex = rawReplacementTable[i].hexCode;

        // Step 1: ANSI �� WideChar (�����ڵ�)
        int wideLen = MultiByteToWideChar(CP_ACP, 0, rawReplacementTable[i].replacement, -1, nullptr, 0);
        if (wideLen <= 0) continue;

        std::wstring wideStr(wideLen, 0);
        MultiByteToWideChar(CP_ACP, 0, rawReplacementTable[i].replacement, -1, &wideStr[0], wideLen);

        // Step 2: WideChar �� UTF-8
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len <= 0) continue;

        std::string utf8(utf8Len - 1, 0); // ������ �� ���� ����
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8[0], utf8Len, nullptr, nullptr);

        // ���
        hexToUtf8[hex] = utf8;
        utf8ToHex[utf8] = hex;
    }

    if (!isToCSV) {
        for (int i = 0; rawReverseReplacementTable[i].hexCode != static_cast<uint16_t>(-1); ++i) {
            uint16_t hex = rawReverseReplacementTable[i].hexCode;

            // Step 1: ANSI �� WideChar (�����ڵ�)
            int wideLen = MultiByteToWideChar(CP_ACP, 0, rawReverseReplacementTable[i].replacement, -1, nullptr, 0);
            if (wideLen <= 0) continue;

            std::wstring wideStr(wideLen, 0);
            MultiByteToWideChar(CP_ACP, 0, rawReverseReplacementTable[i].replacement, -1, &wideStr[0], wideLen);

            // Step 2: WideChar �� UTF-8
            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (utf8Len <= 0) continue;

            std::string utf8(utf8Len - 1, 0); // ������ �� ���� ����
            WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8[0], utf8Len, nullptr, nullptr);

            // ���
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