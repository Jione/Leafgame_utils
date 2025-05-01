#pragma once
#include <string>
#include <vector>

struct OperatorArgument {
    bool isString;      // true: ���ڿ�, false: ����
    std::string value;  // �׻� ���ڿ��� ����
};

struct OperatorData {
    uint32_t address;
    std::string operatorName;
    std::vector<OperatorArgument> args;
};

using FileOperators = std::vector<std::pair<std::string, std::vector<OperatorData>>>;

// JSON���� ����
bool to_json(const FileOperators& files, const std::string& outputFilename);

// JSON���� �б�
bool from_json(const std::string& inputFilename, FileOperators& files);