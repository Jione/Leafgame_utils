#include <iostream>
#include <conio.h>
#include "Util.h"
#include "Script.h"

void PrintTitle() {
    system("cls");
    std::cout << "========================================" << std::endl;
    std::cout << " MES <-> XLSX Converter (Custom)" << std::endl;
    std::cout << "========================================" << std::endl;
}

int main() {
    while (true) {
        PrintTitle();
        std::cout << "0. MES -> TXT (Export)" << std::endl;
        std::cout << "1. MES -> XLSX (Export)" << std::endl;
        std::cout << "2. XLSX -> MES (Import)" << std::endl;
        std::cout << "Q. Quit" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Select: ";

        char input = _getch();
        std::cout << input << std::endl;

        if (input == 'q' || input == 'Q') break;

        if (input != '1' && input != '2' && input != '0') continue;

        int runMode = input - '0';
        bool isExport = !(runMode == 2);
        std::vector<std::wstring> files;

        if (isExport) {
            files = Util::GetMesFiles((LPWSTR)L"MES Files\0*.mes\0");
        }
        else {
            files = Util::GetMesFiles((LPWSTR)L"Excel Files\0*.xlsx\0");
        }

        if (files.empty()) {
            std::cout << "No files selected." << std::endl;
            system("pause");
            continue;
        }

        std::cout << "\nProcessing " << files.size() << " files...\n" << std::endl;

        for (const auto& file : files) {
            if (isExport) {
                Script::ParseMes((LPWSTR)file.c_str(), runMode ? true : false);
            }
            else {
                Script::ApplyTransTo((LPWSTR)file.c_str());
            }
        }

        std::cout << "\nDone." << std::endl;
        system("pause");
    }

    return 0;
}
