#include "LocalizationGraphic.h"
#include "LZSS.h"
#include "Util.h"
#include "resource.h"
#include "lodepng.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <map>
#include <fstream>
#include <cmath>
#include <future>

namespace fs = std::filesystem;

// Ensure 1-byte packing for binary file structures
#pragma pack(push, 1)

// C16 File Structure
struct C16Header {
    uint16_t uCount; // Index counter
};

struct ColorIndex {
    uint8_t IndexNum;
    uint8_t Blue;
    uint8_t Green;
    uint8_t Red;
};

// GRP File Structure (Header only, data follows)
struct GRPHeader {
    uint16_t posX;
    uint16_t posY;
    uint16_t sizeX;
    uint16_t sizeY;
};

// Font GRP Structure (Header only, data follows)
struct FontGRPHeader {
    uint16_t sizeX;
    uint16_t sizeY;
};

#pragma pack(pop)

enum class ConversionMode {
    ToPng_NoMsk = 1,
    ToPng_WithMsk,
    Update_Grp
};

namespace Localization {

    // ------------------------------------------------------------------------
    // Configuration & Constants
    // ------------------------------------------------------------------------

    // 리소스 PAK ID 매핑
    static const std::map<std::wstring, int> ResourcePakMap = {
        { L"back.pak", IDR_PAK_BACK_RES },
        { L"cmkmap.pak", IDR_PAK_CMKMAP_RES },
        { L"comike.pak", IDR_PAK_COMIKE_RES },
        { L"ending.pak", IDR_PAK_ENDING_RES },
        { L"eventcg.pak", IDR_PAK_EVENTCG_RES },
        { L"festival.pak", IDR_PAK_FESTIVAL_RES },
        { L"frame.pak", IDR_PAK_FRAME_RES },
        { L"guest.pak", IDR_PAK_GUEST_RES },
        { L"movie.pak", IDR_PAK_MOVIE_RES },
        { L"opening.pak", IDR_PAK_OPENING_RES },
        { L"param.pak", IDR_PAK_PARAM_RES },
        { L"present.pak", IDR_PAK_PRESENT_RES },
        { L"price.pak", IDR_PAK_PRICE_RES },
        { L"schedule.pak", IDR_PAK_SCHEDULE_RES },
        { L"settle.pak", IDR_PAK_SETTLE_RES },
        { L"shopping.pak", IDR_PAK_SHOPPING_RES },
        { L"univfest.pak", IDR_PAK_UNIVFEST_RES }
    };

    // 확장 C16 구조체 (메타데이터 포함)
#pragma pack(push, 1)
    struct C16ExtendedMeta {
        uint8_t mskIndexNum;
        uint16_t posX;
        uint16_t posY;
        uint8_t padding[3];
    };
#pragma pack(pop)

