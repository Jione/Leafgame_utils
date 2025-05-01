#include "AquaWAConvert.h"

/*
	result value
	-3: not yet supported
	-2: unknown error
	-1: wrong source file
	0: conversion successful
	1: file read successful / conversion failed
	2: Failed to read source file
	3: Failed to read target file
*/
int WA_Convert::convertWAFiles(string in_filename, int comp_opt, string tbl_filename)
{
	int check_type = str.pathChecker(in_filename);
	// file not found
	if (check_type == -1) return 2;

	// folder to pck
	if (comp_opt >= 0) msg.echoTarget(in_filename);
	if (check_type == 1) return pck.pckRepackFile(in_filename, comp_opt);

	// split filename / extension
	string file_name = str.getFilename(in_filename);
	string file_name2 = str.removeUnderbar(file_name);
	string file_ext = str.getExtension(in_filename);
	string file_ext2 = str.getExtension2(in_filename);
	transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);

	// convert file
	try
	{
		if (file_ext == ".pck") check_type = pck.pckUnpackFile(in_filename, comp_opt);
		else if (file_ext == ".sdat") check_type = scr.convertSdat2Csv(in_filename, comp_opt, tbl_filename);
		else if (file_ext == ".csv") check_type = scr.convertCsv2Sdat(in_filename, comp_opt, tbl_filename);
		else if (file_ext == ".tex") check_type = tex.convertTexLZ77(in_filename, comp_opt);
		else if (file_ext == ".lz7") check_type = tex.convertTexLZ77(in_filename, comp_opt);
		else if (file_ext == ".mot") check_type = tex.convertPngLZ77(in_filename, comp_opt);
		else if (file_ext == ".fnt") check_type = fnt.convertFnt2Png_csv(in_filename, comp_opt);
		else if (file_ext2 == ".fnt.png") check_type = fnt.convertPng_csv2Fnt(in_filename, comp_opt);
		else if (file_ext2 == ".parts.png") check_type = tex.updatePartsfile(in_filename, comp_opt);
		else if (file_ext == ".png") check_type = tex.convertPngLZ77(in_filename, comp_opt);
		else return -1;
	}
	catch (...) { check_type = -2; }
	return check_type;
}