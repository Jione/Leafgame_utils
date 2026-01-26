#include <windows.h>
#include <locale.h>

#include "resource.h"
#include "LocalizationText.h"
#include "LocalizationGraphic.h"

static void SetButtonsEnabled(HWND hDlg, BOOL enabled) {
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_TEXT_EXTRACT), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_TEXT_BUILD), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_GFX_EXTRACT), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_GFX_BUILD), enabled);
}

static BOOL ConfirmTask(HWND hDlg, const wchar_t* msgText) {
    const int ret = MessageBoxW(
        hDlg,
        msgText,
        L"경고",
        MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2
    );
    return (ret == IDYES) ? TRUE : FALSE;
}

static void RunTask(HWND hDlg, const wchar_t* taskTitle, void (*taskFn)()) {
    HWND hStatus = GetDlgItem(hDlg, IDC_STATIC_STATUS);
    if (hStatus) SetWindowTextW(hStatus, taskTitle);

    SetButtonsEnabled(hDlg, FALSE);
    SetCursor(LoadCursorW(nullptr, IDC_WAIT));

    if (taskFn) taskFn();

    SetCursor(LoadCursorW(nullptr, IDC_ARROW));
    SetButtonsEnabled(hDlg, TRUE);

    if (hStatus) SetWindowTextW(hStatus, L"대기 중");

    MessageBoxW(hDlg, L"작업이 완료되었습니다.", taskTitle, MB_OK | MB_ICONINFORMATION);
}

static INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;

    switch (msg) {
    case WM_INITDIALOG:
        SetWindowTextW(GetDlgItem(hDlg, IDC_STATIC_STATUS), L"대기 중");
        return TRUE;

    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        switch (id) {
        case IDC_BTN_TEXT_EXTRACT:
            if (!ConfirmTask(hDlg, L"KoS 폴더 내의 작업 내용이 손실될 수 있습니다.\n\n추출 작업을 진행하시겠습니까?")) return TRUE;
            RunTask(hDlg, L"스크립트 추출 중", &Localization::ExtractTextResources);
            return TRUE;

        case IDC_BTN_TEXT_BUILD:
            if (!ConfirmTask(hDlg, L"KoS.pak 파일이 덮어쓰기 됩니다.\n\n병합 작업을 진행하시겠습니까?")) return TRUE;
            RunTask(hDlg, L"KoS 병합 중", &Localization::BuildTextArchive);
            return TRUE;

        case IDC_BTN_GFX_EXTRACT:
            if (!ConfirmTask(hDlg, L"KoR 폴더 내의 작업 내용이 손실될 수 있습니다.\n\n추출 작업을 진행하시겠습니까?")) return TRUE;
            RunTask(hDlg, L"그래픽 추출 중", &Localization::ExtractGraphicResources);
            return TRUE;

        case IDC_BTN_GFX_BUILD:
            if (!ConfirmTask(hDlg, L"KoR.pak 파일이 덮어쓰기 됩니다.\n\n병합 작업을 진행하시겠습니까?")) return TRUE;
            RunTask(hDlg, L"KoR 병합 중", &Localization::BuildGraphicArchive);
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;

        default:
            return FALSE;
        }
    }

    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return TRUE;
    }

    return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
    // Keep locale for any stdout messages inside worker functions.
    setlocale(LC_ALL, "Korean");

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_MAIN_DIALOG), nullptr, MainDlgProc, 0);
    return 0;
}