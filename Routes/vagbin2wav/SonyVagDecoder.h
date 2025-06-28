#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace SonyVag {

    // �޸� ���� VAG ���(48����Ʈ)���� ���÷���Ʈ�� �����մϴ�.
    // data���� VAG ���� ��ü(���+������)�� ��� �־�� �մϴ�.
    bool ParseHeader(const std::vector<uint8_t>& data, int& outSampleRate);

    // VAG ���̷ε�(��� 48����Ʈ ����)�� �����ؼ� PCM 16bit ������ ��ȯ�մϴ�.
    std::vector<int16_t> Decode(const std::vector<uint8_t>& vagPayload);

    // fmt ûũ�� Ȯ�� �κ� ����
    struct WavFormatExtra {
        uint16_t cbSize;           // = 2
        uint16_t samplesPerBlock;  // ��: 505
    };

    // WAV ��� ����ü Ȯ��
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
        uint32_t totalSamples;  // ���� ���� ����
        char     data[4];       // "data"
        uint32_t dataSize;      // ADPCM ������ ����Ʈ ��
    };

    // PCM 16bit ������ WAV(RIFF) ���Ϸ� �����մϴ�.
    bool WriteWav(
        const std::string& outPath,
        int                        sampleRate,
        const std::vector<int16_t>& pcm);

    // PCM 16bit ������ WAV(RIFF) ���Ϸ� �����մϴ�.
    bool WriteWavADPCM(
        const std::string& outPath,
        int                        sampleRate,
        const std::vector<int16_t>& pcm
    );

} // namespace SonyVag