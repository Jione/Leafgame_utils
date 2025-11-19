#include "Common.h"
#include "Util.h"
#include "Archive.h"
#include <conio.h>

int main() {
    // Set locale for console output
    setlocale(LC_ALL, "");

    std::wcout << L"=== LZSS Archiver Tool ===" << std::endl;
    std::wcout << L"1. Extract Archive" << std::endl;
    std::wcout << L"2. Create Archive" << std::endl;
    std::wcout << L"Select Mode > ";

    char choice = _getch();
    std::wcout << choice << std::endl << std::endl;

    if (choice == '1') {
        std::vector<std::wstring> archives;
        if (Util::GetArchiveFiles(archives)) {
            for (auto& file : archives) {
                Archive::ExtractArchive((LPWSTR)file.c_str());
            }
        }
        else {
            std::wcout << L"Canceled." << std::endl;
        }
    }
    else if (choice == '2') {
        std::wstring folder = Util::GetTargetFolder();
        if (!folder.empty()) {
            Archive::CreateArchive((LPWSTR)folder.c_str());
        }
        else {
            std::wcout << L"Canceled." << std::endl;
        }
    }
    else {
        std::wcout << L"Invalid selection." << std::endl;
    }

    std::wcout << L"\nPress any key to exit...";
    _getch();
    return 0;
}
