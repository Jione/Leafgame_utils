#include "Common.h"
#include "Util.h"
#include "GRP.h"

int main() {
    // Ensure console supports proper output
    setlocale(LC_ALL, "");

    while (true) {
        std::cout << "========================================" << std::endl;
        std::cout << " GRP <-> PNG Converter Tool" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "1. Convert GRP to PNG (Ignore MSK)" << std::endl;
        std::cout << "2. Convert GRP to PNG (Apply MSK Transparency)" << std::endl;
        std::cout << "3. Update GRP from PNG folder (Full Update: Rewrite C16)" << std::endl;
        std::cout << "4. Update GRP from PNG folder (GRP Only: Preserve C16)" << std::endl;
        std::cout << "5. Exit" << std::endl;
        std::cout << "Select Mode: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        if (choice == 5) break;

        if (choice == 1 || choice == 2) {
            // ... (기존 코드와 동일)
            std::vector<std::wstring> files = Util::GetGrpFiles();
            if (files.empty()) {
                std::cout << "No files selected." << std::endl;
                continue;
            }

            for (const auto& file : files) {
                GRP::ConvertPng(file, (choice == 2));
            }
            std::cout << "Conversion completed." << std::endl;
        }
        else if (choice == 3 || choice == 4) {
            std::wstring folder = Util::GetUpdateFolder();
            if (folder.empty()) {
                std::cout << "No folder selected." << std::endl;
                continue;
            }

            // Pass true if choice is 4 (Preserve Palette)
            GRP::Update(folder, (choice == 4));
            std::cout << "Update completed." << std::endl;
        }
    }

    return 0;
}
