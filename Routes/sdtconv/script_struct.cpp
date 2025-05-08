#include "scripts_struct.h"
// 캐릭터 이름 매칭 테이블 (Routes)
const CharactorInfo g_charactorList[] = {
    { "",                0 }, // 기본값
    { "리사=빅센",       1 },
    { "유아사 사츠키",   2 },
    { "후시미 유카리",   3 },
    { "타츠타 나나미",   4 },
    { "카지와라 유우나", 5 },
    { "사쿠야",          7 },
    { "에디",            11 },
    { "타카무라",        12 },
    { "다이고 고로",     13 },
    { "나가세 겐지로",   14 },
    { "타카이 스즈미",   15 },
    { "이토 마사요시",   16 },
    { "후쿠하라 쇼조",   17 },
    { "나스 다이하치로", 18 },
    { "나스 코지로",     19 },
    { "검은 실루엣",     20 },
    { "남자 아이1",      21 },
    { "여자 아이",       22 },
    { "남자 아이2",      23 },
    { "",                -1 }, // 끝 표시용
};
/*
// 캐릭터 이름 매칭 테이블 (키즈아토)
const CharactorInfo g_charactorList[] = {
    { "",           0 }, // 기본값
    { "치즈루",     1 },
    { "아즈사",     2 },
    { "카에데",     3 },
    { "하츠네",     4 },
    { "히요시",     5 },
    { "유미코",     6 },
    { "쿄코",       7 },
    { "야나가와",   8 },
    { "나가세",     9 },
    { "타카유키",  10 },
    { "릿카",      16 },
    { "후카",      17 },
    { "그외 여성", 49 },
    { "그외 남성", 50 },
    { "그외 여성2",59 },
    { "그외 남성2",60 },
    { "",          -1 }, // 끝 표시용
};
*/

