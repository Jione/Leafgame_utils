#define NOMINMAX
#include "EventParse.h"
#include "EventFile.h"
#include "Util.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace Event {

    namespace kr {
        const wchar_t* RawCharaNames[] = {
            L"주인공", L"미즈키", L"미나미", L"유우", L"에이미", L"아야", L"아사히", L"레이코",
            L"치사", L"이쿠미", L"타이시", L"수수께끼 남자", L"운송업자", L"편집장", L"오타쿠", L"삼인조",
            L"스태프", L"소녀 목소리", L"소녀 목소리 Ａ", L"소녀 목소리 Ｂ", L"소녀 목소리 Ｃ", L"손님", L"전화", L"？",
            L"종업원", L"아이", L"아저씨", L"누님", L"어머님", L"소녀", L"점원", L"사회자",
            L"해설자", L"판매원", L"웨이트리스", L"여관 종업원", L"모모", L"남자 목소리", L"마히루", L"헤모헤모",
            L"리포터", L"여자 목소리", L"코스플레이어", L"삼인조", L"미호", L"마유", L"유카", L"아빠",
            L"엄마", L"여자", L"역무원", L"선배", L"인쇄소 직원", L"방송", L"아버지", L"어머니"
        };
        const wchar_t* NarrString = L"나레이션";
        const wchar_t* SelectString = L"선택지";
        const int RawCharaCount = sizeof(RawCharaNames) / sizeof(RawCharaNames[0]);
    }

    namespace jp {
        const wchar_t* RawCharaNames[] = {
            L"主人公", L"瑞希", L"南", L"由宇", L"詠美", L"彩", L"あさひ", L"玲子",
            L"千紗", L"郁美", L"大志", L"謎の男", L"運送屋さん", L"編集長", L"おたく", L"三人組",
            L"スタッフ", L"女の子の声", L"女の子の声Ａ", L"女の子の声Ｂ", L"女の子の声Ｃ", L"客", L"電話", L"？",
            L"従業員", L"子供", L"おっちゃん", L"ねえちゃん", L"母親", L"女の子", L"店員", L"司会",
            L"解説", L"売り子", L"ウェイトレス", L"仲居", L"モモ", L"男の声", L"まひる", L"へもへも",
            L"レポーター", L"女の人の声", L"コスプレイヤ", L"三人組", L"美穂", L"まゆ", L"夕香", L"パパ",
            L"ママ", L"女の人", L"駅員", L"先輩", L"印刷所の子", L"放送", L"お父さん", L"お母さん"
        };
        const wchar_t* NarrString = L"ナレーション";
        const wchar_t* SelectString = L"選択肢";
        const int RawCharaCount = sizeof(RawCharaNames) / sizeof(RawCharaNames[0]);
    }

    const wchar_t* GetIdName(int id) {
        if (id >= 0 && id < jp::RawCharaCount) {
            return jp::RawCharaNames[id];
        }
        else if (id == -1) {
            return jp::NarrString;
        }
        else if (id == -2) {
            return jp::SelectString;
        }
        return L"";
    }

    std::string GetUtf8Name(int id) {
        if (id >= 0 && id < kr::RawCharaCount) {
            return Util::WideToMultiByteStr(kr::RawCharaNames[id], CP_UTF8);
        }
        else if (id == -2) {
            return Util::WideToMultiByteStr(kr::SelectString, CP_UTF8);
        }
        return "";
    }

    typedef bool (*IsTargetOpcodeFn)(void* user, uint32_t opcode);

    typedef void (*OnTargetOpcodeFn)(
        void* user,
        uint32_t blockIndex,
        uint32_t wordOffsetInBlock,
        uint32_t opcode,
        const uint32_t* args,
        size_t argWords);

    struct ParseError {
        uint32_t blockIndex;
        uint32_t wordOffsetInBlock;
        uint32_t opcode;
    };

    // Returns true if opcode is known and args are fully within [args, end].
    // outArgWords is the number of DWORD operands (not including the opcode DWORD).
    static bool GetArgWords(uint32_t opcode, const uint32_t* args, const uint32_t* end, size_t* outArgWords)
    {
        if (!outArgWords) return false;

        size_t n = 0;

        switch (opcode) {
        // Variable length:
        // 0x14: [count][id0][id1]...[id(count-1)]
        case 0x14u:
        {
            if (args >= end) return false;
            const uint32_t count = args[0];
            n = 1u + (size_t)count;
            break;
        }

        // Special fixed length:
        // 0x10 consumes 2 DWORDs, then falls through to a shared 3-DWORD compare/jump sequence (LABEL_158).
        // Treat as total 5 DWORD operands to safely "pass".
        case 0x10u:
            n = 5;
            break;

        // 4 operands
        case 0x2Au:
            n = 4;
            break;

        // 3 operands
        case 0x29u:
        case 0x2Du:
        case 0x13Bu: case 0x13Cu: case 0x13Du: case 0x13Eu: case 0x13Fu: case 0x140u:
        case 0x143u:
        case 0x1C2u:
        case 0x1C3u: case 0x1C4u: case 0x1C5u: case 0x1C6u: case 0x1C7u: case 0x1C8u:
        case 0x1C9u: case 0x1CAu: case 0x1CBu: case 0x1CCu: case 0x1CDu:
        case 0x1D0u: case 0x1D1u: case 0x1D2u: case 0x1D3u: case 0x1D4u: case 0x1D5u:
            n = 3;
            break;

        // 2 operands
        case 0x04u: case 0x05u:
        case 0x12Cu: case 0x12Du:
        case 0x131u: case 0x132u:
        case 0x15u:
        case 0x20u:
        case 0x22u:
        case 0x2Eu:
        case 0x3Du: case 0x3Eu: case 0x3Fu: case 0x42u:
        case 0x1CEu: case 0x1CFu:
        case 0x1D6u: case 0x1D7u: case 0x1D8u: case 0x1D9u: case 0x1DAu: case 0x1DBu:
        case 0x1E0u: case 0x1E1u: case 0x1E2u: case 0x1E3u: case 0x1E4u: case 0x1E5u:
            n = 2;
            break;

        // 1 operand
        case 0x01u: case 0x02u: case 0x03u:
        case 0x06u: case 0x07u:
        case 0x08u: case 0x09u: case 0x0Au: case 0x0Bu:
        case 0x0Eu: case 0x0Fu:
        case 0x1Eu: case 0x1Fu:
        case 0x21u:
        case 0x24u:
        case 0x28u:
        case 0x2Fu:
        case 0x33u: case 0x36u: case 0x37u:
        case 0x40u: case 0x41u:
        case 0x4Eu: case 0x4Fu:
        case 0x50u:
        case 0x5Au: case 0x5Bu:
        case 0x64u:
        case 0x6Eu:
        case 0x70u:
        case 0x12Eu: case 0x12Fu: case 0x130u:
        case 0x1EAu:
        case 0x258u:
        case 0x2BDu:
            n = 1;
            break;

        // 0 operands
        case 0x00u:
        case 0x23u:
        case 0x2Bu: case 0x2Cu:
        case 0x32u:
        case 0x34u: case 0x35u:
        case 0x3Cu:
        case 0x46u: case 0x47u: case 0x48u: case 0x49u:
        case 0x4Cu: case 0x4Du:
        case 0x51u:
        case 0x65u: case 0x66u:
        case 0x6Fu:
        case 0x71u:
        case 0x1F4u: case 0x1F5u:
        case 0x259u:
        case 0x2BCu:
        case 0x4D4u:
            n = 0;
            break;

        default:
            return false;
        }

        if (args + n > end) return false;
        *outArgWords = n;
        return true;
    }

    static bool ParseEventBlockLinear(
        const uint32_t* blockWords,
        size_t blockWordCount,
        uint32_t blockIndex,
        void* user,
        IsTargetOpcodeFn isTarget,
        OnTargetOpcodeFn onTarget,
        ParseError* outErr)
    {
        if (!blockWords) return false;

        const uint32_t* pc = blockWords;
        const uint32_t* end = blockWords + blockWordCount;

        while (pc < end)
        {
            const uint32_t wordOffset = (uint32_t)(pc - blockWords);
            const uint32_t opcode = *pc++;

            size_t argWords = 0;
            if (!GetArgWords(opcode, pc, end, &argWords))
            {
                if (outErr)
                {
                    outErr->blockIndex = blockIndex;
                    outErr->wordOffsetInBlock = wordOffset;
                    outErr->opcode = opcode;
                }
                return false;
            }

            if (isTarget && onTarget && isTarget(user, opcode))
            {
                onTarget(user, blockIndex, wordOffset, opcode, pc, argWords);
            }

            pc += argWords;
        }

        return true;
    }

}

