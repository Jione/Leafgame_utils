// main.cpp
#include "text_merge.h"
#include "csv_parser.h"
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "사용법: "
            << argv[0]
            << " <input_csv_file>\n";
        return 1;
    }
    fs::path inFile = argv[1];
    //fs::path inFile = "F:\\_Dev\\_VC\\Leafgame_utils\\Routes\\x64\\Release\\test\\sdt.csv";

    std::string baseName = inFile.stem().string();
    std::string outFile = baseName + ".txt";

    from_csv(inFile.string(), outFile);
    std::cout << "변환 성공\n";
    return 0;
}