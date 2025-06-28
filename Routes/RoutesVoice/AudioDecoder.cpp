#include "AudioDecoder.h"
#include <iostream>
#include <stdexcept>
#include <cstring> // for memcpy

// libvorbisfile 라이브러리 링크
#pragma comment(lib, "libogg.lib")
#pragma comment(lib, "libvorbis.lib")
#pragma comment(lib, "libvorbisfile.lib")

AudioDecoder::AudioDecoder() {}
AudioDecoder::~AudioDecoder() {}

// OGG 디코딩을 위한 콜백 함수들 구현
size_t AudioDecoder::ov_read_func(void* ptr, size_t size, size_t nmemb, void* datasource) {
    OggDataSource* src = static_cast<OggDataSource*>(datasource);
    size_t bytesToRead = size * nmemb;
    if (src->current_pos + bytesToRead > src->size) {
        bytesToRead = src->size - src->current_pos;
    }
    memcpy(ptr, src->data + src->current_pos, bytesToRead);
    src->current_pos += bytesToRead;
    return bytesToRead;
}

int AudioDecoder::ov_seek_func(void* datasource, ogg_int64_t offset, int whence) {
    OggDataSource* src = static_cast<OggDataSource*>(datasource);
    ogg_int64_t new_pos = src->current_pos;

    switch (whence) {
    case SEEK_SET: new_pos = offset; break;
    case SEEK_CUR: new_pos += offset; break;
    case SEEK_END: new_pos = src->size + offset; break;
    default: return -1;
    }

    if (new_pos < 0 || new_pos >(ogg_int64_t)src->size) {
        return -1; // Out of bounds
    }
    src->current_pos = new_pos;
    return 0;
}

int AudioDecoder::ov_close_func(void* datasource) {
    return 0;
}

long AudioDecoder::ov_tell_func(void* datasource) {
    OggDataSource* src = static_cast<OggDataSource*>(datasource);
    return (long)src->current_pos;
}

bool AudioDecoder::DecodeOgg(const uint8_t* inputData, size_t dataSize,
    std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels) {
    outSamplerate = 0;
    outChannels = 0;

    OggDataSource oggSrc = { inputData, dataSize, 0 };

    ov_callbacks callbacks;
    callbacks.read_func = ov_read_func;
    callbacks.seek_func = ov_seek_func;
    callbacks.close_func = ov_close_func;
    callbacks.tell_func = ov_tell_func;

    OggVorbis_File ovf;
    // ov_open_callbacks는 스트림을 열고 초기 헤더를 읽습니다.
    if (ov_open_callbacks(&oggSrc, &ovf, NULL, 0, callbacks) < 0) {
        dprintf("[Error] Fail to open OGG file. (AudioDecoder:libvorbisfile)");
        return false;
    }

    // Vorbis 스트림 정보 가져오기
    vorbis_info* vi = ov_info(&ovf, -1);
    if (!vi) {
        dprintf("[Error] Fail to load OGG Vorbis infomations (AudioDecoder)");
        ov_clear(&ovf); // 자원 해제
        return false;
    }

    outSamplerate = vi->rate;
    outChannels = vi->channels;

    char buffer[4096]; // 디코딩 버퍼
    int bytesRead = 0;
    int current_section; // libvorbisfile 내부 변수

    // 데이터를 16비트 PCM으로 디코딩하여 읽습니다.
    // ov_read의 4번째 인자(endian)는 0(리틀), 1(빅)입니다. Windows는 리틀 엔디언.
    // 5번째 인자(word_size)는 2(16비트).
    // 6번째 인자(signed)는 1(부호 있음).
    while ((bytesRead = ov_read(&ovf, buffer, sizeof(buffer), 0, 2, 1, &current_section)) > 0) {
        // 읽은 PCM 데이터를 출력 벡터에 추가합니다.
        outPCMData.insert(outPCMData.end(), (int16_t*)buffer, (int16_t*)(buffer + bytesRead));
    }

    if (bytesRead < 0) { // 디코딩 오류 발생
        dprintf("[Error] Error decoding OGG. Error code: %d (AudioDecoder)", bytesRead);
        ov_clear(&ovf);
        return false;
    }

    ov_clear(&ovf); // 자원 해제
    return true;
}

