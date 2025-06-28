#pragma once
#ifndef SCRIPTS_STRUCT_H
#define SCRIPTS_STRUCT_H

#include <cstdint>
#include <string>

// 1바이트 정렬
#pragma pack(push, 1)

// 오퍼레이터 인자 타입
enum class ScriptArgumentType : uint8_t {
    NotUsed = 0,    // 사용하지 않음
    AsciiChar,      // 1바이트 아스키 값
    Number,         // 고정 또는 가변 숫자
    String,         // 문자열 (char 길이)
    String2,        // 문자열 (short 길이)
    Register,       // 1바이트 레지스터
    Compare,        // 비교 연산
    AddValue,       // 추가 값
    Count,          // 고정 카운트
    VariableCount,  // 가변 카운트
    CharValue = 16, // 1바이트 Char
    ShortValue,     // 2바이트 Short
    LongValue,      // 4바이트 Long
};

// 스크립트 오퍼레이터 코드
enum class ScriptOperator : uint16_t {
    Start = 0,
    End,

    MovR,
    MovV,
    Swap,
    Rand,

    IfR,
    IfV,
    IfElseR,
    IfElseV,

    Loop,
    Goto,

    Inc,
    Dec,
    Not,
    Neg,

    AddR,
    AddV,
    SubR,
    SubV,
    MulR,
    MulV,
    DivR,
    DivV,
    ModR,
    ModV,
    AndR,
    AndV,
    OrR,
    OrV,
    XorR,
    XorV,

    Calc,

    Pusha,
    Popa,

    Call,
    Ret,

    Wait,
    TWait,
    Run,

    SLoad,
    TStart,

    OpEnd, // 구분용

    // 이벤트 제어 명령 코드
    B = 64,
    BT,
    BC,
    BCT,
    BD,
    BR,
    BF,
    V,
    H,

    S,
    Z,
    FI,
    FIF,
    FO,
    FOF,
    FB,
    PFI,
    PFO,
    PWI,
    PWO,

    Q,
    F,

    C,
    CR,
    CP,
    CL,
    CY,
    CB,
    CA,
    CW,
    CRW,

    W,
    WR,

    KW,
    K,

    M,
    MS,
    MP,
    MV,
    MW,

    SE,
    SEP,
    SES,
    SEW,
    SEV,
    SEVW,

    SetVolumeVoiceChar,
    SetVolumeVoiceScript,
    SV,
    SVEX,

    VV,
    VA,
    VB,
    VC,
    VX,
    VW,
    VS,
    VI,

    R,
    RC,
    RR,

    LF,

    WE,
    WER,

    // 로우 레벨 명령
    SetFlag,
    GetFlag,

    SetGameFlag,
    GetGameFlag,

    LoadScript,
    GameEnd,
    CallFunc,

    SetTimeMode,
    SetTimeBack,
    SetTimeChar,

    SetMatrix,
    ShowChipBack,
    PoseChipBack,
    PlayChipBack,

    SetChipBack,
    SetChipParts,
    SetChipScroll,
    SetChipScroll2,
    SetChipScrollSpeed,
    WaitChipScroll,
    WaitChipScrollSpeed,

    SetChipCharCash,
    ResetChipCharCash,

    SetChipChar,
    SetChipCharAni,
    ResetChipChar,
    SetChipCharParam,
    SetChipCharBright,
    SetChipCharMove,
    SetChipCharMove2,
    SetChipCharMoveSpeed,
    GetChipCharMove,
    GetChipCharMoveSpeed,
    CopyChipCharPos,
    SetChipCharRev,
    ThroughChipCharAni,

    WaitChipCharRepeat,
    WaitChipCharAni,
    WaitChipCharAniLoop,
    WaitChipCharMove,

    GetBack,
    SetBack,
    SetBack2,
    SetCg,
    SetCg2,
    SetBackPos,
    SetBackPosZ,
    SetBackScroll,
    SetBackScrollZ,
    SetBackCScope,
    WaitBackCScope,
    LockBackCScope,
    SetBackFadeIn,
    SetBackFadeOut,
    SetBackFadeRGB,

