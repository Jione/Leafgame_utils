#pragma warning(disable:4996)

#ifndef PRINT_CUSTOM_MESSAGE_H
#define PRINT_CUSTOM_MESSAGE_H
#include <Windows.h>
#include <stdio.h>
#include <string>
#include <conio.h>
#include "StringConvert.h"

using namespace std;

class customMessage {
private:
	strconv str;

	void errorNosupport();
	void errorUnknown(string& path);
	void errorFile(string& path);
	void errorConvert(string& path);
	void errorOpenFile(string& path);
	void errorOpenWritefile();

public:
	void helpMsg(string path);
	void errorMsg(int& error_num, string& path);
	void errorPause(int& error_num, string path);
	bool checkPartsConvert();
	void echoComplete();
	void echoWait();
	void echoAlert(string msg);
	void echoError(string msg);
	void echoTarget(string path);
	void echoConvert(string path);
	void echoDMMconvert();
};
#endif