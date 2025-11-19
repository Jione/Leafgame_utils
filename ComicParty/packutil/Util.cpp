#include "Util.h"
#include <commdlg.h>
#include <shlobj.h>

namespace Util {
    bool GetArchiveFiles(std::vector<std::wstring>& outPaths) {
        wchar_t buffer[32768] = { 0 }; // Large buffer for multi-select
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = L"PAK Archives (*.pak)\0*.pak\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = buffer;
        ofn.nMaxFile = 32768;
        ofn.lpstrInitialDir = L".\\";
        ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

        if (GetOpenFileNameW(&ofn)) {
            wchar_t* p = buffer;
            std::wstring dir = p;
            p += dir.length() + 1;

            if (*p == 0) {
                // Single file selected
                outPaths.push_back(dir);
            }
            else {
                // Multiple files: Directory + \0 + File1 + \0 + File2 ...
                while (*p) {
                    std::wstring file = p;
                    outPaths.push_back(dir + L"\\" + file);
                    p += file.length() + 1;
                }
            }
            return true;
        }
        return false;
    }

    static int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
        WCHAR szInitPath[MAX_PATH];
        if (uMsg == BFFM_INITIALIZED) {
            GetCurrentDirectoryW(MAX_PATH, szInitPath);
            ::SendMessageW(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, (LPARAM)szInitPath);
        }
        return 0;
    }

    std::wstring GetTargetFolder() {
        wchar_t buffer[MAX_PATH];
        BROWSEINFOW bi = { 0 };

        bi.lpszTitle = L"Select Target Folder";
        bi.lpfn = BrowseCallback;
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_DONTGOBELOWDOMAIN;

        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (pidl != 0) {
            SHGetPathFromIDListW(pidl, buffer);
            CoTaskMemFree(pidl);
            return std::wstring(buffer);
        }
        return L"";
    }

    std::wstring SJIS_To_Wide(const char* sjis) {
        int len = MultiByteToWideChar(932, 0, sjis, -1, NULL, 0);
        if (len == 0) return L"";
        std::vector<wchar_t> buf(len);
        MultiByteToWideChar(932, 0, sjis, -1, buf.data(), len);
        return std::wstring(buf.data());
    }

    std::string Wide_To_SJIS(const std::wstring& wide) {
        int len = WideCharToMultiByte(932, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
        if (len == 0) return "";
        std::vector<char> buf(len);
        WideCharToMultiByte(932, 0, wide.c_str(), -1, buf.data(), len, NULL, NULL);
        return std::string(buf.data());
    }

    std::wstring GetCleanFileName(const std::wstring& filename) {
        // Remove everything after '@'
        size_t atPos = filename.find(L'@');
        if (atPos != std::wstring::npos) {
            std::wstring stem = filename.substr(0, atPos);
            // Need to preserve extension? Requirements say: "exclude after @ and include ext"
            // But usually @ logic implies stem modification. 
            // Let's assume @ is part of the name only, checking extension separately.
            // Re-reading: "@문자 이후의 파일명은 제외하고 ext를 포함하여 아카이브"
            // Example: file@01.png -> file.png

            size_t dotPos = filename.rfind(L'.');
            std::wstring ext = L"";
            if (dotPos != std::wstring::npos && dotPos > atPos) {
                ext = filename.substr(dotPos);
            }
            return stem + ext;
        }
        return filename;
    }
}
