#include "StringConvert.h"

// Trim start string
string strconv::trimStart(string str, const string delims)
{
	str.erase(0, str.find_first_not_of(delims));
	return str;
}

// Trim end string
string strconv::trimEnd(string str, const string delims)
{
	str.erase(str.find_last_not_of(delims) + 1);
	return str;
}

// string to lowercase
string strconv::toLower(const string& input) {
	string result = input;
	for (char& chr : result) chr = tolower(chr);
	return result;
}

// string to uppercase
string strconv::toUpper(const string& input) {
	string result = input;
	for (char& chr : result) chr = toupper(chr);
	return result;
}

// tolower and uppercase first char
string strconv::toUpperFirst(const string& input) {
	string result = input;
	bool first_run = true;
	for (char& chr : result)
	{
		if (!first_run) chr = tolower(chr);
		else {
			first_run = false;
			chr = toupper(chr);
		}
	}
	return result;
}

// make directory
void strconv::mkdir(const string path) {
	fs::path p(path);

	if (fs::is_directory(p))
		return;

	fs::create_directories(p);
}

/*
Check path type
-1: is not exist, 0: File, 1: Folder
*/
int strconv::pathChecker(const string path) {
	fs::path p(path);
	if (fs::exists(p))
	{
		if (fs::is_directory(p))
			return 1;
		else if (fs::is_regular_file(p))
			return 0;
		else return -1;
	}
	else return -1;
}

// get directory string
string strconv::getDir(const string path) {
	fs::path p(path);
	return canonical(p).parent_path().string();
}

// remove dir string last slushchar
string strconv::removeLastslushDir(const string path) {
	return trimEnd(path, "\\");
}

// get filename [struct: <dir><filename><extension>]
string strconv::getName(const string path) {
	fs::path p(path);
	return canonical(p).filename().string();
}

// get filename [struct: <dir><filename><extension>]
string strconv::getFilename(const string path) {
	fs::path p(path);
	if (fs::is_regular_file(p))
	{
		string fname = canonical(p).filename().string();
		return fname.substr(0, fname.find_last_of("."));
	}
	else
	{
		string fname = canonical(p).filename().string();
		fname = fname.substr(0, fname.find_last_of("."));
		if (fname.length() == 0)
			return canonical(p).filename().string();
		else return fname;
	}
}

// get double extension [struct: <dir><filename><extension>]
string strconv::getFilename2(const string path) {
	return getFilename(getFilename(path));
}

// get extension [struct: <dir><filename><extension>]
string strconv::getExtension(const string path) {
	fs::path p(path);
	return canonical(p).extension().string();
}

// get double extension [struct: <dir><filename><extension>]
string strconv::getExtension2(const string path) {
	return getExtension(getFilename(path)) + getExtension(path);
}

// remove underbar filename ("_filename" -> "filename")
string strconv::removeUnderbar(const string filename) {
	return trimStart(filename, "_");
}

// add underbar and replace extension
string strconv::replaceFilename(const string filename, const string fileext)
{
	string replace_name = getDir(filename) + "\\_" + removeUnderbar(getFilename(filename)) + fileext;
	return replace_name;
}

// only replace extension
string strconv::replaceExtension(const string filename, const string fileext)
{
	string replace_name = getDir(filename) + "\\" + getFilename(filename) + fileext;
	return replace_name;
}

// Check Total size of vector string files
unsigned long strconv::getTotalsize(string folder, vector<string>& path_vec) {
	unsigned long total_size = 0;
	for (string str : path_vec) total_size += fs::file_size(fs::path(folder + "\\" + str));
	return total_size;
}

// check WhiteAlbum BG file
bool strconv::isWABGFile(string filepath)
{
	regex reg_str("[sS][0-9]{4}|[vV][0-9]{5}");
	return regex_search(getFilename(filepath), reg_str);
}

// Check index value in find_str based on vectors with wildcard strings
int strconv::wildcardVectorSearch(vector<string>& str_vec, const string& find_str) {
	int index = 0;
	regex reg_str;
	for (string search_str : str_vec)
	{
		reg_str = search_str;
		if (regex_search(find_str, reg_str)) return index;
		index++;
	}
	return -1;
}

void strconv::setTableFile(const string& filename)
{
	tbl_filename = filename;
}

// Get utf8 string length
int strconv::u8len(string& str) {
	int len = 0, i, j;
	char* str_chr, chr;
	j = strlen(str.c_str());
	str_chr = new char[j];
	memcpy(str_chr, &str[0], j);
	for (i = 0; i < j; i++)
	{
		chr = str_chr[i];
		if ((chr != 0) && ((chr & 0xC0) != 0x80)) len++;
	}
	delete[]str_chr;
	return len;
}

