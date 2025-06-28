#include "BinVagExtractor.h"
#include <fstream>
#include <iostream>
#include <windows.h>    // CreateDirectoryA, GetFileAttributesA
#include <algorithm>

namespace BinVag {

    bool ReadTable(
        const std::string& binPath,
        std::vector<Entry>& outEntries)
    {
        std::ifstream ifs(binPath, std::ios::binary);
        if (!ifs) {
            std::cerr << "ERROR: Cannot open BIN: " << binPath << "\n";
            return false;
        }

        // 첫 레코드 읽기
        Entry first{};
        ifs.read(reinterpret_cast<char*>(&first.reserved1), 4);
        ifs.read(reinterpret_cast<char*>(&first.dataOffset), 4);
        ifs.read(reinterpret_cast<char*>(&first.dataLength), 4);
        ifs.read(reinterpret_cast<char*>(&first.reserved2), 4);

        if (!ifs) {
            std::cerr << "ERROR: Cannot read first entry\n";
            return false;
        }
        outEntries.push_back(first);

        // table 끝은 첫 dataOffset까지
        std::streampos tableEnd = static_cast<std::streampos>(first.dataOffset);
        while (ifs.tellg() < tableEnd) {
            Entry e{};
            ifs.read(reinterpret_cast<char*>(&e.reserved1), 4);
            ifs.read(reinterpret_cast<char*>(&e.dataOffset), 4);
            ifs.read(reinterpret_cast<char*>(&e.dataLength), 4);
            ifs.read(reinterpret_cast<char*>(&e.reserved2), 4);
            if (!ifs) break;
            outEntries.push_back(e);
        }

        return !outEntries.empty();
    }

    bool ReadRawVag(
        const std::string& binPath,
        const Entry& e,
        std::vector<uint8_t>& outVag)
    {
        std::ifstream ifs(binPath, std::ios::binary);
        if (!ifs) {
            std::cerr << "ERROR: Cannot reopen BIN: " << binPath << "\n";
            return false;
        }
        ifs.seekg(e.dataOffset, std::ios::beg);
        if (!ifs) {
            std::cerr << "ERROR: Seek to " << e.dataOffset << "\n";
            return false;
        }

        outVag.resize(e.dataLength);
        ifs.read(reinterpret_cast<char*>(outVag.data()), e.dataLength);
        if (!ifs) {
            std::cerr << "ERROR: Read vag chunk\n";
            return false;
        }
        return true;
    }

    std::string ExtractName(const std::vector<uint8_t>& vagData) {
        const size_t nameOff = 32, nameLen = 16;
        if (vagData.size() < nameOff + nameLen)
            return "unknown";

        // raw 16바이트 → string
        std::string s(reinterpret_cast<const char*>(vagData.data() + nameOff), nameLen);
        // NULL 이후 자르는 법
        size_t z = s.find('\0');
        if (z != std::string::npos) s.resize(z);

        // 마지막 점('.') 위치
        size_t p = s.rfind('.');
        if (p != std::string::npos)
            s.resize(p);

        return s;
    }

    bool EnsureDirectory(const std::string& dirPath) {
        DWORD attr = GetFileAttributesA(dirPath.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            if (!CreateDirectoryA(dirPath.c_str(), NULL)) {
                std::cerr << "ERROR: Cannot create dir: " << dirPath << "\n";
                return false;
            }
        }
        return true;
    }

} // namespace BinVag