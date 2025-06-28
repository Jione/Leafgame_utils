#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace SonyVag {

    // 메모리 상의 VAG 헤더(48바이트)에서 샘플레이트를 추출합니다.
    // data에는 VAG 파일 전체(헤더+데이터)가 들어 있어야 합니다.
    bool ParseHeader(const std::vector<uint8_t>& data, int& outSampleRate);

    // VAG 페이로드(헤더 48바이트 이후)만 전달해서 PCM 16bit 샘플을 반환합니다.
    std::vector<int16_t> Decode(const std::vector<uint8_t>& vagPayload);

    // fmt 청크의 확장 부분 구조
    struct WavFormatExtra {
        uint16_t cbSize;           // = 2
        uint16_t samplesPerBlock;  // 예: 505
    };

    // WAV 헤더 구조체 확장
    struct WavHeaderADPCM {
        char     riff[4];
        uint32_t chunkSize;
        char     wave[4];
        char     fmt[4];
        uint32_t sub1Size;      // = 20 (16 + cbSize)
        uint16_t audioFmt;      // = 0x11
        uint16_t channels;      // = 1
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample; // = 4
        WavFormatExtra extra;   // cbSize + samplesPerBlock
        char     fact[4];       // "fact"
        uint32_t factSize;      // = 4
        uint32_t totalSamples;  // 실제 샘플 개수
        char     data[4];       // "data"
        uint32_t dataSize;      // ADPCM 데이터 바이트 수
    };

    // PCM 16bit 샘플을 WAV(RIFF) 파일로 저장합니다.
    bool WriteWav(
        const std::string& outPath,
        int                        sampleRate,
        const std::vector<int16_t>& pcm);

    // PCM 16bit 샘플을 WAV(RIFF) 파일로 저장합니다.
    bool WriteWavADPCM(
        const std::string& outPath,
        int                        sampleRate,
        const std::vector<int16_t>& pcm
    );

} // namespace SonyVag