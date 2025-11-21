#include <iostream>
#include <conio.h>
#include "Util.h"
#include "Script.h"

void PrintTitle() {
    system("cls");
    std::cout << "========================================" << std::endl;
    std::cout << " MES <-> XLSX Converter Tool" << std::endl;
    std::cout << "========================================" << std::endl;
}

int main() {
    while (true) {
        PrintTitle();
        std::cout << "1. MES -> XLSX (Auto Detect Encoding)" << std::endl;
        std::cout << "2. XLSX -> MES (System Language: CP_ACP)" << std::endl;
        std::cout << "3. XLSX -> MES (Shift-JIS)" << std::endl;
        std::cout << "4. XLSX -> MES (Custom UTF-8)" << std::endl;
        std::cout << "Q. Quit" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Select Mode: ";

        char input = _getch();
        std::cout << input << std::endl;

        if (input == 'q' || input == 'Q') break;

        std::vector<std::wstring> files;
        Util::EncodingType targetEnc = Util::EncodingType::ShiftJIS;
        bool isExport = false; // true = MES->XLSX, false = XLSX->MES

        switch (input) {
        case '1':
            isExport = true;
            files = Util::GetMesFiles((LPWSTR)L"MES Files\0*.mes\0");
            break;
        case '2':
            targetEnc = Util::EncodingType::ACP;
            files = Util::GetMesFiles((LPWSTR)L"Excel Files\0*.xlsx\0");
            break;
        case '3':
            targetEnc = Util::EncodingType::ShiftJIS;
            files = Util::GetMesFiles((LPWSTR)L"Excel Files\0*.xlsx\0");
            break;
        case '4':
            targetEnc = Util::EncodingType::CustomUTF8;
            files = Util::GetMesFiles((LPWSTR)L"Excel Files\0*.xlsx\0");
            break;
        default:
            continue;
        }

        if (files.empty()) {
            std::cout << "No files selected." << std::endl;
            system("pause");
            continue;
        }

        std::cout << "\nProcessing " << files.size() << " files...\n" << std::endl;

        for (const auto& file : files) {
            if (isExport) {
                Script::ParseMes((LPWSTR)file.c_str());
            }
            else {
                Script::ApplyTransTo((LPWSTR)file.c_str(), targetEnc);
            }
        }

        std::cout << "\nDone." << std::endl;
        system("pause");
    }

    return 0;
}
