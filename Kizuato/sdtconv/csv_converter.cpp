#include "csv_converter.h"
#include "encoding_converter.h"
#include "replacement_table.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

static int nowIndex = 0;

std::string preprocess_to_utf8(const std::string& input) {
    std::ostringstream oss;
    size_t i = 0;
    bool isLast = false;
    while (i < input.size()) {
        uint8_t ch = static_cast<uint8_t>(input[i]);
        uint8_t next;
        uint16_t hexVal = ch;
        bool isWord = false;

        // 다음 문자열 불러오기
        if (++i < input.size()) {
            next = static_cast<uint8_t>(input[i]);
        }
        else {
            isLast = true;
        }

        // 문자열 크기 판단
        if ((ch > 0x80) && !isLast) {
            if (ch < 0xB0 || ch > 0xC8 || (next >= 0xA1 && next <= 0xFE)) {
                ++i;
                hexVal = (ch << 8) | next;
                isWord = true;
            }
        }

        // 치환 문자열 검색 및 변환
        const std::string* rep = find_replacement_string(hexVal);
        if (rep) {
            oss << *rep;
            continue;
        }

        // ASCII 범위
        if (!isWord && ch <= 0x7F) {
            if (ch == '\\' && !isLast && next == 'n') {
                // \n 발견
                oss << "\\n";
                if (++i < input.size()) {
                    if (input[i] == '>') {
                        if (++i < input.size()) {
                            oss << ">\n";
                        }
                        else {
                            oss << ">";
                        }
                    }
                    else if ((i + 1) < input.size()) {
                        uint16_t chw = (static_cast<uint8_t>(input[i]) << 8) | static_cast<uint8_t>(input[i + 1]);
                        if (chw == 0x8140 || chw == 0x8175 || chw == 0x8177) {
                            oss << "\n"; // 중간에만 줄바꿈 삽입
                        }
                        else if (i > 3) {
                            uint8_t ch1 = static_cast<uint8_t>(input[i - 4]);
                            uint8_t ch2 = static_cast<uint8_t>(input[i - 3]);
                            chw = ((ch1 << 8) | ch2);
                            if (((ch1 == (uint8_t)'\\') && (ch2 == (uint8_t)'k')) || (chw == 0x8176) || (chw == 0x8178)) {
                                oss << "\n"; // 중간에만 줄바꿈 삽입
                            }
                        }
                    }
                }
            }
            else {
                oss << static_cast<char>(ch);
            }
        }
        // Shift-JIS 반각 영역
        else if (!isWord) {
            std::string sjis;
            sjis += static_cast<char>(ch);
            oss << shiftjis_to_utf8(sjis);
        }
        else {
            std::string wordString;
            wordString += static_cast<char>(ch);
            wordString += static_cast<char>(next);

            if (ch >= 0xB0 && ch <= 0xC8) {
                oss << euckr_to_utf8(wordString); // EUC-KR
            }
            else {
                oss << shiftjis_to_utf8(wordString); // Shift-JIS 전각
            }
        }
    }

    return oss.str();
}

std::string postprocess_from_utf8(const std::string& input) {
    std::ostringstream oss;
    size_t i = 0;

    while (i < input.size()) {
        // 줄바꿈(\r, \n) 무시
        if ((input[i] == '\n') || (input[i] == '\r')) {
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
                    if (hexVal > 0xFF) {
                        oss.put(static_cast<char>((hexVal >> 8) & 0xFF));
                    }
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
                std::cerr << "[경고] EUC-KR 코드 변환 실패. ID: " << nowIndex << ", 실패 문자 : \"" << euckr << "\"\n";
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

// \n(0x0A) 기준으로 문자열을 분리하여 std::vector<std::string> 반환
std::vector<std::string> splitStringByNewline(const std::string& input) {
    std::vector<std::string> result;
    size_t start = 0;
    while (start <= input.size()) {
        auto pos = input.find('\n', start);
        if (pos == std::string::npos) {
            // 마지막 부분
            result.emplace_back(input.substr(start));
            break;
        }
        // start부터 pos-1까지 부분 문자열
        result.emplace_back(input.substr(start, pos - start));
        start = pos + 1;
    }
    return result;
}

bool to_csv(const std::vector<CsvStringData>& data, const std::string& outputFilename) {
    std::string txtFilename = outputFilename.substr(0, outputFilename.length() - 4) + ".txt";
    std::ostringstream oss;
    std::ostringstream tss;
    oss << "\xEF\xBB\xBF"; // UTF-8 BOM
    tss << "\xEF\xBB\xBF"; // UTF-8 BOM
    oss << "File,ID,Charactor,Original,Translate\r\n";
    std::string findStrA, findStrB;
    findStrA = findStrB += static_cast<char>(0x81);
    findStrA += static_cast<char>(0x76);
    findStrB += static_cast<char>(0x78);
    findStrA = shiftjis_to_utf8(findStrA);
    findStrB = shiftjis_to_utf8(findStrB);

    initialize_replacement_table(true);

    for (const auto& row : data) {
        int i = 0;
        auto pre = [&](const std::string& text) -> std::string {
            return (i++ == 0) ? preprocess_to_utf8(text) : euckr_to_utf8(text);
            };
        auto lines = splitStringByNewline(pre(row.original));

        // 인덱스로 순차 접근
        bool isChar = pre(row.charactor).length() != 0;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (isChar) {
                oss << escape_csv_field(pre(row.filename)) << ".SDT,"
                    << row.id << ','
                    << escape_csv_field(pre(row.charactor)) << ','
                    << escape_csv_field(lines[i]) << ",\r\n";
                isChar = (isChar && !((lines[i].find(findStrA) != std::string::npos) || (lines[i].find(findStrB) != std::string::npos)));
            }
            else {
                oss << escape_csv_field(pre(row.filename)) << ".SDT,"
                    << row.id << ",,"
                    << escape_csv_field(lines[i]) << ",\r\n";
            }
            tss << lines[i] << "\r\n";
        }
    }

    std::ofstream out(outputFilename, std::ios::binary);
    if (!out) return false;
    out << oss.str();
    out.close();

    std::ofstream txtout(txtFilename, std::ios::binary);
    if (!txtout) return false;
    txtout << tss.str();
    txtout.close();

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
    int id = -1;
    std::string idString = "";
    bool isTrans = false;
    bool isData = false;

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
                    if (outData.empty() && !isData) {
                        // 첫 행은 헤더, 무시
                        isData = true;
                    }
                    else if (fields.size() >= 4) {
                        int filedId = -1;
                        isTrans = ((fields.size() >= 5) && (fields[4].length() > 0));
                        if (fields[1].length() != 0) {
                            nowIndex = filedId = std::stoi(fields[1]);
                        }
                        if ((id != filedId) && (filedId != -1) && (id != -1)) {
                            CsvStringData row;
                            row.id = id;
                            row.translated = idString;
                            outData.push_back(row);
                            id = filedId;
                            idString = postprocess_from_utf8(fields[(isTrans ? 4 : 3)]);
                        }
                        else {
                            if (filedId != -1) {
                                id = filedId;
                            }
                            idString += postprocess_from_utf8(fields[(isTrans ? 4 : 3)]);
                        }
                    }
                }
                currentLine.clear();
            }
        }
        ++i;
    }
    if (id != -1) {
        CsvStringData row;
        row.id = id;
        row.translated = idString;
        outData.push_back(row);
    }
    in.close();
    return true;
}