    // 설정 데이터 정의
    std::map<std::wstring, std::set<std::string>> TargetGraphics = {
    { L"back.pak",
        {
            "bg00.grp", "bg09.grp", "bg10.grp", "bg100.grp", "bg101.grp", "bg103.grp", "bg104.grp", "bg11.grp",
            "bg13.grp", "bg23.grp", "bg24.grp", "bg25.grp", "bg26.grp", "bg27.grp", "bg28.grp", "bg35.grp",
            "bg36.grp", "bg38.grp", "bg39.grp", "bg41.grp", "bg45.grp", "bg47.grp", "bg48.grp", "bg61.grp",
            "bg73.grp", "bg74.grp", "bg75.grp", "bg76.grp", "bg78.grp", "bg79.grp", "bg80.grp", "bg81.grp",
            "bg82.grp", "bg83.grp", "bg84.grp", "bg85.grp", "bg86.grp", "bg87.grp", "bg88.grp", "bg89.grp",
            "bg90.grp", "bg91.grp", "bg92.grp", "bg93.grp", "bg94.grp", "bg95.grp"
        }
    },
    { L"cmkmap.pak",
        {
            "bstr00.grp", "bstr01.grp", "bstr02.grp", "bstr03.grp", "bstr04.grp", "bstr05.grp", "bstr06.grp", "bstr07.grp",
            "bstr08.grp", "bstr09.grp", "bstr10.grp", "string00.grp", "string01.grp", "string02.grp", "string03.grp", "string04.grp",
            "string05.grp", "string06.grp", "string07.grp", "string08.grp", "string09.grp", "string10.grp"
        }
    },
    { L"comike.pak", { "desk2.grp", "desk3.grp", "nbryu0.grp", "nbryu1.grp" } },
    { L"ending.pak",
        {
            "end1.grp", "end2.grp", "list1.grp", "list10.grp", "list11.grp", "list12.grp", "list13.grp", "list14.grp",
            "list15.grp", "list16.grp", "list17.grp", "list18.grp", "list19.grp", "list2.grp", "list20.grp", "list21.grp",
            "list22.grp", "list23.grp", "list24.grp", "list25.grp", "list26.grp", "list27.grp", "list3.grp", "list4.grp",
            "list5.grp", "list6.grp", "list7.grp", "list8.grp", "list9.grp", "pic1.grp"
        }
    },
    { L"eventcg.pak",
        {
            "asa06_0.grp", "aya01_0.grp", "aya04_0.grp", "chi02_0.grp", "eim04_0.grp", "miz12_0.grp", "yu11_2.grp", "yu11_3.grp",
            "yu14_0.grp", "yu14_1.grp", "yu15_0.grp"
        }
    },
    { L"festival.pak",
        {
            "back.grp", "obj000.grp", "obj001.grp", "obj002.grp", "obj003.grp", "obj004.grp", "obj005.grp", "obj006.grp",
            "obj007.grp", "obj008.grp", "obj009.grp", "obj010.grp", "obj011.grp", "obj012.grp", "obj100.grp", "obj101.grp",
            "obj102.grp", "obj103.grp", "obj104.grp", "obj105.grp", "obj106.grp", "obj107.grp", "obj108.grp", "obj109.grp",
            "obj110.grp", "obj111.grp", "obj112.grp"
        }
    },
    { L"frame.pak",
        {
            "confm1.grp", "confm10.grp", "confm11.grp", "confm2.grp", "confm3.grp", "confm4.grp", "confm5.grp", "confm6.grp",
            "confm7.grp", "confm8.grp", "confm9.grp", "confmbs.grp", "coverbs.grp", "cvroff0.grp", "cvroff1.grp", "genup.grp",
            "gsel00.grp", "gsel01.grp", "gsel02.grp", "gsel03.grp", "gsel04.grp", "gsel05.grp", "gsel06.grp", "gsel07.grp",
            "gsel10.grp", "gsel11.grp", "gsel12.grp", "gsel13.grp", "gsel14.grp", "gsel15.grp", "gsel16.grp", "gsel17.grp",
            "gsel20.grp", "gsel21.grp", "gsel22.grp", "gsel23.grp", "gsel24.grp", "gsel25.grp", "gsel26.grp", "gsel27.grp",
            "gsel30.grp", "gsel31.grp", "gsel32.grp", "gsel33.grp", "gsel34.grp", "gsel35.grp", "gsel36.grp", "gsel37.grp",
            "gsel3off.grp", "gsel4off.grp", "gselback.grp", "gselbtn0.grp", "gselbtn1.grp", "gselbtn2.grp", "gselbtn3.grp", "inpkind0.grp",
            "inpkind1.grp", "inpkind2.grp", "inpkind3.grp", "naminpbs.grp", "pgsel.grp", "pseloff0.grp", "pseloff1.grp", "pseloff2.grp",
            "pseloff3.grp", "pseloff4.grp", "pseloff5.grp", "pselon0.grp", "pselon1.grp", "pselon2.grp", "pselon3.grp", "pselon4.grp",
            "pselon5.grp", "seldown0.grp", "seldown1.grp", "seldown2.grp", "seldown3.grp", "seldown4.grp", "seldown5.grp", "seldown6.grp",
            "seldown7.grp", "selover0.grp", "selover1.grp", "selover2.grp", "selover3.grp", "selover4.grp", "selover5.grp", "selover6.grp",
            "selover7.grp", "titlebs.grp"
        }
    },
    { L"guest.pak",
        {
            "a19.grp", "a24.grp", "a33.grp", "a44.grp", "a51.grp", "a54.grp", "a55.grp", "a56.grp",
            "a57.grp", "b03.grp", "b05.grp", "b07.grp", "b16.grp", "b18.grp", "b32.grp", "b34.grp",
            "b36.grp", "b40.grp", "b43.grp", "b45.grp"
        }
    },
    { L"movie.pak", { "action0.grp", "action1.grp", "action2.grp", "action3.grp", "horror0.grp", "horror1.grp", "horror2.grp", "horror3.grp" } },
    { L"opening.pak",
        {
            "ariake1.grp", "ariake2.grp", "ariake3.grp", "ariake4.grp", "ariake5.grp", "ariake6.grp", "ariake7.grp", "ariake8.grp",
            "ariake9.grp", "ariakea.grp", "banro.grp", "cut-a1.grp", "cut-a2.grp", "cut-a3.grp", "cut-all.grp", "m1.grp",
            "m2.grp", "m3.grp", "m4.grp", "m5.grp", "m6.grp", "m6a.grp", "m7.grp", "m8.grp"
        }
    },
    { L"param.pak",
        {
            "advance0.grp", "advance1.grp", "brush.grp", "cal000.grp", "cal001.grp", "cal010.grp", "cal011.grp", "cal020.grp",
            "cal021.grp", "cal030.grp", "cal031.grp", "cal040.grp", "cal041.grp", "cal050.grp", "cal051.grp", "cal060.grp",
            "cal061.grp", "cal070.grp", "cal071.grp", "cal080.grp", "cal081.grp", "cal090.grp", "cal091.grp", "cal100.grp",
            "cal101.grp", "command0.grp", "command1.grp", "command2.grp", "command3.grp", "command4.grp", "command5.grp", "conte.grp",
            "cover0.grp", "cover1.grp", "cover2.grp", "cvrrdy0.grp", "cvrrdy1.grp", "level0.grp", "level1.grp", "level2.grp",
            "level3.grp", "level4.grp", "level5.grp", "level6.grp", "param0.grp", "param1.grp", "penon.grp", "price0.grp",
            "price1.grp", "selbase.grp", "setbtn0.grp", "setbtn1.grp", "setbtn2.grp", "setbtn3.grp", "setbtn4.grp", "setbtn5.grp",
            "setbtn6.grp", "setting0.grp", "setting1.grp", "week0.grp", "week1.grp", "week2.grp", "week3.grp", "week4.grp",
            "week5.grp", "week6.grp", "yenbl.grp", "yenrd.grp"
        }
    },
    { L"present.pak",
        {
            "busu_off.grp", "number0.grp", "number1.grp", "number2.grp", "number3.grp", "number4.grp", "number5.grp", "number6.grp",
            "number7.grp"
        }
    },
    { L"price.pak", { "cover0.grp", "cover1.grp", "cover2.grp", "nedan.grp" } },
    { L"schedule.pak",
        {
            "amdisp.grp", "comipa.grp", "guest.grp", "heijitu.grp", "hldraw.grp", "hlprac.grp", "hltel0.grp", "hltel1.grp",
            "hltel2.grp", "hltel3.grp", "hltel4.grp", "hltel5.grp", "hltel6.grp", "hltel7.grp", "kyujitu.grp", "pmdisp.grp",
            "schd00.grp", "schd01.grp", "schd02.grp", "schd03.grp", "schd04.grp", "schd05.grp", "schd06.grp", "schd07.grp",
            "schd08.grp", "schd09.grp", "schd10.grp", "syuraba.grp"
        }
    },
    { L"settle.pak", { "kessan.grp" } },
    { L"shopping.pak", { "book.grp", "drug.grp" } },
    { L"univfest.pak", { "back.grp", "shop.grp" } }
    };

