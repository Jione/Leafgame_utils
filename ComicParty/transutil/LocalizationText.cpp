#include "LocalizationText.h"
#include "LZSS.h"
#include "Util.h"
#include "EventScript.h"
#include "resource.h"
#include <OpenXLSX.hpp>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <map>
#include <fstream>

// OpenXLSX 라이브러리 링크 (빌드 환경에 맞춰 조정)
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

namespace Localization {

    // ------------------------------------------------------------------------
    // Internal Helper Structures
    // ------------------------------------------------------------------------
    struct MesHeaderParsed {
        uint32_t uCount;
        std::vector<uint32_t> scrPos;
        std::vector<uint32_t> scrLen;
        std::vector<uint32_t> vLen;
    };

    const wchar_t* eventFileName[] = { L"asa", L"aya", L"chi", L"com", L"eim", L"iku", L"min", L"miz", L"rei", L"yu" };

    // ------------------------------------------------------------------------
    // Resource & PAK Helper Functions
    // ------------------------------------------------------------------------

    // 리소스(PAK)를 메모리로 로드
    static bool LoadEmbeddedPakResource(std::vector<uint8_t>& outData) {
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAK_EVENT_RES), L"DAT");
        if (!hRes) return false;

        HGLOBAL hMem = LoadResource(NULL, hRes);
        if (!hMem) return false;

        DWORD size = SizeofResource(NULL, hRes);
        void* ptr = LockResource(hMem);

        if (!ptr || size == 0) return false;

        outData.resize(size);
        memcpy(outData.data(), ptr, size);
        return true;
    }

    // 메모리에 로드된 PAK 데이터에서 특정 파일명 검색 및 추출
    static bool ExtractFromEmbeddedPak(const std::vector<uint8_t>& pakData, const std::wstring& targetFilename, std::vector<uint8_t>& outFileData) {
        if (pakData.size() < 4) return false;

        const uint8_t* ptr = pakData.data();
        uint32_t fileCount = *(uint32_t*)ptr;
        ptr += 4;

        // 테이블 영역 체크
        size_t tableSize = fileCount * sizeof(FileEntry);
        if (pakData.size() < 4 + tableSize) return false;

        const FileEntry* table = (const FileEntry*)ptr;

        // 타겟 파일명 (소문자 변환)
        std::wstring targetLower = targetFilename;
        std::transform(targetLower.begin(), targetLower.end(), targetLower.begin(), ::tolower);

        for (uint32_t i = 0; i < fileCount; ++i) {
            const FileEntry& entry = table[i];

            // Entry 이름 변환 (SJIS -> Wide -> Lower)
            std::string sName(entry.Name, strnlen(entry.Name, 16));
            std::wstring wName = Util::MultiByteToWideStr(sName, 932);
            std::transform(wName.begin(), wName.end(), wName.begin(), ::tolower);

            if (wName == targetLower) {
                // 매칭 성공: 압축 해제
                return LZSS::Decompress(pakData, &entry, outFileData);
            }
        }

        return false;
    }

    // ------------------------------------------------------------------------
    // Converter Implementations
    // ------------------------------------------------------------------------
    namespace Converter {

        bool MesToXlsx(const std::vector<uint8_t>& mesData, const std::wstring& savePath, const std::vector<uint8_t>& relatedDatBuffer) {
            if (mesData.size() < 4) return false;

            // 1. Header Parsing
            const uint8_t* ptr = mesData.data();
            uint32_t uCount = *(uint32_t*)ptr;
            ptr += 4;

            // MES Header Structure
            MesHeaderParsed header;

            size_t tablesSize = uCount * 3 * sizeof(uint32_t);
            if (mesData.size() < 4 + tablesSize) return false;

            header.uCount = uCount;
            header.scrPos.resize(uCount);
            header.scrLen.resize(uCount);
            header.vLen.resize(uCount);

            memcpy(header.scrPos.data(), ptr, uCount * 4); ptr += uCount * 4;
            memcpy(header.scrLen.data(), ptr, uCount * 4); ptr += uCount * 4;
            memcpy(header.vLen.data(), ptr, uCount * 4); ptr += uCount * 4;

            const char* blobStart = (const char*)ptr;
            size_t headerSize = 4 + tablesSize;
            size_t blobMaxSize = mesData.size() - headerSize;

            // 2. Character Name Parsing (Using EventScript from Memory)
            std::vector<int> vCharId(uCount, -1);

            // 관련된 dat 데이터가 존재하면 파싱 수행
            if (!relatedDatBuffer.empty()) {
                EventScript::Script::ParseCharaIDTable(relatedDatBuffer, vCharId);
            }

            // 3. Create Excel
            std::string xlsxPathStr = Util::WideToMultiByteStr(savePath, CP_UTF8);
            XLDocument doc;

            try {
                doc.create(xlsxPathStr);
                auto wks = doc.workbook().worksheet("Sheet1");

                // Headers
                wks.cell("A1").value() = "Index";
                wks.cell("B1").value() = "Charactor";
                wks.cell("C1").value() = "Voice";
                wks.cell("D1").value() = "String";
                wks.cell("E1").value() = "Translate";

                // 4. Process Entries
                std::vector<char> stringBlob(blobStart, blobStart + blobMaxSize);

                for (uint32_t i = 0; i < uCount; ++i) {
                    uint32_t sOffset = header.scrPos[i];
                    uint32_t sLen = header.scrLen[i];
                    uint32_t vOffset = sOffset + sLen;
                    uint32_t vLen = header.vLen[i];

                    // --- Extract Character Name ---
                    std::string charStr = "";
                    int cid = (i < vCharId.size()) ? vCharId[i] : -1;

                    if (cid == -2) {
                        charStr = Util::MultiByteToUtf8("선택지", 949);
                    }
                    else if (cid >= 0) {
                        // EventScript를 통해 이름 획득
                        const char* rawName = EventScript::Script::GetCharaName(cid);
                        charStr = Util::MultiByteToUtf8(rawName, 949);
                    }

                    // --- Extract Voice ---
                    std::string voiceStr;
                    if (vLen > 0 && vOffset + vLen <= stringBlob.size()) {
                        voiceStr.assign(&stringBlob[vOffset], vLen - 1); // null 제외
                    }

                    // --- Extract String ---
                    uint32_t processLen = (sLen > 0) ? sLen - 1 : 0;
                    std::string finalStr;
                    if (processLen > 0 && sOffset + processLen <= stringBlob.size()) {
                        finalStr = Util::MesBytesToUtf8(stringBlob, sOffset, processLen);
                    }

                    int row = i + 2;
                    wks.cell(row, 1).value() = (int)(i + 1);
                    wks.cell(row, 2).value() = charStr;
                    wks.cell(row, 3).value() = voiceStr;
                    wks.cell(row, 4).value() = finalStr;
                    wks.cell(row, 5).value() = "";
                }

                doc.save();
                doc.close();
            }
            catch (const std::exception& e) {
                std::cout << "[Error] OpenXLSX Exception: " << e.what() << std::endl;
                return false;
            }

            return true;
        }

        bool XlsxToMes(const std::wstring& srcPath, std::vector<uint8_t>& outMesData) {
            outMesData.clear();
            std::string xlsxPathStr = Util::WideToMultiByteStr(srcPath, CP_UTF8);

            if (!fs::exists(srcPath)) return false;

            XLDocument doc;
            try {
                doc.open(xlsxPathStr);
                auto wks = doc.workbook().worksheet("Sheet1");
                uint32_t rowCount = wks.rowCount();

                // 헤더 정보 준비
                MesHeaderParsed header;
                std::vector<std::vector<char>> blobParts; // 각 라인의 바이너리 청크

                // Row 1 is Header, Start from Row 2
                // OpenXLSX의 rowCount는 실제 데이터가 있는 행까지 반환
                for (uint32_t row = 2; row <= rowCount; ++row) {
                    // Index(A열)가 비어있으면 종료로 간주
                    if (wks.cell(row, 1).value().type() == XLValueType::Empty) break;

                    // 1. Voice 추출 (C열)
                    std::string vStr;
                    if (wks.cell(row, 3).value().type() != XLValueType::Empty) {
                        vStr = wks.cell(row, 3).value().get<std::string>();
                    }

                    // 2. Text 추출 (E열:번역우선, D열:원본)
                    std::string text;
                    if (wks.cell(row, 5).value().type() != XLValueType::Empty) {
                        text = wks.cell(row, 5).value().get<std::string>();
                    }
                    if (text.empty() && wks.cell(row, 4).value().type() != XLValueType::Empty) {
                        text = wks.cell(row, 4).value().get<std::string>();
                    }

                    // 3. 텍스트 변환 (UTF-8 -> MES Bytes)
                    // Util::Utf8ToMesBytes가 제어코드($N 등) 처리를 담당
                    std::vector<char> strBytes = Util::Utf8ToMesBytes(text);

                    // 4. Blob 조립 (String + Null + Voice + Null)
                    std::vector<char> entryBlob;

                    // String
                    entryBlob.insert(entryBlob.end(), strBytes.begin(), strBytes.end());
                    entryBlob.push_back('\0'); // Null Terminator

                    // Voice
                    for (char c : vStr) entryBlob.push_back(c);
                    entryBlob.push_back('\0'); // Null Terminator

                    blobParts.push_back(entryBlob);

                    // 5. 테이블 정보 기록
                    // strBytes.size() + 1 (null)
                    header.scrLen.push_back((uint32_t)strBytes.size() + 1);
                    header.vLen.push_back((uint32_t)vStr.size() + 1);
                }

                header.uCount = (uint32_t)blobParts.size();
                header.scrPos.resize(header.uCount);

                // 6. 전체 Blob 병합 및 Offset 계산
                std::vector<char> finalBlob;
                uint32_t currentOffset = 0;
                for (size_t i = 0; i < header.uCount; ++i) {
                    header.scrPos[i] = currentOffset;
                    finalBlob.insert(finalBlob.end(), blobParts[i].begin(), blobParts[i].end());
                    currentOffset += (uint32_t)blobParts[i].size();
                }

                // 7. 최종 바이너리 생성 (Header + Tables + Blob)
                // Size = 4(Count) + 3*4*Count(Tables) + BlobSize
                size_t totalSize = 4 + (header.uCount * 12) + finalBlob.size();
                outMesData.resize(totalSize);

                uint8_t* ptr = outMesData.data();

                // Write Count
                memcpy(ptr, &header.uCount, 4); ptr += 4;
                // Write Tables
                memcpy(ptr, header.scrPos.data(), header.uCount * 4); ptr += header.uCount * 4;
                memcpy(ptr, header.scrLen.data(), header.uCount * 4); ptr += header.uCount * 4;
                memcpy(ptr, header.vLen.data(), header.uCount * 4); ptr += header.uCount * 4;
                // Write Blob
                memcpy(ptr, finalBlob.data(), finalBlob.size());

                doc.close();
            }
            catch (const std::exception& e) {
                std::cout << "[Error] OpenXLSX Exception: " << e.what() << std::endl;
                return false;
            }

            return true;
        }

    } // namespace Converter


    // ------------------------------------------------------------------------
    // Main Entry Points
    // ------------------------------------------------------------------------

    // Text Extraction
    void ExtractTextResources() {
        std::wcout << L"[Info] Starting Text Extraction (event.pak)..." << std::endl;

        const std::wstring pakName = L"event.pak";
        if (!fs::exists(pakName)) {
            std::wcout << L"[Error] event.pak not found." << std::endl;
            return;
        }

        // 1. PAK 파일 열기
        HANDLE hFile = CreateFileW(pakName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            std::wcout << L"[Error] Failed to open event.pak" << std::endl;
            return;
        }

        // 2. 헤더 및 테이블 읽기
        uint32_t uFileCount = 0;
        DWORD dwRead;
        ReadFile(hFile, &uFileCount, 4, &dwRead, NULL);

        std::vector<FileEntry> table(uFileCount);
        ReadFile(hFile, table.data(), sizeof(FileEntry) * uFileCount, &dwRead, NULL);

        // 3. 출력 폴더 준비
        fs::path outDir = L"KoS";
        fs::create_directories(outDir);

        // Dat 파일 캐시 (Key: Lowercase Filename, Value: Data Buffer)
        std::map<std::wstring, std::vector<uint8_t>> datCache;

        // --- PASS 1: eve.dat 파일들을 메모리에 로드 ---
        std::wcout << L"Phase 1: Loading helper files (.dat) to memory..." << std::endl;
        for (const auto& entry : table) {
            // 이름 변환 (SJIS -> Wide)
            std::string sName(entry.Name);
            std::wstring fName = Util::MultiByteToWideStr(sName, 932); // Shift-JIS

            fs::path p(fName);
            std::wstring ext = p.extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            // .dat 파일인 경우 메모리에 저장
            if (ext == L".dat") {
                std::vector<uint8_t> data;
                if (LZSS::Decompress(hFile, &entry, data)) {
                    std::wstring key = fName;
                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    datCache[key] = std::move(data);
                }
            }
        }

        // --- PASS 2: mes 파일을 xlsx로 변환 (캐시된 dat 사용) ---
        std::wcout << L"Phase 2: Converting text files (.mes)..." << std::endl;
        int count = 0;
        for (const auto& entry : table) {
            std::string sName(entry.Name);
            std::wstring fName = Util::MultiByteToWideStr(sName, 932);

            fs::path p(fName);
            std::wstring ext = p.extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == L".mes") {
                // A. MES 데이터 로드
                std::vector<uint8_t> mesData;
                if (!LZSS::Decompress(hFile, &entry, mesData)) {
                    std::wcout << L"[Error] Failed to load: " << fName << std::endl;
                    continue;
                }

                // B. 파일명 규칙 변경 및 관련된 DAT 파일 찾기
                std::wstring prefix = fName;
                if (prefix.length() > 7) {
                    prefix = prefix.substr(0, prefix.length() - 7);
                }
                else {
                    // 예외 처리: 파일명이 너무 짧은 경우 파일명(stem) 그대로 사용
                    prefix = p.stem().wstring();
                }

                // Eve 파일명 생성 (추출된 이름 + eve.dat)
                std::wstring targetDatName = prefix + L"eve.dat";
                std::transform(targetDatName.begin(), targetDatName.end(), targetDatName.begin(), ::tolower);

                std::vector<uint8_t> emptyVec;
                // 캐시에서 검색 (없으면 emptyVec 전달)
                const std::vector<uint8_t>& refDat = (datCache.count(targetDatName)) ? datCache[targetDatName] : emptyVec;

                // C. 변환 수행
                // 저장명 규칙: [앞3글자].xlsx
                std::wstring shortName = prefix + L".xlsx";
                fs::path savePath = outDir / shortName;

                std::wcout << L"Extracting: " << fName << L" -> " << shortName;
                if (!refDat.empty()) {
                    std::wcout << L" (Linked: " << targetDatName << L")";
                }
                std::wcout << std::endl;

                if (Converter::MesToXlsx(mesData, savePath.wstring(), refDat)) {
                    count++;
                }
                else {
                    std::wcout << L"[Error] Conversion failed: " << fName << std::endl;
                }
            }
        }

        CloseHandle(hFile);
        std::wcout << L"[Info] Extracted " << count << L" text files to KoS folder." << std::endl;
    }
    
    // Building Text Archive
    void BuildTextArchive() {
        fs::path kosDir = L"KoS";
        if (!fs::exists(kosDir)) {
            std::wcout << L"[Error] KoS directory not found." << std::endl;
            return;
        }

        // 1. 내장 리소스(PAK) 로드
        std::vector<uint8_t> embeddedPakData;
        bool hasEmbeddedPak = LoadEmbeddedPakResource(embeddedPakData);
        if (hasEmbeddedPak) {
            std::wcout << L"[Info] Embedded resource PAK loaded (" << embeddedPakData.size() << L" bytes)." << std::endl;
        }
        else {
            std::wcout << L"[Warning] Failed to load embedded resource PAK." << std::endl;
        }

        // 2. 대상 파일 수집
        std::vector<fs::path> targetFiles;
        for (const auto& entry : fs::directory_iterator(kosDir)) {
            if (entry.is_regular_file() && entry.path().extension() == L".xlsx") {
                for (int i = 0; i < (sizeof(eventFileName) / sizeof(eventFileName[0])); ++i) {
                    if (entry.path().stem() == eventFileName[i]) {
                        targetFiles.push_back(entry.path());
                        break;
                    }
                }
            }
        }

        if (targetFiles.empty()) {
            std::wcout << L"No .xlsx files found in KoS." << std::endl;
            return;
        }

        // 3. 총 엔트리 수 계산 (xlsx 대응 파일 + 내장 dat 파일)
        uint32_t totalEntryCount = 0;
        for (const auto& xlsxPath : targetFiles) {
            totalEntryCount++; // .mes 파일

            // 내장된 dat 파일이 있는지 미리 확인 (엔트리 수 확보용)
            if (hasEmbeddedPak) {
                std::wstring stem = xlsxPath.stem().wstring();
                std::wstring datName = stem + L"eve.dat";

                // 실제 데이터를 추출해보지는 않고, 단순히 존재 여부만 체크해도 되지만
                // 여기서는 간단히 하기 위해 추출 시도 함수를 재사용하거나, 
                // 단순히 무조건 체크한다고 가정하고 루프 안에서 처리.
                // *정확성을 위해* 검색만 하는 함수를 분리하는 것이 좋으나, 
                // 여기서는 `ExtractFromEmbeddedPak` 호출 시 데이터가 나오면 카운트 증가.
                std::vector<uint8_t> dummy;
                if (ExtractFromEmbeddedPak(embeddedPakData, datName, dummy)) {
                    totalEntryCount++;
                }
            }
        }

        // 4. 아카이브 생성
        std::wstring pakName = L"KoS.pak";
        HANDLE hArchive = CreateFileW(pakName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hArchive == INVALID_HANDLE_VALUE) {
            std::wcout << L"[Error] Failed to create KoS.pak" << std::endl;
            return;
        }

        DWORD dwWritten;
        WriteFile(hArchive, &totalEntryCount, 4, &dwWritten, NULL);

        uint32_t tableStart = 4;
        uint32_t dataStart = tableStart + (totalEntryCount * sizeof(FileEntry));
        SetFilePointer(hArchive, dataStart, NULL, FILE_BEGIN);

        std::vector<FileEntry> table;
        int successCount = 0;

        auto WriteEntry = [&](const std::wstring& pakFileName, const std::vector<uint8_t>& rawData) {
            FileEntry entry = { 0 };
            std::string sjisName = Util::WideToMultiByteStr(pakFileName, 932);
            strncpy(entry.Name, sjisName.c_str(), 15);

            std::vector<uint8_t> compressedData;
            size_t compSize = LZSS::Compress(rawData, compressedData);

            entry.DataOffset = SetFilePointer(hArchive, 0, NULL, FILE_CURRENT);

            if (compSize > 0 && compSize < rawData.size()) {
                entry.Flags = 1;
                uint32_t totalBlobSize = 4 + 4 + (uint32_t)compressedData.size();
                uint32_t orgSize = (uint32_t)rawData.size();
                entry.DataSize = totalBlobSize;
                WriteFile(hArchive, &totalBlobSize, 4, &dwWritten, NULL);
                WriteFile(hArchive, &orgSize, 4, &dwWritten, NULL);
                WriteFile(hArchive, compressedData.data(), (DWORD)compressedData.size(), &dwWritten, NULL);
            }
            else {
                entry.Flags = 0;
                entry.DataSize = (uint32_t)rawData.size();
                WriteFile(hArchive, rawData.data(), (DWORD)rawData.size(), &dwWritten, NULL);
            }
            table.push_back(entry);
            successCount++;
            };

        // 5. 파일 처리
        for (const auto& xlsxPath : targetFiles) {
            std::wstring stem = xlsxPath.stem().wstring();
            std::wcout << L"Processing: " << stem << L"..." << std::endl;

            // A. 내장 DAT 파일 처리
            if (hasEmbeddedPak) {
                std::wstring datName = stem + L"eve.dat";
                std::vector<uint8_t> datData;
                if (ExtractFromEmbeddedPak(embeddedPakData, datName, datData)) {
                    std::wcout << L"  [+] Injecting Resource: " << datName << std::endl;
                    WriteEntry(datName, datData);
                }
            }

            // B. XLSX -> MES 변환
            std::wstring mesName = stem + L"mes.mes";
            std::wcout << L"  [+] Converting: " << xlsxPath.filename() << L" -> " << mesName << std::endl;

            std::vector<uint8_t> mesData;
            if (Converter::XlsxToMes(xlsxPath.wstring(), mesData)) {
                WriteEntry(mesName, mesData);
            }
            else {
                std::wcout << L"  [!] Conversion Failed: " << xlsxPath.filename() << std::endl;
                // 실패 시 Dummy Entry 채움
                FileEntry dummy = { 0 };
                std::string sName = Util::WideToMultiByteStr(mesName, 932);
                strncpy(dummy.Name, sName.c_str(), 15);
                dummy.DataOffset = SetFilePointer(hArchive, 0, NULL, FILE_CURRENT);
                table.push_back(dummy);
                successCount++;
            }
        }

        // 테이블 쓰기
        SetFilePointer(hArchive, tableStart, NULL, FILE_BEGIN);
        WriteFile(hArchive, table.data(), sizeof(FileEntry) * totalEntryCount, &dwWritten, NULL);

        CloseHandle(hArchive);
        std::wcout << L"Created KoS.pak with " << successCount << L" files." << std::endl;
    }
}
