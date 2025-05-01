#ifndef AQUAPLUS_FONTS_H
#define AQUAPLUS_FONTS_H
#include <Windows.h>
#include <stdio.h>
#include <string>
#include "CSVFile.h"
#include "pngfile.h"
#include "StringConvert.h"
#include "BinaryTools.h"
#include "mainMsg.h"

using namespace std;

class AquaFonts {
private:
	typedef struct Font_Table_Struct {
		char sig[0x14]; // "Table"
		DWORD header_len;
		DWORD size;
		DWORD count;
	} table_struct;

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

	typedef struct DMM_Font_Table_Struct {
		char sig[0x8]; // "Table"
		DWORD header_len;
		DWORD size;
		DWORD count;
	} dmm_table_struct;

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

	typedef struct Font_Array_Struct
	{
		char sig[4];
		float start_x;
		float start_y;
	} font_struct;

	pngSteamTool png;
	strconv str;
	binutil bin;
	customMessage msg;
	CSVStream csv;

	string dwordToUTF8(char* string, bool isBigEndian);
	void utf8ToDWORD(char*& chr_array, bool isBigEndian);
	void writeFonttable2File(FILE* out_file, csv_struct csv_data, pic_data png_data, bool isDMMFile);

public:
	int convertFnt2Png_csv(string input_filename, int echo_option);
	int convertPng_csv2Fnt(string in_filename, int comp_opt);
};
#endif