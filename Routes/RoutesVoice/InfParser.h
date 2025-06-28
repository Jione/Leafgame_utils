#pragma once

#include "GlobalDefinitions.h"
#include <string>
#include <vector>
#include <map>       // mainIndex와 subIndex 매핑을 위해
#include <fstream>   // ifstream
#include <iostream>  // cout

class InfParser {
public:
    // INF 파일을 파싱하여 mainIndex -> (subIndex -> value) 매핑을 로드합니다.
    // 이전의 outInfMappings를 이 함수 내부에서 관리하도록 변경합니다.
    bool SaveDefault(const std::wstring& infFilePath);
    bool LoadDefault();
    bool Parse(const std::wstring& infFilePath);

    // 파싱된 전체 INF 맵을 반환합니다.
    const std::map<int, std::map<int, int>>& GetInfMappings() const { return m_infMappings; }

private:
    // 텍스트 리소스 로드 함수
    std::string LoadTextResource(int resourceId, const char* resourceType);
    std::map<int, std::map<int, int>> m_infMappings; // main_index -> (sub_index -> value) 구조

    // Helper 함수: 오류 메시지 출력 (필요시 추가)
};