bool AudioDecoder::DecodeVag(const uint8_t* inputData, size_t dataSize,
    std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels) {

    // --- 상수 테이블 (PlayStation SPU-ADPCM 계수) ---
    static const double kCoef1[5] = { 0.0, 60.0 / 64, 115.0 / 64, 98.0 / 64, 122.0 / 64 };
    static const double kCoef2[5] = { 0.0,     0.0, -52.0 / 64, -55.0 / 64, -60.0 / 64 };

    // 헤더 크기, 블록 단위 등
    static constexpr int kHeaderSize = 48;
    static constexpr uint8_t kEndFlag = 7;
    //static constexpr double kBiasOffset = 6.755399441055744e15;

    // 헤더 문자가 "VAGp"일 경우에만 디코딩
    outChannels = 0;
    if (dataSize < kHeaderSize) {
        dprintf("[Error] Invalid Data Size. (AudioDecoder:DecodeVag)");
        return false;
    }
    else if (std::memcmp(inputData, "VAGp", 4) == 0) {
        outChannels = 1; // 항상 1채널
    }
    else if (std::memcmp(inputData, "STER", 4) == 0) {
        outChannels = 2; // 항상 2채널 (STEREO)
    }
    else {
        dprintf("[Error] Invalid VAG header. (AudioDecoder:DecodeVag)");
        return false;
    }
    outSamplerate = ((uint32_t(inputData[16]) << 24) | (uint32_t(inputData[17]) << 16) | (uint32_t(inputData[18]) << 8) | (uint32_t(inputData[19]) << 0));
    outPCMData.reserve(outPCMData.size() + (((dataSize - kHeaderSize) / 16) * 7));  // 대략적인 크기를 미리 예약

    double hist1 = 0.0, hist2 = 0.0;
    size_t pos = kHeaderSize;

    if (outChannels == 1) {
        // 모노 채널일 경우
        while (pos + 1 < dataSize) {
            uint8_t headerByte = inputData[pos++];
            int     shift = headerByte & 0x0F;
            int     predict = (headerByte & 0xF0) >> 4;
            uint8_t flag = inputData[pos++];

            // 플래그가 종료 바이트인지 검사
            if (flag == kEndFlag)
                break;

            // 읽어들일 데이터 블록 14바이트가 남아있는지 확인
            if (pos + 14 > dataSize)
                break;

            // 4bit 단위로 나눠서 signed nibble 배열로 변환
            int samples[28];
            for (int i = 0; i < 14; ++i) {
                uint8_t b = inputData[pos++];
                samples[i * 2] = (b & 0x0F);
                samples[i * 2 + 1] = ((b >> 4) & 0x0F);
            }

            // VAG 디코딩
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
                outPCMData.push_back(static_cast<int16_t>(out));
            }
        }
    }
    else {
        // 스테레오 채널일 경우
        double hist3 = 0.0, hist4 = 0.0;
        size_t pos2 = pos + 0x10;

        while (pos + 1 < dataSize) {
            uint8_t headerByte = inputData[pos++];
            int     shift = headerByte & 0x0F;
            int     predict = (headerByte & 0xF0) >> 4;
            uint8_t flag = inputData[pos++];
            // 플래그가 종료 바이트인지 검사
            if (flag == kEndFlag)
                break;
            // 읽어들일 데이터 블록 14바이트가 남아있는지 확인
            if (pos + 14 > dataSize)
                break;

            headerByte = inputData[pos2++];
            int     shift2 = headerByte & 0x0F;
            int     predict2 = (headerByte & 0xF0) >> 4;
            flag = inputData[pos2++];
            // 플래그가 종료 바이트인지 검사
            if (flag == kEndFlag)
                break;
            // 읽어들일 데이터 블록 14바이트가 남아있는지 확인
            if (pos2 + 14 > dataSize)
                break;

            // 4bit 단위로 나눠서 signed nibble 배열로 변환
            int samples[28], samples2[28];
            for (int i = 0; i < 14; ++i) {
                uint8_t b = inputData[pos++];
                samples[i * 2] = (b & 0x0F);
                samples[i * 2 + 1] = ((b >> 4) & 0x0F);
                b = inputData[pos2++];
                samples2[i * 2] = (b & 0x0F);
                samples2[i * 2 + 1] = ((b >> 4) & 0x0F);
            }

            // VAG 디코딩
            for (int i = 0; i < 28; ++i) {
                int j = samples[i] << 12;
                if ((j & 0x8000) != 0) {
                    j = (int)(j | 0xFFFF0000);
                }
                if (4 < predict) {
                    predict = 4;
                }

                double d = (j >> shift) + hist1 * kCoef1[predict] + hist2 * kCoef2[predict];
                hist2 = hist1;
                hist1 = d;

                j = samples2[i] << 12;
                if ((j & 0x8000) != 0) {
                    j = (int)(j | 0xFFFF0000);
                }
                if (4 < predict2) {
                    predict2 = 4;
                }
                d = (j >> shift2) + hist3 * kCoef1[predict2] + hist4 * kCoef2[predict2];
                hist4 = hist3;
                hist3 = d;

                // 16bit clamp
                int32_t out = static_cast<int32_t>(hist1);
                if (out > 32767) out = 32767;
                else if (out < -32768) out = -32768;
                outPCMData.push_back(static_cast<int16_t>(out));

                out = static_cast<int32_t>(hist3);
                if (out > 32767) out = 32767;
                else if (out < -32768) out = -32768;
                outPCMData.push_back(static_cast<int16_t>(out));
            }

            pos += 0x10;
            pos2 += 0x10;
        }
    }

    return true;
}

