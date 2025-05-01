#ifndef AQUAPLUS_TEXTURE_H
#define AQUAPLUS_TEXTURE_H
#include <Windows.h>
#include <stdio.h>
#include <string>
#include "pngfile.h"
#include "BinaryTools.h"
#include "StringConvert.h"
#include "mainMsg.h"
#include "AquaLZ77.h"

using namespace std;

class AquaTexture {
private:
	typedef struct Texture_Header_Struct
	{
		char sig[0x14]; //Texture
		DWORD data_size;
		WORD unknown_0; //0x4000,	maybe version or bit_depth...
		WORD unknown_1; //0x0,		maybe version or bit_depth...
		WORD unknown_2; //0x0,		maybe version or bit_depth...
		WORD unknown_3; //0x810		maybe version or bit_depth...
		DWORD content_size;
		WORD width;
		WORD height;
	} texture_struct;

	typedef struct Parts_Struct {
		char sig[0x14]; // "Parts", "Anime"
		DWORD total_len;
		DWORD count;
	} parts_struct, anime_struct;

	typedef struct Parts_float_Struct {
		float target_width;
		float target_height;
		float width;
		float height;
		float start_x;
		float start_y;
		float end_x;
		float end_y;
	} parts_float;

	typedef struct Anime_dword_Struct {
		DWORD value1;
		DWORD value2;
		DWORD value3;
		DWORD value4;
		DWORD value5;
	} anime_dword;

	typedef struct DMM_Texture_Header_Struct
	{
		char sig[0x8]; //Texture
		DWORD data_size;
		DWORD unknown; //0x8,	maybe version or bit_depth...
		DWORD content_size;
		DWORD target_width;
		DWORD target_height;
		DWORD width;
		DWORD height;
	} dmm_texture_struct;

	typedef struct DMM_Parts_Struct {
		//Texture size = 0x28
		char sig[0x8]; // "Parts"
		DWORD total_len;
		DWORD count;
	} dmm_parts_struct;

	typedef struct DMM_Parts_float_Struct {
		float width;
		float height;
		float start_x;
		float start_y;
		float end_x;
		float end_y;
	} dmm_parts_float;

	int tex_infosize = 0x10;
	int dmm_tex_infosize = 0x18;

	strconv str;
	pngSteamTool png;
	customMessage msg;
	binutil bin;
	LZ77 lz77;

	int makePartsPNGFile(string png_filename, BYTE* raw_data, int raw_size, pic_data png_data, bool isDMMFile);
	int writeParts2File(FILE* out_file, string parts_filename, pic_data png_data, bool toDMMFlag);

public:
	int convertTexLZ77(string input_filename, int compress_option);
	int convertPngLZ77(string input_filename, int compress_option);
	int updatePartsfile(string input_filename, int echo_option);
};
#endif