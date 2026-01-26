#pragma once
#include <Windows.h>
#include <string>
#include "Util.h"

namespace Script {

    // MES -> XLSX
    bool ParseMes(LPWSTR lpTargetFile, bool exportMode);

    // XLSX -> MES
    bool ApplyTransTo(LPWSTR lpTargetFile);
}
