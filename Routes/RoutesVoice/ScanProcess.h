#pragma once
#include <string>
#include <atomic>   // std::atomic
#include <thread>   // std::thread
#include <windows.h> // For DWORD, LPVOID, HWND (though HWND not directly used in this module now)
#include <vector>    // for std::vector

// 대상 프로세스 정보를 저장하는 구조체
struct TargetProcessInfo {
    std::string processName;
    std::string windowCaption; // 창 캡션 (필요시 추가)
};

// 프로세스 메모리에서 읽어들일 현재 상태를 저장하는 구조체
struct ProcessMemoryStatus {
    // 실제 메모리에서 읽어온 값들 (명확한 변수명으로 변경)
    char sdtBase[4];                        // "kos" 또는 "sdt" 문자열
    DWORD textBufferCurrentOffset;          // textBuffer 기준의 현재 포인터
    DWORD textBufferBaseAddress;            // 버퍼용 텍스트가 로드된 위치 (LPVOID 캐스팅 필요)
    char textOutputBusyFlag;                // 현재 프로그램의 텍스트 출력 상태 (0=로드 안함, 1=실행 중, 2=작업 중)
    char currentScriptNumber[6];            // 현재 로드 중인 스크립트명 (%05d.sdt), %05d만 사용
    DWORD printTextBaseAddress;             // 현재 윈도우에 출력 중인 문자의 포인터 (LPVOID 캐스팅 필요)
    WORD printTextBaseCounts;               // 현재 문자 윈도우 블록 출력 길이 (0xFFFF일 경우 끝까지 읽기)

    // 현재 재생 프로그램 상태 저장
    char textBuffer[0x400];                 // 화면에 출력되는 텍스트의 버퍼
    int scriptIndex;                        // 스크립트의 블록 인덱스 값 (재처리 후 sub_idx로 연결)
    int voiceFlag;                          // 보이스 블록 플래그
    int voiceIndexBuffer;                   // 현재 보이스 블록 저장
    bool passVoicePlay;                     // 보이스 재생 일시정지 플래그
    int playOffset;                         // 재생 오프셋 조정

    // 생성자에서 기본값 초기화
    ProcessMemoryStatus() :
        textBufferCurrentOffset(0),
        textBufferBaseAddress(0),
        textOutputBusyFlag(0),
        printTextBaseAddress(0),
        printTextBaseCounts(0xFFFE), // 기본값을 0xFFFE로 설정
        scriptIndex(0),
        voiceFlag(0),
        voiceIndexBuffer(0),
        passVoicePlay(false),
        playOffset(0)
    {
        memset(sdtBase, 0, sizeof(sdtBase));
        memset(currentScriptNumber, 0, sizeof(currentScriptNumber));
        memset(textBuffer, 0, sizeof(textBuffer));
    }
};

enum SCAN_STATUS {
    ADD_OFFSET = 0, // 오프셋 1 증가
    DEC_OFFSET,     // 오프셋 1 감소
    RESET_OFFSET,   // 오프셋 초기화
    GET_OFFSET,     // 현재 오프셋 얻기
    SET_OFFSET,     // 오프셋 값 설정
    GET_BLOCK,      // 현재 블록 얻기
    SET_BLOCK,      // 블록 값 설정
    GET_SDT,        // 현재 SDT 값 설정
};


// ScanProcess 기능을 캡슐화하는 클래스
class ScanProcess {
public:
    static ScanProcess& GetInstance(); // 싱글톤

    // 스캔 스레드 시작 (인자 제거)
    void StartScanThread();
    // 스캔 스레드 종료
    void StopScanThread();
    // 스캔 스레드 종료 대기
    void WaitStopScanThread();
    // 보이스 블록 재생 오프셋 조정
    int ControlPlayOffset(SCAN_STATUS mode, int value = 0);
    // 대상 프로세스 핸들 반환
    HANDLE& GetTargetProcessHandle() { return m_hProcess; }
    // 대상 프로세스 경로 반환
    std::wstring& GetTargetProcessPath() { return m_targetProcessPath; }

private:
    ScanProcess(); // 싱글톤
    ~ScanProcess();
    ScanProcess(const ScanProcess&) = delete;
    ScanProcess& operator=(const ScanProcess&) = delete;

    // FindProcess 함수 (TargetProcessInfo 기반)
    HANDLE FindProcess(const TargetProcessInfo& targetInfo);
    // 유효한 프로세스 핸들을 검색하고 설정하는 함수
    bool FindValidProcessHandle(HANDLE& outProcessId, HANDLE& outHProcess);

    void ProcessMemoryScanLoop(); // 실제 스캔 로직

    void EvaluateProcessStatus(); // m_processStatus 정보를 기반으로 voice 재생 상태를 확인/재생
    bool CalculateCurrentBlockIndex(); // m_processStatus 정보를 기반으로 scriptIndex, voiceFlag 값을 계산
    int BlockCountFromBuffer();        // m_processStatus.textBuffer 문자열의 전체 문자열 블록 수를 계산

    // textBuff에서 textLen만큼 검색하여 voiceFlag, blockCount를 변경
    void BlockCountFromPointer(const char* textBuff, int textLen, int& voiceFlag, int& blockCount);

    // idx와 printCount를 기반으로 유니크 재생 실행여부 확인
    bool UniqueChecker();

    // 로드 된 SCN 파일이 첫 번째 문자인지 확인
    bool CheckFirstLoad(const char* sdtBuffer, SIZE_T readSize) const;

    std::atomic<bool> m_stopScanThread; // 스레드 종료 플래그
    std::thread m_scanThread;           // 스캔 스레드 객체

    std::vector<TargetProcessInfo> m_targetProcesses;   // 대상 프로세스 정보
    std::wstring m_targetProcessPath;                    // 타겟 프로세스의 절대 경로
    ProcessMemoryStatus m_processStatus;                // 프로세스 메모리 상태를 저장하는 구조체
    HANDLE m_hProcess;                                  // 타겟 프로세스 핸들
    char m_sdtBuffer[0x10000];                          // sdt 텍스트 버퍼
    int m_blockCount;                                   // 마지막 block 카운터
    int m_bbCount;                                      // 계산 이전의 block 카운터
    bool m_cinema;                                      // 영상 여부 확인
    bool m_isLoad;                                        // 로드 여부 확인
};