// Separate utf8 string into utf8 character vector
vector<string> strconv::u8chars(string& str) {
	int i = 0, j = 0, k, len;
	char str_buff[0x10]{ 0 };
	char* str_chr, chr;
	len = strlen(str.c_str());
	str_chr = new char[len];
	memcpy(str_chr, &str[0], len);
	vector<string> str_char;

	for (k = 0; k < len; k++)
	{
		chr = str_chr[k];
		if ((i == 0) && (j == 0))
		{
			str_buff[j++] = chr;
		}
		else if ((chr & 0xC0) == 0x80)
		{
			str_buff[j++] = chr;

		}
		else
		{
			str_char.push_back((const char*)&str_buff[0]);
			j = 0;
			memset(&str_buff[0], 0, 0x10);
			str_buff[j++] = chr;
		}
	}
	if (j != 0) str_char.push_back((const char*)&str_buff[0]);
	delete[]str_chr;

	return str_char;
}

// first utf8 string into utf8 character vector
string strconv::u8charFirst(const string& str) {
	int i = 0, j = 0;
	char str_buff[5]{ 0 };
	string f_str;
	for (unsigned char chr : str)
	{
		if (i == 0)
			str_buff[j++] = chr;
		else if (((chr & 0x80) != 0) && ((chr & 0xC0) != 0x40))
			str_buff[j++] = chr;
		else
		{
			f_str = str_buff;
			j = 0;
			break;
		}
	}
	if (j != 0) f_str = str_buff;
	return f_str;
}

char* strconv::getUTF8Char(wchar_t wchar_value, int CP_TYPE)
{
	unsigned int i;
	string wchar_string = "  ";
	const string& char_string = wchar_string;
	char* utf8_string = nullptr;
	LPCCH wchar_ptr;
	LPWSTR utf16_ptr;
	LPSTR utf8_ptr;

	memcpy(&wchar_string, &wchar_value, sizeof(wchar_t));
	i = 0;

	if (CP_TYPE < 0)
	{
		utf16_ptr = new WCHAR[2]{ wchar_value , 0 };
	}
	else
	{
		wchar_ptr = (LPCCH)char_string.c_str();
		i = MultiByteToWideChar(CP_TYPE, 0, wchar_ptr, -1, 0, 0);
		utf16_ptr = new WCHAR[i];
		MultiByteToWideChar(CP_TYPE, 0, wchar_ptr, -1, utf16_ptr, i);
	}
	i = WideCharToMultiByte(CP_UTF8, 0, utf16_ptr, -1, 0, 0, 0, 0);
	utf8_ptr = new char[i + 16];
	ZeroMemory(utf8_ptr, i + 16);
	WideCharToMultiByte(CP_UTF8, 0, utf16_ptr, -1, utf8_ptr, i, 0, 0);
	utf8_string = utf8_ptr;
	return utf8_string;
}

void strconv::makeConvertTable(string table_filename, int CP_TYPE)
{
	FILE* table_file;
	char read_buff[0x20]{ 0 };
	wchar_t wchar_buff;
	int char_buff, i;
	bool isUTF8, isBE;

	isUTF8 = true;
	isBE = false;
	i = 0;

	// open table file
	table_file = fopen(table_filename.c_str(), "rb");
	if (!table_file) return;

	// check table filetype
	switch (char_buff = fgetc(table_file))
	{
	case 0xFF:
		if (fgetc(table_file) == 0xFE)
			isUTF8 = false;
		else fseek(table_file, 0, SEEK_SET);
		break;
	case 0xFE:
		if (fgetc(table_file) == 0xFF)
		{
			isUTF8 = false;
			isBE = true;
		}
		else fseek(table_file, 0, SEEK_SET);
		break;
	case 0xEF:
		if ((fgetc(table_file) == 0xBB) && (fgetc(table_file) == 0xBF))
			break;
		else fseek(table_file, 0, SEEK_SET);
	}

	while ((char_buff = fgetc(table_file)) != EOF)
	{
		if (isBE)
			wchar_buff = char_buff = (char_buff << 8) + fgetc(table_file);
		else if (!isUTF8)
			wchar_buff = char_buff += (fgetc(table_file) << 8);

		switch (char_buff)
		{
		case 0xA:
			if (read_buff != 0)
				lang_vec.convert_utf8.push_back(u8charFirst(read_buff));
			else 
				lang_vec.convert_utf8.push_back(" ");
			memset(read_buff, 0, 0x20);
			i = 0;
		case 0xD:
			break;
		case '=':
			wchar_buff = strtol(read_buff, NULL, 16);
			if (!((wchar_buff <= 0x80) || ((0xA0 <= wchar_buff) && (wchar_buff <= 0xDF)) || ((0xFD <= wchar_buff) && (wchar_buff <= 0xFF))))
				wchar_buff = char_buff = ((wchar_buff & 0xFF) << 8) + ((wchar_buff & 0xFF00) >> 8);
			lang_vec.target_sjis.push_back(getUTF8Char(wchar_buff, SJIS_CP));
			memset(read_buff, 0, 0x20);
			i = 0;
			break;
		default:
			if (!isUTF8)
				for (char chr : (string)getUTF8Char(wchar_buff, -1))
					read_buff[i++] = chr;
			else
				read_buff[i++] = char_buff;
		}
	}
	if (strlen(read_buff) != 0)
	{
		if (read_buff != 0)
			lang_vec.convert_utf8.push_back(u8charFirst(read_buff));
		else
			lang_vec.convert_utf8.push_back(" ");
	}
	
	// make map table
	i = 0;
	for (string utf : lang_vec.convert_utf8)
	{
		lang_vec.UTF8_to_SJIS[utf] = lang_vec.target_sjis[i];
		lang_vec.SJIS_to_UTF8[(lang_vec.target_sjis[i++])] = utf;
	}
	lang_vec.convert_utf8.clear();
	lang_vec.target_sjis.clear();
	lang_vec.convert_utf8.shrink_to_fit();
	lang_vec.target_sjis.shrink_to_fit();
}

