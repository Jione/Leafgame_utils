#include <windows.h> // WinAPI 기본 헤더
#include <winnls32.h>
#include <commctrl.h>
#include <iostream>  // 콘솔 디버그 출력용 (실제 GUI 앱에서는 OutputDebugString 사용 권장)
#include <string>
#include <vector>
#include <sstream>   // std::stringstream
#include <atlstr.h>  // CString, OutputDebugStringA/W를 위해

// 기존에 작성했던 매니저 및 파서 헤더
#include "resource.h"
#include "DataManager.h"
#include "AudioManager.h"
#include "ScanProcess.h"
#include "Dialogs.h"
#include "StringUtil.h"

// 전역 변수 (간단한 예제를 위해)
HINSTANCE hInstGlobal;
HWND hMainWindow;
const char g_szClassName[] = "RoutesVoiceWindowClass"; // 윈도우 클래스 이름
static constexpr LONG gWindowMinWidth = 480;
static constexpr LONG gWindowMinHight = 220;

// 볼륨 콜백용 구조체
typedef struct {
    int selectLevel;
    float dwVolume;
}GUI_VOLUME;
static GUI_VOLUME guiVolume{ 10, 1.0f };

// 트레이 아이콘용 전역 변수
static NOTIFYICONDATAW nid{};
static UINT uShellRestart;

// 메시지 출력 핸들
HWND hEdit;
HWND hDebug;

// 윈도우 프로시저 (메시지 처리 함수)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 메시지 서버 함수


// WinMain 함수 (애플리케이션 진입점)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "RoutesVoicePlayerMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND h_wnd = FindWindowA(g_szClassName, NULL);
        if (h_wnd != NULL) {
            ShowWindow(h_wnd, SW_SHOWNORMAL);
            SetForegroundWindow(h_wnd);
        }
        if (hMutex) {
            CloseHandle(hMutex); // 기존 뮤텍스 핸들을 닫아줌
        }
        return 1; // 프로그램 종료
    }

    hInstGlobal = hInstance; // 인스턴스 핸들 저장

    // 윈도우 클래스 등록
    MSG cbMessage;
    WNDCLASSEX wndClass{};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = 0;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON));
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = g_szClassName;
    wndClass.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 16, 16, 0);

    if (!RegisterClassEx(&wndClass)) {
        MessageBoxW(NULL, L"윈도우 클래스 등록 실패!", L"오류", MB_ICONERROR | MB_OK);
        return 0;
    }

    // 윈도우 생성
    hMainWindow = CreateWindowEx(
        WS_EX_CLIENTEDGE,       // 확장 스타일
        g_szClassName,          // 윈도우 클래스 이름
        "Routes Voice Player",  // 윈도우 제목
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME, // 윈도우 스타일 (기본 창)
        CW_USEDEFAULT, CW_USEDEFAULT, // 위치
        gWindowMinWidth, gWindowMinHight,   // 크기 (너비, 높이)
        NULL,                   // 부모 윈도우
        NULL,                   // 메뉴
        hInstance,              // 인스턴스 핸들
        NULL                    // 추가 매개변수
    );

    if (hMainWindow == NULL) {
        MessageBoxW(NULL, L"윈도우 생성 실패!", L"오류", MB_ICONERROR | MB_OK);
        return 0;
    }

    // 윈도우 보이기
    SendMessageA(hMainWindow, WM_TRAY_SETUP, NULL, NULL);
    ShowWindow(hMainWindow, SW_SHOWNOACTIVATE);
    UpdateWindow(hMainWindow);
    SetFocus(hMainWindow);
    WINNLSEnableIME(hMainWindow, FALSE);

    // 타이머 설정
    SetTimer(hMainWindow, IDT_VOICEPLAYER_TIMER, 10, NULL);

    // 스레드 시작
    ScanProcess::GetInstance().StartScanThread();
    DataManager::GetInstance().StartDataManagerThread();
    if (!AudioManager::GetInstance().Initialize()) {
        MessageBoxW(NULL, L"오디오 시스템 초기화 실패. 프로그램을 종료합니다.", L"오류", MB_ICONERROR | MB_OK);
        return 1;
    }
    PrintMessage::GetInstance().StartPrintMessageThread();

    // 메시지 루프
    while (GetMessage(&cbMessage, NULL, 0, 0) > 0) {
        TranslateMessage(&cbMessage);
        DispatchMessage(&cbMessage);
    }

    // 스레드 종료
    ScanProcess::GetInstance().StopScanThread();
    DataManager::GetInstance().StopDataManagerThread();
    PrintMessage::GetInstance().StopPrintMessageThread();
    AudioManager::GetInstance().Shutdown();

    // 스레드 종료 대기
    PrintMessage::GetInstance().WaitStopPrintMessageThread();
    DataManager::GetInstance().WaitStopDataManagerThread();
    ScanProcess::GetInstance().WaitStopScanThread();

    CloseHandle(hMutex); // 기존 뮤텍스 핸들을 닫아줌
    return cbMessage.wParam;
}

