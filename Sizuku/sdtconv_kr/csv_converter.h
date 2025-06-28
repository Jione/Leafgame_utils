#pragma once
#include <string>
#include <vector>
#include <functional>

struct CsvStringData {
    std::string filename;
    int id;
    std::string charactor;
    std::string original;
    std::string translated;
};

// 메모리 -> CSV 파일 변환
bool to_csv(const std::vector<CsvStringData>& data, const std::string& outputFilename);

// CSV 텍스트 -> 메모리 변환
bool from_csv(const std::string& inputFilename, std::vector<CsvStringData>& outData);
