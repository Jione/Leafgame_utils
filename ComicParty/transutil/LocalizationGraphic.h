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
    // 그래픽 추출 대상 파일 목록 관리 (Pak이름 -> 내부 파일명 리스트)
    extern std::map<std::wstring, std::set<std::string>> TargetGraphics;

    // 그래픽 추출
    void ExtractGraphicResources();

    // 그래픽 아카이브
    void BuildGraphicArchive();

    // --- 내부 변환 인터페이스 ---
    namespace Converter {
        // GRP 바이너리 -> PNG 파일 저장
        bool GrpToPng(const std::vector<uint8_t>& grpData, const std::wstring& savePath);

        // PNG 파일 로드 -> GRP 바이너리
        bool PngToGrp(const std::wstring& srcPath, std::vector<uint8_t>& outGrpData);
    }
}