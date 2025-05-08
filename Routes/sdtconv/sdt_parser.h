#pragma once
#ifndef SDT_PARSER_H
#define SDT_PARSER_H

#include <string>
#include <vector>
#include <cstdint>

// ���۷����� �Ķ����
struct OperatorParam {
    int valueType;              // 0 = null, 1 = int, 2 = String
    int32_t intValue;           // ���� ��
    uint32_t uintValue;         // ��ȣ ���� ���� ��
    char strValue[256];         // ���ڿ� �� (null ����)

    OperatorParam() : valueType(1), intValue(0), uintValue(0) {
        strValue[0] = '\0';
    }
};

// ���۷����� �ϳ�
struct ParsedOperator {
    uint32_t address;                  // ���۷������� �ּҰ�
    std::string name;                  // ���۷����� ��Ī
    std::vector<OperatorParam> params; // �Ķ���� ���
};

// ���� �ϳ��� �Ľ� ���
struct ParsedSdtFile {
    std::string filename;               // ���ϸ� (Ȯ���� ����)
    std::vector<ParsedOperator> ops;     // ���۷����� �迭
};

// ����� ���ڿ� (CSV��)
struct ParsedString {
    std::string filename;       // ���ϸ� (Ȯ���� ����)
    int32_t id;                 // ���ڿ� ��ȣ (���Ϻ� 1����)
    std::string charactorName;  // ĳ���͸� �Ǵ� ������
    std::string originalText;   // ���� ���ڿ�
    std::string translatedText; // ���� ���ڿ� (�ʱ⿡�� originalText ����)
};

class SdtParser {
public:
    bool parseFolder(const std::string& folderPath);
    bool parseFile(const std::string& filepath);

    const std::vector<ParsedSdtFile>& getParsedSdts() const;
    const std::vector<ParsedString>& getParsedStrings() const;

private:
    std::vector<ParsedSdtFile> parsedFiles;
    std::vector<ParsedString> parsedStrings;

    int currentStringId = 1;
    std::string currentCharactorName;

    void reset();
};

#endif // SDT_PARSER_H