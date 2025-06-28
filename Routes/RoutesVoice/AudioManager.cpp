#include "AudioManager.h"
#include "AudioDecoder.h"
#include "DataManager.h" // DataManager ����
#include <iostream>
#include <sstream>
#include <algorithm> // std::transform
#include <cmath>     // std::log10
#include <exception> // For std::stoi exceptions

// DirectSound ���̺귯�� ��ũ
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")

#undef max

// ���� (�Ǵ� AudioManager ���) ���� ����
// soundDS.cpp���� ������ bgmVolume, voiceVolume, seVolumeó��
// AudioManager���� ���������� �����ϴ� ���� �����ϴ�.
// ���⼭�� �ϴ� ����ó�� bgmVolume, voiceVolume ��� ���÷� �Ӵϴ�.
extern int bgmVolume; // soundDS.cpp���� ���ǵ� ���� ������� ����
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

    // GUI ���ø����̼��� ���� ������ �ڵ��� ���
    // DSSCL_NORMAL�� ��Ŀ���� �Ҿ ��׶��� ����� �����ϰ� �մϴ�.
    hr = m_pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
    if (DirectSoundErrorCheck(hr, "Failed to set DirectSound Cooperative Level")) {
        m_pDS->Release();
        m_pDS = nullptr;
        dprintf("[Error] Failed to set DirectSound Cooperative Level. Exit the program. (AudioManager)");
        return false; // ���� ���� ���� ���� �� ġ���� ������ �����ϰ� ����
    }

    // �� ���� ���� (���� ����������, DirectSound �ʱ�ȭ�� ǥ�� �κ�)
    // �� ���۴� ���ø����̼��� ��Ŀ���� ������ �ڵ����� �����ǹǷ�,
    // ���� ���۴� DSSCL_NORMAL�� �������� �� ��׶��� ����� ������ �� �ֽ��ϴ�.
    // ������ Primary Buffer�� ���� ���� ��忡�� ���ǹǷ� ���⼭�� �Ϲ����� �������.
    DSBUFFERDESC dsbdPrimary;
    ZeroMemory(&dsbdPrimary, sizeof(DSBUFFERDESC));
    dsbdPrimary.dwSize = sizeof(DSBUFFERDESC);
    dsbdPrimary.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME; // �� ���ۿ��� ���� ���� �߰�
    hr = m_pDS->CreateSoundBuffer(&dsbdPrimary, &m_pDSPrimaryBuffer, NULL);
    if (DirectSoundErrorCheck(hr, "Failed to create Main Buffer")) {
        // �� ���� ���� ���д� ġ�������� ���� �� �����Ƿ� ��� �ϰ� ���� ����
        dprintf("[Alert] Failed to create Main Buffer. (AudioManager)");
        m_pDSPrimaryBuffer = nullptr; // nullptr�� �����Ͽ� ���� ��� ����
    }
    else {
        // �� ���� ���� ���� (�ý��� �⺻ ���� ���)
        WAVEFORMATEX wfxPrimary;
        ZeroMemory(&wfxPrimary, sizeof(wfxPrimary));
        // �� ���� ������ �ý����� �⺻ ����� ��ġ ������ ������ ���� �Ϲ����Դϴ�.
        // GetCaps�� GetFormat�� ���� �����ͼ� ������ �� ������, ���⼭�� ������.
        // �Ǵ� �׳� �������� �ʾƵ� �ý����� �˾Ƽ� ó���ϴ� ��쵵 �����ϴ�.
        // HRESULT hrGet = m_pDSPrimaryBuffer->GetFormat(&wfxPrimary, sizeof(wfxPrimary), NULL);
        // if (SUCCEEDED(hrGet)) {
        //     m_pDSPrimaryBuffer->SetFormat(&wfxPrimary);
        // }
    }

    // SetVolume(1.0f); // ��ü ������ �� ���۰� �ִٸ� �� ���ۿ� ����
    // ���� ���� ���۴� ���� �� ���� ������ �ʿ�
    dprintf("AudioManager initialization completed.");
    return true;
}

