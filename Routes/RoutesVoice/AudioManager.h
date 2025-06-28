#pragma once

#include "GlobalDefinitions.h"
#include <string>
#include <vector>
#include <atomic> // 재생 상태 관리를 위해
#include <iostream> // cout
#include <map> // 다중 버퍼 관리를 위해
#include <memory> // unique_ptr

// DirectSound 관련 헤더

#include <dsound.h>
#include <windows.h> // HWND, HRESULT 등

// AudioDecoder 클래스 선언
class AudioDecoder;

class AudioManager {
public:
    static AudioManager& GetInstance(); // 싱글톤

    bool Initialize();
    void Shutdown();

    // PAK 파일에서 오디오 데이터를 직접 가져와 재생 (핸들 반환)
    // repeat: 0 = 무한 반복, 1 = 한 번 재생
    int PlayAudio(const PakFileInfo& fileInfo, int repeat = 1);
    int PlayMultiAudio(const std::vector<PakFileInfo> fileArray, bool isBond, int repeat = 1);
    void StopAudio(int handle); // 특정 핸들의 오디오 정지
    void StopAllAudio(); // 모든 오디오 정지
    void PauseAudio(int handle);
    void ResumeAudio(int handle);

    // 볼륨 설정 (0.0 ~ 1.0)
    void SetVolume(float volume); // 전체 볼륨
    // void SetVolume(int handle, float volume); // 특정 오디오의 볼륨 (필요시)

    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };
    PlaybackState GetPlaybackState(int handle) const; // 특정 핸들의 재생 상태
    std::string GetLastPlayFileName() const; // 마지막으로 재생된 파일명 얻기
    // std::string GetCurrentPlayingUniqueId(int handle) const; // 특정 핸들 오디오의 ID (필요시)


    // ----------------------------------------------------
    // 추가되는 오디오 재생 제어 함수들
    // ----------------------------------------------------
    // 현재 저장된 Current index를 재생하고 sub index를 1 증가하는 함수 (연속 재생)
    void ContinuePlay();

    // sub index를 1 감소하고 재생하는 함수 (이전 재생)
    void PrevPlay();

    // 인자로 받은 sub index를 Current sub_idx로 설정하고 재생하는 함수
    void PlaySpecificSubIndex(bool isBond, int numArgs, ...);

    // 현재 저장된 Current index를 재생만 하는 함수 (다시 재생)
    void PlayCurrentAudio(bool isUnique = false);


private:
    AudioManager();
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // DirectSound 관련 멤버 변수
    HWND handleWindow;
    LPDIRECTSOUND8 m_pDS;             // DirectSound 메인 객체
    LPDIRECTSOUNDBUFFER m_pDSPrimaryBuffer; // 주 버퍼 (선택 사항, 필요시)

    // 여러 개의 사운드 버퍼를 관리하기 위한 맵
    std::map<int, std::unique_ptr<SoundBufferInfo>> m_soundBuffers;
    int m_nextSoundHandle; // 다음으로 할당될 사운드 핸들

    std::atomic<bool> m_isShuttingDown; // 종료 중인지 여부 (스레드 안전)

    std::unique_ptr<AudioDecoder> m_audioDecoder;

    // 현재 재생 중인 오디오의 정보를 추적 (재생 재개, 이전/다음 재생을 위함)
    // PakFileInfo는 복사 가능한 구조체여야 함.
    PakFileInfo m_currentPlayingPakInfo;
    std::vector<int> m_uniqueHandle;              // 유니크 핸들 집합

    // 내부 헬퍼 함수
    HRESULT DirectSoundErrorCheck(HRESULT hr, const std::string& msg);

    // DirectSound 버퍼 생성 및 데이터 로드 헬퍼 (PCM 데이터를 받아서)
    bool CreateAndLoadSecondaryBuffer(SoundBufferInfo* sbInfo, const std::vector<int16_t>& pcmData,
        long samplerate, int channels);

    // 스레드 함수 (static으로 선언)
    static DWORD WINAPI SoundLoop(LPVOID param);
    // static DWORD WINAPI FadeOutLoop(LPVOID param); // 페이드아웃 기능 필요시
};