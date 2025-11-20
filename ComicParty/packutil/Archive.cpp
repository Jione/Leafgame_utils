#include "Archive.h"
#include "LZSS.h"
#include "Util.h"
#include <filesystem>
#include <map>
#include <algorithm>
#include <set>
#include <fstream>

namespace fs = std::filesystem;

namespace Archive {

    void ExtractArchive(LPWSTR lpTargetFile) {
        HANDLE hFile = CreateFileW(lpTargetFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            std::wcout << L"Failed to open archive: " << lpTargetFile << std::endl;
            return;
        }

        // Read Header
        uint32_t uFileCount = 0;
        DWORD dwRead;
        ReadFile(hFile, &uFileCount, 4, &dwRead, NULL);

        std::wcout << L"Extracting " << uFileCount << L" files..." << std::endl;

        // Read Table
        std::vector<FileEntry> table(uFileCount);
        ReadFile(hFile, table.data(), sizeof(FileEntry) * uFileCount, &dwRead, NULL);

        // Prepare Output Directory
        fs::path archivePath(lpTargetFile);
        fs::path outDir = archivePath.parent_path() / archivePath.stem();
        fs::create_directories(outDir);

        std::map<std::wstring, int> nameTotalCounts;
        for (const auto& entry : table) {
            std::wstring originalName = Util::SJIS_To_Wide(entry.Name);
            nameTotalCounts[originalName]++;
        }

        // Extract Loop
        std::map<std::wstring, int> nameCurrentIndex;

        for (const auto& entry : table) {
            // Decode Name
            std::wstring originalName = Util::SJIS_To_Wide(entry.Name);
            std::wstring finalName = originalName;

            if (nameTotalCounts[originalName] > 1) {
                int idx = nameCurrentIndex[originalName]++;

                wchar_t suffix[16];
                swprintf_s(suffix, L"@%02d", idx); // @00, @01, @02 ...

                fs::path p(originalName);
                std::wstring stem = p.stem().wstring();
                std::wstring ext = p.extension().wstring();

                finalName = stem + suffix + ext;
            }

            fs::path finalPath = outDir / finalName;
            std::wcout << L"Extracting: " << finalName << L"..." << std::endl;

            if (!LZSS::Decompress(hFile, &entry, finalPath.c_str())) {
                std::wcout << L"Failed to decompress: " << finalName << std::endl;
            }
        }

        CloseHandle(hFile);
        std::wcout << L"Extraction Complete." << std::endl;
    }

    void CreateArchive(LPWSTR lpTargetFolder) {
        fs::path root(lpTargetFolder);
        if (!fs::exists(root)) return;

        // Collect Files
        std::vector<fs::path> files;
        for (const auto& entry : fs::directory_iterator(root)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }

        if (files.empty()) {
            std::wcout << L"No files found." << std::endl;
            return;
        }

        std::wstring archiveName = root.filename().wstring() + L".pak";
        HANDLE hArchive = CreateFileW(archiveName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        // 1. Write Header (Count)
        uint32_t uCount = (uint32_t)files.size();
        DWORD dwWritten;
        WriteFile(hArchive, &uCount, 4, &dwWritten, NULL);

        // 2. Reserve Table Space
        uint32_t tableStart = 4;
        uint32_t dataStart = tableStart + (uCount * sizeof(FileEntry));
        SetFilePointer(hArchive, dataStart, NULL, FILE_BEGIN);

        // 3. Process Files
        std::vector<FileEntry> table;
        std::set<std::wstring> excludeExts = { L".wav", L".c16", L".mp3", L".ogg" }; // No Compression extensions

        for (const auto& filePath : files) {
            FileEntry entry = { 0 };

            // Name Handling
            std::wstring fileName = filePath.filename().wstring();
            std::wstring cleanName = Util::GetCleanFileName(fileName);
            std::string sjisName = Util::Wide_To_SJIS(cleanName);
            strncpy(entry.Name, sjisName.c_str(), 15); // Truncate to 15 + null

            // Read Source File
            std::ifstream inFile(filePath, std::ios::binary | std::ios::ate);
            size_t fileSize = inFile.tellg();
            inFile.seekg(0, std::ios::beg);
            std::vector<uint8_t> fileData(fileSize);
            if (fileSize > 0) inFile.read((char*)fileData.data(), fileSize);
            inFile.close();

            entry.DataSize = (uint32_t)fileSize;
            entry.DataOffset = SetFilePointer(hArchive, 0, NULL, FILE_CURRENT);

            // Compression Decision
            std::wstring ext = filePath.extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            bool bShouldCompress = (excludeExts.find(ext) == excludeExts.end());

            if (bShouldCompress) {
                std::vector<uint8_t> compressedData;

                std::vector<uint8_t> lzssBody;
                size_t lzssSize = LZSS::Compress(fileData, lzssBody);

                // Check if compression was effective
                if (lzssSize > 0 && lzssSize < fileSize) {
                    entry.Flags = 1;

                    uint32_t totalBlobSize = 4 + 4 + (uint32_t)lzssBody.size(); // SizeField + UncompField + Body
                    entry.DataSize = totalBlobSize;
                    WriteFile(hArchive, &totalBlobSize, 4, &dwWritten, NULL);
                    WriteFile(hArchive, &fileSize, 4, &dwWritten, NULL);
                    WriteFile(hArchive, lzssBody.data(), (DWORD)lzssBody.size(), &dwWritten, NULL);
                }
                else {
                    // Compression failed or increased size -> Store Raw
                    entry.Flags = 0;
                    WriteFile(hArchive, fileData.data(), (DWORD)fileSize, &dwWritten, NULL);
                }
            }
            else {
                entry.Flags = 0;
                WriteFile(hArchive, fileData.data(), (DWORD)fileSize, &dwWritten, NULL);
            }

            std::wcout << L"Packed: " << cleanName << (entry.Flags ? L" [Compressed]" : L" [Raw]") << std::endl;
            table.push_back(entry);
        }

        // 4. Write Table
        SetFilePointer(hArchive, tableStart, NULL, FILE_BEGIN);
        WriteFile(hArchive, table.data(), sizeof(FileEntry) * uCount, &dwWritten, NULL);

        CloseHandle(hArchive);
        std::wcout << L"Archive Created: " << archiveName << std::endl;
    }
}
