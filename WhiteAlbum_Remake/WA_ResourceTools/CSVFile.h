#pragma warning(disable:4996)

#ifndef CSV_FILE_TOOLS_H
#define CSV_FILE_TOOLS_H
#include <Windows.h>
#include <stdio.h>
#include <string>
#include <regex>
#include <map>

using namespace std;

typedef vector<vector<string>> csv_struct;

class CSVStream {
private:

public:
	csv_struct init(int row_size, int column_size);
	int getRowSize(csv_struct& csv_data);
	int getColumnSize(csv_struct& csv_data);
	int getCount(csv_struct& csv_data);
	int getCount2(csv_struct& csv_data);
	void setRowSize(csv_struct& csv_data, int size);
	void setColumnSize(csv_struct& csv_data, int size);
	void addRow(csv_struct& csv_data);
	void addColumn(csv_struct& csv_data);
	void allClear(csv_struct& csv_data);
	string getValue(csv_struct& csv_data, int row_index, int column_index);
	bool setValue(csv_struct& csv_data, int row_index, int column_index, string string_text, bool extendable);
	int matchColumnSize(csv_struct& csv_data);
	vector<string> getFilterColumn(csv_struct& csv_data, string filterText, int filterColumn_index, int targetColumn_index);
	vector<string> getFilterColumn2(csv_struct& csv_data, string filterText, int filterColumn_index, int targetColumn_index1, int targetColumn_index2);
	int readFile(string csv_filename, csv_struct& csv_data);
	bool readFile2Map(string csv_filename, map<string, map<int, string>>& csvMap);
	bool writeFile(csv_struct& csv_data, string csv_filename);
	void writeStream(FILE* csv_file, csv_struct& csv_data, int start_row, int end_row, bool isFirst);
};
#endif