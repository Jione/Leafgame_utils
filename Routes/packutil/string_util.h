#pragma once
#include <windows.h>
#include <string>

std::wstring shiftJisToWstring(const std::string& sjis);
std::string wstringToShiftJis(const std::wstring& wstr);
std::wstring stringToWstring(const std::string& str);
std::string wstringToString(const std::wstring& wstr);
std::string wstringToUtf8(const std::wstring& wstr);
std::string acpToShiftJis(const std::string& acp);
std::string shiftJisToAcp(const std::string& sjis);