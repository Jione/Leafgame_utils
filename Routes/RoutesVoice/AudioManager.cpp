#include "AudioManager.h"
#include "AudioDecoder.h"
#include "DataManager.h" // DataManager 포함
#include <iostream>
#include <sstream>
#include <algorithm> // std::transform
#include <cmath>     // std::log10
#include <exception> // For std::stoi exceptions

// DirectSound 라이브러리 링크
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")

#undef max

// 전역 (또는 AudioManager 멤버) 볼륨 변수
// soundDS.cpp에서 가져온 bgmVolume, voiceVolume, seVolume처럼
// AudioManager에서 통합적으로 관리하는 것이 좋습니다.
// 여기서는 일단 기존처럼 bgmVolume, voiceVolume 사용 예시로 둡니다.
extern int bgmVolume; // soundDS.cpp에서 정의된 전역 변수라고 가정
extern int voiceVolume;

AudioManager::AudioManager()
    : m_pDS(nullptr), m_pDSPrimaryBuffer(nullptr),
    m_nextSoundHandle(1),
    m_isShuttingDown(false),
    m_audioDecoder(std::make_unique<AudioDecoder>()),
    handleWindow(nullptr)
{
    m_uniqueHandle.clear();
}

AudioManager::~AudioManager() {
    Shutdown();
}

AudioManager& AudioManager::GetInstance() {
    static AudioManager instance;
    return instance;
}

bool AudioManager::Initialize() {
    HWND hWnd = GetWinHandle();
    HRESULT hr;
    handleWindow = hWnd;
    hr = DirectSoundCreate8(NULL, &m_pDS, NULL);
    if (DirectSoundErrorCheck(hr, "Failed to create DirectSound Object")) return false;

    // GUI 애플리케이션의 메인 윈도우 핸들을 사용
    // DSSCL_NORMAL은 포커스를 잃어도 백그라운드 재생이 가능하게 합니다.
    hr = m_pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
    if (DirectSoundErrorCheck(hr, "Failed to set DirectSound Cooperative Level")) {
        m_pDS->Release();
        m_pDS = nullptr;
        dprintf("[Error] Failed to set DirectSound Cooperative Level. Exit the program. (AudioManager)");
        return false; // 협조 수준 설정 실패 시 치명적 오류로 간주하고 종료
    }

    // 주 버퍼 생성 (선택 사항이지만, DirectSound 초기화의 표준 부분)
    // 주 버퍼는 애플리케이션이 포커스를 잃으면 자동으로 정지되므로,
    // 보조 버퍼는 DSSCL_NORMAL로 설정했을 때 백그라운드 재생이 가능할 수 있습니다.
    // 하지만 Primary Buffer는 보통 독점 모드에서 사용되므로 여기서는 일반적인 방식으로.
    DSBUFFERDESC dsbdPrimary;
    ZeroMemory(&dsbdPrimary, sizeof(DSBUFFERDESC));
    dsbdPrimary.dwSize = sizeof(DSBUFFERDESC);
    dsbdPrimary.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME; // 주 버퍼에도 볼륨 제어 추가
    hr = m_pDS->CreateSoundBuffer(&dsbdPrimary, &m_pDSPrimaryBuffer, NULL);
    if (DirectSoundErrorCheck(hr, "Failed to create Main Buffer")) {
        // 주 버퍼 생성 실패는 치명적이지 않을 수 있으므로 경고만 하고 진행 가능
        dprintf("[Alert] Failed to create Main Buffer. (AudioManager)");
        m_pDSPrimaryBuffer = nullptr; // nullptr로 설정하여 이후 사용 방지
    }
    else {
        // 주 버퍼 포맷 설정 (시스템 기본 포맷 사용)
        WAVEFORMATEX wfxPrimary;
        ZeroMemory(&wfxPrimary, sizeof(wfxPrimary));
        // 주 버퍼 포맷은 시스템의 기본 오디오 장치 포맷을 따르는 것이 일반적입니다.
        // GetCaps나 GetFormat을 통해 가져와서 설정할 수 있지만, 여기서는 간단히.
        // 또는 그냥 설정하지 않아도 시스템이 알아서 처리하는 경우도 많습니다.
        // HRESULT hrGet = m_pDSPrimaryBuffer->GetFormat(&wfxPrimary, sizeof(wfxPrimary), NULL);
        // if (SUCCEEDED(hrGet)) {
        //     m_pDSPrimaryBuffer->SetFormat(&wfxPrimary);
        // }
    }

    // SetVolume(1.0f); // 전체 볼륨은 주 버퍼가 있다면 주 버퍼에 적용
    // 개별 사운드 버퍼는 생성 시 볼륨 설정이 필요
    dprintf("AudioManager initialization completed.");
    return true;
}

