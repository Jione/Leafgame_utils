#include "LZSS.h"

namespace LZSS {
    using namespace LZSS_Const;

    // ------------------------
    // Internal LZSS decoder
    // ------------------------
    static bool DecodeLZSS(const uint8_t* src, size_t srcSize, uint8_t* dst, size_t expectedSize) {
        if (!src || !dst || expectedSize == 0) {
            return expectedSize == 0;
        }

        // text buffer: ring buffer with look-ahead
        uint8_t text_buf[N] = { 0 };

        int r = 0;
        unsigned int flags = 0;

        size_t srcPos = 0;
        size_t dstPos = 0;
        int filled = 0;

        while (srcPos < srcSize && dstPos < expectedSize) {
            // Reload flags every 8 bits
            if (((flags >>= 1) & 0x100u) == 0) {
                if (srcPos >= srcSize) {
                    break;
                }
                flags = static_cast<unsigned int>(src[srcPos++]) | 0xFF00u;
            }

            if (flags & 1u) {
                // Literal
                if (srcPos >= srcSize) {
                    break;
                }
                uint8_t c = src[srcPos++];
                dst[dstPos++] = c;

                text_buf[r] = c;
                r = (r + 1) & (N - 1);
                if (filled < N) ++filled;
            }
            else {
                // Back-reference
                if (srcPos + 1 > srcSize) {
                    break;
                }

                int j = src[srcPos++];
                if (srcPos >= srcSize) {
                    break;
                }
                int i = src[srcPos++];

                // Offset: 12 bits
                i = (i << 4) + ((j & 0xF0) >> 4);
                j &= 0x0F;

                // Length: low 4 bits + THRESHOLD
                if ((j == 0x0F) && (srcPos < srcSize)) {
                    j += src[srcPos++];
                }
                j += THRESHOLD;

                int dstPosBuf = dstPos;
                for (int k = 0; k <= j && dstPos < expectedSize; ++k) {
                    dst[dstPosBuf++] = text_buf[((i + k) % filled)];
                }

                for (int k = 0; k <= j; ++k) {
                    text_buf[r] = dst[dstPos++];
                    r = (r + 1) & (N - 1);
                    if (filled < N) ++filled;
                }
            }
        }

        // We expect to fully reconstruct expectedSize bytes
        return (dstPos == expectedSize);
    }

