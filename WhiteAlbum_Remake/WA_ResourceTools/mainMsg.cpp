#include "mainMsg.h"

void customMessage::helpMsg(string path)
{
	SetConsoleOutputCP(CP_UTF8);

	string exe_name = str.getFilename(path);
	printf("\n");
	printf("Resource Conversion tool for WhiteAlbum Steam version\n");
	printf("\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	printf("Usage: %s.exe <Options> [.pck | Folder | .tex | .png | .lz7 | .mot | .fnt | .fnt.png | .sdat | .csv ]\n", exe_name.c_str());
	printf("\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	printf("[*] Supported Formats\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf("%s.exe [file.pck or Folder]        :  Unpack / Repack pck file\n", exe_name.c_str());
	printf("%s.exe [file.tex or file.png]      :  convert tex <-> png file\n", exe_name.c_str());
	printf("%s.exe [file.lz7 or file.mot]      :  convert lz7 <-> mot file\n", exe_name.c_str());
	printf("%s.exe [font.fnt or font.fnt.png]  :  convert fnt <-> fnt.png file\n", exe_name.c_str());
	printf("%s.exe [script.sdat or script.csv] :  convert sdat <-> csv file\n", exe_name.c_str());
	printf("\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	printf("[*] Options\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf("%s.exe [/N | /D | /O] [/V] [/P]\n", exe_name.c_str());
	printf("\n");
	printf(" [/N | /D | /O] is Change LZ77 compression mode\n");
	printf(" /N : None-Compression      (Fast, Largest File size)\n");
	printf(" /D : Defualt Compression   (Slow, W/A used Compression algorithm)\n");
	printf(" /O : Optimized Compression (Slow, Enhenced Compression algorithm)\n");
	printf(" /V : Mute system messages  (Echo-off mode)\n");
	printf(" /P : Previous version      (old PC-DMM version convert mode)\n");
	printf("\n");
}

void customMessage::errorNosupport()
{
	printf("\r[*] Alert    :  Not yet implemented.\n");
}

void customMessage::errorUnknown(string& path)
{
	printf("\r[*] Error    :  Unknwon error. (%s)\n", str.getName(path).c_str());
}

void customMessage::errorFile(string& path)
{
	string ext_name = str.getExtension2(path);
	printf("\r[*] Error    :  Invalid file format. (%s)\n", ext_name.c_str());
}

void customMessage::errorConvert(string& path)
{
	printf("\r[*] Error    :  Conversion failed. Verify that the file is valid.\n");
}

void customMessage::errorOpenFile(string& path)
{
	printf("\r[*] Error    :  Unable to open target file. (%s)\n", str.getName(path).c_str());
}

void customMessage::errorOpenWritefile()
{
	printf("\r[*] Error    :  Unable to open conversion file.\n");
}

void customMessage::errorMsg(int& error_num, string& path)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	if (error_num == -3)
		errorNosupport();
	else if (error_num == -2)
		errorUnknown(path);
	else if (error_num == -1)
		errorFile(path);
	else if (error_num == 1)
		errorConvert(path);
	else if (error_num == 2)
		errorOpenFile(path);
	else if (error_num == 3)
		errorOpenWritefile();
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	return;
}

void customMessage::errorPause(int& error_num, string path)
{
	errorMsg(error_num, path);
	SetConsoleOutputCP(CP_UTF8);
	printf("\n");
	system("pause");
}

bool customMessage::checkPartsConvert()
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	printf("\n ********** Parts convision mode **********");
	printf("\n Caution: Experimental feature not tested.");
	printf("\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	printf("\n Found parts.png file.");
	printf("\n Update parts file? [Yes / No] <Y>: ");
	while (true)
	{
		switch (getch()) {
		case 0x0A:
		case 0x0D:
		case 'y':
		case 'Y':
			printf("Y\n\n");
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
			return true;
		case 0x1B:
		case 'n':
		case 'N':
			printf("N\n\n");
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
			return false;
		}
	}
}

void customMessage::echoComplete()
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	printf("\r[*] Convert completed!");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	Sleep(2000);
	printf("\n");
}

void customMessage::echoWait()
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	printf("\r[*] Please wait...");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

void customMessage::echoError(string msg)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	printf("\r[*] Error    :  %s\n", msg.c_str());
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

void customMessage::echoAlert(string msg)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
	printf("\r[*] Alert    :  %s\n", msg.c_str());
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

void customMessage::echoTarget(string path)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	printf("\r[*] Target   :  %s\n", path.c_str());
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

void customMessage::echoConvert(string path)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	printf("\r[*] Convert  :  %s\n\n", path.c_str());
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

void customMessage::echoDMMconvert()
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
	printf("\r[*] Alert    :  DMM Version convert mode\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}