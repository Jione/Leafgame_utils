#pragma once

#include "GlobalDefinitions.h"
#include <string>
#include <vector>
#include <cstdint>

// OGG Vorbis ���ڵ��� ���� libvorbisfile ���
#include <vorbis/vorbisfile.h>

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    // outPCMData: ���ڵ��� PCM ������ (16��Ʈ Signed Integer)
    // outSamplerate: ���÷���Ʈ (Hz)
    // outChannels: ä�� �� (1=���, 2=���׷���)
    // 
    // �޸� ���� OGG �����͸� PCM���� ���ڵ�
    bool DecodeOgg(const uint8_t* inputData, size_t dataSize,
        std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels);

    // �޸� ���� VAG �����͸� PCM���� ���ڵ�
    bool DecodeVag(const uint8_t* inputData, size_t dataSize,
        std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels);

    // �޸� ���� WAV �����͸� PCM���� ���ڵ�
    bool DecodeWav(const uint8_t* inputData, size_t dataSize,
        std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels);

private:
    // OGG ���Ͽ� �ݹ� �Լ� (libvorbisfile ���)
    struct OggDataSource {
        const uint8_t* data;
        size_t size;
        size_t current_pos;
    };

    static size_t ov_read_func(void* ptr, size_t size, size_t nmemb, void* datasource);
    static int ov_seek_func(void* datasource, ogg_int64_t offset, int whence);
    static int ov_close_func(void* datasource);
    static long ov_tell_func(void* datasource);
};