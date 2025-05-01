#include "CSVFile.h"

csv_struct CSVStream::init(int row_size, int col_size)
{
	return csv_struct(row_size, vector<string>(col_size, ""));
}

int CSVStream::getRowSize(csv_struct& csv_data)
{
	return csv_data.size();
}

int CSVStream::getColumnSize(csv_struct& csv_data)
{
	int size = 0;
	for (vector<string>& col : csv_data) if (size < col.size()) size = col.size();
	return size;
}

int CSVStream::getCount(csv_struct& csv_data)
{
	int count = 0;
	for (vector<string>& col : csv_data)
		for (string str : col)
			if (str != "") count++;
	return count;
}

// get total length of utf-8 string
int CSVStream::getCount2(csv_struct& csv_data)
{
	int count = 0;
	for (vector<string>& col : csv_data)
		for (string str : col)
			for (unsigned char chr : str)
				if ((chr != 0) && ((chr & 0xC0) != 0x80)) count++;
	return count;
}

void CSVStream::setRowSize(csv_struct& csv_data, int size)
{
	int idx, col_size;
	idx = 0;
	if (size > csv_data.size())
	{
		col_size = getColumnSize(csv_data);
		while (size > csv_data.size()) csv_data.push_back(vector<string>(col_size, ""));
	}
	else if (size < csv_data.size())
		idx = csv_data.size() - 1;
		while (size < csv_data.size())
		{
			csv_data[idx].clear();
			csv_data[idx--].shrink_to_fit();
			csv_data.erase(csv_data.end() - 1);
		}
	return;
}

void CSVStream::setColumnSize(csv_struct& csv_data, int size)
{
	for (vector<string>& col : csv_data)
	{
		if (size > col.size())
			while (size > col.size()) col.push_back("");
		else if (size < col.size())
			while (size < col.size()) col.erase(col.end() - 1);
	}
	return;
}

void CSVStream::addRow(csv_struct& csv_data)
{
	int size = getRowSize(csv_data);
	setRowSize(csv_data, ++size);
}

void CSVStream::addColumn(csv_struct& csv_data)
{
	int size = getColumnSize(csv_data);
	setColumnSize(csv_data, ++size);
}

void CSVStream::allClear(csv_struct& csv_data)
{
	int size;
	while (csv_data.size() != 0)
	{
		size = csv_data.size() - 1;
		csv_data[size].clear();
		csv_data[size].shrink_to_fit();
		csv_data.erase(csv_data.end() - 1);
	}
	csv_data.clear();
	csv_data.shrink_to_fit();
}

string CSVStream::getValue(csv_struct& csv_data, int row, int col)
{
	if ((getRowSize(csv_data) <= row) || (getColumnSize(csv_data) <= col))
		return "";
	return csv_data[row][col];
}

bool CSVStream::setValue(csv_struct& csv_data, int row, int col, string str, bool extend)
{
	bool extend_flag = !extend;
	if (extend)
	{
		if (getRowSize(csv_data) <= row) { setRowSize(csv_data, row + 1); extend_flag = true; }
		if (getColumnSize(csv_data) <= col) {setColumnSize(csv_data, col + 1); extend_flag = true; }
	}
	else if ((getRowSize(csv_data) <= row) || (getColumnSize(csv_data) <= col)) return false;
	else extend_flag = true;
	csv_data[row][col] = str;
	return extend_flag;
}

int CSVStream::matchColumnSize(csv_struct& csv_data)
{
	int size = getColumnSize(csv_data);
	for (vector<string>& col : csv_data)
	{
		if (size < col.size())
			while (size < col.size()) col.push_back("");
	}
	return size;
}

vector<string> CSVStream::getFilterColumn(csv_struct& csv_data, string f_text, int f_col, int t_col)
{
	int count = 0;
	int max_size = csv_data.size();
	regex reg_str(f_text);
	vector<string> string_vector(max_size);
	for (vector<string>& col : csv_data)
		if (regex_search(col[f_col], reg_str))
			string_vector[count++] = col[t_col];
	for (max_size--; count < max_size; max_size--)
		string_vector.erase(string_vector.end() - 1);
	return string_vector;
}

