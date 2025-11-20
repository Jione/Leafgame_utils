#pragma once
#include "Common.h"

namespace GRP {
    // Convert GRP/C16 to PNG
    bool ConvertPng(std::wstring lpTargetFile, bool bApplyMask);

    // Update GRP/C16/MSK from PNG
    // bPreservePalette: true = keep existing c16, false = generate new c16
    bool Update(std::wstring lpTargetFolder, bool bPreservePalette = false);
}
