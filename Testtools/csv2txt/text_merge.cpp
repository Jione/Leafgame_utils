// main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

#include "text_merge.h"


// Writing-buffer ��Ģ�� �����Ͽ� in ���ڿ��� ��ȯ�� �� ��ȯ
static std::string applyWritingRules(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        char c = in[i];
        if ((c == '\r') || (c == '\n')) {
            // 0x0A�� ����
            continue;
        }
        else if (c == '\\' && i + 1 < in.size() && in[i + 1] == 'n') {
            // "\\n" �߰� �� �״�� ���� CRLF �߰�
            out.push_back('\\');
            out.push_back('n');
            out.append("\r\n");
            ++i; // 'n' ��ŵ
        }
        else {
            out.push_back(c);
        }
    }
    return out;
}

// ���ڿ��� CRLF("\r\n")�� �������� �˻�
static bool endsWithCRLF(const std::string& s) {
    return s.size() >= 2
        && s[s.size() - 2] == '\r'
        && s[s.size() - 1] == '\n';
}

// ���ڿ��� CRLF("\r\n")�� �������� �˻�
static bool findPos(const std::string& raw, size_t& pos, size_t seekpos) {
    size_t pos1 = raw.find(".SDT", seekpos);
    size_t pos2 = raw.find(".sdt", seekpos);
    if (pos1 != std::string::npos && (pos2 == std::string::npos || pos1 < pos2)) {
        pos = pos1;
        return true;
    }
    else if (pos2 != std::string::npos) {
        pos = pos2;
        return true;
    }
    else {
        pos = std::string::npos;
        return false;
    }
}

// Type 1 ó��: raw ��ü�� ���� .SDT/.sdt �������� ������ �κ��� �и��Ͽ� ��Ģ ����
void processType1(const std::string& raw, std::ofstream& ofs) {
    size_t pos, seekpos = 0;
    findPos(raw, pos, seekpos);

    while (pos != std::string::npos) {
        int i = pos;
        while (seekpos < i) {
            if (raw[--i] == '\r') {
                break;
            }
        }
        std::string str = raw.substr(seekpos, (i - seekpos));

        while (!str.empty() && str.back() == ',') str.pop_back();
        if (!str.empty() && str.front() == '"') str.erase(0, 1);
        if (!str.empty() && str.back() == '"') str.pop_back();

        std::string str_out = applyWritingRules(str);
        ofs << str_out;
        if (!endsWithCRLF(str_out)) {
            ofs << "\r\n";
        }
        seekpos = pos + 4;
        int commaCount = 0;
        while (seekpos < raw.size() && commaCount < 3) {
            if (raw[seekpos] == ',') ++commaCount;
            ++seekpos;
        }
        findPos(raw, pos, seekpos);
    }
    std::string str = raw.substr(seekpos);

    while (!str.empty() && str.back() == ',') str.pop_back();
    if (!str.empty() && str.front() == '"') str.erase(0, 1);
    if (!str.empty() && str.back() == '"') str.pop_back();

    std::string str_out = applyWritingRules(str);
    ofs << str_out;
    if (!endsWithCRLF(str_out)) {
        ofs << "\r\n";
    }
}

// Type 2 ó��: �� �پ� �о� ���� "\\n"���� ���� ����, ���� ������ CRLF�� ����
void processType2(const std::string& path, std::ofstream& ofs) {
    std::ifstream ifs(path);
    std::string line;
    while (std::getline(ifs, line)) {
        // CRLF �� getline�� '\n' ����, ���� '\r' ����
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        std::string out;
        out.reserve(line.size());
        size_t start = 0;
        while (start < line.size()) {
            size_t p = line.find("\\n", start);
            if (p == std::string::npos) {
                out.append(line.substr(start));
                break;
            }
            // "\\n" �߰�
            out.append(line.substr(start, p - start + 2)); // include "\n"
            if (p + 2 < line.size()) {
                // ������ ���� CRLF ����
                out.append("\r\n");
                start = p + 2;
            }
            else {
                // ���� ���� ���� ��ȯ �� �ϰ� �״�� �ΰ� ����
                start = p + 2;
                break;
            }
        }
        // �� �ٸ��� ���� ������ CRLF
        out.append("\r\n");
        ofs << out;
    }
}

int textmerge(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "����: "
            << argv[0]
            << " <input_file_or_directory> <output_file>\n";
        return 1;
    }
    fs::path inPath = argv[1];
    fs::path outFile = argv[2];

    // ��� ���� ����
    std::ofstream ofs(outFile.string(), std::ios::binary);
    if (!ofs) {
        std::cerr << "��� ���� ���� ����: " << outFile << "\n";
        return 1;
    }

    // ó���� .txt ���� ��� ����
    std::vector<fs::path> files;
    if (fs::is_directory(inPath)) {
        for (auto& entry : fs::directory_iterator(inPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt")
                files.push_back(entry.path());
        }
    }
    else if (fs::is_regular_file(inPath) && inPath.extension() == ".txt") {
        files.push_back(inPath);
    }
    else {
        std::cerr << "�Է� ��ΰ� ���͸���, .txt ���ϵ� �ƴմϴ�.\n";
        return 1;
    }

    // �� ���ϸ��� Type �˻� �� ó��
    for (auto& fp : files) {
        std::cout << "ó����: " << fp.filename() << std::endl;

        // ���� ��ü�� �о�鿩 Type1/Type2 �Ǵ�
        std::ifstream ifs(fp.string(), std::ios::binary);
        std::string raw((std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>());
        ifs.close();

        if (raw.find(".SDT") != std::string::npos ||
            raw.find(".sdt") != std::string::npos) {
            processType1(raw, ofs);
        }
        else {
            processType2(fp.string(), ofs);
        }
    }

    return 0;
}