    // ------------------------
    // Public Decompress API
    // ------------------------
    bool Decompress(HANDLE hSrcFile, const FileEntry* entry, std::vector<uint8_t>& outBuffer) {
        if (!entry || hSrcFile == INVALID_HANDLE_VALUE) return false;

        outBuffer.clear();

        // 1. 데이터 위치로 이동
        if (SetFilePointer(hSrcFile, entry->DataOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
            return false;
        }

        // Case A: 압축되지 않음 (Raw)
        if (entry->Flags == 0) {
            outBuffer.resize(entry->DataSize);
            DWORD bytesRead = 0;
            if (entry->DataSize > 0) {
                if (!ReadFile(hSrcFile, outBuffer.data(), entry->DataSize, &bytesRead, NULL) || bytesRead != entry->DataSize) {
                    return false;
                }
            }
            return true;
        }

        // Case B: LZSS 압축됨
        if (entry->Flags == 1) {
            uint32_t blockSize = 0;
            DWORD bytesRead = 0;

            // 전체 블록 크기 읽기
            if (!ReadFile(hSrcFile, &blockSize, 4, &bytesRead, NULL) || bytesRead != 4) return false;

            if (blockSize < 8) return false; // 최소 헤더 크기 미달

            uint32_t remaining = blockSize - 4;
            std::vector<uint8_t> block(remaining);
            if (!ReadFile(hSrcFile, block.data(), remaining, &bytesRead, NULL) || bytesRead != remaining) return false;

            // 원본 크기 읽기
            uint32_t originalSize = 0;
            memcpy(&originalSize, block.data(), 4);

            const uint8_t* compData = block.data() + 4;
            size_t compSize = remaining - 4;

            outBuffer.resize(originalSize);

            // 메모리 상에서 압축 해제
            return DecodeLZSS(compData, compSize, outBuffer.data(), originalSize);
        }

        return false;
    }

    bool Decompress(const std::vector<uint8_t>& pakBuffer, const FileEntry* entry, std::vector<uint8_t>& outBuffer) {
        if (!entry || pakBuffer.empty()) return false;
        if (entry->DataOffset >= pakBuffer.size()) return false;

        outBuffer.clear();
        const uint8_t* pData = pakBuffer.data() + entry->DataOffset;
        size_t availableSize = pakBuffer.size() - entry->DataOffset;

        // Case A: Raw
        if (entry->Flags == 0) {
            if (entry->DataSize > availableSize) return false;
            outBuffer.assign(pData, pData + entry->DataSize);
            return true;
        }

        // Case B: LZSS Compressed
        if (entry->Flags == 1) {
            if (availableSize < 4) return false;
            uint32_t blockSize = *(uint32_t*)pData;
            pData += 4; availableSize -= 4;

            if (blockSize < 8 || availableSize < (size_t)(blockSize - 4)) return false;

            uint32_t originalSize = *(uint32_t*)pData;
            const uint8_t* compData = pData + 4;
            size_t compSize = blockSize - 8; // Header(4) already skipped, now skip OrigSize(4)

            outBuffer.resize(originalSize);
            return DecodeLZSS(compData, compSize, outBuffer.data(), originalSize);
        }

        return false;
    }

    // --- Compression Logic (Helper Class to avoid globals) ---
    class LZSSEncoder {
    public:
        LZSSEncoder() {
            // Initialize ring buffer with 0
            for (int i = 0; i < N; ++i) {
                window[i] = 0;
            }
        }

        size_t Encode(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst) {
            dst.clear();
            if (src.empty()) {
                return 0;
            }

            const size_t srcSize = src.size();
            size_t srcPos = 0;

            int r = 0; // write position in ring buffer

            // One flag byte + up to 3 bytes per token, 8 tokens per flag
            uint8_t codeBuf[1 + 3 * 8];
            uint8_t flags = 0;
            uint8_t mask = 1;
            int codePos = 1; // next write position in codeBuf
            int filled = 0; // number of valid bytes in window

            while (srcPos < srcSize) {
                int bestLen = 0;
                int bestOffset = 0;

                const size_t remain = srcSize - srcPos;
                const size_t maxMatchLen = (remain < 273) ? remain : 273; // 3..18

                // Find best match in dictionary (brute-force search)
                if (maxMatchLen >= (THRESHOLD + 1)) { // at least 3
                    for (int offset = 0; offset < filled; ++offset) {
                        int len = 0;
                        while (len < static_cast<int>(maxMatchLen)) {
                            uint16_t c1 = window[(offset + len) % filled];
                            uint8_t c2 = src[srcPos + static_cast<size_t>(len)];
                            if (c1 != c2) {
                                break;
                            }
                            ++len;
                        }

                        if (len > bestLen) {
                            bestLen = len;
                            bestOffset = offset;
                            if (len == static_cast<int>(maxMatchLen)) {
                                break;
                            }
                        }
                    }
                }

                if (bestLen >= (THRESHOLD + 1)) {
                    // Encode as match
                    int length = bestLen; // 3..273

                    // 12-bit offset
                    uint16_t off = static_cast<uint16_t>(bestOffset & 0x0FFF);
                    uint8_t  low4 = static_cast<uint8_t>(off & 0x0F);   // bits 3..0
                    uint8_t  high8 = static_cast<uint8_t>(off >> 4);     // bits 11..4

                    if (length <= 17) {
                        // Normal length: 3..17
                        // Decoder:
                        //   j = (j & 0x0F) + THRESHOLD;
                        //   // no extra byte
                        //   length = j + 1;
                        int lenNibble = length - 3; // 0..14
                        uint8_t jByte = static_cast<uint8_t>((low4 << 4) | (lenNibble & 0x0F));

                        // flags bit = 0 (match), so we do not set it here
                        codeBuf[codePos++] = jByte;
                        codeBuf[codePos++] = high8;
                    }
                    else {
                        // Extended length: 18..273
                        // Decoder:
                        //   j = (j & 0x0F) + THRESHOLD;      // j == 0x0F + THRESHOLD == 17
                        //   if (j == 0x0F + THRESHOLD) {
                        //       j += extra;
                        //   }
                        //   length = j + 1 = 18 + extra
                        int     lenNibble = 0x0F;
                        uint8_t jByte = static_cast<uint8_t>((low4 << 4) | lenNibble);
                        uint8_t extra = static_cast<uint8_t>(length - 18); // 0..255

                        codeBuf[codePos++] = jByte;
                        codeBuf[codePos++] = high8;
                        codeBuf[codePos++] = extra;
                    }

                    // Update dictionary with matched bytes
                    for (int k = 0; k < length; ++k) {
                        uint8_t c = src[srcPos++];
                        window[r] = c;
                        r = (r + 1) & (N - 1);
                        if (filled < N) ++filled;
                    }
                }
                else {
                    // Encode literal
                    uint8_t c = src[srcPos++];

                    // Literal => set flag bit
                    flags |= mask;
                    codeBuf[codePos++] = c;

                    // Update dictionary
                    window[r] = c;
                    r = (r + 1) & (N - 1);
                    if (filled < N) ++filled;
                }

                // Advance mask and flush every 8 tokens
                mask = static_cast<uint8_t>(mask << 1);
                if (mask == 0) {
                    codeBuf[0] = flags;
                    dst.insert(dst.end(), codeBuf, codeBuf + codePos);

                    flags = 0;
                    mask = 1;
                    codePos = 1;
                }
            }

            // Flush remaining tokens
            if (codePos > 1) {
                codeBuf[0] = flags;
                dst.insert(dst.end(), codeBuf, codeBuf + codePos);
            }

            return dst.size();
        }

    private:
        uint8_t window[N];
    };

    size_t Compress(const std::vector<uint8_t>& src, std::vector<uint8_t>& dest) {
        dest.clear();
        if (src.empty()) {
            return 0;
        }

        LZSSEncoder encoder;
        size_t compSize = encoder.Encode(src, dest);

        // If compression is not effective, store raw instead
        if (compSize == 0 || compSize >= src.size()) {
            dest.clear();
            return 0;
        }

        return compSize;
    }
}
