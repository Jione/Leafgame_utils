#pragma once
#include <string>
#include <vector>

struct OperatorArgument {
    int valueType;      // 0 = Unsigned long, 1 = int, 2 = String
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