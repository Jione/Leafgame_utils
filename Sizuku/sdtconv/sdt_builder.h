#pragma once

#include <string>
#include <vector>
#include <map>
#include "json_converter.h"
#include "csv_converter.h"

// ���� �ɼ�
struct SdtBuildOptions {
    bool verbose = false;
};

// ���� SDT ���� ���
struct BuiltSdtFile {
    std::string filename;
    std::vector<uint8_t> binary;
};

class SdtBuilder {
public:
    // JSON + CSV �ε�
    bool load(const std::string& jsonPath, const std::string& csvPath);

    // SDT ���� ����
    bool build(const SdtBuildOptions& options = {});

    // ����� SDT ���� ����
    bool save(const std::string& outputFolder);

    // ����� ��� ����Ʈ
    const std::vector<BuiltSdtFile>& getResults() const;

private:
    FileOperators fileOperators;
    std::vector<CsvStringData> csvStrings;
    std::vector<BuiltSdtFile> results;

    std::string findTranslatedString(int id);
    void updateBranchAddresses(std::vector<OperatorData>& ops, const std::map<uint32_t, uint32_t>& addrMap);
    std::vector<uint8_t> encodeOperator(const OperatorData& op);
    uint32_t calculateOperatorSize(const OperatorData& op);
};