// 트레이 아이콘으로 추가
static void OnBnClickedTrayAdd(HWND hwnd, HICON hIcon) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.uID = IDI_MYICON;
    nid.dwInfoFlags = NIIF_NONE;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.hWnd = hwnd;
    nid.hIcon = hIcon;
    nid.uCallbackMessage = WM_TRAY_MENU;
    nid.uTimeout = 3000;
    lstrcpyW(nid.szInfoTitle, L"Routes 보이스 플레이어");
    lstrcpyW(nid.szInfo, L"Routes 보이스 플레이어가 트레이에서 동작합니다...");
    lstrcpyW(nid.szTip, L"Routes Voice Player");

    //Shell_NotifyIconW(NIM_ADD, &nid);
}

// 윈도우 프로시저 구현
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // 디스플레이 메시지 영역
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        hEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_VISIBLE | WS_CHILD | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            0, 0, 100, 100,
            hwnd,
            (HMENU)IDC_MAIN_EDIT,
            GetModuleHandle(NULL),
            NULL
        );
        hDebug = CreateWindowExA(0, "msctls_statusbar32", 0, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, rcClient.bottom, rcClient.right, 20, hwnd, NULL, hInstGlobal, NULL);
        // 맑은고딕 폰트 생성 및 적용
        static HFONT hFont = NULL;
        if (!hFont) {
            LOGFONTW lf = { 0 };
            lf.lfHeight = -14; // 16px 크기
            wcscpy_s(lf.lfFaceName, L"Consolas");
            lf.lfCharSet = HANGUL_CHARSET;
            hFont = CreateFontIndirectW(&lf);
        }
        if (hFont) {
            SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            //SendMessageW(hDebug, WM_SETFONT, (WPARAM)hFont, TRUE);
        }
        HMENU hMenu, hSubMenu, hPopupMenu;

        hMenu = CreateMenu();

        hSubMenu = CreatePopupMenu();
        AppendMenuW(hSubMenu, MF_STRING, IDR_FILE_SELECT_PAK, L"PAK 파일 변경(&P)");
        AppendMenuW(hSubMenu, MF_STRING, IDR_FILE_SELECT_INF, L"INF 파일 변경(&I)");
        AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hSubMenu, MF_STRING, IDR_FILE_SAVE_INF, L"기본 INF 파일 생성(&S)");
        AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hSubMenu, MF_STRING, IDR_FILE_EXIT, L"종료(&X)");
        AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, L"파일(&F)");

        hSubMenu = CreatePopupMenu();
        hPopupMenu = CreatePopupMenu();
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL0, L"&0%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL1, L"&10%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL2, L"&20%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL3, L"&30%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL4, L"&40%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL5, L"&50%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL6, L"&60%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL7, L"&70%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL8, L"&80%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL9, L"&90%");
        AppendMenuW(hPopupMenu, MF_STRING, IDR_MENU_VOL10, L"&100%");
        AppendMenuW(hSubMenu, MF_STRING | MF_POPUP, (UINT64)hPopupMenu, L"볼륨 설정(&S)");
        AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hSubMenu, MF_STRING, IDR_MENU_SETOFFSET, L"오프셋 설정(&O)");
        AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, L"설정(&S)");

        hSubMenu = CreatePopupMenu();
        AppendMenuW(hSubMenu, MF_STRING, IDR_VIEW_LIST, L"현재 스크립트 보이스(&L)");
        AppendMenuW(hSubMenu, MF_STRING, IDR_VIEW_ALL, L"모든 음성 파일(&A)");
        AppendMenuW(hSubMenu, MF_STRING, IDR_VIEW_WDIR, L"파일 경로(&R)");
        AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, L"보기(&V)");

        hSubMenu = CreatePopupMenu();
        AppendMenuW(hSubMenu, MF_STRING, IDR_HELP_INST, L"사용법(&I)");
        AppendMenuW(hSubMenu, MF_STRING, IDR_HELP_ABOUT, L"프로그램 정보(&A)");
        AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, L"도움말(&H)");

        SetMenu(hwnd, hMenu);

        uShellRestart = RegisterWindowMessage("TaskbarCreated");
        break;
    }
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_LEFT:
        case VK_DOWN: {
            // 왼쪽,아래쪽 방향키 처리
            int offset = ScanProcess::GetInstance().ControlPlayOffset(DEC_OFFSET);
            //int newidx = DataManager::GetInstance().CalculateAdjustedSubIndex(ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK) + offset);
            int newidx = DataManager::GetInstance().GetCurrentSubIndex() - 1;
            if (0 < newidx) {
                DataManager::GetInstance().SetCurrentSubIndex(newidx);
                if (wParam == VK_LEFT) {
                    AudioManager::GetInstance().PlayCurrentAudio();
                }
            }
            break;
        }
        case VK_RIGHT:
        case VK_UP: {
            // 오른쪽,위쪽 방향키 처리
            int offset = ScanProcess::GetInstance().ControlPlayOffset(ADD_OFFSET);
            // int newidx = DataManager::GetInstance().CalculateAdjustedSubIndex(ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK) + offset);
            int newidx = DataManager::GetInstance().GetCurrentSubIndex() + 1;
            DataManager::GetInstance().SetCurrentSubIndex(newidx);
            if (wParam == VK_RIGHT) {
                AudioManager::GetInstance().PlayCurrentAudio();
            }
            break;
        }
        case VK_RETURN:
        case VK_ESCAPE: {
            // 엔터키, ESC 처리
            ScanProcess::GetInstance().ControlPlayOffset(RESET_OFFSET);
            int newidx = DataManager::GetInstance().CalculateAdjustedSubIndex(ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK));
            DataManager::GetInstance().SetCurrentSubIndex(newidx);
            if (wParam == VK_RETURN) {
                AudioManager::GetInstance().PlayCurrentAudio();
            }
            break;
        }
        case VK_SPACE: {
            // 스페이스 처리
            AudioManager::GetInstance().PlayCurrentAudio();
            break;
        }
        }
        break;
    case WM_GETMINMAXINFO: {
        MINMAXINFO* pMinMax = (MINMAXINFO*)lParam;
        pMinMax->ptMinTrackSize.x = gWindowMinWidth;
        pMinMax->ptMinTrackSize.y = gWindowMinHight;
        break;
    }
    case WM_SIZE:
        SetFocus(hwnd);
        switch (LOWORD(wParam)) {
        case SIZE_MINIMIZED:
            if (ScanProcess::GetInstance().GetTargetProcessHandle() != NULL) {
                Shell_NotifyIconW(NIM_ADD, &nid);
                ShowWindow(hwnd, SW_HIDE);
            }
            break;
        default:
            hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            //SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
            // status bar 크기 조정
            if (hDebug) {
                SendMessageA(hDebug, WM_SIZE, 0, 0);
            }
            // 에디트 컨트롤 크기 조정 (status bar 높이만큼 빼기)
            int statusBarHeight = 0;
            if (hDebug) {
                RECT rcStatus;
                GetWindowRect(hDebug, &rcStatus);
                statusBarHeight = rcStatus.bottom - rcStatus.top;
            }
            SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom - statusBarHeight, SWP_NOZORDER);
            break;
        }
        break;

    case WM_INITMENUPOPUP:
        SetFocus(hwnd);
        CheckMenuRadioItem((HMENU)wParam, IDR_MENU_VOL0, IDR_MENU_VOL10, (guiVolume.selectLevel + IDR_MENU_VOL0), MF_BYCOMMAND);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDR_FILE_SELECT_PAK: {
            std::wstring filePathW = DialogUtil::GetInstance().ShowOpenFileDialogW(hwnd, L"PAK 파일 (*.pak,*.bin,*.sfs)\0*.pak;*.bin;*.sfs\0모든 파일 (*.*)\0*.*\0", L"PAK 파일 선택");
            if (filePathW != L"") {
                DataManager::GetInstance().SetPackPath(filePathW);
                MessageBoxW(hwnd, (filePathW + L"\n\npak 파일 경로가 변경되었습니다.").c_str(), L"PAK 파일 변경", MB_OK);
            }
            break;
        }
        case IDR_FILE_SELECT_INF: {
            std::wstring filePathW = DialogUtil::GetInstance().ShowOpenFileDialogW(hwnd, L"재생 정보 (*.inf)\0*.inf\0모든 파일 (*.*)\0*.*\0", L"INF 파일 선택");
            if (filePathW != L"") {
                DataManager::GetInstance().SetInfPath(filePathW);
                MessageBoxW(hwnd, (filePathW + L"\n\ninf 파일 경로가 변경되었습니다.").c_str(), L"INF 파일 변경", MB_OK);
            }
            break;
        }
        case IDR_FILE_SAVE_INF:
            if (DataManager::GetInstance().SaveDefaultInf()) {
                MessageBoxW(hwnd
                    , (DataManager::GetInstance().GetExecutablePath() + L"voice.inf\n\n실행 중인 폴더에 voice.inf 파일을 저장하였습니다.").c_str()
                    , L"INF 파일 저장 성공", MB_OK | MB_ICONASTERISK);
            }
            else {
                MessageBoxW(hwnd, L"실행 중인 폴더에 voice.inf 파일이 존재합니다.", L"INF 파일 저장 실패", MB_OK | MB_ICONERROR);
            }
            break;
        case IDR_FILE_EXIT:
            PostMessageA(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDR_MENU_VOL0:
        case IDR_MENU_VOL1:
        case IDR_MENU_VOL2:
        case IDR_MENU_VOL3:
        case IDR_MENU_VOL4:
        case IDR_MENU_VOL5:
        case IDR_MENU_VOL6:
        case IDR_MENU_VOL7:
        case IDR_MENU_VOL8:
        case IDR_MENU_VOL9:
        case IDR_MENU_VOL10:
            guiVolume.selectLevel = LOWORD(wParam) - IDR_MENU_VOL0;
            guiVolume.dwVolume = (float)guiVolume.selectLevel / 10.0;
            AudioManager::GetInstance().SetVolume(guiVolume.dwVolume);
            break;
        case IDR_MENU_SETOFFSET: {
            int enteredNumber = 0;
            std::wstring promptText = L"변경할 오프셋 값 입력";
            if (DialogUtil::GetInstance().ShowNumberInputDialog(hwnd, enteredNumber, promptText)) {
                int newidx = DataManager::GetInstance().CalculateAdjustedSubIndex(ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK) + enteredNumber);
                ScanProcess::GetInstance().ControlPlayOffset(SET_OFFSET, enteredNumber);
                DataManager::GetInstance().SetCurrentSubIndex(newidx);
                AudioManager::GetInstance().PlayCurrentAudio();
            }
            break;
        }
        case IDR_VIEW_LIST: {
            int i = 0;
            int selectValue = 0;
            std::map<int, int> currentAudio = DataManager::GetInstance().GetCurrentAudioIndex();
            std::map<int, std::string> audioList;
            for (auto it_sub = currentAudio.begin(); it_sub != currentAudio.end(); ++it_sub) {
                int indexNum = it_sub->second;
                if (0 < indexNum) {
                    auto pakFileIndex = DataManager::GetInstance().GetPakFileInfo(indexNum);
                    auto& subMap = audioList[indexNum]; // indexNum 없으면 자동 생성
                    subMap = pakFileIndex->fileName;
                }
            }
            DialogUtil::GetInstance().ShowListInputDialog(hwnd, selectValue, L"현재 스크립트 보이스 목록", audioList);
            audioList.erase(audioList.begin(), audioList.end());
            break;
        }
        case IDR_VIEW_ALL: {
            int i = 0;
            int selectValue = 0;
            std::map<int, std::string> audioList;
            for (auto& it : DataManager::GetInstance().GetPakFiles()) {
                auto& subMap = audioList[i++]; // indexNum 없으면 자동 생성
                subMap = it.fileName;
            }
            DialogUtil::GetInstance().ShowListInputDialog(hwnd, selectValue, L"전체 음성 목록", audioList);
            audioList.erase(audioList.begin(), audioList.end());
            break;
        }
        case IDR_VIEW_WDIR: {
            wchar_t buff[MAX_PATH];
            GetCurrentDirectoryW(MAX_PATH, buff);
            std::wstringstream ss;
            std::wstring packPath = DataManager::GetInstance().GetPackPath();
            if (packPath.size() == 0) {
                packPath = L"로드되지 않음";
            }
            std::wstring infPath = DataManager::GetInstance().GetInfPath();
            if (infPath.size() == 0) {
                infPath = L"내장 INF 파일 적용 중...";
            }
            ss << L"VoicePlayer 실행 경로:\n" << DataManager::GetInstance().GetExecutablePath() << "\n";
            ss << L"\nRoutesDVD 실행 경로:\n" << ScanProcess::GetInstance().GetTargetProcessPath() << "\n";
            ss << L"\nVoice Pack 파일:\n" << packPath << "\n";
            ss << L"\nVoice INF 파일:\n" << infPath << "\n";
            MessageBoxW(hwnd, ss.str().c_str(), L"파일 검색 경로", MB_OK);
            break;
        }
        case IDR_HELP_INST:
            MessageBoxW(
                hwnd,
                TEXT(
                    L"보이스 파일 준비 (택1)\n"
                    L"   - 미리 패키징된 voice.pak 파일\n"
                    L"   - 또는 PS2 이미지 내의 RTSOUND.SFS 파일\n"
                    L"   - 또는 PSP 이미지 내의 V0_FILE.BIN, V1_FILE.BIN 파일\n"
                    L"\n"
                    L"\n"
                    L"1. 보이스 파일과 inf 설정 파일을 게임 폴더 또는 \n"
                    L"   RoutesVoice.exe와 동일한 폴더로 복사합니다.\n"
                    L"\n"
                    L"2. RoutesVoice.exe와 RoutesDVD.exe를 실행합니다.\n"
                    L"   보이스 재생은 실행 순서와 관계 없이 동작합니다.\n"
                    L"   (게임 파일: RoutesDVD, RoutesDVD_JP, RoutesDVD_KR)\n"
                    L"\n"
                    L"3. 게임 진행 중 음성 싱크가 다를 경우 inf 파일을 생성 후 편집합니다.\n"
                    L"   INF는 RoutesVoice가 실행 중인 폴더에 생성됩니다.\n"
                    L"   단축키를 눌러 오프셋 변경하며 싱크를 맞춰본 다음,\n"
                    L"   화면에 출력된 문자를 inf에 입력하고 저장하면 적용됩니다.\n"
                    L"   (예상되는 오프셋이며 정확하지 않을 수 있습니다)\n"
                    L"\n"
                    L"\n"
                    L"**오프셋 변경 단축키**\n"
                    L"\n"
                    L"좌우 방향키: 오프셋 증가/감소 (오디오 재생)\n"
                    L"상하 방향키: 오프셋 증가/감소 (재생 안함)\n"
                    L"ESC, 엔터키: 오프셋 초기화 (ESC=재생 안함, 엔터=재생)\n"
                    L"스페이스 키: 현재 오디오 재생\n"
                ),
                TEXT(L"사용법"),
                MB_OK
            );
            break;
        case IDR_HELP_ABOUT:
