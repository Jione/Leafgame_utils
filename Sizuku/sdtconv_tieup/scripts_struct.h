#pragma once
#ifndef SCRIPTS_STRUCT_H
#define SCRIPTS_STRUCT_H

#include <cstdint>
#include <string>

// 1����Ʈ ����
#pragma pack(push, 1)

// ���۷����� ���� Ÿ��
enum class ScriptArgumentType : uint8_t {
    NotUsed = 0,    // ������� ����
    AsciiChar,      // 1����Ʈ �ƽ�Ű ��
    Number,         // ���� �Ǵ� ���� ����
    String,         // ���ڿ� (char ����)
    String2,        // ���ڿ� (short ����)
    Register,       // 1����Ʈ ��������
    Compare,        // �� ����
    AddValue,       // �߰� ��
    Count,          // ���� ī��Ʈ
    VariableCount,  // ���� ī��Ʈ
    CharValue = 16, // 1����Ʈ Char
    ShortValue,     // 2����Ʈ Short
    LongValue,      // 4����Ʈ Long
};

// ��ũ��Ʈ ���۷����� �ڵ�
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

    OpEnd, // ���п�

    // �̺�Ʈ ���� ��� �ڵ�
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

    // �ο� ���� ���
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

    OpMax, // �� ǥ�ÿ�
};

// ���۷����ͺ� ���� ����ü
struct ScriptOperatorFormat {
    ScriptArgumentType args[15]; // �ִ� 15���� ����
    uint8_t defaultArgCount;     // �⺻ ���� ����
};

// ��ũ��Ʈ ���� ��� ����ü
struct ScriptHeader {
    int16_t header1;                     // ��� �ñ״�ó 1
    int16_t header2;                     // ��� �ñ״�ó 2
    int32_t totalFileSize;               // ���� ��ü ũ��
    uint32_t blockOffsets[256];          // ��� ���� ������ ����Ʈ
};

// ���۷����� �̸� + �ڵ� ���� ����ü
struct OperatorInfo {
    std::string name;        // ���۷����� ��Ī
    ScriptOperator code;     // ���۷����� �ڵ�(enum class)
};

// ĳ���� �̸� + ĳ���� �ڵ� ����ü
struct CharactorInfo {
    std::string name;        // ĳ���� �̸�
    int32_t     code;        // ĳ���� �ڵ�
};

#pragma pack(pop)

// �ܺ� �迭 ����
extern const CharactorInfo g_charactorList[];
extern const OperatorInfo g_operatorList[];
extern const ScriptOperatorFormat g_operatorFormats[];

// ���۷����� �̸����� �ڵ� �˻� �Լ�
ScriptOperator getOperatorCode(std::string opName);

#endif // SCRIPTS_STRUCT_H