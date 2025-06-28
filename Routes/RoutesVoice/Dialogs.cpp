#include <exception>  // For std::stoi exceptions
#include <iostream>
#include <string>     // std::string
#include <vector>     // std::vector
#include <algorithm>  // std::transform
#include <cctype>     // ::isdigit

#include "Dialogs.h"
#include "resource.h" // 다이얼로그 리소스 ID 포함
#include "DataManager.h"
#include "AudioManager.h"
#include "StringUtil.h"


DialogUtil::DialogUtil()
{
    // 생성자
}

DialogUtil::~DialogUtil() {
    // 소멸자
}

DialogUtil& DialogUtil::GetInstance() {
    static DialogUtil instance;
    return instance;
}

// 헬퍼 함수: 문자열이 유효한 숫자인지 확인 (양의 정수)
// 여기서는 간단하게 모든 문자가 숫자인지 확인하고, 비어있지 않은지 확인합니다.
// 실제로는 오버플로우 등 더 강력한 유효성 검사가 필요할 수 있습니다.
bool DialogUtil::IsValidNumber(const std::wstring& str) {
    if (str.empty()) {
        return false;
    }

    size_t startIndex = 0;
    if (str[0] == L'-') {
        if (str.length() == 1) { // "-" 단독으로는 유효하지 않음
            return false;
        }
        startIndex = 1; // 첫 문자가 '-'이면 다음 문자부터 검사 시작
    }

    // 나머지 문자들은 모두 숫자여야 함
    for (size_t i = startIndex; i < str.length(); ++i) {
        if (!::isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

// 다이얼로그 프로시저 (콜백 함수)
// 다이얼로그에 발생하는 메시지를 처리합니다.
INT_PTR CALLBACK DialogUtil::NumberInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        // 다이얼로그 초기화 시 호출됩니다.
        // 에디트 컨트롤에 기본값 설정 (빈 문자열)
        SetDlgItemTextW(hDlg, IDC_EDIT_NUMBER, L"");
        // g_initialValue 값을 에디트 컨트롤에 설정합니다.
        SetDlgItemTextW(hDlg, IDC_EDIT_NUMBER, std::to_wstring(NumberInputDialogState.g_initialValue).c_str());
        // 프롬프트 텍스트 설정
        SetDlgItemTextW(hDlg, IDC_STATIC_PROMPT, NumberInputDialogState.g_promptText.c_str());
        // 에러 메시지 초기화 및 숨김
        SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, L"");
        ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ERROR), SW_HIDE);
        return TRUE; // 포커스를 첫 번째 컨트롤에 설정
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDOK: {
            // "확인" 버튼 클릭
            wchar_t szInput[256];
            GetDlgItemTextW(hDlg, IDC_EDIT_NUMBER, szInput, sizeof(szInput) / sizeof(szInput[0]));
            std::wstring sInput(szInput);

            if (IsValidNumber(sInput)) {
                try {
                    NumberInputDialogState.g_inputValue = std::stoi(sInput);
                    NumberInputDialogState.g_inputSuccess = true;
                    EndDialog(hDlg, IDOK); // 다이얼로그 종료, 반환 값 IDOK
                }
                catch (const std::out_of_range&) {
                    // 숫자가 너무 크거나 작을 경우
                    SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, L"숫자가 너무 크거나 작습니다.");
                    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ERROR), SW_SHOW);
                }
                catch (const std::invalid_argument&) {
                    // 숫자로 변환할 수 없는 경우 (IsValidNumber에서 대부분 걸러지지만 혹시)
                    SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, L"유효하지 않은 숫자 형식입니다.");
                    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ERROR), SW_SHOW);
                }
            }
            else {
                // 유효하지 않은 입력 (숫자가 아니거나 빈 문자열)
                SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, L"유효한 숫자를 입력하세요.");
                ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ERROR), SW_SHOW);
            }
            return TRUE;
        }

        case IDCANCEL: {
            // "취소" 버튼 클릭 또는 ESC 키
            NumberInputDialogState.g_inputSuccess = false; // 취소되었음을 알림
            EndDialog(hDlg, IDCANCEL); // 다이얼로그 종료, 반환 값 IDCANCEL
            return TRUE;
        }
        }
        break;
    }

    case WM_CLOSE: {
        // X 버튼 클릭
        NumberInputDialogState.g_inputSuccess = false; // 취소되었음을 알림
        EndDialog(hDlg, IDCANCEL); // 다이얼로그 종료
        return TRUE;
    }
    }
    return FALSE; // 메시지를 처리하지 않았음을 나타냄
}

