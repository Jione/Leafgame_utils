#ifndef WHITEALBUM_RESOURCE_CONVERT_H
#define WHITEALBUM_RESOURCE_CONVERT_H
#include <stdio.h>
#include <string>
#include "mainMsg.h"
#include "StringConvert.h"
#include "AquaPackfile.h"
#include "AquaTexture.h"
#include "AquaFonts.h"
#include "AquaScripts.h"

using namespace std;

class WA_Convert {
private:
	strconv str;
	customMessage msg;
	AquaPack pck;
	AquaTexture tex;
	AquaFonts fnt;
	AquaScripts scr;

public:
	int convertWAFiles(string input, int options, string tbl_filename);
};
#endif