#include "packager.h"
#include "extractor.h"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: packutil.exe <folder_path | file.pak>\n\n";
        system("PAUSE");
        return 1;
    }

    std::string inputPath = argv[1];

    if (inputPath.size() > 4 &&
        (inputPath.substr(inputPath.size() - 4) == ".pak" || inputPath.substr(inputPath.size() - 4) == ".PAK")) {
        if (!extract_package(inputPath)) {
            std::cerr << "[Error] Extraction failed.\n\n";
            system("PAUSE");
            return 1;
        }
    }
    else {
        if (!create_package(inputPath)) {
            std::cerr << "[Error] Packaging failed.\n\n";
            system("PAUSE");
            return 1;
        }
    }

    system("PAUSE");
    return 0;
}