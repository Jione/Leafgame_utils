#include <windows.h> // WinAPI �⺻ ���
#include <winnls32.h>
#include <commctrl.h>
#include <iostream>  // �ܼ� ����� ��¿� (���� GUI �ۿ����� OutputDebugString ��� ����)
#include <string>
#include <vector>
#include <sstream>   // std::stringstream
#include <atlstr.h>  // CString, OutputDebugStringA/W�� ����

// ������ �ۼ��ߴ� �Ŵ��� �� �ļ� ���
#include "resource.h"
#include "DataManager.h"
#include "AudioManager.h"
#include "ScanProcess.h"
#include "Dialogs.h"
#include "StringUtil.h"

// ���� ���� (������ ������ ����)
HINSTANCE hInstGlobal;
HWND hMainWindow;
const char g_szClassName[] = "RoutesVoiceWindowClass"; // ������ Ŭ���� �̸�
static constexpr LONG gWindowMinWidth = 480;
static constexpr LONG gWindowMinHight = 220;

// ���� �ݹ�� ����ü
typedef struct {
    int selectLevel;
    float dwVolume;
}GUI_VOLUME;
static GUI_VOLUME guiVolume{ 10, 1.0f };

// Ʈ���� �����ܿ� ���� ����
static NOTIFYICONDATAW nid{};
static UINT uShellRestart;

// �޽��� ��� �ڵ�
HWND hEdit;
HWND hDebug;

// ������ ���ν��� (�޽��� ó�� �Լ�)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// �޽��� ���� �Լ�


// WinMain �Լ� (���ø����̼� ������)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "RoutesVoicePlayerMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND h_wnd = FindWindowA(g_szClassName, NULL);
        if (h_wnd != NULL) {
            ShowWindow(h_wnd, SW_SHOWNORMAL);
            SetForegroundWindow(h_wnd);
        }
        if (hMutex) {
            CloseHandle(hMutex); // ���� ���ؽ� �ڵ��� �ݾ���
        }
        return 1; // ���α׷� ����
    }

    hInstGlobal = hInstance; // �ν��Ͻ� �ڵ� ����

    // ������ Ŭ���� ���
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
        MessageBoxW(NULL, L"������ Ŭ���� ��� ����!", L"����", MB_ICONERROR | MB_OK);
        return 0;
    }

    // ������ ����
    hMainWindow = CreateWindowEx(
        WS_EX_CLIENTEDGE,       // Ȯ�� ��Ÿ��
        g_szClassName,          // ������ Ŭ���� �̸�
        "Routes Voice Player",  // ������ ����
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME, // ������ ��Ÿ�� (�⺻ â)
        CW_USEDEFAULT, CW_USEDEFAULT, // ��ġ
        gWindowMinWidth, gWindowMinHight,   // ũ�� (�ʺ�, ����)
        NULL,                   // �θ� ������
        NULL,                   // �޴�
        hInstance,              // �ν��Ͻ� �ڵ�
        NULL                    // �߰� �Ű�����
    );

    if (hMainWindow == NULL) {
        MessageBoxW(NULL, L"������ ���� ����!", L"����", MB_ICONERROR | MB_OK);
        return 0;
    }

    // ������ ���̱�
    SendMessageA(hMainWindow, WM_TRAY_SETUP, NULL, NULL);
    ShowWindow(hMainWindow, SW_SHOWNOACTIVATE);
    UpdateWindow(hMainWindow);
    SetFocus(hMainWindow);
    WINNLSEnableIME(hMainWindow, FALSE);

    // Ÿ�̸� ����
    SetTimer(hMainWindow, IDT_VOICEPLAYER_TIMER, 10, NULL);

    // ������ ����
    ScanProcess::GetInstance().StartScanThread();
    DataManager::GetInstance().StartDataManagerThread();
    if (!AudioManager::GetInstance().Initialize()) {
        MessageBoxW(NULL, L"����� �ý��� �ʱ�ȭ ����. ���α׷��� �����մϴ�.", L"����", MB_ICONERROR | MB_OK);
        return 1;
    }
    PrintMessage::GetInstance().StartPrintMessageThread();

    // �޽��� ����
    while (GetMessage(&cbMessage, NULL, 0, 0) > 0) {
        TranslateMessage(&cbMessage);
        DispatchMessage(&cbMessage);
    }

    // ������ ����
    ScanProcess::GetInstance().StopScanThread();
    DataManager::GetInstance().StopDataManagerThread();
    PrintMessage::GetInstance().StopPrintMessageThread();
    AudioManager::GetInstance().Shutdown();

    // ������ ���� ���
    PrintMessage::GetInstance().WaitStopPrintMessageThread();
    DataManager::GetInstance().WaitStopDataManagerThread();
    ScanProcess::GetInstance().WaitStopScanThread();

    CloseHandle(hMutex); // ���� ���ؽ� �ڵ��� �ݾ���
    return cbMessage.wParam;
}

