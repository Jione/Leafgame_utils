#define _CRT_SECURE_NO_WARNINGS_GLOBALS
#include "AquaWAConvert.h"

void main(int argc, char* argv[])
{
	unsigned int nowConsole = GetConsoleOutputCP();

	WA_Convert WAConv;
	strconv str;
	customMessage msg;

	int i, file_count, parts_count, error_num, comp_opt;
	string filename, tbl_filename, check_opt, check_ext;
	bool echo_opt, dmm_opt, check_parts, conv_parts;

	vector<string> file_list, parts_list;
	vector<string> allowedOptions = { "/n", "-n", "/d", "-d", "/o", "-o", "/v", "-v", "/p", "-p" };
	vector<string> allowedExtensions = { ".pck", ".tex", ".lz7", ".fnt", ".png", ".mot", ".sdat", ".fnt.png" , ".csv" };

	// set default option flags
	// comp_opt: bitmask flag
	// 1: optimized compress, 2: none compress, 4: DMM File convert, reverse bit: echo-off
	comp_opt = 0;
	echo_opt = true;
	dmm_opt = false;
	check_parts = true;
	conv_parts = false;

	// parsing args value
	for (i = 1; i < argc; i++)
	{
		check_opt = argv[i];
		transform(check_opt.begin(), check_opt.end(), check_opt.begin(), ::tolower);
		if (find(allowedOptions.begin(), allowedOptions.end(), check_opt) != allowedOptions.end())
		{
			check_opt = check_opt.substr(1, 1);
			if (check_opt == "d") comp_opt = 0;
			else if (check_opt == "o") comp_opt = 1;
			else if (check_opt == "n") comp_opt = 2;
			else if (check_opt == "p") dmm_opt = true;
			else if (check_opt == "v") echo_opt = false;
		}
		else
		{
			check_ext = str.getExtension(check_opt);
			check_opt = str.getExtension2(check_opt);
			if (find(allowedExtensions.begin(), allowedExtensions.end(), check_ext) != allowedExtensions.end())
			{
				if (check_ext == ".png")
				{
					if (check_opt == ".parts.png")
					{
						if (check_parts) {
							check_parts = false;
							conv_parts = msg.checkPartsConvert();
						}
						if (conv_parts)
							parts_list.push_back(argv[i]);
					}
					else if (find(allowedExtensions.begin(), allowedExtensions.end(), check_opt) != allowedExtensions.end())
						file_list.push_back(argv[i]);
				}
				else if (check_ext == ".csv")
				{
					if (str.pathChecker(str.replaceExtension(argv[i], ".org")) == 0)
						file_list.push_back(argv[i]);
				}
				else file_list.push_back(argv[i]);
			}
			else
			{
				check_opt = argv[i];
				if (str.pathChecker(check_opt) == 1)
				{
					check_opt = str.removeLastslushDir(check_opt);
					file_list.push_back(check_opt);
				}
			}
		}
	}
	file_count = file_list.size();
	parts_count = parts_list.size();
	tbl_filename = str.replaceExtension(argv[0], "_Table.txt");

	// make flag
	if (dmm_opt) comp_opt += 4;
	if (!echo_opt) comp_opt = ~comp_opt;

	// run main program
	if ((file_count + parts_count) == 0)
	{
		msg.helpMsg(argv[0]);
		printf("\n");
		system("pause");
		SetConsoleOutputCP(nowConsole);
		return;
	}
	else if ((file_count + parts_count) == 1)
	{
		if (parts_count == 0) filename = file_list[0];
		else filename = parts_list[0];
		error_num = WAConv.convertWAFiles(filename, comp_opt, tbl_filename);
		if (error_num == -1)
		{
			msg.helpMsg(argv[0]);
			printf("\n");
			msg.errorPause(error_num, filename);
			SetConsoleOutputCP(nowConsole);
			return;
		}
		else if (error_num)
		{
			msg.errorPause(error_num, filename);
			SetConsoleOutputCP(nowConsole);
			return;
		}
	}
	else
	{
		for (i = 0; i < parts_count; i++)
		{
			filename = parts_list[i];
			error_num = WAConv.convertWAFiles(filename, comp_opt, tbl_filename);
			if (error_num != 0)
			{
				msg.errorMsg(error_num, filename);
				printf("\n");
			}
		}
		for (i = 0; i < file_count; i++)
		{
			filename = file_list[i];
			error_num = WAConv.convertWAFiles(filename, comp_opt, tbl_filename);
			if (error_num != 0)
			{
				msg.errorMsg(error_num, filename);
				printf("\n");
			}
		}
	}
	if (echo_opt) msg.echoComplete();
	SetConsoleOutputCP(nowConsole);
}