// 스크립트 오퍼레이터 이름 + 코드 매칭 테이블
const OperatorInfo g_operatorList[] = {
    // 스크립트 오퍼레이터 (0x00부터 시작)
    { "Start",   ScriptOperator::Start },
    { "End",     ScriptOperator::End },

    { "MovR",    ScriptOperator::MovR },
    { "MovV",    ScriptOperator::MovV },
    { "Swap",    ScriptOperator::Swap },
    { "Rand",    ScriptOperator::Rand },

    { "IfR",     ScriptOperator::IfR },
    { "IfV",     ScriptOperator::IfV },
    { "IfElseR", ScriptOperator::IfElseR },
    { "IfElseV", ScriptOperator::IfElseV },

    { "Loop",    ScriptOperator::Loop },
    { "Goto",    ScriptOperator::Goto },

    { "Inc",     ScriptOperator::Inc },
    { "Dec",     ScriptOperator::Dec },
    { "Not",     ScriptOperator::Not },
    { "Neg",     ScriptOperator::Neg },

    { "AddR",    ScriptOperator::AddR },
    { "AddV",    ScriptOperator::AddV },
    { "SubR",    ScriptOperator::SubR },
    { "SubV",    ScriptOperator::SubV },
    { "MulR",    ScriptOperator::MulR },
    { "MulV",    ScriptOperator::MulV },
    { "DivR",    ScriptOperator::DivR },
    { "DivV",    ScriptOperator::DivV },
    { "ModR",    ScriptOperator::ModR },
    { "ModV",    ScriptOperator::ModV },
    { "AndR",    ScriptOperator::AndR },
    { "AndV",    ScriptOperator::AndV },
    { "OrR",     ScriptOperator::OrR },
    { "OrV",     ScriptOperator::OrV },
    { "XorR",    ScriptOperator::XorR },
    { "XorV",    ScriptOperator::XorV },

    { "Calc",    ScriptOperator::Calc },

    { "Pusha",   ScriptOperator::Pusha },
    { "Popa",    ScriptOperator::Popa },

    { "Call",    ScriptOperator::Call },
    { "Ret",     ScriptOperator::Ret },

    { "Wait",    ScriptOperator::Wait },
    { "TWait",   ScriptOperator::TWait },
    { "Run",     ScriptOperator::Run },

    { "SLoad",   ScriptOperator::SLoad },
    { "TStart",  ScriptOperator::TStart },

    // 예약된 영역 (0x2A - 0x3F)
    { "", static_cast<ScriptOperator>(0x002A)},
    { "", static_cast<ScriptOperator>(0x002B) },
    { "", static_cast<ScriptOperator>(0x002C) },
    { "", static_cast<ScriptOperator>(0x002D) },
    { "", static_cast<ScriptOperator>(0x002E) },
    { "", static_cast<ScriptOperator>(0x002F) },

    { "", static_cast<ScriptOperator>(0x0030) },
    { "", static_cast<ScriptOperator>(0x0031) },
    { "", static_cast<ScriptOperator>(0x0032) },
    { "", static_cast<ScriptOperator>(0x0033) },
    { "", static_cast<ScriptOperator>(0x0034) },
    { "", static_cast<ScriptOperator>(0x0035) },
    { "", static_cast<ScriptOperator>(0x0036) },
    { "", static_cast<ScriptOperator>(0x0037) },
    { "", static_cast<ScriptOperator>(0x0038) },
    { "", static_cast<ScriptOperator>(0x0039) },
    { "", static_cast<ScriptOperator>(0x003A) },
    { "", static_cast<ScriptOperator>(0x003B) },
    { "", static_cast<ScriptOperator>(0x003C) },
    { "", static_cast<ScriptOperator>(0x003D) },
    { "", static_cast<ScriptOperator>(0x003E) },
    { "", static_cast<ScriptOperator>(0x003F) },

    // 이벤트 제어 오퍼레이터 (0x40부터 시작)
    { "B",                    ScriptOperator::B },
    { "BT",                   ScriptOperator::BT },
    { "BC",                   ScriptOperator::BC },
    { "BCT",                  ScriptOperator::BCT },
    { "BD",                   ScriptOperator::BD },
    { "BR",                   ScriptOperator::BR },
    { "BF",                   ScriptOperator::BF },
    { "V",                    ScriptOperator::V },
    { "H",                    ScriptOperator::H },

    { "S",                    ScriptOperator::S },
    { "Z",                    ScriptOperator::Z },
    { "FI",                   ScriptOperator::FI },
    { "FIF",                  ScriptOperator::FIF },
    { "FO",                   ScriptOperator::FO },
    { "FOF",                  ScriptOperator::FOF },
    { "FB",                   ScriptOperator::FB },
    { "PFI",                  ScriptOperator::PFI },
    { "PFO",                  ScriptOperator::PFO },
    { "PWI",                  ScriptOperator::PWI },
    { "PWO",                  ScriptOperator::PWO },

    { "Q",                    ScriptOperator::Q },
    { "F",                    ScriptOperator::F },

    { "C",                    ScriptOperator::C },
    { "CR",                   ScriptOperator::CR },
    { "CP",                   ScriptOperator::CP },
    { "CL",                   ScriptOperator::CL },
    { "CY",                   ScriptOperator::CY },
    { "CB",                   ScriptOperator::CB },
    { "CA",                   ScriptOperator::CA },
    { "CW",                   ScriptOperator::CW },
    { "CRW",                  ScriptOperator::CRW },

    { "W",                    ScriptOperator::W },
    { "WR",                   ScriptOperator::WR },

    { "KW",                   ScriptOperator::KW },
    { "K",                    ScriptOperator::K },

    { "M",                    ScriptOperator::M },
    { "MS",                   ScriptOperator::MS },
    { "MP",                   ScriptOperator::MP },
    { "MV",                   ScriptOperator::MV },
    { "MW",                   ScriptOperator::MW },

    { "SE",                   ScriptOperator::SE },
    { "SEP",                  ScriptOperator::SEP },
    //{ "SE2",                  ScriptOperator::SE2 },
    //{ "SEP2",                 ScriptOperator::SEP2 },
    { "SES",                  ScriptOperator::SES },
    { "SEW",                  ScriptOperator::SEW },
    { "SEV",                  ScriptOperator::SEV },
    { "SEVW",                 ScriptOperator::SEVW },

    //{ "SetVolumeVoiceChar",   ScriptOperator::SetVolumeVoiceChar },
    //{ "SetVolumeVoiceScript", ScriptOperator::SetVolumeVoiceScript },
    //{ "SV",                   ScriptOperator::SV },
    //{ "SVEX",                 ScriptOperator::SVEX },

    //{ "VV",                   ScriptOperator::VV },
    //{ "VA",                   ScriptOperator::VA },
    //{ "VB",                   ScriptOperator::VB },
    //{ "VC",                   ScriptOperator::VC },
    //{ "VX",                   ScriptOperator::VX },
    //{ "VW",                   ScriptOperator::VW },
    //{ "VS",                   ScriptOperator::VS },
    //{ "VI",                   ScriptOperator::VI },

    { "R",                    ScriptOperator::R },
    { "RC",                   ScriptOperator::RC },
    { "RR",                   ScriptOperator::RR },

    { "LF",                   ScriptOperator::LF },

    { "WE",                   ScriptOperator::WE },
    { "WER",                  ScriptOperator::WER },

    // 저레벨 제어 오퍼레이터
    { "SetFlag",              ScriptOperator::SetFlag },
    { "GetFlag",              ScriptOperator::GetFlag },

    { "SetGameFlag",          ScriptOperator::SetGameFlag },
    { "GetGameFlag",          ScriptOperator::GetGameFlag },

    { "LoadScript",           ScriptOperator::LoadScript },
    { "GameEnd",              ScriptOperator::GameEnd },
    { "CallFunc",             ScriptOperator::CallFunc },

    //{ "ViewClock",            ScriptOperator::ViewClock },

    //{ "SetWeatherMode",       ScriptOperator::SetWeatherMode },

    { "SetTimeMode",          ScriptOperator::SetTimeMode },
    { "SetTimeBack",          ScriptOperator::SetTimeBack },
    { "SetTimeChar",          ScriptOperator::SetTimeChar },

    { "SetMatrix",            ScriptOperator::SetMatrix },
    { "ShowChipBack",         ScriptOperator::ShowChipBack },
    { "PoseChipBack",         ScriptOperator::PoseChipBack },
    { "PlayChipBack",         ScriptOperator::PlayChipBack },

    { "SetChipBack",          ScriptOperator::SetChipBack },
    { "SetChipParts",         ScriptOperator::SetChipParts },
    { "SetChipScroll",        ScriptOperator::SetChipScroll },
    { "SetChipScroll2",       ScriptOperator::SetChipScroll2 },
    { "SetChipScrollSpeed",   ScriptOperator::SetChipScrollSpeed },
    { "WaitChipScroll",       ScriptOperator::WaitChipScroll },
    { "WaitChipScrollSpeed",  ScriptOperator::WaitChipScrollSpeed },

    { "SetChipCharCash",      ScriptOperator::SetChipCharCash },
    { "ResetChipCharCash",    ScriptOperator::ResetChipCharCash },

    { "SetChipChar",          ScriptOperator::SetChipChar },
    { "SetChipCharAni",       ScriptOperator::SetChipCharAni },
    { "ResetChipChar",        ScriptOperator::ResetChipChar },
    { "SetChipCharParam",     ScriptOperator::SetChipCharParam },
    { "SetChipCharBright",    ScriptOperator::SetChipCharBright },
    { "SetChipCharMove",      ScriptOperator::SetChipCharMove },
    { "SetChipCharMove2",     ScriptOperator::SetChipCharMove2 },
    { "SetChipCharMoveSpeed", ScriptOperator::SetChipCharMoveSpeed },
    { "GetChipCharMove",      ScriptOperator::GetChipCharMove },
    { "GetChipCharMoveSpeed", ScriptOperator::GetChipCharMoveSpeed },
    { "CopyChipCharPos",      ScriptOperator::CopyChipCharPos },
    { "SetChipCharRev",       ScriptOperator::SetChipCharRev },
    { "ThroughChipCharAni",   ScriptOperator::ThroughChipCharAni },

    { "WaitChipCharRepeat",   ScriptOperator::WaitChipCharRepeat },
    { "WaitChipCharAni",      ScriptOperator::WaitChipCharAni },
    { "WaitChipCharAniLoop",  ScriptOperator::WaitChipCharAniLoop },
    { "WaitChipCharMove",     ScriptOperator::WaitChipCharMove },

    { "GetBack",              ScriptOperator::GetBack },
    { "SetBack",              ScriptOperator::SetBack },
    { "SetBack2",             ScriptOperator::SetBack2 },
    { "SetCg",                ScriptOperator::SetCg },
    { "SetCg2",               ScriptOperator::SetCg2 },
    { "SetBackPos",           ScriptOperator::SetBackPos },
    { "SetBackPosZ",          ScriptOperator::SetBackPosZ },
    { "SetBackScroll",        ScriptOperator::SetBackScroll },
    { "SetBackScrollZ",       ScriptOperator::SetBackScrollZ },
    { "SetBackCScope",        ScriptOperator::SetBackCScope },
    { "WaitBackCScope",       ScriptOperator::WaitBackCScope },
    { "LockBackCScope",       ScriptOperator::LockBackCScope },
    { "SetBackFadeIn",        ScriptOperator::SetBackFadeIn },
    { "SetBackFadeOut",       ScriptOperator::SetBackFadeOut },
    { "SetBackFadeRGB",       ScriptOperator::SetBackFadeRGB },

    { "SetShake",             ScriptOperator::SetShake },
    { "StopShake",            ScriptOperator::StopShake },

    { "SetFlash",             ScriptOperator::SetFlash },

    { "SetChar",              ScriptOperator::SetChar },
    { "ResetChar",            ScriptOperator::ResetChar },
    { "SetCharPose",          ScriptOperator::SetCharPose },
    { "SetCharLocate",        ScriptOperator::SetCharLocate },
    { "SetCharLayer",         ScriptOperator::SetCharLayer },
    { "WaitChar",             ScriptOperator::WaitChar },

    { "SetBlock",             ScriptOperator::SetBlock },
    { "SetWindow",            ScriptOperator::SetWindow },
    { "ResetWindow",          ScriptOperator::ResetWindow },
    { "SetMessage",           ScriptOperator::SetMessage },
    { "SetMessage2",          ScriptOperator::SetMessage2 },
    { "SetMessageEx",         ScriptOperator::SetMessageEx },
    { "SetChipMessage",       ScriptOperator::SetChipMessage },

    { "AddMessage",           ScriptOperator::AddMessage },
    { "AddMessage2",          ScriptOperator::AddMessage2 },
    { "SetMessageWait",       ScriptOperator::SetMessageWait },
    { "ResetMessage",         ScriptOperator::ResetMessage },

    { "WaitKey",              ScriptOperator::WaitKey },

    { "SetSelectMes",         ScriptOperator::SetSelectMes },
    { "SetSelectMesEx",       ScriptOperator::SetSelectMesEx },
    { "SetSelect",            ScriptOperator::SetSelect },
    { "SetSelectEx",          ScriptOperator::SetSelectEx },

    { "PlayBgm",              ScriptOperator::PlayBgm },
    { "PlayBgmEx",            ScriptOperator::PlayBgmEx },
    { "StopBgm",              ScriptOperator::StopBgm },
    { "StopBgmEx",            ScriptOperator::StopBgmEx },
    { "SetVolumeBgm",         ScriptOperator::SetVolumeBgm },
    { "SetVolumeBgmEx",       ScriptOperator::SetVolumeBgmEx },

    { "PlaySe",               ScriptOperator::PlaySe },
    { "PlaySeEx",             ScriptOperator::PlaySeEx },
    { "StopSeEx",             ScriptOperator::StopSeEx },
    { "SetVolumeSe",          ScriptOperator::SetVolumeSe },

    //{ "SetMoveEffect",        ScriptOperator::SetMoveEffect },
    //{ "ResetMoveEffect",      ScriptOperator::ResetMoveEffect },

    { "SetWeather",           ScriptOperator::SetWeather },
    { "ChangeWeather",        ScriptOperator::ChangeWeather },
    { "ResetWeather",         ScriptOperator::ResetWeather },
    { "SetLensFrea",          ScriptOperator::SetLensFrea },
    { "SetWavEffect",         ScriptOperator::SetWavEffect },
    { "ResetWavEffect",       ScriptOperator::ResetWavEffect },

    //{ "SetWarp",              ScriptOperator::SetWarp },
    //{ "ResetWarp",            ScriptOperator::ResetWarp },

    { "WaitFrame",            ScriptOperator::WaitFrame },

    { "SetBmp",               ScriptOperator::SetBmp },
    //{ "SetBmpEx",             ScriptOperator::SetBmpEx },
    { "SetBmp4Bmp",           ScriptOperator::SetBmp4Bmp },
    { "SetBmpPrim",           ScriptOperator::SetBmpPrim },
    { "ResetBmp",             ScriptOperator::ResetBmp },
    { "ResetBmpAll",          ScriptOperator::ResetBmpAll },
    { "SetBmpAnime",          ScriptOperator::SetBmpAnime },
    { "ResetBmpAnime",        ScriptOperator::ResetBmpAnime },
    { "WaitBmpAnime",         ScriptOperator::WaitBmpAnime },
    { "SetTitle",             ScriptOperator::SetTitle },
    { "SetEnding",            ScriptOperator::SetEnding },
    //{ "SetOpening",           ScriptOperator::SetOpening },

    { "SetAvi",               ScriptOperator::SetAvi },
    { "ResetAvi",             ScriptOperator::ResetAvi },
    { "WaitAvi",              ScriptOperator::WaitAvi },
    { "SetAviFull",           ScriptOperator::SetAviFull },
    { "WaitAviFull",          ScriptOperator::WaitAviFull },

    { "SetBmpDisp",           ScriptOperator::SetBmpDisp },
    { "SetBmpLayer",          ScriptOperator::SetBmpLayer },
    { "SetBmpParam",          ScriptOperator::SetBmpParam },
    { "SetBmpRevParam",       ScriptOperator::SetBmpRevParam },
    { "SetBmpBright",         ScriptOperator::SetBmpBright },
    { "SetBmpMove",           ScriptOperator::SetBmpMove },
    { "SetBmpPos",            ScriptOperator::SetBmpPos },
    { "SetBmpZoom",           ScriptOperator::SetBmpZoom },
    { "SetBmpZoom2",          ScriptOperator::SetBmpZoom2 },

    { "SetDemoFlag",          ScriptOperator::SetDemoFlag },
    { "SetSceneNo",           ScriptOperator::SetSceneNo },
    { "SetEndingNo",          ScriptOperator::SetEndingNo },
    { "SetReplayNo",          ScriptOperator::SetReplayNo },

    { "SetSoundEvent",        ScriptOperator::SetSoundEvent },
    { "SetSoundEventVolume",  ScriptOperator::SetSoundEventVolume },

    { "SetPotaPota",          ScriptOperator::SetPotaPota },

    { "GetTime",              ScriptOperator::GetTime },
    { "WaitTime",             ScriptOperator::WaitTime },

    { "SetTextFormat",        ScriptOperator::SetTextFormat },
    { "SetTextSync",          ScriptOperator::SetTextSync },
    { "SetText",              ScriptOperator::SetText },
    { "SetTextEx",            ScriptOperator::SetTextEx },
    { "ResetText",            ScriptOperator::ResetText },
    { "WaitText",             ScriptOperator::WaitText },
    { "ResetTextAll",         ScriptOperator::ResetTextAll },

    { "SetDemoFadeFlag",      ScriptOperator::SetDemoFadeFlag },

    { "Mov2",                 ScriptOperator::Mov2 },
    //{ "Sin",                  ScriptOperator::Sin },
    //{ "Cos",                  ScriptOperator::Cos },
    //{ "Abs",                  ScriptOperator::Abs },

    { "SetCutCut",            ScriptOperator::SetCutCut },

    { "SetNoise",             ScriptOperator::SetNoise },
    { "T",                    ScriptOperator::T },
    { "SetUsoErr",            ScriptOperator::SetUsoErr },
    { "LoadScriptNum",        ScriptOperator::LoadScriptNum },

    { "SetRipple",            ScriptOperator::SetRipple },
    { "SetRippleSet",         ScriptOperator::SetRippleSet },
    { "WaitRipple",           ScriptOperator::WaitRipple },
    { "SetRippleLost",        ScriptOperator::SetRippleLost },
    { "MLW",                  ScriptOperator::MLW },

    { "VT",                   ScriptOperator::VT },
    { "HT",                   ScriptOperator::HT },

    //{ "SetMapEvent",          ScriptOperator::SetMapEvent },
    //{ "VIB",                  ScriptOperator::VIB },

    //{ "ViewCalender",         ScriptOperator::ViewCalender },

    //{ "SetSakura",            ScriptOperator::SetSakura },
    //{ "StopSakura",           ScriptOperator::StopSakura },

    //{ "SkipDate",             ScriptOperator::SkipDate },

    { "SetMovie",             ScriptOperator::SetMovie },
    //{ "DebugBox",             ScriptOperator::DebugBox },

    //{ "VHFlag",               ScriptOperator::VHFlag },

    //{ "GetSystemTime",        ScriptOperator::GetSystemTime },

    //{ "SetName",              ScriptOperator::SetName },    // 키즈아토 추가 명령
    //{ "SetMes",               ScriptOperator::SetMes },     // 키즈아토 추가 명령
    //{ "WN",                   ScriptOperator::WN },         // 키즈아토 추가 명령
    //{ "WNS",                  ScriptOperator::WNS },        // 키즈아토 추가 명령

    { "",                     ScriptOperator::OpMax },      // 끝 표시용
};


