#pragma once

#include "GlobalDefinitions.h"
#include <string>
#include <vector>
#include <cstdint>

// OGG Vorbis 디코딩을 위한 libvorbisfile 헤더
#include <vorbis/vorbisfile.h>

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    // outPCMData: 디코딩된 PCM 데이터 (16비트 Signed Integer)
    // outSamplerate: 샘플레이트 (Hz)
    // outChannels: 채널 수 (1=모노, 2=스테레오)
    // 
    // 메모리 상의 OGG 데이터를 PCM으로 디코딩
    bool DecodeOgg(const uint8_t* inputData, size_t dataSize,
        std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels);

    // 메모리 상의 VAG 데이터를 PCM으로 디코딩
    bool DecodeVag(const uint8_t* inputData, size_t dataSize,
        std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels);

    // 메모리 상의 WAV 데이터를 PCM으로 디코딩
    bool DecodeWav(const uint8_t* inputData, size_t dataSize,
        std::vector<int16_t>& outPCMData, long& outSamplerate, int& outChannels);

private:
    // OGG 파일용 콜백 함수 (libvorbisfile 사용)
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