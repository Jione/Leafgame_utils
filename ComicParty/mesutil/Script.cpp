#define NOMINMAX
#include "Script.h"
#include "EventParse.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <OpenXLSX.hpp>

#ifdef _AMD64_
#ifdef _DEBUG
#pragma comment(lib,"OpenXLSX-x64-dbg.lib")
#else
#pragma comment(lib,"OpenXLSX-x64-rel.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib,"OpenXLSX-x86-dbg.lib")
#else
#pragma comment(lib,"OpenXLSX-x86-rel.lib")
#endif
#endif

using namespace OpenXLSX;
namespace fs = std::filesystem;

namespace Script {

    struct MesHeader {
        uint32_t uCount;
        std::vector<uint32_t> scrPos;
        std::vector<uint32_t> scrLen;
        std::vector<uint32_t> vLen;
    };

    bool ParseMes(LPWSTR lpTargetFile) {
        std::wstring filePath = lpTargetFile;
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::wcout << L"Failed to open: " << filePath << std::endl;
            return false;
        }

        // 1. Read Structure
        MesHeader header;
        file.read(reinterpret_cast<char*>(&header.uCount), sizeof(uint32_t));

        header.scrPos.resize(header.uCount);
        header.scrLen.resize(header.uCount);
        header.vLen.resize(header.uCount);

        file.read(reinterpret_cast<char*>(header.scrPos.data()), header.uCount * sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(header.scrLen.data()), header.uCount * sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(header.vLen.data()), header.uCount * sizeof(uint32_t));

        // Read Data Blob
        // Use stream to vector directly
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        size_t headerSize = sizeof(uint32_t) + (header.uCount * 3 * sizeof(uint32_t));
        size_t blobSize = (size_t)fileSize - headerSize;

        std::vector<char> stringBlob(blobSize);
        file.seekg(headerSize, std::ios::beg);
        file.read(stringBlob.data(), blobSize);
        file.close();

        std::cout << "Analyzing " << header.uCount << " entries..." << std::endl;

        std::vector<int> vCharId;
        vCharId.reserve(header.uCount);
        for (int i = 0; i < header.uCount; ++i) {
            vCharId[i] = -1;
        }
        Event::ParseCharaIDTable(lpTargetFile, vCharId);

        // 2. Create Excel
        std::wstring xlsxPath = filePath;
        xlsxPath.replace(xlsxPath.find(L".mes"), 4, L".xlsx");
        std::string xlsxPathStr = Util::WideToMultiByteStr(xlsxPath, CP_UTF8);

        XLDocument doc;
        doc.create(xlsxPathStr);
        auto wks = doc.workbook().worksheet("Sheet1");

        // Headers
        wks.cell("A1").value() = "Index";
        wks.cell("B1").value() = "Charactor";
        wks.cell("C1").value() = "Voice";
        wks.cell("D1").value() = "String";
        wks.cell("E1").value() = "Translate";

        // 3. Process Data
        for (uint32_t i = 0; i < header.uCount; ++i) {
            uint32_t sOffset = header.scrPos[i];
            uint32_t sLen = header.scrLen[i];

            // Voice follows string in blob logical structure, but blob is linear
            // The blob structure described is { scr1, v1, scr2, v2 ... }
            // so vOffset is sOffset + sLen
            uint32_t vOffset = sOffset + sLen;
            uint32_t vLen = header.vLen[i];

            if (sOffset >= stringBlob.size()) continue;

            // --- Extract Charactor ---
            std::string charStr = "";
            if (vCharId[i] == -2) {
                charStr = Util::MultiByteToUtf8("¼±ÅÃÁö", 949);
            }
            else if ((vCharId[i] >= 0) && (vCharId[i] < 0x38)) {
                charStr = Util::MultiByteToUtf8(CharaName[(vCharId[i])], 949);
            }

            // --- Extract Voice (ASCII) ---
            std::string voiceStr;
            if (vLen > 0 && vOffset + vLen <= stringBlob.size()) {
                // vLen includes null.
                voiceStr.assign(&stringBlob[vOffset], vLen - 1);
            }

            // --- Extract & Convert Main String ---
            // We use sLen - 1 to exclude the null terminator from processing
            uint32_t processLen = (sLen > 0) ? sLen - 1 : 0;
            std::string finalStr;
            if (processLen > 0 && sOffset + processLen <= stringBlob.size()) {
                finalStr = Util::MesBytesToUtf8(stringBlob, sOffset, processLen);
            }

            // Write Row
            int row = i + 2;
            wks.cell(row, 1).value() = (int)(i + 1);
            wks.cell(row, 2).value() = charStr;
            wks.cell(row, 3).value() = voiceStr;
            wks.cell(row, 4).value() = finalStr;
            wks.cell(row, 5).value() = "";
        }

