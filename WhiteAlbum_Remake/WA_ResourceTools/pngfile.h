#pragma warning(disable:4996)

#ifndef PNGFILE_H
#define PNGFILE_H
#include <string>
#include <stdio.h>
#include "zconf.h"
#include "zlib.h"
#include "png.h"

using namespace std;

#define PNG_BYTES_TO_CHECK 4
#define HAVE_ALPHA 1
#define NO_ALPHA 0

typedef struct PIC_Data
{
	unsigned int width, height;
	int bit_depth;
	int flag;

	unsigned char* rgba;
} pic_data;

class pngSteamTool {
public:
	int readPNGFile(string input_filename, pic_data* out_data);
	int writePNGFile(string output_filename, pic_data* input_data);
};
#endif