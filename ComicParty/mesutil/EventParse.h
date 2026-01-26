#pragma once
#include <Windows.h>
#include <string>
#include <vector>

namespace Event {

    // eve.dat scan -> speaker table (index = msgId, value = characterId)
    // value meanings:
    //   >=0 : character id
    //   -1  : no visible speaker / unknown
    //   -2  : used by choice (0x14) and never printed as message (so far)
    bool ParseCharaIDTable(LPWSTR lpTargetFile, std::vector<int>& outTable);
    const wchar_t* GetIdName(int id);
    std::string GetUtf8Name(int id);
}