void AudioManager::Shutdown() {
    m_isShuttingDown.store(true); // 종료 플래그 설정

    StopAllAudio(); // 모든 재생 중인 오디오 정지 및 정리

    // 모든 사운드 버퍼 정리 (StopAllAudio에서 대부분 처리되지만 안전을 위해)
    for (auto& pair : m_soundBuffers) {
        if (pair.second) {
            // SoundBufferInfo 소멸자에서 lpDSBuffer, hLoopThread, hEvent 정리
            // unique_ptr가 자동으로 delete를 호출
        }
    }
    m_soundBuffers.clear();

    if (m_pDSPrimaryBuffer) {
        m_pDSPrimaryBuffer->Release();
        m_pDSPrimaryBuffer = nullptr;
    }
    if (m_pDS) {
        m_pDS->Release();
        m_pDS = nullptr;
    }
    dprintf("AudioManager shutdown completed.");
}

int AudioManager::PlayMultiAudio(const std::vector<PakFileInfo> fileArray, bool isBond, int repeat) {
    if (!m_pDS) {
        dprintf("[Error] DirectSound is not initialized. (AudioManager)");
        return -1;
    }

    // 기존 재생 중인 오디오는 모두 정지
    StopAllAudio();

    // OGG, VAG, WAV 파일 처리
    const std::string oggExtension = ".ogg";
    const std::string vagExtension = ".vag";
    const std::string wavExtension = ".wav";
    int index = 0;
    int assignedHandle = -1;

    while (index < fileArray.size()) {
        // 새 사운드 버퍼 정보 객체 생성
        std::unique_ptr<SoundBufferInfo> sbInfo = std::make_unique<SoundBufferInfo>();
        sbInfo->handle = m_nextSoundHandle++;
        sbInfo->repeat = 1;
        sbInfo->pakFileInfo = fileArray.at(index); // 원본 정보 저장

        std::vector<int16_t> pcmData;
        long samplerate = 0;
        int channels = 0;

        int rep = (isBond ? fileArray.size() : (index + 1));
        pcmData.clear();
        PakFileInfo fileInfo;

        for (int i = index; index < rep; ++index) {
            // 파일 정보 얻기
            fileInfo = fileArray.at(index);

            std::vector<uint8_t>& pakData = DataManager::GetInstance().GetPakData(fileInfo);
            const uint8_t* audioRawData = pakData.data();
            size_t audioRawSize = fileInfo.dataLength;

            std::string lowerFileName = fileInfo.fileName;
            std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

            // 현재 재생 중인 오디오 정보 저장
            m_currentPlayingPakInfo = fileInfo;
            dprintf("Request audio playback: %s", fileInfo.fileName.c_str());

            // OGG일 경우
            if (lowerFileName.length() >= oggExtension.length() &&
                lowerFileName.rfind(oggExtension) == lowerFileName.length() - oggExtension.length())
            {
                if (!m_audioDecoder->DecodeOgg(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
                    dprintf("[Error] Failed to OGG decode. (AudioManager): %s", fileInfo.fileName.c_str());
                }
            }
            // VAG일 경우
            else if (lowerFileName.length() >= vagExtension.length() &&
                lowerFileName.rfind(vagExtension) == lowerFileName.length() - vagExtension.length())
            {
                if (!m_audioDecoder->DecodeVag(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
                    dprintf("[Error] Failed to VAG decode. (AudioManager): %s", fileInfo.fileName.c_str());
                }
            }
            // WAV일 경우
            else if (lowerFileName.length() >= wavExtension.length() &&
                lowerFileName.rfind(wavExtension) == lowerFileName.length() - wavExtension.length())
            {
                if (!m_audioDecoder->DecodeWav(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
                    dprintf("[Error] Failed to WAV decode. (AudioManager): %s", fileInfo.fileName.c_str());
                }
            }
            else {
                dprintf("[Error] Audio formats not supported. (OGG only) (AudioManager): %s", fileInfo.fileName.c_str());
            }
            pakData.erase(pakData.begin(), pakData.end());
        }
        if (pcmData.empty()) {
            dprintf("[Error] Decoded audio data is empty. (AudioManager)");
            continue;
        }

        // DirectSound 버퍼 생성 및 PCM 데이터 로드
        if (!CreateAndLoadSecondaryBuffer(sbInfo.get(), pcmData, samplerate, channels)) {
            continue;
        }

        // ====================================================================
        // 중요 수정: 사운드 버퍼 생성 직후 개별 볼륨을 설정합니다.
        // soundDS.cpp의 voiceVolume (0-10)을 기준으로 변환합니다.
        // 0.0f ~ 1.0f 범위로 AudioManager::SetVolume(float) 호출을 통해
        // 외부에서 설정되는 볼륨 값(예: 1.0f)을 기준으로 DirectSound 볼륨을 계산하여 적용합니다.
        // ====================================================================
        LONG initialDsVolume = DSBVOLUME_MIN;
        float defaultPlaybackVolume = 1.0f; // 기본적으로 100% 볼륨으로 재생
        if (defaultPlaybackVolume > 0.0f) {
            float effectiveVolume = std::max(0.01f, defaultPlaybackVolume);
            initialDsVolume = static_cast<LONG>(std::log10(effectiveVolume) * 2000);
        }
        sbInfo->lpDSBuffer->SetVolume(initialDsVolume);


        // 스레드 생성 및 시작
        // SoundLoop 스레드는 재생을 시작하고 상태를 관리합니다.
        sbInfo->hLoopThread = CreateThread(NULL, 0, SoundLoop, sbInfo.get(), 0, NULL);
        if (!sbInfo->hLoopThread) {
            DirectSoundErrorCheck(E_FAIL, "Failed to create playback thread (Handle: " + std::to_string(sbInfo->handle) + ")");
            // 버퍼는 이미 생성되었으므로, 여기서 정리해야 합니다.
            if (sbInfo->lpDSBuffer) {
                sbInfo->lpDSBuffer->Release();
                sbInfo->lpDSBuffer = nullptr;
            }
            continue;
        }

        // 맵에 추가
        assignedHandle = sbInfo->handle;
        m_soundBuffers[assignedHandle] = std::move(sbInfo); // unique_ptr 이동

        dprintf("Request audio playback: %s (Handle: %d)", fileInfo.fileName.c_str(), assignedHandle);
    }

    return assignedHandle;
}

int AudioManager::PlayAudio(const PakFileInfo& fileInfo, int repeat) {
    if (!m_pDS) {
        dprintf("[Error] DirectSound is not initialized. (AudioManager)");
        return -1;
    }

    // 기존 재생 중인 오디오는 모두 정지
    StopAllAudio();

    // 새 사운드 버퍼 정보 객체 생성
    std::unique_ptr<SoundBufferInfo> sbInfo = std::make_unique<SoundBufferInfo>();
    sbInfo->handle = m_nextSoundHandle++;
    sbInfo->repeat = repeat;
    sbInfo->pakFileInfo = fileInfo; // 원본 정보 저장

    std::vector<uint8_t>& pakData = DataManager::GetInstance().GetPakData(fileInfo);
    const uint8_t* audioRawData = pakData.data();
    size_t audioRawSize = fileInfo.dataLength;

    std::vector<int16_t> pcmData;
    long samplerate = 0;
    int channels = 0;

    std::string lowerFileName = fileInfo.fileName;
    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

    // OGG, VAG, WAV 파일 처리
    const std::string oggExtension = ".ogg";
    const std::string vagExtension = ".vag";
    const std::string wavExtension = ".wav";

    pcmData.clear();

    // OGG일 경우
    if (lowerFileName.length() >= oggExtension.length() &&
        lowerFileName.rfind(oggExtension) == lowerFileName.length() - oggExtension.length())
    {
        if (!m_audioDecoder->DecodeOgg(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
            dprintf("[Error] Failed to OGG decode. (AudioManager): %s", fileInfo.fileName.c_str());
            return -1;
        }
    }
    // VAG일 경우
    else if (lowerFileName.length() >= vagExtension.length() &&
        lowerFileName.rfind(vagExtension) == lowerFileName.length() - vagExtension.length())
    {
        if (!m_audioDecoder->DecodeVag(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
            dprintf("[Error] Failed to VAG decode. (AudioManager): %s", fileInfo.fileName.c_str());
            return -1;
        }
    }
    // WAV일 경우
    else if (lowerFileName.length() >= wavExtension.length() &&
        lowerFileName.rfind(wavExtension) == lowerFileName.length() - wavExtension.length())
    {
        if (!m_audioDecoder->DecodeWav(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
            dprintf("[Error] Failed to WAV decode. (AudioManager): %s", fileInfo.fileName.c_str());
            return -1;
        }
    }
    else {
        dprintf("[Error] Audio formats not supported. (OGG only) (AudioManager): %s", fileInfo.fileName.c_str());
        return -1;
    }
    pakData.erase(pakData.begin(), pakData.end());

    if (pcmData.empty()) {
        dprintf("[Error] Decoded audio data is empty. (AudioManager): %s", fileInfo.fileName.c_str());
        return -1;
    }

    // DirectSound 버퍼 생성 및 PCM 데이터 로드
    if (!CreateAndLoadSecondaryBuffer(sbInfo.get(), pcmData, samplerate, channels)) {
        return -1;
    }
    pcmData.erase(pcmData.begin(), pcmData.end());

    // ====================================================================
    // 중요 수정: 사운드 버퍼 생성 직후 개별 볼륨을 설정합니다.
    // soundDS.cpp의 voiceVolume (0-10)을 기준으로 변환합니다.
    // 0.0f ~ 1.0f 범위로 AudioManager::SetVolume(float) 호출을 통해
    // 외부에서 설정되는 볼륨 값(예: 1.0f)을 기준으로 DirectSound 볼륨을 계산하여 적용합니다.
    // ====================================================================
    LONG initialDsVolume = DSBVOLUME_MIN;
    float defaultPlaybackVolume = 1.0f; // 기본적으로 100% 볼륨으로 재생
    if (defaultPlaybackVolume > 0.0f) {
        float effectiveVolume = std::max(0.01f, defaultPlaybackVolume);
        initialDsVolume = static_cast<LONG>(std::log10(effectiveVolume) * 2000);
    }
    sbInfo->lpDSBuffer->SetVolume(initialDsVolume);


    // 스레드 생성 및 시작
    // SoundLoop 스레드는 재생을 시작하고 상태를 관리합니다.
    sbInfo->hLoopThread = CreateThread(NULL, 0, SoundLoop, sbInfo.get(), 0, NULL);
    if (!sbInfo->hLoopThread) {
        DirectSoundErrorCheck(E_FAIL, "Failed to create playback thread (Handle: " + std::to_string(sbInfo->handle) + ")");
        // 버퍼는 이미 생성되었으므로, 여기서 정리해야 합니다.
        if (sbInfo->lpDSBuffer) {
            sbInfo->lpDSBuffer->Release();
            sbInfo->lpDSBuffer = nullptr;
        }
        return -1;
    }

    // 현재 재생 중인 오디오 정보 저장
    m_currentPlayingPakInfo = fileInfo;

    // 맵에 추가
    int assignedHandle = sbInfo->handle;
    m_soundBuffers[assignedHandle] = std::move(sbInfo); // unique_ptr 이동

    dprintf("Request audio playback: %s (Handle: %d)", fileInfo.fileName.c_str(), assignedHandle);

    return assignedHandle;
}

void AudioManager::StopAudio(int handle) {
    auto it = m_soundBuffers.find(handle);
    if (it != m_soundBuffers.end()) {
        SoundBufferInfo* sbInfo = it->second.get();
        EnterCriticalSection(&sbInfo->cr_section);
        sbInfo->status = PCM_STOP; // 스레드에 정지 신호
        if (sbInfo->lpDSBuffer) {
            sbInfo->lpDSBuffer->Stop();
            sbInfo->lpDSBuffer->SetCurrentPosition(0);
        }
        LeaveCriticalSection(&sbInfo->cr_section);
        // 스레드가 종료될 때까지 기다림 (선택 사항, 자원 해제를 확실히 하려면)
        if (sbInfo->hLoopThread) {
            // 스레드가 SetEvent를 기다리고 있다면 Signal
            SetEvent(sbInfo->hEvent[0]); // Sleep(1) 루프를 깨우기 위해
            WaitForSingleObject(sbInfo->hLoopThread, INFINITE); // 스레드 종료 대기
            CloseHandle(sbInfo->hLoopThread);
            sbInfo->hLoopThread = NULL;
        }
        m_soundBuffers.erase(it); // 맵에서 제거 및 unique_ptr 소멸 (SoundBufferInfo 소멸자 호출)
#ifdef _DEBUG
        dprintf("Stop audio playback: Handle: %d", handle);
#endif
    }
}

void AudioManager::StopAllAudio() {
    // 모든 사운드 버퍼를 역순으로 순회하며 정지 (맵에서 요소 제거가 안전)
    // 맵을 복사하거나, 역방향 이터레이터를 사용하는 것이 안전할 수 있습니다.
    // 여기서는 핸들을 모아서 개별적으로 StopAudio를 호출하는 방식을 사용
    std::vector<int> handlesToStop;
    for (const auto& pair : m_soundBuffers) {
        handlesToStop.push_back(pair.first);
    }
    for (int handle : handlesToStop) {
        if ((find(m_uniqueHandle.begin(), m_uniqueHandle.end(), handle)) == m_uniqueHandle.end()) {
            StopAudio(handle);
        }
    }
    // m_soundBuffers는 StopAudio에서 이미 제거하므로 여기서는 clear() 불필요
}


void AudioManager::PauseAudio(int handle) {
    auto it = m_soundBuffers.find(handle);
    if (it != m_soundBuffers.end()) {
        SoundBufferInfo* sbInfo = it->second.get();
        EnterCriticalSection(&sbInfo->cr_section);
        if (sbInfo->lpDSBuffer && sbInfo->status == PCM_PLAY) {
            sbInfo->lpDSBuffer->Stop();
            sbInfo->status = PCM_FADEOUTBREAK; // 임시로 PCM_FADEOUTBREAK 사용 (재생을 멈추고 상태를 유지)
        }
        LeaveCriticalSection(&sbInfo->cr_section);
        dprintf("Pause audio playback: Handle: %d", handle);
    }
}

void AudioManager::ResumeAudio(int handle) {
    auto it = m_soundBuffers.find(handle);
    if (it != m_soundBuffers.end()) {
        SoundBufferInfo* sbInfo = it->second.get();
        EnterCriticalSection(&sbInfo->cr_section);
        if (sbInfo->lpDSBuffer && sbInfo->status == PCM_FADEOUTBREAK) { // 일시정지 상태에서만 재개
            sbInfo->lpDSBuffer->Play(0, 0, 0);
            sbInfo->status = PCM_PLAY;
        }
        LeaveCriticalSection(&sbInfo->cr_section);
        dprintf("Resume audio playback: Handle: %d", handle);
    }
}

void AudioManager::SetVolume(float volume) {
    // 0.0f ~ 1.0f 범위를 DirectSound 데시벨로 변환
    LONG dsVolume = DSBVOLUME_MIN; // 기본 최소값 (음소거)
    if (volume > 0.0f) {
        // DirectSound 볼륨은 로그 스케일이므로 log10을 사용
        // DSBVOLUME_MAX는 0, DSBVOLUME_MIN은 -10000
        // -10000은 약 0.0001 (0.01%)에 해당하므로, 0.0001부터 1.0까지 매핑.
        // float normalizedVolume = std::max(0.0001f, volume); // 0이하 방지
        // dsVolume = static_cast<LONG>(std::log10(normalizedVolume) * 2000); // 20log10(amp) * 100

        // soundDS.cpp의 공식에 맞게 다시 적용 (bgmVolume 0-10 스케일 참고)
        // 0.0 ~ 1.0 볼륨을 0 ~ 10 스케일로 보고 계산
        float scaledVolume = volume * 10.0f;
        if (scaledVolume <= 0.0f) {
            dsVolume = DSBVOLUME_MIN;
        }
        else {
            // (입력 볼륨 / 최대 볼륨) * 10000 (DSBVOLUME_MAX - DSBVOLUME_MIN)
            // 여기서는 soundDS.cpp의 log10((volume/10.0f)*i/16.0f) * 40 * 100 공식과 유사하게
            // 0.0f ~ 1.0f 범위에 대해 dsVolume을 계산합니다.
            // 0.01f를 최소값으로 하여 로그 계산 시 문제가 없도록 합니다.
            float effectiveVolume = std::max(0.01f, volume); // 0.01f 이하로 내려가지 않도록
            dsVolume = static_cast<LONG>(std::log10(effectiveVolume) * 2000); // 20 * 100 * log10(volume)
        }
    }

    // 주 버퍼가 있다면 주 버퍼 볼륨 조절
    if (m_pDSPrimaryBuffer) {
        m_pDSPrimaryBuffer->SetVolume(dsVolume);
    }
    // 또는 모든 활성화된 보조 버퍼에 볼륨 적용.
    else {
        for (auto& pair : m_soundBuffers) {
            if (pair.second && pair.second->lpDSBuffer) {
                pair.second->lpDSBuffer->SetVolume(dsVolume);
            }
        }
    }
}

AudioManager::PlaybackState AudioManager::GetPlaybackState(int handle) const {
    auto it = m_soundBuffers.find(handle);
    if ((handle == -1) && (it == m_soundBuffers.end())) {
        if (--it == m_soundBuffers.begin()) {
            it = m_soundBuffers.end();
        }
    }
    if (it != m_soundBuffers.end()) {
        SoundBufferInfo* sbInfo = it->second.get();
        EnterCriticalSection(&sbInfo->cr_section); // 스레드 안전하게 접근
        CL_PCMSTATUS currentStatus = sbInfo->status;
        LeaveCriticalSection(&sbInfo->cr_section);

        if (currentStatus == PCM_PLAY) {
            DWORD status;
            if (sbInfo->lpDSBuffer && SUCCEEDED(sbInfo->lpDSBuffer->GetStatus(&status))) {
                if (status & DSBSTATUS_PLAYING) {
                    return PlaybackState::Playing;
                }
                else {
                    // 스레드에서 PCM_PLAY로 설정했지만 DirectSound 버퍼가 멈췄다면 Stopped
                    return PlaybackState::Stopped;
                }
            }
            return PlaybackState::Stopped; // 버퍼가 없거나 상태 가져오기 실패
        }
        else if (currentStatus == PCM_FADEOUTBREAK) { // 일시정지 상태로 사용
            return PlaybackState::Paused;
        }
        else {
            return PlaybackState::Stopped;
        }
    }
    return PlaybackState::Stopped; // 해당 핸들을 찾을 수 없음
}

std::string AudioManager::GetLastPlayFileName() const {
    std::string fname = "";
    if (!m_soundBuffers.empty()) {
        auto it = m_soundBuffers.begin();
        for (int i = 1; i < m_soundBuffers.size(); i++) {
            it++;
        }
        if (it != m_soundBuffers.end()) {
            SoundBufferInfo* sbInfo = it->second.get();
            if (sbInfo != NULL) {
                try {
                    EnterCriticalSection(&sbInfo->cr_section);
                    fname = sbInfo->pakFileInfo.fileName;
                    LeaveCriticalSection(&sbInfo->cr_section);
                }
                catch (const std::exception& e) {
                    dprintf("[Error] Failed to get last playback filename. (%s)", e.what());
                }
            }
        }
    }
    return fname;
}


HRESULT AudioManager::DirectSoundErrorCheck(HRESULT hr, const std::string& msg) {
    if (FAILED(hr)) {
        dprintf("[Error] %s (HR: 0x%08X)", msg, hr);
    }
    return hr;
}

bool AudioManager::CreateAndLoadSecondaryBuffer(SoundBufferInfo* sbInfo, const std::vector<int16_t>& pcmData,
    long samplerate, int channels) {
    HRESULT hr;

    if (sbInfo->lpDSBuffer) {
        sbInfo->lpDSBuffer->Release();
        sbInfo->lpDSBuffer = nullptr;
    }

    WAVEFORMATEX wfx;
    ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = static_cast<WORD>(channels);
    wfx.nSamplesPerSec = static_cast<DWORD>(samplerate);
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    DSBUFFERDESC dsbd;
    ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_STATIC | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS; // 현재 위치 가져오기 플래그 추가
    dsbd.dwBufferBytes = pcmData.size() * sizeof(int16_t);
    dsbd.lpwfxFormat = &wfx;

    hr = m_pDS->CreateSoundBuffer(&dsbd, &sbInfo->lpDSBuffer, NULL);
    if (DirectSoundErrorCheck(hr, "Failed to create Secondary buffer (Handle: " + std::to_string(sbInfo->handle) + ")")) return false;

    VOID* pbWaveData = NULL;
    DWORD dwWaveBytes;

    hr = sbInfo->lpDSBuffer->Lock(0, dsbd.dwBufferBytes, &pbWaveData, &dwWaveBytes, NULL, NULL, 0);
    if (DirectSoundErrorCheck(hr, "Failed to Lock buffer (Handle: " + std::to_string(sbInfo->handle) + ")")) return false;

    memcpy(pbWaveData, pcmData.data(), dwWaveBytes);

    hr = sbInfo->lpDSBuffer->Unlock(pbWaveData, dwWaveBytes, NULL, 0);
    if (DirectSoundErrorCheck(hr, "Failed to Unlock buffer (Handle: " + std::to_string(sbInfo->handle) + ")")) return false;

    return true;
}

// ===============================================
// 사운드 재생 루프 스레드 함수
// soundDS.cpp의 SoundLoop 함수와 유사한 역할을 합니다.
// ===============================================
DWORD WINAPI AudioManager::SoundLoop(LPVOID param) {
    SoundBufferInfo* lpSoundBuffer = static_cast<SoundBufferInfo*>(param);
    if (!lpSoundBuffer || !lpSoundBuffer->lpDSBuffer) {
        ExitThread(FALSE);
        return 0L;
    }

    // ====================================================================
    // 중요 수정: 스레드에서 재생 시작 직전에 볼륨을 다시 설정합니다.
    // 이는 스레드가 스케줄링되어 실행될 때 혹시라도 볼륨이 초기화되거나 변경되는 것을 방지합니다.
    // ====================================================================
    LONG currentVolume;
    HRESULT hrGetVolume = lpSoundBuffer->lpDSBuffer->GetVolume(&currentVolume);
    if (FAILED(hrGetVolume)) {
        dprintf("[Error] Failed to get volume. (Handle: %d) HR: 0x%08X", lpSoundBuffer->handle, hrGetVolume);
    }
    else {
        // 이미 PlayAudio에서 설정했으므로 그대로 유지하거나, 필요시 재설정
        // lpSoundBuffer->lpDSBuffer->SetVolume(currentVolume); // 이미 설정된 볼륨을 그대로 유지
    }


    EnterCriticalSection(&lpSoundBuffer->cr_section);
    lpSoundBuffer->status = PCM_PLAY;
    HRESULT hr = lpSoundBuffer->lpDSBuffer->Play(0, 0, (lpSoundBuffer->repeat == 0) ? DSBPLAY_LOOPING : 0);
    LeaveCriticalSection(&lpSoundBuffer->cr_section);

    if (FAILED(hr)) {
        dprintf("[Error] Failed to start audio playback. (Handle: %d) HR: 0x%08X", lpSoundBuffer->handle, hr);
        lpSoundBuffer->status = PCM_STOP; // 실패 시 상태 변경
        ExitThread(FALSE);
        return 0L;
    }

    // 재생 완료 대기 루프 (soundDS.cpp의 SoundLoop와 유사)
    DWORD status = 0;
    while (true) {
        if (AudioManager::GetInstance().m_isShuttingDown.load()) {
            break;
        }

        EnterCriticalSection(&lpSoundBuffer->cr_section);
        if (lpSoundBuffer->status != PCM_PLAY) {
            LeaveCriticalSection(&lpSoundBuffer->cr_section);
            break;
        }

        if (lpSoundBuffer->lpDSBuffer) {
            // DirectSound 버퍼의 재생 상태를 가져옵니다.
            HRESULT hrStatus = lpSoundBuffer->lpDSBuffer->GetStatus(&status);
            if (FAILED(hrStatus)) {
                dprintf("[Error] Failed to GetStatus. (Handle: %d) HR: 0x%08X", lpSoundBuffer->handle, hrStatus);
                lpSoundBuffer->status = PCM_STOP; // 오류 발생 시 정지
                LeaveCriticalSection(&lpSoundBuffer->cr_section);
                break;
            }

            if (!(status & DSBSTATUS_PLAYING)) {
                // 재생이 멈췄을 때
                if (lpSoundBuffer->repeat == 0) { // 무한 반복 모드
                    // DSBPLAY_LOOPING 플래그를 사용했으므로 여기에 도달하면 예상치 못한 정지
                    dprintf("[Error] Buffer stops unexpectedly (Infinite loop mode, Handle: %d). Try to restart.", lpSoundBuffer->handle);
                    lpSoundBuffer->lpDSBuffer->SetCurrentPosition(0); // 위치 초기화
                    lpSoundBuffer->lpDSBuffer->Play(0, 0, DSBPLAY_LOOPING); // 다시 재생
                }
                else { // 단일 재생 모드 (또는 반복 횟수만큼 재생 완료)
                    // 정상적인 재생 종료
                    lpSoundBuffer->status = PCM_STOP;
                    LeaveCriticalSection(&lpSoundBuffer->cr_section);
                    break;
                }
            }
        }
        LeaveCriticalSection(&lpSoundBuffer->cr_section);

        // CPU 사용량을 줄이기 위해 잠시 대기
        Sleep(5); // 10ms -> 5ms로 줄여서 반응성 개선 시도
    }

    EnterCriticalSection(&lpSoundBuffer->cr_section);
    if (lpSoundBuffer->lpDSBuffer) {
        lpSoundBuffer->lpDSBuffer->Stop();
        lpSoundBuffer->lpDSBuffer->SetCurrentPosition(0);
    }
    lpSoundBuffer->status = PCM_STOP; // 최종 상태 정지
    LeaveCriticalSection(&lpSoundBuffer->cr_section);

    // 스레드 종료 시 이벤트 신호 (필요시 SetEvent(lpSoundBuffer->hEvent[0]); )
    //std::cout << "[SoundLoop] 스레드 종료 (핸들: " << lpSoundBuffer->handle << ")" << std::endl;
    ExitThread(TRUE);
    return 0L;
}

void AudioManager::ContinuePlay() {
    // DataManager를 통해 현재 main_idx와 sub_idx를 가져옵니다.
    // DataManager는 ScanProcess가 업데이트하는 currentMainIndex와
    // 사용자가 제어하는 currentSubIndex를 관리합니다.
    int main_idx = DataManager::GetInstance().GetCurrentMainIndex();
    int sub_idx = DataManager::GetInstance().GetCurrentSubIndex();

    if (main_idx == -1) {
        dprintf("[Error] Main Index is not currently set. It cannot be played.");
        return;
    }

    dprintf("Try playing the next audio: Main: %d, Sub: %d", main_idx, sub_idx);
    int pakFileIndex = DataManager::GetInstance().SearchIndex(main_idx, sub_idx);

    if (pakFileIndex != -1) {
        const PakFileInfo& fileInfo = DataManager::GetInstance().GetPakFiles()[pakFileIndex];
        // PlayAudio 내부에서 StopAllAudio()를 호출하므로 여기서 직접 StopAllAudio()를 호출할 필요 없음.
        PlayAudio(fileInfo, 1); // 1번 재생

        // 재생 후 sub_idx 1 증가
        if (DataManager::GetInstance().SearchIndex(main_idx, ++sub_idx) != -1) {
            DataManager::GetInstance().SetCurrentSubIndex(sub_idx);
            dprintf("Increased SubIndex: %d.", sub_idx);
        }
        else {
            DataManager::GetInstance().SetCurrentSubIndex(sub_idx);
            dprintf("[Error] Failed to Increased SubIndex: %d.", sub_idx);
        }
    }
    else {
        dprintf("[Error] could not find audio corresponding to the MainIndex (%d) and SubIndex (%d).", main_idx, sub_idx);
    }
}

void AudioManager::PrevPlay() {
    int main_idx = DataManager::GetInstance().GetCurrentMainIndex();
    int current_sub_idx = DataManager::GetInstance().GetCurrentSubIndex();

    if (main_idx == -1) {
        dprintf("[Error] Main Index is not currently set. It cannot be played.");
        return;
    }

    // 현재 Sub Index에서 1을 감소시킵니다. (음수 방지)
    current_sub_idx = std::max(1, (current_sub_idx - 1));
    int target_sub_idx = std::max(1, (current_sub_idx - 1));

    // DataManager의 sub_idx를 먼저 업데이트
    DataManager::GetInstance().SetCurrentSubIndex(current_sub_idx);

    dprintf("Try playing the previous audio: Main: %d, Sub: %d", main_idx, target_sub_idx);
    int pakFileIndex = DataManager::GetInstance().SearchIndex(main_idx, target_sub_idx);

    if (pakFileIndex != -1) {
        const PakFileInfo& fileInfo = DataManager::GetInstance().GetPakFiles()[pakFileIndex];
        PlayAudio(fileInfo, 1); // 1번 재생
    }
    else {
        dprintf("[Error] could not find audio corresponding to the MainIndex (%d) and SubIndex (%d).", main_idx, target_sub_idx);
    }
}

void AudioManager::PlaySpecificSubIndex(bool isBond, int numArgs, ...) {
    int main_idx = DataManager::GetInstance().GetCurrentMainIndex();
    if (main_idx == -1) {
        dprintf("[Error] Main Index is not currently set. It cannot be played.");
        return;
    }

    va_list args;
    va_start(args, numArgs); // numArgs 뒤의 인자부터 시작

    std::vector<int> argValues;
    std::vector<PakFileInfo> pakArray;
    for (int i = 0; i < numArgs; ++i) {
        int currentArg = va_arg(args, int); // 다음 int 타입 인자를 가져옴
        argValues.push_back(currentArg);
    }
    va_end(args); // va_list 사용 종료

    std::stringstream debugStr;
    debugStr << "Try playing a particular Subindex audio : Main: %d, Sub :";

    for (int target_sub_idx : argValues) {
        debugStr << " " << target_sub_idx;
        int pakFileIndex = DataManager::GetInstance().SearchIndex(main_idx, target_sub_idx);
        if (pakFileIndex != -1) {
            const PakFileInfo& fileInfo = DataManager::GetInstance().GetPakFiles()[pakFileIndex];
            pakArray.push_back(fileInfo);
        }
    }
    dprintf(debugStr.str().c_str(), main_idx);
    if (pakArray.size() > 0) {
        PlayMultiAudio(pakArray, isBond);
    }
    else {
        dprintf("[Error] Playable SubIndex not included.");
    }
}

void AudioManager::PlayCurrentAudio(bool isUnique) {
    int main_idx = DataManager::GetInstance().GetCurrentMainIndex();
    int sub_idx = DataManager::GetInstance().GetCurrentSubIndex();
    if (main_idx < 0) { // sub_idx가 0보다 작으면 유효하지 않음
        dprintf("Audio information to replay is invalid. Play audio first.");
        return;
    }
#ifdef _DEBUG
    dprintf("Playing the current audio: Main: %d, Sub: %d", main_idx, sub_idx);
#endif
    int pakFileIndex = DataManager::GetInstance().SearchIndex(main_idx, sub_idx);
    int hNum;

    if (pakFileIndex != -1) {
        const PakFileInfo& fileInfo = DataManager::GetInstance().GetPakFiles()[pakFileIndex];
        hNum = PlayAudio(fileInfo, 1); // 1번 재생
        if (isUnique) {
            m_uniqueHandle.push_back(hNum);
        }
    }
    else if (sub_idx < 1) {
        dprintf("[Alert] MainIndex (%d) Audio playback passed.", main_idx);
    }
    else {
        dprintf("[Error] could not find audio corresponding to the MainIndex (%d) and SubIndex (%d).", main_idx, sub_idx);
    }
}