#include "InfParser.h"
#include "resource.h"
#include <windows.h>
#include <string>      // std::string
#include <sstream>     // istringstream
#include <algorithm>   // std::remove_if, std::isspace
#include <cctype>      // std::isspace
#include <regex>       // std::regex (���� �Ľ��� �� �����ϰ�)

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
    m_infMappings.clear(); // ���� ������ �ʱ�ȭ

    std::string line;
    int lineNumber = 0;
    int currentMainIndex = -1; // ���� �Ľ� ���� [mainIndex]

    // [00000] ������ ���� ����� ��Ī�ϱ� ���� ���Խ�
    std::regex sectionRegex("\\[(\\d+)\\]");

    while (std::getline(iss, line)) {
        lineNumber++;
        // ���� �յ� ���� ����
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());

        if (line.empty() || line[0] == ';') { // �� ���̰ų� �ּ��̸� �ǳʶٱ�
            continue;
        }

        std::smatch matches;
        if (std::regex_match(line, matches, sectionRegex)) {
            // ���� ��� ��Ī ����
            try {
                currentMainIndex = std::stoi(matches[1].str());
                //dprintf("[Info] Parsing INF section: [%05d]", currentMainIndex);
            }
            catch (const std::exception& e) {
                dprintf("[Error] INF file parsing error: invalid main index format (line %d): %s - %s", lineNumber, line.c_str(), e.what());
                currentMainIndex = -1; // ��ȿ���� ���� �������� ����
            }
            continue; // ���� �ٷ� �̵�
        }

        // ���� ������ key=value �� �Ľ�
        if (currentMainIndex != -1) {
            std::stringstream ss(line);
            std::string keyStr, equals, valueStr;

            // "key=value" ���� �Ľ� (���� ����)
            if (std::getline(ss, keyStr, '=') && std::getline(ss, valueStr)) {
                // keyStr�� valueStr�� �յ� ���� ����
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
    // std::ifstream�� scope�� ����� �ڵ����� �����ϴ�.
    return true;
}

bool InfParser::Parse(const std::wstring& infFilePath) {
    m_infMappings.clear(); // ���� ������ �ʱ�ȭ

    std::ifstream file(infFilePath);
    if (!file.is_open()) {
        dprintf("[Error] INF file opening failure (no file or no access): %s", infFilePath.c_str());
        return false;
    }

    std::string line;
    int lineNumber = 0;
    int currentMainIndex = -1; // ���� �Ľ� ���� [mainIndex]

    // [00000] ������ ���� ����� ��Ī�ϱ� ���� ���Խ�
    std::regex sectionRegex("\\[(\\d+)\\]");

    while (std::getline(file, line)) {
        lineNumber++;
        // ���� �յ� ���� ����
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());

        if (line.empty() || line[0] == ';') { // �� ���̰ų� �ּ��̸� �ǳʶٱ�
            continue;
        }

        std::smatch matches;
        if (std::regex_match(line, matches, sectionRegex)) {
            // ���� ��� ��Ī ����
            try {
                currentMainIndex = std::stoi(matches[1].str());
                //dprintf("[Info] Parsing INF section: [%05d]", currentMainIndex);
            }
            catch (const std::exception& e) {
                dprintf("[Error] INF file parsing error: invalid main index format (line %d): %s - %s", lineNumber, line.c_str(), e.what());
                currentMainIndex = -1; // ��ȿ���� ���� �������� ����
            }
            continue; // ���� �ٷ� �̵�
        }

        // ���� ������ key=value �� �Ľ�
        if (currentMainIndex != -1) {
            std::stringstream ss(line);
            std::string keyStr, equals, valueStr;

            // "key=value" ���� �Ľ� (���� ����)
            if (std::getline(ss, keyStr, '=') && std::getline(ss, valueStr)) {
                // keyStr�� valueStr�� �յ� ���� ����
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
    // std::ifstream�� scope�� ����� �ڵ����� �����ϴ�.
    return true;
}