string strconv::changeUTF8Lang(string utf8_string, bool isReverse)
{
	if (lastLangTable != tbl_filename)
	{
		makeConvertTable(tbl_filename, SJIS_CP);
		lastLangTable = tbl_filename;
	}
	int len;
	char* u8char;
	string u8str = utf8_string;
	vector<string> u8vec = u8chars(u8str);
	u8str = "";
	for (string u8str_char : u8vec)
	{
		if (isReverse)
		{
			if (lang_vec.UTF8_to_SJIS.find(u8str_char) == lang_vec.UTF8_to_SJIS.end())
				u8str += u8str_char;
			else
				u8str += lang_vec.UTF8_to_SJIS[u8str_char];
		}
		else
		{
			if (lang_vec.SJIS_to_UTF8.find(u8str_char) == lang_vec.SJIS_to_UTF8.end())
				u8str += u8str_char;
			else
				u8str += lang_vec.SJIS_to_UTF8[u8str_char];
		}
	}
	len = strlen(u8str.c_str());
	u8char = new char[++len];
	memset(u8char, 0, len--);
	memcpy(u8char, &u8str[0], len);
	u8vec.clear();
	u8vec.shrink_to_fit();
	return u8char;
}

string strconv::convertUTF8SJISEndian(const string& source_string, bool isShiftJIS, bool useTBLFile)
{
	unsigned int i, j;
	string in_string, out_string = "";
	LPCSTR source_ptr;
	LPWSTR utf16_ptr;
	LPSTR target_ptr;
	int sourceCP, targetCP;

	in_string = source_string;

	if (isShiftJIS)
	{
		sourceCP = SJIS_CP;
		targetCP = CP_UTF8;
	}
	else
	{
		sourceCP = CP_UTF8;
		targetCP = SJIS_CP;
		if (useTBLFile)
			in_string = changeUTF8Lang(source_string, true);
	}

	// Convert text endian
	i = j = 0;
	source_ptr = in_string.c_str();
	i = MultiByteToWideChar(sourceCP, 0, source_ptr, -1, 0, 0);
	if (i != 0)
	{
		utf16_ptr = new WCHAR[i * 2 + 2];
		if (MultiByteToWideChar(sourceCP, 0, source_ptr, -1, utf16_ptr, i) != 0)
		{
			j = WideCharToMultiByte(targetCP, 0, (LPCWSTR)utf16_ptr, -1, 0, 0, 0, 0);
			if (j != 0)
			{
				target_ptr = new char[j * 2 + 16];
				ZeroMemory(target_ptr, j * 2 + 16);
				if (WideCharToMultiByte(targetCP, 0, (LPCWSTR)utf16_ptr, -1, (LPSTR)target_ptr, j, 0, 0) != 0)
				{
					if (isShiftJIS && useTBLFile)
					{
						out_string = changeUTF8Lang(target_ptr, false);
						delete[]target_ptr;
					}
					else out_string = target_ptr;
				}
				else delete[]target_ptr;
			}
		}
		delete[]utf16_ptr;
	}
	return out_string;
}

bool strconv::checkSJIS(const string& utf8_string)
{
	unsigned int slen, ulen;
	bool chkFlag = false;
	string sjis_string = convertUTF8SJISEndian(utf8_string, false, false);

	// Compare question marks count
	slen = ulen = 0;
	for (unsigned char chr : string(utf8_string))
		if (chr == '?') ulen++;
	for (unsigned char chr : string(sjis_string))
	{
		switch (chr)
		{
		case 0xE3:
			chkFlag = true;
			break;
		case 0x3F:
			chkFlag = true;
		case 0x83:
			if (chkFlag) slen++;
		default:
			chkFlag = false;
			break;
		}
	}

	if (slen > (ulen * 2))
		return true;

	return false;
}
