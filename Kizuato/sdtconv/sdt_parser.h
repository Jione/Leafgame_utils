#pragma once
#ifndef SDT_PARSER_H
#define SDT_PARSER_H

#include <string>
#include <vector>
#include <cstdint>

// 오퍼레이터 파라미터
struct OperatorParam {
    bool isString;             // true = strValue 사용, false = intValue 사용
    int32_t intValue;           // 숫자 값
    char strValue[256];         // 문자열 값 (null 종료)

    OperatorParam() : isString(false), intValue(0) {
        strValue[0] = '\0';
    }
};

// 오퍼레이터 하나
struct ParsedOperator {
    uint32_t address;                  // 오퍼레이터의 주소값
    std::string name;                  // 오퍼레이터 명칭
    std::vector<OperatorParam> params; // 파라미터 목록
};

// 파일 하나의 파싱 결과
struct ParsedSdtFile {
    std::string filename;               // 파일명 (확장자 제외)
    std::vector<ParsedOperator> ops;     // 오퍼레이터 배열
};

// 추출된 문자열 (CSV용)
struct ParsedString {
    std::string filename;       // 파일명 (확장자 제외)
    int32_t id;                 // 문자열 번호 (파일별 1부터)
    std::string charactorName;  // 캐릭터명 또는 선택지
    std::string originalText;   // 원본 문자열
    std::string translatedText; // 번역 문자열 (초기에는 originalText 복제)
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