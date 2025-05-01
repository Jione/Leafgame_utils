#include "BinaryTools.h"

// round float to int
int binutil::round(float fnum) { return int(fnum + 0.5); }

// round-up float to int
int binutil::roundUp(float fnum) { return int(fnum + 0.75); }

// round-Down float to int
int binutil::roundDown(float fnum) { return int(fnum + 0.25); }

// calulate padding length
int binutil::getPaddlen(int target_length, int zerofill_length)
{
	return (zerofill_length - (target_length % zerofill_length)) & (zerofill_length - 1);
}

// write zero to filestream
void binutil::fileZerofill(FILE* file, size_t numBytes) {
	if (numBytes > 0) memset(zero_buffer, 0, 1024);
	if (numBytes >= 1024) {
		for (int i = (numBytes >> 10); i > 0; i--)
		{
			fwrite(zero_buffer, 1, 1024, file);
		}
	}
	if ((numBytes % 1024) > 0) fwrite(zero_buffer, 1, numBytes % 1024, file);
}

/***********************************************************
******************** x86 assembly code *********************
// Change BigEndian <-> LittleEndian for WORD (DWORD Type)
void binutil::endianConvWord(DWORD& word_val)
{
    __asm {
        PUSH EBP
        MOV EBP, word_val
        XCHG WORD PTR DS : [EBP] , AX
        XCHG AH, AL
        XCHG WORD PTR DS : [EBP] , AX
        POP EBP
    }
}

// Change BigEndian <-> LittleEndian for DWORD
void binutil::endianConvD(DWORD& dword_val)
{
    __asm {
        PUSH EBP
        MOV EBP, dword_val
        XCHG WORD PTR DS : [EBP + 2] , AX
        XCHG AH, AL
        XCHG WORD PTR DS : [EBP] , AX
        XCHG AH, AL
        XCHG WORD PTR DS : [EBP + 2] , AX
        POP EBP
    }
}

// Change BigEndian <-> LittleEndian for WORD
void binutil::endianConvW(WORD& word_val)
{
    binaryStructs bin_struct;
    bin_struct.dword_num = word_val;
    endianConvWord(bin_struct.dword_num);
    word_val = bin_struct.dword_num;
}

// Change BigEndian <-> LittleEndian for float
void binutil::endianConvF(float& float_val)
{
    binaryStructs bin_struct;
    bin_struct.float_num = float_val;
    memcpy(&bin_struct.dword_num, &bin_struct.float_num, 4);
    endianConvD(bin_struct.dword_num);
    memcpy(&bin_struct.float_num, &bin_struct.dword_num, 4);
    float_val = bin_struct.float_num;
}
***********************************************************/

// Change BigEndian <-> LittleEndian for DWORD
void binutil::endianConvD(DWORD& dword_val)
{
    bin_struct.dword_num = dword_val;
    memcpy(&bin_struct.char_buf[0], &bin_struct.dword_num, 4);
    bin_struct.char_buf[4] = bin_struct.char_buf[3];
    bin_struct.char_buf[5] = bin_struct.char_buf[2];
    bin_struct.char_buf[6] = bin_struct.char_buf[1];
    bin_struct.char_buf[7] = bin_struct.char_buf[0];
    memcpy(&bin_struct.dword_num, &bin_struct.char_buf[4], 4);
    dword_val = bin_struct.dword_num;
}

// Change BigEndian <-> LittleEndian for WORD
void binutil::endianConvW(WORD& word_val)
{
    bin_struct.dword_num = word_val;
    memcpy(&bin_struct.char_buf[0], &bin_struct.dword_num, 4);
    bin_struct.char_buf[4] = bin_struct.char_buf[1];
    bin_struct.char_buf[5] = bin_struct.char_buf[0];
    memcpy(&bin_struct.dword_num, &bin_struct.char_buf[4], 4);
    word_val = bin_struct.dword_num;
}

// Change BigEndian <-> LittleEndian for float
void binutil::endianConvF(float& float_val)
{
    bin_struct.float_num = float_val;
    memcpy(&bin_struct.dword_num, &bin_struct.float_num, 4);
    memcpy(&bin_struct.char_buf[0], &bin_struct.dword_num, 4);
    bin_struct.char_buf[4] = bin_struct.char_buf[3];
    bin_struct.char_buf[5] = bin_struct.char_buf[2];
    bin_struct.char_buf[6] = bin_struct.char_buf[1];
    bin_struct.char_buf[7] = bin_struct.char_buf[0];
    memcpy(&bin_struct.dword_num, &bin_struct.char_buf[4], 4);
    memcpy(&bin_struct.float_num, &bin_struct.dword_num, 4);
    float_val = bin_struct.float_num;
}

// Convert BGRA <-> RGBA array
void binutil::convertRGBAtoBGRA(BYTE*& raw_data, int raw_len)
{
    BYTE buffer = 0;
    for (int i = (raw_len >> 2) - 1; i >= 0; i--)
    {
        buffer = raw_data[i * 4 + 2];
        raw_data[i * 4 + 2] = raw_data[i * 4];
        raw_data[i * 4] = buffer;
    }
}

// Flip raw image up and down
void binutil::flipRawImage(BYTE*& raw_data, int raw_len, int width)
{
    DWORD read_size = width * 4, d_buff;
    int height = raw_len / read_size;
    BYTE* buffer = new BYTE[read_size];
    memset(buffer, 0, read_size);

    if (raw_len % (width * 4) != 0)
    {
        d_buff = read_size * ++height;
        BYTE* rawbuffer = new BYTE[d_buff];
        memset(rawbuffer, 0, d_buff);
        memcpy(rawbuffer, raw_data, raw_len);
        raw_len = d_buff;
        delete[]raw_data;
        raw_data = rawbuffer;
    }

    height--;
    for (int i = 0; height > i; i++)
    {
        memcpy(buffer, &raw_data[read_size * i], read_size);
        memcpy(&raw_data[read_size * i], &raw_data[read_size * height], read_size);
        memcpy(&raw_data[read_size * height--], buffer, read_size);
    }
    delete[]buffer;
}

// Create fixed color-table that is random calc based on fixed variables
DWORD* binutil::genRGBATable(int expr)
{
    DWORD* rgba = new DWORD[expr];
    BYTE bit[16] = { 15, 9, 4, 14, 8, 3, 13, 7, 2, 12, 6, 1, 11, 5, 0, 10 };
    BYTE rh, gh, bh, rl, gl, bl, r, g, b;
    rh = 14; rl = 15; gh = 0; gl = 3; bh = 0; bl = 13;
    for (int i = 0; i < expr; i++)
    {
        if (i % 16 == 0) {
            bh++;
            if (i % 256 == 0) {
                gh++;
                if (i % 4096 == 0) {
                    rh++; bl++;
                    if (i % 65536 == 0) {
                        bh++; gl++;
                        if (i % 1048576 == 0) { rh++; rl++; }
                    }
                }
            }
        }
        rh %= 16; rl %= 16; gh %= 16; gl %= 16; bh %= 16; bl %= 16;
        r = (bit[rh++] << 4) + bit[rl++];
        g = (bit[gh++] << 4) + bit[gl++];
        b = (bit[bh++] << 4) + bit[bl++];
        rgba[i] = 0xFF000000 + (b << 16) + (g << 8) + r;
    }

    return rgba;
}