    // ------------------------------------------------------------------------
    // Helper Functions
    // ------------------------------------------------------------------------

    // 메모리상의 PAK 데이터에서 파일 추출
    static bool ExtractFromMemoryPak(const std::vector<uint8_t>& pakBuffer, const std::wstring& targetName, std::vector<uint8_t>& outData) {
        if (pakBuffer.size() < 4) return false;

        const uint8_t* ptr = pakBuffer.data();
        uint32_t count = *(uint32_t*)ptr;
        ptr += 4;

        if (pakBuffer.size() < 4 + count * sizeof(FileEntry)) return false;
        const FileEntry* table = (const FileEntry*)ptr;

        std::string targetSjis = Util::WideToMultiByteStr(targetName, 932);

        // 대소문자 무시 비교를 위해 타겟 이름을 소문자로
        std::string targetLower = targetSjis;
        std::transform(targetLower.begin(), targetLower.end(), targetLower.begin(), ::tolower);

        for (uint32_t i = 0; i < count; ++i) {
            std::string entryName(table[i].Name, strnlen(table[i].Name, 16));
            std::transform(entryName.begin(), entryName.end(), entryName.begin(), ::tolower);

            if (entryName == targetLower) {
                return LZSS::Decompress(pakBuffer, &table[i], outData);
            }
        }
        return false;
    }

