#include "GlobalDefinitions.h"
#include "DataManager.h"
#include "ScanProcess.h"
#include "AudioManager.h"
#include "StringUtil.h"

// x86 빌드 시 주의 사항 (for Windows XP)
// VS2022 기준 메뉴-도구(T)-도구 및 기능 가져오기(T)-개별 구성 요소 탭- v141 검색 후 x86 관련 툴 업데이트
// DirectX SDK, Windows SDK 7.1A 설치 필요
// 참고 링크: https://en.wikipedia.org/wiki/Microsoft_Windows_SDK
// https://www.microsoft.com/ko-kr/download/details.aspx?id=6812
// https://www.microsoft.com/en-us/download/details.aspx?id=8442

static HWND gWindowHandle;
static wchar_t gPrintMessage[1024];
static char gDebugMessage[256];

// 디버그 문자열 출력 함수
void dprintf(const char* formatstring, ...) {
    int nSize = 0;
    memset(gDebugMessage, 0, sizeof(gDebugMessage));
    va_list args;
    va_start(args, formatstring);
    nSize = _vsnprintf(gDebugMessage, sizeof(gDebugMessage), formatstring, args);
#ifdef _DEBUG
    OutputDebugString(gDebugMessage);
    OutputDebugString("\n");
#endif
    va_end(args);
    SendMessageA(gWindowHandle, WM_DEBUG_MESSAGE, 0, 0);
}

// 윈도우 핸들로 프로세스 아이디 얻기
static ULONG ProcIDFromWnd(HWND hwnd) {
    ULONG idProc;
    GetWindowThreadProcessId(hwnd, &idProc);
    return idProc;
}

// 프로세스 아이디로 윈도우 핸들 얻기
static HWND GetHWind(ULONG pid) {
    HWND tempHwnd = FindWindow(NULL, NULL);   // 최상위 윈도우 핸들 찾기 
    while (tempHwnd != NULL) {
        if (GetParent(tempHwnd) == NULL)             // 최상위 핸들인지 체크, 버튼 등도 핸들을 가질 수 있으므로 무시하기 위해 
            if (pid == ProcIDFromWnd(tempHwnd))
                return tempHwnd;
        tempHwnd = GetWindow(tempHwnd, GW_HWNDNEXT); // 다음 윈도우 핸들 찾기 
    }
    return GetConsoleWindow();
}

HWND GetWinHandle() {
    return (gWindowHandle != NULL) ? gWindowHandle : (gWindowHandle = GetHWind(GetCurrentProcessId()));
}

// 마지막 메시지 얻기
LPCWSTR GetPrintMessage() { return gPrintMessage; }
LPCSTR GetDebugMessage() { return gDebugMessage; }

// 메시지 서버 생성
PrintMessage::PrintMessage() {
    GetWinHandle();
    // 생성자
}

PrintMessage::~PrintMessage() {
    // 소멸자
}

PrintMessage& PrintMessage::GetInstance() {
    static PrintMessage instance;
    return instance;
}


void PrintMessage::StartPrintMessageThread() {
    if (m_printMessageThread.joinable()) {
        dprintf("Print Message thread is already running. (PrintMessage)");
        return;
    }

    // 종료 플래그 초기화
    m_stopPrintMessageThread.store(false);

    // std::thread를 사용하여 멤버 함수를 스레드로 실행
    // this 포인터를 람다 캡처 목록으로 전달하여 멤버 함수 호출
    m_printMessageThread = std::thread([this]() {
        this->PrintMessageLoop();
        });
    dprintf("Start the Print Message thread. (PrintMessage)");
}

void PrintMessage::StopPrintMessageThread() {
    if (m_printMessageThread.joinable()) {
        m_stopPrintMessageThread.store(true); // 스레드에 종료 신호
    }
}

void PrintMessage::WaitStopPrintMessageThread() {
    if (m_printMessageThread.joinable()) {
        m_printMessageThread.join();         // 스레드 종료 대기
    }
    dprintf("Print Message thread terminated. (PrintMessage)");
}

void PrintMessage::PrintMessageLoop() {
    std::string audioName = "";
    std::wstring audioWName = L"";
    std::wstring NewString = L"";
    std::wstring LastString = L"";
    int offset = 0;
    int block = 0;
    int sdtnumber = 0;
    int audioindex = 0;

    while (!m_stopPrintMessageThread.load()) {
        Sleep(200); // 0.2초당 1회 업데이트
        audioName = AudioManager::GetInstance().GetLastPlayFileName();
        if (audioName.size() == 0) {
            Sleep(100);
            audioName = AudioManager::GetInstance().GetLastPlayFileName();
        }
        if (audioName.size() == 0) {
            audioWName = L"없음 (재생 대기 중)";
        }
        else {
            audioWName = StringUtil::GetInstance().StringToWstring(audioName);
        }
        offset = ScanProcess::GetInstance().ControlPlayOffset(GET_OFFSET);
        block = ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK);
        sdtnumber = ScanProcess::GetInstance().ControlPlayOffset(GET_SDT);
        audioindex = DataManager::GetInstance().GetCurrentSubIndex();
        if (offset == 0) {
            _snwprintf(gPrintMessage, (_countof(gPrintMessage) - 1), L"스크립트 번호:\t%05d\r\n블록 번호:\t%d\r\n오디오 인덱스:\t%d\r\n오디오 파일명:\t%s\0", sdtnumber, block, audioindex, audioWName.c_str());
        }
        else {
            if (offset == -1) {
                _snwprintf(gPrintMessage, (_countof(gPrintMessage) - 1), L"스크립트 번호:\t%05d\r\n블록 번호:\t%d (오프셋 -1)\r\n오디오 인덱스:\t%d\r\n오디오 파일명:\t%s\r\n\r\n\r\n[%05d]\r\n%d=-1\0", sdtnumber, block, audioindex, audioWName.c_str(), sdtnumber, block);
            }
            else if (0 < offset) {
                int idx = DataManager::GetInstance().CalculateAdjustedSubIndex((block + offset), true);
                _snwprintf(gPrintMessage, (_countof(gPrintMessage) - 1), L"스크립트 번호:\t%05d\r\n블록 번호:\t%d (오프셋 +%d)\r\n오디오 인덱스:\t%d\r\n오디오 파일명:\t%s\r\n\r\n\r\n[%05d]\r\n%d=%d\0", sdtnumber, block, offset, audioindex, audioWName.c_str(), sdtnumber, block, idx);
            }
            else {
                int idx = DataManager::GetInstance().CalculateAdjustedSubIndex((block + offset), true);
                _snwprintf(gPrintMessage, (_countof(gPrintMessage) - 1), L"스크립트 번호:\t%05d\r\n블록 번호:\t%d (오프셋 %d)\r\n오디오 인덱스:\t%d\r\n오디오 파일명:\t%s\r\n\r\n\r\n[%05d]\r\n%d=%d\r\n\r\n또는\r\n\r\n[%05d]\r\n%d=-1\0", sdtnumber, block, offset, audioindex, audioWName.c_str(), sdtnumber, block, idx, sdtnumber, (block + offset + 1));
            }
        }
        gPrintMessage[(_countof(gPrintMessage) - 1)] = L'\0';
        NewString = gPrintMessage;
        if (NewString != LastString) {
            LastString = NewString;
            SendMessageA(gWindowHandle, WM_PRINT_MESSAGE, 0, 0);
        }
    }
}