void AudioManager::Shutdown() {
    m_isShuttingDown.store(true); // ���� �÷��� ����

    StopAllAudio(); // ��� ��� ���� ����� ���� �� ����

    // ��� ���� ���� ���� (StopAllAudio���� ��κ� ó�������� ������ ����)
    for (auto& pair : m_soundBuffers) {
        if (pair.second) {
            // SoundBufferInfo �Ҹ��ڿ��� lpDSBuffer, hLoopThread, hEvent ����
            // unique_ptr�� �ڵ����� delete�� ȣ��
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

    // ���� ��� ���� ������� ��� ����
    StopAllAudio();

    // OGG, VAG, WAV ���� ó��
    const std::string oggExtension = ".ogg";
    const std::string vagExtension = ".vag";
    const std::string wavExtension = ".wav";
    int index = 0;
    int assignedHandle = -1;

    while (index < fileArray.size()) {
        // �� ���� ���� ���� ��ü ����
        std::unique_ptr<SoundBufferInfo> sbInfo = std::make_unique<SoundBufferInfo>();
        sbInfo->handle = m_nextSoundHandle++;
        sbInfo->repeat = 1;
        sbInfo->pakFileInfo = fileArray.at(index); // ���� ���� ����

        std::vector<int16_t> pcmData;
        long samplerate = 0;
        int channels = 0;

        int rep = (isBond ? fileArray.size() : (index + 1));
        pcmData.clear();
        PakFileInfo fileInfo;

        for (int i = index; index < rep; ++index) {
            // ���� ���� ���
            fileInfo = fileArray.at(index);

            std::vector<uint8_t>& pakData = DataManager::GetInstance().GetPakData(fileInfo);
            const uint8_t* audioRawData = pakData.data();
            size_t audioRawSize = fileInfo.dataLength;

            std::string lowerFileName = fileInfo.fileName;
            std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

            // ���� ��� ���� ����� ���� ����
            m_currentPlayingPakInfo = fileInfo;
            dprintf("Request audio playback: %s", fileInfo.fileName.c_str());

            // OGG�� ���
            if (lowerFileName.length() >= oggExtension.length() &&
                lowerFileName.rfind(oggExtension) == lowerFileName.length() - oggExtension.length())
            {
                if (!m_audioDecoder->DecodeOgg(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
                    dprintf("[Error] Failed to OGG decode. (AudioManager): %s", fileInfo.fileName.c_str());
                }
            }
            // VAG�� ���
            else if (lowerFileName.length() >= vagExtension.length() &&
                lowerFileName.rfind(vagExtension) == lowerFileName.length() - vagExtension.length())
            {
                if (!m_audioDecoder->DecodeVag(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
                    dprintf("[Error] Failed to VAG decode. (AudioManager): %s", fileInfo.fileName.c_str());
                }
            }
            // WAV�� ���
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

        // DirectSound ���� ���� �� PCM ������ �ε�
        if (!CreateAndLoadSecondaryBuffer(sbInfo.get(), pcmData, samplerate, channels)) {
            continue;
        }

        // ====================================================================
        // �߿� ����: ���� ���� ���� ���� ���� ������ �����մϴ�.
        // soundDS.cpp�� voiceVolume (0-10)�� �������� ��ȯ�մϴ�.
        // 0.0f ~ 1.0f ������ AudioManager::SetVolume(float) ȣ���� ����
        // �ܺο��� �����Ǵ� ���� ��(��: 1.0f)�� �������� DirectSound ������ ����Ͽ� �����մϴ�.
        // ====================================================================
        LONG initialDsVolume = DSBVOLUME_MIN;
        float defaultPlaybackVolume = 1.0f; // �⺻������ 100% �������� ���
        if (defaultPlaybackVolume > 0.0f) {
            float effectiveVolume = std::max(0.01f, defaultPlaybackVolume);
            initialDsVolume = static_cast<LONG>(std::log10(effectiveVolume) * 2000);
        }
        sbInfo->lpDSBuffer->SetVolume(initialDsVolume);


        // ������ ���� �� ����
        // SoundLoop ������� ����� �����ϰ� ���¸� �����մϴ�.
        sbInfo->hLoopThread = CreateThread(NULL, 0, SoundLoop, sbInfo.get(), 0, NULL);
        if (!sbInfo->hLoopThread) {
            DirectSoundErrorCheck(E_FAIL, "Failed to create playback thread (Handle: " + std::to_string(sbInfo->handle) + ")");
            // ���۴� �̹� �����Ǿ����Ƿ�, ���⼭ �����ؾ� �մϴ�.
            if (sbInfo->lpDSBuffer) {
                sbInfo->lpDSBuffer->Release();
                sbInfo->lpDSBuffer = nullptr;
            }
            continue;
        }

        // �ʿ� �߰�
        assignedHandle = sbInfo->handle;
        m_soundBuffers[assignedHandle] = std::move(sbInfo); // unique_ptr �̵�

        dprintf("Request audio playback: %s (Handle: %d)", fileInfo.fileName.c_str(), assignedHandle);
    }

    return assignedHandle;
}

int AudioManager::PlayAudio(const PakFileInfo& fileInfo, int repeat) {
    if (!m_pDS) {
        dprintf("[Error] DirectSound is not initialized. (AudioManager)");
        return -1;
    }

    // ���� ��� ���� ������� ��� ����
    StopAllAudio();

    // �� ���� ���� ���� ��ü ����
    std::unique_ptr<SoundBufferInfo> sbInfo = std::make_unique<SoundBufferInfo>();
    sbInfo->handle = m_nextSoundHandle++;
    sbInfo->repeat = repeat;
    sbInfo->pakFileInfo = fileInfo; // ���� ���� ����

    std::vector<uint8_t>& pakData = DataManager::GetInstance().GetPakData(fileInfo);
    const uint8_t* audioRawData = pakData.data();
    size_t audioRawSize = fileInfo.dataLength;

    std::vector<int16_t> pcmData;
    long samplerate = 0;
    int channels = 0;

    std::string lowerFileName = fileInfo.fileName;
    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

    // OGG, VAG, WAV ���� ó��
    const std::string oggExtension = ".ogg";
    const std::string vagExtension = ".vag";
    const std::string wavExtension = ".wav";

    pcmData.clear();

    // OGG�� ���
    if (lowerFileName.length() >= oggExtension.length() &&
        lowerFileName.rfind(oggExtension) == lowerFileName.length() - oggExtension.length())
    {
        if (!m_audioDecoder->DecodeOgg(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
            dprintf("[Error] Failed to OGG decode. (AudioManager): %s", fileInfo.fileName.c_str());
            return -1;
        }
    }
    // VAG�� ���
    else if (lowerFileName.length() >= vagExtension.length() &&
        lowerFileName.rfind(vagExtension) == lowerFileName.length() - vagExtension.length())
    {
        if (!m_audioDecoder->DecodeVag(audioRawData, audioRawSize, pcmData, samplerate, channels)) {
            dprintf("[Error] Failed to VAG decode. (AudioManager): %s", fileInfo.fileName.c_str());
            return -1;
        }
    }
    // WAV�� ���
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

    // DirectSound ���� ���� �� PCM ������ �ε�
    if (!CreateAndLoadSecondaryBuffer(sbInfo.get(), pcmData, samplerate, channels)) {
        return -1;
    }
    pcmData.erase(pcmData.begin(), pcmData.end());

    // ====================================================================
    // �߿� ����: ���� ���� ���� ���� ���� ������ �����մϴ�.
    // soundDS.cpp�� voiceVolume (0-10)�� �������� ��ȯ�մϴ�.
    // 0.0f ~ 1.0f ������ AudioManager::SetVolume(float) ȣ���� ����
    // �ܺο��� �����Ǵ� ���� ��(��: 1.0f)�� �������� DirectSound ������ ����Ͽ� �����մϴ�.
    // ====================================================================
    LONG initialDsVolume = DSBVOLUME_MIN;
    float defaultPlaybackVolume = 1.0f; // �⺻������ 100% �������� ���
    if (defaultPlaybackVolume > 0.0f) {
        float effectiveVolume = std::max(0.01f, defaultPlaybackVolume);
        initialDsVolume = static_cast<LONG>(std::log10(effectiveVolume) * 2000);
    }
    sbInfo->lpDSBuffer->SetVolume(initialDsVolume);


    // ������ ���� �� ����
    // SoundLoop ������� ����� �����ϰ� ���¸� �����մϴ�.
    sbInfo->hLoopThread = CreateThread(NULL, 0, SoundLoop, sbInfo.get(), 0, NULL);
    if (!sbInfo->hLoopThread) {
        DirectSoundErrorCheck(E_FAIL, "Failed to create playback thread (Handle: " + std::to_string(sbInfo->handle) + ")");
        // ���۴� �̹� �����Ǿ����Ƿ�, ���⼭ �����ؾ� �մϴ�.
        if (sbInfo->lpDSBuffer) {
            sbInfo->lpDSBuffer->Release();
            sbInfo->lpDSBuffer = nullptr;
        }
        return -1;
    }

    // ���� ��� ���� ����� ���� ����
    m_currentPlayingPakInfo = fileInfo;

    // �ʿ� �߰�
    int assignedHandle = sbInfo->handle;
    m_soundBuffers[assignedHandle] = std::move(sbInfo); // unique_ptr �̵�

    dprintf("Request audio playback: %s (Handle: %d)", fileInfo.fileName.c_str(), assignedHandle);

    return assignedHandle;
}

void AudioManager::StopAudio(int handle) {
    auto it = m_soundBuffers.find(handle);
    if (it != m_soundBuffers.end()) {
        SoundBufferInfo* sbInfo = it->second.get();
        EnterCriticalSection(&sbInfo->cr_section);
        sbInfo->status = PCM_STOP; // �����忡 ���� ��ȣ
        if (sbInfo->lpDSBuffer) {
            sbInfo->lpDSBuffer->Stop();
            sbInfo->lpDSBuffer->SetCurrentPosition(0);
        }
        LeaveCriticalSection(&sbInfo->cr_section);
        // �����尡 ����� ������ ��ٸ� (���� ����, �ڿ� ������ Ȯ���� �Ϸ���)
        if (sbInfo->hLoopThread) {
            // �����尡 SetEvent�� ��ٸ��� �ִٸ� Signal
            SetEvent(sbInfo->hEvent[0]); // Sleep(1) ������ ����� ����
            WaitForSingleObject(sbInfo->hLoopThread, INFINITE); // ������ ���� ���
            CloseHandle(sbInfo->hLoopThread);
            sbInfo->hLoopThread = NULL;
        }
        m_soundBuffers.erase(it); // �ʿ��� ���� �� unique_ptr �Ҹ� (SoundBufferInfo �Ҹ��� ȣ��)
#ifdef _DEBUG
        dprintf("Stop audio playback: Handle: %d", handle);
#endif
    }
}

void AudioManager::StopAllAudio() {
    // ��� ���� ���۸� �������� ��ȸ�ϸ� ���� (�ʿ��� ��� ���Ű� ����)
    // ���� �����ϰų�, ������ ���ͷ����͸� ����ϴ� ���� ������ �� �ֽ��ϴ�.
    // ���⼭�� �ڵ��� ��Ƽ� ���������� StopAudio�� ȣ���ϴ� ����� ���
    std::vector<int> handlesToStop;
    for (const auto& pair : m_soundBuffers) {
        handlesToStop.push_back(pair.first);
    }
    for (int handle : handlesToStop) {
        if ((find(m_uniqueHandle.begin(), m_uniqueHandle.end(), handle)) == m_uniqueHandle.end()) {
            StopAudio(handle);
        }
    }
    // m_soundBuffers�� StopAudio���� �̹� �����ϹǷ� ���⼭�� clear() ���ʿ�
}


void AudioManager::PauseAudio(int handle) {
    auto it = m_soundBuffers.find(handle);
    if (it != m_soundBuffers.end()) {
        SoundBufferInfo* sbInfo = it->second.get();
        EnterCriticalSection(&sbInfo->cr_section);
        if (sbInfo->lpDSBuffer && sbInfo->status == PCM_PLAY) {
            sbInfo->lpDSBuffer->Stop();
            sbInfo->status = PCM_FADEOUTBREAK; // �ӽ÷� PCM_FADEOUTBREAK ��� (����� ���߰� ���¸� ����)
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
        if (sbInfo->lpDSBuffer && sbInfo->status == PCM_FADEOUTBREAK) { // �Ͻ����� ���¿����� �簳
            sbInfo->lpDSBuffer->Play(0, 0, 0);
            sbInfo->status = PCM_PLAY;
        }
        LeaveCriticalSection(&sbInfo->cr_section);
        dprintf("Resume audio playback: Handle: %d", handle);
    }
}

void AudioManager::SetVolume(float volume) {
    // 0.0f ~ 1.0f ������ DirectSound ���ú��� ��ȯ
    LONG dsVolume = DSBVOLUME_MIN; // �⺻ �ּҰ� (���Ұ�)
    if (volume > 0.0f) {
        // DirectSound ������ �α� �������̹Ƿ� log10�� ���
        // DSBVOLUME_MAX�� 0, DSBVOLUME_MIN�� -10000
        // -10000�� �� 0.0001 (0.01%)�� �ش��ϹǷ�, 0.0001���� 1.0���� ����.
        // float normalizedVolume = std::max(0.0001f, volume); // 0���� ����
        // dsVolume = static_cast<LONG>(std::log10(normalizedVolume) * 2000); // 20log10(amp) * 100

        // soundDS.cpp�� ���Ŀ� �°� �ٽ� ���� (bgmVolume 0-10 ������ ����)
        // 0.0 ~ 1.0 ������ 0 ~ 10 �����Ϸ� ���� ���
        float scaledVolume = volume * 10.0f;
        if (scaledVolume <= 0.0f) {
            dsVolume = DSBVOLUME_MIN;
        }
        else {
            // (�Է� ���� / �ִ� ����) * 10000 (DSBVOLUME_MAX - DSBVOLUME_MIN)
            // ���⼭�� soundDS.cpp�� log10((volume/10.0f)*i/16.0f) * 40 * 100 ���İ� �����ϰ�
            // 0.0f ~ 1.0f ������ ���� dsVolume�� ����մϴ�.
            // 0.01f�� �ּҰ����� �Ͽ� �α� ��� �� ������ ������ �մϴ�.
            float effectiveVolume = std::max(0.01f, volume); // 0.01f ���Ϸ� �������� �ʵ���
            dsVolume = static_cast<LONG>(std::log10(effectiveVolume) * 2000); // 20 * 100 * log10(volume)
        }
    }

    // �� ���۰� �ִٸ� �� ���� ���� ����
    if (m_pDSPrimaryBuffer) {
        m_pDSPrimaryBuffer->SetVolume(dsVolume);
    }
    // �Ǵ� ��� Ȱ��ȭ�� ���� ���ۿ� ���� ����.
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
        EnterCriticalSection(&sbInfo->cr_section); // ������ �����ϰ� ����
        CL_PCMSTATUS currentStatus = sbInfo->status;
        LeaveCriticalSection(&sbInfo->cr_section);

        if (currentStatus == PCM_PLAY) {
            DWORD status;
            if (sbInfo->lpDSBuffer && SUCCEEDED(sbInfo->lpDSBuffer->GetStatus(&status))) {
                if (status & DSBSTATUS_PLAYING) {
                    return PlaybackState::Playing;
                }
                else {
                    // �����忡�� PCM_PLAY�� ���������� DirectSound ���۰� ����ٸ� Stopped
                    return PlaybackState::Stopped;
                }
            }
            return PlaybackState::Stopped; // ���۰� ���ų� ���� �������� ����
        }
        else if (currentStatus == PCM_FADEOUTBREAK) { // �Ͻ����� ���·� ���
            return PlaybackState::Paused;
        }
        else {
            return PlaybackState::Stopped;
        }
    }
    return PlaybackState::Stopped; // �ش� �ڵ��� ã�� �� ����
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
    dsbd.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_STATIC | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS; // ���� ��ġ �������� �÷��� �߰�
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
// ���� ��� ���� ������ �Լ�
// soundDS.cpp�� SoundLoop �Լ��� ������ ������ �մϴ�.
// ===============================================
DWORD WINAPI AudioManager::SoundLoop(LPVOID param) {
    SoundBufferInfo* lpSoundBuffer = static_cast<SoundBufferInfo*>(param);
    if (!lpSoundBuffer || !lpSoundBuffer->lpDSBuffer) {
        ExitThread(FALSE);
        return 0L;
    }

    // ====================================================================
    // �߿� ����: �����忡�� ��� ���� ������ ������ �ٽ� �����մϴ�.
    // �̴� �����尡 �����ٸ��Ǿ� ����� �� Ȥ�ö� ������ �ʱ�ȭ�ǰų� ����Ǵ� ���� �����մϴ�.
    // ====================================================================
    LONG currentVolume;
    HRESULT hrGetVolume = lpSoundBuffer->lpDSBuffer->GetVolume(&currentVolume);
    if (FAILED(hrGetVolume)) {
        dprintf("[Error] Failed to get volume. (Handle: %d) HR: 0x%08X", lpSoundBuffer->handle, hrGetVolume);
    }
    else {
        // �̹� PlayAudio���� ���������Ƿ� �״�� �����ϰų�, �ʿ�� �缳��
        // lpSoundBuffer->lpDSBuffer->SetVolume(currentVolume); // �̹� ������ ������ �״�� ����
    }


    EnterCriticalSection(&lpSoundBuffer->cr_section);
    lpSoundBuffer->status = PCM_PLAY;
    HRESULT hr = lpSoundBuffer->lpDSBuffer->Play(0, 0, (lpSoundBuffer->repeat == 0) ? DSBPLAY_LOOPING : 0);
    LeaveCriticalSection(&lpSoundBuffer->cr_section);

    if (FAILED(hr)) {
        dprintf("[Error] Failed to start audio playback. (Handle: %d) HR: 0x%08X", lpSoundBuffer->handle, hr);
        lpSoundBuffer->status = PCM_STOP; // ���� �� ���� ����
        ExitThread(FALSE);
        return 0L;
    }

    // ��� �Ϸ� ��� ���� (soundDS.cpp�� SoundLoop�� ����)
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
            // DirectSound ������ ��� ���¸� �����ɴϴ�.
            HRESULT hrStatus = lpSoundBuffer->lpDSBuffer->GetStatus(&status);
            if (FAILED(hrStatus)) {
                dprintf("[Error] Failed to GetStatus. (Handle: %d) HR: 0x%08X", lpSoundBuffer->handle, hrStatus);
                lpSoundBuffer->status = PCM_STOP; // ���� �߻� �� ����
                LeaveCriticalSection(&lpSoundBuffer->cr_section);
                break;
            }

            if (!(status & DSBSTATUS_PLAYING)) {
                // ����� ������ ��
                if (lpSoundBuffer->repeat == 0) { // ���� �ݺ� ���
                    // DSBPLAY_LOOPING �÷��׸� ��������Ƿ� ���⿡ �����ϸ� ����ġ ���� ����
                    dprintf("[Error] Buffer stops unexpectedly (Infinite loop mode, Handle: %d). Try to restart.", lpSoundBuffer->handle);
                    lpSoundBuffer->lpDSBuffer->SetCurrentPosition(0); // ��ġ �ʱ�ȭ
                    lpSoundBuffer->lpDSBuffer->Play(0, 0, DSBPLAY_LOOPING); // �ٽ� ���
                }
                else { // ���� ��� ��� (�Ǵ� �ݺ� Ƚ����ŭ ��� �Ϸ�)
                    // �������� ��� ����
                    lpSoundBuffer->status = PCM_STOP;
                    LeaveCriticalSection(&lpSoundBuffer->cr_section);
                    break;
                }
            }
        }
        LeaveCriticalSection(&lpSoundBuffer->cr_section);

        // CPU ��뷮�� ���̱� ���� ��� ���
        Sleep(5); // 10ms -> 5ms�� �ٿ��� ������ ���� �õ�
    }

    EnterCriticalSection(&lpSoundBuffer->cr_section);
    if (lpSoundBuffer->lpDSBuffer) {
        lpSoundBuffer->lpDSBuffer->Stop();
        lpSoundBuffer->lpDSBuffer->SetCurrentPosition(0);
    }
    lpSoundBuffer->status = PCM_STOP; // ���� ���� ����
    LeaveCriticalSection(&lpSoundBuffer->cr_section);

    // ������ ���� �� �̺�Ʈ ��ȣ (�ʿ�� SetEvent(lpSoundBuffer->hEvent[0]); )
    //std::cout << "[SoundLoop] ������ ���� (�ڵ�: " << lpSoundBuffer->handle << ")" << std::endl;
    ExitThread(TRUE);
    return 0L;
}

void AudioManager::ContinuePlay() {
    // DataManager�� ���� ���� main_idx�� sub_idx�� �����ɴϴ�.
    // DataManager�� ScanProcess�� ������Ʈ�ϴ� currentMainIndex��
    // ����ڰ� �����ϴ� currentSubIndex�� �����մϴ�.
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
        // PlayAudio ���ο��� StopAllAudio()�� ȣ���ϹǷ� ���⼭ ���� StopAllAudio()�� ȣ���� �ʿ� ����.
        PlayAudio(fileInfo, 1); // 1�� ���

        // ��� �� sub_idx 1 ����
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

    // ���� Sub Index���� 1�� ���ҽ�ŵ�ϴ�. (���� ����)
    current_sub_idx = std::max(1, (current_sub_idx - 1));
    int target_sub_idx = std::max(1, (current_sub_idx - 1));

    // DataManager�� sub_idx�� ���� ������Ʈ
    DataManager::GetInstance().SetCurrentSubIndex(current_sub_idx);

    dprintf("Try playing the previous audio: Main: %d, Sub: %d", main_idx, target_sub_idx);
    int pakFileIndex = DataManager::GetInstance().SearchIndex(main_idx, target_sub_idx);

    if (pakFileIndex != -1) {
        const PakFileInfo& fileInfo = DataManager::GetInstance().GetPakFiles()[pakFileIndex];
        PlayAudio(fileInfo, 1); // 1�� ���
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
    va_start(args, numArgs); // numArgs ���� ���ں��� ����

    std::vector<int> argValues;
    std::vector<PakFileInfo> pakArray;
    for (int i = 0; i < numArgs; ++i) {
        int currentArg = va_arg(args, int); // ���� int Ÿ�� ���ڸ� ������
        argValues.push_back(currentArg);
    }
    va_end(args); // va_list ��� ����

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
    if (main_idx < 0) { // sub_idx�� 0���� ������ ��ȿ���� ����
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
        hNum = PlayAudio(fileInfo, 1); // 1�� ���
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