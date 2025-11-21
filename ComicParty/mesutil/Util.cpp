#include "Util.h"
#include <commdlg.h>
#include <vector>
#include <iostream>

// Link against comdlg32.lib if not already done by project settings
#pragma comment(lib, "comdlg32.lib")

namespace Util {

    std::vector<std::wstring> GetMesFiles(LPWSTR lpExt) {
        std::vector<std::wstring> filePaths;
        // Buffer for multiple file names (roughly 32KB)
        const int BUFFER_SIZE = 32768;
        wchar_t* buffer = new wchar_t[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE * sizeof(wchar_t));

        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GetConsoleWindow();
        ofn.lpstrFile = buffer;
        ofn.nMaxFile = BUFFER_SIZE;
        ofn.lpstrFilter = lpExt; // e.g., L"MES Files\0*.mes\0Excel Files\0*.xlsx\0"
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = L".\\";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_DONTADDTORECENT;

        if (GetOpenFileNameW(&ofn)) {
            // Check if multiple files are selected
            // Format: Directory\0File1\0File2\0\0
            // Or single: FullPath\0\0
            std::wstring dir = buffer;
            size_t offset = dir.length() + 1;

            if (buffer[offset] == 0) {
                // Single file
                filePaths.push_back(dir);
            }
            else {
                // Multiple files
                while (buffer[offset] != 0) {
                    std::wstring filename = &buffer[offset];
                    std::wstring fullPath = dir;
                    if (fullPath.back() != L'\\') fullPath += L"\\";
                    fullPath += filename;
                    filePaths.push_back(fullPath);
                    offset += filename.length() + 1;
                }
            }
        }

        delete[] buffer;
        return filePaths;
    }

    UINT GetCodePageID(EncodingType type) {
        switch (type) {
        case EncodingType::ACP: return CP_ACP;
        case EncodingType::ShiftJIS: return 932;
        case EncodingType::UTF8:
        case EncodingType::CustomUTF8: return CP_UTF8;
        default: return CP_ACP;
        }
    }

    void EncodeCustomUTF8(std::string& text) {
        // Rule: 3-byte UTF8. First byte (E0-EF) <-> (A0-AF) via XOR 0x40
        // Valid UTF-8 3-byte sequence starts with 1110xxxx (0xE0-0xEF)
        // Custom uses 0xA0-0xAF instead.

        size_t i = 0;
        while (i < text.length()) {
            unsigned char c = static_cast<unsigned char>(text[i]);

            // Check if it is a target for swapping (0xE0-0xEF or 0xA0-0xAF)
            // And ensure we have enough bytes following for validity (at least basic check)
            bool bIsTarget = ((c & 0xF0) == 0xE0) || ((c & 0xF0) == 0xA0);

            if (bIsTarget) {
                // XOR the first byte
                text[i] = static_cast<char>(c ^ 0x40);
                // Skip the next 2 bytes as they are continuation bytes in 3-byte UTF8
                i += 3;
            }
            else if ((c & 0x80) == 0) {
                // ASCII
                i++;
            }
            else {
                // Other multibyte, skip based on logic or just ++ (simple iteration here)
                // For robustness, one should parse UTF8 length, but simplified:
                i++;
            }
        }
    }

    // 예외 매핑 테이블 정의 (필요에 따라 추가)
    // 예시: Shift-JIS 0xF001 <-> Unicode U+FF01 (전각 느낌표)
    // 주의: 0xF0은 Shift-JIS에서 사용자 정의 영역(Gaiji)의 리드 바이트일 수 있음
    bool GetSjisException(unsigned char lead, unsigned char trail, wchar_t& outUnicode) {
        // 예시 규칙: 0xF0 0x40 -> U+FE3F
        if (lead == 0xF0 && trail >= 0x40 && trail <= 0x48) {
            switch (trail) {
            case 0x40:
                outUnicode = 0x23E4; // 수평바
                break;
            case 0x41:
                outUnicode = 0x266A; // 음표
                printf("from:0x%04X\n", outUnicode);
                break;
            case 0x42:
                outUnicode = 0x2665; // 채워진 하트
                break;
            case 0x43:
                outUnicode = 0x3030; // 물결표
                break;
            case 0x44:
                outUnicode = 0xFF1B; // 땀
                break;
            case 0x45:
                outUnicode = 0x25A7; // 펼쳐진 책1
                break;
            case 0x46:
                outUnicode = 0x25A8; // 펼쳐진 책2
                break;
            case 0x47:
                outUnicode = 0x25B2; // 채워진 삼각 기호
                break;
            case 0x48:
                outUnicode = 0x25BC; // 채워진 역삼각 기호
            }
            return true;
        }
        else if (lead == 0x81 && trail == 0xF4) {
            outUnicode = 0x2669; // 4분 음표
            return true;
        }
        return false;
    }

    bool GetUnicodeException(wchar_t unicode, unsigned char& outLead, unsigned char& outTrail) {
        // 역방향 규칙
        switch (unicode) {
        case 0x2669:
            outLead = 0x81;
            outTrail = 0xF4;
            return true;
        case 0x23E4:
            outTrail = 0x40;
            goto RET_TRUE;
        case 0x266A:
            outTrail = 0x41;
            goto RET_TRUE;
        case 0x2665:
            outTrail = 0x42;
            goto RET_TRUE;
        case 0x3030:
            outTrail = 0x43;
            goto RET_TRUE;
        case 0xFF1B:
            outTrail = 0x44;
            goto RET_TRUE;
        case 0x25A7:
            outTrail = 0x45;
            goto RET_TRUE;
        case 0x25A8:
            outTrail = 0x46;
            goto RET_TRUE;
        case 0x25B2:
            outTrail = 0x47;
            goto RET_TRUE;
        case 0x25BC:
            outTrail = 0x48;
        RET_TRUE:
            outLead = 0xF0;
            return true;
        }
        return false;
    }

