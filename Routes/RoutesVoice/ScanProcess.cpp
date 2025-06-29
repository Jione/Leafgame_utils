#include "ScanProcess.h"
#include "DataManager.h" // DataManager의 SetCurrentMainIndex를 사용하기 위해 포함
#include "AudioManager.h"
#include "GlobalDefinitions.h"
#include "ScriptStruct.h"
#include "StringUtil.h"
#include <iostream>
#include <tlhelp32.h> // For Process32First, Process32Next
#include <exception> // For std::stoi exceptions
#include <chrono>    // For std::chrono::seconds, milliseconds (Sleep)
#include <vector>    // for std::vector
#include <Psapi.h>   // For GetModuleFileNameEx

#if PSAPI_VERSION == 1
#pragma comment(lib, "Psapi.lib")
#endif // PSAPI_VERSION = 1

// 메모리 주소 정의
static LPVOID SDT_ADDR = reinterpret_cast<LPVOID>(0x4D57AC);         // sdt 경로 검사 (sdt or kos)
static LPVOID TEXT_OFFSET_ADDR = reinterpret_cast<LPVOID>(0xDB7D00); // sdt 메모리 오프셋
static LPVOID TEXT_PTR_ADDR = reinterpret_cast<LPVOID>(0xDB7D14);    // sdt 메모리 포인터
static LPVOID TEXT_BUSY_ADDR = reinterpret_cast<LPVOID>(0xDB7D18);   // 텍스트 로드 상태 검사
static LPVOID SCR_NO_ADDR = reinterpret_cast<LPVOID>(0xDE2720);      // 현재 스크립트 번호
static LPVOID CURTXT_PTR_ADDR = reinterpret_cast<LPVOID>(0x6CE0D4);  // 출력 중인 문자열 포인터
static LPVOID CURTXT_COUNT_ADDR = reinterpret_cast<LPVOID>(0x6CE0F4);// 출력 중인 문자열 블록 수
//static LPVOID CURTXT_COUNT_ADDR = reinterpret_cast<LPVOID>(0x6CE0F4);// 출력 중인 문자열 블록 수
static LPVOID WIN_STATUS_ADDR = reinterpret_cast<LPVOID>(0xDAF762);  // 문자창 활성화 여부 (0 = OFF, 1 = ON)

ScanProcess::ScanProcess()
    : m_stopScanThread(false),
    m_targetProcessPath(L""),
    m_processStatus(),      // ProcessMemoryStatus 구조체 멤버 초기화
    m_hProcess(NULL),
    m_blockCount(-1),
    m_bbCount(-1),
    m_cinema(false),
    m_isLoad(false)
{
    memset(m_sdtBuffer, 0, sizeof(m_sdtBuffer));
    // 대상 프로세스 정보 초기화
    m_targetProcesses.push_back({ "RoutesDVD_KR.exe", "Routes" });
    m_targetProcesses.push_back({ "RoutesDVD_JP.exe", "Routes" });
    m_targetProcesses.push_back({ "RoutesDVD.exe", "Routes" });
}

ScanProcess::~ScanProcess() {
    StopScanThread(); // 소멸 시 스레드 정리
}

ScanProcess& ScanProcess::GetInstance() {
    static ScanProcess instance;
    return instance;
}

// 보이스 블록 재생 오프셋 조정
int ScanProcess::ControlPlayOffset(SCAN_STATUS mode, int value) {
    switch (mode) {
    case ADD_OFFSET:   // Add 메소드
        return ++m_processStatus.playOffset;
    case DEC_OFFSET:   // Dec 메소드
        return --m_processStatus.playOffset;
    case RESET_OFFSET: // Reset 메소드
        return (m_processStatus.playOffset = 0);
    case GET_OFFSET:   // Get 메소드
        return m_processStatus.playOffset;
    case SET_OFFSET:   // Set 메소드
        return (m_processStatus.playOffset = value);
    case GET_BLOCK:    // GetBlock 메소드
        return m_blockCount;
    case SET_BLOCK:    // SetBlock 메소드
        return (m_blockCount = value);
    case GET_SDT:      // GetSdt 메소드
        return m_processStatus.scriptIndex;
    } 
    return 0;
}

void ScanProcess::StartScanThread() {
    if (m_scanThread.joinable()) {
        dprintf("Scan thread is already running. (ScanProcess)");
        return;
    }

    // 종료 플래그 초기화
    m_stopScanThread.store(false);

    // std::thread를 사용하여 멤버 함수를 스레드로 실행
    // this 포인터를 람다 캡처 목록으로 전달하여 멤버 함수 호출
    m_scanThread = std::thread([this]() {
        this->ProcessMemoryScanLoop();
        });
    dprintf("Start the memory scan thread. (ScanProcess)");
}

void ScanProcess::StopScanThread() {
    if (m_scanThread.joinable()) {
        m_stopScanThread.store(true); // 스레드에 종료 신호
    }
}

void ScanProcess::WaitStopScanThread() {
    if (m_scanThread.joinable()) {
        m_scanThread.join();         // 스레드 종료 대기
    }
    dprintf("Memory scan thread terminated. (ScanProcess)");
}

// 대상 프로세스 정보 구조체를 사용하여 프로세스 ID 및 캡션 확인
HANDLE ScanProcess::FindProcess(const TargetProcessInfo& targetInfo) {
    HANDLE hSnapshot = NULL;
    PROCESSENTRY32 pe32 = {};
    HWND hWnd = NULL;
    DWORD processId = 0;
    char windowTitle[256];

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        dprintf("CreateToolhelp32Snapshot failed. Error: %d", GetLastError());
        return NULL;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, targetInfo.processName.c_str()) == 0) {
                // 프로세스 ID를 찾았으므로, 해당 프로세스의 메인 창 핸들을 찾음
                processId = pe32.th32ProcessID;
                hSnapshot = NULL; // 스냅샷 핸들을 닫기 전에 NULL로 설정하여 중복 닫기 방지

                // 프로세스 ID를 기반으로 윈도우 캡션을 확인
                // 여기서는 GetWindowText를 통해 캡션을 확인합니다.
                hWnd = GetTopWindow(NULL);
                while (hWnd != NULL) {
                    DWORD currentProcessId;
                    GetWindowThreadProcessId(hWnd, &currentProcessId);
                    if (currentProcessId == processId) {
                        GetWindowTextA(hWnd, windowTitle, sizeof(windowTitle));
                        if (strstr(windowTitle, targetInfo.windowCaption.c_str()) != NULL) {
                            CloseHandle(hSnapshot); // 핸들 닫기
                            return (HANDLE)processId;
                        }
                    }
                    hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot); // 스냅샷 핸들 닫기
    return NULL;
}

