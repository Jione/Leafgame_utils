#include <exception>  // For std::stoi exceptions
#include <iostream>
#include <string>     // std::string
#include <vector>     // std::vector
#include <algorithm>  // std::transform
#include <cctype>     // ::isdigit

#include "Dialogs.h"
#include "resource.h" // ���̾�α� ���ҽ� ID ����
#include "DataManager.h"
#include "AudioManager.h"
#include "StringUtil.h"


DialogUtil::DialogUtil()
{
    // ������
}

DialogUtil::~DialogUtil() {
    // �Ҹ���
}

DialogUtil& DialogUtil::GetInstance() {
    static DialogUtil instance;
    return instance;
}

// ���� �Լ�: ���ڿ��� ��ȿ�� �������� Ȯ�� (���� ����)
// ���⼭�� �����ϰ� ��� ���ڰ� �������� Ȯ���ϰ�, ������� ������ Ȯ���մϴ�.
// �����δ� �����÷ο� �� �� ������ ��ȿ�� �˻簡 �ʿ��� �� �ֽ��ϴ�.
bool DialogUtil::IsValidNumber(const std::wstring& str) {
    if (str.empty()) {
        return false;
    }

    size_t startIndex = 0;
    if (str[0] == L'-') {
        if (str.length() == 1) { // "-" �ܵ����δ� ��ȿ���� ����
            return false;
        }
        startIndex = 1; // ù ���ڰ� '-'�̸� ���� ���ں��� �˻� ����
    }

    // ������ ���ڵ��� ��� ���ڿ��� ��
    for (size_t i = startIndex; i < str.length(); ++i) {
        if (!::isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

// ���̾�α� ���ν��� (�ݹ� �Լ�)
// ���̾�α׿� �߻��ϴ� �޽����� ó���մϴ�.
INT_PTR CALLBACK DialogUtil::NumberInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        // ���̾�α� �ʱ�ȭ �� ȣ��˴ϴ�.
        // ����Ʈ ��Ʈ�ѿ� �⺻�� ���� (�� ���ڿ�)
        SetDlgItemTextW(hDlg, IDC_EDIT_NUMBER, L"");
        // g_initialValue ���� ����Ʈ ��Ʈ�ѿ� �����մϴ�.
        SetDlgItemTextW(hDlg, IDC_EDIT_NUMBER, std::to_wstring(NumberInputDialogState.g_initialValue).c_str());
        // ������Ʈ �ؽ�Ʈ ����
        SetDlgItemTextW(hDlg, IDC_STATIC_PROMPT, NumberInputDialogState.g_promptText.c_str());
        // ���� �޽��� �ʱ�ȭ �� ����
        SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, L"");
        ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ERROR), SW_HIDE);
        return TRUE; // ��Ŀ���� ù ��° ��Ʈ�ѿ� ����
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDOK: {
            // "Ȯ��" ��ư Ŭ��
            wchar_t szInput[256];
            GetDlgItemTextW(hDlg, IDC_EDIT_NUMBER, szInput, sizeof(szInput) / sizeof(szInput[0]));
            std::wstring sInput(szInput);

            if (IsValidNumber(sInput)) {
                try {
                    NumberInputDialogState.g_inputValue = std::stoi(sInput);
                    NumberInputDialogState.g_inputSuccess = true;
                    EndDialog(hDlg, IDOK); // ���̾�α� ����, ��ȯ �� IDOK
                }
                catch (const std::out_of_range&) {
                    // ���ڰ� �ʹ� ũ�ų� ���� ���
                    SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, L"���ڰ� �ʹ� ũ�ų� �۽��ϴ�.");
                    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ERROR), SW_SHOW);
                }
                catch (const std::invalid_argument&) {
                    // ���ڷ� ��ȯ�� �� ���� ��� (IsValidNumber���� ��κ� �ɷ������� Ȥ��)
                    SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, L"��ȿ���� ���� ���� �����Դϴ�.");
                    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ERROR), SW_SHOW);
                }
            }
            else {
                // ��ȿ���� ���� �Է� (���ڰ� �ƴϰų� �� ���ڿ�)
                SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, L"��ȿ�� ���ڸ� �Է��ϼ���.");
                ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ERROR), SW_SHOW);
            }
            return TRUE;
        }

        case IDCANCEL: {
            // "���" ��ư Ŭ�� �Ǵ� ESC Ű
            NumberInputDialogState.g_inputSuccess = false; // ��ҵǾ����� �˸�
            EndDialog(hDlg, IDCANCEL); // ���̾�α� ����, ��ȯ �� IDCANCEL
            return TRUE;
        }
        }
        break;
    }

    case WM_CLOSE: {
        // X ��ư Ŭ��
        NumberInputDialogState.g_inputSuccess = false; // ��ҵǾ����� �˸�
        EndDialog(hDlg, IDCANCEL); // ���̾�α� ����
        return TRUE;
    }
    }
    return FALSE; // �޽����� ó������ �ʾ����� ��Ÿ��
}

// ���ڸ� �Է¹޴� ���� �Լ�
bool DialogUtil::ShowNumberInputDialog(HWND hParent, int& outNumber, const std::wstring& prompt, int initialValue) {
    // ���̾�α� ���� �ʱ�ȭ
    NumberInputDialogState.g_inputValue = 0;
    NumberInputDialogState.g_inputSuccess = false;
    NumberInputDialogState.g_promptText = prompt;
    NumberInputDialogState.g_initialValue = initialValue;

    // ���̾�α� ǥ�� (��� ���)
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

// ����Ʈ ���̾�α� ���ν���
INT_PTR CALLBACK DialogUtil::ListInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        // ������Ʈ �ؽ�Ʈ ����
        SetDlgItemTextW(hDlg, IDC_STATIC_PROMPT, ListInputDialogState.g_promptText.c_str());
        // ����Ʈ ��Ʈ�ѿ� �׸� �߰�
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
                AudioManager::GetInstance().PlayAudio(fileInfo, 1); // 1�� ���
                ListInputDialogState.g_inputSuccess = true;
            }
            return TRUE;
        }
        case IDCANCEL: {
            ListInputDialogState.g_inputSuccess = false;
            HWND hList = GetDlgItem(hDlg, IDC_LIST_ITEMS);
            // ����Ʈ ��Ʈ���� ��� �׸� ����
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
                        AudioManager::GetInstance().PlayAudio(fileInfo, 1); // 1�� ���
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

// vector<pair<int, wstring>> ��� ����Ʈ �Է� ���̾�α� �Լ�
bool DialogUtil::ShowListInputDialog(HWND hParent, int& outValue, const std::wstring& prompt, const std::map<int, std::string>& items) {
    if (items.size() == 0) {
        MessageBoxW(hParent, L"����� �� �ִ� ������ �����ϴ�.", prompt.c_str(), MB_OK);
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

// ���� ���� ���̾�α� �Լ�
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