// Ʈ���� ���������� �߰�
static void OnBnClickedTrayAdd(HWND hwnd, HICON hIcon) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.uID = IDI_MYICON;
    nid.dwInfoFlags = NIIF_NONE;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.hWnd = hwnd;
    nid.hIcon = hIcon;
    nid.uCallbackMessage = WM_TRAY_MENU;
    nid.uTimeout = 3000;
    lstrcpyW(nid.szInfoTitle, L"Routes ���̽� �÷��̾�");
    lstrcpyW(nid.szInfo, L"Routes ���̽� �÷��̾ Ʈ���̿��� �����մϴ�...");
    lstrcpyW(nid.szTip, L"Routes Voice Player");

    //Shell_NotifyIconW(NIM_ADD, &nid);
}

// ������ ���ν��� ����
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // ���÷��� �޽��� ����
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
        // ������� ��Ʈ ���� �� ����
        static HFONT hFont = NULL;
        if (!hFont) {
            LOGFONTW lf = { 0 };
            lf.lfHeight = -14; // 16px ũ��
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
        AppendMenuW(hSubMenu, MF_STRING, IDR_FILE_SELECT_PAK, L"PAK ���� ����(&P)");
        AppendMenuW(hSubMenu, MF_STRING, IDR_FILE_SELECT_INF, L"INF ���� ����(&I)");
        AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hSubMenu, MF_STRING, IDR_FILE_SAVE_INF, L"�⺻ INF ���� ����(&S)");
        AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hSubMenu, MF_STRING, IDR_FILE_EXIT, L"����(&X)");
        AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, L"����(&F)");

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
        AppendMenuW(hSubMenu, MF_STRING | MF_POPUP, (UINT64)hPopupMenu, L"���� ����(&S)");
        AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hSubMenu, MF_STRING, IDR_MENU_SETOFFSET, L"������ ����(&O)");
        AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, L"����(&S)");

        hSubMenu = CreatePopupMenu();
        AppendMenuW(hSubMenu, MF_STRING, IDR_VIEW_LIST, L"���� ��ũ��Ʈ ���̽�(&L)");
        AppendMenuW(hSubMenu, MF_STRING, IDR_VIEW_ALL, L"��� ���� ����(&A)");
        AppendMenuW(hSubMenu, MF_STRING, IDR_VIEW_WDIR, L"���� ���(&R)");
        AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, L"����(&V)");

        hSubMenu = CreatePopupMenu();
        AppendMenuW(hSubMenu, MF_STRING, IDR_HELP_INST, L"����(&I)");
        AppendMenuW(hSubMenu, MF_STRING, IDR_HELP_ABOUT, L"���α׷� ����(&A)");
        AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, L"����(&H)");

        SetMenu(hwnd, hMenu);

        uShellRestart = RegisterWindowMessage("TaskbarCreated");
        break;
    }
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_LEFT:
        case VK_DOWN: {
            // ����,�Ʒ��� ����Ű ó��
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
            // ������,���� ����Ű ó��
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
            // ����Ű, ESC ó��
            ScanProcess::GetInstance().ControlPlayOffset(RESET_OFFSET);
            int newidx = DataManager::GetInstance().CalculateAdjustedSubIndex(ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK));
            DataManager::GetInstance().SetCurrentSubIndex(newidx);
            if (wParam == VK_RETURN) {
                AudioManager::GetInstance().PlayCurrentAudio();
            }
            break;
        }
        case VK_SPACE: {
            // �����̽� ó��
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
            // status bar ũ�� ����
            if (hDebug) {
                SendMessageA(hDebug, WM_SIZE, 0, 0);
            }
            // ����Ʈ ��Ʈ�� ũ�� ���� (status bar ���̸�ŭ ����)
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
            std::wstring filePathW = DialogUtil::GetInstance().ShowOpenFileDialogW(hwnd, L"PAK ���� (*.pak,*.bin,*.sfs)\0*.pak;*.bin;*.sfs\0��� ���� (*.*)\0*.*\0", L"PAK ���� ����");
            if (filePathW != L"") {
                DataManager::GetInstance().SetPackPath(filePathW);
                MessageBoxW(hwnd, (filePathW + L"\n\npak ���� ��ΰ� ����Ǿ����ϴ�.").c_str(), L"PAK ���� ����", MB_OK);
            }
            break;
        }
        case IDR_FILE_SELECT_INF: {
            std::wstring filePathW = DialogUtil::GetInstance().ShowOpenFileDialogW(hwnd, L"��� ���� (*.inf)\0*.inf\0��� ���� (*.*)\0*.*\0", L"INF ���� ����");
            if (filePathW != L"") {
                DataManager::GetInstance().SetInfPath(filePathW);
                MessageBoxW(hwnd, (filePathW + L"\n\ninf ���� ��ΰ� ����Ǿ����ϴ�.").c_str(), L"INF ���� ����", MB_OK);
            }
            break;
        }
        case IDR_FILE_SAVE_INF:
            if (DataManager::GetInstance().SaveDefaultInf()) {
                MessageBoxW(hwnd
                    , (DataManager::GetInstance().GetExecutablePath() + L"voice.inf\n\n���� ���� ������ voice.inf ������ �����Ͽ����ϴ�.").c_str()
                    , L"INF ���� ���� ����", MB_OK | MB_ICONASTERISK);
            }
            else {
                MessageBoxW(hwnd, L"���� ���� ������ voice.inf ������ �����մϴ�.", L"INF ���� ���� ����", MB_OK | MB_ICONERROR);
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
            std::wstring promptText = L"������ ������ �� �Է�";
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
                    auto& subMap = audioList[indexNum]; // indexNum ������ �ڵ� ����
                    subMap = pakFileIndex->fileName;
                }
            }
            DialogUtil::GetInstance().ShowListInputDialog(hwnd, selectValue, L"���� ��ũ��Ʈ ���̽� ���", audioList);
            audioList.erase(audioList.begin(), audioList.end());
            break;
        }
        case IDR_VIEW_ALL: {
            int i = 0;
            int selectValue = 0;
            std::map<int, std::string> audioList;
            for (auto& it : DataManager::GetInstance().GetPakFiles()) {
                auto& subMap = audioList[i++]; // indexNum ������ �ڵ� ����
                subMap = it.fileName;
            }
            DialogUtil::GetInstance().ShowListInputDialog(hwnd, selectValue, L"��ü ���� ���", audioList);
            audioList.erase(audioList.begin(), audioList.end());
            break;
        }
        case IDR_VIEW_WDIR: {
            wchar_t buff[MAX_PATH];
            GetCurrentDirectoryW(MAX_PATH, buff);
            std::wstringstream ss;
            std::wstring packPath = DataManager::GetInstance().GetPackPath();
            if (packPath.size() == 0) {
                packPath = L"�ε���� ����";
            }
            std::wstring infPath = DataManager::GetInstance().GetInfPath();
            if (infPath.size() == 0) {
                infPath = L"���� INF ���� ���� ��...";
            }
            ss << L"VoicePlayer ���� ���:\n" << DataManager::GetInstance().GetExecutablePath() << "\n";
            ss << L"\nRoutesDVD ���� ���:\n" << ScanProcess::GetInstance().GetTargetProcessPath() << "\n";
            ss << L"\nVoice Pack ����:\n" << packPath << "\n";
            ss << L"\nVoice INF ����:\n" << infPath << "\n";
            MessageBoxW(hwnd, ss.str().c_str(), L"���� �˻� ���", MB_OK);
            break;
        }
        case IDR_HELP_INST:
            MessageBoxW(
                hwnd,
                TEXT(
                    L"���̽� ���� �غ� (��1)\n"
                    L"   - �̸� ��Ű¡�� voice.pak ����\n"
                    L"   - �Ǵ� PS2 �̹��� ���� RTSOUND.SFS ����\n"
                    L"   - �Ǵ� PSP �̹��� ���� V0_FILE.BIN, V1_FILE.BIN ����\n"
                    L"\n"
                    L"\n"
                    L"1. ���̽� ���ϰ� inf ���� ������ ���� ���� �Ǵ� \n"
                    L"   RoutesVoice.exe�� ������ ������ �����մϴ�.\n"
                    L"\n"
                    L"2. RoutesVoice.exe�� RoutesDVD.exe�� �����մϴ�.\n"
                    L"   ���̽� ����� ���� ������ ���� ���� �����մϴ�.\n"
                    L"   (���� ����: RoutesDVD, RoutesDVD_JP, RoutesDVD_KR)\n"
                    L"\n"
                    L"3. ���� ���� �� ���� ��ũ�� �ٸ� ��� inf ������ ���� �� �����մϴ�.\n"
                    L"   INF�� RoutesVoice�� ���� ���� ������ �����˴ϴ�.\n"
                    L"   ����Ű�� ���� ������ �����ϸ� ��ũ�� ���纻 ����,\n"
                    L"   ȭ�鿡 ��µ� ���ڸ� inf�� �Է��ϰ� �����ϸ� ����˴ϴ�.\n"
                    L"   (����Ǵ� �������̸� ��Ȯ���� ���� �� �ֽ��ϴ�)\n"
                    L"\n"
                    L"\n"
                    L"**������ ���� ����Ű**\n"
                    L"\n"
                    L"�¿� ����Ű: ������ ����/���� (����� ���)\n"
                    L"���� ����Ű: ������ ����/���� (��� ����)\n"
                    L"ESC, ����Ű: ������ �ʱ�ȭ (ESC=��� ����, ����=���)\n"
                    L"�����̽� Ű: ���� ����� ���\n"
                ),
                TEXT(L"����"),
                MB_OK
            );
            break;
        case IDR_HELP_ABOUT:
#ifdef _WIN64
            MessageBoxW(
                hwnd,
                TEXT(
                    L"Routes Voice Player v1.0 (64��Ʈ)\n"
                    L"\n"
                    L"����OS: Windows 7 �̻�\n"
                ),
                TEXT(L"���α׷� ����"),
                MB_OK | MB_ICONASTERISK
            );
#else
            MessageBoxW(
                hwnd,
                TEXT(
                    L"Routes Voice Player v1.0 (32��Ʈ)\n"
                    L"\n"
                    L"����OS: Windows XP �̻�\n"
                ),
                TEXT(L"���α׷� ����"),
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
            AppendMenuW(hSubMenu, MF_STRING, IDR_TRAY_SHOW, L"����(&O)");
            AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hSubMenu, MF_STRING, IDR_TRAY_EXIT, L"����(&E)");

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