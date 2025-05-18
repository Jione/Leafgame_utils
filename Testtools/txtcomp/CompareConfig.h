#pragma once
#include <vector>

namespace CompareConfig {
    // 첫 번째 문자: 전각 공백 및 낫표 열기
    static const std::vector<wchar_t> openingChars = {
        L'\u3000', // 전각 공백
        L'\u300C', // 「
        L'\u300E'  // 『
    };

    // 마지막 문자: 낫표 닫기
    static const std::vector<wchar_t> closingChars = {
        L'\u300D', // 」
        L'\u300F'  // 』
    };

    // 낫표 개수 비교용
    static const std::vector<wchar_t> quoteChars = {
        L'\u300C', // 「
        L'\u300E', // 『
        L'\u300D', // 」
        L'\u300F'  // 』
    };
}