// 유효한 프로세스 핸들을 검색하고 설정하는 함수
bool ScanProcess::FindValidProcessHandle(HANDLE& outProcessId, HANDLE& outHProcess) {
    // 기존 핸들이 유효한지 확인
    if (outHProcess != NULL && GetProcessId(outHProcess) != 0) {
        // 핸들이 여전히 유효하다면 재사용
        return true;
    }

    // 핸들이 유효하지 않거나 NULL인 경우 새로 검색
    outProcessId = 0;
    if (outHProcess) {
        CloseHandle(outHProcess);
        outHProcess = NULL;
    }
    m_targetProcessPath = L""; // 경로 초기화

    for (const auto& targetInfo : m_targetProcesses) {
        outProcessId = FindProcess(targetInfo);
        if (outProcessId != NULL) {
            dprintf("Found process '%s' with ID: %lu", targetInfo.processName.c_str(), (unsigned long)outProcessId);
            outHProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, (DWORD)outProcessId);
            if (outHProcess != NULL) {
                // 프로세스 핸들 획득 성공 시 경로 얻기
                wchar_t path[MAX_PATH];
                if (GetModuleFileNameExW(outHProcess, NULL, path, sizeof(path))) {
                    std::wstring fullPath = path;

                    // 마지막 백슬래시 또는 슬래시 찾기
                    size_t lastSlashPos = fullPath.find_last_of(L"\\/");
                    if (lastSlashPos != std::string::npos) {
                        // 파일명을 제외하고 디렉토리 경로만 추출
                        m_targetProcessPath = fullPath.substr(0, lastSlashPos + 1); // +1 하여 마지막 슬래시 포함
                    }
                    else {
                        // 슬래시가 없는 경우 (예: 실행 파일이 현재 디렉토리에만 있는 경우)
                        m_targetProcessPath = L".\\"; // 현재 디렉토리로 설정
                    }

                    dprintf("Target Process Directory: %s", StringUtil::GetInstance().WstringToString(m_targetProcessPath).c_str());
                }
                else {
                    dprintf("Failed to get process path for '%s'. Error code: %d", targetInfo.processName.c_str(), GetLastError());
                }
                memset(&m_processStatus, 0, sizeof(ProcessMemoryStatus));
                m_processStatus.printTextBaseCounts = 0xFFFE;
                m_processStatus.passVoicePlay = false;

                SIZE_T bytesRead;
                if (ReadProcessMemory(outHProcess, SDT_ADDR, m_processStatus.sdtBase, sizeof(m_processStatus.sdtBase), &bytesRead)) {
                    if ((bytesRead == sizeof(m_processStatus.sdtBase)) && (m_processStatus.sdtBase[3] == '\0')) {
                        dprintf("sdtBase (0x%X): %s", SDT_ADDR, m_processStatus.sdtBase);
                        dprintf("Successfully obtained process handle for '%s'.", targetInfo.processName.c_str());
                        DataManager::GetInstance().SetCurrentMainIndex(-1);
                        DataManager::GetInstance().SetCurrentSubIndex(0);
                        m_hProcess = outHProcess;
                        return true;
                    }
                    else {
                        dprintf("[Error] Failed to read full sdtBase. Read %zu bytes instead of %zu.", bytesRead, sizeof(m_processStatus.sdtBase));
                    }
                }
                else {
                    dprintf("[Error] Failed to read sdtBase (0x%X). Error: %d", SDT_ADDR, GetLastError());
                }
            }
            else {
                DWORD error = GetLastError();
                dprintf("Failed to OpenProcess for '%s'. Error code: %d", targetInfo.processName.c_str(), error);
                outProcessId = 0; // 핸들 획득 실패 시 다음 루프에서 PID 재탐색
            }
        }
    }

    if (outProcessId == 0) {
        //dprintf("No target process found. Retry after 2 seconds.");
    }
    return false;
}