namespace Event {

    static bool EndsWithNoCase(const std::wstring& s, const wchar_t* suffix) {
        if (!suffix) return false;
        const size_t slen = s.size();
        size_t tlen = 0;
        while (suffix[tlen] != 0) ++tlen;
        if (tlen > slen) return false;

        const size_t start = slen - tlen;
        for (size_t i = 0; i < tlen; ++i) {
            wchar_t a = s[start + i];
            wchar_t b = suffix[i];
            a = (wchar_t)towlower(a);
            b = (wchar_t)towlower(b);
            if (a != b) return false;
        }
        return true;
    }

    static std::wstring ResolveEventPathFromInput(const std::wstring& inputPath) {
        // Accept:
        //  - "...eve.dat"  -> 그대로
        //  - "...mes.mes"  -> "...eve.dat"
        //  - otherwise     -> 그대로 (caller can pass eve directly)
        if (EndsWithNoCase(inputPath, L"eve.dat")) {
            return inputPath;
        }

        if (EndsWithNoCase(inputPath, L"mes.mes")) {
            std::wstring out = inputPath;
            out.resize(out.size() - 7); // remove "mes.mes"
            out += L"eve.dat";
            return out;
        }

        return inputPath;
    }

    static void EnsureSize(std::vector<int>& v, uint32_t index, int fillValue) {
        if (index >= (uint32_t)v.size()) {
            v.resize((size_t)index + 1, fillValue);
        }
    }