#define NOT     ScriptArgumentType::NotUsed        //  0 | 추가 인자 없음
#define ASC     ScriptArgumentType::AsciiChar      //  1 | 1바이트 unsigned Char 값
#define NUM     ScriptArgumentType::Number         //  2 | 1바이트 Mode + (Mode1: 4바이트 Int 또는 1바이트 unsigned Char 길이 + 문자열)
#define STR     ScriptArgumentType::String         //  3 | 1바이트 unsigned Char 길이 + 문자열
#define STR2    ScriptArgumentType::String2        //  4 | 2바이트 unsigned Short 길이 + 문자열
#define REG     ScriptArgumentType::Register       //  5 | 1바이트 unsigned Char 값 (레지스터 번호)
#define CMP     ScriptArgumentType::Compare        //  6 | 1바이트(레지스터1) + 1바이트(레지스터2) + 1바이트 Mode + (Mode1: 4바이트 Int 또는 1바이트 unsigned Char 길이 + 문자열)
#define ADD     ScriptArgumentType::AddValue       //  7 | 1바이트 unsigned Char 값 (플래그 +값)
#define CNT     ScriptArgumentType::Count          //  8 | 2바이트 unsigned Short 값 (카운터)
#define VCNT    ScriptArgumentType::VariableCount  //  9 | 2바이트 unsigned Short 값 (가변 카운터)
#define CHR     ScriptArgumentType::CharValue      // 16 | 1바이트 unsigned Char 값
#define SHRT    ScriptArgumentType::ShortValue     // 17 | 2바이트 unsigned Short 값
#define LONG    ScriptArgumentType::LongValue      // 18 | 4바이트 Int 값

