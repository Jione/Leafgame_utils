#include <iostream>
#include "stdlib.h"
#include "font_exporter.h"
#include "font_loader.h"
#include "image_builder.h"

static int get_font_file_Type(const std::string& filename) {
    size_t base = filename.find_last_of("/\\");
    std::string name = (base == std::string::npos) ? filename : filename.substr(base + 1);

    if (name.length() < 10 || name.substr(0, 4) != "font")
        return -1;

    std::string ext = name.substr(6, 3); // ".fd" or ".fk"
    if (ext == ".fd" || ext == ".fk") {
        if (13 < name.length()) {
            ext = name.substr(10, 4);
            if (ext == ".png") {
                return 1;
            }
        }
        else {
            return 0;
        }
    }
    return -1;
}

int main(int argc, char* argv[]) {
    std::string inputPath;
    int argType = -1;

    if (2 <= argc) {
        inputPath = argv[1];
        argType = get_font_file_Type(inputPath);

        if (argType == 0) {
            std::string outputPath = inputPath + ".png";
            std::vector<unsigned char> image;
            int width = 0, height = 0;

            if (!load_font_file_to_image(inputPath, image, width, height)) {
                std::cerr << "[Error] Failed to convert font file.\n\n";
                system("PAUSE");
                return 1;
            }

            if (!save_image_as_png(outputPath, image, width, height)) {
                std::cerr << "[Error] Failed to save PNG file.\n\n";
                system("PAUSE");
                return 1;
            }

            std::cout << "[Success] Saved image: " << outputPath << " (" << width << "x" << height << ")\n\n";
            system("PAUSE");
            return 0;
        }
        else if (argType == 1) {
            if (!export_png_to_font_binary(inputPath)) {
                std::cerr << "[Error] Failed to save .fdN / .fkN file.\n\n";
                system("PAUSE");
                return 1;
            }
            system("PAUSE");
            return 0;
        }
    }

    std::cerr << "Usage: fontutil.exe <font##.fd#/fk# | font##.fd#/fk#.png>\n\n";
    system("PAUSE");
    return 1;
}