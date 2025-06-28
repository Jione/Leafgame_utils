#pragma once

#include "GlobalDefinitions.h"
#include <string>
#include <vector>
#include <atomic> // ��� ���� ������ ����
#include <iostream> // cout
#include <map> // ���� ���� ������ ����
#include <memory> // unique_ptr

// DirectSound ���� ���

#include <dsound.h>
#include <windows.h> // HWND, HRESULT ��

// AudioDecoder Ŭ���� ����
class AudioDecoder;

class AudioManager {
public:
    static AudioManager& GetInstance(); // �̱���

    bool Initialize();
    void Shutdown();

    // PAK ���Ͽ��� ����� �����͸� ���� ������ ��� (�ڵ� ��ȯ)
    // repeat: 0 = ���� �ݺ�, 1 = �� �� ���
    int PlayAudio(const PakFileInfo& fileInfo, int repeat = 1);
    int PlayMultiAudio(const std::vector<PakFileInfo> fileArray, bool isBond, int repeat = 1);
    void StopAudio(int handle); // Ư�� �ڵ��� ����� ����
    void StopAllAudio(); // ��� ����� ����
    void PauseAudio(int handle);
    void ResumeAudio(int handle);

    // ���� ���� (0.0 ~ 1.0)
    void SetVolume(float volume); // ��ü ����
    // void SetVolume(int handle, float volume); // Ư�� ������� ���� (�ʿ��)

    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };
    PlaybackState GetPlaybackState(int handle) const; // Ư�� �ڵ��� ��� ����
    std::string GetLastPlayFileName() const; // ���������� ����� ���ϸ� ���
    // std::string GetCurrentPlayingUniqueId(int handle) const; // Ư�� �ڵ� ������� ID (�ʿ��)


    // ----------------------------------------------------
    // �߰��Ǵ� ����� ��� ���� �Լ���
    // ----------------------------------------------------
    // ���� ����� Current index�� ����ϰ� sub index�� 1 �����ϴ� �Լ� (���� ���)
    void ContinuePlay();

    // sub index�� 1 �����ϰ� ����ϴ� �Լ� (���� ���)
    void PrevPlay();

    // ���ڷ� ���� sub index�� Current sub_idx�� �����ϰ� ����ϴ� �Լ�
    void PlaySpecificSubIndex(bool isBond, int numArgs, ...);

    // ���� ����� Current index�� ����� �ϴ� �Լ� (�ٽ� ���)
    void PlayCurrentAudio(bool isUnique = false);


private:
    AudioManager();
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // DirectSound ���� ��� ����
    HWND handleWindow;
    LPDIRECTSOUND8 m_pDS;             // DirectSound ���� ��ü
    LPDIRECTSOUNDBUFFER m_pDSPrimaryBuffer; // �� ���� (���� ����, �ʿ��)

    // ���� ���� ���� ���۸� �����ϱ� ���� ��
    std::map<int, std::unique_ptr<SoundBufferInfo>> m_soundBuffers;
    int m_nextSoundHandle; // �������� �Ҵ�� ���� �ڵ�

    std::atomic<bool> m_isShuttingDown; // ���� ������ ���� (������ ����)

    std::unique_ptr<AudioDecoder> m_audioDecoder;

    // ���� ��� ���� ������� ������ ���� (��� �簳, ����/���� ����� ����)
    // PakFileInfo�� ���� ������ ����ü���� ��.
    PakFileInfo m_currentPlayingPakInfo;
    std::vector<int> m_uniqueHandle;              // ����ũ �ڵ� ����

    // ���� ���� �Լ�
    HRESULT DirectSoundErrorCheck(HRESULT hr, const std::string& msg);

    // DirectSound ���� ���� �� ������ �ε� ���� (PCM �����͸� �޾Ƽ�)
    bool CreateAndLoadSecondaryBuffer(SoundBufferInfo* sbInfo, const std::vector<int16_t>& pcmData,
        long samplerate, int channels);

    // ������ �Լ� (static���� ����)
    static DWORD WINAPI SoundLoop(LPVOID param);
    // static DWORD WINAPI FadeOutLoop(LPVOID param); // ���̵�ƿ� ��� �ʿ��
};