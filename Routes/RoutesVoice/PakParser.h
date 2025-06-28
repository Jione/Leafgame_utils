#pragma once

#include "GlobalDefinitions.h"
#include <string>
#include <vector>
#include <fstream> // ifstream
#include <iostream> // cout

class PakParser {
public:
    // PAK ������ �Ľ��Ͽ� ���� ��ϰ� ��ü �����͸� �ε��մϴ�.
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

    // Helper �Լ�: ���� �޽��� ���
    std::string ExtractUniqueId(const std::string& fullFileName);
    std::string ExtractVagName(const std::string& fileName);
    bool IsAudioFileExtension(const std::string& fileName);
};