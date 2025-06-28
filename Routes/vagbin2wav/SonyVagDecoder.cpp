#include "SonyVagDecoder.h"
#include <cstring>
#include <fstream>
#include <iostream>
//#include <windows.h>  // for DWORD

namespace SonyVag {

    // --- 상수 테이블 (PlayStation SPU-ADPCM 계수) ---
    static const double kCoef1[5] = { 0.0, 60.0 / 64, 115.0 / 64, 98.0 / 64, 122.0 / 64 };
    static const double kCoef2[5] = { 0.0,     0.0, -52.0 / 64, -55.0 / 64, -60.0 / 64 };

    // 헤더 크기, 블록 단위 등
    static constexpr int kHeaderSize = 48;
    static constexpr uint8_t kEndFlag = 7;
    //static constexpr double kBiasOffset = 6.755399441055744e15;

    // 메모리 상에서 BE32 읽기
    static uint32_t ReadBE32(const uint8_t* p) {
        return (uint32_t(p[0]) << 24)
            | (uint32_t(p[1]) << 16)
            | (uint32_t(p[2]) << 8)
            | (uint32_t(p[3]) << 0);
    }

    bool ParseHeader(const std::vector<uint8_t>& data, int& outSampleRate) {
        if (data.size() < kHeaderSize ||
            std::memcmp(data.data(), "VAGp", 4) != 0)
        {
            std::cerr << "ERROR: Invalid VAG header\n";
            return false;
        }
        outSampleRate = static_cast<int>(ReadBE32(data.data() + 16));
        return true;
    }

    std::vector<int16_t> Decode(const std::vector<uint8_t>& vagPayload) {
        std::vector<int16_t> pcm;
        pcm.reserve(vagPayload.size());  // 대략

        double hist1 = 0.0, hist2 = 0.0;
        size_t pos = 0, len = vagPayload.size();

        while (pos + 1 < len) {
            uint8_t headerByte = vagPayload[pos++];
            int     shift = headerByte & 0x0F;
            int     predict = (headerByte & 0xF0) >> 4;
            uint8_t flag = vagPayload[pos++];

            if (flag == kEndFlag)
                break;

            // 14바이트 블록
            if (pos + 14 > len)
                break;

            // 4bit→signed nibble 배열
            int samples[28];
            for (int i = 0; i < 14; ++i) {
                uint8_t b = vagPayload[pos++];
                samples[i * 2] = (b & 0x0F);
                samples[i * 2 + 1] = ((b >> 4) & 0x0F);
            }

            // 디코딩
            for (int i = 0; i < 28; ++i) {
                int j = samples[i] << 12;
                if ((j & 0x8000) != 0)
                {
                    j = (int)(j | 0xFFFF0000);
                }
                if (4 < predict) {
                    predict = 4;
                }

                double d = (j >> shift)
                            //+ kBiasOffset
                            + hist1 * kCoef1[predict]
                            + hist2 * kCoef2[predict];
                        hist2 = hist1;
                        hist1 = d;

                        // 16bit clamp
                        int32_t out = static_cast<int32_t>(d);
                        if (out > 32767) out = 32767;
                        else if (out < -32768) out = -32768;
                        pcm.push_back(static_cast<int16_t>(out));
            }
        }

        return pcm;
    }

    // IMA ADPCM 표준 테이블
    static const int indexTable[16] = {
       -1, -1, -1, -1, 2, 4, 6, 8,
       -1, -1, -1, -1, 2, 4, 6, 8
    };
    // IMA ADPCM 표준 스텝 사이즈 테이블 (총 89개)
    static const int stepSizeTable[89] = {
         7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
        19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
        50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
       130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
       337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
       876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
      2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
      5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
     15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
    };

    // little-endian 32비트 쓰기
    static void writeLE32(std::ofstream& ofs, uint32_t v) {
        ofs.put(static_cast<char>(v & 0xFF));
        ofs.put(static_cast<char>((v >> 8) & 0xFF));
        ofs.put(static_cast<char>((v >> 16) & 0xFF));
        ofs.put(static_cast<char>((v >> 24) & 0xFF));
    }

    // little-endian 16비트 쓰기
    static void writeLE16(std::ofstream& ofs, uint16_t v) {
        ofs.put(static_cast<char>(v & 0xFF));
        ofs.put(static_cast<char>((v >> 8) & 0xFF));
    }

