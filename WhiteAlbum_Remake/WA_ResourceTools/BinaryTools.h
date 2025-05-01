#ifndef FILE_BINARY_TOOLS_H
#define FILE_BINARY_TOOLS_H
#include <Windows.h>
#include <stdio.h>
#include <string>

using namespace std;

class binutil {
private:
	typedef struct binaryStructs
	{
		float float_num;
		DWORD dword_num;
		char char_buf[8];
	};
	binaryStructs bin_struct{0, 0, ""};
	void endianConvWord(DWORD& word_val);
	BYTE* zero_buffer = new BYTE[1024];

public:
	int round(float float_number);
	int roundUp(float float_number);
	int roundDown(float float_number);
	int getPaddlen(int length, int max_paddlen);
	void fileZerofill(FILE* filestream, size_t length);
	void endianConvD(DWORD& dword_ptr);
	void endianConvW(WORD& word_ptr);
	void endianConvF(float& float_ptr);
	void convertRGBAtoBGRA(BYTE*& rgba_array, int array_length);
	void flipRawImage(BYTE*& rgba_array, int array_length, int width);
	DWORD* genRGBATable(int gen_length);
};
#endif