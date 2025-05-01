#pragma warning(disable:4996)

#ifndef AQUAPLUS_SCRIPTS_H
#define AQUAPLUS_SCRIPTS_H
#include <Windows.h>
#include <stdio.h>
#include <string>
#include "StringConvert.h"
#include "BinaryTools.h"
#include "CSVFile.h"
#include "mainMsg.h"

using namespace std;

class AquaScripts {
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

	typedef struct DMM_Lang_Vector {
		vector<DWORD> target_sjis;
		vector<DWORD> convert_utf8;
	} dmm_lang_vector;

    strconv str;
    binutil bin;
	CSVStream csv;
	customMessage msg;

    void convertMes2utf8(string scriptname, BYTE*& raw_data, DWORD& raw_len, csv_struct& csv_data, int& row, int& col, bool isDMMFile, bool isCustom);
    void updateMesStrings(string scriptname, BYTE*& raw_data, DWORD& raw_len, map<string, map<int, string>>& csvMap, bool toDMMFile, bool isConv, bool isCustom);
	
public:
	int  convertSdat2Csv(string in_filename, int comp_opt, string tbl_filename);
	int  convertCsv2Sdat(string in_filename, int comp_opt, string tbl_filename);
};

#endif