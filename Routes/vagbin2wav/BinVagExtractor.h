#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace BinVag {

    // 16바이트씩 읽어들이는 구조체
    struct Entry {
        uint32_t reserved1;
        uint32_t dataOffset;
        uint32_t dataLength;
        uint32_t reserved2;
    };

    // 첫 번째 레코드의 dataOffset 값만큼 반복해서 테이블을 읽어들입니다.
    bool ReadTable(const std::string& binPath, std::vector<Entry>& outEntries);

    // 개별 Entry에서 vagData를 꺼내 메모리에 담습니다.
    bool ReadRawVag(
        const std::string& binPath,
        const Entry& e,
        std::vector<uint8_t>& outVag);

    // vagData 내부 32바이트부터 16바이트를 문자열로 뽑아, 확장자 없이 반환
    std::string ExtractName(const std::vector<uint8_t>& vagData);

    // WinAPI로 디렉터리를 만들고, 이미 있으면 무시합니다.
    bool EnsureDirectory(const std::string& dirPath);

} // namespace BinVag