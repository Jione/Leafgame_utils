#include "GRP.h"
#include <map>
#include <cmath>
#include <numeric>

namespace GRP {

    // Helper to read C16 palette
    std::vector<ColorIndex> ReadPalette(const fs::path& c16Path) {
        std::vector<ColorIndex> palette;
        std::ifstream file(c16Path, std::ios::binary);
        if (!file.is_open()) return palette;

        C16Header header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        palette.resize(header.uCount);
        file.read(reinterpret_cast<char*>(palette.data()), header.uCount * sizeof(ColorIndex));
        return palette;
    }

    // Find the closest color index in the palette (Euclidean distance)
    // Skips reserved indices (0 and 0xF0-0xFF)
    uint8_t FindClosestIndex(const std::vector<ColorIndex>& palette, uint8_t r, uint8_t g, uint8_t b) {
        int minDistance = INT_MAX;
        uint8_t bestIndex = 1; // Default fallback safe index

        for (const auto& color : palette) {
            // Check Reserved Indices: 0 and 0xF0(240) ~ 0xFF(255)
            if (color.IndexNum == 0 || (color.IndexNum >= 0xF0 && color.IndexNum <= 0xFF)) continue;

            int diffR = (int)color.Red - r;
            int diffG = (int)color.Green - g;
            int diffB = (int)color.Blue - b;

            // Squared Euclidean distance
            int distance = (diffR * diffR) + (diffG * diffG) + (diffB * diffB);

            if (distance < minDistance) {
                minDistance = distance;
                bestIndex = color.IndexNum;
            }
        }
        return bestIndex;
    }

