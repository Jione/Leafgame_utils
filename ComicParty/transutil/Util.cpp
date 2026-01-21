#include "Util.h"
#include <commdlg.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "comdlg32.lib")

namespace Util {

    // --- Helpers ---
    std::string WideToMultiByteStr(const std::wstring& wstr, UINT codePage) {
        if (wstr.empty()) return "";
        int len = WideCharToMultiByte(codePage, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
        if (len <= 0) return "";
        std::string res(len, 0);
        WideCharToMultiByte(codePage, 0, wstr.c_str(), (int)wstr.size(), &res[0], len, NULL, NULL);
        return res;
    }

    std::wstring MultiByteToWideStr(const std::string& str, UINT codePage) {
        if (str.empty()) return L"";
        int len = MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.size(), NULL, 0);
        if (len <= 0) return L"";
        std::wstring res(len, 0);
        MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.size(), &res[0], len);
        return res;
    }

    std::string MultiByteToUtf8(const std::string& str, UINT codePage) {
        if (str.empty()) return "";
        int len = MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.size(), NULL, 0);
        if (len <= 0) return "";
        std::wstring wstr(len, 0);
        len = MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.size(), &wstr[0], len);
        if (len <= 0) return "";
        len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
        if (len <= 0) return "";
        std::string res(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &res[0], len, NULL, NULL);
        return res;
    }

    // --- Parsing Logic Helpers ---

    // Check if byte is a control code
    bool IsControlCode(unsigned char c) {
        static const std::string ctrlChars = "bcklnqrswxy";
        return ctrlChars.find((char)c) != std::string::npos;
    }

    // Parse numbers from binary stream (e.g., "10, 20")
    // Returns bytes consumed from source
    int ParseNumbersFromBin(const unsigned char* src, int maxLen, std::vector<uint32_t>& outNums, int maxCount) {
        int consumed = 0;
        int count = 0;
        bool building = false;
        uint32_t current = 0;

        while (consumed < maxLen && count < maxCount) {
            unsigned char c = src[consumed];
            if (isdigit(c)) {
                current = current * 10 + (c - '0');
                building = true;
                consumed++;
            }
            else if (c == ',' || c == ' ') {
                if (building) {
                    outNums.push_back(current);
                    current = 0;
                    building = false;
                    count++;
                }
                consumed++;
            }
            else {
                break; // Not a digit or separator
            }
        }
        if (building && count < maxCount) {
            outNums.push_back(current);
        }
        return consumed;
    }

    // --- MES -> UTF-8 (Export) ---
    std::string MesBytesToUtf8(const std::vector<char>& data, uint32_t offset, uint32_t len) {
        std::string result;
        uint32_t i = 0;
        const unsigned char* buffer = (const unsigned char*)&data[offset];

        // We iterate carefully. len includes null terminator, but we stop at null manually or by len.
        while (i < len) {
            unsigned char c = buffer[i];
            if (c == 0x00) break; // End of string

            // 1. Control Codes
            // Check for %f, %CN first
            if (c == '%') {
                if (i + 1 < len) {
                    unsigned char c2 = buffer[i + 1];
                    if (c2 == 'f') { // %f -> $N#
                        std::vector<uint32_t> nums;
                        int advanced = ParseNumbersFromBin(&buffer[i + 2], len - (i + 2), nums, 1);
                        result += "$N";
                        if (!nums.empty()) result += std::to_string(nums[0]);
                        i += 2 + advanced;
                        continue;
                    }
                    else if (c2 == 'C' && i + 2 < len && buffer[i + 2] == 'N') { // %CN -> $C#
                        std::vector<uint32_t> nums;
                        int advanced = ParseNumbersFromBin(&buffer[i + 3], len - (i + 3), nums, 1);
                        result += "$C";
                        if (!nums.empty()) result += std::to_string(nums[0]);
                        i += 3 + advanced;
                        continue;
                    }
                }
            }
            // Single char control codes
            if (IsControlCode(c)) {
                // Logic for each control code
                char cc = (char)c;
                if (cc == 'k') { result += "$PA"; i++; continue; }
                if (cc == 'l') { result += "$SLG"; i++; continue; }
                if (cc == 'n') { result += "$SMD"; i++; continue; }
                if (cc == 'q') { result += "$SSM"; i++; continue; }
                if (cc == 'r') { result += "$NL"; i++; continue; }
                if (cc == 's') { result += "$SXS"; i++; continue; }

                // Codes with arguments
                if (cc == 'x' || cc == 'y') {
                    std::vector<uint32_t> nums;
                    int advanced = ParseNumbersFromBin(&buffer[i + 1], len - (i + 1), nums, 1);
                    result += (cc == 'x' ? "$X" : "$Y");
                    if (!nums.empty()) result += std::to_string(nums[0]);
                    i += 1 + advanced;
                    continue;
                }
                if (cc == 'b' || cc == 'c') {
                    std::vector<uint32_t> nums;
                    int advanced = ParseNumbersFromBin(&buffer[i + 1], len - (i + 1), nums, 2);
                    result += (cc == 'b' ? "$B<" : "$C<");
                    if (nums.size() > 0) result += std::to_string(nums[0]);
                    if (nums.size() > 1) { result += ","; result += std::to_string(nums[1]); }
                    result += ">";
                    i += 1 + advanced;
                    continue;
                }
                if (cc == 'w') {
                    std::vector<uint32_t> nums;
                    int advanced = ParseNumbersFromBin(&buffer[i + 1], len - (i + 1), nums, 5);
                    result += "$W<";
                    for (size_t k = 0; k < nums.size(); k++) {
                        if (k > 0) result += ",";
                        result += std::to_string(nums[k]);
                    }
                    result += ">";
                    i += 1 + advanced;
                    continue;
                }
            }

            // 2. Special Exception Table (Shift-JIS range)
            if (i + 1 < len) {
                unsigned char c2 = buffer[i + 1];
                uint16_t val = (c << 8) | c2;
                wchar_t uni = 0;

                if (val == 0x81F4) uni = 0x2669;
                //else if (val == 0x8972) uni = 0x91CC;
                else if (val == 0xF040) uni = 0x23E4;
                else if (val == 0xF041) uni = 0x266A;
                else if (val == 0xF042) uni = 0x2665;
                else if (val == 0xF043) uni = 0x3030;
                else if (val == 0xF044) uni = 0xFF1B;
                else if (val == 0xF045) uni = 0x25A7;
                else if (val == 0xF046) uni = 0x25A8;
                else if (val == 0xF047) uni = 0x25B2;
                else if (val == 0xF048) uni = 0x25BC;

                if (uni != 0) {
                    std::wstring w; w += uni;
                    result += WideToMultiByteStr(w, CP_UTF8);
                    i += 2;
                    continue;
                }
            }

            // 3. Standard Shift-JIS or EUC-KR Check
            // We simply assume if it looks like valid SJIS lead byte, try it.
            // SJIS Lead: 0x81-0x9F, 0xE0-0xFC
            if ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC)) {
                if (i + 1 < len) {
                    char raw[2] = { (char)c, (char)buffer[i + 1] };
                    std::string sjs(raw, 2);
                    std::wstring ws = MultiByteToWideStr(sjs, 932); // SJIS
                    if (!ws.empty() && ws[0] != 0xFFFD) { // Valid
                        result += WideToMultiByteStr(ws, CP_UTF8);
                        i += 2;
                        continue;
                    }
                }
            }
            // EUC-KR Lead : 0xA1-0xDF (overlap with SJIS, but handled if SJIS fails or specific range)
            // EUC-KR Trail: 0xA1-0xFE (overlap with Custom UTF-8 and Extended ASCII) 
            // Here, we just check IsDBCSLeadByteEx logic for 949 if SJIS condition wasn't exclusive enough.
            // But structure says: EUC-KR [0xA1-0xDF] [0xA1-0xFE].
            // This overlaps heavily with SJIS half-width katakana and some SJIS kanji. 
            // Since user said "Shift-JIS... EUC-KR... Priority", we try EUC-KR if SJIS failed or logic dictates.
            // Let's trust IsDBCSLeadByteEx(949) for simplicity or raw range check.
            if (c >= 0xA1 && c <= 0xDF) {
                if (i + 1 < len && buffer[i + 1] >= 0xA1 && buffer[i + 1] <= 0xFE) {
                    char raw[2] = { (char)c, (char)buffer[i + 1] };
                    std::string kr(raw, 2);
                    std::wstring ws = MultiByteToWideStr(kr, 949);
                    if (!ws.empty()) {
                        result += WideToMultiByteStr(ws, CP_UTF8);
                        i += 2;
                        continue;
                    }
                }
            }

            // 4. Extended ASCII (0xB[H] 0x7[L])
            if ((c & 0xF0) == 0xB0) {
                if (i + 1 < len && (buffer[i + 1] & 0xF0) == 0x70) {
                    // Restore: High Nibble from c, Low from c+1
                    // c = 0xB[H], c2 = 0x7[L]
                    unsigned char high = (c & 0x0F);
                    unsigned char low = (buffer[i + 1] & 0x0F);
                    unsigned char ascii = (high << 4) | low;
                    result += (char)ascii;
                    i += 2;
                    continue;
                }
            }

            // 5. Custom UTF-8
            // 3-byte: [0] ^ 0x40, [1] ^ 0xC0, [2] ^ 0xC0
            if (((c & 0xF0) == 0xE0) || ((c & 0xF0) == 0xA0)) {
                // Potential 3-byte. Custom starts with A0-AF (E0 ^ 40 = A0)
                // Check if next bytes exist
                if (i + 2 < len) {
                    unsigned char b1 = c ^ 0x40; // Restore first byte
                    unsigned char b2 = buffer[i + 1] ^ 0xC0;
                    unsigned char b3 = buffer[i + 2] ^ 0xC0;

                    // Basic validation: Valid UTF-8 3-byte starts 1110xxxx 10xxxxxx 10xxxxxx
                    if ((b1 & 0xF0) == 0xE0 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
                        result += (char)b1;
                        result += (char)b2;
                        result += (char)b3;
                        i += 3;
                        continue;
                    }
                }
            }
            // 2-byte: u8Str[1] ^= 0xC0. 
            // Standard UTF-8 2-byte: 110xxxxx (C0-DF) 10xxxxxx (80-BF)
            if (c >= 0xC0 && c <= 0xDF) {
                if (i + 1 < len) {
                    unsigned char b1 = c;
                    unsigned char b2 = buffer[i + 1] ^ 0xC0; // Restore second byte
                    if ((b2 & 0xC0) == 0x80) {
                        result += (char)b1;
                        result += (char)b2;
                        i += 2;
                        continue;
                    }
                }
            }

            // 6. Fallback / ASCII / Half-width Space (0x20)
            result += (char)c;
            i++;
        }
        return result;
    }

    // --- UTF-8 -> MES (Import) ---
    std::vector<char> Utf8ToMesBytes(const std::string& utf8Str) {
        std::vector<char> blob;
        std::wstring wSrc = MultiByteToWideStr(utf8Str, CP_UTF8); // Work in Unicode (UCS-2/UTF-16)

        for (size_t i = 0; i < wSrc.size(); ++i) {
            wchar_t wc = wSrc[i];

            // 1. Check Control Tags ($...)
            if (wc == L'$') {
                // Look ahead to match tags
                std::wstring sub = wSrc.substr(i);

                // Helpers for parsing args from tag
                auto parseArgs = [&](const std::wstring& s, size_t startIdx, char endChar) -> std::vector<uint32_t> {
                    std::vector<uint32_t> args;
                    size_t p = startIdx;
                    uint32_t num = 0;
                    bool build = false;
                    while (p < s.size() && s[p] != endChar) {
                        if (isdigit(s[p])) { num = num * 10 + (s[p] - '0'); build = true; }
                        else if (s[p] == ',' || s[p] == ' ') { if (build) args.push_back(num); num = 0; build = false; }
                        p++;
                    }
                    if (build) args.push_back(num);
                    return args;
                    };
                auto writeArgs = [&](const std::vector<uint32_t>& args) {
                    for (size_t k = 0; k < args.size(); ++k) {
                        if (k > 0) blob.push_back(',');
                        std::string n = std::to_string(args[k]);
                        for (char c : n) blob.push_back(c);
                    }
                    };

                // Match Tags
                if (sub.find(L"$PA") == 0) { blob.push_back('k'); i += 2; continue; }
                if (sub.find(L"$SLG") == 0) { blob.push_back('l'); i += 3; continue; }
                if (sub.find(L"$SMD") == 0) { blob.push_back('n'); i += 3; continue; }
                if (sub.find(L"$SSM") == 0) { blob.push_back('q'); i += 3; continue; }
                if (sub.find(L"$NL") == 0) { blob.push_back('r'); i += 2; continue; }
                if (sub.find(L"$SXS") == 0) { blob.push_back('s'); i += 3; continue; }

                if (sub.find(L"$B<") == 0) {
                    size_t end = sub.find(L">");
                    if (end != std::string::npos) {
                        blob.push_back('b');
                        writeArgs(parseArgs(sub, 3, '>'));
                        i += end; continue;
                    }
                }
                if (sub.find(L"$C<") == 0) {
                    size_t end = sub.find(L">");
                    if (end != std::string::npos) {
                        blob.push_back('c');
                        writeArgs(parseArgs(sub, 3, '>'));
                        i += end; continue;
                    }
                }
                if (sub.find(L"$W<") == 0) {
                    size_t end = sub.find(L">");
                    if (end != std::string::npos) {
                        blob.push_back('w');
                        writeArgs(parseArgs(sub, 3, '>'));
                        i += end; continue;
                    }
                }
                // $X#, $Y#, $N#, $C# (Assume simple number follows)
                auto parseSingleArg = [&](char prefix, const std::wstring& tagStr) -> int {
                    size_t numEnd = tagStr.size();
                    for (size_t k = 2; k < tagStr.size(); ++k) { if (!isdigit(tagStr[k])) { numEnd = k; break; } }
                    if (numEnd > 2) {
                        std::wstring numStr = tagStr.substr(2, numEnd - 2);
                        if (prefix == '%') { // Special for %f, %CN
                            if (tagStr[1] == 'N') { blob.push_back('%'); blob.push_back('f'); } // $N -> %f
                            else if (tagStr[1] == 'C') { blob.push_back('%'); blob.push_back('C'); blob.push_back('N'); } // $C -> %CN
                        }
                        else {
                            blob.push_back(prefix);
                        }
                        std::string n = WideToMultiByteStr(numStr, CP_ACP);
                        for (char c : n) blob.push_back(c);
                        return (int)numEnd - 1;
                    }
                    return 0;
                    };

                if (sub.find(L"$X") == 0) { int adv = parseSingleArg('x', sub); if (adv) { i += adv; continue; } }
                if (sub.find(L"$Y") == 0) { int adv = parseSingleArg('y', sub); if (adv) { i += adv; continue; } }
                if (sub.find(L"$N") == 0) { int adv = parseSingleArg('%', sub); if (adv) { i += adv; continue; } }
                if (sub.find(L"$C") == 0) { int adv = parseSingleArg('%', sub); if (adv) { i += adv; continue; } }
            }

            // 2. Special Exception Table (Reverse)
            unsigned char b1 = 0, b2 = 0;
            bool special = false;
            if (wc == 0x2669) { b1 = 0x81; b2 = 0xF4; special = true; }
            //else if (wc == 0x91CC) { b1 = 0x89; b2 = 0x72; special = true; }
            else if (wc == 0x23E4) { b1 = 0xF0; b2 = 0x40; special = true; }
            else if (wc == 0x266A) { b1 = 0xF0; b2 = 0x41; special = true; }
            else if (wc == 0x2665) { b1 = 0xF0; b2 = 0x42; special = true; }
            else if (wc == 0x3030) { b1 = 0xF0; b2 = 0x43; special = true; }
            else if (wc == 0xFF1B) { b1 = 0xF0; b2 = 0x44; special = true; }
            else if (wc == 0x25A7) { b1 = 0xF0; b2 = 0x45; special = true; }
            else if (wc == 0x25A8) { b1 = 0xF0; b2 = 0x46; special = true; }
            else if (wc == 0x25B2) { b1 = 0xF0; b2 = 0x47; special = true; }
            else if (wc == 0x25BC) { b1 = 0xF0; b2 = 0x48; special = true; }

            if (special) {
                blob.push_back(b1); blob.push_back(b2);
                continue;
            }

            // 3. Try Encoding Priority
            // Priority A: ASCII -> Extended ASCII
            if (wc < 0x80) {
                // Basic ASCII
                if (wc == 0x20) { // Space
                    blob.push_back(0x20);
                }
                else {
                    // Convert to 0xB[H] 0x7[L]
                    unsigned char c = (unsigned char)wc;
                    unsigned char h = (c >> 4) & 0x0F;
                    unsigned char l = (c & 0x0F);
                    blob.push_back(0xB0 | h);
                    blob.push_back(0x70 | l);
                }
                continue;
            }

            // Priority B: Shift-JIS
            std::wstring wTemp; wTemp += wc;
            BOOL usedDef = FALSE;
            std::string sjis = WideToMultiByteStr(wTemp, 932);
            WideCharToMultiByte(932, 0, wTemp.c_str(), 1, NULL, 0, NULL, &usedDef);

            if (!usedDef && !sjis.empty() && (sjis.size() >= 2)) {
                unsigned char c = sjis[0];
                if ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC)) {
                    for (char ch : sjis) blob.push_back(ch);
                    continue;
                }
            }

            // Priority C: EUC-KR
            usedDef = FALSE;
            std::string euc = WideToMultiByteStr(wTemp, 949);
            WideCharToMultiByte(949, 0, wTemp.c_str(), 1, NULL, 0, NULL, &usedDef);
            if (!usedDef && !euc.empty() && (euc.size() >= 2)) {
                unsigned char c = euc[0];
                unsigned char trail = euc[1];
                if (c >= 0xA1 && c <= 0xDF && trail >= 0xA1 && trail <= 0xFE) {
                    for (char ch : euc) blob.push_back(ch);
                    continue;
                }
            }

            // Priority D: Custom UTF-8 Fallback
            std::string u8 = WideToMultiByteStr(wTemp, CP_UTF8);
            if (u8.size() == 3) {
                blob.push_back(u8[0] ^ 0x40);
                blob.push_back(u8[1] ^ 0xC0);
                blob.push_back(u8[2] ^ 0xC0);
            }
            else if (u8.size() == 2) {
                blob.push_back(u8[0]);
                blob.push_back(u8[1] ^ 0xC0);
            }
            else {
                // Should not happen for 1-byte (caught by ASCII check) or 4-byte (not supported/fallback)
                for (char c : u8) blob.push_back(c);
            }
        }
        return blob;
    }
}
