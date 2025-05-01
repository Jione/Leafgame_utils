#pragma once
#include <string>
#include <vector>

struct OperatorArgument {
    bool isString;      // true: 문자열, false: 숫자
    std::string value;  // 항상 문자열로 저장
};

struct OperatorData {
    uint32_t address;
    std::string operatorName;
    std::vector<OperatorArgument> args;
};

using FileOperators = std::vector<std::pair<std::string, std::vector<OperatorData>>>;

// JSON으로 저장
bool to_json(const FileOperators& files, const std::string& outputFilename);

// JSON으로 읽기
bool from_json(const std::string& inputFilename, FileOperators& files);