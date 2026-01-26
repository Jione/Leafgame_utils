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
#include <future>

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
                    // EventScript를 통해 이름 획득
                    int cid = (i < vCharId.size()) ? vCharId[i] : -1;
                    std::string charStr = EventScript::Script::GetUtf8Name(cid);

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

    // 병렬 처리 결과를 담을 구조체
    struct ProcessResult {
        bool success;
        std::wstring fileName;
        FileEntry entry;
        std::vector<uint8_t> dataBlob; // 압축된 데이터 (또는 Raw)
    };

    // Building Text Archive
    void BuildTextArchive() {
        std::wcout << L"==============================================" << std::endl;
        std::wcout << L"       Building Patch Archive (KoS.pak)       " << std::endl;
        std::wcout << L"==============================================" << std::endl;

        fs::path kosDir = L"KoS";
        if (!fs::exists(kosDir)) return;

        // 1. 내장 리소스 로드 (메인 스레드에서 미리 로드하여 읽기 전용으로 만듦)
        std::vector<uint8_t> embeddedPakData;
        bool hasEmbeddedPak = LoadEmbeddedPakResource(embeddedPakData);

        // 2. 파일 목록 수집
        std::vector<fs::path> targetFiles;
        for (const auto& entry : fs::directory_iterator(kosDir)) {
            if (entry.is_regular_file() && entry.path().extension() == L".xlsx") {
                targetFiles.push_back(entry.path());
            }
        }

        if (targetFiles.empty()) return;

        // 3. 병렬 작업 생성 (Task Launch)
        std::wcout << L"[Info] Launching parallel tasks for " << targetFiles.size() << L" files..." << std::endl;
        std::vector<std::future<std::vector<ProcessResult>>> futures;

        for (const auto& xlsxPath : targetFiles) {
            // 각 파일 처리를 비동기로 실행
            futures.push_back(std::async(std::launch::async, [xlsxPath, hasEmbeddedPak, &embeddedPakData]() {
                std::vector<ProcessResult> results;
                std::wstring stem = xlsxPath.stem().wstring();

                // A. 내장 DAT 파일 처리 (존재 시)
                if (hasEmbeddedPak) {
                    std::wstring datName = stem + L"eve.dat";
                    std::vector<uint8_t> datData;
                    if (ExtractFromEmbeddedPak(embeddedPakData, datName, datData)) {
                        ProcessResult res;
                        res.success = true;
                        res.fileName = datName;

                        FileEntry entry = { 0 };
                        std::string sName = Util::WideToMultiByteStr(datName, 932);
                        strncpy(entry.Name, sName.c_str(), 15);

                        // 압축 수행
                        std::vector<uint8_t> compressed;
                        size_t compSize = LZSS::Compress(datData, compressed);

                        if (compSize > 0 && compSize < datData.size()) {
                            res.entry = entry;
                            res.entry.Flags = 1;
                            res.entry.DataSize = 4 + 4 + (uint32_t)compressed.size(); // Total Size

                            // 포맷 조립: [TotalSize][OrgSize][Body]
                            uint32_t orgSize = (uint32_t)datData.size();
                            res.dataBlob.resize(res.entry.DataSize);
                            memcpy(res.dataBlob.data(), &res.entry.DataSize, 4);
                            memcpy(res.dataBlob.data() + 4, &orgSize, 4);
                            memcpy(res.dataBlob.data() + 8, compressed.data(), compressed.size());
                        }
                        else {
                            res.entry = entry;
                            res.entry.Flags = 0;
                            res.entry.DataSize = (uint32_t)datData.size();
                            res.dataBlob = std::move(datData);
                        }
                        results.push_back(std::move(res));
                    }
                }

                // B. XLSX -> MES 변환
                std::wstring mesName = stem + L"mes.mes";
                std::vector<uint8_t> mesData;
                ProcessResult mesRes;
                mesRes.fileName = mesName;

                if (Converter::XlsxToMes(xlsxPath.wstring(), mesData)) {
                    mesRes.success = true;
                    FileEntry entry = { 0 };
                    std::string sName = Util::WideToMultiByteStr(mesName, 932);
                    strncpy(entry.Name, sName.c_str(), 15);

                    // 압축 수행
                    std::vector<uint8_t> compressed;
                    size_t compSize = LZSS::Compress(mesData, compressed);

                    if (compSize > 0 && compSize < mesData.size()) {
                        mesRes.entry = entry;
                        mesRes.entry.Flags = 1;
                        mesRes.entry.DataSize = 4 + 4 + (uint32_t)compressed.size();

                        uint32_t orgSize = (uint32_t)mesData.size();
                        mesRes.dataBlob.resize(mesRes.entry.DataSize);
                        memcpy(mesRes.dataBlob.data(), &mesRes.entry.DataSize, 4);
                        memcpy(mesRes.dataBlob.data() + 4, &orgSize, 4);
                        memcpy(mesRes.dataBlob.data() + 8, compressed.data(), compressed.size());
                    }
                    else {
                        mesRes.entry = entry;
                        mesRes.entry.Flags = 0;
                        mesRes.entry.DataSize = (uint32_t)mesData.size();
                        mesRes.dataBlob = std::move(mesData);
                    }
                }
                else {
                    // 실패 시 더미 엔트리
                    mesRes.success = false;
                    FileEntry entry = { 0 };
                    std::string sName = Util::WideToMultiByteStr(mesName, 932);
                    strncpy(entry.Name, sName.c_str(), 15);
                    mesRes.entry = entry;
                    mesRes.entry.Flags = 0;
                    mesRes.entry.DataSize = 0;
                }
                results.push_back(std::move(mesRes));

                return results;
                }));
        }

        // 4. 결과 수집 및 파일 쓰기 (Main Thread)
        std::wstring pakName = L"KoS.pak";
        HANDLE hArchive = CreateFileW(pakName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hArchive == INVALID_HANDLE_VALUE) return;

        // 헤더 공간 예약 (나중에 업데이트)
        // 최대 예상 개수: 파일 수 * 2 (dat + mes)
        uint32_t estimatedCount = (uint32_t)targetFiles.size() * 2;
        DWORD dwWritten;
        WriteFile(hArchive, &estimatedCount, 4, &dwWritten, NULL);

        uint32_t tableStart = 4;
        uint32_t dataStart = tableStart + (estimatedCount * sizeof(FileEntry));
        SetFilePointer(hArchive, dataStart, NULL, FILE_BEGIN);

        std::vector<FileEntry> finalTable;
        int successCount = 0;

        for (auto& fut : futures) {
            // 작업 완료 대기
            auto batchResults = fut.get();

            for (auto& res : batchResults) {
                if (res.success) {
                    std::wcout << L"Packed: " << res.fileName << std::endl;
                }
                else {
                    std::wcout << L"[Fail] " << res.fileName << std::endl;
                }

                // 오프셋 설정 및 데이터 쓰기
                res.entry.DataOffset = SetFilePointer(hArchive, 0, NULL, FILE_CURRENT);
                if (!res.dataBlob.empty()) {
                    WriteFile(hArchive, res.dataBlob.data(), (DWORD)res.dataBlob.size(), &dwWritten, NULL);
                }

                finalTable.push_back(res.entry);
                successCount++;
            }
        }

        // 헤더 및 테이블 업데이트
        uint32_t finalCount = (uint32_t)finalTable.size();
        SetFilePointer(hArchive, 0, NULL, FILE_BEGIN);
        WriteFile(hArchive, &finalCount, 4, &dwWritten, NULL);

        SetFilePointer(hArchive, tableStart, NULL, FILE_BEGIN);
        WriteFile(hArchive, finalTable.data(), sizeof(FileEntry) * finalCount, &dwWritten, NULL);

        CloseHandle(hArchive);
        std::wcout << L"Created KoS.pak with " << finalCount << L" files." << std::endl;
    }
}
