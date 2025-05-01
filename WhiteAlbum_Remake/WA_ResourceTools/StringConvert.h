#pragma warning(disable:4996)

#ifndef FILE_STRINGCONV_H
#define FILE_STRINGCONV_H
#include <Windows.h>
#include <stdio.h>
#include <string>
#include <regex>
#include <map>

/*#ifdef __cpp_lib_filesystem
#include <filesystem>
namespace fs = std::filesystem;
#elif __cpp_lib_experimental_filesystem*/
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING 1;
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
/*#endif*/

using namespace std;

class strconv {
private:
	typedef struct Lang_Table_Vector {
		map<string, string> SJIS_to_UTF8;
		map<string, string> UTF8_to_SJIS;
		vector<string> target_sjis;
		vector<string> convert_utf8;
	} lang_table_vector;

	unsigned int SJIS_CP = 932;
	unsigned int EUCKR_CP = 949;

	lang_table_vector lang_vec;
	string lastLangTable;
	string tbl_filename;

	void makeConvertTable(string table_filename, int CP_TYPE);

public:
	string trimStart(string target, const string remove_string);
	string trimEnd(string target, const string remove_string);
	string toLower(const string& input_string);
	string toUpper(const string& input_string);
	string toUpperFirst(const string& input_string);
	void mkdir(const string directoty_name);
	int pathChecker(const string path);
	string getDir(const string path);
	string removeLastslushDir(const string path);
	string getName(const string path);
	string getFilename(const string path);
	string getFilename2(const string path);
	string getExtension(const string path);
	string getExtension2(const string path);
	string removeUnderbar(const string filename);
	string replaceFilename(const string path, const string extension);
	string replaceExtension(const string path, const string extension);
	unsigned long getTotalsize(string folder, vector<string>& vector_path);
	bool isWABGFile(string path);
	int wildcardVectorSearch(vector<string>& wildcard_vector, const string& search_string);
	void setTableFile(const string& filename);
	int u8len(string& utf8_string);
	vector<string> u8chars(string& utf8_string);
	string u8charFirst(const string& str);
	char* getUTF8Char(wchar_t wchar_value, int CP_TYPE);
	string changeUTF8Lang(string utf8_string, bool isReverse);
	string convertUTF8SJISEndian(const string& source_string, bool isShiftJIS, bool useTBLFile);
	bool checkSJIS(const string& utf8_string);
};
#endif