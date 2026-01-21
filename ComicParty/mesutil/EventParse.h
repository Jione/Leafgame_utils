#pragma once
#include <Windows.h>
#include <string>
#include <vector>

static char unk_46B204[] = "어머니";
static char unk_46B210[] = "아버지";
static char unk_46B21C[] = "방송";
static char unk_46B224[] = "인쇄소 직원";
static char unk_46B230[] = "선배";
static char unk_46B238[] = "역무원";
static char unk_46B240[] = "여자";
static char unk_46B248[] = "엄마";
static char unk_46B250[] = "아빠";
static char unk_46B258[] = "유카";
static char unk_46B260[] = "마유";
static char unk_46B268[] = "미호";
static char unk_46B270[] = "코스플레이어";
static char unk_46B280[] = "여자 목소리";
static char unk_46B28C[] = "리포터";
static char unk_46B298[] = "헤모헤모";
static char unk_46B2A4[] = "마히루";
static char unk_46B2AC[] = "남자 목소리";
static char unk_46B2B4[] = "모모";
static char unk_46B2BC[] = "여관 종업원";
static char unk_46B2C4[] = "웨이트리스";
static char unk_46B2D4[] = "판매원";
static char unk_46B2DC[] = "해설자";
static char unk_46B2E4[] = "사회자";
static char unk_46B2EC[] = "점원";
static char unk_46B2F4[] = "소녀";
static char unk_46B2FC[] = "어머님";
static char unk_46B304[] = "누님";
static char unk_46B310[] = "아저씨";
static char unk_46B31C[] = "아이";
static char unk_46B324[] = "종업원";
static char unk_46B32C[] = "？";
static char unk_46B330[] = "전화";
static char unk_46B338[] = "손님";
static char unk_46B33C[] = "소녀 목소리 Ｃ";
static char unk_46B34C[] = "소녀 목소리 Ｂ";
static char unk_46B35C[] = "소녀 목소리 Ａ";
static char unk_46B36C[] = "소녀 목소리";
static char unk_46B378[] = "스태프";
static char unk_46B384[] = "삼인조";
static char unk_46B38C[] = "오타쿠";
static char unk_46B394[] = "편집장";
static char unk_46B39C[] = "운송업자";
static char unk_46B3A8[] = "수수께끼 남자";
static char unk_46B3B0[] = "타이시";
static char unk_46B3B8[] = "이쿠미";
static char unk_46B3C0[] = "치사";
static char unk_46B3C8[] = "레이코";
static char unk_46B3D0[] = "아사히";
static char unk_46B3D8[] = "아야";
static char unk_46B3DC[] = "에이미";
static char unk_46B3E4[] = "유우";
static char unk_46B3EC[] = "미나미";
static char unk_46B3F0[] = "미즈키";
static char unk_46B3F8[] = "주인공";

static char* CharaName[] = {
    unk_46B3F8, unk_46B3F0, unk_46B3EC, unk_46B3E4, unk_46B3DC, unk_46B3D8, unk_46B3D0, unk_46B3C8,
    unk_46B3C0, unk_46B3B8, unk_46B3B0, unk_46B3A8, unk_46B39C, unk_46B394, unk_46B38C, unk_46B384,
    unk_46B378, unk_46B36C, unk_46B35C, unk_46B34C, unk_46B33C, unk_46B338, unk_46B330, unk_46B32C,
    unk_46B324, unk_46B31C, unk_46B310, unk_46B304, unk_46B2FC, unk_46B2F4, unk_46B2EC, unk_46B2E4,
    unk_46B2DC, unk_46B2D4, unk_46B2C4, unk_46B2BC, unk_46B2B4, unk_46B2AC, unk_46B2A4, unk_46B298,
    unk_46B28C, unk_46B280, unk_46B270, unk_46B384, unk_46B268, unk_46B260, unk_46B258, unk_46B250,
    unk_46B248, unk_46B240, unk_46B238, unk_46B230, unk_46B224, unk_46B21C, unk_46B210, unk_46B204
};

namespace Event {

    // eve.dat scan -> speaker table (index = msgId, value = characterId)
    // value meanings:
    //   >=0 : character id
    //   -1  : no visible speaker / unknown
    //   -2  : used by choice (0x14) and never printed as message (so far)
    bool ParseCharaIDTable(LPWSTR lpTargetFile, std::vector<int>& outTable);
}
