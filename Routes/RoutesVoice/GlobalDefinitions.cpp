#include "GlobalDefinitions.h"
#include "DataManager.h"
#include "ScanProcess.h"
#include "AudioManager.h"
#include "StringUtil.h"

// x86 ���� �� ���� ���� (for Windows XP)
// VS2022 ���� �޴�-����(T)-���� �� ��� ��������(T)-���� ���� ��� ��- v141 �˻� �� x86 ���� �� ������Ʈ
// DirectX SDK, Windows SDK 7.1A ��ġ �ʿ�
// ���� ��ũ: https://en.wikipedia.org/wiki/Microsoft_Windows_SDK
// https://www.microsoft.com/ko-kr/download/details.aspx?id=6812
// https://www.microsoft.com/en-us/download/details.aspx?id=8442

static HWND gWindowHandle;
static wchar_t gPrintMessage[1024];
static char gDebugMessage[256];

// ����� ���ڿ� ��� �Լ�
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

// ������ �ڵ�� ���μ��� ���̵� ���
static ULONG ProcIDFromWnd(HWND hwnd) {
    ULONG idProc;
    GetWindowThreadProcessId(hwnd, &idProc);
    return idProc;
}

// ���μ��� ���̵�� ������ �ڵ� ���
static HWND GetHWind(ULONG pid) {
    HWND tempHwnd = FindWindow(NULL, NULL);   // �ֻ��� ������ �ڵ� ã�� 
    while (tempHwnd != NULL) {
        if (GetParent(tempHwnd) == NULL)             // �ֻ��� �ڵ����� üũ, ��ư � �ڵ��� ���� �� �����Ƿ� �����ϱ� ���� 
            if (pid == ProcIDFromWnd(tempHwnd))
                return tempHwnd;
        tempHwnd = GetWindow(tempHwnd, GW_HWNDNEXT); // ���� ������ �ڵ� ã�� 
    }
    return GetConsoleWindow();
}

HWND GetWinHandle() {
    return (gWindowHandle != NULL) ? gWindowHandle : (gWindowHandle = GetHWind(GetCurrentProcessId()));
}

// ������ �޽��� ���
LPCWSTR GetPrintMessage() { return gPrintMessage; }
LPCSTR GetDebugMessage() { return gDebugMessage; }

// �޽��� ���� ����
PrintMessage::PrintMessage() {
    GetWinHandle();
    // ������
}

PrintMessage::~PrintMessage() {
    // �Ҹ���
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

    // ���� �÷��� �ʱ�ȭ
    m_stopPrintMessageThread.store(false);

    // std::thread�� ����Ͽ� ��� �Լ��� ������� ����
    // this �����͸� ���� ĸó ������� �����Ͽ� ��� �Լ� ȣ��
    m_printMessageThread = std::thread([this]() {
        this->PrintMessageLoop();
        });
    dprintf("Start the Print Message thread. (PrintMessage)");
}

void PrintMessage::StopPrintMessageThread() {
    if (m_printMessageThread.joinable()) {
        m_stopPrintMessageThread.store(true); // �����忡 ���� ��ȣ
    }
}

void PrintMessage::WaitStopPrintMessageThread() {
    if (m_printMessageThread.joinable()) {
        m_printMessageThread.join();         // ������ ���� ���
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
        Sleep(200); // 0.2�ʴ� 1ȸ ������Ʈ
        audioName = AudioManager::GetInstance().GetLastPlayFileName();
        if (audioName.size() == 0) {
            Sleep(100);
            audioName = AudioManager::GetInstance().GetLastPlayFileName();
        }
        if (audioName.size() == 0) {
            audioWName = L"���� (��� ��� ��)";
        }
        else {
            audioWName = StringUtil::GetInstance().StringToWstring(audioName);
        }
        offset = ScanProcess::GetInstance().ControlPlayOffset(GET_OFFSET);
        block = ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK);
        sdtnumber = ScanProcess::GetInstance().ControlPlayOffset(GET_SDT);
        audioindex = DataManager::GetInstance().GetCurrentSubIndex();
        if (offset == 0) {
            _snwprintf(gPrintMessage, (_countof(gPrintMessage) - 1), L"��ũ��Ʈ ��ȣ:\t%05d\r\n��� ��ȣ:\t%d\r\n����� �ε���:\t%d\r\n����� ���ϸ�:\t%s\0", sdtnumber, block, audioindex, audioWName.c_str());
        }
        else {
            if (offset == -1) {
                _snwprintf(gPrintMessage, (_countof(gPrintMessage) - 1), L"��ũ��Ʈ ��ȣ:\t%05d\r\n��� ��ȣ:\t%d (������ -1)\r\n����� �ε���:\t%d\r\n����� ���ϸ�:\t%s\r\n\r\n\r\n[%05d]\r\n%d=-1\0", sdtnumber, block, audioindex, audioWName.c_str(), sdtnumber, block);
            }
            else if (0 < offset) {
                int idx = DataManager::GetInstance().CalculateAdjustedSubIndex((block + offset), true);
                _snwprintf(gPrintMessage, (_countof(gPrintMessage) - 1), L"��ũ��Ʈ ��ȣ:\t%05d\r\n��� ��ȣ:\t%d (������ +%d)\r\n����� �ε���:\t%d\r\n����� ���ϸ�:\t%s\r\n\r\n\r\n[%05d]\r\n%d=%d\0", sdtnumber, block, offset, audioindex, audioWName.c_str(), sdtnumber, block, idx);
            }
            else {
                int idx = DataManager::GetInstance().CalculateAdjustedSubIndex((block + offset), true);
                _snwprintf(gPrintMessage, (_countof(gPrintMessage) - 1), L"��ũ��Ʈ ��ȣ:\t%05d\r\n��� ��ȣ:\t%d (������ %d)\r\n����� �ε���:\t%d\r\n����� ���ϸ�:\t%s\r\n\r\n\r\n[%05d]\r\n%d=%d\r\n\r\n�Ǵ�\r\n\r\n[%05d]\r\n%d=-1\0", sdtnumber, block, offset, audioindex, audioWName.c_str(), sdtnumber, block, idx, sdtnumber, (block + offset + 1));
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