#define _CRT_SECURE_NO_WARNINGS
#include "EventScript.h"
#include <new>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace EventScript {

    // ------------------------------------------------------------------------
    // 1. Character Names Data (Internal)
    // ------------------------------------------------------------------------
    namespace {
        const char* RawCharaNames[] = {
            "주인공", "미즈키", "미나미", "유우", "에이미", "아야", "아사히", "레이코",
            "치사", "이쿠미", "타이시", "수수께끼 남자", "운송업자", "편집장", "오타쿠", "삼인조",
            "스태프", "소녀 목소리", "소녀 목소리 Ａ", "소녀 목소리 Ｂ", "소녀 목소리 Ｃ", "손님", "전화", "？",
            "종업원", "아이", "아저씨", "누님", "어머님", "소녀", "점원", "사회자",
            "해설자", "판매원", "웨이트리스", "여관 종업원", "모모", "남자 목소리", "마히루", "헤모헤모",
            "리포터", "여자 목소리", "코스플레이어", "삼인조", "미호", "마유", "유카", "아빠",
            "엄마", "여자", "역무원", "선배", "인쇄소 직원", "방송", "아버지", "어머니"
        };
        const int RawCharaCount = sizeof(RawCharaNames) / sizeof(RawCharaNames[0]);
    }

    const char* Script::GetCharaName(int id) {
        if (id >= 0 && id < RawCharaCount) {
            return RawCharaNames[id];
        }
        return "";
    }

    // ------------------------------------------------------------------------
    // 2. Script Class Implementation (File/Memory Loading)
    // ------------------------------------------------------------------------
    Script::Script() : file_bytes(nullptr), file_size_bytes(0), block_count(0), offsets(nullptr), code_base(nullptr), code_words(0) {}

    Script::~Script() { Unload(); }

    void Script::Unload() {
        delete[] offsets; offsets = nullptr;
        delete[] file_bytes; file_bytes = nullptr;
        file_size_bytes = 0; block_count = 0; code_base = nullptr; code_words = 0;
    }

    bool Script::LoadFromMemory(const uint8_t* buffer, size_t size) {
        Unload();
        if (!buffer || size == 0) return false;

        file_bytes = new (std::nothrow) uint8_t[size];
        if (!file_bytes) return false;
        memcpy(file_bytes, buffer, size);
        file_size_bytes = size;

        return ParseAndValidate();
    }

    bool Script::IsValid() const {
        return file_bytes != nullptr && offsets != nullptr && code_base != nullptr && block_count > 0;
    }

    bool Script::ParseAndValidate() {
        if (!file_bytes || file_size_bytes < 4) return false;
        size_t aligned_bytes = (file_size_bytes / 4) * 4;
        if (aligned_bytes < 4) return false;

        const uint32_t* u32 = (const uint32_t*)file_bytes;
        size_t total_words = aligned_bytes / 4;

        uint32_t n = u32[0];
        if (n == 0) return false;

        size_t header_words = 1ull + (size_t)n;
        if (header_words > total_words) return false;

        offsets = new (std::nothrow) uint32_t[n];
        if (!offsets) return false;

        for (uint32_t i = 0; i < n; ++i) offsets[i] = u32[1 + i];

        code_base = u32 + header_words;
        code_words = total_words - header_words;
        block_count = n;

        // Validate Offsets
        for (uint32_t i = 0; i < n; ++i) {
            if (offsets[i] > code_words) return false;
            if (i > 0 && offsets[i - 1] > offsets[i]) return false;
        }
        return true;
    }

    bool Script::GetBlock(uint32_t blockIndex, BlockInfo& outBlock) const {
        if (!IsValid() || blockIndex >= block_count) return false;

        size_t start_w = (size_t)offsets[blockIndex];
        size_t end_w = (blockIndex + 1 < block_count) ? (size_t)offsets[blockIndex + 1] : code_words;

        if (end_w < start_w || end_w > code_words) return false;

        outBlock.index = blockIndex;
        outBlock.base = code_base + start_w;
        outBlock.wordCount = end_w - start_w;
        return true;
    }

    // ------------------------------------------------------------------------
    // 3. Opcode Logic (Internal Helpers)
    // ------------------------------------------------------------------------
    namespace {
        // Opcode Argument Sizes
        bool GetArgWords(uint32_t opcode, const uint32_t* args, const uint32_t* end, size_t* outArgWords) {
            if (!outArgWords) return false;
            size_t n = 0;

            switch (opcode) {
            case 0x14u: // Variable length: [count][id...]
                if (args >= end) return false;
                n = 1u + (size_t)args[0];
                break;
            case 0x10u: n = 5; break;
            case 0x2Au: n = 4; break;
            case 0x29u: case 0x2Du: case 0x13Bu: case 0x13Cu: case 0x13Du: case 0x13Eu: case 0x13Fu: case 0x140u:
            case 0x143u: case 0x1C2u: case 0x1C3u: case 0x1C4u: case 0x1C5u: case 0x1C6u: case 0x1C7u: case 0x1C8u:
            case 0x1C9u: case 0x1CAu: case 0x1CBu: case 0x1CCu: case 0x1CDu: case 0x1D0u: case 0x1D1u: case 0x1D2u:
            case 0x1D3u: case 0x1D4u: case 0x1D5u: n = 3; break;
            case 0x04u: case 0x05u: case 0x12Cu: case 0x12Du: case 0x131u: case 0x132u: case 0x15u: case 0x20u:
            case 0x22u: case 0x2Eu: case 0x3Du: case 0x3Eu: case 0x3Fu: case 0x42u: case 0x1CEu: case 0x1CFu:
            case 0x1D6u: case 0x1D7u: case 0x1D8u: case 0x1D9u: case 0x1DAu: case 0x1DBu: case 0x1E0u: case 0x1E1u:
            case 0x1E2u: case 0x1E3u: case 0x1E4u: case 0x1E5u: n = 2; break;
            case 0x01u: case 0x02u: case 0x03u: case 0x06u: case 0x07u: case 0x08u: case 0x09u: case 0x0Au:
            case 0x0Bu: case 0x0Eu: case 0x0Fu: case 0x1Eu: case 0x1Fu: case 0x21u: case 0x24u: case 0x28u:
            case 0x2Fu: case 0x33u: case 0x36u: case 0x37u: case 0x40u: case 0x41u: case 0x4Eu: case 0x4Fu:
            case 0x50u: case 0x5Au: case 0x5Bu: case 0x64u: case 0x6Eu: case 0x70u: case 0x12Eu: case 0x12Fu:
            case 0x130u: case 0x1EAu: case 0x258u: case 0x2BDu: n = 1; break;
            default: n = 0; break; // Most 0-arg opcodes
            }

            if (args + n > end) return false;
            *outArgWords = n;
            return true;
        }

        // Parsing Context
        struct EventCharaContext {
            uint32_t currentSpeakerId;
            bool speakerVisible;
            std::vector<int> vars;
            std::vector<int>* speakerByMsgId;
        };

        // Helpers for Priority
        void UpdateByPriority(std::vector<int>& table, uint32_t msgId, int newValue) {
            if (msgId >= table.size()) table.resize(msgId + 1, -1);
            int oldValue = table[msgId];

            auto getPrio = [](int v) {
                if (v >= 0) return 3; // Explicit Speaker
                if (v == -1) return 2; // Narration
                if (v == -2) return 1; // Choice
                return 0;
                };

            if ((newValue == -2) || (getPrio(newValue) >= getPrio(oldValue))) {
                table[msgId] = newValue;
            }
        }

        void RecordMessage(EventCharaContext& ctx, uint32_t msgId, bool isChoice) {
            if (!ctx.speakerByMsgId) return;
            if (isChoice) {
                UpdateByPriority(*ctx.speakerByMsgId, msgId, -2);
            }
            else {
                int val = ctx.speakerVisible ? (int)ctx.currentSpeakerId : -1;
                UpdateByPriority(*ctx.speakerByMsgId, msgId, val);
            }
        }
    }

    // ------------------------------------------------------------------------
    // 4. Parsing Logic (Public Static Method)
    // ------------------------------------------------------------------------
    bool Script::ParseCharaIDTable(const std::vector<uint8_t>& datBuffer, std::vector<int>& outTable) {
        if (datBuffer.empty()) return false;

        Script script;
        if (!script.LoadFromMemory(datBuffer.data(), datBuffer.size()) || !script.IsValid()) {
            return false;
        }

        EventCharaContext ctx{};
        ctx.currentSpeakerId = 0;
        ctx.speakerVisible = false;
        ctx.vars.assign(2000, 0); // 8000 bytes -> 2000 ints
        ctx.speakerByMsgId = &outTable;

        for (uint32_t i = 0; i < script.GetBlockCount(); ++i) {
            BlockInfo blk{};
            if (!script.GetBlock(i, blk)) continue;

            const uint32_t* pc = blk.base;
            const uint32_t* end = blk.base + blk.wordCount;

            while (pc < end) {
                uint32_t opcode = *pc++;
                size_t argWords = 0;
                if (!GetArgWords(opcode, pc, end, &argWords)) break; // Error in stream

                const uint32_t* args = pc;
                pc += argWords; // Advance

                // Process Opcode
                switch (opcode) {
                case 0x64u: // Set Speaker & Show
                    if (argWords >= 1) { ctx.currentSpeakerId = args[0]; ctx.speakerVisible = true; }
                    break;
                case 0x65u: // Hide Speaker
                    ctx.currentSpeakerId = 0; ctx.speakerVisible = false;
                    break;
                case 0x2BDu: // Set Speaker Only
                    if (argWords >= 1) ctx.currentSpeakerId = args[0];
                    break;
                case 0x33u: case 0x37u: // Message
                    if (argWords >= 1) RecordMessage(ctx, args[0], false);
                    break;
                case 0x14u: // Choice
                    if (argWords >= 1) {
                        uint32_t cnt = args[0];
                        for (uint32_t k = 0; k < cnt && k < argWords - 1; k++) RecordMessage(ctx, args[1 + k], true);
                    }
                    break;

                    // Variable Ops (Context Tracking)
                case 0x12Cu: case 0x12Du: // Set
                    if (argWords >= 2 && args[0] < ctx.vars.size()) ctx.vars[args[0]] = (int)args[1];
                    break;
                case 0x12Eu: // Zero
                    if (argWords >= 1 && args[0] < ctx.vars.size()) ctx.vars[args[0]] = 0;
                    break;
                case 0x36u: // Msg via Var
                    if (argWords >= 1 && args[0] < ctx.vars.size()) {
                        int mid = ctx.vars[args[0]];
                        if (mid >= 0) RecordMessage(ctx, (uint32_t)mid, false);
                    }
                    break;
                }
            }
        }
        return true;
    }
}