/*
	If column contents matched input data,
	Returns target column 2 sorted by target column 1.
	column 1 is a string converted from int type.
*/
vector<string> CSVStream::getFilterColumn2(csv_struct& csv_data, string f_text, int f_col, int t_col1, int t_col2)
{
	int max_count = 0;
	int count = 0;
	int max_size = csv_data.size();
	regex reg_str(f_text);
	vector<string> string_vector(max_size, "");
	for (vector<string>& col : csv_data)
		if (regex_search(col[f_col], reg_str))
		{
			count = stoi(col[t_col1]);
			if (max_count < count) max_count = count;
			string_vector[count] = col[t_col2];
		}
	for (max_size--; max_count < max_size; max_size--)
		string_vector.erase(string_vector.end() - 1);
	return string_vector;
}

int CSVStream::readFile(string csv_filename, csv_struct& csv_data)
{
	// initialize variables
	int total_len, max_column, idx, col, row;
	bool isNest;
	char utf8_bom[4]{ 0xEF, 0xBB, 0xBF, 0 }, check_bom[4]{ 0 }, char_buff[0x1000]{ 0 }, read_char;
	FILE* csv_file;
	read_char = 0;
	max_column = 0;
	idx = 0;
	isNest = false;
	csv_data = init(1, 1);
	col = row = 0;

	csv_file = fopen(csv_filename.c_str(), "rb");
	if (!csv_file) return 2; // Failed to read source file

	// Check utf8 BOM
	fseek(csv_file, 0, SEEK_END);
	total_len = ftell(csv_file);
	fseek(csv_file, 0, SEEK_SET);
	fread(check_bom, 3, 1, csv_file);
	if (strncmp(check_bom, utf8_bom, 3) != 0) fseek(csv_file, 0, SEEK_SET);
	
	// read csv and write index
	while ((read_char = fgetc(csv_file)) != EOF)
	{
		switch (read_char)
		{
		case '"':
			if (!isNest && (strlen(char_buff) == 0))
				isNest = true;
			else if (isNest)
			{
				if (fgetc(csv_file) == '"')
				{
					char_buff[idx++] = read_char;
				}
				else
				{
					fseek(csv_file, -1, SEEK_CUR);
					isNest = false;
				}
			}
			else char_buff[idx++] = read_char;
			break;
		case ',':
			if (isNest) char_buff[idx++] = read_char;
			else
			{
				setValue(csv_data, row, col++, char_buff, true);
				memset(char_buff, 0, 0x1000);
				idx = 0;
				break;
			}
			break;
		case 0x0D:
			if (!isNest)
			{
				setValue(csv_data, row++, col++, char_buff, true);
				memset(char_buff, 0, 0x1000);
				idx = 0;
				if (max_column < col) max_column = col;
				addRow(csv_data);
				col = 0;
			}
		case 0x0A:
			if (isNest) char_buff[idx++] = read_char;
			break;
		default:
			char_buff[idx++] = read_char;
		}
	}
	if (strlen(char_buff) != 0)
	{
		setValue(csv_data, row, col++, char_buff, true);
		if (max_column < col) max_column = col;
	}
	setColumnSize(csv_data, max_column);
	fclose(csv_file);
	return 0;
}

