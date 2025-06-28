#include "StringUtil.h"

StringUtil::StringUtil() {
    // 생성자
}

StringUtil::~StringUtil() {
    // 소멸자
}

StringUtil& StringUtil::GetInstance() {
    static StringUtil instance;
    return instance;
}

// ShiftJIS(CP932) 멀티바이트(std::string) -> UTF-16 wstring 변환
std::wstring StringUtil::ShiftJisToWstring(const std::string& sjis) {
    if (sjis.empty()) return {};
    int size_needed = MultiByteToWideChar(
        932,                // CP_SHIFTJIS
        MB_ERR_INVALID_CHARS,
        sjis.data(),
        static_cast<int>(sjis.size()),
        nullptr, 0);
    std::wstring wstr;
    wstr.resize(size_needed);
    int converted = MultiByteToWideChar(
        932, 0,
        sjis.data(), static_cast<int>(sjis.size()),
        &wstr[0], size_needed);
    return wstr;
}

// UTF-16 std::wstring -> ShiftJIS(CP932) 멀티바이트(std::string) 변환
std::string StringUtil::WstringToShiftJis(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int sizeNeeded = WideCharToMultiByte(
        932, 0,
        wstr.data(), static_cast<int>(wstr.size()),
        nullptr, 0,
        nullptr, nullptr);
    std::string str(sizeNeeded, '\0');
    WideCharToMultiByte(
        932, 0,
        wstr.data(), static_cast<int>(wstr.size()),
        &str[0], sizeNeeded,
        nullptr, nullptr);
    return str;
}

// ANSI 멀티바이트(std::string) -> UTF-16 std::wstring 변환
std::wstring StringUtil::StringToWstring(const std::string& str) {
    if (str.empty()) return {};
    int sizeNeeded = MultiByteToWideChar(
        CP_ACP, 0,
        str.data(), static_cast<int>(str.size()),
        nullptr, 0);
    std::wstring wstr(sizeNeeded, L'\0');
    MultiByteToWideChar(
        CP_ACP, 0,
        str.data(), static_cast<int>(str.size()),
        &wstr[0], sizeNeeded);
    return wstr;
}

// UTF-16 std::wstring -> ANSI 멀티바이트(std::string) 변환
std::string StringUtil::WstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int sizeNeeded = WideCharToMultiByte(
        CP_ACP, 0,
        wstr.data(), static_cast<int>(wstr.size()),
        nullptr, 0,
        nullptr, nullptr);
    std::string str(sizeNeeded, '\0');
    WideCharToMultiByte(
        CP_ACP, 0,
        wstr.data(), static_cast<int>(wstr.size()),
        &str[0], sizeNeeded,
        nullptr, nullptr);
    return str;
}

// UTF-16 std::wstring -> UTF8(std::string) 변환
std::string StringUtil::WstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int sizeNeeded = ::WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.data(), (int)wstr.size(),
        nullptr, 0,
        nullptr, nullptr
    );
    std::string str(sizeNeeded, '\0');
    WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.data(), (int)wstr.size(),
        &str[0], sizeNeeded,
        nullptr, nullptr
    );
    return str;
}

// ANSI 문자열을 Shift-JIS로 바로 변환
std::string StringUtil::AcpToShiftJis(const std::string& acp) {
    std::wstring wide = StringToWstring(acp);
    return WstringToShiftJis(wide);
}

// Shift-JIS 문자열을 ANSI로 바로 변환
std::string StringUtil::ShiftJisToAcp(const std::string& sjis) {
    std::wstring wide = ShiftJisToWstring(sjis);
    return WstringToString(wide);
}