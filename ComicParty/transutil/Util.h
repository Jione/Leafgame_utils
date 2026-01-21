#pragma once
#include <Windows.h>
#include <string>
#include <vector>

namespace Util {
    // --- Helpers ---
    std::string WideToMultiByteStr(const std::wstring& wstr, UINT codePage);
    std::wstring MultiByteToWideStr(const std::string& str, UINT codePage);
    std::string MultiByteToUtf8(const std::string& str, UINT codePage);

    // --- MES -> UTF-8 (Export) ---
    // 게임 내 독자 포맷 바이트 배열을 엑셀용 UTF-8 문자열로 변환
    std::string MesBytesToUtf8(const std::vector<char>& data, uint32_t offset, uint32_t len);

    // --- UTF-8 -> MES (Import) ---
    // 엑셀의 UTF-8 문자열을 게임 내 독자 포맷 바이트 배열로 변환
    std::vector<char> Utf8ToMesBytes(const std::string& utf8Str);
}