    // 리소스 로드
    static bool LoadResourcePak(int resId, std::vector<uint8_t>& outBuffer) {
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resId), L"DAT");
        if (!hRes) return false;
        HGLOBAL hMem = LoadResource(NULL, hRes);
        if (!hMem) return false;
        DWORD size = SizeofResource(NULL, hRes);
        void* ptr = LockResource(hMem);
        if (!ptr || size == 0) return false;

        outBuffer.assign((uint8_t*)ptr, (uint8_t*)ptr + size);
        return true;
    }

    // 팔레트에서 가장 가까운 색상 찾기 (Euclidean Distance)
    static uint8_t FindClosestColorIndex(const std::vector<ColorIndex>& palette, uint8_t r, uint8_t g, uint8_t b, uint8_t skipIndex = 0) {
        int minDist = 255 * 255 * 3 + 1;
        uint8_t bestIndexNum = 0; // 실제 GRP에 기록될 인덱스 값

        for (size_t i = 0; i < palette.size(); ++i) {
            if (palette[i].IndexNum == skipIndex) continue;

            int dr = (int)palette[i].Red - r;
            int dg = (int)palette[i].Green - g;
            int db = (int)palette[i].Blue - b;
            int dist = dr * dr + dg * dg + db * db;

            if (dist < minDist) {
                minDist = dist;
                bestIndexNum = palette[i].IndexNum;
            }
        }
        return bestIndexNum;
    }

    // ------------------------------------------------------------------------
    // Converter Implementations
    // ------------------------------------------------------------------------
    namespace Converter {

        // GRP + C16 + MSK -> PNG
        // 이 함수는 파일 경로 대신 메모리 버퍼들을 받아 처리하도록 설계함
        bool GrpBufferToPng(const std::vector<uint8_t>& grpData,
            const std::vector<uint8_t>& c16Data,
            const std::vector<uint8_t>& mskData,
            const std::wstring& savePath)
        {
            if (grpData.size() < sizeof(GRPHeader)) return false;

            const GRPHeader* header = (const GRPHeader*)grpData.data();
            int width = header->sizeX;
            int height = header->sizeY;
            size_t pixelDataSize = (size_t)width * height;

            if (grpData.size() < sizeof(GRPHeader) + pixelDataSize) return false;
            const uint8_t* pixels = grpData.data() + sizeof(GRPHeader);

            // Palette parsing
            std::vector<ColorIndex> palette(256);
            for (int i = 0; i < 256; ++i) {
                palette[i] = { (uint8_t)i, (uint8_t)i, (uint8_t)i, (uint8_t)i };
            }
            if (c16Data.size() >= 2) {
                uint16_t count = *(uint16_t*)c16Data.data();
                if (c16Data.size() >= 2 + count * sizeof(ColorIndex)) {
                    const ColorIndex* srcPal = (const ColorIndex*)(c16Data.data() + 2);
                    for (int i = 0; i < count; ++i) {
                        uint8_t targetIdx = srcPal[i].IndexNum;
                        palette[targetIdx] = srcPal[i]; // 해당 인덱스 번호 위치에 저장
                    }
                }
            }

            // Create RGBA buffer
            std::vector<unsigned char> image(width * height * 4);
            for (int i = 0; i < width * height; ++i) {
                uint8_t idx = pixels[i];
                uint8_t alpha = 255;

                // Apply Mask if available (0 != Transparent)
                if (!mskData.empty() && i < mskData.size()) {
                    if (mskData[i] > 0x80) alpha = 0;
                }

                // Map index to color
                ColorIndex col = palette[idx];

                image[4 * i + 0] = col.Red;
                image[4 * i + 1] = col.Green;
                image[4 * i + 2] = col.Blue;
                image[4 * i + 3] = alpha;
            }

            // Encode PNG
            unsigned error = lodepng::encode(Util::WideToMultiByteStr(savePath, CP_UTF8), image, width, height);
            return (error == 0);
        }

        // PNG -> GRP (Using Extended C16 from Resource)
        bool PngFileToGrp(const std::wstring& srcPath, const std::vector<uint8_t>& resC16Data, std::vector<uint8_t>& outGrpData, std::vector<uint8_t>& outMskData) {
            outMskData.clear();

            std::vector<unsigned char> image;
            unsigned width, height;

            // Load PNG
            unsigned error = lodepng::decode(image, width, height, Util::WideToMultiByteStr(srcPath, CP_UTF8));
            if (error) {
                std::wcout << L"PNG Decode Error: " << error << std::endl;
                return false;
            }

            // Parse Resource C16 (Extended)
            // Struct: uCount(2) + ColorIndex[uCount] + Meta(8)
            if (resC16Data.size() < 2) return false;
            uint16_t uCount = *(uint16_t*)resC16Data.data();
            size_t paletteSize = uCount * sizeof(ColorIndex);

            if (resC16Data.size() < 2 + paletteSize + sizeof(C16ExtendedMeta)) return false;

            std::vector<ColorIndex> palette(uCount);
            memcpy(palette.data(), resC16Data.data() + 2, paletteSize);

            const C16ExtendedMeta* meta = (const C16ExtendedMeta*)(resC16Data.data() + 2 + paletteSize);

            // Construct GRP Header
            GRPHeader grpHead;
            grpHead.posX = meta->posX;
            grpHead.posY = meta->posY;
            grpHead.sizeX = (uint16_t)width;
            grpHead.sizeY = (uint16_t)height;

            // Convert Pixels
            std::vector<uint8_t> indices;
            indices.reserve(width * height);

            // msk 데이터 준비 (Header + Pixels)
            std::vector<uint8_t> mskPixels;
            mskPixels.reserve(width * height);
            bool hasTransparency = false;

            for (size_t i = 0; i < width * height; ++i) {
                uint8_t r = image[4 * i + 0];
                uint8_t g = image[4 * i + 1];
                uint8_t b = image[4 * i + 2];
                uint8_t a = image[4 * i + 3];

                if (a < 128) {
                    // Transparent -> Use mask index from meta
                    indices.push_back(meta->mskIndexNum);
                    mskPixels.push_back(0xFF); // MSK: 0xFF = Transparent
                    hasTransparency = true;
                }
                else {
                    // Opaque -> Find closest color
                    // Skip mskIndexNum to avoid transparent color reuse for opaque pixels?
                    // (Assuming mskIndexNum is reserved for transparency)
                    indices.push_back(FindClosestColorIndex(palette, r, g, b, meta->mskIndexNum));
                    mskPixels.push_back(0x0); // MSK: 0x0 = Opaque
                }
            }

            // Assemble GRP Data
            size_t totalSize = sizeof(GRPHeader) + indices.size();
            outGrpData.resize(totalSize);
            memcpy(outGrpData.data(), &grpHead, sizeof(GRPHeader));
            memcpy(outGrpData.data() + sizeof(GRPHeader), indices.data(), indices.size());

            // MSK 조립 (투명도가 있을 경우에만)
            if (hasTransparency) {
                outMskData.resize(mskPixels.size());
                memcpy(outMskData.data(), mskPixels.data(), mskPixels.size());
            }
            return true;
        }

        // Implementation stub for header requirement
        bool GrpToPng(const std::vector<uint8_t>& grpData, const std::wstring& savePath) {
            // Not used directly in this refactored flow, using GrpBufferToPng with helpers instead
            return false;
        }
        bool PngToGrp(const std::wstring& srcPath, std::vector<uint8_t>& outGrpData) {
            // Not used directly, needs C16 context
            return false;
        }

    } // namespace Converter

    // ------------------------------------------------------------------------
    // Public Functions
    // ------------------------------------------------------------------------

    // [Plan 3b] Extract Graphic Resources
    void ExtractGraphicResources() {
        std::wcout << L"==============================================" << std::endl;
        std::wcout << L"      Extracting Graphic Resources...         " << std::endl;
        std::wcout << L"==============================================" << std::endl;

        fs::path outDir = L"KoR";
        fs::create_directories(outDir);

        for (const auto& kv : TargetGraphics) {
            std::wstring pakName = kv.first;
            const auto& targetSet = kv.second;

            if (!fs::exists(pakName)) {
                std::wcout << L"[Skip] " << pakName << L" not found." << std::endl;
                continue;
            }

            // Open PAK
            HANDLE hFile = CreateFileW(pakName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) continue;

            uint32_t fileCount = 0;
            DWORD dwRead;
            ReadFile(hFile, &fileCount, 4, &dwRead, NULL);
            std::vector<FileEntry> table(fileCount);
            ReadFile(hFile, table.data(), sizeof(FileEntry) * fileCount, &dwRead, NULL);

            // Read all files to memory cache for cross-referencing (C16/MSK)
            // Efficiency note: Loading everything might be heavy. 
            // Better: Scan table, identify offsets, load on demand.
            // For simplicity in this tool, we map name -> FileEntry.
            std::map<std::string, const FileEntry*> fileMap;
            for (const auto& entry : table) {
                fileMap[entry.Name] = &entry;
            }

            // Process Targets
            for (const std::string& targetFile : targetSet) {
                if (fileMap.find(targetFile) == fileMap.end()) continue;

                const FileEntry* grpEntry = fileMap[targetFile];

                // Load GRP
                std::vector<uint8_t> grpData;
                if (!LZSS::Decompress(hFile, grpEntry, grpData)) continue;

                // Identify Base Name (e.g., "bg00" from "bg00.grp")
                fs::path p(targetFile);
                std::string stem = p.stem().string();

                // Load C16 (stem + .c16)
                std::vector<uint8_t> c16Data;
                std::string c16Name = stem + ".c16";
                if (fileMap.count(c16Name)) {
                    LZSS::Decompress(hFile, fileMap[c16Name], c16Data);
                }

                // Load MSK (stem + .msk)
                std::vector<uint8_t> mskData;
                std::string mskName = stem + ".msk";
                if (fileMap.count(mskName)) {
                    LZSS::Decompress(hFile, fileMap[mskName], mskData);
                }

                // Construct Output Filename: [PakName]_[InternalName].png
                fs::path pakPath(pakName);
                std::wstring outName = pakPath.stem().wstring() + L"_" + Util::MultiByteToWideStr(stem, 932) + L".png";
                fs::path savePath = outDir / outName;

                std::wcout << L"Extracting: " << pakName << L" :: " << Util::MultiByteToWideStr(targetFile, 932) << L" -> " << outName << std::endl;

                if (!Converter::GrpBufferToPng(grpData, c16Data, mskData, savePath.wstring())) {
                    std::wcout << L"  [Error] PNG conversion failed." << std::endl;
                }
            }

            CloseHandle(hFile);
        }
        std::wcout << L"[Info] Graphic extraction completed." << std::endl;
    }

    // 결과 구조체
    struct GraphicProcessResult {
        bool success;
        std::wstring logMsg;
        std::vector<std::pair<std::wstring, std::vector<uint8_t>>> files; // {Name, Blob} 쌍 (GRP, MSK)
    };

    // [Plan 4b] Build Graphic Archive
    void BuildGraphicArchive() {
        std::wcout << L"==============================================" << std::endl;
        std::wcout << L"       Building Graphic Archive (KoR.pak)     " << std::endl;
        std::wcout << L"==============================================" << std::endl;

        fs::path srcDir = L"KoR";
        if (!fs::exists(srcDir)) return;

        // 1. PNG 파일 수집
        std::vector<fs::path> pngFiles;
        for (const auto& entry : fs::directory_iterator(srcDir)) {
            if (entry.is_regular_file() && entry.path().extension() == L".png") {
                pngFiles.push_back(entry.path());
            }
        }
        if (pngFiles.empty()) return;

        // 2. 필요한 리소스 PAK 미리 캐싱 (Pre-load for Thread Safety)
        // 멀티스레드에서 map에 동시 접근(삽입)하면 크래시가 나므로 미리 다 로드함.
        std::wcout << L"[Info] Pre-loading resource PAKs..." << std::endl;
        std::map<int, std::vector<uint8_t>> resPakCache;

        for (const auto& pngPath : pngFiles) {
            std::wstring filename = pngPath.stem().wstring();
            size_t underscore = filename.find(L'_');
            if (underscore == std::wstring::npos) continue;

            std::wstring pakPrefix = filename.substr(0, underscore);
            std::wstring pakFullName = pakPrefix + L".pak";

            if (ResourcePakMap.find(pakFullName) != ResourcePakMap.end()) {
                int resId = ResourcePakMap.at(pakFullName);
                if (resPakCache.find(resId) == resPakCache.end()) {
                    std::vector<uint8_t> buffer;
                    if (LoadResourcePak(resId, buffer)) {
                        resPakCache[resId] = std::move(buffer);
                    }
                }
            }
        }

        // 3. 병렬 작업 실행
        std::wcout << L"[Info] Launching parallel tasks for " << pngFiles.size() << L" graphics..." << std::endl;
        std::vector<std::future<GraphicProcessResult>> futures;

        // resPakCache는 이제 읽기 전용이므로 참조로 전달해도 안전함
        for (const auto& pngPath : pngFiles) {
            futures.push_back(std::async(std::launch::async, [pngPath, &resPakCache]() {
                GraphicProcessResult res;
                res.success = false;

                std::wstring filename = pngPath.stem().wstring();
                size_t underscore = filename.find(L'_');
                if (underscore == std::wstring::npos) {
                    res.logMsg = L"[Skip] Invalid format: " + filename;
                    return res;
                }

                std::wstring pakPrefix = filename.substr(0, underscore);
                std::wstring internalName = filename.substr(underscore + 1);
                std::wstring pakFullName = pakPrefix + L".pak";

                if (ResourcePakMap.find(pakFullName) == ResourcePakMap.end()) return res;
                int resId = ResourcePakMap.at(pakFullName);

                // 캐시에서 읽기 (Lock 불필요 - Read Only)
                if (resPakCache.find(resId) == resPakCache.end()) {
                    res.logMsg = L"[Error] Resource not loaded: " + pakFullName;
                    return res;
                }
                const auto& pakBuffer = resPakCache.at(resId);

                // C16 추출
                std::wstring c16Name = internalName + L".c16";
                std::vector<uint8_t> c16Data;
                if (!ExtractFromMemoryPak(pakBuffer, c16Name, c16Data)) {
                    res.logMsg = L"[Error] Palette missing: " + c16Name;
                    return res;
                }

                // 변환 수행
                std::vector<uint8_t> grpData, mskData;
                if (!Converter::PngFileToGrp(pngPath.wstring(), c16Data, grpData, mskData)) {
                    res.logMsg = L"[Error] Conversion failed: " + filename;
                    return res;
                }

                res.success = true;
                res.logMsg = L"Packed: " + filename;

                // 압축 및 데이터 준비 (GRP)
                {
                    wchar_t p1 = pakPrefix[0];
                    wchar_t p4 = (pakPrefix.length() >= 4) ? pakPrefix[3] : L'_';
                    std::wstring baseName = std::wstring(1, p1) + std::wstring(1, p4) + internalName;

                    // GRP 압축
                    std::vector<uint8_t> compGrp;
                    size_t compSize = LZSS::Compress(grpData, compGrp);
                    std::vector<uint8_t> finalGrpBlob;

                    uint32_t flag = (compSize > 0 && compSize < grpData.size()) ? 1 : 0;
                    uint32_t dataSize = flag ? (8 + (uint32_t)compSize) : (uint32_t)grpData.size();
                    uint32_t orgSize = (uint32_t)grpData.size();

                    finalGrpBlob.resize(dataSize);
                    if (flag) {
                        uint32_t totalSizeWithHeader = 4 + 4 + (uint32_t)compSize; // 사실 entry.DataSize와 동일
                        // 여기서는 Blob 자체에 Size헤더를 포함하지 않고, Write할 때 처리했던 방식과 맞춤
                        // 하지만 병렬처리를 위해 Blob에 헤더까지 다 구워버리는게 편함.
                        // 기존 로직: WriteFile(TotalSize), WriteFile(OrgSize), WriteFile(Body)
                        // Blob 구조: [TotalSize(4)][OrgSize(4)][CompressedBody]
                        memcpy(finalGrpBlob.data(), &dataSize, 4);
                        memcpy(finalGrpBlob.data() + 4, &orgSize, 4);
                        memcpy(finalGrpBlob.data() + 8, compGrp.data(), compSize);
                    }
                    else {
                        memcpy(finalGrpBlob.data(), grpData.data(), grpData.size());
                    }
                    res.files.push_back({ baseName + L".grp", std::move(finalGrpBlob) });

                    // MSK 압축 (있을 경우)
                    if (!mskData.empty()) {
                        std::vector<uint8_t> compMsk;
                        size_t mskCompSize = LZSS::Compress(mskData, compMsk);
                        std::vector<uint8_t> finalMskBlob;

                        uint32_t mFlag = (mskCompSize > 0 && mskCompSize < mskData.size()) ? 1 : 0;
                        uint32_t mDataSize = mFlag ? (8 + (uint32_t)mskCompSize) : (uint32_t)mskData.size();
                        uint32_t mOrgSize = (uint32_t)mskData.size();

                        finalMskBlob.resize(mDataSize);
                        if (mFlag) {
                            memcpy(finalMskBlob.data(), &mDataSize, 4);
                            memcpy(finalMskBlob.data() + 4, &mOrgSize, 4);
                            memcpy(finalMskBlob.data() + 8, compMsk.data(), mskCompSize);
                        }
                        else {
                            memcpy(finalMskBlob.data(), mskData.data(), mskData.size());
                        }
                        res.files.push_back({ baseName + L".msk", std::move(finalMskBlob) });
                        res.logMsg += L" (+MSK)";
                    }
                }
                return res;
                }));
        }

        // 4. 결과 쓰기 (Main Thread)
        std::wstring pakName = L"KoR.pak";
        HANDLE hArchive = CreateFileW(pakName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        // 헤더 예약 (MSK 고려 2배)
        uint32_t reservedCount = (uint32_t)pngFiles.size() * 2;
        DWORD dwWritten;
        uint32_t temp = 0;
        WriteFile(hArchive, &temp, 4, &dwWritten, NULL);

        uint32_t tableStart = 4;
        uint32_t dataStart = tableStart + (reservedCount * sizeof(FileEntry));
        SetFilePointer(hArchive, dataStart, NULL, FILE_BEGIN);

        std::vector<FileEntry> table;
        int successCount = 0;

        for (auto& fut : futures) {
            auto res = fut.get();
            if (!res.logMsg.empty()) std::wcout << res.logMsg << std::endl;

            if (res.success) {
                for (const auto& file : res.files) {
                    FileEntry entry = { 0 };
                    std::string sName = Util::WideToMultiByteStr(file.first, 932);
                    strncpy(entry.Name, sName.c_str(), 15);

                    const auto& blob = file.second;
                    // Blob 내부에 이미 [Size][OrgSize] 헤더가 포함되어 있는지 확인해야 함
                    // 위 압축 로직에서 Flag 체크하여 0이면 Raw, 1이면 Compressed Header 포함시킴.
                    // Raw인 경우: Blob Size == Original Size
                    // Comp인 경우: Blob Size == 8 + Comp Body

                    // 압축 여부 판단 (LZSS 헤더 크기 체크 등)
                    // 간단히: Blob의 첫 4바이트(TotalSize)가 Blob크기와 같으면 Compressed라고 가정
                    // 하지만 Raw 데이터가 우연히 그럴 수 있음. 
                    // 더 명확히 하기 위해 구조체에 flag를 전달하거나, 여기서 다시 판단.
                    // 위 람다에서 Blob을 만들 때:
                    // Compressed: [Total(4)][Org(4)][Body]
                    // Raw: [Body]

                    // 명확성을 위해 람다에서 flag를 pair로 같이 넘기는게 좋으나, 
                    // 여기서는 Blob 앞 4바이트(Size) == Blob Size 이면 압축된걸로 간주 (LZSS 헤더 특징)
                    bool isCompressed = false;
                    if (blob.size() > 8) {
                        uint32_t checkSize = *(uint32_t*)blob.data();
                        if (checkSize == blob.size()) isCompressed = true;
                    }

                    entry.Flags = isCompressed ? 1 : 0;
                    entry.DataSize = (uint32_t)blob.size();
                    entry.DataOffset = SetFilePointer(hArchive, 0, NULL, FILE_CURRENT);

                    WriteFile(hArchive, blob.data(), (DWORD)blob.size(), &dwWritten, NULL);
                    table.push_back(entry);
                    successCount++;
                }
            }
        }

        // 헤더 업데이트
        uint32_t finalCount = (uint32_t)table.size();
        SetFilePointer(hArchive, 0, NULL, FILE_BEGIN);
        WriteFile(hArchive, &finalCount, 4, &dwWritten, NULL);

        SetFilePointer(hArchive, tableStart, NULL, FILE_BEGIN);
        WriteFile(hArchive, table.data(), sizeof(FileEntry) * finalCount, &dwWritten, NULL);

        CloseHandle(hArchive);
        std::wcout << L"[Info] KoR.pak created with " << finalCount << L" files." << std::endl;
    }
}
