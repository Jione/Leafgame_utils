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

// �޸� -> CSV ���� ��ȯ
bool to_csv(const std::vector<CsvStringData>& data, const std::string& outputFilename);

// CSV �ؽ�Ʈ -> �޸� ��ȯ
bool from_csv(const std::string& inputFilename, std::vector<CsvStringData>& outData);