        doc.save();
        std::wcout << L"Exported to: " << xlsxPath << std::endl;
        return true;
    }

    bool ApplyTransTo(LPWSTR lpTargetFile) {
        std::wstring filePath = lpTargetFile;
        std::string xlsxPathStr = Util::WideToMultiByteStr(filePath, CP_UTF8);

        if (!fs::exists(filePath)) {
            std::wcout << L"File not found: " << filePath << std::endl;
            return false;
        }

        XLDocument doc;
        try {
            doc.open(xlsxPathStr);
        }
        catch (...) {
            std::cerr << "Failed to open Excel file." << std::endl;
            return false;
        }

        auto wks = doc.workbook().worksheet("Sheet1");
        uint32_t rowCount = wks.rowCount();

        MesHeader header;
        std::vector<std::vector<char>> blobParts; // Stores {string_bytes, null, voice_bytes, null}

        // 1. Read Excel
        for (uint32_t row = 3; row <= rowCount; ++row) {
            // Stop on empty index
            if (wks.cell(row, 1).value().type() == XLValueType::Empty) break;

            // Read Voice
            std::string vStr;
            if (wks.cell(row, 3).value().type() != XLValueType::Empty) {
                vStr = wks.cell(row, 3).value().get<std::string>();
            }

            // Read String (Translate -> Original)
            std::string text;
            if (wks.cell(row, 5).value().type() != XLValueType::Empty) {
                text = wks.cell(row, 5).value().get<std::string>();
            }
            if (text.empty() && wks.cell(row, 4).value().type() != XLValueType::Empty) {
                text = wks.cell(row, 4).value().get<std::string>();
            }

            // Convert String (UTF-8 -> MES Bytes)
            std::vector<char> strBytes = Util::Utf8ToMesBytes(text);

            // Combine for this entry
            std::vector<char> entryBlob;

            // 1. String Bytes
            entryBlob.insert(entryBlob.end(), strBytes.begin(), strBytes.end());
            entryBlob.push_back('\0'); // String Null

            // 2. Voice Bytes (Simple Copy + Null)
            for (char c : vStr) entryBlob.push_back(c);
            entryBlob.push_back('\0'); // Voice Null

            blobParts.push_back(entryBlob);

            // Metadata
            header.scrLen.push_back((uint32_t)strBytes.size() + 1);
            header.vLen.push_back((uint32_t)vStr.size() + 1);
        }

        header.uCount = (uint32_t)blobParts.size();
        header.scrPos.resize(header.uCount);

        // 2. Flatten Blob & Calculate Offsets
        std::vector<char> finalBlob;
        uint32_t currentOffset = 0;

        for (size_t i = 0; i < header.uCount; ++i) {
            header.scrPos[i] = currentOffset;

            const auto& part = blobParts[i];
            finalBlob.insert(finalBlob.end(), part.begin(), part.end());

            currentOffset += (uint32_t)part.size();
        }

        // 3. Write MES File
        std::wstring mesPath = filePath;
        mesPath.replace(mesPath.find(L".xlsx"), 5, L".mes");

        std::ofstream file(mesPath, std::ios::binary);
        if (!file.is_open()) {
            std::wcout << L"Failed to save: " << mesPath << std::endl;
            return false;
        }

        file.write(reinterpret_cast<char*>(&header.uCount), sizeof(uint32_t));
        file.write(reinterpret_cast<char*>(header.scrPos.data()), header.uCount * sizeof(uint32_t));
        file.write(reinterpret_cast<char*>(header.scrLen.data()), header.uCount * sizeof(uint32_t));
        file.write(reinterpret_cast<char*>(header.vLen.data()), header.uCount * sizeof(uint32_t));
        file.write(finalBlob.data(), finalBlob.size());

        file.close();
        std::wcout << L"Imported to: " << mesPath << std::endl;
        return true;
    }
}
