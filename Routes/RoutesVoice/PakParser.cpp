#include "PakParser.h"
#include "StringUtil.h"
#include <algorithm> // std::transform
#include <cstdio>    // sprintf_s for byte dump (optional)
#include <Windows.h> // MessageBoxA for error

bool PakParser::Parse(const std::wstring& pakFilePath,
    std::vector<PakFileInfo>& outPakFiles) {
    size_t lastDot = pakFilePath.find_last_of(L".");
    std::wstring ext = pakFilePath.substr(lastDot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == L"pak") {
        return ParsePak(pakFilePath, outPakFiles);
    }
    else if (ext == L"sfs") {
        return ParseSfs(pakFilePath, outPakFiles);
    }
    else if (ext == L"bin") {
        return ParseBin(pakFilePath, outPakFiles);
    }
    else {
        dprintf("Invalid extension: %s", StringUtil::GetInstance().WstringToString(ext).c_str());
        return false;
    }
}
bool PakParser::ParsePak(const std::wstring& pakFilePath,
    std::vector<PakFileInfo>& outPakFiles) {
    std::ifstream file(pakFilePath, std::ios::binary | std::ios::in);
    if (!file.is_open()) {
        dprintf("Failed to open PAK File: %s", StringUtil::GetInstance().WstringToString(pakFilePath).c_str());
        //MessageBoxA(NULL, (std::string("Failed to open PAK File: ") + pakFilePath).c_str(), "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // 1. PAK 헤더 읽기
    char signature[4];
    uint32_t numFiles;

    if (!file.read(signature, 4)) {
        dprintf("Failed to read PAK File Signature.");
        return false;
    }
    if (std::string(signature, 4) != "KCAP") {
        dprintf("Invalid PAK Signature. (Mismatch 'KCAP')");
        return false;
    }

    if (!file.read(reinterpret_cast<char*>(&numFiles), 4)) {
        dprintf("Failed to read contained file counts.");
        return false;
    }
    // file.read(reinterpret_cast<char*>(&numFiles), 4); // C++14에서는 명시적 캐스팅

    // 2. 파일 정보 파싱
    outPakFiles.clear();
    for (uint32_t i = 0; i < numFiles; ++i) {
        // 파일 정보 (36바이트)
        uint32_t fileType;
        char fileNameRaw[24];
        uint32_t dataOffset;
        uint32_t dataLength;

        if (!file.read(reinterpret_cast<char*>(&fileType), 4) ||
            !file.read(fileNameRaw, 24) ||
            !file.read(reinterpret_cast<char*>(&dataOffset), 4) ||
            !file.read(reinterpret_cast<char*>(&dataLength), 4))
        {
            dprintf("Failed to read PAK file item(Index: %d)", i);
            return false;
        }

        std::string currentFileName(fileNameRaw, 24);
        // Zero-padding된 문자열에서 실제 문자열만 추출
        size_t firstNull = currentFileName.find('\0');
        if (firstNull != std::string::npos) {
            currentFileName = currentFileName.substr(0, firstNull);
        }

        // 비압축(0x00000000) 이고, OGG 또는 WAV 파일만 저장
        if (fileType == 0x00000000 && IsAudioFileExtension(currentFileName)) {
            PakFileInfo info;
            info.fileName = currentFileName;
            info.dataOffset = dataOffset;
            info.dataLength = dataLength;
            outPakFiles.push_back(info);
        }
    }

    return true;
}

// SFS 파일 파싱 함수, 구현 필요
bool PakParser::ParseSfs(const std::wstring& pakFilePath,
    std::vector<PakFileInfo>& outPakFiles) {
    std::ifstream file(pakFilePath, std::ios::binary | std::ios::in);
    if (!file.is_open()) {
        dprintf("Failed to open SFS File: %s", StringUtil::GetInstance().WstringToString(pakFilePath).c_str());
        return false;
    }

    // SFS 파서 상수 설정
    static constexpr int headerSize = 0x30;
    static constexpr int alignBytes = 0x800;

    // 파일 정보 파싱
    std::vector<uint32_t> parsedOffset;
    uint32_t firstOffset;

    // 첫 레코드 읽기
    file.read(reinterpret_cast<char*>(&firstOffset), 4);
    if (!file) return false;

    parsedOffset.clear();
    parsedOffset.push_back(firstOffset);

    // table 끝은 첫 dataOffset까지
    std::streampos tableEnd = static_cast<std::streampos>(firstOffset * alignBytes);
    while (file.tellg() < tableEnd) {
        uint32_t dataOffset;
        file.read(reinterpret_cast<char*>(&dataOffset), 4);
        if ((!file) || (dataOffset == 0))  break;
        parsedOffset.push_back(dataOffset);
    }

    // 오프셋 유효성 검사 및 파일 정보 파싱
    outPakFiles.clear();
    char signature[4];
    uint8_t dataLengthBE[4]{ 0, };

    for (uint32_t dataOffset : parsedOffset) {
        // 데이터 오프셋으로 이동
        dataOffset *= alignBytes;
        file.seekg(dataOffset, std::ios::beg);
        if (!file) {
            dprintf("[Error] Failed to seek to SFS File: %s, 0x%08X", StringUtil::GetInstance().WstringToString(pakFilePath).c_str(), dataOffset);
            continue;
        }

        // VAGp 헤더 검사
        if (!file.read(signature, 4)) {
            dprintf("Failed to read SFS File Signature.");
            continue;
        }
        bool isStereo = (std::string(signature, 4) == "STER");
        if (!(isStereo || (std::string(signature, 4) == "VAGp"))) {
            //dprintf("Invalid SFS Signature. (Mismatch 'VAGp')");
            continue;
        }

        char fileNameRaw[16]{ 0, };

        // 데이터 길이 검사
        file.seekg(8, std::ios::cur);
        if (!file.read(reinterpret_cast<char*>(dataLengthBE), 4)) {
            dprintf("Failed to read VAG File Length.");
            continue;
        }
        uint32_t dataLength = (uint32_t(dataLengthBE[0]) << 24) | (uint32_t(dataLengthBE[1]) << 16) | (uint32_t(dataLengthBE[2]) << 8) | (uint32_t(dataLengthBE[3]) << 0);
        if (isStereo) {
            dataLength *= 2;
        }
        dataLength += headerSize;
        file.seekg(16, std::ios::cur);
        if (!file.read(fileNameRaw, 16)) {
            dprintf("Failed to read VAG File name.");
            continue;
        }
        std::string currentFileName(fileNameRaw, 16);
        currentFileName = ExtractVagName(currentFileName);

        PakFileInfo info;
        info.fileName = currentFileName;
        info.dataOffset = dataOffset;
        info.dataLength = dataLength;
        outPakFiles.push_back(info);
    }

    return (outPakFiles.size() != 0);
}

// BIN 파일 파싱 함수
bool PakParser::ParseBin(const std::wstring& pakFilePath,
    std::vector<PakFileInfo>& outPakFiles) {
    // 16바이트씩 읽어들이는 구조체
    struct BinEntry {
        uint32_t reserved1;
        uint32_t dataOffset;
        uint32_t dataLength;
        uint32_t reserved2;
    };
    static constexpr int headerSize = 0x30;

    // 입력받은 bin 파일 경로
    std::wstring pakPath = pakFilePath;

    // NULL 이후 문자 삭제
    size_t sz = pakPath.find(L'\0');
    if (sz != std::string::npos) pakPath.resize(sz);

    // 파일 정보 파싱
    std::vector<BinEntry> parsedEntries;
    bool isFirst = true;
    outPakFiles.clear();
    char signature[4];
    uint8_t dataLengthBE[4]{ 0, };

    for (int i = 0; i <= 1; ++i) {
        if (i != 0) {
            isFirst = false;
            if ((pakPath.size() >= 11) && (pakPath.substr(pakPath.size() - 11) == L"V0_FILE.BIN")) {
                pakPath.replace((pakPath.size() - 10), 1, L"1");
            }
            else {
                break;
            }
        }
        std::ifstream file(pakPath, std::ios::binary | std::ios::in);
        if (!file.is_open()) {
            dprintf("Failed to open BIN File: %s", StringUtil::GetInstance().WstringToString(pakPath).c_str());
            continue;
        }

        // 첫 레코드 읽기
        BinEntry first{};
        file.read(reinterpret_cast<char*>(&first.reserved1), 4);
        file.read(reinterpret_cast<char*>(&first.dataOffset), 4);
        file.read(reinterpret_cast<char*>(&first.dataLength), 4);
        file.read(reinterpret_cast<char*>(&first.reserved2), 4);
        if (!file) continue;

        parsedEntries.clear();
        parsedEntries.push_back(first);

        // table 끝은 첫 dataOffset까지
        std::streampos tableEnd = static_cast<std::streampos>(first.dataOffset);
        while (file.tellg() < tableEnd) {
            BinEntry entry{};
            file.read(reinterpret_cast<char*>(&entry.reserved1), 4);
            file.read(reinterpret_cast<char*>(&entry.dataOffset), 4);
            file.read(reinterpret_cast<char*>(&entry.dataLength), 4);
            file.read(reinterpret_cast<char*>(&entry.reserved2), 4);
            if (!file) break;
            parsedEntries.push_back(entry);
        }

        for (BinEntry& entry : parsedEntries) {
            // 데이터 오프셋으로 이동
            file.seekg(entry.dataOffset, std::ios::beg);
            if (!file) {
                dprintf("[Error] Failed to seek to BIN File: %s, 0x%08X", StringUtil::GetInstance().WstringToString(pakPath).c_str(), entry.dataOffset);
                continue;
            }

            // VAGp 헤더 검사
            if (!file.read(signature, 4)) {
                dprintf("Failed to read BIN File Signature.");
                continue;
            }
            if (std::string(signature, 4) != "VAGp") {
                dprintf("Invalid BIN Signature. (Mismatch 'VAGp')");
                continue;
            }

            char fileNameRaw[16]{ 0, };
            uint32_t dataOffset = entry.dataOffset;

            // 데이터 길이 검사
            file.seekg(8, std::ios::cur);
            if (!file.read(reinterpret_cast<char*>(dataLengthBE), 4)) {
                dprintf("Failed to read VAG File Length.");
                continue;
            }
            uint32_t dataLength = headerSize + (uint32_t(dataLengthBE[0]) << 24) | (uint32_t(dataLengthBE[1]) << 16) | (uint32_t(dataLengthBE[2]) << 8) | (uint32_t(dataLengthBE[3]) << 0);
            file.seekg(16, std::ios::cur);
            if (!file.read(fileNameRaw, 16)) {
                dprintf("Failed to read VAG File name.");
                continue;
            }
            std::string currentFileName(fileNameRaw, 16);
            currentFileName = ExtractVagName(currentFileName);

            PakFileInfo info;
            info.fileName = currentFileName;
            info.dataOffset = dataOffset;
            info.dataLength = dataLength;
            info.isFirst = isFirst;
            outPakFiles.push_back(info);
        }
        file.close();
    }

    return (outPakFiles.size() != 0);
}

std::string PakParser::ExtractUniqueId(const std::string& fullFileName) {
    size_t firstUnderscore = fullFileName.find('_');
    if (firstUnderscore == std::string::npos) {
        return ""; // 언더바 없으면 오류 또는 적절한 처리
    }
    size_t secondUnderscore = fullFileName.find('_', firstUnderscore + 1);
    if (secondUnderscore == std::string::npos) {
        // 두 번째 언더바가 없으면 첫 번째 이후 모두 포함 (예: "00010_001.OGG" -> "00010_001")
        return fullFileName.substr(0, fullFileName.find('.')); // 확장자 제거
    }
    // 예: "00010_001_03.OGG" -> "00010_001"
    return fullFileName.substr(0, secondUnderscore);
}


std::string PakParser::ExtractVagName(const std::string& fileName) {
    std::string baseName = fileName.substr(0, fileName.find('.'));
    return baseName + ".vag";
}

bool PakParser::IsAudioFileExtension(const std::string& fileName) {
    std::string lowerFileName = fileName;
    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

    // Replace ends_with with manual string comparison
    return lowerFileName.size() >= 4 &&
        (lowerFileName.compare(lowerFileName.size() - 4, 4, ".ogg") == 0 ||
            lowerFileName.compare(lowerFileName.size() - 4, 4, ".wav") == 0);
}