// 오퍼레이터별 인자 타입 포맷 정의
const ScriptOperatorFormat g_operatorFormats[] = {
    // 스크립트 오퍼레이터 (0x00부터 시작)
    { { NOT }, 0 },                     // Start
    { { NOT }, 0 },                     // End

    { { REG, CHR, NOT }, 2 },           // MovR
    { { REG,LONG, NOT }, 2 },           // MovV
    { { REG, REG, NOT }, 2 },           // Swap
    { { REG, NOT }, 0 },                // Rand

    { { REG, CHR, CHR,LONG, NOT }, 4 }, // IfR
    { { REG, CHR,LONG,LONG, NOT }, 4 }, // IfV
    { { REG, CHR, CHR,LONG, NOT }, 4 }, // IfElseR
    { { REG, CHR,LONG,LONG, NOT }, 4 }, // IfElseV

    { { REG,LONG, NOT }, 2 },           // Loop
    { {LONG, NOT }, 1 },                // Goto

    { { REG, NOT }, 1 },                // Inc
    { { REG, NOT }, 1 },                // Dec
    { { REG, NOT }, 1 },                // Not
    { { REG, NOT }, 1 },                // Neg

    { { REG, CHR, NOT }, 2 },           // AddR
    { { REG,LONG, NOT }, 2 },           // AddV
    { { REG, CHR, NOT }, 2 },           // SubR
    { { REG,LONG, NOT }, 2 },           // SubV
    { { REG, CHR, NOT }, 2 },           // MulR
    { { REG,LONG, NOT }, 2 },           // MulV
    { { REG, CHR, NOT }, 2 },           // DivR
    { { REG,LONG, NOT }, 2 },           // DivV
    { { REG, CHR, NOT }, 2 },           // ModR
    { { REG,LONG, NOT }, 2 },           // ModV
    { { REG, CHR, NOT }, 2 },           // AndR
    { { REG,LONG, NOT }, 2 },           // AndV
    { { REG, CHR, NOT }, 2 },           // OrR
    { { REG,LONG, NOT }, 2 },           // OrV
    { { REG, CHR, NOT }, 2 },           // XorR
    { { REG,LONG, NOT }, 2 },           // XorV

    { { NOT }, 0 },                     // Calc

    { { NOT }, 0 },                     // Pusha
    { { NOT }, 0 },                     // Popa

    { { CHR,LONG, NOT }, 2 },           // Call
    { { NOT }, 0 },                     // Ret

    { {SHRT, NOT }, 1 },                // Wait
    { {SHRT, NOT }, 1 },                // TWait
    { { NOT }, 0 },                     // Run

    { { NOT }, 0 },                     // SLoad
    { { NOT }, 0 },                     // TStart


    // 예약된 영역 (0x2A - 0x3F)
    { { NOT }, 0 }, // 0x002A
    { { NOT }, 0 }, // 0x002B
    { { NOT }, 0 }, // 0x002C
    { { NOT }, 0 }, // 0x002D
    { { NOT }, 0 }, // 0x002E
    { { NOT }, 0 }, // 0x002F
    { { NOT }, 0 }, // 0x0030
    { { NOT }, 0 }, // 0x0031
    { { NOT }, 0 }, // 0x0032
    { { NOT }, 0 }, // 0x0033
    { { NOT }, 0 }, // 0x0034
    { { NOT }, 0 }, // 0x0035
    { { NOT }, 0 }, // 0x0036
    { { NOT }, 0 }, // 0x0037
    { { NOT }, 0 }, // 0x0038
    { { NOT }, 0 }, // 0x0039
    { { NOT }, 0 }, // 0x003A
    { { NOT }, 0 }, // 0x003B
    { { NOT }, 0 }, // 0x003C
    { { NOT }, 0 }, // 0x003D
    { { NOT }, 0 }, // 0x003E
    { { NOT }, 0 }, // 0x003F


    // 이벤트 제어 오퍼레이터 (0x40부터 시작)
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 7 },                                    // B
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 7 },                                    // BT
    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // BC
    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // BCT
    { { NOT }, 0 },                                                                       // BD
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // BR
    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // BF
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 7 },                                    // V
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 7 },                                    // H

    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // S
    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // Z
    { { NOT }, 0 },                                                                       // FI
    { { NUM, NOT }, 1 },                                                                  // FIF
    { { NOT }, 0 },                                                                       // FO
    { { NUM, NOT }, 1 },                                                                  // FOF
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // FB
    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // PFI
    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // PFO
    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // PWI
    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // PWO

    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // Q
    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // F

    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 8 },                               // C
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // CR
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // CP
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // CL
    { { NUM, NUM, NOT }, 2 },                                                             // CY
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // CB
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // CA
    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // CW
    { { NUM, NUM, NOT }, 2 },                                                             // CRW

    { { NUM, NOT }, 1 },                                                                  // W
    { { NUM, NOT }, 1 },                                                                  // WR

    { { NUM, NOT }, 1 },                                                                  // KW
    { { NOT }, 0 },                                                                       // K

    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // M
    { { NUM, NOT }, 1 },                                                                  // MS
    { { NUM, NOT }, 1 },                                                                  // MP
    { { NUM, NUM, NOT }, 2 },                                                             // MV
    { { NOT }, 0 },                                                                       // MW

    { { NUM, NUM, NOT }, 2 },                                                             // SE
    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // SEP
    //{ { NUM, NUM, NOT }, 2 },                                                             // SE2
    //{ { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // SEP2
    { { NUM, NUM, NOT }, 2 },                                                             // SES
    { { NUM, NOT }, 1 },                                                                  // SEW
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SEV
    { { NUM, NOT }, 1 },                                                                  // SEVW

    //{ { NUM, NUM, NOT }, 2 },                                                             // SetVolumeVoiceChar
    //{ { NUM, NOT }, 1 },                                                                  // SetVolumeVoiceScript

    //{ { NUM, NUM, NUM, VCNT, NUM, NOT }, 5 },                                             // SV
    //{ { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // SVEX

    //{ { NUM, NUM, NUM, VCNT, NUM, NOT }, 5 },                                             // VV
    //{ { NUM, NUM, NUM, VCNT, NUM, NOT }, 5 },                                             // VA
    //{ { NUM, NUM, NUM, VCNT, NUM, NOT }, 5 },                                             // VB
    //{ { NUM, NUM, NUM, VCNT, NUM, NOT }, 5 },                                             // VC

    //{ { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // VX
    //{ { NUM, NOT }, 1 },                                                                  // VW
    //{ { NUM, NUM, NOT }, 2 },                                                             // VS
    //{ { VCNT, NUM, NUM, NOT }, 3 },                                                       // VI

    { { NUM, NUM, NUM, NOT }, 3 },                                                        // R
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // RC
    { { NOT }, 0 },                                                                       // RR

    { { NUM, NUM, NUM, NOT }, 3 },                                                        // LF

    { { NUM, NUM, NUM, NOT }, 3 },                                                        // WE
    { { NOT }, 0 },                                                                       // WER

    { { NUM, NUM, NOT }, 2 },                                                             // SetFlag
    { { NUM, REG, NOT }, 2 },                                                             // GetFlag

    { { NUM, NUM, NOT }, 2 },                                                             // SetGameFlag
    { { NUM, REG, NOT }, 2 },                                                             // GetGameFlag

    { { STR, NOT }, 1 },                                                                  // LoadScript
    { { NUM, NOT }, 1 },                                                                  // GameEnd
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, }, 15},// CallFunc

    //{ { NUM, NUM, NUM, NOT }, 3 },                                                        // ViewClock

    //{ { NUM, NOT }, 1 },                                                                  // SetWeatherMode

    { { NUM, NUM, NOT }, 2 },                                                             // SetTimeMode
    { { NUM, NUM, NOT }, 2 },                                                             // SetTimeBack
    { { NUM, NUM, NOT }, 2 },                                                             // SetTimeChar


    // 배경 처리 오퍼레이터
    { { NUM, NOT }, 1 },                                                                  // SetMatrix
    { { NUM, NUM, NOT }, 2 },                                                             // ShowChipBack
    { { NOT }, 0 },                                                                       // PoseChipBack
    { { NOT }, 0 },                                                                       // PlayChipBack

    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // SetChipBack
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 13},      // SetChipParts
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetChipScroll
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetChipScroll2

    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetChipScrollSpeed
    { { NOT }, 0 },                                                                       // WaitChipScroll
    { { NOT }, 0 },                                                                       // WaitChipScrollSpeed

    { { NUM, NUM, NOT }, 2 },                                                             // SetChipCharCash
    { { NOT }, 0 },                                                                       // ResetChipCharCash

    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // SetChipChar
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetChipCharAni
    { { NUM, NOT }, 1 },                                                                  // ResetChipChar
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetChipCharParam
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetChipCharBright
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetChipCharMove
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetChipCharMove2
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetChipCharMoveSpeed
    { { NUM, REG, REG, NOT }, 3 },                                                        // GetChipCharMove
    { { NUM, REG, REG, NOT }, 3 },                                                        // GetChipCharMoveSpeed

    { { NUM, NUM, NOT }, 2 },                                                             // CopyChipCharPos
    { { NUM, NUM, NOT }, 2 },                                                             // SetChipCharRev

    { { NUM, NOT }, 1 },                                                                  // ThroughChipCharAni
    { { NUM, NUM, NOT }, 2 },                                                             // WaitChipCharRepeat
    { { NUM, NUM, NOT }, 2 },                                                             // WaitChipCharAni
    { { NUM, NUM, NOT }, 2 },                                                             // WaitChipCharAniLoop
    { { NUM, NOT }, 1 },                                                                  // WaitChipCharMove

    { { NOT }, 0 },                                                                       // GetBack
    { { NUM, NUM, NOT }, 2 },                                                             // SetBack
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 7 },                                    // SetBack2
    { { NUM, NUM, NOT }, 2 },                                                             // SetCg
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetCg2
    { { NUM, NUM, NOT }, 2 },                                                             // SetBackPos
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetBackPosZ
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetBackScroll
    { { NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 6 },                                         // SetBackScrollZ
    { { NUM, NOT }, 1 },                                                                  // SetBackCScope
    { { NOT }, 0 },                                                                       // WaitBackCScope
    { { NUM, NOT }, 1 },                                                                  // LockBackCScope

    { { NUM, NUM, NOT }, 2 },                                                             // SetBackFadeIn
    { { NUM, NUM, NOT }, 2 },                                                             // SetBackFadeOut
    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // SetBackFadeRGB
    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // SetShake
    { { NOT }, 0 },                                                                       // StopShake
    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // SetFlash


    // 캐릭터 제어 오퍼레이터
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 8 },                               // SetChar
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // ResetChar
    { { NUM, NUM, NOT }, 2 },                                                             // SetCharPose
    { { NUM, NUM, NOT }, 2 },                                                             // SetCharLocate
    { { NUM, NUM, NOT }, 2 },                                                             // SetCharLayer
    { { NUM, NOT }, 1 },                                                                  // WaitChar


    // 윈도우 제어 오퍼레이터
    { { NUM, NOT }, 1 },                                                                  // SetBlock
    { { NUM, NOT }, 1 },                                                                  // SetWindow
    { { NUM, NOT }, 1 },                                                                  // ResetWindow
    { { NUM, STR, CNT, NOT }, 3 },                                                        // SetMessage
    { {STR2, ADD, CNT, NOT }, 3 },                                                        // SetMessage2
    { { NUM, NUM, STR, NUM, CNT, NOT }, 5 },                                              // SetMessageEx
    { { STR, CNT, NOT }, 2 },                                                             // SetChipMessage

    { { NUM, STR, NOT }, 2 },                                                             // AddMessage
    { {STR2, ADD, NOT }, 2 },                                                             // AddMessage2
    { { NUM, NOT }, 1 },                                                                  // SetMessageWait
    { { NOT }, 0 },                                                                       // ResetMessage

    { { NOT }, 0 },                                                                       // WaitKey

    { { STR, NUM, NUM, NOT }, 3 },                                                        // SetSelectMes
    { { STR, STR, NUM, NUM, NOT }, 4 },                                                   // SetSelectMesEx
    { { REG, NOT }, 1 },                                                                  // SetSelect
    { { NOT }, 0 },                                                                       // SetSelectEx


    // 오디오 재생 오퍼레이터
    { { NUM, NUM, NOT }, 2 },                                                             // PlayBgm
    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // PlayBgmEx
    { { NUM, NOT }, 1 },                                                                  // StopBgm
    { { NUM, NUM, NOT }, 2 },                                                             // StopBgmEx
    { { NUM, NUM, NOT }, 2 },                                                             // SetVolumeBgm
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetVolumeBgmEx
    { { NUM, NOT }, 1 },                                                                  // PlaySe
    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // PlaySeEx
    { { NUM, NUM, NOT }, 2 },                                                             // StopSeEx
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetVolumeSe


    // 기타 오퍼레이터
    //{ { NOT }, 1 },                                                                       // SetMoveEffect
    //{ { NOT }, 1 },                                                                       // ResetMoveEffect
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetWeather
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // ChangeWeather
    { { NOT }, 0 },                                                                       // ResetWeather
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetLensFrea
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetWavEffect
    { { NOT }, 0 },                                                                       // ResetWavEffect

    //{ { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 8 },                               // SetWarp
    //{ { NUM, NOT }, 1 },                                                                  // ResetWarp

    { { NUM, NOT }, 1 },                                                                  // WaitFrame

    { { NUM, NUM, STR, NUM, NUM, STR, NUM, NUM, NOT }, 8 },                               // SetBmp
    //{ { NUM, NUM, STR, NUM, NUM, NUM, STR, NOT }, 7 },                                    // SetBmpEx
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetBmp4Bmp
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetBmpPrim
    { { NUM, NOT }, 1 },                                                                  // ResetBmp
    { { NOT }, 0 },                                                                       // ResetBmpAll
    { { NUM, STR, NUM, NUM, NUM, NOT }, 5 },                                              // SetBmpAnime
    { { NUM, NOT }, 1 },                                                                  // ResetBmpAnime
    { { NUM, NOT }, 1 },                                                                  // WaitBmpAnime

    { { NOT }, 0 },                                                                       // SetTitle
    { { NUM, NUM, NOT }, 2 },                                                             // SetEnding

    //{ { NOT }, 0 },                                                                       // SetOpening

    { { NUM, NUM, NUM, NUM, STR, NOT }, 5 },                                              // SetAvi
    { { NOT }, 0 },                                                                       // ResetAvi
    { { NOT }, 0 },                                                                       // WaitAvi
    { { NOT }, 0 },                                                                       // SetAviFull
    { { NOT }, 0 },                                                                       // WaitAviFull

    { { NUM, NUM, NOT }, 2 },                                                             // SetBmpDisp
    { { NUM, NUM, NOT }, 2 },                                                             // SetBmpLayer
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetBmpParam
    { { NUM, NUM, NOT }, 2 },                                                             // SetBmpRevParam
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetBmpBright
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetBmpMove
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 7 },                                    // SetBmpPos
    { { NUM, NUM, NUM, NUM, NUM, NOT }, 5 },                                              // SetBmpZoom
    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetBmpZoom2
    { { NUM, NUM, NOT }, 2 },                                                             // SetDemoFlag
    { { NUM, NOT }, 1 },                                                                  // SetSceneNo
    { { NUM, NOT }, 1 },                                                                  // SetEndingNo
    { { NUM, NOT }, 1 },                                                                  // SetReplayNo

    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 7 },                                    // SetSoundEvent

    { { NUM, NUM, NUM, NUM, NOT }, 4 },                                                   // SetSoundEventVolume

    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetPotaPota

    { { REG, NOT }, 1 },                                                                  // GetTime
    { { NUM, NOT }, 1 },                                                                  // WaitTime

    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 11},                // SetTextFormat
    { { NUM, NUM, NOT }, 2 },                                                             // SetTextSync
    { { NUM, STR, NOT }, 2 },                                                             // SetText
    { { STR, NOT }, 1 },                                                                  // SetTextEx
    { { NUM, NOT }, 1 },                                                                  // ResetText
    { { NUM, NOT }, 1 },                                                                  // WaitText
    { { NOT }, 0 },                                                                       // ResetTextAll

    { { NUM, NOT }, 1 },                                                                  // SetDemoFadeFlag

    { { REG, NUM, NOT }, 2 },                                                             // Mov2
    //{ { REG, NUM, NUM, NOT }, 3 },                                                        // Sin
    //{ { REG, NUM, NUM, NOT }, 3 },                                                        // Cos
    //{ { REG, NUM, NOT }, 2 },                                                             // Abs
    { { NUM, NOT }, 1 },                                                                  // SetCutCut
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetNoise
    { { NUM, NUM, NOT }, 2 },                                                             // T
    { { NOT }, 0 },                                                                       // SetUsoErr
    { { NUM, NOT }, 1 },                                                                  // LoadScriptNum
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetRipple
    { { NUM, NUM, NUM, NOT }, 3 },                                                        // SetRippleSet
    { { NOT }, 0 },                                                                       // WaitRipple
    { { NOT }, 0 },                                                                       // SetRippleLost
    { { NOT }, 0 },                                                                       // MLW
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 7 },                                    // VT
    { { NUM, NUM, NUM, NUM, NUM, NUM, NUM, NOT }, 7 },                                    // HT

    //{ { NUM, NUM, NUM, STR, NOT }, 4 },                                                   // SetMapEvent
    //{ { NUM, NUM, NUM, NOT }, 3 },                                                        // VIB
    //{ { NUM, NUM, NOT }, 2 },                                                             // ViewCalender

    //{ { NUM, NUM, NUM, NOT }, 3 },                                                        // SetSakura
    //{ { NOT }, 0 },                                                                       // StopSakura
    //{ { NUM, NUM, NOT }, 2 },                                                             // SkipDate

    { { NOT }, 0 },                                                                       // SetMovie
    //{ { NUM, STR, NOT }, 2 },                                                             // DebugBox

    //{ { NUM, NUM, NOT }, 2 },                                                             // VHFlag
    //{ { REG, REG, REG, REG, NOT }, 4 },                                                   // GetSystemTime

    //{ { STR, NOT }, 1 },                                                                  // SetName
    //{ { STR, STR, NOT }, 2 },                                                             // SetMes
    //{ { NUM, NOT }, 1 },                                                                  // WN
    //{ { STR, NOT }, 1 },                                                                  // WNS
};

ScriptOperator getOperatorCode(std::string opName) {
    int i,
        j = static_cast<int>(ScriptOperator::OpMax),
        k = static_cast<int>(ScriptOperator::OpEnd);

    for (i = static_cast<int>(ScriptOperator::Start); i < j; i++) {
        if (i == k) {
            i = static_cast<int>(ScriptOperator::B);
        }
        if (g_operatorList[i].name == opName) {
            return g_operatorList[i].code;
        }
    }
    return ScriptOperator::Start;
}