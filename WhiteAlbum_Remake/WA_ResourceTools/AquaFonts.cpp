#include "AquaFonts.h"

string AquaFonts::dwordToUTF8(char* chr_array, bool isDMMFile)
{
	char conv_array[4]{ 0 }, buff_array[4]{ 0 };
	int i, j;
	if (!isDMMFile)
	{
		buff_array[3] = chr_array[0];
		buff_array[2] = chr_array[1];
		buff_array[1] = chr_array[2];
		buff_array[0] = chr_array[3];
	}
	else memcpy(buff_array, chr_array, 4);
	for (i = 0; i < 4; i++)
		if (buff_array[i] != 0) break;
	for (j = 0; i < 4; i++)
		conv_array[j++] = buff_array[i];
	return conv_array;
}

void AquaFonts::utf8ToDWORD(char*& chr_array, bool isDMMFile)
{
	char buff_array[4]{ 0 };
	int i, j;
	j = 0;
	for (i = strlen(chr_array), i >= 0; i--;)
		buff_array[j++] = chr_array[i];
	if (isDMMFile)
	{
		chr_array[0] = buff_array[3];
		chr_array[1] = buff_array[2];
		chr_array[2] = buff_array[1];
		chr_array[3] = buff_array[0];
	}
	else memcpy(chr_array, buff_array, 4);
}

void AquaFonts::writeFonttable2File(FILE* out_file, csv_struct csv_data, pic_data png_data, bool isDMMFile)
{
	int width, height, font_size, row, col, max_row, max_col, i;
	float f_width, f_height, x_buff, y_buff;
	char* write_buff = new char[4]{ 0 };
	vector<string> strbuff;
	f_width = width = png_data.width;
	f_height = height = png_data.height;
	max_row = csv.getRowSize(csv_data);
	max_col = csv.getColumnSize(csv_data);
	font_size = png_data.width / max_col;
	row = 0;
	i = 0;
	for (vector<string> col_vec : csv_data)
	{
		col = 0;
		y_buff = float(row * font_size) / f_height;
		if (isDMMFile) bin.endianConvF(y_buff);
		for (string str_data : col_vec)
		{
			strbuff = str.u8chars(str_data);
			for (string str_char : strbuff)
				if (str_char != "")
				{
					memset(write_buff, 0, 4);
					memcpy(write_buff, &str_char, 4);
					utf8ToDWORD(write_buff, isDMMFile);
					x_buff = float(col * font_size) / f_width;
					if (isDMMFile) bin.endianConvF(x_buff);
					fwrite(write_buff, 4, 1, out_file);
					fwrite(&x_buff, 4, 1, out_file);
					fwrite(&y_buff, 4, 1, out_file);
				}
			strbuff.clear();
			strbuff.shrink_to_fit();
			col++;
		}
		row++;
	}
	delete[]write_buff;
}