    // Shift-JIS -> WideChar conversion with exception handling
    std::wstring SjisToWide_WithException(const std::string& src) {
        std::wstring result;
        result.reserve(src.size());

        size_t i = 0;
        while (i < src.size()) {
            unsigned char c1 = (unsigned char)src[i];

            // Check if it is a Shift-JIS (CP932) Lead Byte
            // Lead Byte ranges: 0x81-0x9F, 0xE0-0xFC
            if (((0x81 <= c1) && (c1 <= 0x9F)) || ((0xE0 <= c1) && (c1 <= 0xFC))) {
                // Handle 2-byte character
                unsigned char c2 = (i + 1 < src.size()) ? (unsigned char)src[i + 1] : 0;

                wchar_t exceptionChar = 0;

                // Check exception table (2-byte)
                if (GetSjisException(c1, c2, exceptionChar)) {
                    result += exceptionChar;
                }
                else {
                    // Standard conversion (Windows API)
                    wchar_t wBuf[2] = { 0 };
                    // Explicitly handle 2 bytes
                    char mbBuf[2] = { (char)c1, (char)c2 };
                    if (MultiByteToWideChar(932, 0, mbBuf, 2, wBuf, 2) > 0) {
                        result += wBuf[0];
                    }
                    else {
                        // Fallback for conversion failure
                        result += L'?';
                    }
                }
                i += 2;
            }
            else {
                // Handle 1-byte character (ASCII or Half-width Katakana 0xA1-0xDF)
                // Check exception for 1-byte if needed (passing 0 as trail byte)
                wchar_t exceptionChar = 0;
                if (GetSjisException(c1, 0, exceptionChar)) {
                    result += exceptionChar;
                }
                else {
                    // Standard conversion
                    wchar_t wBuf[2] = { 0 };
                    char mbBuf[1] = { (char)c1 };
                    if (MultiByteToWideChar(932, 0, mbBuf, 1, wBuf, 2) > 0) {
                        result += wBuf[0];
                    }
                    else {
                        result += (wchar_t)c1; // ASCII fallback
                    }
                }
                i += 1;
            }
        }
        return result;
    }

    // 예외 처리를 포함한 WideChar -> Shift-JIS 변환
    std::string WideToSjis_WithException(const std::wstring& src) {
        std::string result;
        // 대략 2배 예약
        result.reserve(src.size() * 2);

        for (wchar_t wc : src) {
            unsigned char l = 0, t = 0;

            // 1. 역방향 예외 확인
            if (GetUnicodeException(wc, l, t)) {
                result += (char)l;
                if (t != 0) result += (char)t; // 2바이트인 경우만
            }
            else {
                // 2. 일반 변환
                char mbBuf[4] = { 0 };
                // 기본적으로 매핑 실패시 '?' 대신 원래 로직을 따름 (Flag 이용 가능)
                int len = WideCharToMultiByte(932, 0, &wc, 1, mbBuf, 4, NULL, NULL);
                if (len > 0) {
                    result.append(mbBuf, len);
                }
            }
        }
        return result;
    }


    std::string StringConvert(const std::string& src, EncodingType srcEnc, EncodingType targetEnc) {
        if (src.empty()) return "";
        if (srcEnc == targetEnc && srcEnc != EncodingType::CustomUTF8) return src;

        std::string tempSrc = src;

        // 1. Handle Input Custom Logic (Revert to standard UTF8)
        if (srcEnc == EncodingType::CustomUTF8) {
            EncodeCustomUTF8(tempSrc);
        }

        std::wstring wStr;
        std::wstring tempStr;

        // 2. Convert to WideChar (Unicode)
        // [변경] Shift-JIS인 경우 예외 처리 로직 적용
        if (srcEnc == EncodingType::ShiftJIS) {
            wStr = SjisToWide_WithException(tempSrc);
        }
        else {
            // 일반 변환
            UINT uSrcCP = GetCodePageID(srcEnc);
            int nLen = MultiByteToWideChar(uSrcCP, 0, tempSrc.c_str(), (int)tempSrc.size(), NULL, 0);
            if (nLen == 0) return "";
            wStr.resize(nLen);
            MultiByteToWideChar(uSrcCP, 0, tempSrc.c_str(), (int)tempSrc.size(), &wStr[0], nLen);
        }

        // 3. Convert to Target Encoding
        std::string result;

        // [변경] Target이 Shift-JIS인 경우 예외 처리 로직 적용
        if (targetEnc == EncodingType::ShiftJIS) {
            result = WideToSjis_WithException(wStr);
        }
        else {
            // 일반 변환
            UINT uDstCP = GetCodePageID(targetEnc);
            int nResultLen = WideCharToMultiByte(uDstCP, 0, wStr.c_str(), (int)wStr.size(), NULL, 0, NULL, NULL);
            if (nResultLen == 0) return "";
            result.resize(nResultLen);
            WideCharToMultiByte(uDstCP, 0, wStr.c_str(), (int)wStr.size(), &result[0], nResultLen, NULL, NULL);
        }

        // 4. Handle Output Custom Logic
        if (targetEnc == EncodingType::CustomUTF8) {
            EncodeCustomUTF8(result); // Apply Custom transform to UTF8
        }

        return result;
    }
}
