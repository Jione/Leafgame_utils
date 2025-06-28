#pragma once
#include <windows.h>
#include <string>

class StringUtil {
public:
    static StringUtil& GetInstance(); // ΩÃ±€≈Ê

    // Shift-JIS -> UTF-16
    std::wstring ShiftJisToWstring(const std::string& sjis);
    // UTF-16 -> Shift-JIS
    std::string WstringToShiftJis(const std::wstring& wstr);
    // CP-ACP -> UTF-16
    std::wstring StringToWstring(const std::string& str);
    // UTF-16 -> CP-ACP
    std::string WstringToString(const std::wstring& wstr);
    // UTF-16 -> UTF-8
    std::string WstringToUtf8(const std::wstring& wstr);
    // CP-ACP -> Shift-JIS
    std::string AcpToShiftJis(const std::string& acp);
    // Shift-JIS -> CP-ACP
    std::string ShiftJisToAcp(const std::string& sjis);

private:
    StringUtil(); // ΩÃ±€≈Ê
    ~StringUtil();
    StringUtil(const StringUtil&) = delete;
    StringUtil& operator=(const StringUtil&) = delete;

};