// .fnt -> .png & .csv file
int AquaFonts::convertFnt2Png_csv(string in_filename, int comp_opt)
{
	//Initialize values
	int row, col, max_row, max_col, padd_len, font_size, i, j;
	bool isDMMFile, errvalue, isExtend;
	char hdrbuffer[0x15]{ 0 };
	float width_f, height_f;

	DWORD total_len, raw_size;
	string png_filename, csv_filename, csv_string;
	BYTE* raw_data;
	FILE* in_file;
	table_struct tbl_hdr;
	texture_struct tex_hdr;
	dmm_table_struct dmm_tbl_hdr;
	dmm_texture_struct dmm_tex_hdr;
	font_struct font_array;
	pic_data png_data{0, 0, 8, HAVE_ALPHA, nullptr};
	csv_struct csv_data;

	padd_len = 8;
	isDMMFile = false;
	errvalue = false;
	png_filename = str.replaceFilename(in_filename, ".fnt.png");
	csv_filename = str.replaceFilename(in_filename, ".fnt.csv");

	// Load fnt file
	in_file = fopen(in_filename.c_str(), "rb");
	if (!in_file) return 2; // Failed to read source file

	// Check file type
	fread(hdrbuffer, 0x14, 1, in_file);
	if (strncmp(hdrbuffer, "Table", 5) != 0)
	{
		fclose(in_file);
		return 1;
	}
	else if (strncmp(&hdrbuffer[8], "    ", 4) != 0) isDMMFile = true;

	fseek(in_file, 0, SEEK_END);
	total_len = ftell(in_file);
	fseek(in_file, 0, SEEK_SET);

	if (isDMMFile)
	{
		padd_len = 4;
		if (comp_opt >= 0) msg.echoDMMconvert();
		fread(&dmm_tbl_hdr, sizeof(dmm_table_struct), 1, in_file);
		bin.endianConvD(dmm_tbl_hdr.header_len);
		bin.endianConvD(dmm_tbl_hdr.size);
		bin.endianConvD(dmm_tbl_hdr.count);
		fseek(in_file, dmm_tbl_hdr.header_len, SEEK_SET);
		fread(&dmm_tex_hdr, sizeof(dmm_texture_struct), 1, in_file);
		bin.endianConvD(dmm_tex_hdr.width);
		bin.endianConvD(dmm_tex_hdr.height);
		bin.endianConvD(dmm_tex_hdr.content_size);
		png_data.width = dmm_tex_hdr.width;
		png_data.height = dmm_tex_hdr.height;
		raw_size = dmm_tex_hdr.content_size;
		font_size = dmm_tbl_hdr.size;
		max_row = png_data.height / font_size;
		max_col = png_data.width / font_size;
		j = dmm_tbl_hdr.count;
	}
	else
	{
		fread(&tbl_hdr, sizeof(table_struct), 1, in_file);
		fseek(in_file, tbl_hdr.header_len + bin.getPaddlen(tbl_hdr.header_len, padd_len), SEEK_SET);
		fread(&tex_hdr, sizeof(texture_struct), 1, in_file);
		png_data.width = tex_hdr.width;
		png_data.height = tex_hdr.height;
		raw_size = tex_hdr.content_size;
		font_size = tbl_hdr.size;
		max_row = png_data.height / font_size;
		max_col = png_data.width / font_size;
		j = tbl_hdr.count;
	}

	// Write png file
	raw_data = new BYTE[raw_size];
	png_data.rgba = raw_data;
	memset(raw_data, 0, raw_size);
	fread(raw_data, raw_size, 1, in_file);
	if (isDMMFile) fseek(in_file, sizeof(dmm_table_struct), SEEK_SET);
	else
	{
		fseek(in_file, sizeof(table_struct), SEEK_SET);
		bin.convertRGBAtoBGRA(raw_data, raw_size);
	}
	if (png.writePNGFile(png_filename, &png_data) != 0)
	{
		delete[]raw_data;
		return 3; // Failed to read target file
	}
	delete[]raw_data;

	// Write csv file
	csv_data = csv.init(max_row, max_col);
	width_f = png_data.width;
	height_f = png_data.height;
	for (i = 0; i < j; i++)
	{
		fread(&font_array, sizeof(font_struct), 1, in_file);
		if (isDMMFile)
		{
			bin.endianConvF(font_array.start_x);
			bin.endianConvF(font_array.start_y);
		}
		row = bin.round(height_f * font_array.start_y) / font_size;
		col = bin.round(width_f * font_array.start_x) / font_size;
		csv_string = csv.getValue(csv_data, row, col);
		csv_string += dwordToUTF8(font_array.sig, isDMMFile);
		isExtend = csv.setValue(csv_data, row, col, csv_string, true);
		if (isExtend)
		{
			if (!errvalue) msg.echoAlert("Extended text found. need to compare with png file.");
			errvalue = true;
		}
	}
	if (!csv.writeFile(csv_data, csv_filename))
	{
		csv.allClear(csv_data);
		return 3; // Failed to read target file
	}
	csv.allClear(csv_data);
	if (comp_opt >= 0) msg.echoConvert(png_filename);
	return 0;
}

