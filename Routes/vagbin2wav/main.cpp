#include "SonyVagDecoder.h"
#include "BinVagExtractor.h"
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: vagbin2wav <input.bin>\n";
        return 1;
    }

    std::string binPath = argv[1];
    // 1) ���̺� �б�
    std::vector<BinVag::Entry> entries;
    if (!BinVag::ReadTable(binPath, entries))
        return 1;

    // 2) ��� ���͸� = BIN �̸�(Ȯ���� ����)
    size_t dot = binPath.rfind('.');
    std::string outDir = (dot == std::string::npos ? binPath : binPath.substr(0, dot));
    if (!BinVag::EnsureDirectory(outDir))
        return 1;

    // 3) �� ���ڵ� ó��
    for (size_t i = 0; i < entries.size(); ++i) {
        // VAG ���� �б�
        std::vector<uint8_t> vagData;
        if (!BinVag::ReadRawVag(binPath, entries[i], vagData))
            continue;

        // ������� ���÷���Ʈ �̱�
        int sampleRate = 0;
        if (!SonyVag::ParseHeader(vagData, sampleRate)) {
            std::cerr << "Skip chunk #" << (i + 1) << "\n";
            continue;
        }

        // ���̷ε常 �߶� ���ڵ�
        std::vector<uint8_t> payload(vagData.begin() + 48, vagData.end());
        auto pcm = SonyVag::Decode(payload);

        // ���ϸ� ����
        std::string baseName = BinVag::ExtractName(vagData);
        std::string outPath = outDir + "\\" + baseName + ".WAV";

        // WAV ����
        if (SonyVag::WriteWav(outPath, sampleRate, pcm)) {
            std::cout << "Saved: " << outPath << "\n";
        }
    }

    std::cout << "All done.\n";
    return 0;
}