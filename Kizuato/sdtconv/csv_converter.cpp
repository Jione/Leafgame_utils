#include "csv_converter.h"
#include "encoding_converter.h"
#include "replacement_table.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

std::string preprocess_to_utf8(const std::string& input) {
    std::ostringstream oss;
    size_t i = 0;
    while (i < input.size()) {
        uint8_t ch = static_cast<uint8_t>(input[i]);

        // ASCII 범위
        if (ch <= 0x7F) {
            if (ch == '\\' && (i + 1) < input.size() && input[i + 1] == 'n') {
                // \n 발견
                oss << "\\n";
                if ((i + 2) < input.size()) {
                    if (input[i + 2] == '>') {
                        if ((i + 3) < input.size()) {
                            oss << ">\n";
                        }
                        else {
                            oss << ">";
                        }
                        i++;
                    }
                    else {
                        oss << "\n"; // 중간에만 줄바꿈 삽입
                    }
                }
                i += 2;
            }
            else {
                oss << static_cast<char>(ch);
                ++i;
            }
        }
        // EUC-KR 영역
        else if (ch >= 0xB0 && ch <= 0xC8 && (i + 1) < input.size()) {
            uint8_t next = static_cast<uint8_t>(input[i + 1]);
            if (next >= 0xA1 && next <= 0xFE) {
                uint16_t hexVal = (ch << 8) | next;
                const std::string* rep = find_replacement_string(hexVal);
                if (rep) {
                    oss << *rep;
                }
                else {
                    std::string euckr;
                    euckr += static_cast<char>(ch);
                    euckr += static_cast<char>(next);
                    oss << euckr_to_utf8(euckr);
                }
                i += 2;
                continue;
            }
        }
        // Shift-JIS 영역 (반각 + 전각)
        else {
            if ((i + 1) < input.size()) {
                uint16_t hexVal = (ch << 8) | static_cast<uint8_t>(input[i + 1]);
                const std::string* rep = find_replacement_string(hexVal);
                if (rep) {
                    oss << *rep;
                }
                else {
                    std::string sjis;
                    sjis += static_cast<char>(ch);
                    sjis += static_cast<char>(input[i + 1]);
                    oss << shiftjis_to_utf8(sjis);
                }
                i += 2;
            }
            else {
                oss << static_cast<char>(ch);
                ++i;
            }
        }
    }

    return oss.str();
}

std::string postprocess_from_utf8(const std::string& input) {
    std::ostringstream oss;
    size_t i = 0;

    while (i < input.size()) {
        // 줄바꿈(\n) 무시
        if (input[i] == '\n') {
            ++i;
            continue;
        }

        // 치환 문자열 우선 검색 (여러 글자 가능)
        bool replaced = false;
        for (int len = 4; len >= 1; --len) { // 긴 치환 문자열 우선
            if (i + len <= input.size()) {
                std::string sub = input.substr(i, len);
                int hexVal = find_hexcode_from_text(sub);
                if (hexVal >= 0) {
                    oss.put(static_cast<char>((hexVal >> 8) & 0xFF));
                    oss.put(static_cast<char>(hexVal & 0xFF));
                    i += len;
                    replaced = true;
                    break;
                }
            }
        }
        if (replaced) continue;

        // 일반 문자 처리
        uint8_t ch = static_cast<uint8_t>(input[i]);

        if (ch <= 0x7F) {
            // ASCII는 그대로 저장
            oss.put(static_cast<char>(ch));
            ++i;
        }
        else {
            // UTF-8 멀티바이트 문자 시작
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

            std::string utf8char = input.substr(i, utf8_len);

            // 우선 EUC-KR로 변환
            std::string euckr = utf8_to_euckr(utf8char);
            if (euckr.size() == 2) {
                uint8_t first = static_cast<uint8_t>(euckr[0]);
                uint8_t second = static_cast<uint8_t>(euckr[1]);
                if (first >= 0xB0 && first <= 0xC8 && second >= 0xA1 && second <= 0xFE) {
                    oss.write(euckr.data(), euckr.size());
                    i += utf8_len;
                    continue;
                }
            }

            // EUC-KR 실패 → Shift-JIS 변환
            std::string shiftjis = utf8_to_shiftjis(utf8char);
            oss.write(shiftjis.data(), shiftjis.size());
            if (shiftjis == "?") {
                std::cerr << "[경고] EUC-KR 코드 변환 실패. 실패 문자: \"" << euckr << "\"\n";
            }
            i += utf8_len;
        }
    }

    return oss.str();
}

static std::string escape_csv_field(const std::string& field) {
    bool need_quote = field.find_first_of(",\"\n\r") != std::string::npos;
    if (!need_quote) return field;

    std::string escaped = "\"";
    for (char ch : field) {
        if (ch == '"') {
            escaped += "\"\""; // " → ""
        }
        else {
            escaped += ch;
        }
    }
    escaped += "\"";
    return escaped;
}

bool to_csv(const std::vector<CsvStringData>& data, const std::string& outputFilename) {
    std::ostringstream oss;
    oss << "\xEF\xBB\xBF"; // UTF-8 BOM
    oss << "File,ID,Charactor,Original,Translate\r\n";

    initialize_replacement_table(true);

    for (const auto& row : data) {
        auto pre = [&](const std::string& text) -> std::string {
            return preprocess_to_utf8(text);
            };

        oss << escape_csv_field(pre(row.filename)) << ".SDT,"
            << row.id << ','
            << escape_csv_field(pre(row.charactor)) << ','
            //<< escape_csv_field(pre(row.original)) << ','
            //<< escape_csv_field(pre(row.translated)) << "\r\n";
            << escape_csv_field(pre(row.original)) << ",\r\n";
    }

    std::ofstream out(outputFilename, std::ios::binary);
    if (!out) return false;
    out << oss.str();
    out.close();
    return true;
}

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

bool from_csv(const std::string& inputFilename, std::vector<CsvStringData>& outData) {
    std::ifstream in(inputFilename, std::ios::binary);
    if (!in) return false;
    outData.clear();

    initialize_replacement_table(false);

    std::string csvText((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    size_t bomSkip = (csvText.size() >= 3 &&
        (unsigned char)csvText[0] == 0xEF &&
        (unsigned char)csvText[1] == 0xBB &&
        (unsigned char)csvText[2] == 0xBF) ? 3 : 0;

    std::vector<std::string> fields;
    std::string currentLine;
    bool insideQuote = false;
    size_t i = bomSkip;

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

                if (outData.empty() && currentLine.find("File,ID,Charactor,Original,Translate") != std::string::npos) {
                    // 첫 행은 헤더, 무시
                    currentLine.clear();
                    ++i;
                    continue;
                }

                if (parse_csv_line(currentLine, fields)) {
                    if (fields.size() >= 4) {
                        CsvStringData row;
                        //row.filename = fields[0];
                        row.id = std::stoi(fields[1]);
                        //row.charactor = fields[2];
                        //row.original = fields[3];
                        //row.translated = fields[4];
                        if ((fields.size() >= 5) && (fields[4].length() > 0)) {
                            row.translated = postprocess_from_utf8(fields[4]);
                        }
                        else {
                            row.translated = postprocess_from_utf8(fields[3]);
                        }
                        outData.push_back(row);
                    }
                }

                currentLine.clear();
            }
        }
        ++i;
    }
    in.close();
    return true;
}