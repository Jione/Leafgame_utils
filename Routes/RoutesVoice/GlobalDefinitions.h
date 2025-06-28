#pragma once
#pragma warning(disable : 4996)

#include <string>
#include <vector>
#include <cstdint> // uint8_t, uint32_t ��
#include <windows.h> // HANDLE, LPDIRECTSOUNDBUFFER
#include <thread>   // std::thread
#include <atomic>   // std::atomic
#include <dsound.h>

// ���� �޽��� ����
#define WM_TRAY_SETUP				WM_USER + 1
#define WM_TRAY_MENU				WM_USER + 2
#define WM_DEBUG_MESSAGE			WM_USER + 3
#define WM_PRINT_MESSAGE			WM_USER + 4


// PAK ���� ��� ����ü
// �� ����ü�� ���� PAK ������ ����� ���� �����Ǿ�� �մϴ�.
// ����: ���� �ñ״�ó, ���� ���� ��
#pragma pack(push, 1) // 1����Ʈ ����
struct PakHeader {
    uint32_t signature; // PAK �������� ��Ÿ���� �ñ״�ó (��: 'PAK ' �Ǵ� �ٸ� Magic Number)
    uint32_t numFiles;  // PAK ���� ���Ե� ������ �� ����
    // ��Ÿ �ʿ��� ��� ����...
};

// PAK ���� ���� �� ���Ͽ� ���� ���� ����ü
struct PakFileInfo {
    std::string fileName;    // ��: "00010_001_03.OGG" (���� ���ϸ�)
    uint32_t dataOffset = 0; // PAK ���� ������ ���� �����Ͱ� ���۵Ǵ� ������
    uint32_t dataLength = 0; // ���� �������� ���� (����Ʈ)
    bool isFirst = true;
    // ��Ÿ �ʿ��� ���Ϻ� ����...
};

// ������ �Ŵ��� ���� ��
enum DM_STATUS {
    WAIT = 0,
    LOAD,
    BUSY,
    FAIL,
};

// WAV RIFF ����ü
// RIFF ���
struct RiffHeader {
    uint32_t chunkId;      // "RIFF"
    uint32_t chunkSize;    // ���� ũ�� - 8 ����Ʈ
    uint32_t format;       // "WAVE"
};

// fmt ûũ
struct FmtChunk {
    uint32_t chunkId;      // "fmt "
    uint32_t chunkSize;    // fmt ûũ�� ũ�� (�Ϲ������� 16 �Ǵ� 18)
    uint16_t audioFormat;  // ����� ���� (1 = PCM)
    uint16_t numChannels;  // ä�� �� (1 = ���, 2 = ���׷���)
    uint32_t sampleRate;   // ���÷���Ʈ (Hz)
    uint32_t byteRate;     // byteRate = sampleRate * numChannels * bitsPerSample / 8
    uint16_t blockAlign;   // blockAlign = numChannels * bitsPerSample / 8
    uint16_t bitsPerSample; // ���ô� ��Ʈ �� (��: 8, 16, 24, 32)
    // uint16_t extraParamSize; // chunkSize�� 16 �̻��� ��� �߰� �ʵ�
};

// Data ûũ
struct DataChunk {
    uint32_t chunkId;      // "data"
    uint32_t chunkSize;    // �������� ũ�� (����Ʈ)
};

// DirectSound ���� ���� ����
struct IDirectSoundBuffer8; // LPDIRECTSOUNDBUFFER�� �ش�

enum CL_PCMSTATUS {
    PCM_STOP = 0,
    PCM_PLAY,
    PCM_FADEOUT,
    PCM_FADEOUTBREAK,
};

struct SoundBufferInfo {
    int                     handle;         // ���� �ڵ� (AudioManager ���ο��� ����)
    CL_PCMSTATUS            status;         // ���� ��� ����
    int                     repeat;         // �ݺ� Ƚ�� (0: ���� �ݺ�, 1: �� �� ��� ��)
    bool                    bFade;          // ���̵�ƿ� ������ ����
    WORD                    wFormatTag;     // WAV ���� �±� (PCM, OGG ��)

    LPDIRECTSOUNDBUFFER     lpDSBuffer;     // DirectSound ���� ���� ������

    HANDLE                  hLoopThread;    // ��� ������ ����� ������ �ڵ�
    CRITICAL_SECTION        cr_section;     // ũ��Ƽ�� ���� (������ ����ȭ��)
    HANDLE                  hEvent[1];      // �̺�Ʈ �ڵ� (��� �Ϸ� �� ��ȣ��)

    // ���� ����� �������� ��Ÿ����
    PakFileInfo             pakFileInfo;    // ���� PAK ���� ���� (�ĺ���)

    SoundBufferInfo() : handle(-1), status(PCM_STOP), repeat(0), bFade(false), wFormatTag(WAVE_FORMAT_PCM),
        lpDSBuffer(nullptr), hLoopThread(NULL)
    {
        InitializeCriticalSection(&cr_section);
        hEvent[0] = CreateEvent(NULL, FALSE, FALSE, NULL); // auto-reset, non-signaled
    }

    ~SoundBufferInfo() {
        if (lpDSBuffer) {
            lpDSBuffer->Stop();
            lpDSBuffer->Release();
            lpDSBuffer = nullptr;
        }
        if (hLoopThread) {
            // �����尡 ���� ���̸� ���� ��ȣ�� ���� �� ��ٸ� (�ʿ��)
            // ���� ���������� �����尡 ���� ����ǵ��� Signal �� ���
            CloseHandle(hLoopThread);
            hLoopThread = NULL;
        }
        if (hEvent[0]) {
            CloseHandle(hEvent[0]);
            hEvent[0] = NULL;
        }
        DeleteCriticalSection(&cr_section);
    }
};
#pragma pack(pop)

// ����� ���
void dprintf(const char* formatstring, ...);

// ���� ���μ����� ������ �ڵ� ��� (Console/Window, Dll/Exe �ڵ� �˻�)
HWND GetWinHandle();

// ������ ��� �޽��� ���
LPCWSTR GetPrintMessage();
// ������ ����� �޽��� ���
LPCSTR GetDebugMessage();

// �޽��� ���� ���� Ŭ����
class PrintMessage {
public:
    static PrintMessage& GetInstance(); // �̱���

    // ����Ʈ �޽��� ������ ����
    void StartPrintMessageThread();
    // ����Ʈ �޽��� ������ ����
    void StopPrintMessageThread();
    // ����Ʈ �޽��� ������ ���� ���
    void WaitStopPrintMessageThread();


private:
    PrintMessage(); // �̱���
    ~PrintMessage();
    PrintMessage(const PrintMessage&) = delete;
    PrintMessage& operator=(const PrintMessage&) = delete;

    void PrintMessageLoop();  // ���� �۵� ����

    std::atomic<bool> m_stopPrintMessageThread; // ������ ���� �÷���
    std::thread m_printMessageThread;           // ����Ʈ �޽��� ������ ��ü
};