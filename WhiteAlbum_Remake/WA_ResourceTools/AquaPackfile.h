#pragma warning(disable:4996)

#ifndef AQUAPLUS_PACKFILE_H
#define AQUAPLUS_PACKFILE_H
#include <Windows.h>
#include <stdio.h>
#include <string>
#include "StringConvert.h"
#include "BinaryTools.h"
#include "mainMsg.h"

using namespace std;

class AquaPack {
private:
	typedef struct PCK_Filename_Header {
		char sig[0x14]; // "Filename"
		DWORD length;
	} pck_name_struct;

	typedef struct PCK_Pack_Header {
		char sig[0x14]; // "Pack"
		DWORD index_size;
		DWORD part_count;
	} pck_pack_struct;

	typedef struct DMM_PCK_Filename_Header {
		char sig[0x8]; // "Filename"
		DWORD length;
	} dmm_pck_name_struct;

	typedef struct DMM_PCK_Pack_Header {
		char sig[0x8]; // "Pack"
		DWORD index_size;
		DWORD part_count;
	} dmm_pck_pack_struct;

	vector<string> allowedExtensions = {
		".amb", ".txt", ".exl", ".lz7", ".bin", ".tex",
		".voice", ".fnt", ".env", ".bgm", ".se", ".pck", ".flag"
	};

	strconv str;
	binutil bin;
	customMessage msg;

public:
	int pckUnpackFile(string input_filename, int echo_option);
	int pckRepackFile(string input_foldername, int pack_echo_opt);
};

#endif