    /// @brief  PCM16 → IMA ADPCM 블록 단위 인코더
    /// @param pcm        입력 16bit PCM 샘플 (mono)
    /// @param adpcmOut   출력 ADPCM 바이트 벡터
    /// @param samplesPerBlock  한 블록에 몇 샘플 묶을지 (보통 505)
    static void EncodeImaAdpcm(
        const std::vector<int16_t>& pcm,
        std::vector<uint8_t>& adpcmOut,
        uint16_t                    samplesPerBlock = 505)
    {
        size_t pos = 0, total = pcm.size();
        adpcmOut.clear();

        int predictor = pcm[0];       // 예측기 초기값 = 첫 PCM
        int index = 0;            // 스텝 인덱스 초기값
        int step = stepSizeTable[index];

        // 블록 헤더: predictor(2B, LE), index(1B), reserved(1B=0)
        while (pos < total) {
            size_t blockSamples = std::min<size_t>(samplesPerBlock, total - pos);
            // 블록 헤더 쓰기
            adpcmOut.push_back(static_cast<uint8_t>(predictor & 0xFF));
            adpcmOut.push_back(static_cast<uint8_t>((predictor >> 8) & 0xFF));
            adpcmOut.push_back(static_cast<uint8_t>(index & 0xFF));
            adpcmOut.push_back(0);

            uint8_t nibbleByte = 0;
            bool    highNibble = true;

            // 샘플 당 4비트 코드 생성
            for (size_t i = 0; i < blockSamples; ++i) {
                int16_t sample = pcm[pos++];
                int diff = sample - predictor;
                int sign = (diff < 0) ? 8 : 0;
                if (diff < 0) diff = -diff;

                // 4bit code 계산
                int delta = 0;
                int temp = step;
                if (diff >= temp) { delta |= 4; diff -= temp; }
                temp = step >> 1;
                if (diff >= temp) { delta |= 2; diff -= temp; }
                temp = step >> 2;
                if (diff >= temp) { delta |= 1; }
                delta |= sign;

                // predictor 업데이트
                int diffq = step >> 3;
                if (delta & 4) diffq += step;
                if (delta & 2) diffq += step >> 1;
                if (delta & 1) diffq += step >> 2;
                predictor += (sign ? -diffq : diffq);
                predictor = std::max(-32768, std::min(32767, predictor));

                // index 업데이트
                index += indexTable[delta];
                index = std::max(0, std::min(88, index));
                step = stepSizeTable[index];

                // output nibble
                if (highNibble) {
                    nibbleByte = static_cast<uint8_t>(delta & 0x0F);
                    highNibble = false;
                }
                else {
                    nibbleByte |= static_cast<uint8_t>((delta & 0x0F) << 4);
                    adpcmOut.push_back(nibbleByte);
                    highNibble = true;
                    nibbleByte = 0;
                }
            }
            // 샘플 수가 홀수면 남은 하이니블 쓰기
            if (!highNibble) {
                adpcmOut.push_back(nibbleByte);
            }
        }
    }

    bool WriteWav(
        const std::string& outPath,
        int                        sampleRate,
        const std::vector<int16_t>& pcm)
    {
        struct WavHdr {
            char     riff[4];
            uint32_t chunkSize;
            char     wave[4];
            char     fmt[4];
            uint32_t sub1Size;
            uint16_t audioFmt;
            uint16_t channels;
            uint32_t sampleRate;
            uint32_t byteRate;
            uint16_t blockAlign;
            uint16_t bitsPerSample;
            char     data[4];
            uint32_t dataSize;
        } hdr;

        std::ofstream ofs(outPath, std::ios::binary);
        if (!ofs) {
            std::cerr << "ERROR: Cannot open output WAV: " << outPath << "\n";
            return false;
        }

        std::memcpy(hdr.riff, "RIFF", 4);
        hdr.dataSize = static_cast<uint32_t>(pcm.size() * sizeof(int16_t));
        hdr.chunkSize = 36 + hdr.dataSize;
        std::memcpy(hdr.wave, "WAVE", 4);
        std::memcpy(hdr.fmt, "fmt ", 4);
        hdr.sub1Size = 16;
        hdr.audioFmt = 1;
        hdr.channels = 1;
        hdr.sampleRate = static_cast<uint32_t>(sampleRate);
        hdr.bitsPerSample = 16;
        hdr.blockAlign = hdr.channels * hdr.bitsPerSample / 8;
        hdr.byteRate = hdr.sampleRate * hdr.blockAlign;
        std::memcpy(hdr.data, "data", 4);

        ofs.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        ofs.write(reinterpret_cast<const char*>(pcm.data()), hdr.dataSize);
        return true;
    }

    /// @brief  Mono IMA ADPCM WAV 생성
    /// @param outPath    출력 경로
    /// @param sampleRate 샘플레이트
    /// @param pcm        16bit PCM 샘플(모노)
    bool WriteWavADPCM(
        const std::string& outPath,
        int                         sampleRate,
        const std::vector<int16_t>& pcm)
    {
        // 1) PCM→ADPCM
        std::vector<uint8_t> adpcm;
        const uint16_t spb = 505;
        EncodeImaAdpcm(pcm, adpcm, spb);

        // 2) 파라미터 계산
        const uint16_t channels = 1;
        const uint16_t bitsSample = 4;
        const uint16_t cbSize = 2;
        const uint32_t dataBytes = (uint32_t)adpcm.size();
        const uint16_t blockAlign = uint16_t((bitsSample * spb) / 8 + 4);
        const uint32_t byteRate = sampleRate * blockAlign / spb * spb;

        const uint32_t fmtSize = 16 + cbSize;
        const uint32_t factSize = 4;
        uint32_t riffSize = 4
            + (8 + fmtSize)
            + (8 + factSize)
            + (8 + dataBytes);

        std::ofstream ofs(outPath, std::ios::binary);
        if (!ofs) {
            std::cerr << "ERROR: open " << outPath << "\n";
            return false;
        }

        // RIFF
        ofs.write("RIFF", 4);
        writeLE32(ofs, riffSize);
        ofs.write("WAVE", 4);

        // fmt 
        ofs.write("fmt ", 4);
        writeLE32(ofs, fmtSize);
        writeLE16(ofs, 0x0011);       // IMA ADPCM
        writeLE16(ofs, channels);
        writeLE32(ofs, sampleRate);
        writeLE32(ofs, byteRate);
        writeLE16(ofs, blockAlign);
        writeLE16(ofs, bitsSample);
        writeLE16(ofs, cbSize);
        writeLE16(ofs, spb);

        // fact
        ofs.write("fact", 4);
        writeLE32(ofs, factSize);
        writeLE32(ofs, static_cast<uint32_t>(pcm.size()));

        // data
        ofs.write("data", 4);
        writeLE32(ofs, dataBytes);
        ofs.write(reinterpret_cast<const char*>(adpcm.data()), dataBytes);

        return true;
    }

} // namespace SonyVag