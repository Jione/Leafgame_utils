#include "InfParser.h"
#include "resource.h"
#include <windows.h>
#include <string>      // std::string
#include <sstream>     // istringstream
#include <algorithm>   // std::remove_if, std::isspace
#include <cctype>      // std::isspace
#include <regex>       // std::regex (섹션 파싱을 더 강건하게)

std::string InfParser::LoadTextResource(int resourceId, const char* resourceType) {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), resourceType);
    if (!hRes) return "";

    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) return "";

    DWORD size = SizeofResource(NULL, hRes);
    const char* data = static_cast<const char*>(LockResource(hData));
    if (!data) return "";

    return std::string(data, size);
}

bool InfParser::SaveDefault(const std::wstring& infFilePath) {
    dprintf("Save default INF data...");

    std::ifstream checkFile(infFilePath);
    if (checkFile.is_open()) {
        dprintf("[Error] INF file already exists.: %s", infFilePath.c_str());
        checkFile.close();
        return false;
    }

    std::ofstream file(infFilePath, std::ios::binary);
    file << LoadTextResource(IDR_TEXT1, "TEXT");
    file.close();
    return true;
}

bool InfParser::LoadDefault() {
    dprintf("Load default INF data...");

    std::istringstream iss(LoadTextResource(IDR_TEXT1, "TEXT"));
    m_infMappings.clear(); // 기존 데이터 초기화

    std::string line;
    int lineNumber = 0;
    int currentMainIndex = -1; // 현재 파싱 중인 [mainIndex]

    // [00000] 형태의 섹션 헤더를 매칭하기 위한 정규식
    std::regex sectionRegex("\\[(\\d+)\\]");

    while (std::getline(iss, line)) {
        lineNumber++;
        // 줄의 앞뒤 공백 제거
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());

        if (line.empty() || line[0] == ';') { // 빈 줄이거나 주석이면 건너뛰기
            continue;
        }

        std::smatch matches;
        if (std::regex_match(line, matches, sectionRegex)) {
            // 섹션 헤더 매칭 성공
            try {
                currentMainIndex = std::stoi(matches[1].str());
                //dprintf("[Info] Parsing INF section: [%05d]", currentMainIndex);
            }
            catch (const std::exception& e) {
                dprintf("[Error] INF file parsing error: invalid main index format (line %d): %s - %s", lineNumber, line.c_str(), e.what());
                currentMainIndex = -1; // 유효하지 않은 섹션으로 간주
            }
            continue; // 다음 줄로 이동
        }

        // 섹션 내부의 key=value 쌍 파싱
        if (currentMainIndex != -1) {
            std::stringstream ss(line);
            std::string keyStr, equals, valueStr;

            // "key=value" 형태 파싱 (공백 무시)
            if (std::getline(ss, keyStr, '=') && std::getline(ss, valueStr)) {
                // keyStr과 valueStr의 앞뒤 공백 제거
                keyStr.erase(keyStr.begin(), std::find_if(keyStr.begin(), keyStr.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                keyStr.erase(std::find_if(keyStr.rbegin(), keyStr.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), keyStr.end());
                valueStr.erase(valueStr.begin(), std::find_if(valueStr.begin(), valueStr.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                valueStr.erase(std::find_if(valueStr.rbegin(), valueStr.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), valueStr.end());

                try {
                    int subIndex = std::stoi(keyStr);
                    int value = std::stoi(valueStr);
                    m_infMappings[currentMainIndex][subIndex] = value;
                }
                catch (const std::exception& e) {
                    dprintf("[Error] INF file parsing error: invalid key=value format (line %d): %s - %s", lineNumber, line.c_str(), e.what());
                }
            }
            else {
                dprintf("[Error] INF file parsing error: invalid key=value pair (line %d): %s", lineNumber, line.c_str());
            }
        }
        else {
            dprintf("[Warning] INF file parsing: key=value pair found outside of any section (line %d): %s", lineNumber, line.c_str());
        }
    }
    dprintf("[Info] Parsing INF complete.");
    // std::ifstream은 scope를 벗어나면 자동으로 닫힙니다.
    return true;
}

bool InfParser::Parse(const std::wstring& infFilePath) {
    m_infMappings.clear(); // 기존 데이터 초기화

    std::ifstream file(infFilePath);
    if (!file.is_open()) {
        dprintf("[Error] INF file opening failure (no file or no access): %s", infFilePath.c_str());
        return false;
    }

    std::string line;
    int lineNumber = 0;
    int currentMainIndex = -1; // 현재 파싱 중인 [mainIndex]

    // [00000] 형태의 섹션 헤더를 매칭하기 위한 정규식
    std::regex sectionRegex("\\[(\\d+)\\]");

    while (std::getline(file, line)) {
        lineNumber++;
        // 줄의 앞뒤 공백 제거
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());

        if (line.empty() || line[0] == ';') { // 빈 줄이거나 주석이면 건너뛰기
            continue;
        }

        std::smatch matches;
        if (std::regex_match(line, matches, sectionRegex)) {
            // 섹션 헤더 매칭 성공
            try {
                currentMainIndex = std::stoi(matches[1].str());
                //dprintf("[Info] Parsing INF section: [%05d]", currentMainIndex);
            }
            catch (const std::exception& e) {
                dprintf("[Error] INF file parsing error: invalid main index format (line %d): %s - %s", lineNumber, line.c_str(), e.what());
                currentMainIndex = -1; // 유효하지 않은 섹션으로 간주
            }
            continue; // 다음 줄로 이동
        }

        // 섹션 내부의 key=value 쌍 파싱
        if (currentMainIndex != -1) {
            std::stringstream ss(line);
            std::string keyStr, equals, valueStr;

            // "key=value" 형태 파싱 (공백 무시)
            if (std::getline(ss, keyStr, '=') && std::getline(ss, valueStr)) {
                // keyStr과 valueStr의 앞뒤 공백 제거
                keyStr.erase(keyStr.begin(), std::find_if(keyStr.begin(), keyStr.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                keyStr.erase(std::find_if(keyStr.rbegin(), keyStr.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), keyStr.end());
                valueStr.erase(valueStr.begin(), std::find_if(valueStr.begin(), valueStr.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                valueStr.erase(std::find_if(valueStr.rbegin(), valueStr.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), valueStr.end());

                try {
                    int subIndex = std::stoi(keyStr);
                    int value = std::stoi(valueStr);
                    m_infMappings[currentMainIndex][subIndex] = value;
                }
                catch (const std::exception& e) {
                    dprintf("[Error] INF file parsing error: invalid key=value format (line %d): %s - %s", lineNumber, line.c_str(), e.what());
                }
            }
            else {
                dprintf("[Error] INF file parsing error: invalid key=value pair (line %d): %s", lineNumber, line.c_str());
            }
        }
        else {
            dprintf("[Warning] INF file parsing: key=value pair found outside of any section (line %d): %s", lineNumber, line.c_str());
        }
    }
    dprintf("[Info] Parsing INF complete.");
    // std::ifstream은 scope를 벗어나면 자동으로 닫힙니다.
    return true;
}