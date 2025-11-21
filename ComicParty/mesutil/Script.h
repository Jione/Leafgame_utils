#pragma once
#include <Windows.h>
#include <string>
#include "Util.h"

namespace Script {

    // MES -> XLSX
    bool ParseMes(LPWSTR lpTargetFile);

    // XLSX -> MES
    bool ApplyTransTo(LPWSTR lpTargetFile, Util::EncodingType targetEnc);
}
