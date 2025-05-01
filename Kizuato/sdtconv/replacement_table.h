#pragma once
#include <string>
#include <vector>

// 치환 테이블 검색용 함수
const std::string* find_replacement_string(uint16_t hex);

// 치환 테이블 역검색용 함수
int find_hexcode_from_text(const std::string& utf8text);

// 치환 테이블 초기화 (UTF-8 변환 대응용)
void initialize_replacement_table(const bool isToCSV);