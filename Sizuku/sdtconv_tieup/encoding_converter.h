#pragma once
#include <string>

// Shift-JIS → UTF-8 변환
std::string shiftjis_to_utf8(const std::string& shiftJisText);

// UTF-8 → Shift-JIS 변환
std::string utf8_to_shiftjis(const std::string& utf8Text);

// EUC-KR → UTF-8 변환
std::string euckr_to_utf8(const std::string& eucKrText);

// UTF-8 → EUC-KR 변환
std::string utf8_to_euckr(const std::string& utf8Text);