// 헬퍼 함수: 4바이트 문자열을 uint32_t로 변환 (Big-endian to Little-endian for magic numbers)
inline uint32_t FourCCToUint32(const char* s) {
    return static_cast<uint32_t>(s[0]) |
        (static_cast<uint32_t>(s[1]) << 8) |
        (static_cast<uint32_t>(s[2]) << 16) |
        (static_cast<uint32_t>(s[3]) << 24);
}

bool AudioDecoder::DecodeWav(const uint8_t* inputData, size_t dataSize,
    std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels) {
    outSamplerate = 0;
    outChannels = 0;

    if (!inputData || dataSize < sizeof(RiffHeader)) {
        dprintf("[Error] Input data is null or too small for RIFF header. (AudioDecoder:DecodeWav)");
        return false;
    }

    const uint8_t* currentPtr = inputData;
    size_t bytesRemaining = dataSize;

    // 1. RIFF 헤더 파싱
    const RiffHeader* riffHeader = reinterpret_cast<const RiffHeader*>(currentPtr);

    // 시그니처 확인 (리틀 엔디안으로 저장된 ASCII 문자열)
    if (riffHeader->chunkId != FourCCToUint32("RIFF") || riffHeader->format != FourCCToUint32("WAVE")) {
        dprintf("[Error] Invalid RIFF or WAVE header signature. (AudioDecoder:DecodeWav)");
        return false;
    }

    currentPtr += sizeof(RiffHeader);
    bytesRemaining -= sizeof(RiffHeader);

    // 2. 청크 탐색 (fmt 청크와 data 청크)
    const FmtChunk* fmtChunk = nullptr;
    const DataChunk* dataChunk = nullptr;

    while (bytesRemaining > 0) {
        if (bytesRemaining < sizeof(uint32_t) * 2) { // 최소한 청크 ID와 크기를 읽을 수 있어야 함
            dprintf("[Error] Incomplete chunk header. (AudioDecoder:DecodeWav)");
            return false;
        }

        uint32_t chunkId = *reinterpret_cast<const uint32_t*>(currentPtr);
        uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(currentPtr + 4);

        if (bytesRemaining < sizeof(uint32_t) * 2 + chunkSize) { // 청크 데이터까지 충분한지 확인
            dprintf("[Error] Chunk size exceeds remaining data. (AudioDecoder:DecodeWav)");
            return false;
        }

        if (chunkId == FourCCToUint32("fmt ")) {
            if (chunkSize < sizeof(FmtChunk) - sizeof(uint32_t) * 2) { // fmt 청크 최소 크기 확인
                dprintf("[Error] Invalid fmt chunk size. (AudioDecoder:DecodeWav)");
                return false;
            }
            fmtChunk = reinterpret_cast<const FmtChunk*>(currentPtr);
        }
        else if (chunkId == FourCCToUint32("data")) {
            dataChunk = reinterpret_cast<const DataChunk*>(currentPtr);
        }

        // 다음 청크로 이동 (청크 헤더 8바이트 + 청크 데이터 크기)
        currentPtr += (sizeof(uint32_t) * 2 + chunkSize);
        bytesRemaining -= (sizeof(uint32_t) * 2 + chunkSize);
    }

    if (!fmtChunk) {
        dprintf("[Error] fmt chunk not found. (AudioDecoder:DecodeWav)");
        return false;
    }
    if (!dataChunk) {
        dprintf("[Error] data chunk not found. (AudioDecoder:DecodeWav)");
        return false;
    }

    // 3. fmt 청크에서 정보 추출
    if (fmtChunk->audioFormat != 1) { // 1 = WAVE_FORMAT_PCM
        dprintf("[Error] Unsupported audio format: %d (not PCM) (AudioDecoder:DecodeWav)", fmtChunk->audioFormat);
        return false;
    }
    if (fmtChunk->bitsPerSample != 8 && fmtChunk->bitsPerSample != 16) { // 8 또는 16비트 PCM만 지원
        dprintf("[Error] Unsupported bits per sample: %d (only 8 or 16-bit PCM supported) (AudioDecoder:DecodeWav)", fmtChunk->bitsPerSample);
        return false;
    }
    if (fmtChunk->numChannels == 0) {
        dprintf("[Error] Invalid number of channels (0).");
        return false;
    }

    outSamplerate = fmtChunk->sampleRate;
    outChannels = fmtChunk->numChannels;

    // 4. data 청크에서 PCM 데이터 추출
    // dataChunk는 청크 ID와 청크 크기부터 시작하므로, 실제 데이터는 그 뒤에 있습니다.
    const uint8_t* pcmDataPtr = reinterpret_cast<const uint8_t*>(dataChunk) + sizeof(DataChunk);
    size_t pcmDataSize = dataChunk->chunkSize;

    // 예상되는 샘플 개수
    size_t bytesPerSample = fmtChunk->bitsPerSample / 8;
    size_t totalSamples = pcmDataSize / bytesPerSample;

    // outPCMData는 int16_t 벡터이므로, 16비트 샘플을 가정합니다.
    // 8비트 PCM은 부호 없는 값(0-255)을 가지므로 16비트 부호 있는 값(-32768 ~ 32767)으로 변환해야 합니다.
    // 16비트 PCM은 이미 int16_t와 호환됩니다.
    size_t pcmSize = outPCMData.size();
    outPCMData.reserve(pcmSize + totalSamples); // 대략적인 크기를 미리 예약

    if (fmtChunk->bitsPerSample == 8) {
        for (size_t i = 0; i < pcmDataSize; ++i) {
            // 8비트 unsigned PCM을 16비트 signed PCM으로 변환
            // 0 ~ 255 -> -128 ~ 127 로 오프셋 조정 후 256배 (8비트 -> 16비트)
            // (byte_val - 128) * 256
            outPCMData.push_back(static_cast<int16_t>((static_cast<int>(pcmDataPtr[i]) - 128) * 256));
        }
    }
    else if (fmtChunk->bitsPerSample == 16) {
        // 16비트 PCM 데이터는 이미 int16_t 형식
        // 데이터를 직접 복사
        size_t numInt16Samples = pcmDataSize / sizeof(int16_t);
        outPCMData.resize(pcmSize + numInt16Samples);
        memcpy(&outPCMData.data()[pcmSize], pcmDataPtr, pcmDataSize);
    }
    // 24비트나 32비트 PCM은 현재 처리하지 않음

    return true;
}
