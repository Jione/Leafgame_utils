#include "Util.h"
#include <shlobj.h> // For SHBrowseForFolder

namespace Util {

    std::vector<std::wstring> GetGrpFiles() {
        std::vector<std::wstring> selectedFiles;

        // Buffer for multiple file names. Size is arbitrary but large enough.
        const int BUFFER_SIZE = 32768;
        wchar_t* filenameBuffer = new wchar_t[BUFFER_SIZE];
        memset(filenameBuffer, 0, sizeof(wchar_t) * BUFFER_SIZE);

        OPENFILENAMEW ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = filenameBuffer;
        ofn.nMaxFile = BUFFER_SIZE;
        ofn.lpstrFilter = L"GRP Files (*.grp)\0*.grp\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = L".\\";
        // OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_DONTADDTORECENT
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_DONTADDTORECENT;

        if (GetOpenFileNameW(&ofn)) {
            // Parse the buffer.
            // If multiple files: Directory\0File1\0File2\0...
            // If single file: FullPath\0

            std::wstring dir = filenameBuffer;
            size_t len = dir.length();

            if (filenameBuffer[len + 1] == 0) {
                // Single file selection
                selectedFiles.push_back(dir);
            }
            else {
                // Multiple file selection
                wchar_t* p = filenameBuffer + len + 1;
                while (*p) {
                    std::wstring fullPath = dir;
                    if (fullPath.back() != L'\\') fullPath += L'\\';
                    fullPath += p;
                    selectedFiles.push_back(fullPath);
                    p += wcslen(p) + 1;
                }
            }
        }

        delete[] filenameBuffer;
        return selectedFiles;
    }

    static int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
        WCHAR szInitPath[MAX_PATH];
        if (uMsg == BFFM_INITIALIZED) {
            GetCurrentDirectoryW(MAX_PATH, szInitPath);
            ::SendMessageW(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, (LPARAM)szInitPath);
        }
        return 0;
    }

    std::wstring GetUpdateFolder() {
        wchar_t path[MAX_PATH];
        BROWSEINFOW bi = { 0 };
        bi.lpszTitle = L"Select folder containing GRP files to update";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        bi.lpfn = BrowseCallback;

        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (pidl != 0) {
            if (SHGetPathFromIDListW(pidl, path)) {
                fs::path selectedPath(path);
                fs::path pngPath = selectedPath / L"png";

                // Check if internal 'png' folder exists
                if (fs::exists(pngPath) && fs::is_directory(pngPath)) {
                    CoTaskMemFree(pidl);
                    return std::wstring(path);
                }
                else {
                    MessageBoxW(NULL, L"Selected folder does not contain a 'png' subfolder.", L"Error", MB_ICONERROR);
                }
            }
            CoTaskMemFree(pidl);
        }
        return L"";
    }
}