    static int PriorityOfValue(int value) {
        // Higher priority overwrites lower.
        // >=0: definite speaker
        // -1 : narration/hidden speaker
        // -2 : choice-only
        if (value >= 0) return 3;
        if (value == -1) return 2;
        if (value == -2) return 1;
        return 0;
    }

    static void UpdateByPriority(std::vector<int>& table, uint32_t msgId, int newValue) {
        EnsureSize(table, msgId, -1);
        const int oldValue = table[msgId];
        //if (PriorityOfValue(newValue) >= PriorityOfValue(oldValue)) {
            table[msgId] = newValue;
        //}
    }

    struct EventCharaContext {
        // VM state subset
        uint32_t currentSpeakerId;
        bool speakerVisible;

        // VM variable table (this+619800). Size is 0x7D00 bytes / 4 = 8000 dwords.
        std::vector<int> vars;

        // Outputs
        std::vector<int>* speakerByMsgId;
    };

    static bool IsTargetOpcode_EventChara(void* user, uint32_t opcode) {
        (void)user;

        // We only "handle" these, but ParseEventBlockLinear will still pass everything else safely.
        switch (opcode)
        {
            // speaker state
        case 0x64u: // speaker set + show
        case 0x65u: // speaker clear + hide
        case 0x2BDu: // speaker set only

            // message output
        case 0x33u:
        case 0x36u:
        case 0x37u:

            // choice
        case 0x14u:

            // variable ops needed to resolve 0x36
        case 0x12Cu: // var[idx]=value
        case 0x12Du: // same (shared label)
        case 0x12Eu: // var[idx]=0
        case 0x12Fu: // var[idx]+=1 (cap 100)
        case 0x130u: // var[idx]-=1 (floor 0)
        case 0x131u: // var[idx]+=delta (cap 100)
        case 0x132u: // var[idx]-=delta (floor 0)
            return true;

        default:
            return false;
        }
    }

    static void RecordMessage(EventCharaContext& ctx, uint32_t msgId, bool isChoice) {
        if (ctx.speakerByMsgId) {
            if (isChoice) {
                // Mark as choice-only unless later overwritten by real message printing.
                UpdateByPriority(*ctx.speakerByMsgId, msgId, -2);
            }
            else {
                const int speakerValue = ctx.speakerVisible ? (int)ctx.currentSpeakerId : -1;
                UpdateByPriority(*ctx.speakerByMsgId, msgId, speakerValue);
            }
        }
    }

