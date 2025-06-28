#pragma once

#include "GlobalDefinitions.h"
#include <string>
#include <vector>
#include <fstream> // ifstream
#include <iostream> // cout

class PakParser {
public:
    // PAK 파일을 파싱하여 파일 목록과 전체 데이터를 로드합니다.
    bool Parse(const std::wstring& pakFilePath,
        std::vector<PakFileInfo>& outPakFiles);

private:
    std::string GetExecutablePath();

    bool ParsePak(const std::wstring& pakFilePath,
        std::vector<PakFileInfo>& outPakFiles);

    bool ParseSfs(const std::wstring& pakFilePath,
        std::vector<PakFileInfo>& outPakFiles);

    bool ParseBin(const std::wstring& pakFilePath,
        std::vector<PakFileInfo>& outPakFiles);

    // Helper 함수: 오류 메시지 출력
    std::string ExtractUniqueId(const std::string& fullFileName);
    std::string ExtractVagName(const std::string& fileName);
    bool IsAudioFileExtension(const std::string& fileName);
};