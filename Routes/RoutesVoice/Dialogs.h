#pragma once
#include <windows.h> // HWND, INT_PTR �� WinAPI Ÿ��
#include <string>    // std::wstring
#include <map>		 // std::map

class DialogUtil {
public:
    static DialogUtil& GetInstance(); // �̱���

    // ���ڸ� �Է¹޴� ��� ���̾�α׸� ǥ���ϴ� �Լ�
    bool ShowNumberInputDialog(HWND hParent, int& outNumber, const std::wstring& prompt, int initialValue = 0);

    // ���� â�� ����ϴ� ���̾�α� ǥ�� �Լ�
    bool ShowListInputDialog(HWND hParent, int& outValue, const std::wstring& prompt, const std::map<int, std::string>& items);

    // ���� ��θ� ��� ���̾�α� �Լ�
    std::wstring ShowOpenFileDialogW(HWND hwnd, const wchar_t* filter = L"��� ���� (*.*)\0*.*\0", const wchar_t* title = L"���� ����");

private:
    DialogUtil(); // �̱���
    ~DialogUtil();
    DialogUtil(const DialogUtil&) = delete;
    DialogUtil& operator=(const DialogUtil&) = delete;

    // ���� �Է� ���̾�α� ���ν���
    INT_PTR CALLBACK NumberInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // ����Ʈ ���̾�α� ���ν���
    INT_PTR CALLBACK ListInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // ���� �Լ�: ���ڿ��� ��ȿ�� �������� Ȯ�� (���� ���)
    bool IsValidNumber(const std::wstring& str);

    // ���� ����ü
    struct StatusNumberInputDialog {
        bool g_inputSuccess = false;
        std::wstring g_promptText = L"";
        int g_inputValue = 0;
        int g_initialValue = 0;
    } NumberInputDialogState;

    struct StatusInputDialogState {
        bool g_inputSuccess;
        std::wstring g_promptText;
        std::map<int, std::string>* g_items = nullptr;
        int g_selectedValue;
    } ListInputDialogState;
};