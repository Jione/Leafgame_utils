#pragma once
#include "Common.h"

namespace Util {
    // File Selection
    bool GetArchiveFiles(std::vector<std::wstring>& outPaths);
    std::wstring GetTargetFolder();

    // String Conversion
    std::wstring SJIS_To_Wide(const char* sjis);
    std::string Wide_To_SJIS(const std::wstring& wide);

    // Filename Handling
    std::wstring GetCleanFileName(const std::wstring& filename); // Remove @...
}
