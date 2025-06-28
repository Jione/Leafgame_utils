#pragma once
#pragma warning(disable : 4996)

#include <string>
#include <vector>
#include <cstdint> // uint8_t, uint32_t 등
#include <windows.h> // HANDLE, LPDIRECTSOUNDBUFFER
#include <thread>   // std::thread
#include <atomic>   // std::atomic
#include <dsound.h>

// 유저 메시지 정의
#define WM_TRAY_SETUP				WM_USER + 1
#define WM_TRAY_MENU				WM_USER + 2
#define WM_DEBUG_MESSAGE			WM_USER + 3
#define WM_PRINT_MESSAGE			WM_USER + 4


// PAK 파일 헤더 구조체
// 이 구조체는 실제 PAK 파일의 헤더에 따라 조정되어야 합니다.
// 예시: 파일 시그니처, 파일 개수 등
#pragma pack(push, 1) // 1바이트 정렬
struct PakHeader {
    uint32_t signature; // PAK 파일임을 나타내는 시그니처 (예: 'PAK ' 또는 다른 Magic Number)
    uint32_t numFiles;  // PAK 내에 포함된 파일의 총 개수
    // 기타 필요한 헤더 정보...
};

// PAK 파일 내의 각 파일에 대한 정보 구조체
struct PakFileInfo {
    std::string fileName;    // 예: "00010_001_03.OGG" (실제 파일명)
    uint32_t dataOffset = 0; // PAK 파일 내에서 실제 데이터가 시작되는 오프셋
    uint32_t dataLength = 0; // 실제 데이터의 길이 (바이트)
    bool isFirst = true;
    // 기타 필요한 파일별 정보...
};

// 데이터 매니저 상태 값
enum DM_STATUS {
    WAIT = 0,
    LOAD,
    BUSY,
    FAIL,
};

// WAV RIFF 구조체
// RIFF 헤더
struct RiffHeader {
    uint32_t chunkId;      // "RIFF"
    uint32_t chunkSize;    // 파일 크기 - 8 바이트
    uint32_t format;       // "WAVE"
};

// fmt 청크
struct FmtChunk {
    uint32_t chunkId;      // "fmt "
    uint32_t chunkSize;    // fmt 청크의 크기 (일반적으로 16 또는 18)
    uint16_t audioFormat;  // 오디오 포맷 (1 = PCM)
    uint16_t numChannels;  // 채널 수 (1 = 모노, 2 = 스테레오)
    uint32_t sampleRate;   // 샘플레이트 (Hz)
    uint32_t byteRate;     // byteRate = sampleRate * numChannels * bitsPerSample / 8
    uint16_t blockAlign;   // blockAlign = numChannels * bitsPerSample / 8
    uint16_t bitsPerSample; // 샘플당 비트 수 (예: 8, 16, 24, 32)
    // uint16_t extraParamSize; // chunkSize가 16 이상일 경우 추가 필드
};

// Data 청크
struct DataChunk {
    uint32_t chunkId;      // "data"
    uint32_t chunkSize;    // 데이터의 크기 (바이트)
};

// DirectSound 관련 전방 선언
struct IDirectSoundBuffer8; // LPDIRECTSOUNDBUFFER에 해당

enum CL_PCMSTATUS {
    PCM_STOP = 0,
    PCM_PLAY,
    PCM_FADEOUT,
    PCM_FADEOUTBREAK,
};

struct SoundBufferInfo {
    int                     handle;         // 사운드 핸들 (AudioManager 내부에서 관리)
    CL_PCMSTATUS            status;         // 현재 재생 상태
    int                     repeat;         // 반복 횟수 (0: 무한 반복, 1: 한 번 재생 등)
    bool                    bFade;          // 페이드아웃 중인지 여부
    WORD                    wFormatTag;     // WAV 포맷 태그 (PCM, OGG 등)

    LPDIRECTSOUNDBUFFER     lpDSBuffer;     // DirectSound 보조 버퍼 포인터

    HANDLE                  hLoopThread;    // 재생 루프를 담당할 스레드 핸들
    CRITICAL_SECTION        cr_section;     // 크리티컬 섹션 (스레드 동기화용)
    HANDLE                  hEvent[1];      // 이벤트 핸들 (재생 완료 등 신호용)

    // 원래 오디오 데이터의 메타정보
    PakFileInfo             pakFileInfo;    // 원본 PAK 파일 정보 (식별용)

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
            // 스레드가 실행 중이면 종료 신호를 보낸 후 기다림 (필요시)
            // 실제 구현에서는 스레드가 정상 종료되도록 Signal 등 고려
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

// 디버그 출력
void dprintf(const char* formatstring, ...);

// 현재 프로세스의 윈도우 핸들 얻기 (Console/Window, Dll/Exe 자동 검사)
HWND GetWinHandle();

// 마지막 출력 메시지 얻기
LPCWSTR GetPrintMessage();
// 마지막 디버그 메시지 얻기
LPCSTR GetDebugMessage();

// 메시지 서버 생성 클래스
class PrintMessage {
public:
    static PrintMessage& GetInstance(); // 싱글톤

    // 프린트 메시지 스레드 시작
    void StartPrintMessageThread();
    // 프린트 메시지 스레드 종료
    void StopPrintMessageThread();
    // 프린트 메시지 스레드 종료 대기
    void WaitStopPrintMessageThread();


private:
    PrintMessage(); // 싱글톤
    ~PrintMessage();
    PrintMessage(const PrintMessage&) = delete;
    PrintMessage& operator=(const PrintMessage&) = delete;

    void PrintMessageLoop();  // 실제 작동 루프

    std::atomic<bool> m_stopPrintMessageThread; // 스레드 종료 플래그
    std::thread m_printMessageThread;           // 프린트 메시지 스레드 객체
};