// 숫자를 입력받는 공개 함수
bool DialogUtil::ShowNumberInputDialog(HWND hParent, int& outNumber, const std::wstring& prompt, int initialValue) {
    // 다이얼로그 상태 초기화
    NumberInputDialogState.g_inputValue = 0;
    NumberInputDialogState.g_inputSuccess = false;
    NumberInputDialogState.g_promptText = prompt;
    NumberInputDialogState.g_initialValue = initialValue;

    // 다이얼로그 표시 (모달 방식)
    INT_PTR result = DialogBoxW(
        GetModuleHandleW(NULL),
        MAKEINTRESOURCEW(IDD_NUMBER_INPUT_DIALOG),
        hParent,
        [](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
            return DialogUtil::GetInstance().NumberInputDlgProc(hDlg, message, wParam, lParam);
        }
    );

    if (result == IDOK && NumberInputDialogState.g_inputSuccess) {
        outNumber = NumberInputDialogState.g_inputValue;
        return true;
    }
    return false;
}

// 리스트 다이얼로그 프로시저
INT_PTR CALLBACK DialogUtil::ListInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        // 프롬프트 텍스트 설정
        SetDlgItemTextW(hDlg, IDC_STATIC_PROMPT, ListInputDialogState.g_promptText.c_str());
        // 리스트 컨트롤에 항목 추가
        HWND hList = GetDlgItem(hDlg, IDC_LIST_ITEMS);
        for (const auto& item : *ListInputDialogState.g_items) {
            std::string listString = item.second;
            int idx = (int)SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)listString.c_str());
            SendMessageA(hList, LB_SETITEMDATA, idx, (LPARAM)item.first);
        }
        return TRUE;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDOK: {
            HWND hList = GetDlgItem(hDlg, IDC_LIST_ITEMS);
            int sel = (int)SendMessageA(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                ListInputDialogState.g_selectedValue = (int)SendMessageA(hList, LB_GETITEMDATA, sel, 0);
                auto& fileInfo = DataManager::GetInstance().GetPakFiles()[ListInputDialogState.g_selectedValue];
                AudioManager::GetInstance().PlayAudio(fileInfo, 1); // 1번 재생
                ListInputDialogState.g_inputSuccess = true;
            }
            return TRUE;
        }
        case IDCANCEL: {
            ListInputDialogState.g_inputSuccess = false;
            HWND hList = GetDlgItem(hDlg, IDC_LIST_ITEMS);
            // 리스트 컨트롤의 모든 항목 삭제
            SendMessageA(hList, LB_RESETCONTENT, 0, 0);
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        case IDC_LIST_ITEMS: {
            switch (HIWORD(wParam)) {
                case LBN_DBLCLK: {
                    HWND hList = GetDlgItem(hDlg, IDC_LIST_ITEMS);
                    int sel = (int)SendMessageA(hList, LB_GETCURSEL, 0, 0);
                    if (sel != LB_ERR) {
                        ListInputDialogState.g_selectedValue = (int)SendMessageA(hList, LB_GETITEMDATA, sel, 0);
                        auto& fileInfo = DataManager::GetInstance().GetPakFiles()[ListInputDialogState.g_selectedValue];
                        AudioManager::GetInstance().PlayAudio(fileInfo, 1); // 1번 재생
                        ListInputDialogState.g_inputSuccess = true;
                    }
                    return TRUE;
                }
            }
            
            case WM_KEYDOWN:
                switch (wParam) {
                }
                break;
            break;
        }
        }
        break;
    }
    case WM_CLOSE: {
        ListInputDialogState.g_inputSuccess = false;
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    }
    return FALSE;
}

// vector<pair<int, wstring>> 기반 리스트 입력 다이얼로그 함수
bool DialogUtil::ShowListInputDialog(HWND hParent, int& outValue, const std::wstring& prompt, const std::map<int, std::string>& items) {
    if (items.size() == 0) {
        MessageBoxW(hParent, L"재생할 수 있는 파일이 없습니다.", prompt.c_str(), MB_OK);
        return false;
    }
    ListInputDialogState.g_items = const_cast<std::map<int, std::string>*>(&items);
    ListInputDialogState.g_selectedValue = 0;
    ListInputDialogState.g_inputSuccess = false;
    ListInputDialogState.g_promptText = prompt;

    INT_PTR result = DialogBoxW(
        GetModuleHandleW(NULL),
        MAKEINTRESOURCEW(IDD_LIST_INPUT_DIALOG),
        hParent,
        [](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
            return DialogUtil::GetInstance().ListInputDlgProc(hDlg, message, wParam, lParam);
        }
    );

    if (result == IDOK && ListInputDialogState.g_inputSuccess) {
        outValue = ListInputDialogState.g_selectedValue;
        return true;
    }
    return false;
}

// 파일 선택 다이얼로그 함수
std::wstring DialogUtil::ShowOpenFileDialogW(HWND hwnd, const wchar_t* filter, const wchar_t* title) {
    OPENFILENAMEW ofn = { 0 };
    
    wchar_t szFile[MAX_PATH] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ofn.lpstrTitle = title;

    std::wstring pathW = DataManager::GetInstance().GetExecutablePath();
    ofn.lpstrInitialDir = pathW.c_str();

    if (GetOpenFileNameW(&ofn) == TRUE) {
        return szFile;
    }
    return L"";
}