    bool ConvertPng(std::wstring lpTargetFile, bool bApplyMask) {
        fs::path grpPath(lpTargetFile);
        std::wstring filename = grpPath.filename().wstring();

        // Check if it is a font file (starts with "font")
        bool isFont = (filename.find(L"font") == 0);

        std::ifstream grpFile(grpPath, std::ios::binary);
        if (!grpFile.is_open()) return false;

        uint16_t width = 0, height = 0;
        std::vector<uint8_t> indices;
        std::vector<unsigned char> rawImage; // RGBA buffer for lodepng

        // 1. Read GRP
        if (isFont) {
            FontGRPHeader header;
            grpFile.read(reinterpret_cast<char*>(&header), sizeof(header));
            width = header.sizeX;
            height = header.sizeY;

            // Size Inference for Font is less common but safe to check basic validity if needed
            // Usually Font GRPs are standard, but if 0, logic below for standard GRP can be adapted.
            // For now, assuming Font headers are correct as per spec, but basic read check:

            size_t expectedSize = (size_t)width * height;
            size_t fileSize = fs::file_size(grpPath);
            size_t dataSize = fileSize - sizeof(FontGRPHeader);

            if (expectedSize == 0 && dataSize > 0) {
                // Fallback if both 0
                width = (uint16_t)std::sqrt(dataSize);
                height = width;
            }

            indices.resize(dataSize);
            grpFile.read(reinterpret_cast<char*>(indices.data()), indices.size());

            // Font to RGBA (No Palette, 0x0=White, 0x1A=Black)
            rawImage.resize(width * height * 4);
            for (size_t i = 0; i < indices.size(); ++i) {
                uint8_t val = indices[i];
                size_t p = i * 4;
                if (val == 0x1A) { // Black
                    rawImage[p + 0] = 0; rawImage[p + 1] = 0; rawImage[p + 2] = 0; rawImage[p + 3] = 255;
                }
                else { // White
                    rawImage[p + 0] = 255; rawImage[p + 1] = 255; rawImage[p + 2] = 255; rawImage[p + 3] = 255;
                }
            }
        }
        else {
            GRPHeader header;
            grpFile.read(reinterpret_cast<char*>(&header), sizeof(header));
            width = header.sizeX;
            height = header.sizeY;

            // Size Inference Logic
            size_t fileSize = fs::file_size(grpPath);
            size_t dataSize = fileSize - sizeof(GRPHeader);

            if (width == 0 && height == 0) {
                // Both missing: Assume Square
                width = (uint16_t)std::sqrt(dataSize);
                height = width;
                std::wcout << L"[Info] Inferred Size (Square): " << width << L"x" << height << std::endl;
            }
            else if (width == 0 && height > 0) {
                width = (uint16_t)(dataSize / height);
                std::wcout << L"[Info] Inferred Width: " << width << std::endl;
            }
            else if (height == 0 && width > 0) {
                height = (uint16_t)(dataSize / width);
                std::wcout << L"[Info] Inferred Height: " << height << std::endl;
            }

            // Safety check to prevent vector alloc errors
            if (width == 0 || height == 0) {
                std::cerr << "Error: Could not determine dimensions for " << filename.c_str() << std::endl;
                return false;
            }

            indices.resize(width * height);
            grpFile.read(reinterpret_cast<char*>(indices.data()), indices.size());

            // Load C16 Palette
            fs::path c16Path = grpPath;
            c16Path.replace_extension(L".c16");
            std::vector<ColorIndex> palette = ReadPalette(c16Path);

            // Duplicate Color Check
            std::map<uint32_t, std::vector<uint8_t>> colorMapCheck;
            for (const auto& p : palette) {
                if (p.IndexNum == 0) continue; // Skip reserved transparent
                uint32_t rgb = (p.Red << 16) | (p.Green << 8) | p.Blue;
                colorMapCheck[rgb].push_back(p.IndexNum);
            }

            for (const auto& item : colorMapCheck) {
                if (item.second.size() > 1) {
                    std::wcout << L"[Warning] Duplicate Color Detected (RGB: " << std::hex << item.first << std::dec << L") at Indices: ";
                    for (auto idx : item.second) std::cout << (int)idx << " ";
                    std::cout << std::endl;
                }
            }

            // Load MSK if requested
            std::vector<uint8_t> maskData;
            if (bApplyMask) {
                fs::path mskPath = grpPath;
                mskPath.replace_extension(L".msk");
                if (fs::exists(mskPath)) {
                    std::ifstream mskFile(mskPath, std::ios::binary);
                    maskData.resize(width * height);
                    mskFile.read(reinterpret_cast<char*>(maskData.data()), maskData.size());
                }
            }

            // Prepare Color Lookup Table (Map 0-255 index to RGB)
            struct SimpleColor { uint8_t r, g, b; };
            std::vector<SimpleColor> colorMap(256);

            // Initialize with grayscale as fallback
            for (int i = 0; i < 256; ++i) {
                colorMap[i] = { (uint8_t)i, (uint8_t)i, (uint8_t)i };
            }

            // Fill map using IndexNum from C16 palette
            for (const auto& entry : palette) {
                colorMap[entry.IndexNum] = { entry.Red, entry.Green, entry.Blue };
            }

            // Indexed to RGBA
            rawImage.resize(width * height * 4);
            for (size_t i = 0; i < indices.size(); ++i) {
                uint8_t idx = indices[i];
                size_t p = i * 4;

                uint8_t a = 255;

                // Index 0 is always Transparent
                if (idx == 0) {
                    a = 0;
                }

                // Use mapped color
                uint8_t r = colorMap[idx].r;
                uint8_t g = colorMap[idx].g;
                uint8_t b = colorMap[idx].b;

                // Apply Mask (Override)
                if (bApplyMask && !maskData.empty()) {
                    // If mask says hidden (0xFF), force alpha 0
                    if (maskData[i] == 0xFF) a = 0;
                }

                rawImage[p + 0] = r;
                rawImage[p + 1] = g;
                rawImage[p + 2] = b;
                rawImage[p + 3] = a;
            }
        }

        // 2. Write PNG
        fs::path pngDir = grpPath.parent_path() / L"png";
        fs::create_directories(pngDir);
        fs::path pngPath = pngDir / filename;
        pngPath.replace_extension(L".png");

        unsigned error = lodepng::encode(pngPath.string(), rawImage, width, height);
        if (error) {
            std::cout << "Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
            return false;
        }

        std::wcout << L"Converted: " << filename << L" -> png\\" << pngPath.filename() << std::endl;
        return true;
    }