#ifdef _WIN64
            MessageBoxW(
                hwnd,
                TEXT(
                    L"Routes Voice Player v1.0 (64비트)\n"
                    L"\n"
                    L"지원OS: Windows 7 이상\n"
                ),
                TEXT(L"프로그램 정보"),
                MB_OK | MB_ICONASTERISK
            );
#else
            MessageBoxW(
                hwnd,
                TEXT(
                    L"Routes Voice Player v1.0 (32비트)\n"
                    L"\n"
                    L"지원OS: Windows XP 이상\n"
                ),
                TEXT(L"프로그램 정보"),
                MB_OK | MB_ICONASTERISK
            );
#endif
            break;
        case IDR_TRAY_SHOW:
            Shell_NotifyIconW(NIM_DELETE, &nid);
            ShowWindow(hwnd, SW_SHOWNORMAL);
            SetForegroundWindow(hwnd);
            break;
        case IDR_TRAY_EXIT:
            Shell_NotifyIconW(NIM_DELETE, &nid);
            PostQuitMessage(0);
            break;
        }
        break;
    case WM_CLOSE:
        Shell_NotifyIconW(NIM_DELETE, &nid);
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    case WM_TRAY_SETUP:
        OnBnClickedTrayAdd(hwnd, LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON)));
        break;
    case WM_TRAY_MENU:
        switch ((UINT)lParam) {
        case WM_LBUTTONUP:
            Shell_NotifyIconW(NIM_DELETE, &nid);
            ShowWindow(hwnd, SW_SHOWNORMAL);
            SetForegroundWindow(hwnd);
            break;
        case WM_RBUTTONUP:
            HMENU hSubMenu;
            POINT trackPt;
            hSubMenu = CreatePopupMenu();
            AppendMenuW(hSubMenu, MF_STRING, IDR_TRAY_SHOW, L"열기(&O)");
            AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hSubMenu, MF_STRING, IDR_TRAY_EXIT, L"종료(&E)");

            GetCursorPos(&trackPt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hSubMenu, TPM_LEFTALIGN, trackPt.x, trackPt.y, 0, hwnd, NULL);
            break;
        }
        break;
    case WM_DEBUG_MESSAGE:
        SendMessageA(hDebug, SB_SETTEXTA, 0, (LPARAM)GetDebugMessage());
        break;
    case WM_PRINT_MESSAGE:
        SetWindowTextW(hEdit, GetPrintMessage());
        break;
    default:
        if (msg == uShellRestart) {
            Shell_NotifyIconW(NIM_DELETE, &nid);
            OnBnClickedTrayAdd(hwnd, nid.hIcon);
        }
        else {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}