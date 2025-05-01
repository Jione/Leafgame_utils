#include "lzss.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdint>

// 압축 설정 상수
static const int historyBufferSize = 4096;
static const int maxMatchLength = 18;
static const int threshold = 2;
static const int nil = historyBufferSize;

// 내부 상태
static unsigned long int textsize = 0;
static unsigned long int printcount = 0;
static unsigned char text_buf[historyBufferSize + maxMatchLength - 1];
static int match_position, match_length;
static int leftChild[historyBufferSize + 1], rightChild[historyBufferSize + 257], parent[historyBufferSize + 1];

// ------------------------------------
// 압축 (Encode)
// ------------------------------------

static std::ifstream inFile;
static std::ostream* outStream = nullptr;

// 트리 초기화
static void InitTree() {
    for (int i = historyBufferSize + 1; i <= historyBufferSize + 256; i++)
        rightChild[i] = nil;
    for (int i = 0; i < historyBufferSize; i++)
        parent[i] = nil;
}

// 삽입
static void InsertNode(int r) {
    int i, p, cmp;
    unsigned char* key = &text_buf[r];
    cmp = 1;
    p = historyBufferSize + 1 + key[0];
    rightChild[r] = leftChild[r] = nil;
    match_length = 0;

    for (;;) {
        if (cmp >= 0) {
            if (rightChild[p] != nil) p = rightChild[p];
            else { rightChild[p] = r; parent[r] = p; return; }
        }
        else {
            if (leftChild[p] != nil) p = leftChild[p];
            else { leftChild[p] = r; parent[r] = p; return; }
        }

        for (i = 1; i < maxMatchLength; i++) {
            if ((cmp = key[i] - text_buf[p + i]) != 0)
                break;
        }

        if (i > match_length) {
            match_position = p;
            if ((match_length = i) >= maxMatchLength)
                break;
        }
    }

    parent[r] = parent[p];
    leftChild[r] = leftChild[p];
    rightChild[r] = rightChild[p];
    parent[leftChild[p]] = r;
    parent[rightChild[p]] = r;

    if (rightChild[parent[p]] == p) rightChild[parent[p]] = r;
    else leftChild[parent[p]] = r;

    parent[p] = nil;
}

// 삭제
static void DeleteNode(int p) {
    int q;
    if (parent[p] == nil) return;

    if (rightChild[p] == nil) q = leftChild[p];
    else if (leftChild[p] == nil) q = rightChild[p];
    else {
        q = leftChild[p];
        if (rightChild[q] != nil) {
            do { q = rightChild[q]; } while (rightChild[q] != nil);
            rightChild[parent[q]] = leftChild[q];
            parent[leftChild[q]] = parent[q];
            leftChild[q] = leftChild[p];
            parent[leftChild[p]] = q;
        }
        rightChild[q] = rightChild[p];
        parent[rightChild[p]] = q;
    }

    parent[q] = parent[p];
    if (rightChild[parent[p]] == p) rightChild[parent[p]] = q;
    else leftChild[parent[p]] = q;

    parent[p] = nil;
}

// 압축 엔진 (Encode)
static void Encode() {
    int i, c, len, r, s, last_match_length, code_buf_ptr;
    unsigned char code_buf[17], mask;

    InitTree();
    code_buf[0] = 0;
    code_buf_ptr = mask = 1;
    s = 0;
    r = historyBufferSize - maxMatchLength;

    for (i = s; i < r; i++) text_buf[i] = ' ';
    for (len = 0; len < maxMatchLength && (c = inFile.get()) != EOF; len++)
        text_buf[r + len] = c;

    if ((textsize = len) == 0) return;

    for (i = 1; i <= maxMatchLength; i++) InsertNode(r - i);
    InsertNode(r);

    do {
        if (match_length > len) match_length = len;

        if (match_length <= threshold) {
            match_length = 1;
            code_buf[0] |= mask;
            code_buf[code_buf_ptr++] = text_buf[r];
        }
        else {
            code_buf[code_buf_ptr++] = (unsigned char)match_position;
            code_buf[code_buf_ptr++] = (unsigned char)(((match_position >> 4) & 0xf0) | (match_length - (threshold + 1)));
        }

        if ((mask <<= 1) == 0) {
            for (i = 0; i < code_buf_ptr; i++) outStream->put(code_buf[i]);
            code_buf[0] = 0;
            code_buf_ptr = mask = 1;
        }

        last_match_length = match_length;
        for (i = 0; i < last_match_length && (c = inFile.get()) != EOF; i++) {
            DeleteNode(s);
            text_buf[s] = c;
            if (s < maxMatchLength - 1) text_buf[s + historyBufferSize] = c;
            s = (s + 1) & (historyBufferSize - 1);
            r = (r + 1) & (historyBufferSize - 1);
            InsertNode(r);
        }

        textsize += i;
        while (i++ < last_match_length) {
            DeleteNode(s);
            s = (s + 1) & (historyBufferSize - 1);
            r = (r + 1) & (historyBufferSize - 1);
            if (--len) InsertNode(r);
        }
    } while (len > 0);

    if (code_buf_ptr > 1)
        for (i = 0; i < code_buf_ptr; i++)
            outStream->put(code_buf[i]);
}

