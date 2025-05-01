#ifndef AQUAPLUS_LZ77_H
#define AQUAPLUS_LZ77_H
#include <Windows.h>
#include <stdio.h>

typedef struct LZ77_Struct
{
	char sig[4]; //LZ77
	DWORD total_len;
	DWORD flag_count;
	DWORD data_offset;
	unsigned char* compress_data;
	int compress_size;
} lz77_struct;

class LZ77 {
private:
	int window = 0xFF; // Window size
	int kmp_min = 2; // Minimum Match length - 1
	int kmp_max = 0xFF + kmp_min + 1; // Maximum Match length
	int kmp_best_pos = 0;
	int kmp_best_len = 0;
	WORD kmp_table[0x102]{ 0 }; // kmp_table[kmp_max]
	void kmp_match(BYTE*& raw_data, int& raw_size, int& read_position, bool& compress_mode);
	void NoCompress(BYTE* raw_data, int raw_size, lz77_struct* output_struct);

public:
	int lz77_hdrsize = 0x10;
	void Compress(BYTE* raw_data, int raw_size, lz77_struct* output_struct, int compress_mode);
	void Decompress(lz77_struct* input_struct, BYTE*& out_data);
};

#endif