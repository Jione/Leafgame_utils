#define NOMINMAX

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <locale>
#include "EncodingDetector.h"
#include "CompareConfig.h"

// 전체 텍스트를 줄 단위로 분할
static std::vector<std::wstring> splitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    std::wstring current;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'\r') {
            if (i + 1 < text.size() && text[i + 1] == L'\n') {
                lines.push_back(current);
                current.clear();
                ++i;
            }
            else {
                lines.push_back(current);
                current.clear();
            }
        }
        else if (text[i] == L'\n') {
            lines.push_back(current);
            current.clear();
        }
        else {
            current.push_back(text[i]);
        }
    }
    lines.push_back(current);
    return lines;
}

int wmain(int argc, wchar_t* argv[]) {
    // 콘솔에서 UTF-8 출력 활성화
    SetConsoleOutputCP(CP_UTF8);
    std::locale::global(std::locale(""));
    int errcount = 0;

    if (argc != 3) {
        std::wcerr << L"사용법: compare <파일1> <파일2>\n";
        return 1;
    }
    std::string file1 = std::string(argv[1], argv[1] + wcslen(argv[1]));
    std::string file2 = std::string(argv[2], argv[2] + wcslen(argv[2]));

    auto content1 = readFileAsUTF16(file1);
    auto content2 = readFileAsUTF16(file2);
    auto lines1 = splitLines(content1);
    auto lines2 = splitLines(content2);

    size_t maxLines = std::max(lines1.size(), lines2.size());
    for (size_t i = 0; i < maxLines; ++i) {
        const auto& l1 = i < lines1.size() ? lines1[i] : L"";
        const auto& l2 = i < lines2.size() ? lines2[i] : L"";
        int lineNo = static_cast<int>(i) + 1;
        if (errcount >= 10) break;

        // 첫 번째 문자 비교
        if (!l1.empty() && std::find(CompareConfig::openingChars.begin(), CompareConfig::openingChars.end(), l1[0]) != CompareConfig::openingChars.end()) {
            if (l2.empty() || l1[0] != l2[0]) {
                std::wcout << L"Line " << std::setw(5) << std::setfill(L'0') << lineNo
                    << L":첫 번째 문자 형식이 다름\n";
                errcount++;
            }
        }
        // 마지막 문자 비교
        if (!l1.empty()) {
            wchar_t last1 = l1.back();
            if (std::find(CompareConfig::closingChars.begin(), CompareConfig::closingChars.end(), last1) != CompareConfig::closingChars.end()) {
                wchar_t last2 = l2.empty() ? L'\0' : l2.back();
                if (last1 != last2) {
                    std::wcout << L"Line " << std::setw(5) << std::setfill(L'0') << lineNo
                        << L":마지막 문자 형식이 다름\n";
                    errcount++;
                }
            }
        }
        // 낫표 개수 비교
        for (auto i = 0; i < CompareConfig::quoteChars.size(); i++) {
            auto findchar = CompareConfig::quoteChars[i];
            int count1 = std::count(l1.begin(), l1.end(), findchar);
            int count2 = std::count(l2.begin(), l2.end(), findchar);
            if (count1 != count2) {
                std::wcout << L"Line " << std::setw(5) << std::setfill(L'0') << lineNo
                    << L":문자 \""<< findchar << L"\"의 개수가 다름 (" << count1 << " : " << count2  << ")\n";
                errcount++;
                break;
            }
        }
        // 제어코드 비교
        auto countSub = [](const std::wstring& s, const std::wstring& sub) {
            int cnt = 0; size_t pos = 0;
            while ((pos = s.find(sub, pos)) != std::wstring::npos) {
                ++cnt; pos += sub.length();
            }
            return cnt;
            };
        if (countSub(l1, L"\\k") != countSub(l2, L"\\k") ||
            countSub(l1, L"\\k\\n") != countSub(l2, L"\\k\\n")) {
            std::wcout << L"Line " << std::setw(5) << std::setfill(L'0') << lineNo
                << L":제어코드 정보가 손실됨\n";
            errcount++;
        }
    }
    std::wcout << L"총 " << errcount << L"개의 에러가 발견됨.";
    return 0;
}
