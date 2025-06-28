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
    // 1) 테이블 읽기
    std::vector<BinVag::Entry> entries;
    if (!BinVag::ReadTable(binPath, entries))
        return 1;

    // 2) 출력 디렉터리 = BIN 이름(확장자 제외)
    size_t dot = binPath.rfind('.');
    std::string outDir = (dot == std::string::npos ? binPath : binPath.substr(0, dot));
    if (!BinVag::EnsureDirectory(outDir))
        return 1;

    // 3) 각 레코드 처리
    for (size_t i = 0; i < entries.size(); ++i) {
        // VAG 원본 읽기
        std::vector<uint8_t> vagData;
        if (!BinVag::ReadRawVag(binPath, entries[i], vagData))
            continue;

        // 헤더에서 샘플레이트 뽑기
        int sampleRate = 0;
        if (!SonyVag::ParseHeader(vagData, sampleRate)) {
            std::cerr << "Skip chunk #" << (i + 1) << "\n";
            continue;
        }

        // 페이로드만 잘라서 디코딩
        std::vector<uint8_t> payload(vagData.begin() + 48, vagData.end());
        auto pcm = SonyVag::Decode(payload);

        // 파일명 추출
        std::string baseName = BinVag::ExtractName(vagData);
        std::string outPath = outDir + "\\" + baseName + ".WAV";

        // WAV 쓰기
        if (SonyVag::WriteWav(outPath, sampleRate, pcm)) {
            std::cout << "Saved: " << outPath << "\n";
        }
    }

    std::cout << "All done.\n";
    return 0;
}