// 스레드에서 실행될 실제 스캔 로직
void ScanProcess::ProcessMemoryScanLoop() {
    HANDLE hProcess = NULL;
    HANDLE curProcess = NULL;
    HANDLE processId = 0;
    ProcessMemoryStatus readStatus;
    SIZE_T bytesRead, readSize;

    while (!m_stopScanThread.load()) {
        // 프로세스 핸들 찾기 또는 유효성 검사
        if (!FindValidProcessHandle(processId, hProcess)) {
            if (curProcess != NULL) {
                AudioManager::GetInstance().StopAllAudio();
                curProcess = NULL;
            }
            // 2초마다 메모리 스캔
            for (int i = 0; i < 20; i++) {
                if (m_stopScanThread.load()) {
                    break;
                }
                Sleep(100);
            }
            continue;
        }
        curProcess = hProcess;
        
        // 데이터가 로드되었는지 확인
        if (DataManager::GetInstance().GetStatus() != DM_STATUS::LOAD) {
            Sleep(10); // 10밀리초 후 재시도
            continue;
        }
        
        // 메모리 읽기
        Sleep(1); // 1밀리초마다 메모리 스캔
        bool isChanged = false;

        readSize = 5; // sdt 파일명 검색
        ReadProcessMemory(hProcess, SCR_NO_ADDR, readStatus.currentScriptNumber, readSize, &bytesRead);
        // char[5] 값을 int로 변환. 예: "00010" -> 10
        if (readStatus.currentScriptNumber[4] != 0) {
            try {
                int read_main_idx = std::stoi(std::string(readStatus.currentScriptNumber));
                if ((read_main_idx == 0) || (read_main_idx == 90900)) {
                    m_cinema = true;
                }
                else {
                    m_cinema = false;
                }
            }
            catch (const std::exception& e) {
                m_cinema = false;
            }
        }

        readSize = sizeof(readStatus.textOutputBusyFlag);
        if (ReadProcessMemory(hProcess, WIN_STATUS_ADDR, &readStatus.textOutputBusyFlag, readSize, &bytesRead)) {
            if (bytesRead != readSize) {
                dprintf("[Error] ReadProcessMemory failed for textOutputBusyFlag (0x%X). Error: %d", WIN_STATUS_ADDR, GetLastError());
            }
            else if (!m_cinema && (readStatus.textOutputBusyFlag != 1)) {
                //dprintf("[Alert] textOutput is busy. Flag: %d", readStatus.textOutputBusyFlag);
                dprintf("Wait for playing...");
                while (readStatus.textOutputBusyFlag != 1) {
                    if (m_stopScanThread.load()) {
                        if (hProcess) {
                            CloseHandle(hProcess);
                        }
                        return;
                    }
                    Sleep(100); // 0.1초 대기

                    readSize = sizeof(readStatus.textOutputBusyFlag);
                    if (!ReadProcessMemory(hProcess, WIN_STATUS_ADDR, &readStatus.textOutputBusyFlag, readSize, &bytesRead) || (bytesRead != readSize)) {
                        break;
                    }

                    readSize = 5; // sdt 파일명 검색
                    ReadProcessMemory(hProcess, SCR_NO_ADDR, readStatus.currentScriptNumber, readSize, &bytesRead);
                    // char[5] 값을 int로 변환. 예: "00010" -> 10
                    if (readStatus.currentScriptNumber[4] != 0) {
                        try {
                            int read_main_idx = std::stoi(std::string(readStatus.currentScriptNumber));
                            if ((read_main_idx == 0) || (read_main_idx == 90900)) {
                                m_cinema = true;
                                break;
                            }
                        }
                        catch (const std::exception& e) {
                            continue;
                        }
                    }

                    readSize = sizeof(readStatus.textBufferBaseAddress);
                    if (!ReadProcessMemory(hProcess, TEXT_PTR_ADDR, &readStatus.textBufferBaseAddress, readSize, &bytesRead)) {
                        break;
                    }
                    else if (readStatus.textBufferBaseAddress != m_processStatus.textBufferBaseAddress) {
                        m_processStatus.textBufferBaseAddress = readStatus.textBufferBaseAddress;
                        readSize = sizeof(readStatus.textBufferCurrentOffset);
                        ReadProcessMemory(hProcess, TEXT_OFFSET_ADDR, &readStatus.textBufferCurrentOffset, readSize, &bytesRead);
                        if (bytesRead != readSize) {
                            break;
                        }
                        ReadProcessMemory(hProcess, (LPCVOID)readStatus.textBufferBaseAddress, m_sdtBuffer, (readStatus.textBufferCurrentOffset + 2), &bytesRead);
                        if (bytesRead < readStatus.textBufferCurrentOffset) {
                            break;
                        }
                        m_isLoad = CheckFirstLoad(m_sdtBuffer, readStatus.textBufferCurrentOffset);
                        continue;
                    }
                }
                readSize = 5; // sdt 파일명 검색
                ReadProcessMemory(hProcess, SCR_NO_ADDR, readStatus.currentScriptNumber, readSize, &bytesRead);
                // char[5] 값을 int로 변환. 예: "00010" -> 10
                if (readStatus.currentScriptNumber[4] != 0) {
                    try {
                        int read_main_idx = std::stoi(std::string(readStatus.currentScriptNumber));
                        if (DataManager::GetInstance().GetCurrentMainIndex() != read_main_idx) {
                            AudioManager::GetInstance().StopAllAudio();
                            continue;
                        }
                    }
                    catch (const std::exception& e) {
                        AudioManager::GetInstance().StopAllAudio();
                        continue;
                    }
                }
                readSize = sizeof(readStatus.textBufferCurrentOffset);
                ReadProcessMemory(hProcess, TEXT_OFFSET_ADDR, &readStatus.textBufferCurrentOffset, readSize, &bytesRead);
                if (readStatus.textBufferCurrentOffset != m_processStatus.textBufferCurrentOffset) {
                    AudioManager::GetInstance().StopAllAudio();
                }
                dprintf("Play status checked.");
                continue;
            }
            else {
                // BusyFlag가 1인 경우 실행
                m_processStatus.textOutputBusyFlag = 1;

                readSize = 5; // sdt 파일명 검색
                ReadProcessMemory(hProcess, SCR_NO_ADDR, readStatus.currentScriptNumber, readSize, &bytesRead);
                // char[5] 값을 int로 변환. 예: "00010" -> 10
                if (readStatus.currentScriptNumber[4] != 0) {
                    try {
                        int read_main_idx = std::stoi(std::string(readStatus.currentScriptNumber));

                        // DataManager 싱글톤을 통해 값 업데이트 (현재 값과 다를 경우만 출력)
                        if (DataManager::GetInstance().GetCurrentMainIndex() != read_main_idx) {
                            DataManager::GetInstance().SetCurrentMainIndex(read_main_idx);
                            DataManager::GetInstance().SetCurrentSubIndex(0);
                            m_processStatus.scriptIndex = read_main_idx;
                            m_blockCount = -1;

                            Sleep(200); // 200밀리초 대기
                            m_processStatus.playOffset = 0;
                            memset(m_processStatus.textBuffer, 0, sizeof(m_processStatus.textBuffer));
                            isChanged = true;
                            m_processStatus.printTextBaseCounts = 0xFFFE;
                            memcpy(m_processStatus.currentScriptNumber, readStatus.currentScriptNumber, readSize);
                            dprintf("Read new Main Index from Memory Address 0x%08X: \"%s\"", SCR_NO_ADDR, readStatus.currentScriptNumber);

                            readSize = sizeof(readStatus.textBufferBaseAddress);
                            ReadProcessMemory(hProcess, TEXT_PTR_ADDR, &readStatus.textBufferBaseAddress, readSize, &bytesRead);
                            if (bytesRead != readSize) {
                                dprintf("[Error] ReadProcessMemory failed for textBufferBaseAddress (0x%X). Error: %d", TEXT_PTR_ADDR, GetLastError());
                                continue;
                            }
                            readSize = sizeof(readStatus.textBufferCurrentOffset);
                            ReadProcessMemory(hProcess, TEXT_OFFSET_ADDR, &readStatus.textBufferCurrentOffset, readSize, &bytesRead);
                            if (bytesRead != readSize) {
                                dprintf("[Error] ReadProcessMemory failed for textBufferCurrentOffset (0x%X). Error: %d", TEXT_OFFSET_ADDR, GetLastError());
                                continue;
                            }

                            m_processStatus.textBufferBaseAddress = readStatus.textBufferBaseAddress;
                            m_processStatus.textBufferCurrentOffset = readStatus.textBufferCurrentOffset;

                            readSize = sizeof(readStatus.printTextBaseCounts);
                            ReadProcessMemory(hProcess, CURTXT_COUNT_ADDR, &readStatus.printTextBaseCounts, readSize, &bytesRead);
                            if (bytesRead != readSize) {
                                dprintf("[Error] ReadProcessMemory failed for printTextBaseCounts (0x%X). Error: %d", CURTXT_COUNT_ADDR, GetLastError());
                                continue;
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        dprintf("[Error] Failed to convert char[%d] value to int ('%s'): %s", readSize, readStatus.currentScriptNumber, e.what());
                        // 1초마다 메모리 스캔
                        for (int i = 0; i < 10; i++) {
                            if (m_stopScanThread.load()) {
                                if (hProcess) {
                                    CloseHandle(hProcess);
                                }
                                return;
                            }
                            Sleep(100);
                        }
                        continue;
                    }
                }
                else {
                    dprintf("[Error] ReadProcessMemory failed for currentScriptNumber (0x%X). Error: %d", SCR_NO_ADDR, GetLastError());
                    // 1초마다 메모리 스캔
                    for (int i = 0; i < 10; i++) {
                        if (m_stopScanThread.load()) {
                            if (hProcess) {
                                CloseHandle(hProcess);
                            }
                            return;
                        }
                        Sleep(100);
                    }
                    continue;
                }

                readSize = sizeof(readStatus.textBufferCurrentOffset);
                ReadProcessMemory(hProcess, TEXT_OFFSET_ADDR, &readStatus.textBufferCurrentOffset, readSize, &bytesRead);
                if (bytesRead != readSize) {
                    dprintf("[Error] ReadProcessMemory failed for textBufferCurrentOffset (0x%X). Error: %d", TEXT_OFFSET_ADDR, GetLastError());
                    continue;
                }

                readSize = sizeof(readStatus.printTextBaseCounts);
                ReadProcessMemory(hProcess, CURTXT_COUNT_ADDR, &readStatus.printTextBaseCounts, readSize, &bytesRead);
                if (bytesRead != readSize) {
                    dprintf("[Error] ReadProcessMemory failed for printTextBaseCounts (0x%X). Error: %d", CURTXT_COUNT_ADDR, GetLastError());
                    continue;
                }

                readSize = sizeof(readStatus.printTextBaseAddress);
                ReadProcessMemory(hProcess, CURTXT_PTR_ADDR, &readStatus.printTextBaseAddress, readSize, &bytesRead);
                if (bytesRead != readSize) {
                    dprintf("[Error] ReadProcessMemory failed for printTextBaseAddress (0x%X). Error: %d", CURTXT_PTR_ADDR, GetLastError());
                    continue;
                }

                m_processStatus.printTextBaseAddress = readStatus.printTextBaseAddress;

                if (m_processStatus.textBufferCurrentOffset != readStatus.textBufferCurrentOffset) {
                    if (readStatus.textBufferCurrentOffset < m_processStatus.textBufferCurrentOffset) {
                        DataManager::GetInstance().SetCurrentMainIndex(-1);
                        continue;
                    }
                    m_processStatus.textBufferCurrentOffset = readStatus.textBufferCurrentOffset;
                    m_processStatus.printTextBaseCounts = readStatus.printTextBaseCounts;
                    isChanged = true;
                }
                else if (m_processStatus.printTextBaseCounts != readStatus.printTextBaseCounts) {
                    if (readStatus.printTextBaseCounts == 0xFFFF) {
                        int i = BlockCountFromBuffer();
                        if (i != m_processStatus.printTextBaseCounts) {
                            isChanged = true;
                        }
                    }
                    else {
                        isChanged = true;
                    }
                    m_processStatus.printTextBaseCounts = readStatus.printTextBaseCounts;
                }

                if (!m_cinema) {
                    readSize = sizeof(readStatus.textBuffer);
                    ReadProcessMemory(hProcess, (LPCVOID)readStatus.printTextBaseAddress, readStatus.textBuffer, readSize, &bytesRead);
                    if (bytesRead != readSize) {
                        dprintf("[Error] ReadProcessMemory failed for textBuffer (0x%X). Error: %d", (LPVOID)readStatus.printTextBaseAddress, GetLastError());
                        continue;
                    }
                    memcpy(m_processStatus.textBuffer, readStatus.textBuffer, readSize);
                }

                // 진행 변경이 확인된 경우 음성 재생 조건인지 체크 및 재생
                if (isChanged) {
                    EvaluateProcessStatus();
                }
            }
        }
        else {
            DWORD error = GetLastError();
            dprintf("ReadProcessMemory failed. Error code: %d", error);
            if (error == ERROR_INVALID_HANDLE || error == ERROR_ACCESS_DENIED || error == ERROR_PARTIAL_COPY) {
                // 프로세스가 종료되었거나, 권한이 없어 더 이상 읽을 수 없는 경우
                dprintf("Deactivate process handle. Attempt to rediscover.");
                SendMessageA(GetWinHandle(), WM_TRAY_MENU, 0, WM_LBUTTONUP);
                if (hProcess) {
                    CloseHandle(hProcess);
                    hProcess = NULL;
                }
                processId = 0; // 다음 루프에서 프로세스 재탐색
            }
        }
    }

    if (hProcess) {
        CloseHandle(hProcess);
    }
}

// SCN 파일을 로드하고 첫 번째 문자인지 확인
bool ScanProcess::CheckFirstLoad(const char* sdtBuffer, SIZE_T readSize) const {
    ScriptOperator opcode = ScriptOperator::Start;
    int i = 0;

    memcpy(&opcode, &sdtBuffer[readSize], sizeof(opcode));
    if (opcode == ScriptOperator::SetMessage2) {
        return false;
    }

    while (i < readSize) {
        memcpy(&opcode, &sdtBuffer[i], sizeof(opcode));
        if ((opcode == ScriptOperator::SetMessage2) || (opcode == ScriptOperator::AddMessage2)) {
            return true;
        }
        else if ((opcode == ScriptOperator::OpEnd) && (opcode >= ScriptOperator::OpMax)) {
            dprintf("[Error] ReadProcessMemory failed for CalculateCurrentBlockIndex (0x%X). Error: Invalid Operator code(%d)", (LPVOID)m_processStatus.textBufferBaseAddress, opcode);
            return false;
        }
        i += 2;
        const ScriptOperatorFormat& format = g_operatorFormats[(int)opcode];
        for (int paramIndex = 0; paramIndex < 15; ++paramIndex) {
            if (format.args[paramIndex] == ScriptArgumentType::NotUsed) {
                break;
            }
            ScriptArgumentType argType = format.args[paramIndex];

            switch (argType) {
            case ScriptArgumentType::AsciiChar:
            case ScriptArgumentType::Register:
            case ScriptArgumentType::AddValue:
            case ScriptArgumentType::CharValue: {
                i++;
                break;
            }
            case ScriptArgumentType::Count:
            case ScriptArgumentType::VariableCount:
            case ScriptArgumentType::ShortValue: {
                i += 2;
                break;
            }
            case ScriptArgumentType::LongValue: {
                i += 4;
                break;
            }
            case ScriptArgumentType::Compare: {
                i += 2;
            }
            case ScriptArgumentType::Number: {
                uint8_t mode = sdtBuffer[i++];
                if (mode <= 1) {
                    i += 4;
                }
                else if (mode == 2) {
                    uint8_t length = sdtBuffer[i++];
                    i += length;
                }
                break;
            }
            case ScriptArgumentType::String:
            case ScriptArgumentType::String2: {
                uint16_t strLength = 0;
                if (argType == ScriptArgumentType::String) {
                    memcpy(&strLength, &sdtBuffer[i], 1);
                }
                else {
                    memcpy(&strLength, &sdtBuffer[i++], 2);
                }
                i += strLength + 1;
                break;
            }
            default:
                break;
            }
        }
    }
    return false;
}

// m_processStatus 정보를 기반으로 voiceFlag 값을 계산
bool ScanProcess::CalculateCurrentBlockIndex() {
    SIZE_T bytesRead, readSize;
    readSize = m_processStatus.textBufferCurrentOffset;
    ReadProcessMemory(m_hProcess, (LPCVOID)m_processStatus.textBufferBaseAddress, m_sdtBuffer, (readSize + 0x100), &bytesRead);
    if (bytesRead < readSize) {
        dprintf("[Error] ReadProcessMemory failed for CalculateCurrentBlockIndex (0x%X). Error: %d", (LPVOID)m_processStatus.textBufferBaseAddress, GetLastError());
        readSize = bytesRead;
        // return false;
    }
    int i = 0;
    int vFlag = 0;
    int blockCount = 0;
    bool isLastMessage = true;
    bool includeMessage = false;
    bool trueEnd = false;
    bool flag1 = false;
    bool flag2 = false;
    int flag3 = 0;
    m_processStatus.voiceFlag = 0;
    m_processStatus.voiceIndexBuffer = 0;
    ScriptOperator opcode = ScriptOperator::Start;
    ReadProcessMemory(m_hProcess, (LPCVOID)(m_processStatus.textBufferBaseAddress + readSize), &opcode, sizeof(opcode), &bytesRead);
    if (bytesRead != sizeof(opcode)) {
        dprintf("[Alert] ReadProcessMemory failed for Read next Operator code (0x%X). Error: %d", (LPVOID)(m_processStatus.textBufferBaseAddress + readSize), GetLastError());
    }
    else if (opcode != ScriptOperator::SetMessage2) {
        isLastMessage = false;
#ifdef _DEBUG
        if (opcode < ScriptOperator::OpMax) {
            dprintf("[Alert] Next Operator code is not SetMessage2 (0x%X). Code: %s(%d)", (LPVOID)(m_processStatus.textBufferBaseAddress + readSize), g_operatorList[(int)opcode].name.c_str(), opcode);
        }
        else {
            dprintf("[Alert] ReadProcessMemory failed for CalculateCurrentBlockIndex (0x%X). Error: Invalid Operator code(%d)", (LPVOID)m_processStatus.textBufferBaseAddress, opcode);
        }
#endif
    }
    while (i < readSize) {
        memcpy(&opcode, &m_sdtBuffer[i], sizeof(opcode));
        if ((opcode == ScriptOperator::OpEnd) && (opcode >= ScriptOperator::OpMax)) {
            dprintf("[Error] ReadProcessMemory failed for CalculateCurrentBlockIndex (0x%X). Error: Invalid Operator code(%d)", (LPVOID)m_processStatus.textBufferBaseAddress, opcode);
            return (includeMessage || isLastMessage);
        }
        // 트루 엔딩 플래그
        if (((m_processStatus.scriptIndex == 70660) && (m_processStatus.voiceIndexBuffer == 245) && (opcode == ScriptOperator::SEW))
            || ((m_processStatus.scriptIndex == 30680) && (m_processStatus.voiceIndexBuffer == 66))
            || ((m_processStatus.scriptIndex == 40910) && (m_processStatus.voiceIndexBuffer == 143))
            ) {
            trueEnd = true;

            if ((m_processStatus.scriptIndex == 30680) && (m_processStatus.voiceIndexBuffer == 66)) {
                m_processStatus.voiceIndexBuffer == ++blockCount;
            }
        }
        else if ((m_processStatus.scriptIndex == 70030) && (((m_processStatus.voiceIndexBuffer == 1) && (opcode == ScriptOperator::B)) || flag1)) {
            if (flag1) {
                ++blockCount;
            }
            flag1 = !flag1;
        }
        else if ((m_processStatus.scriptIndex == 70470) && (((m_processStatus.voiceIndexBuffer == 0) && (opcode == ScriptOperator::M)) || flag2)) {
            if (flag2) {
                m_processStatus.voiceIndexBuffer = ++blockCount;
            }
            flag2 = !flag2;
        }
        else if ((m_processStatus.scriptIndex == 70510) && (flag3 == 0)) {
            if ((m_processStatus.voiceIndexBuffer == 0) && (opcode == ScriptOperator::M)) {
                flag3 = 2;
            }
            if (m_processStatus.voiceIndexBuffer == 15) {
                flag3 = 1;
            }
            if (m_processStatus.voiceIndexBuffer == 20) {
                flag3 = 3;
            }
        }
        else if ((m_processStatus.scriptIndex == 40130) && (flag3 == 0)) {
            if (m_processStatus.voiceIndexBuffer == 25) {
                flag3 = 1;
            }
        }
        else if ((m_processStatus.scriptIndex == 40261) && (flag3 == 0)) {
            if (m_processStatus.voiceIndexBuffer == 55) {
                flag3 = 2;
            }
        }
        else if ((m_processStatus.scriptIndex == 40350) && (flag3 == 0)) {
            if (m_processStatus.voiceIndexBuffer == 0) {
                flag3 = 1;
            }
            else if (m_processStatus.voiceIndexBuffer == 5) {
                flag3 = 2;
            }
        }
        else if ((m_processStatus.scriptIndex == 50120) && (flag3 == 0)) {
            if (m_processStatus.voiceIndexBuffer == 1) {
                flag3 = 14;
            }
        }

        i += 2;
        const ScriptOperatorFormat& format = g_operatorFormats[(int)opcode];
        for (int paramIndex = 0; paramIndex < 15; ++paramIndex) {
            if (format.args[paramIndex] == ScriptArgumentType::NotUsed) {
                break;
            }
            ScriptArgumentType argType = format.args[paramIndex];

            switch (argType) {
            case ScriptArgumentType::AsciiChar:
            case ScriptArgumentType::Register:
            case ScriptArgumentType::AddValue:
            case ScriptArgumentType::CharValue: {
                i++;
                break;
            }
            case ScriptArgumentType::Count:
            case ScriptArgumentType::VariableCount:
            case ScriptArgumentType::ShortValue: {
                i += 2;
                break;
            }
            case ScriptArgumentType::LongValue: {
                i += 4;
                break;
            }
            case ScriptArgumentType::Compare: {
                i += 2;
            }
            case ScriptArgumentType::Number: {
                uint8_t mode = m_sdtBuffer[i++];
                if (mode <= 1) {
                    i += 4;
                }
                else if (mode == 2) {
                    uint8_t length = m_sdtBuffer[i++];
                    i += length;
                }
                break;
            }
            case ScriptArgumentType::String:
            case ScriptArgumentType::String2: {
                uint16_t strLength = 0;
                if (argType == ScriptArgumentType::String) {
                    memcpy(&strLength, &m_sdtBuffer[i], 1);
                }
                else {
                    memcpy(&strLength, &m_sdtBuffer[i++], 2);
                }
                i++;
                if ((opcode == ScriptOperator::SetMessage2) || (opcode == ScriptOperator::AddMessage2)) {
                    if ((m_processStatus.scriptIndex == 60030) && (blockCount == 59)) {
                        blockCount++;
                    }
                    if (opcode == ScriptOperator::SetMessage2) {
                        includeMessage = true;
                        m_processStatus.voiceFlag = vFlag;
                        m_processStatus.voiceIndexBuffer = blockCount;
                        if (0 < flag3) {
                            if (flag3-- == 1) {
                                m_processStatus.voiceIndexBuffer = ++blockCount;
                            }
                        }
                        if ((m_processStatus.scriptIndex == 70510) && ((m_processStatus.voiceIndexBuffer == 4) || (m_processStatus.voiceIndexBuffer == 6) || (m_processStatus.voiceIndexBuffer == 9) || (m_processStatus.voiceIndexBuffer == 12))) {
                            m_processStatus.voiceIndexBuffer == ++blockCount;
                        }
                    }
                    if (trueEnd) {
                        blockCount++;
                    }
                    else if ((m_processStatus.scriptIndex == 30380) && (m_processStatus.voiceIndexBuffer == 2)) {
                        blockCount++;
                    }
                    else {
                        BlockCountFromPointer(&m_sdtBuffer[i], strLength, vFlag, blockCount);
                    }

                    if ((m_processStatus.scriptIndex == 20030) && (blockCount == 111)) {
                        blockCount++;
                    }
                    if ((m_processStatus.scriptIndex == 70010) && (blockCount == 128)) {
                        vFlag = 0;
                    }
                    i += strLength;
                }
                else {
                    i += strLength;
                }
                break;
            }
            default:
                break;
            }
        }
    }
    if (isLastMessage || trueEnd) {
        m_processStatus.voiceFlag = vFlag;
        m_processStatus.voiceIndexBuffer = blockCount;
    }
    m_processStatus.passVoicePlay = false;
#ifdef _DEBUG
    dprintf("[Info] CalculateCurrentBlockIndex(0x%X, Size=0x%X). vFlag: %d, idx: %d", (LPVOID)m_processStatus.textBufferBaseAddress, readSize, m_processStatus.voiceFlag, m_processStatus.voiceIndexBuffer);
#endif
    return (includeMessage || isLastMessage);
}

// m_processStatus.textBuffer 문자열의 전체 문자열 블록 수를 계산
int ScanProcess::BlockCountFromBuffer() {
    SIZE_T bytesRead, readSize;
    readSize = sizeof(m_processStatus.textBuffer);
    ReadProcessMemory(m_hProcess, (LPCVOID)m_processStatus.printTextBaseAddress, m_processStatus.textBuffer, readSize, &bytesRead);
    if (bytesRead != readSize) {
        dprintf("[Error] ReadProcessMemory failed for BlockCountFromBuffer (0x%X). Error: %d", (LPVOID)m_processStatus.textBufferBaseAddress, GetLastError());
        return false;
    }
    int textLen;
    char byteRead;
    char* textBuff = m_processStatus.textBuffer;
    int printCount = 0;
    for (textLen = 0; textLen < 512; textLen++) {
        byteRead = textBuff[textLen];
        if (byteRead == 0) {
            break;
        }
        else if (byteRead == 0x5C) {
            byteRead = textBuff[textLen++];
            if (byteRead == 0x6B) {
                printCount++;
            }
        }
        else if (byteRead > 0x7F) {
            textLen++;
        }
    }
    return printCount;
}

// textBuff에서 textLen만큼 검색하여 voiceFlag, blockCount를 변경
void ScanProcess::BlockCountFromPointer(const char* textBuff, int textLen, int& voiceFlag, int& blockCount) {
    unsigned char startCh = 0x77;
    unsigned char endCh = 0x78;

    unsigned char byteRead;
    unsigned char byteRead2;
    unsigned char lastMatch;
    bool isFirst = true;
    int isNested = 0;
    int bCount = 0;
    int lastVoice = -1;
    int openBlock = -1;
    bool addAll = (
        ((m_processStatus.scriptIndex == 70470) && (m_processStatus.voiceIndexBuffer == 1))
        || ((m_processStatus.scriptIndex == 70510) &&
            ((m_processStatus.voiceIndexBuffer == 1)
                || (m_processStatus.voiceIndexBuffer == 5)
                || (m_processStatus.voiceIndexBuffer == 8)
                || (m_processStatus.voiceIndexBuffer == 11)
                || (m_processStatus.voiceIndexBuffer == 14)
                || (m_processStatus.voiceIndexBuffer == 16)
                || (m_processStatus.voiceIndexBuffer == 21)
                )
            )
        || ((m_processStatus.scriptIndex == 40130) && (m_processStatus.voiceIndexBuffer == 29))
        || ((m_processStatus.scriptIndex == 40261) && (m_processStatus.voiceIndexBuffer == 58))
        || ((m_processStatus.scriptIndex == 50120) && (m_processStatus.voiceIndexBuffer == 2))
    );
    bool addAll2 = ((m_processStatus.scriptIndex == 40230)
        || (m_processStatus.scriptIndex == 40300)
        || (m_processStatus.scriptIndex == 40380)
        || (m_processStatus.scriptIndex == 40410)
        || (m_processStatus.scriptIndex == 40730)
        || (m_processStatus.scriptIndex == 40740)
        || (m_processStatus.scriptIndex == 40750)
        || (m_processStatus.scriptIndex == 40760)
    );
    bool passBlock = (
        ((m_processStatus.scriptIndex == 10060) && (m_processStatus.voiceIndexBuffer == 112))
        || ((m_processStatus.scriptIndex == 20220) && (m_processStatus.voiceIndexBuffer == 10))
        || ((m_processStatus.scriptIndex == 20510) && (m_processStatus.voiceIndexBuffer == 2))
        || ((m_processStatus.scriptIndex == 20530) && (m_processStatus.voiceIndexBuffer == 92))
    );
    bool byPass = (
        ((m_processStatus.scriptIndex == 70210) && (32 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 39))
        || ((m_processStatus.scriptIndex == 70410) && (m_processStatus.voiceIndexBuffer == 8))
        || ((m_processStatus.scriptIndex == 70430) && (m_processStatus.voiceIndexBuffer == 17))
        || ((m_processStatus.scriptIndex == 50430) && (m_processStatus.voiceIndexBuffer == 2))
        || ((m_processStatus.scriptIndex == 60020) && (m_processStatus.voiceIndexBuffer == 235))
        || ((m_processStatus.scriptIndex == 60030) && ((m_processStatus.voiceIndexBuffer == 41) || (m_processStatus.voiceIndexBuffer == 118) || (m_processStatus.voiceIndexBuffer == 124) || (m_processStatus.voiceIndexBuffer == 323)))
    );

    if (((m_processStatus.scriptIndex == 20250) && (79 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 97))
        || ((m_processStatus.scriptIndex == 20380) && (395 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 437))
        || ((m_processStatus.scriptIndex == 20460) && (37 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 41))
        || ((m_processStatus.scriptIndex == 70210) && (32 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 39))
        || ((m_processStatus.scriptIndex == 70430) && (69 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 79))
        || ((m_processStatus.scriptIndex == 70450) && (100 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 105))
        || ((m_processStatus.scriptIndex == 40260) && (1 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 3))
        || ((m_processStatus.scriptIndex == 50280) && (21 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 51))
        || ((m_processStatus.scriptIndex == 60040) && (62 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 69))
       ) {
        startCh = 0x69;
        endCh = 0x6A;
    }
    if (((m_processStatus.scriptIndex == 30120) && (m_processStatus.voiceIndexBuffer == 139) && (blockCount == 139))
        || ((m_processStatus.scriptIndex == 70470) && (m_processStatus.voiceIndexBuffer == 173) && (voiceFlag == -1))
        || ((m_processStatus.scriptIndex == 70540) && (m_processStatus.voiceIndexBuffer == 10))
        || ((m_processStatus.scriptIndex == 40250) && ((m_processStatus.voiceIndexBuffer == 11) || (m_processStatus.voiceIndexBuffer == 39)))
    ) {
        voiceFlag = 0;
    }

    if ((m_processStatus.scriptIndex == 40350) && (
            ((1 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 5))
            || ((7 <= m_processStatus.voiceIndexBuffer) && (m_processStatus.voiceIndexBuffer < 11))
        )
    ) {
        blockCount++;
        m_processStatus.passVoicePlay = false;
        return;
    }

    for (int i = 0; i < textLen; i++) {
        byteRead = textBuff[i];
        if (byteRead == 0) {
            bCount++;
            break;
        }
        else if (byteRead == 0x5C) {
            byteRead2 = textBuff[++i];
            if (byteRead2 == 0x6B) {
                bCount++;
                isFirst = true;
                if (addAll) {
                    isFirst = false;
                    blockCount++;
                }
                continue;
            }
            else if (byteRead2 == 0x6E) {
                continue;
            }
        }
        else if (byteRead == 0x81) {
            byteRead2 = textBuff[++i];
            if (byteRead2 == 0x40) {
                continue;
            }
            if (isFirst && (byteRead2 == 0x63) && (m_processStatus.scriptIndex == 30020)) {
                continue;
            }
            if (isFirst && (byteRead2 == 0x5C) && (m_processStatus.scriptIndex == 10240) && (m_processStatus.voiceIndexBuffer == 34)) {
                continue;
            }
            else if ((byteRead2 == 0x75) || (byteRead2 == startCh)) {
                if ((isFirst || passBlock || byPass) && (addAll2 || (voiceFlag == 0))) {
                    if (passBlock || byPass) {
                        isFirst = true;
                    }
                    openBlock = bCount;
                    lastMatch = byteRead2;
                }
                voiceFlag++;
            }
            else if ((byteRead2 == 0x76) || (byteRead2 == endCh)) {
                voiceFlag--;
                if ((openBlock == bCount) && (voiceFlag == 0) && !addAll2) {
                    byteRead = textBuff[(i + 1)];
                    byteRead2 = textBuff[(i + 2)];
                    if ((0 < isNested) && (byteRead == '>')) {
                        isNested--;
                        i++;
                        byteRead = byteRead2;
                        byteRead2 = textBuff[(i + 2)];
                    }
                    if (((i + 1) < textLen)
                        && !((byteRead == 0)
                            || ((byteRead == 0x5C) && (byteRead2 == 0x6B))
                            || ((byteRead == 0x81) && (byteRead2 == 0x69))
                            || ((byteRead == 0x81) && (byteRead2 == lastMatch))
                            || ((byteRead == 0x2E) && (byteRead2 == 0x5C) && (textBuff[(i + 3)] == 0x6B))
                            || ((byteRead == 0x5C) && (byteRead2 == 0x6E) && ((textBuff[(i + 3)] == '<') || (textBuff[(i + 3)] == 0x5C) && (textBuff[(i + 4)] == 0x6B)))
                            || byPass
                            //|| ((m_processStatus.scriptIndex == 10150) && ((m_processStatus.voiceIndexBuffer == 60) || (m_processStatus.voiceIndexBuffer == 65)))
                            )
                        ) {
                        blockCount--;
                        openBlock = -1;
                        lastVoice = -1;
                    }
                }
            }
        }
        else if (byteRead == '<') {
            isNested++;
            continue;
        }
        else if (isFirst && (0 < isNested) && (byteRead <= 0x7F)) {
            continue;
        }
        else if ((0 < isNested) && (byteRead == '>')) {
            isNested--;
            continue;
        }
        else if (byteRead > 0x7F) {
            i++;
        }

        if (isFirst && ((0 < voiceFlag) || addAll2)) {
            if ((m_processStatus.scriptIndex == 10150) && (m_processStatus.voiceIndexBuffer == 60)) {
                blockCount += 2;
            }
            lastVoice = bCount;
            blockCount++;
        }

        isFirst = false;
    }
    m_processStatus.passVoicePlay = (addAll || addAll2) ? false : (lastVoice != bCount);
}

// m_processStatus 정보를 기반으로 voice 재생 상태를 확인/재생
bool ScanProcess::UniqueChecker() {
    //int idx = m_processStatus.voiceIndexBuffer;
    //int printCount = m_processStatus.printTextBaseCounts;
    //int offset = m_processStatus.textBufferCurrentOffset;
    //int nowScr = m_processStatus.scriptIndex;
    switch (m_processStatus.scriptIndex) {
    case 10150: {
        if ((m_processStatus.voiceIndexBuffer == 60) && (m_blockCount == 63) && !m_processStatus.passVoicePlay) {
            for (int i = 61; i <= 62; i++) {
                int calCount = DataManager::GetInstance().CalculateAdjustedSubIndex(i) + m_processStatus.playOffset;
                if (calCount != DataManager::GetInstance().GetCurrentSubIndex()) {
                    DataManager::GetInstance().SetCurrentSubIndex(calCount);
                    AudioManager::GetInstance().PlayCurrentAudio();
                    WORD curCount = m_processStatus.printTextBaseCounts;
                    SIZE_T bytesRead;
                    WORD getCount = 0;
                    dprintf("unique play: blockCount = %d, calCount = %d", m_blockCount, calCount);

                    int readSize = sizeof(m_processStatus.printTextBaseCounts);
                    for (int waitCount = 1000; waitCount > 0; waitCount--) {
                        Sleep(10);
                        ReadProcessMemory(m_hProcess, CURTXT_COUNT_ADDR, &getCount, readSize, &bytesRead);
                        if (curCount != getCount) {
                            dprintf("Stop loop: Count changed");
                            return true;
                        }
                        if (AudioManager::GetInstance().GetPlaybackState(-1) != AudioManager::PlaybackState::Playing) {
                            dprintf("Stop loop: Audio not play");
                            break;
                        }
                    }
                }
            }
            return true;
        } break;
    }
    case 10350: {
        // 리사 트루 엔딩
        if (m_processStatus.voiceIndexBuffer == 128) {
            if (m_processStatus.printTextBaseCounts == 0) {
                // 1초마다 메모리 스캔
                for (int i = 0; i < 10; i++) {
                    if (m_stopScanThread.load()) {
                        return false;
                    }
                    Sleep(100);
                }
                m_blockCount++;
                return true;
            }
        } break;
    }
    case 20030: {
        if ((m_processStatus.voiceIndexBuffer == 105) && (m_blockCount == 108) && !m_processStatus.passVoicePlay) {
            int calCount = DataManager::GetInstance().CalculateAdjustedSubIndex(107) + m_processStatus.playOffset;
            if (calCount != DataManager::GetInstance().GetCurrentSubIndex()) {
                dprintf("unique play: blockCount = %d, calCount = %d", m_blockCount, calCount);
                DataManager::GetInstance().SetCurrentSubIndex(calCount);
                AudioManager::GetInstance().PlayCurrentAudio(true);
            }
            return true;
        } break;
    }
    case 20550: {
        if ((m_processStatus.voiceIndexBuffer == 53) && (m_blockCount == 55) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(55) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        } break;
    }
    case 30350: {
        if ((m_processStatus.voiceIndexBuffer == 194) && (m_blockCount == 199) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(199) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        }
        else if ((m_processStatus.voiceIndexBuffer == 354) && (m_blockCount == 355) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(355) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        } break;
    }
    case 30380: {
        if (m_processStatus.voiceIndexBuffer == 2) {
            return true;
        }
        break;
    }
    case 30680: {
        // 유카리 트루 엔딩
        if ((m_processStatus.voiceIndexBuffer == 70) && (m_blockCount == 70)) {
            m_blockCount++;
            m_processStatus.passVoicePlay = true;
            // 1초마다 메모리 스캔
            for (int i = 0; i < 10; i++) {
                if (m_stopScanThread.load()) {
                    return false;
                }
                Sleep(100);
            }
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(67) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        }
        break;
    }
    case 40160: {
        if ((m_processStatus.voiceIndexBuffer == 113) && (m_blockCount == 116) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(113) + 1 + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            int calCount3 = calCount1 + 2;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 3, calCount1, calCount2, calCount3);
            return false;
        }
        break;
    }
    case 40230: {
        if ((m_processStatus.voiceIndexBuffer == 22) && (m_blockCount == 23) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(23) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        }
        break;
    }
    case 40910: {
        // 나나미 트루 엔딩
        if ((m_processStatus.voiceIndexBuffer == 146) && (m_processStatus.printTextBaseCounts == 0)) {
            return true;
        }
        break;
    }
    case 60030: {
        if ((m_processStatus.voiceIndexBuffer == 41) && (m_blockCount == 46) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(45) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(false, 2, calCount1, calCount2);
            return false;
        }
        else if ((m_processStatus.voiceIndexBuffer == 41) && (m_blockCount == 48) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(47) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        }
        else if ((m_processStatus.voiceIndexBuffer == 323) && (m_blockCount == 328) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(328) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            int calCount3 = calCount1 + 2;
            AudioManager::GetInstance().PlaySpecificSubIndex(false, 3, calCount1, calCount2, calCount3);
            return false;
        }
        break;
    }
    case 70030: {
        if ((m_processStatus.voiceIndexBuffer == 2) && (m_bbCount == 1)) {
            return true;
        }
        break;
    }
    case 70460: {
        if ((m_processStatus.voiceIndexBuffer == 26) && (m_blockCount == 27) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(27) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        }
        if ((m_processStatus.voiceIndexBuffer == 42) && (m_blockCount == 43) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(43) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        }
        break;
    }
    case 70610: {
        if ((m_processStatus.voiceIndexBuffer == 62) && (m_blockCount == 63) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(63) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        }
        if ((m_processStatus.voiceIndexBuffer == 135) && (m_blockCount == 136) && !m_processStatus.passVoicePlay) {
            m_processStatus.passVoicePlay = true;
            int calCount1 = DataManager::GetInstance().CalculateAdjustedSubIndex(136) + m_processStatus.playOffset;
            int calCount2 = calCount1 + 1;
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, calCount1, calCount2);
            return false;
        }
        break;
    }
    case 70660: {
        // Roots 트루 엔딩
        if (m_processStatus.voiceIndexBuffer == 247) {
            m_blockCount = 246;
            Sleep(1);
            return true;
        } break;
    }
    case 00000:
    case 90900: {
        SIZE_T bytesRead, readSize = 4;
        DWORD CurrentOffset = m_processStatus.textBufferCurrentOffset;
        if ((CurrentOffset == 6786) && (m_blockCount != 1)) { // OK
            // 6초 대기
            for (int i = 0; i < 60; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 1;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlaySpecificSubIndex(true, 2, 001, 002);
            return false;
        }
        if ((6896 <= CurrentOffset) && (CurrentOffset <= 7066) && (m_blockCount != 3)) { // OK
            m_blockCount = 3;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
        }
        if ((CurrentOffset == 7142) && (m_blockCount != 4)) { // OK
            // 1초 대기
            for (int i = 0; i < 8; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 4;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((7326 <= CurrentOffset) && (CurrentOffset <= 7463) && (m_blockCount != 5)) { // OK
            m_blockCount = 5;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 7539) && (m_blockCount != 6)) { // OK
            // 1초 대기
            for (int i = 0; i < 10; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 6;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((7660 <= CurrentOffset) && (CurrentOffset <= 7785) && (m_blockCount != 7)) { // OK
            // 1초 대기
            for (int i = 0; i < 10; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 7;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 7925) && (m_blockCount != 8)) { // OK
            // 0.5초 대기
            for (int i = 0; i < 5; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 8;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((8022 <= CurrentOffset) && (CurrentOffset <= 8110) && (m_blockCount != 9)) { // OK
            // 1초 대기
            for (int i = 0; i < 10; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 9;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((8163 <= CurrentOffset) && (CurrentOffset <= 8251) && (m_blockCount != 10)) { // OK
            // 0.5초 대기
            for (int i = 0; i < 8; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 10;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((8456 <= CurrentOffset) && (CurrentOffset <= 8655) && (m_blockCount != 11)) { // OK
            // 1.5초 대기
            for (int i = 0; i < 18; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 11;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 8732) && (m_blockCount != 12)) { // OK
            m_blockCount = 12;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 8809) && (m_blockCount != 13)) { // OK
            m_blockCount = 13;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 8886) && (m_blockCount != 14)) { // OK
            m_blockCount = 14;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((8905 <= CurrentOffset) && (CurrentOffset <= 9210) && (m_blockCount != 15)) { // OK
            m_blockCount = 15;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio(true);
            return false;
        }
        if ((CurrentOffset == 9268) && (m_blockCount != 16)) { // OK
            m_blockCount = 16;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 9326) && (m_blockCount != 17)) { // OK
            m_blockCount = 17;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 9384) && (m_blockCount != 18)) { // OK
            m_blockCount = 18;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 9442) && (m_blockCount != 19)) { // OK
            m_blockCount = 19;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 9500) && (m_blockCount != 20)) { // OK
            m_blockCount = 20;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((9577 <= CurrentOffset) && (CurrentOffset <= 9632) && (m_blockCount != 21)) { // OK
            m_blockCount = 21;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((9782 <= CurrentOffset) && (CurrentOffset <= 9870) && (m_blockCount != 22)) { // OK
            // 0.5초 대기
            for (int i = 0; i < 10; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 22;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((9889 <= CurrentOffset) && (CurrentOffset <= 9947) && (m_blockCount != 23)) { // OK
            m_blockCount = 23;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((9954 <= CurrentOffset) && (CurrentOffset <= 10024) && (m_blockCount != 24)) { // OK
            m_blockCount = 24;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 10101) && (m_blockCount != 25)) { // OK
            // 0.5초 대기
            for (int i = 0; i < 5; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 25;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 10237) && (m_blockCount != 26)) { // OK
            // 0.5초 대기
            for (int i = 0; i < 5; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 26;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio(true);
            return false;
        }
        if ((CurrentOffset == 10295) && (m_blockCount != 27)) { // OK
            m_blockCount = 27;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((10321 <= CurrentOffset) && (CurrentOffset <= 10498) && (m_blockCount != 28)) { // OK
            m_blockCount = 28;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio(true);
            return false;
        }
        if ((10765 <= CurrentOffset) && (CurrentOffset <= 10823) && (m_blockCount != 29)) { // OK
            m_blockCount = 29;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((11183 <= CurrentOffset) && (CurrentOffset <= 11318) && (m_blockCount != 30)) { // OK
            m_blockCount = 30;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((11818 <= CurrentOffset) && (CurrentOffset <= 12014) && (m_blockCount != 31)) { // OK
            m_blockCount = 31;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((12560 <= CurrentOffset) && (CurrentOffset <= 12800) && (m_blockCount != 32)) { // OK
            m_blockCount = 32;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 13682) && (m_blockCount != 33)) { // OK
            m_blockCount = 33;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 13823) && (m_blockCount != 34)) { // OK
            m_blockCount = 34;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio(true);
            return false;
        }
        if ((CurrentOffset == 13915) && (m_blockCount != 35)) { // OK
            // 0.3초 대기
            for (int i = 0; i < 3; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 35;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 14007) && (m_blockCount != 36)) { // OK
            m_blockCount = 36;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 14099) && (m_blockCount != 37)) { // OK
            m_blockCount = 37;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 14191) && (m_blockCount != 38)) { // OK
            m_blockCount = 38;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio(true);
            return false;
        }
        if ((CurrentOffset == 14283) && (m_blockCount != 39)) { // OK
            // 0.5초 대기
            for (int i = 0; i < 5; i++) {
                if (m_stopScanThread.load() || !ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &CurrentOffset, readSize, &bytesRead)) {
                    return false;
                }
                Sleep(100);
            }
            m_blockCount = 39;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 14375) && (m_blockCount != 40)) { // OK
            m_blockCount = 40;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((14481 <= CurrentOffset) && (CurrentOffset <= 14515) && (m_blockCount != 41)) { // OK
            m_blockCount = 41;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
        if ((CurrentOffset == 14629) && (m_blockCount != 42)) { // OK
            m_blockCount = 42;
            DataManager::GetInstance().SetCurrentSubIndex(m_blockCount);
            AudioManager::GetInstance().PlayCurrentAudio();
            return false;
        }
    }
    default: return false;
    }
    return false;
}

// m_processStatus 정보를 기반으로 voice 재생 상태를 확인/재생
void ScanProcess::EvaluateProcessStatus() {
    SIZE_T bytesRead, readSize;
    m_bbCount = m_blockCount;

    // 모든 정보 업데이트
    Sleep(30); // 업데이트 대기
    readSize = sizeof(m_processStatus.textBufferBaseAddress);
    ReadProcessMemory(m_hProcess, TEXT_PTR_ADDR, &m_processStatus.textBufferBaseAddress, readSize, &bytesRead);
    readSize = sizeof(m_processStatus.textBufferCurrentOffset);
    ReadProcessMemory(m_hProcess, TEXT_OFFSET_ADDR, &m_processStatus.textBufferCurrentOffset, readSize, &bytesRead);
    readSize = 5;
    ReadProcessMemory(m_hProcess, SCR_NO_ADDR, m_processStatus.currentScriptNumber, readSize, &bytesRead);
    // char[5] 값을 int로 변환. 예: "00010" -> 10
    if (m_processStatus.currentScriptNumber[4] != 0) {
        try {
            int read_main_idx = std::stoi(std::string(m_processStatus.currentScriptNumber));

            // DataManager 싱글톤을 통해 값 업데이트 (현재 값과 다를 경우만 출력)
            if (DataManager::GetInstance().GetCurrentMainIndex() != read_main_idx) {
                Sleep(50);
                return;
            }
        }
        catch (const std::exception& e) {
            dprintf("[Error] Failed to convert char[%d] value to int ('%s'): %s", readSize, m_processStatus.currentScriptNumber, e.what());
        }
    }
    if (m_stopScanThread.load()) { return; }

    if (!m_cinema) {
        // 현재 스크립트의 voiceFlag를 계산하여 초기화
        if (!CalculateCurrentBlockIndex()) {
            Sleep(80); // 업데이트 대기
            if (m_stopScanThread.load()) { return; }
            dprintf("[Alert] CalculateCurrentBlockIndex returns No Information. try again...");
            m_processStatus.printTextBaseCounts = 0xFFFE;
            return;
        }
        if (m_isLoad) {
            m_isLoad = CheckFirstLoad(m_sdtBuffer, m_processStatus.textBufferCurrentOffset);
            if (m_isLoad) {
                return;
            }
        }

        Sleep(80); // 업데이트 대기
        if (m_stopScanThread.load()) { return; }
        readSize = sizeof(m_processStatus.printTextBaseCounts);
        ReadProcessMemory(m_hProcess, CURTXT_COUNT_ADDR, &m_processStatus.printTextBaseCounts, readSize, &bytesRead);
        readSize = sizeof(m_processStatus.printTextBaseAddress);
        ReadProcessMemory(m_hProcess, CURTXT_PTR_ADDR, &m_processStatus.printTextBaseAddress, readSize, &bytesRead);
        readSize = sizeof(m_processStatus.textBuffer);
        ReadProcessMemory(m_hProcess, (LPCVOID)m_processStatus.printTextBaseAddress, m_processStatus.textBuffer, readSize, &bytesRead);
        if (bytesRead != readSize) {
            dprintf("[Error] ReadProcessMemory failed for EvaluateProcessStatus (0x%X). Error: %d", (LPVOID)m_processStatus.printTextBaseAddress, GetLastError());
            return;
        }
    }

    int textLen = 0;
    char* textBuff = m_processStatus.textBuffer;
    int printCount = m_processStatus.printTextBaseCounts;
    int oldCount = (short)printCount;
    if (!m_cinema) {
        unsigned char byteRead;
        for (textLen = 0; textLen < 512; textLen++) {
            byteRead = textBuff[textLen];
            if (byteRead == 0) {
                break;
            }
            else if (byteRead == 0x5C) {
                byteRead = textBuff[++textLen];
                if (byteRead == 0x6B) {
                    if (printCount-- == 0) {
                        textLen--;
                        break;
                    }
                }
            }
            else if (byteRead > 0x7F) {
                textLen++;
            }
        }
    }
    int vFlag = m_processStatus.voiceFlag;
    if (!m_cinema) {
        m_blockCount = m_processStatus.voiceIndexBuffer;
        BlockCountFromPointer(textBuff, textLen, vFlag, m_blockCount);
    }
    bool uniqueCheck = UniqueChecker();

#ifdef _DEBUG
    if (!m_cinema) {
        dprintf("[Info] EvaluateProcessStatus (sdt=%s, idx=%d, blockNo=%d, printCount=%d)", m_processStatus.currentScriptNumber, m_processStatus.voiceIndexBuffer, m_blockCount, oldCount);
    }
    else {
        dprintf("[Info] CinemaCurrentOffset (sdt=%s, CurrentOffset=%d)", m_processStatus.currentScriptNumber, m_processStatus.textBufferCurrentOffset);
    }
#endif

    if (((!m_processStatus.passVoicePlay) && (m_bbCount < m_blockCount)) || uniqueCheck) {
        int calCount = DataManager::GetInstance().CalculateAdjustedSubIndex(m_blockCount) + m_processStatus.playOffset;
#ifdef _DEBUG
        dprintf("textLen = %d, vFlag = %d, blockCount = %d, calCount = %d", textLen, vFlag, m_blockCount, calCount);
#endif
        if (calCount != DataManager::GetInstance().GetCurrentSubIndex()) {
            if (0 < calCount) {
                DataManager::GetInstance().SetCurrentSubIndex(calCount);
                AudioManager::GetInstance().PlayCurrentAudio();
            }
            else {
                dprintf("Bypass audio playback: sdt=%s, idx=%d, blockNo=%d, printCount=%d", m_processStatus.currentScriptNumber, m_processStatus.voiceIndexBuffer, m_blockCount, oldCount);
            }
        }
    }
    //dprintf("EvaluateProcessStatus");
}