// col-0: Filename , col-1: Index , col-2: Text
bool CSVStream::readFile2Map(string csv_filename, map<string, map<int, string>>& csvMap) {
	// initialize variables
	unsigned int total_len, idx, col, i;
	string map_str;
	bool isNest;
	char utf8_bom[4]{ 0xEF, 0xBB, 0xBF, 0 }, check_bom[4]{ 0 }, char_buff[0x1000]{ 0 }, read_char;
	FILE* csv_file;
	read_char = idx = col = i = 0;
	isNest = false;

	csv_file = fopen(csv_filename.c_str(), "rb");
	if (!csv_file) return false;

	// Check utf8 BOM
	fseek(csv_file, 0, SEEK_END);
	total_len = ftell(csv_file);
	fseek(csv_file, 0, SEEK_SET);
	fread(check_bom, 3, 1, csv_file);
	if (strncmp(check_bom, utf8_bom, 3) != 0) fseek(csv_file, 0, SEEK_SET);

	// read csv and write index
	while ((read_char = fgetc(csv_file)) != EOF)
	{
		switch (read_char)
		{
		case '"':
			if (!isNest && (strlen(char_buff) == 0))
				isNest = true;
			else if (isNest)
			{
				if (fgetc(csv_file) == '"')
				{
					char_buff[idx++] = read_char;
				}
				else
				{
					fseek(csv_file, -1, SEEK_CUR);
					isNest = false;
				}
			}
			else char_buff[idx++] = read_char;
			break;
		case ',':
			if (isNest) char_buff[idx++] = read_char;
			else
			{
				if (col == 0)
				{
					map_str = char_buff;
				}
				else if (col == 1)
				{
					if ((char_buff[0] & 0xF0) == 0x30)
						i = stoi(char_buff);
					else i = 0;
				}
				memset(char_buff, 0, 0x1000);
				col++;
				idx = 0;
				break;
			}
			break;
		case 0x0D:
			if (!isNest)
			{
				csvMap[map_str][i] = char_buff;
				memset(char_buff, 0, 0x1000);
				col = idx = 0;
			}
		case 0x0A:
			if (isNest) char_buff[idx++] = read_char;
			break;
		default:
			char_buff[idx++] = read_char;
		}
	}
	if (strlen(char_buff) != 0)
		csvMap[map_str][i] = char_buff;
	fclose(csv_file);

	return true;
}

bool CSVStream::writeFile(csv_struct& csv_data, string csv_filename)
{
	// initialize variables
	int data_col, data_row, col, row;
	char utf8_bom[3]{ 0xEF, 0xBB, 0xBF }, add_col[2]{ 0x0D, 0x0A };
	FILE* csv_file;
	regex reg_str("\"|\r|\n|,|\xE3\x80\x80|\xEF\xBC[\x81-\xBF]|\xEF\xBD[\x80-\x9E]|\xEF\xBF[\xA0-\xA6]"), reg_ptn("\"");
	string out_string, rep_str;
	
	rep_str = "\"\"";
	data_col = getColumnSize(csv_data);
	data_row = getRowSize(csv_data);
	col = row = 0;

	// Check data and file
	if ((data_col == 0) || (data_row == 0)) return false;
	setColumnSize(csv_data, data_col);

	csv_file = fopen(csv_filename.c_str(), "wb");
	if (!csv_file) return false;

	// Write csv file
	fwrite(utf8_bom, 3, 1, csv_file);
	for (row = 0; row < data_row; row++)
	{
		for (col = 0; col < data_col; col++)
		{
			out_string = (string)csv_data[row][col];
			if (regex_search(out_string, reg_str))
				out_string = "\"" + regex_replace(out_string, reg_ptn, rep_str) +"\"";
			if (col < data_col - 1) out_string += ",";
			fwrite(&out_string[0], strlen(out_string.c_str()), 1, csv_file);
		}
		if (row < data_row - 1) fwrite(add_col, 2, 1, csv_file);
	}
	fclose(csv_file);
	return true;
}

void CSVStream::writeStream(FILE* csv_file, csv_struct& csv_data, int s_row, int e_row, bool isFirst)
{
	// initialize variables
	int row, col;
	char utf8_bom[3]{ 0xEF, 0xBB, 0xBF }, add_col[2]{ 0x0D, 0x0A };
	regex reg_str("\"|\r|\n|,|\xE3\x80\x80|\xEF\xBC[\x81-\xBF]|\xEF\xBD[\x80-\x9E]|\xEF\xBF[\xA0-\xA6]"), reg_ptn("\"");
	string out_string, rep_str;
	row = s_row;
	col = 0;
	rep_str = "\"\"";

	// Write csv file
	if (isFirst) fwrite(utf8_bom, 3, 1, csv_file);
	while (row <= e_row)
	{
		for (col = 0; col < 3; col++)
		{
			out_string = (string)csv_data[row][col];
			if (regex_search(out_string, reg_str))
				out_string = "\"" + regex_replace(out_string, reg_ptn, rep_str) + "\"";
			if (col < 2) out_string += ",";
			fwrite(&out_string[0], strlen(out_string.c_str()), 1, csv_file);
		}
		fwrite(add_col, 2, 1, csv_file);
		row++;
	}
}
