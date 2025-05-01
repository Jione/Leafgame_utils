#pragma once

#include <string>
#include <vector>
#include <map>
#include "json_converter.h"
#include "csv_converter.h"

// 빌더 옵션
struct SdtBuildOptions {
    bool verbose = false;
};

// 최종 SDT 파일 결과
struct BuiltSdtFile {
    std::string filename;
    std::vector<uint8_t> binary;
};

class SdtBuilder {
public:
    // JSON + CSV 로딩
    bool load(const std::string& jsonPath, const std::string& csvPath);

    // SDT 파일 빌드
    bool build(const SdtBuildOptions& options = {});

    // 빌드된 SDT 파일 저장
    bool save(const std::string& outputFolder);

    // 빌드된 결과 리스트
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