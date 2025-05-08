#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cctype>

#include "stdlib.h"
#include "sdt_parser.h"
#include "json_converter.h"
#include "csv_converter.h"
#include "sdt_builder.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#if __cplusplus < 201703L
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

// 문자열 소문자 변환
std::string toLower(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
    return res;
}

// 확장자 추출
std::string getExtension(const std::string& path) {
    size_t dot = path.find_last_of('.');
    return (dot != std::string::npos) ? toLower(path.substr(dot + 1)) : "";
}

// 파일명만 추출
std::string getFilenameWithoutExt(const std::string& path) {
    fs::path p(path);
    return p.stem().string();
}

// 폴더 존재 확인
bool folderExists(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

// 파일 존재 확인
bool fileExists(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

// [기능 1] SDT → JSON + CSV
bool exportSdtToJsonCsv(const std::string& path) {
    SdtParser parser;

    if (fs::is_directory(path)) {
        if (!parser.parseFolder(path)) {
            std::cerr << "[오류] 폴더 파싱 실패: " << path << std::endl;
            return false;
        }
    }
    else if (fs::is_regular_file(path)) {
        if (!parser.parseFile(path)) {
            std::cerr << "[오류] 파일 파싱 실패: " << path << std::endl;
            return false;
        }
    }
    else {
        std::cerr << "[오류] 유효하지 않은 경로: " << path << std::endl;
        return false;
    }

    std::string name = fs::is_directory(path)
        ? fs::path(path).filename().string()
        : getFilenameWithoutExt(path);

    std::string jsonOut = name + ".json";
    std::string csvOut = name + ".csv";

    const auto& parsedSdts = parser.getParsedSdts();
    FileOperators fileOps;

    for (size_t f = 0; f < parsedSdts.size(); ++f) {
        const ParsedSdtFile& file = parsedSdts[f];
        std::vector<OperatorData> ops;

        for (size_t o = 0; o < file.ops.size(); ++o) {
            const ParsedOperator& op = file.ops[o];
            OperatorData data;
            data.address = op.address;
            data.operatorName = op.name;

            for (size_t p = 0; p < op.params.size(); ++p) {
                const OperatorParam& param = op.params[p];
                OperatorArgument arg;
                arg.valueType = param.valueType;
                if (param.valueType == 2) {
                    arg.value = std::string(param.strValue);
                }
                else if (param.valueType == 1) {
                    arg.value = std::to_string(param.intValue);
                }
                else if (param.valueType == 0) {
                    arg.value = std::to_string(param.uintValue);
                }
                data.args.push_back(arg);
            }
            ops.push_back(std::move(data));
        }

        fileOps.push_back(std::make_pair(file.filename, std::move(ops)));
    }

    if (!to_json(fileOps, jsonOut)) {
        std::cerr << "[오류] JSON 저장 실패: " << jsonOut << std::endl;
        return false;
    }

    std::vector<CsvStringData> csvData;
    for (const auto& ps : parser.getParsedStrings()) {
        CsvStringData row;
        row.filename = ps.filename;
        row.id = ps.id;
        row.charactor = ps.charactorName;
        row.original = ps.originalText;
        //row.translated = ps.translatedText;
        csvData.push_back(row);
    }

    if (!to_csv(csvData, csvOut)) {
        std::cerr << "[오류] CSV 저장 실패: " << csvOut << std::endl;
        return false;
    }

    std::cout << "변환 완료: " << jsonOut << ", " << csvOut << std::endl;
    return true;
}

// [기능 2] JSON + CSV → SDT 복원
bool importJsonCsvToSdt(const std::string& jsonOrCsvPath) {
    fs::path path(jsonOrCsvPath);
    std::string baseName = path.stem().string();
    std::string folder = path.parent_path().string();
    std::string jsonFile = baseName + ".json";
    std::string csvFile = baseName + ".csv";

    if (!fileExists(jsonFile) || !fileExists(csvFile)) {
        std::cerr << "[오류] 필요한 JSON 또는 CSV 파일이 누락됨\n";
        return false;
    }

    // 순차 폴더 이름 찾기
    std::string outFolder;
    for (int i = 0; i < 100; ++i) {
        outFolder = baseName + "_" + (i < 10 ? "0" : "") + std::to_string(i);
        if (!folderExists(outFolder)) {
#ifdef _WIN32
            _mkdir(outFolder.c_str());
#else
            mkdir(outFolder.c_str(), 0755);
#endif
            break;
        }
    }

    // 빌드 수행
    SdtBuilder builder;
    if (!builder.load(jsonFile, csvFile)) {
        std::cerr << "[오류] JSON 또는 CSV 로드 실패\n";
        return false;
    }

    if (!builder.build()) {
        std::cerr << "[오류] SDT 빌드 실패\n";
        return false;
    }

    if (!builder.save(outFolder)) {
        std::cerr << "[오류] SDT 저장 실패\n";
        return false;
    }

    std::cout << "SDT 복원 완료: " << outFolder << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "사용법: stdconv.exe [파일 또는 폴더 경로]\n\n";
        system("PAUSE");
        return 1;
    }

    std::string input = argv[1];
    std::string ext = getExtension(input);

    if (ext == "sdt" || folderExists(input)) {
        bool res = exportSdtToJsonCsv(input);
        std::cout << "\n";
        system("PAUSE");
        return res ? 0 : 1;
    }

    if (ext == "json" || ext == "csv") {
        bool res = importJsonCsvToSdt(input);
        std::cout << "\n";
        system("PAUSE");
        return res ? 0 : 1;
    }

    std::cerr << "[오류] 지원되지 않는 입력 형식입니다.\n\n";
    system("PAUSE");
    return 1;
}