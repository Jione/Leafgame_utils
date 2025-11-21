#define NOMINMAX
#include "Script.h"
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
        header.vLen.resize(header.uCount); // Resize vLen

        file.read(reinterpret_cast<char*>(header.scrPos.data()), header.uCount * sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(header.scrLen.data()), header.uCount * sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(header.vLen.data()), header.uCount * sizeof(uint32_t)); // Read vLen

        // Read Data Blob (Strings + Voice strings)
        std::vector<char> stringBlob((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        // 2. Encoding Detection
        Util::EncodingType currentEncoding = Util::EncodingType::ShiftJIS;

        // Scan for Half-width Katakana (0xA1 - 0xDF)
        bool tok = false;
        for (char c : stringBlob) {
            unsigned char uc = static_cast<unsigned char>(c);
            if (!tok) {
                if (uc >= 0xA1 && uc <= 0xDF) {
                    currentEncoding = Util::EncodingType::CustomUTF8;
                    break;
                }
                else if (0x80 <= uc) {
                    tok = true;
                }
            }
            else {
                tok = false;
            }
        }

        std::cout << "Detected Encoding: " << (currentEncoding == Util::EncodingType::CustomUTF8 ? "Custom UTF-8" : "Shift-JIS") << std::endl;

        // 3. Create Excel
        std::wstring xlsxPath = filePath;
        xlsxPath.replace(xlsxPath.find(L".mes"), 4, L".xlsx");
        std::string xlsxPathStr = Util::StringConvert(std::string(xlsxPath.begin(), xlsxPath.end()), Util::EncodingType::ACP, Util::EncodingType::UTF8);

        XLDocument doc;
        doc.create(xlsxPathStr);
        auto wks = doc.workbook().worksheet("Sheet1");

        // Headers
        wks.cell("A1").value() = "Index";
        wks.cell("B1").value() = "Voice";
        wks.cell("C1").value() = "String";
        wks.cell("D1").value() = "Translate";

        // 4. Process Data
        for (uint32_t i = 0; i < header.uCount; ++i) {
            // Calculate offsets
            // String offset is explicitly given
            uint32_t sOffset = header.scrPos[i];
            uint32_t sLen = header.scrLen[i];

            // Voice offset follows immediately after the string
            // Structure: { scr1[len], v1[len], ... }
            uint32_t vOffset = sOffset + sLen;
            uint32_t vLen = header.vLen[i];

            if (sOffset >= stringBlob.size()) continue;

            // --- Extract Main String ---
            std::string rawStr;
            if (sLen > 0 && sOffset + sLen <= stringBlob.size()) {
                rawStr.assign(&stringBlob[sOffset], sLen - 1); // Exclude null for cell
            }
            else {
                rawStr = "";
            }

            // --- Extract Voice String ---
            std::string voiceStr;
            if (vLen > 0 && vOffset + vLen <= stringBlob.size()) {
                // Voice string is just ASCII/Filename, usually no embedded nulls
                // Length includes null terminator, exclude it for display
                voiceStr.assign(&stringBlob[vOffset], vLen - 1);
            }
            else {
                voiceStr = "";
            }

            // Convert String to UTF-8
            std::string finalStr = Util::StringConvert(rawStr, currentEncoding, Util::EncodingType::UTF8);

            // --- Verification Logic (Shift-JIS) ---
            if (currentEncoding == Util::EncodingType::ShiftJIS) {
                std::string checkStr = Util::StringConvert(finalStr, Util::EncodingType::UTF8, Util::EncodingType::ShiftJIS);
                if (rawStr != checkStr) {
                    std::cerr << "[Warning] Conversion mismatch at Index " << (i + 1) << std::endl;
                }
            }

            // Write Row
            int row = i + 2;
            wks.cell(row, 1).value() = (int)(i + 1);
            wks.cell(row, 2).value() = voiceStr; // Write Voice
            wks.cell(row, 3).value() = finalStr;
            wks.cell(row, 4).value() = "";
        }

        doc.save();
        std::wcout << L"Converted to: " << xlsxPath << std::endl;
        return true;
    }

    bool ApplyTransTo(LPWSTR lpTargetFile, Util::EncodingType targetEnc) {
        std::wstring filePath = lpTargetFile;
        std::string xlsxPathStr = Util::StringConvert(std::string(filePath.begin(), filePath.end()), Util::EncodingType::ACP, Util::EncodingType::UTF8);

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
        std::vector<std::string> outStrings; // Main Strings
        std::vector<std::string> outVoices;  // Voice Strings

        // 1. Read Excel Rows
        for (uint32_t row = 2; row <= rowCount; ++row) {
            if (wks.cell(row, 1).value().type() == XLValueType::Empty) break;

            // Read Voice
            std::string vStr;
            if (wks.cell(row, 2).value().type() != XLValueType::Empty) {
                vStr = wks.cell(row, 2).value().get<std::string>();
            }
            outVoices.push_back(vStr);

            // Read String
            std::string text;
            if (wks.cell(row, 4).value().type() != XLValueType::Empty) {
                text = wks.cell(row, 4).value().get<std::string>();
            }
            if (text.empty() && wks.cell(row, 3).value().type() != XLValueType::Empty) {
                text = wks.cell(row, 3).value().get<std::string>();
            }

            // Convert Encoding
            std::string encodedStr = Util::StringConvert(text, Util::EncodingType::UTF8, targetEnc);

            outStrings.push_back(encodedStr);
        }

        header.uCount = (uint32_t)outStrings.size();
        header.scrPos.resize(header.uCount);
        header.scrLen.resize(header.uCount);
        header.vLen.resize(header.uCount);

        // 3. Reconstruct Blob
        uint32_t currentOffset = 0;
        std::vector<char> finalBlob;

        for (size_t i = 0; i < header.uCount; ++i) {
            header.scrPos[i] = currentOffset; // Position of Main String

            // Process Main String
            const std::string& s = outStrings[i];
            uint32_t sLen = (uint32_t)s.size() + 1; // + Null
            header.scrLen[i] = sLen;

            // Process Voice String
            const std::string& v = outVoices[i];
            uint32_t vLen = (uint32_t)v.size() + 1; // + Null
            header.vLen[i] = vLen;

            // Append Main String
            finalBlob.insert(finalBlob.end(), s.begin(), s.end());
            finalBlob.push_back('\0');

            // Append Voice String
            finalBlob.insert(finalBlob.end(), v.begin(), v.end());
            finalBlob.push_back('\0');

            // Update offset (String Len + Voice Len)
            currentOffset += sLen + vLen;
        }

        // 4. Write MES File
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
        file.write(reinterpret_cast<char*>(header.vLen.data()), header.uCount * sizeof(uint32_t)); // Write vLen
        file.write(finalBlob.data(), finalBlob.size());

        file.close();
        std::wcout << L"Converted to: " << mesPath << std::endl;
        return true;
    }
}