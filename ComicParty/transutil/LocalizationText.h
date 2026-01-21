#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cstdint>

namespace Localization {

    // 텍스트 추출
    void ExtractTextResources();

    // 텍스트 아카이브
    void BuildTextArchive();

    // --- 내부 변환 인터페이스 ---
    namespace Converter {
        // MES 바이너리 -> XLSX 파일 저장
        bool MesToXlsx(const std::vector<uint8_t>& mesData, const std::wstring& savePath, const std::vector<uint8_t>& relatedDatBuffer);

        // XLSX 파일 로드 -> MES 바이너리
        bool XlsxToMes(const std::wstring& srcPath, std::vector<uint8_t>& outMesData);
    }
}