    // Internal function to process single PNG update
    void ProcessSingleUpdate(const fs::path& pngPath, const fs::path& targetFolder, bool bPreservePalette) {
        std::wstring filename = pngPath.filename().wstring();
        bool isFont = (filename.find(L"font") == 0);
        std::wstring baseName = pngPath.stem().wstring();

        std::vector<unsigned char> image;
        unsigned width, height;
        unsigned error = lodepng::decode(image, width, height, pngPath.string());
        if (error) {
            std::cout << "Decoder error " << error << std::endl;
            return;
        }

        // Paths
        fs::path grpPath = targetFolder / baseName;
        grpPath.replace_extension(L".grp");
        fs::path c16Path = targetFolder / baseName;
        c16Path.replace_extension(L".c16");
        fs::path mskPath = targetFolder / baseName;
        mskPath.replace_extension(L".msk");

        // Determine transparency for Mask
        std::vector<uint8_t> mskData(width * height, 0x00); // Default visible
        bool hasTransparency = false;

        // Check if we should skip mask update (Same as before)
        bool bSkipMaskUpdate = false;
        if (fs::exists(mskPath)) {
            std::ifstream oldMsk(mskPath, std::ios::binary | std::ios::ate);
            if (oldMsk.is_open()) {
                std::streamsize size = oldMsk.tellg();
                oldMsk.seekg(0, std::ios::beg);
                if (size > 0) {
                    std::vector<uint8_t> tempMsk(size);
                    oldMsk.read(reinterpret_cast<char*>(tempMsk.data()), size);
                    bool allZero = true;
                    for (auto b : tempMsk) { if (b != 0x00) { allZero = false; break; } }
                    if (allZero) bSkipMaskUpdate = true;
                }
            }
        }

        for (size_t i = 0; i < width * height; ++i) {
            if (image[i * 4 + 3] < 128) {
                mskData[i] = 0xFF;
                hasTransparency = true;
            }
        }

        // Read existing metadata & EXTRA DATA
        uint16_t preservePosX = 0;
        uint16_t preservePosY = 0;
        bool bZeroSizeX = false;
        bool bZeroSizeY = false;
        std::vector<uint8_t> extraData; // Buffer for data beyond the image

        if (!isFont && fs::exists(grpPath)) {
            std::ifstream oldGrp(grpPath, std::ios::binary);
            if (oldGrp.is_open()) {
                // Get total file size first
                oldGrp.seekg(0, std::ios::end);
                size_t fileSize = oldGrp.tellg();
                oldGrp.seekg(0, std::ios::beg);

                GRPHeader oldHead;
                if (oldGrp.read(reinterpret_cast<char*>(&oldHead), sizeof(oldHead))) {
                    preservePosX = oldHead.posX;
                    preservePosY = oldHead.posY;
                    if (oldHead.sizeX == 0) bZeroSizeX = true;
                    if (oldHead.sizeY == 0) bZeroSizeY = true;

                    // Calculate if there is extra data
                    // Only perform this if dimensions are valid (not 0)
                    if (oldHead.sizeX > 0 && oldHead.sizeY > 0) {
                        size_t headerSize = sizeof(GRPHeader);
                        size_t imageSize = (size_t)oldHead.sizeX * oldHead.sizeY;
                        size_t expectedEnd = headerSize + imageSize;

                        if (fileSize > expectedEnd) {
                            size_t extraSize = fileSize - expectedEnd;
                            extraData.resize(extraSize);

                            // Jump to the end of image data
                            oldGrp.seekg(expectedEnd, std::ios::beg);
                            oldGrp.read(reinterpret_cast<char*>(extraData.data()), extraSize);

                            std::wcout << L"[Info] Extra data preserved: " << extraSize << L" bytes." << std::endl;
                        }
                    }
                }
            }
        }

        std::ofstream grpFile(grpPath, std::ios::binary);

        if (isFont) {
            FontGRPHeader header;
            header.sizeX = (uint16_t)width;
            header.sizeY = (uint16_t)height;
            grpFile.write(reinterpret_cast<char*>(&header), sizeof(header));

            std::vector<uint8_t> indices(width * height);
            for (size_t i = 0; i < width * height; ++i) {
                uint8_t r = image[i * 4 + 0];
                indices[i] = (r < 128) ? 0x1A : 0x00;
            }
            grpFile.write(reinterpret_cast<char*>(indices.data()), indices.size());
        }
        else {
            std::vector<uint8_t> indices(width * height);

            if (bPreservePalette) {
                if (!fs::exists(c16Path)) {
                    std::cerr << "Error: C16 file not found: " << baseName.c_str() << std::endl;
                    return;
                }
                std::vector<ColorIndex> currentPalette = ReadPalette(c16Path);
                for (size_t i = 0; i < width * height; ++i) {
                    uint8_t r = image[i * 4 + 0];
                    uint8_t g = image[i * 4 + 1];
                    uint8_t b = image[i * 4 + 2];
                    uint8_t a = image[i * 4 + 3];
                    if (a < 128) indices[i] = 0;
                    else indices[i] = FindClosestIndex(currentPalette, r, g, b);
                }
            }
            else {
                liq_attr* attr = liq_attr_create();
                const int MAX_COLORS = 239;
                liq_set_max_colors(attr, MAX_COLORS);
                liq_image* liqImg = liq_image_create_rgba(attr, image.data(), width, height, 0);
                liq_result* res = nullptr;

                res = liq_quantize_image(attr, liqImg);
                if (res == NULL) {
                    std::cerr << "Quantization failed." << std::endl;
                    return;
                }
                const liq_palette* pal = liq_get_palette(res);

                std::ofstream c16File(c16Path, std::ios::binary);
                C16Header c16Head;
                c16Head.uCount = (uint16_t)pal->count;
                c16File.write(reinterpret_cast<char*>(&c16Head), sizeof(c16Head));
                std::vector<ColorIndex> colorTable(pal->count);
                for (int i = 0; i < pal->count; i++) {
                    colorTable[i].IndexNum = (uint8_t)(i + 1);
                    colorTable[i].Red = pal->entries[i].r;
                    colorTable[i].Green = pal->entries[i].g;
                    colorTable[i].Blue = pal->entries[i].b;
                }
                c16File.write(reinterpret_cast<char*>(colorTable.data()), colorTable.size() * sizeof(ColorIndex));
                c16File.close();

                liq_write_remapped_image(res, liqImg, indices.data(), indices.size());
                for (size_t i = 0; i < width * height; ++i) {
                    if (image[i * 4 + 3] < 128) indices[i] = 0;
                    else indices[i] += 1;
                }
                liq_result_destroy(res);
                liq_image_destroy(liqImg);
                liq_attr_destroy(attr);
            }

            // Write GRP
            GRPHeader grpHead;
            grpHead.posX = preservePosX;
            grpHead.posY = preservePosY;
            grpHead.sizeX = bZeroSizeX ? 0 : (uint16_t)width;
            grpHead.sizeY = bZeroSizeY ? 0 : (uint16_t)height;

            grpFile.write(reinterpret_cast<char*>(&grpHead), sizeof(grpHead));
            grpFile.write(reinterpret_cast<char*>(indices.data()), indices.size());

            // Append Extra Data (Footer)
            if (!extraData.empty()) {
                grpFile.write(reinterpret_cast<char*>(extraData.data()), extraData.size());
            }
        }

        grpFile.close();

        if (hasTransparency && !bSkipMaskUpdate) {
            std::ofstream mskFile(mskPath, std::ios::binary);
            mskFile.write(reinterpret_cast<char*>(mskData.data()), mskData.size());
        }

        std::wcout << L"Updated: " << baseName << std::endl;
    }

    bool Update(std::wstring lpTargetFolder, bool bPreservePalette) {
        fs::path root = lpTargetFolder;
        fs::path pngFolder = root / L"png";

        if (!fs::exists(pngFolder)) return false;

        for (const auto& entry : fs::directory_iterator(pngFolder)) {
            if (entry.path().extension() == L".png") {
                ProcessSingleUpdate(entry.path(), root, bPreservePalette);
            }
        }
        return true;
    }
}
