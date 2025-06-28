#include "AudioDecoder.h"
#include <iostream>
#include <stdexcept>
#include <cstring> // for memcpy

// libvorbisfile ���̺귯�� ��ũ
#pragma comment(lib, "libogg.lib")
#pragma comment(lib, "libvorbis.lib")
#pragma comment(lib, "libvorbisfile.lib")

AudioDecoder::AudioDecoder() {}
AudioDecoder::~AudioDecoder() {}

// OGG ���ڵ��� ���� �ݹ� �Լ��� ����
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
    // ov_open_callbacks�� ��Ʈ���� ���� �ʱ� ����� �н��ϴ�.
    if (ov_open_callbacks(&oggSrc, &ovf, NULL, 0, callbacks) < 0) {
        dprintf("[Error] Fail to open OGG file. (AudioDecoder:libvorbisfile)");
        return false;
    }

    // Vorbis ��Ʈ�� ���� ��������
    vorbis_info* vi = ov_info(&ovf, -1);
    if (!vi) {
        dprintf("[Error] Fail to load OGG Vorbis infomations (AudioDecoder)");
        ov_clear(&ovf); // �ڿ� ����
        return false;
    }

    outSamplerate = vi->rate;
    outChannels = vi->channels;

    char buffer[4096]; // ���ڵ� ����
    int bytesRead = 0;
    int current_section; // libvorbisfile ���� ����

    // �����͸� 16��Ʈ PCM���� ���ڵ��Ͽ� �н��ϴ�.
    // ov_read�� 4��° ����(endian)�� 0(��Ʋ), 1(��)�Դϴ�. Windows�� ��Ʋ �����.
    // 5��° ����(word_size)�� 2(16��Ʈ).
    // 6��° ����(signed)�� 1(��ȣ ����).
    while ((bytesRead = ov_read(&ovf, buffer, sizeof(buffer), 0, 2, 1, &current_section)) > 0) {
        // ���� PCM �����͸� ��� ���Ϳ� �߰��մϴ�.
        outPCMData.insert(outPCMData.end(), (int16_t*)buffer, (int16_t*)(buffer + bytesRead));
    }

    if (bytesRead < 0) { // ���ڵ� ���� �߻�
        dprintf("[Error] Error decoding OGG. Error code: %d (AudioDecoder)", bytesRead);
        ov_clear(&ovf);
        return false;
    }

    ov_clear(&ovf); // �ڿ� ����
    return true;
}

