#pragma once
#include "Common.h"

namespace Util {
    // Select GRP files (allows multiple selection)
    std::vector<std::wstring> GetGrpFiles();

    // Select folder for update (checks for 'png' subfolder)
    std::wstring GetUpdateFolder();
}
