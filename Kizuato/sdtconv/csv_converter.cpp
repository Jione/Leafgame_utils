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

        // ���� ���ڿ� �ҷ�����
        if (++i < input.size()) {
            next = static_cast<uint8_t>(input[i]);
        }
        else {
            isLast = true;
        }

        // ���ڿ� ũ�� �Ǵ�
        if ((ch > 0x80) && !isLast) {
            if (ch < 0xB0 || ch > 0xC8 || (next >= 0xA1 && next <= 0xFE)) {
                ++i;
                hexVal = (ch << 8) | next;
                isWord = true;
            }
        }

        // ġȯ ���ڿ� �˻� �� ��ȯ
        const std::string* rep = find_replacement_string(hexVal);
        if (rep) {
            oss << *rep;
            continue;
        }

        // ASCII ����
        if (!isWord && ch <= 0x7F) {
            if (ch == '\\' && !isLast && next == 'n') {
                // \n �߰�
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
                            oss << "\n"; // �߰����� �ٹٲ� ����
                        }
                        else if (i > 3) {
                            uint8_t ch1 = static_cast<uint8_t>(input[i - 4]);
                            uint8_t ch2 = static_cast<uint8_t>(input[i - 3]);
                            chw = ((ch1 << 8) | ch2);
                            if (((ch1 == (uint8_t)'\\') && (ch2 == (uint8_t)'k')) || (chw == 0x8176) || (chw == 0x8178)) {
                                oss << "\n"; // �߰����� �ٹٲ� ����
                            }
                        }
                    }
                }
            }
            else {
                oss << static_cast<char>(ch);
            }
        }
        // Shift-JIS �ݰ� ����
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
                oss << shiftjis_to_utf8(wordString); // Shift-JIS ����
            }
        }
    }

    return oss.str();
}

std::string postprocess_from_utf8(const std::string& input) {
    std::ostringstream oss;
    size_t i = 0;

    while (i < input.size()) {
        // �ٹٲ�(\r, \n) ����
        if ((input[i] == '\n') || (input[i] == '\r')) {
            ++i;
            continue;
        }

        // ġȯ ���ڿ� �켱 �˻� (���� ���� ����)
        bool replaced = false;
        for (int len = 4; len >= 1; --len) { // �� ġȯ ���ڿ� �켱
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

        // �Ϲ� ���� ó��
        uint8_t ch = static_cast<uint8_t>(input[i]);

        if (ch <= 0x7F) {
            // ASCII�� �״�� ����
            oss.put(static_cast<char>(ch));
            ++i;
        }
        else {
            // UTF-8 ��Ƽ����Ʈ ���� ����
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

            // �켱 EUC-KR�� ��ȯ
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

            // EUC-KR ���� �� Shift-JIS ��ȯ
            std::string shiftjis = utf8_to_shiftjis(utf8char);
            oss.write(shiftjis.data(), shiftjis.size());
            if (shiftjis == "?") {
                std::cerr << "[���] EUC-KR �ڵ� ��ȯ ����. ID: " << nowIndex << ", ���� ���� : \"" << euckr << "\"\n";
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
            escaped += "\"\""; // " �� ""
        }
        else {
            escaped += ch;
        }
    }
    escaped += "\"";
    return escaped;
}

// \n(0x0A) �������� ���ڿ��� �и��Ͽ� std::vector<std::string> ��ȯ
std::vector<std::string> splitStringByNewline(const std::string& input) {
    std::vector<std::string> result;
    size_t start = 0;
    while (start <= input.size()) {
        auto pos = input.find('\n', start);
        if (pos == std::string::npos) {
            // ������ �κ�
            result.emplace_back(input.substr(start));
            break;
        }
        // start���� pos-1���� �κ� ���ڿ�
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

        // �ε����� ���� ����
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

// CSV 1���� �ʵ� ������ �и�
static bool parse_csv_line(const std::string& line, std::vector<std::string>& fields) {
    fields.clear();
    size_t i = 0;
    const size_t len = line.length();

    while (i < len) {
        std::string field;
        if (line[i] == '"') {
            // ����ǥ ���� �ʵ�
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
            // �Ϲ� �ʵ�
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
            // ����ǥ ���� ���
            insideQuote = !insideQuote;
        }

        if (!insideQuote && ch == '\n') {
            // �� ��
            if (!currentLine.empty() && (currentLine.back() == '\n')) {
                if (currentLine.size() >= 2 && currentLine[currentLine.size() - 2] == '\r') {
                    currentLine.erase(currentLine.size() - 2); // CRLF ����
                }
                else {
                    currentLine.erase(currentLine.size() - 1); // LF�� ����
                }

                if (currentLine.empty()) {
                    ++i;
                    continue;
                }

                if (parse_csv_line(currentLine, fields)) {
                    if (outData.empty() && !isData) {
                        // ù ���� ���, ����
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