    SetShake,
    StopShake,

    SetFlash,

    SetChar,
    ResetChar,
    SetCharPose,
    SetCharLocate,
    SetCharLayer,
    WaitChar,

    SetBlock,
    SetWindow,
    ResetWindow,
    SetMessage,
    SetMessage2,
    SetMessageEx,
    SetChipMessage,

    AddMessage,
    AddMessage2,
    SetMessageWait,
    ResetMessage,

    WaitKey,

    SetSelectMes,
    SetSelectMesEx,
    SetSelect,
    SetSelectEx,

    PlayBgm,
    PlayBgmEx,
    StopBgm,
    StopBgmEx,
    SetVolumeBgm,
    SetVolumeBgmEx,

    PlaySe,
    PlaySeEx,
    StopSeEx,
    SetVolumeSe,

    SetWeather,
    ChangeWeather,
    ResetWeather,
    SetLensFrea,
    SetWavEffect,
    ResetWavEffect,

    SetWarp,
    ResetWarp,

    WaitFrame,

    SetBmp,
    SetBmpEx,
    SetBmp4Bmp,
    SetBmpPrim,
    ResetBmp,
    ResetBmpAll,
    SetBmpAnime,
    ResetBmpAnime,
    WaitBmpAnime,
    SetTitle,
    SetEnding,

    SetAvi,
    ResetAvi,
    WaitAvi,
    SetAviFull,
    WaitAviFull,

    SetBmpDisp,
    SetBmpLayer,
    SetBmpParam,
    SetBmpRevParam,
    SetBmpBright,
    SetBmpMove,
    SetBmpPos,
    SetBmpZoom,
    SetBmpZoom2,

    SetDemoFlag,
    SetSceneNo,
    SetEndingNo,
    SetReplayNo,

    SetSoundEvent,
    SetSoundEventVolume,

    SetPotaPota,

    GetTime,
    WaitTime,

    SetTextFormat,
    SetTextSync,
    SetText,
    SetTextEx,
    ResetText,
    WaitText,
    ResetTextAll,

    SetDemoFadeFlag,

    Mov2,
    Sin,
    Cos,
    Abs,

    SetCutCut,

    SetNoise,
    T,
    SetUsoErr,
    LoadScriptNum,

    SetRipple,
    SetRippleSet,
    WaitRipple,
    SetRippleLost,
    MLW,

    VT,
    HT,

    SetMovie,
    DebugBox,

    VHFlag,

    OpMax, // 끝 표시용
};

// 오퍼레이터별 포맷 구조체
struct ScriptOperatorFormat {
    ScriptArgumentType args[15]; // 최대 15개의 인자
    uint8_t defaultArgCount;     // 기본 인자 개수
};

// 스크립트 파일 헤더 구조체
struct ScriptHeader {
    int16_t header1;                     // 헤더 시그니처 1
    int16_t header2;                     // 헤더 시그니처 2
    int32_t totalFileSize;               // 파일 전체 크기
    uint32_t blockOffsets[256];          // 블록 시작 오프셋 리스트
};

// 오퍼레이터 이름 + 코드 매핑 구조체
struct OperatorInfo {
    std::string name;        // 오퍼레이터 명칭
    ScriptOperator code;     // 오퍼레이터 코드(enum class)
};

// 캐릭터 이름 + 캐릭터 코드 구조체
struct CharactorInfo {
    std::string name;        // 캐릭터 이름
    int32_t     code;        // 캐릭터 코드
};

#pragma pack(pop)

// 외부 배열 선언
extern const CharactorInfo g_charactorList[];
extern const OperatorInfo g_operatorList[];
extern const ScriptOperatorFormat g_operatorFormats[];

// 오퍼레이터 이름으로 코드 검색 함수
ScriptOperator getOperatorCode(std::string opName);

#endif // SCRIPTS_STRUCT_H