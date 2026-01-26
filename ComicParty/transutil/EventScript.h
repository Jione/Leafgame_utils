#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace EventScript {

    // 스크립트 블록 정보 (내부 구조체)
    struct BlockInfo {
        uint32_t index;
        const uint32_t* base; // 데이터 시작 포인터
        size_t wordCount;     // DWORD 단위 길이
    };

    // 통합된 이벤트 스크립트 클래스
    class Script {
    public:
        Script();
        ~Script();

        // 메모리 버퍼에서 스크립트 로드 (dat 파일 데이터)
        bool LoadFromMemory(const uint8_t* buffer, size_t size);

        // 데이터 해제
        void Unload();

        // 유효성 검사
        bool IsValid() const;

        // 내부 블록 접근 (파싱 로직용)
        bool GetBlock(uint32_t blockIndex, BlockInfo& outBlock) const;
        uint32_t GetBlockCount() const { return block_count; }

        // --- 정적 헬퍼 함수 (파싱 로직) ---

        // 메모리에 있는 dat 데이터를 파싱하여 화자 ID 테이블 추출
        // (내부적으로 Script 클래스를 인스턴스화 하여 처리)
        static bool ParseCharaIDTable(const std::vector<uint8_t>& datBuffer, std::vector<int>& outTable);

        // 캐릭터 ID에 해당하는 이름 문자열 반환 (CP949)
        // 범위 밖이거나 알 수 없는 경우 빈 문자열 반환
        static const wchar_t* GetIdName(int id);
        static std::string Script::GetUtf8Name(int id);

    private:
        bool ParseAndValidate();

    private:
        uint8_t* file_bytes;
        size_t file_size_bytes;

        uint32_t block_count;
        uint32_t* offsets;
        const uint32_t* code_base;
        size_t code_words;
    };
}
