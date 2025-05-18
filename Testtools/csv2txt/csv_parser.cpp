#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <functional>

// CSV 1줄을 필드 단위로 분리
static bool parse_csv_line(const std::string& line, std::vector<std::string>& fields) {
    fields.clear();
    size_t i = 0;
    const size_t len = line.length();

    while (i < len) {
        std::string field;
        if (line[i] == '"') {
            // 따옴표 감싼 필드
            ++i;
            while (i < len) {
                if (line[i] == '"') {
                    if (i + 1 < len && line[i + 1] == '"') {
                        field += '"';
                        i += 2;
                    }
                    else {
                        ++i;
                        break;
                    }
                }
                else {
                    field += line[i++];
                }
            }
        }
        else {
            // 일반 필드
            while (i < len && line[i] != ',') {
                field += line[i++];
            }
        }

        fields.push_back(field);

        if (i < len && line[i] == ',') ++i;
    }
    return true;
}

void postprocess_from_csv(const std::string& input, std::ofstream& oss, size_t& feedLen) {
    size_t i = 0;
    feedLen = 1;
    bool isLF = false;
    while (i < input.size()) {
        // 줄바꿈(\r, \n) 무시
        if ((input[i] == '\r') || (input[i] == '\n')) {
            ++i;
            //isLF = true;
            continue;
        }

        if (isLF && (i != (input.size() - 1))) {
            oss << "\r\n";
            feedLen++;
            //isLF = false;
        }

        // 일반 문자 처리
        uint8_t ch = static_cast<uint8_t>(input[i]);

        if (ch <= 0x7F) {
            oss.put(static_cast<char>(ch));
            ++i;
            /*if (ch == '\\') {
                uint8_t second = static_cast<uint8_t>(input[i]);
                if (second == 'n') {
                    oss.put(static_cast<char>(second));
                    isLF = true;
                    ++i;
                }
            }*/
        }
        else {
            size_t utf8_len = 0;
            if ((ch & 0xE0) == 0xC0) utf8_len = 2;
            else if ((ch & 0xF0) == 0xE0) utf8_len = 3;
            else if ((ch & 0xF8) == 0xF0) utf8_len = 4;
            else {
                oss.put(static_cast<char>(ch));
                ++i;
                continue;
            }

            if (i + utf8_len > input.size()) {
                break;
            }

            oss << input.substr(i, utf8_len);
            i += utf8_len;
        }
    }
    oss << "\r\n";
}


bool from_csv(const std::string& inputFilename, const std::string& outputFilename) {
    // 타겟 위치
    int targetRow = 3;

    std::ifstream in(inputFilename, std::ios::binary);
    std::ofstream out(outputFilename, std::ios::binary);
    if (!in) {
        std::cerr << "입력 파일을 열 수 없습니다.\n";
        return false;
    }
    if (!out) {
        std::cerr << "출력 파일을 열 수 없습니다.\n";
        return false;
    }
    std::ofstream outInfo(outputFilename + ".info", std::ios::binary);

    std::string csvText((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    size_t bomSkip = (csvText.size() >= 3 &&
        (unsigned char)csvText[0] == 0xEF &&
        (unsigned char)csvText[1] == 0xBB &&
        (unsigned char)csvText[2] == 0xBF) ? 3 : 0;

    std::vector<std::string> fields;
    std::string currentLine;
    bool insideQuote = false;
    size_t i = bomSkip;
    size_t blockLen;
    if (bomSkip == 3) {
        out.put(0xEF);
        out.put(0xBB);
        out.put(0xBF);
    }
    int row = 0;

    while (i < csvText.size()) {
        char ch = csvText[i];
        currentLine += ch;

        if (ch == '"') {
            // 따옴표 상태 토글
            insideQuote = !insideQuote;
        }

        if (!insideQuote && ch == '\n') {
            // 줄 끝
            if (!currentLine.empty() && (currentLine.back() == '\n')) {
                if (currentLine.size() >= 2 && currentLine[currentLine.size() - 2] == '\r') {
                    currentLine.erase(currentLine.size() - 2); // CRLF 제거
                }
                else {
                    currentLine.erase(currentLine.size() - 1); // LF만 제거
                }

                if (currentLine.empty()) {
                    ++i;
                    continue;
                }

                if (parse_csv_line(currentLine, fields)) {
                    if (row == 0) {

                    }
                    else {
                        postprocess_from_csv(fields[targetRow], out, blockLen);
                        outInfo << fields[0] << "," << fields[1] << "," << blockLen << "\n";
                    }
                    row++;
                }

                currentLine.clear();
            }
        }
        ++i;
    }
    in.close();
    out.close();
    outInfo.close();
    return true;
}