    static void OnTargetOpcode_EventChara(
        void* user,
        uint32_t blockIndex,
        uint32_t wordOffsetInBlock,
        uint32_t opcode,
        const uint32_t* args,
        size_t argWords)
    {
        (void)blockIndex;
        (void)wordOffsetInBlock;

        EventCharaContext& ctx = *(EventCharaContext*)user;

        // Safety: ParseEventBlockLinear already guarantees [args..args+argWords) in range.
        switch (opcode)
        {
        case 0x64u:
            // [speakerId]
            if (argWords >= 1) {
                ctx.currentSpeakerId = args[0];
                ctx.speakerVisible = true;
            }
            break;

        case 0x65u:
            // []
            ctx.currentSpeakerId = 0;
            ctx.speakerVisible = false;
            break;

        case 0x2BDu:
            // [speakerId] (no visibility change)
            if (argWords >= 1) {
                ctx.currentSpeakerId = args[0];
            }
            break;

        case 0x12Cu:
        case 0x12Du:
            // [varIndex][value]
            if (argWords >= 2) {
                const uint32_t varIndex = args[0];
                const uint32_t value = args[1];
                if (varIndex < (uint32_t)ctx.vars.size()) {
                    ctx.vars[varIndex] = (int)value;
                }
            }
            break;

        case 0x12Eu:
            // [varIndex] -> 0
            if (argWords >= 1) {
                const uint32_t varIndex = args[0];
                if (varIndex < (uint32_t)ctx.vars.size()) {
                    ctx.vars[varIndex] = 0;
                }
            }
            break;

        case 0x12Fu:
            // [varIndex] -> +1 (cap 100)
            if (argWords >= 1) {
                const uint32_t varIndex = args[0];
                if (varIndex < (uint32_t)ctx.vars.size()) {
                    int v = ctx.vars[varIndex];
                    v = (v <= 99) ? (v + 1) : 100;
                    ctx.vars[varIndex] = v;
                }
            }
            break;

        case 0x130u:
            // [varIndex] -> -1 (floor 0)
            if (argWords >= 1) {
                const uint32_t varIndex = args[0];
                if (varIndex < (uint32_t)ctx.vars.size()) {
                    int v = ctx.vars[varIndex];
                    v = (v >= 1) ? (v - 1) : 0;
                    ctx.vars[varIndex] = v;
                }
            }
            break;

        case 0x131u:
            // [varIndex][delta] -> +delta (cap 100)
            if (argWords >= 2) {
                const uint32_t varIndex = args[0];
                const uint32_t delta = args[1];
                if (varIndex < (uint32_t)ctx.vars.size()) {
                    int v = ctx.vars[varIndex];
                    int add = (int)delta;
                    if (add < 0) add = 0;
                    v = (v + add <= 100) ? (v + add) : 100;
                    ctx.vars[varIndex] = v;
                }
            }
            break;

        case 0x132u:
            // [varIndex][delta] -> -delta (floor 0)
            if (argWords >= 2) {
                const uint32_t varIndex = args[0];
                const uint32_t delta = args[1];
                if (varIndex < (uint32_t)ctx.vars.size()) {
                    int v = ctx.vars[varIndex];
                    int sub = (int)delta;
                    if (sub < 0) sub = 0;
                    v = (v - sub >= 0) ? (v - sub) : 0;
                    ctx.vars[varIndex] = v;
                }
            }
            break;

        case 0x33u:
        case 0x37u:
            // [msgId]
            if (argWords >= 1) {
                RecordMessage(ctx, args[0], false);
            }
            break;

        case 0x36u:
            // [varIndex] -> msgId = vars[varIndex]
            if (argWords >= 1) {
                const uint32_t varIndex = args[0];
                if (varIndex < (uint32_t)ctx.vars.size()) {
                    int msgId = ctx.vars[varIndex];
                    if (msgId >= 0) {
                        RecordMessage(ctx, (uint32_t)msgId, false);
                    }
                }
            }
            break;

        case 0x14u:
            // [count][msgId0..]
            // ParseEventBlockLinear already provided argWords = 1 + count.
            if (argWords >= 1) {
                const uint32_t count = args[0];
                const uint32_t* ids = args + 1;
                const size_t idCount = (argWords > 1) ? (argWords - 1) : 0;
                const size_t safeCount = (count < idCount) ? (size_t)count : idCount;

                for (size_t i = 0; i < safeCount; ++i) {
                    RecordMessage(ctx, ids[i], true);
                }
            }
            break;

        default:
            break;
        }
    }


    bool ParseCharaIDTable(LPWSTR lpTargetFile, std::vector<int>& outTable) {
        if (!lpTargetFile) return false;
        std::vector<int>* outSpeakerByMsgId = &outTable;
        //if (outSpeakerByMsgId) outSpeakerByMsgId->clear();

        std::wstring inputPath = lpTargetFile;
        std::wstring evePath = ResolveEventPathFromInput(inputPath);

        EventFile dat;
        if (!dat.Load(evePath.c_str()) || !dat.IsValid()) {
            std::wcout << L"Failed to load eve.dat: " << evePath << std::endl;
            return false;
        }

        EventCharaContext ctx{};
        ctx.currentSpeakerId = 0;
        ctx.speakerVisible = false;
        ctx.vars.assign(0x7D00 / 4, 0); // 8000 dwords
        ctx.speakerByMsgId = outSpeakerByMsgId;

        for (uint32_t i = 0; i < dat.block_count; ++i) {
            EventBlocks blk{};
            if (!dat.GetBlock(i, blk)) {
                std::cout << "GetBlock failed: " << i << std::endl;
                return false;
            }

            ParseError err{};
            const bool ok = ParseEventBlockLinear(
                blk.base,
                blk.word_count,
                i,
                &ctx,
                &IsTargetOpcode_EventChara,
                &OnTargetOpcode_EventChara,
                &err);

            if (!ok) {
                std::cout << "ParseEventBlockLinear failed: block=" << err.blockIndex
                    << " wordOff=0x" << std::hex << err.wordOffsetInBlock
                    << " op=0x" << std::hex << err.opcode
                    << std::dec << std::endl;
                return false;
            }
        }

        return true;
    }

}