bool AudioDecoder::DecodeVag(const uint8_t* inputData, size_t dataSize,
    std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels) {

    // --- ��� ���̺� (PlayStation SPU-ADPCM ���) ---
    static const double kCoef1[5] = { 0.0, 60.0 / 64, 115.0 / 64, 98.0 / 64, 122.0 / 64 };
    static const double kCoef2[5] = { 0.0,     0.0, -52.0 / 64, -55.0 / 64, -60.0 / 64 };

    // ��� ũ��, ��� ���� ��
    static constexpr int kHeaderSize = 48;
    static constexpr uint8_t kEndFlag = 7;
    //static constexpr double kBiasOffset = 6.755399441055744e15;

    // ��� ���ڰ� "VAGp"�� ��쿡�� ���ڵ�
    outChannels = 0;
    if (dataSize < kHeaderSize) {
        dprintf("[Error] Invalid Data Size. (AudioDecoder:DecodeVag)");
        return false;
    }
    else if (std::memcmp(inputData, "VAGp", 4) == 0) {
        outChannels = 1; // �׻� 1ä��
    }
    else if (std::memcmp(inputData, "STER", 4) == 0) {
        outChannels = 2; // �׻� 2ä�� (STEREO)
    }
    else {
        dprintf("[Error] Invalid VAG header. (AudioDecoder:DecodeVag)");
        return false;
    }
    outSamplerate = ((uint32_t(inputData[16]) << 24) | (uint32_t(inputData[17]) << 16) | (uint32_t(inputData[18]) << 8) | (uint32_t(inputData[19]) << 0));
    outPCMData.reserve(outPCMData.size() + (((dataSize - kHeaderSize) / 16) * 7));  // �뷫���� ũ�⸦ �̸� ����

    double hist1 = 0.0, hist2 = 0.0;
    size_t pos = kHeaderSize;

    if (outChannels == 1) {
        // ��� ä���� ���
        while (pos + 1 < dataSize) {
            uint8_t headerByte = inputData[pos++];
            int     shift = headerByte & 0x0F;
            int     predict = (headerByte & 0xF0) >> 4;
            uint8_t flag = inputData[pos++];

            // �÷��װ� ���� ����Ʈ���� �˻�
            if (flag == kEndFlag)
                break;

            // �о���� ������ ��� 14����Ʈ�� �����ִ��� Ȯ��
            if (pos + 14 > dataSize)
                break;

            // 4bit ������ ������ signed nibble �迭�� ��ȯ
            int samples[28];
            for (int i = 0; i < 14; ++i) {
                uint8_t b = inputData[pos++];
                samples[i * 2] = (b & 0x0F);
                samples[i * 2 + 1] = ((b >> 4) & 0x0F);
            }

            // VAG ���ڵ�
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
        // ���׷��� ä���� ���
        double hist3 = 0.0, hist4 = 0.0;
        size_t pos2 = pos + 0x10;

        while (pos + 1 < dataSize) {
            uint8_t headerByte = inputData[pos++];
            int     shift = headerByte & 0x0F;
            int     predict = (headerByte & 0xF0) >> 4;
            uint8_t flag = inputData[pos++];
            // �÷��װ� ���� ����Ʈ���� �˻�
            if (flag == kEndFlag)
                break;
            // �о���� ������ ��� 14����Ʈ�� �����ִ��� Ȯ��
            if (pos + 14 > dataSize)
                break;

            headerByte = inputData[pos2++];
            int     shift2 = headerByte & 0x0F;
            int     predict2 = (headerByte & 0xF0) >> 4;
            flag = inputData[pos2++];
            // �÷��װ� ���� ����Ʈ���� �˻�
            if (flag == kEndFlag)
                break;
            // �о���� ������ ��� 14����Ʈ�� �����ִ��� Ȯ��
            if (pos2 + 14 > dataSize)
                break;

            // 4bit ������ ������ signed nibble �迭�� ��ȯ
            int samples[28], samples2[28];
            for (int i = 0; i < 14; ++i) {
                uint8_t b = inputData[pos++];
                samples[i * 2] = (b & 0x0F);
                samples[i * 2 + 1] = ((b >> 4) & 0x0F);
                b = inputData[pos2++];
                samples2[i * 2] = (b & 0x0F);
                samples2[i * 2 + 1] = ((b >> 4) & 0x0F);
            }

            // VAG ���ڵ�
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

// ���� �Լ�: 4����Ʈ ���ڿ��� uint32_t�� ��ȯ (Big-endian to Little-endian for magic numbers)
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

    // 1. RIFF ��� �Ľ�
    const RiffHeader* riffHeader = reinterpret_cast<const RiffHeader*>(currentPtr);

    // �ñ״�ó Ȯ�� (��Ʋ ��������� ����� ASCII ���ڿ�)
    if (riffHeader->chunkId != FourCCToUint32("RIFF") || riffHeader->format != FourCCToUint32("WAVE")) {
        dprintf("[Error] Invalid RIFF or WAVE header signature. (AudioDecoder:DecodeWav)");
        return false;
    }

    currentPtr += sizeof(RiffHeader);
    bytesRemaining -= sizeof(RiffHeader);

    // 2. ûũ Ž�� (fmt ûũ�� data ûũ)
    const FmtChunk* fmtChunk = nullptr;
    const DataChunk* dataChunk = nullptr;

    while (bytesRemaining > 0) {
        if (bytesRemaining < sizeof(uint32_t) * 2) { // �ּ��� ûũ ID�� ũ�⸦ ���� �� �־�� ��
            dprintf("[Error] Incomplete chunk header. (AudioDecoder:DecodeWav)");
            return false;
        }

        uint32_t chunkId = *reinterpret_cast<const uint32_t*>(currentPtr);
        uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(currentPtr + 4);

        if (bytesRemaining < sizeof(uint32_t) * 2 + chunkSize) { // ûũ �����ͱ��� ������� Ȯ��
            dprintf("[Error] Chunk size exceeds remaining data. (AudioDecoder:DecodeWav)");
            return false;
        }

        if (chunkId == FourCCToUint32("fmt ")) {
            if (chunkSize < sizeof(FmtChunk) - sizeof(uint32_t) * 2) { // fmt ûũ �ּ� ũ�� Ȯ��
                dprintf("[Error] Invalid fmt chunk size. (AudioDecoder:DecodeWav)");
                return false;
            }
            fmtChunk = reinterpret_cast<const FmtChunk*>(currentPtr);
        }
        else if (chunkId == FourCCToUint32("data")) {
            dataChunk = reinterpret_cast<const DataChunk*>(currentPtr);
        }

        // ���� ûũ�� �̵� (ûũ ��� 8����Ʈ + ûũ ������ ũ��)
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

    // 3. fmt ûũ���� ���� ����
    if (fmtChunk->audioFormat != 1) { // 1 = WAVE_FORMAT_PCM
        dprintf("[Error] Unsupported audio format: %d (not PCM) (AudioDecoder:DecodeWav)", fmtChunk->audioFormat);
        return false;
    }
    if (fmtChunk->bitsPerSample != 8 && fmtChunk->bitsPerSample != 16) { // 8 �Ǵ� 16��Ʈ PCM�� ����
        dprintf("[Error] Unsupported bits per sample: %d (only 8 or 16-bit PCM supported) (AudioDecoder:DecodeWav)", fmtChunk->bitsPerSample);
        return false;
    }
    if (fmtChunk->numChannels == 0) {
        dprintf("[Error] Invalid number of channels (0).");
        return false;
    }

    outSamplerate = fmtChunk->sampleRate;
    outChannels = fmtChunk->numChannels;

    // 4. data ûũ���� PCM ������ ����
    // dataChunk�� ûũ ID�� ûũ ũ����� �����ϹǷ�, ���� �����ʹ� �� �ڿ� �ֽ��ϴ�.
    const uint8_t* pcmDataPtr = reinterpret_cast<const uint8_t*>(dataChunk) + sizeof(DataChunk);
    size_t pcmDataSize = dataChunk->chunkSize;

    // ����Ǵ� ���� ����
    size_t bytesPerSample = fmtChunk->bitsPerSample / 8;
    size_t totalSamples = pcmDataSize / bytesPerSample;

    // outPCMData�� int16_t �����̹Ƿ�, 16��Ʈ ������ �����մϴ�.
    // 8��Ʈ PCM�� ��ȣ ���� ��(0-255)�� �����Ƿ� 16��Ʈ ��ȣ �ִ� ��(-32768 ~ 32767)���� ��ȯ�ؾ� �մϴ�.
    // 16��Ʈ PCM�� �̹� int16_t�� ȣȯ�˴ϴ�.
    size_t pcmSize = outPCMData.size();
    outPCMData.reserve(pcmSize + totalSamples); // �뷫���� ũ�⸦ �̸� ����

    if (fmtChunk->bitsPerSample == 8) {
        for (size_t i = 0; i < pcmDataSize; ++i) {
            // 8��Ʈ unsigned PCM�� 16��Ʈ signed PCM���� ��ȯ
            // 0 ~ 255 -> -128 ~ 127 �� ������ ���� �� 256�� (8��Ʈ -> 16��Ʈ)
            // (byte_val - 128) * 256
            outPCMData.push_back(static_cast<int16_t>((static_cast<int>(pcmDataPtr[i]) - 128) * 256));
        }
    }
    else if (fmtChunk->bitsPerSample == 16) {
        // 16��Ʈ PCM �����ʹ� �̹� int16_t ����
        // �����͸� ���� ����
        size_t numInt16Samples = pcmDataSize / sizeof(int16_t);
        outPCMData.resize(pcmSize + numInt16Samples);
        memcpy(&outPCMData.data()[pcmSize], pcmDataPtr, pcmDataSize);
    }
    // 24��Ʈ�� 32��Ʈ PCM�� ���� ó������ ����

    return true;
}
