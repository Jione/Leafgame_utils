#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace BinVag {

    // 16����Ʈ�� �о���̴� ����ü
    struct Entry {
        uint32_t reserved1;
        uint32_t dataOffset;
        uint32_t dataLength;
        uint32_t reserved2;
    };

    // ù ��° ���ڵ��� dataOffset ����ŭ �ݺ��ؼ� ���̺��� �о���Դϴ�.
    bool ReadTable(const std::string& binPath, std::vector<Entry>& outEntries);

    // ���� Entry���� vagData�� ���� �޸𸮿� ����ϴ�.
    bool ReadRawVag(
        const std::string& binPath,
        const Entry& e,
        std::vector<uint8_t>& outVag);

    // vagData ���� 32����Ʈ���� 16����Ʈ�� ���ڿ��� �̾�, Ȯ���� ���� ��ȯ
    std::string ExtractName(const std::vector<uint8_t>& vagData);

    // WinAPI�� ���͸��� �����, �̹� ������ �����մϴ�.
    bool EnsureDirectory(const std::string& dirPath);

} // namespace BinVag