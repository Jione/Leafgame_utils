#pragma once
#include <windows.h> // HWND, INT_PTR 등 WinAPI 타입
#include <string>    // std::wstring
#include <map>		 // std::map

class DialogUtil {
public:
    static DialogUtil& GetInstance(); // 싱글톤

    // 숫자를 입력받는 모달 다이얼로그를 표시하는 함수
    bool ShowNumberInputDialog(HWND hParent, int& outNumber, const std::wstring& prompt, int initialValue = 0);

    // 문자 창을 출력하는 다이얼로그 표시 함수
    bool ShowListInputDialog(HWND hParent, int& outValue, const std::wstring& prompt, const std::map<int, std::string>& items);

    // 파일 경로를 얻는 다이얼로그 함수
    std::wstring ShowOpenFileDialogW(HWND hwnd, const wchar_t* filter = L"모든 파일 (*.*)\0*.*\0", const wchar_t* title = L"파일 선택");

private:
    DialogUtil(); // 싱글톤
    ~DialogUtil();
    DialogUtil(const DialogUtil&) = delete;
    DialogUtil& operator=(const DialogUtil&) = delete;

    // 숫자 입력 다이얼로그 프로시저
    INT_PTR CALLBACK NumberInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // 리스트 다이얼로그 프로시저
    INT_PTR CALLBACK ListInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // 헬퍼 함수: 문자열이 유효한 숫자인지 확인 (내부 사용)
    bool IsValidNumber(const std::wstring& str);

    // 내장 구조체
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