// 외부 노출 함수: 파일을 압축하여 outputStream에 기록
bool lzss_compress_file_to_stream(const std::string& inputFilePath, std::ostream& outputStream) {
    std::ifstream input(inputFilePath, std::ios::binary);
    if (!input.is_open()) {
        std::cerr << "[Error] Cannot open: " << inputFilePath << "\n";
        return false;
    }

    // 압축 결과를 메모리 버퍼에 먼저 기록
    std::ostringstream compressedData(std::ios::binary);
    inFile = std::move(input);
    outStream = &compressedData;

    Encode();

    inFile.close();
    outStream = nullptr;

    std::string lzssBytes = compressedData.str();
    uint32_t compressedTotalSize = static_cast<uint32_t>(lzssBytes.size()) + 8;

    std::ifstream original(inputFilePath, std::ios::binary | std::ios::ate);
    uint32_t originalSize = static_cast<uint32_t>(original.tellg());
    original.close();

    outputStream.write(reinterpret_cast<const char*>(&compressedTotalSize), sizeof(uint32_t));
    outputStream.write(reinterpret_cast<const char*>(&originalSize), sizeof(uint32_t));
    outputStream.write(lzssBytes.data(), lzssBytes.size());

    return outputStream.good();
}

// ------------------------------------
// 압축 해제 (Decode)
// ------------------------------------

bool lzss_decompress_stream(std::istream& inputStream, std::ostream& outputStream) {
    uint32_t compressedSize = 0;
    uint32_t originalSize = 0;
    inputStream.read(reinterpret_cast<char*>(&compressedSize), sizeof(uint32_t));
    inputStream.read(reinterpret_cast<char*>(&originalSize), sizeof(uint32_t));

    if (compressedSize < 8 || originalSize == 0) {
        std::cerr << "[Error] Invalid LZSS header.\n";
        return false;
    }

    int i, j, k, r, c;
    unsigned int flags;

    for (i = 0; i < historyBufferSize - maxMatchLength; i++)
        text_buf[i] = ' ';
    r = historyBufferSize - maxMatchLength;
    flags = 0;

    size_t totalWritten = 0;
    while (totalWritten < originalSize) {
        if (((flags >>= 1) & 256) == 0) {
            if ((c = inputStream.get()) == EOF) break;
            flags = c | 0xFF00;
        }

        if (flags & 1) {
            if ((c = inputStream.get()) == EOF) break;
            outputStream.put((char)c);
            text_buf[r++] = c;
            r &= (historyBufferSize - 1);
            totalWritten++;
        }
        else {
            if ((i = inputStream.get()) == EOF) break;
            if ((j = inputStream.get()) == EOF) break;
            i |= ((j & 0xF0) << 4);
            j = (j & 0x0F) + threshold;
            for (k = 0; k <= j; k++) {
                c = text_buf[(i + k) & (historyBufferSize - 1)];
                outputStream.put((char)c);
                text_buf[r++] = c;
                r &= (historyBufferSize - 1);
                totalWritten++;
            }
        }
    }

    return totalWritten == originalSize;
}