// .png & .csv -> .fnt file
int AquaFonts::convertPng_csv2Fnt(string in_filename, int comp_opt)
{
	//Initialize values
	int font_count, font_size, errvalue;
	bool isEcho, isDMMFile;
	char alert_char[0x100]{ 0 };
	string fnt_filename, csv_filename;
	FILE* fnt_file;
	table_struct tbl_hdr{};
	texture_struct tex_hdr{"", 0, 0x4000, 0, 0, 0x810, 0, 0, 0};
	dmm_table_struct dmm_tbl_hdr{};
	dmm_texture_struct dmm_tex_hdr{ "", 0, 8, 0, 0, 0, 0, 0 };
	pic_data png_data;
	csv_struct csv_data;

	errvalue = false;
	isDMMFile = false;
	fnt_filename = str.replaceExtension(in_filename, "");
	csv_filename = str.replaceExtension(in_filename, ".csv");

	// Parsing convert option
	isEcho = comp_opt >= 0;
	isDMMFile = (comp_opt & 4) != 0;
	if (!isEcho) isDMMFile = !isDMMFile;
	if (isEcho && isDMMFile) msg.echoDMMconvert();

	// Load csv data
	errvalue = csv.readFile(csv_filename, csv_data);
	if (errvalue != 0)
	{
		sprintf(alert_char, "%s file does not exist in the folder.", str.getFilename(csv_filename).c_str());
		msg.echoAlert(alert_char);
		return 2; // Failed to read source file
	}

	// Load png data
	errvalue = png.readPNGFile(in_filename, &png_data);
	if (errvalue != 0) return 2; // Failed to read source file

	// Open target file
	fnt_file = fopen(fnt_filename.c_str(), "wb");
	if (!fnt_file)
	{
		delete[]png_data.rgba;
		csv.allClear(csv_data);
		return 3; // Failed to read target file
	}

	// make font and texture table
	font_count = csv.getCount2(csv_data);
	font_size = png_data.width / csv.getColumnSize(csv_data);
	sprintf(alert_char, "Calculated font size is %dx%d pixels.", font_size, font_size);
	if (isEcho) msg.echoAlert(alert_char);

	if (isDMMFile)
	{
		memset(dmm_tex_hdr.sig, 0x20, 0x8);
		memcpy(dmm_tex_hdr.sig, "Texture", 7);
		memset(dmm_tbl_hdr.sig, 0x20, 0x8);
		memcpy(dmm_tbl_hdr.sig, "Table", 5);
		dmm_tex_hdr.content_size = png_data.width * png_data.height * 4;
		dmm_tex_hdr.data_size = sizeof(dmm_texture_struct) + dmm_tex_hdr.content_size;
		dmm_tex_hdr.target_width = dmm_tex_hdr.width = png_data.width;
		dmm_tex_hdr.target_height = dmm_tex_hdr.height = png_data.height;
		dmm_tex_hdr.unknown = 0;
		dmm_tbl_hdr.header_len = sizeof(dmm_table_struct) + sizeof(font_struct) * font_count;
		dmm_tbl_hdr.size = font_size;
		dmm_tbl_hdr.count = font_count;
		bin.endianConvD(dmm_tex_hdr.data_size);
		//bin.endianConvD(dmm_tex_hdr.unknown);
		bin.endianConvD(dmm_tex_hdr.content_size);
		bin.endianConvD(dmm_tex_hdr.target_width);
		bin.endianConvD(dmm_tex_hdr.target_height);
		bin.endianConvD(dmm_tex_hdr.width);
		bin.endianConvD(dmm_tex_hdr.height);
		bin.endianConvD(dmm_tbl_hdr.header_len);
		bin.endianConvD(dmm_tbl_hdr.size);
		bin.endianConvD(dmm_tbl_hdr.count);
		fwrite(&dmm_tbl_hdr, sizeof(dmm_table_struct), 1, fnt_file);
		writeFonttable2File(fnt_file, csv_data, png_data, isDMMFile);
		fwrite(&dmm_tex_hdr, sizeof(dmm_texture_struct), 1, fnt_file);
		bin.endianConvD(dmm_tex_hdr.data_size);
		bin.endianConvD(dmm_tex_hdr.content_size);
		fwrite(png_data.rgba, dmm_tex_hdr.content_size, 1, fnt_file);
		bin.fileZerofill(fnt_file, bin.getPaddlen(dmm_tex_hdr.data_size, 0x8));
	}
	else
	{
		memset(tex_hdr.sig, 0x20, 0x14);
		memcpy(tex_hdr.sig, "Texture", 7);
		memset(tbl_hdr.sig, 0x20, 0x14);
		memcpy(tbl_hdr.sig, "Table", 5);
		tex_hdr.content_size = png_data.width * png_data.height * 4 + 0x2C; // 0x2C Padding?
		tex_hdr.data_size = sizeof(texture_struct) + tex_hdr.content_size;
		tex_hdr.width = png_data.width;
		tex_hdr.height = png_data.height;
		tbl_hdr.header_len = sizeof(table_struct) + sizeof(font_struct) * font_count;
		tbl_hdr.size = font_size;
		tbl_hdr.count = font_count;
		fwrite(&tbl_hdr, sizeof(table_struct), 1, fnt_file);
		writeFonttable2File(fnt_file, csv_data, png_data, isDMMFile);
		bin.fileZerofill(fnt_file, bin.getPaddlen(tbl_hdr.header_len, 8));
		bin.convertRGBAtoBGRA(png_data.rgba, png_data.width * png_data.height * 4);
		fwrite(&tex_hdr, sizeof(texture_struct), 1, fnt_file);
		fwrite(png_data.rgba, tex_hdr.content_size - 0x2C, 1, fnt_file);
		bin.fileZerofill(fnt_file, 0x2C);
		bin.fileZerofill(fnt_file, bin.getPaddlen(tex_hdr.data_size, 0x10));
	}
	
	fclose(fnt_file);
	delete[]png_data.rgba;
	csv.allClear(csv_data);

	if (isEcho) msg.echoConvert(fnt_filename);
	return 0;
}