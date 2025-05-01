#include "AquaTexture.h"

int AquaTexture::makePartsPNGFile(string png_filename, BYTE* raw_data, int raw_size, pic_data png_data, bool isDMMFile)
{
	// Initialize values
	int width, height, x_pos, x_size, y_pos, y_size, read_len, total_len, count, errvalue, seek_pos, overay_pos, i, j, k;
	BYTE* rgba_data;
	DWORD* rgba_table;
	parts_struct parts_data;
	parts_float float_data;
	dmm_parts_struct dmm_parts_data;
	dmm_parts_float dmm_float_data;

	seek_pos = read_len = total_len = count = 0;
	if (isDMMFile)
	{
		seek_pos = sizeof(dmm_parts_data);
		memcpy(&dmm_parts_data, raw_data, seek_pos);
		read_len = sizeof(dmm_parts_float);
		bin.endianConvD(dmm_parts_data.total_len);
		bin.endianConvD(dmm_parts_data.count);
		total_len = dmm_parts_data.total_len;
		count = dmm_parts_data.count;
	}
	else
	{
		seek_pos = sizeof(parts_data);
		memcpy(&parts_data, raw_data, seek_pos);
		total_len = parts_data.total_len;
		read_len = sizeof(parts_float);
		count = parts_data.count;
	}
	if ((total_len > raw_size) || ((count * read_len) > total_len)) return 1;
	if (count <= 1) return -1;

	rgba_table = bin.genRGBATable(count);
	width = png_data.width;
	height = png_data.height;
	rgba_data = new BYTE[width * height * 4];
	memset(rgba_data, 0, width * height * 4);
	png_data.rgba = rgba_data;

	// Overpaint layer color
	for (i = 0; i < count; i++)
	{
		if (isDMMFile)
		{
			memcpy(&dmm_float_data, &raw_data[seek_pos], read_len);
			bin.endianConvF(dmm_float_data.start_x);
			bin.endianConvF(dmm_float_data.start_y);
			bin.endianConvF(dmm_float_data.end_x);
			bin.endianConvF(dmm_float_data.end_y);
			x_pos = dmm_float_data.start_x * (float)width;
			x_size = dmm_float_data.end_x * (float)width - x_pos;
			y_pos = dmm_float_data.start_y * (float)height;
			y_size = dmm_float_data.end_y * (float)height - y_pos;
		}
		else
		{
			memcpy(&float_data, &raw_data[seek_pos], read_len);
			x_pos = bin.round(float_data.start_x * (float)width);
			x_size = bin.round(float_data.end_x * (float)width) - x_pos;
			y_pos = bin.round(float_data.start_y * (float)height);
			y_size = bin.round(float_data.end_y * (float)height) - y_pos;
		}
		for (j = 0; j < y_size; j++)
		{
			overay_pos = (y_pos + j) * width + x_pos;
			for (k = 0; k < x_size; k++) memcpy(&rgba_data[(overay_pos + k) * 4], &rgba_table[i], 4);
		}
		seek_pos += read_len;
	}

	// make png file
	errvalue = png.writePNGFile(png_filename, &png_data);
	delete[]rgba_table;
	delete[]rgba_data;

	if (errvalue != 0) return 3; // Failed to read target file
	return 0;
}

int AquaTexture::writeParts2File(FILE* out_file, string parts_filename, pic_data png_data, bool toDMMFlag)
{
	int padd_len, raw_len, i;
	bool isBGFile, isDMMFile;
	char hdrbuffer[0x15]{ 0 };
	BYTE* raw_data;
	FILE* parts_file;
	parts_struct parts_data{ "", sizeof(parts_struct) + sizeof(parts_float), 1 };
	parts_float float_data{ 0, 0, png_data.width, png_data.height, 0, 0, 1, 1 };
	anime_struct anime_data{ "", sizeof(anime_struct) + sizeof(anime_dword), 1};
	anime_dword dword_data{ 8, 1, 1, 0, 1 };
	dmm_parts_struct dmm_parts_data{ "", sizeof(dmm_parts_struct) + sizeof(dmm_parts_float), 1};
	dmm_parts_float dmm_float_data{ png_data.width, png_data.height, 0, 0, 1, 1 };
	memset(parts_data.sig, 0x20, sizeof(parts_data.sig));
	memset(anime_data.sig, 0x20, sizeof(anime_data.sig));
	memset(dmm_parts_data.sig, 0x20, sizeof(dmm_parts_data.sig));
	memcpy(parts_data.sig, "Parts", 5);
	memcpy(anime_data.sig, "Anime", 5);
	memcpy(dmm_parts_data.sig, "Parts", 5);
	dmm_float_data.end_x = (double)((double)dmm_float_data.width + 0.5) / (double)dmm_float_data.width;
	dmm_float_data.end_y = (double)((double)dmm_float_data.height + 0.5) / (double)dmm_float_data.height;
	dmm_float_data.start_x = 0.5 / (double)dmm_float_data.width;
	dmm_float_data.start_y = 0.5 / (double)dmm_float_data.height;

	isDMMFile = false;
	padd_len = 8;
	if (toDMMFlag) padd_len = 4;
	isBGFile = str.isWABGFile(parts_filename);
	if (isBGFile)
	{
		if (toDMMFlag) bin.fileZerofill(out_file, padd_len);
		else
		{
			anime_data.total_len = sizeof(anime_struct);
			anime_data.count = 0;
			fwrite(&parts_data, sizeof(parts_struct), 1, out_file);
			fwrite(&float_data, sizeof(parts_float), 1, out_file);
			bin.fileZerofill(out_file, bin.getPaddlen(parts_data.total_len, padd_len));
			fwrite(&anime_data, sizeof(anime_struct), 1, out_file);
			bin.fileZerofill(out_file, bin.getPaddlen(anime_data.total_len, padd_len));
			bin.fileZerofill(out_file, 8);
		}
		return 0;
	}

	parts_file = fopen(parts_filename.c_str(), "rb");

	if (!parts_file)
	{
		if (toDMMFlag)
		{
			bin.endianConvD(dmm_parts_data.total_len);
			bin.endianConvD(dmm_parts_data.count);
			bin.endianConvF(dmm_float_data.width);
			bin.endianConvF(dmm_float_data.height);
			bin.endianConvF(dmm_float_data.start_x);
			bin.endianConvF(dmm_float_data.start_y);
			bin.endianConvF(dmm_float_data.end_x);
			bin.endianConvF(dmm_float_data.end_y);
			fwrite(&dmm_parts_data, sizeof(dmm_parts_struct), 1, out_file);
			fwrite(&dmm_float_data, sizeof(dmm_parts_float), 1, out_file);
			bin.fileZerofill(out_file, padd_len);
		}
		else
		{
			float_data.target_width = float_data.width * 0.5;
			float_data.target_height = float_data.height * 0.5;
			fwrite(&parts_data, sizeof(parts_struct), 1, out_file);
			fwrite(&float_data, sizeof(parts_float), 1, out_file);
			bin.fileZerofill(out_file, bin.getPaddlen(parts_data.total_len, padd_len));
			fwrite(&anime_data, sizeof(anime_struct), 1, out_file);
			fwrite(&dword_data, sizeof(anime_dword), 1, out_file);
			bin.fileZerofill(out_file, bin.getPaddlen(anime_data.total_len, padd_len));
			bin.fileZerofill(out_file, padd_len);
		}
		if (str.pathChecker(parts_filename) != -1) return 2;
		return 0;
	}

	fread(hdrbuffer, 0x14, 1, parts_file);
	if (strncmp(hdrbuffer, "Parts", 5) != 0) return 1;
	else if (strncmp(&hdrbuffer[8], "    ", 4) != 0) isDMMFile = true;

	fseek(parts_file, 0, SEEK_END);
	raw_len = ftell(parts_file);
	fseek(parts_file, 0, SEEK_SET);
	raw_data = new BYTE[raw_len];
	memset(raw_data, 0, raw_len);
	fread(raw_data, raw_len, 1, parts_file);
	fclose(parts_file);

	if ((isDMMFile ^ toDMMFlag) == 0) fwrite(raw_data, raw_len, 1, out_file);
	else if (isDMMFile)
	{
		msg.echoAlert("Option(Steam) - Parts(DMM) File Mismatch. Check need to run with '/P' option.");
		memcpy(&dmm_parts_data, raw_data, sizeof(dmm_parts_struct));
		bin.endianConvD(dmm_parts_data.count);
		parts_data.count = dmm_parts_data.count;
		parts_data.total_len = sizeof(parts_struct) + sizeof(parts_float) * parts_data.count;
		fwrite(&parts_data, sizeof(parts_struct), 1, out_file);
		for (i = 0; i < parts_data.count; i++)
		{
			memcpy(&dmm_float_data, &raw_data[sizeof(dmm_parts_struct) + sizeof(dmm_parts_float) * i], sizeof(dmm_parts_float));
			bin.endianConvF(dmm_float_data.width);
			bin.endianConvF(dmm_float_data.height);
			bin.endianConvF(dmm_float_data.start_x);
			bin.endianConvF(dmm_float_data.start_y);
			bin.endianConvF(dmm_float_data.end_x);
			bin.endianConvF(dmm_float_data.end_y);
			dmm_float_data.width = bin.round(dmm_float_data.width);
			dmm_float_data.height = bin.round(dmm_float_data.height);
			dmm_float_data.end_x = (float)(DWORD)(dmm_float_data.end_x * dmm_float_data.width) / dmm_float_data.width;
			dmm_float_data.end_y = (float)(DWORD)(dmm_float_data.end_y * dmm_float_data.height) / dmm_float_data.height;
			dmm_float_data.start_x = (float)(DWORD)(dmm_float_data.start_x * dmm_float_data.width) / dmm_float_data.width;
			dmm_float_data.start_y = (float)(DWORD)(dmm_float_data.start_y * dmm_float_data.height) / dmm_float_data.height;
			memcpy(&float_data.width, &dmm_float_data, sizeof(dmm_parts_float));
			float_data.target_width = float_data.width * (float)0.5;
			float_data.target_height = float_data.height * (float)0.5;
			fwrite(&float_data, sizeof(parts_float), 1, out_file);
		}
		bin.fileZerofill(out_file, bin.getPaddlen(parts_data.total_len, padd_len));
		fwrite(&anime_data, sizeof(anime_struct), 1, out_file);
		fwrite(&dword_data, sizeof(anime_dword), 1, out_file);
		bin.fileZerofill(out_file, bin.getPaddlen(anime_data.total_len, padd_len));
		bin.fileZerofill(out_file, padd_len);
	}
	else
	{
		msg.echoAlert("Option(DMM) - Parts(Steam) File Mismatch. Check need to exclude '/P' option.");
		memcpy(&parts_data, raw_data, sizeof(parts_struct));
		dmm_parts_data.count = parts_data.count;
		dmm_parts_data.total_len = sizeof(dmm_parts_struct) + sizeof(dmm_parts_float) * dmm_parts_data.count;
		bin.endianConvD(dmm_parts_data.count);
		bin.endianConvD(dmm_parts_data.total_len);
		fwrite(&dmm_parts_data, sizeof(dmm_parts_struct), 1, out_file);
		for (i = 0; i < parts_data.count; i++)
		{
			memcpy(&dmm_float_data, &raw_data[sizeof(parts_struct) + sizeof(parts_float) * i + 2], sizeof(dmm_parts_float));
			dmm_float_data.end_x = (double)((double)bin.round(dmm_float_data.end_x * dmm_float_data.width) + 0.5) / (double)dmm_float_data.width;
			dmm_float_data.end_y = (double)((double)bin.round(dmm_float_data.end_y * dmm_float_data.height) + 0.5) / (double)dmm_float_data.height;
			dmm_float_data.start_x = (double)((double)bin.round(dmm_float_data.start_x * dmm_float_data.width) + 0.5) / (double)dmm_float_data.width - 1.0;
			dmm_float_data.start_y = (double)((double)bin.round(dmm_float_data.start_y * dmm_float_data.height) + 0.5) / (double)dmm_float_data.height - 1.0;
			bin.endianConvF(dmm_float_data.width);
			bin.endianConvF(dmm_float_data.height);
			bin.endianConvF(dmm_float_data.start_x);
			bin.endianConvF(dmm_float_data.start_y);
			bin.endianConvF(dmm_float_data.end_x);
			bin.endianConvF(dmm_float_data.end_y);
			fwrite(&dmm_float_data, sizeof(dmm_parts_float), 1, out_file);
		}
		bin.fileZerofill(out_file, padd_len);
	}

	delete[]raw_data;
	return 0;
}

int AquaTexture::convertTexLZ77(string in_filename, int comp_opt)
{
	// Initialize values
	FILE* in_file, * out_file;
	BYTE* raw_data, * lz77_data;
	DWORD total_len, d_buff;
	string out_filename, parts_filename;
	bool isDMMFile;
	char conv_mod, hdrbuffer[0x15]{ 0 };
	int errvalue, raw_size, lz_size, lz77_pos, padd_len;
	texture_struct tex_hdr;
	dmm_texture_struct dmm_tex_hdr;
	lz77_struct lz77_hdr;
	pic_data png_data;
	
	isDMMFile = false;
	d_buff = 0;
	conv_mod = 0;
	padd_len = 8;

	// Load texture file
	in_file = fopen(in_filename.c_str(), "rb");
	if (!in_file) return 2; // Failed to read source file

	fseek(in_file, 0, SEEK_END);
	total_len = ftell(in_file);
	fseek(in_file, 0, SEEK_SET);

	// Check file type
	// conv_mod - 0: default, 1: tex_all, 2: .lz7 file
	fread(hdrbuffer, 0x14, 1, in_file);
	if (strncmp(hdrbuffer, "Texture", 7) == 0)
	{
		if (strncmp(&hdrbuffer[8], "    ", 4) != 0) isDMMFile = true;
	}
	else if (strncmp(hdrbuffer, "LZ77", 4) == 0)
	{
		conv_mod = 2;
		memcpy(&d_buff, &hdrbuffer[0xC], 4);
		if (total_len < d_buff)
		{
			memcpy(&d_buff, &hdrbuffer[0xC], 4);
			bin.endianConvD(d_buff);
			if (total_len < d_buff)
			{
				fclose(in_file);
				return 1;
			}
			else isDMMFile = true;
		}
	}
	else
	{
		conv_mod = 1;
		memcpy(&d_buff, hdrbuffer, 4);
		if (hdrbuffer[3] == 8) bin.endianConvD(d_buff);
		else d_buff = d_buff >> 8;
		switch (d_buff)
		{
		case 8:
			isDMMFile = true;
		case 0x40:
			break;
		default:
			fclose(in_file);
			return 1;
		}
	}

	if (isDMMFile)
	{
		if (comp_opt >= 0) msg.echoDMMconvert();
		padd_len = 4;
	}
	if (conv_mod == 2) out_filename = str.replaceFilename(in_filename, ".mot");
	else out_filename = str.replaceFilename(in_filename, ".png");

	// Parsing Texture data
	fseek(in_file, 0, SEEK_SET);

	lz77_pos = 0;
	lz_size = total_len;

	switch (conv_mod)
	{
	case 0:
		if (isDMMFile)
		{
			lz77_pos = sizeof(dmm_tex_hdr.sig) + sizeof(dmm_tex_hdr.data_size);
			fread(&dmm_tex_hdr, lz77_pos, 1, in_file);
			bin.endianConvD(dmm_tex_hdr.data_size);
		}
		else
		{

			lz77_pos = sizeof(tex_hdr.sig) + sizeof(tex_hdr.data_size);
			fread(&tex_hdr, lz77_pos, 1, in_file);
		}
	case 1:
		if (isDMMFile)
		{
			fread(&dmm_tex_hdr.unknown, dmm_tex_infosize, 1, in_file);
			bin.endianConvD(dmm_tex_hdr.content_size);
			bin.endianConvD(dmm_tex_hdr.width);
			bin.endianConvD(dmm_tex_hdr.height);
			lz77_pos += dmm_tex_infosize;
			lz_size = dmm_tex_hdr.content_size;
			png_data.width = dmm_tex_hdr.width;
			png_data.height = dmm_tex_hdr.height;
		}
		else
		{
			fread(&tex_hdr.unknown_0, tex_infosize, 1, in_file);
			lz77_pos += tex_infosize;
			lz_size = tex_hdr.content_size;
			png_data.width = tex_hdr.width;
			png_data.height = tex_hdr.height;
		}
		break;
	}
	fread(&lz77_hdr, lz77.lz77_hdrsize, 1, in_file);
	lz_size -= lz77.lz77_hdrsize;
	lz77_data = new BYTE[lz_size];
	fread(lz77_data, lz_size, 1, in_file);
	if (isDMMFile)
	{
		bin.endianConvD(lz77_hdr.total_len);
		bin.endianConvD(lz77_hdr.flag_count);
		bin.endianConvD(lz77_hdr.data_offset);
	}
	lz77_hdr.compress_data = lz77_data;
	lz77_hdr.compress_size = lz_size;

	// Decompress LZ77 Data
	lz77.Decompress(&lz77_hdr, raw_data);
	
	// create converted file
	if (conv_mod == 2)
	{
		out_file = fopen(out_filename.c_str(), "wb");
		if (!out_file)
		{
			fclose(in_file);
			delete[]raw_data;
			delete[]lz77_data;
			return 3; // Failed to read target file
		}
		fwrite(raw_data, 1, lz77_hdr.total_len, out_file);
		fclose(out_file);
	}
	else
	{
		switch (isDMMFile)
		{
		case false:
			bin.convertRGBAtoBGRA(raw_data, lz77_hdr.total_len);
		default:
			if (conv_mod == 1) bin.flipRawImage(raw_data, lz77_hdr.total_len, png_data.width);
			png_data.bit_depth = 8;
			png_data.flag = HAVE_ALPHA;
			png_data.rgba = raw_data;
			errvalue = png.writePNGFile(out_filename, &png_data);
			if (errvalue != 0)
			{
				fclose(in_file);
				delete[]raw_data;
				delete[]lz77_data;
				return 3; // Failed to read target file
			}
		}
	}
	delete[]raw_data;
	delete[]lz77_data;

	if (conv_mod == 0)
	{
		lz_size += lz77.lz77_hdrsize;
		lz77_pos += lz_size + bin.getPaddlen(lz_size, padd_len);
		memset(hdrbuffer, 0, 0x15);
		fseek(in_file, lz77_pos, SEEK_SET);
		fread(&hdrbuffer, 5, 1, in_file);
		if (strcmp(hdrbuffer, "Parts") == 0)
		{
			fseek(in_file, lz77_pos, SEEK_SET);
			raw_size = total_len - lz77_pos;
			raw_data = new BYTE[raw_size];
			fread(raw_data, raw_size, 1, in_file);
			parts_filename = str.replaceFilename(in_filename, ".parts");

			errvalue = makePartsPNGFile(parts_filename + ".png", raw_data, raw_size, png_data, isDMMFile);
			if (errvalue == 0)
			{
				out_file = fopen(parts_filename.c_str(), "wb");
				if (!out_file)
				{
					fclose(in_file);
					delete[]raw_data;
					return 3; // Failed to read target file
				}
				fwrite(raw_data, raw_size, 1, out_file);
				fclose(out_file);
			}
			else if (errvalue > 0)
			{
				fclose(in_file);
				delete[]raw_data;
				return errvalue;
			}
			delete[]raw_data;
		}
	}

	fclose(in_file);
	if (comp_opt >= 0) msg.echoConvert(out_filename);
	return 0;
}

int AquaTexture::convertPngLZ77(string in_filename, int comp_opt)
{
	// Initialize values
	FILE* in_file, * out_file;
	BYTE* raw_data, * lz77_data;
	string out_filename, parts_filename;
	bool toDMMFlag;
	char conv_mod;
	int errvalue, raw_size, lz_size, padd_len;
	texture_struct tex_hdr;
	dmm_texture_struct dmm_tex_hdr;
	lz77_struct lz77_hdr;
	pic_data png_data;

	toDMMFlag = (comp_opt & 4) != 0;
	if (!(comp_opt >= 0)) toDMMFlag = !toDMMFlag;
	conv_mod = 0;
	padd_len = 8;

	if ((toDMMFlag) && (comp_opt >= 0))  msg.echoDMMconvert();

	// Check file type
	// conv_mod - 0: default, 1: tex_all, 2: .lz7 file
	if (str.toLower(str.getExtension(in_filename)) == ".mot") conv_mod = 2;
	else if (str.toLower(str.removeUnderbar(str.getFilename(in_filename))) == "tex_all") conv_mod = 1;

	// Load raw binary
	if (conv_mod != 2)
	{
		errvalue = png.readPNGFile(in_filename, &png_data);
		if (errvalue != 0) return 2; // Failed to read source file
		raw_data = png_data.rgba;
		raw_size = png_data.width * png_data.height * 4;
		if (conv_mod == 1) bin.flipRawImage(raw_data, raw_size, png_data.width);
		if (!toDMMFlag) bin.convertRGBAtoBGRA(raw_data, raw_size);
		out_filename = str.replaceExtension(in_filename, ".tex");
		parts_filename = str.replaceExtension(in_filename, ".parts");
		if (toDMMFlag)
		{
			memset(dmm_tex_hdr.sig, 0x20, sizeof(dmm_tex_hdr.sig));
			memcpy(dmm_tex_hdr.sig, "Texture", 7);
			dmm_tex_hdr.unknown = 0x8;
			dmm_tex_hdr.width = png_data.width;
			dmm_tex_hdr.height = png_data.height;
			bin.endianConvD(dmm_tex_hdr.unknown);
			bin.endianConvD(dmm_tex_hdr.width);
			bin.endianConvD(dmm_tex_hdr.height);
			dmm_tex_hdr.target_width = dmm_tex_hdr.width;
			dmm_tex_hdr.target_height = dmm_tex_hdr.height;
		}
		else
		{
			memset(tex_hdr.sig, 0x20, sizeof(tex_hdr.sig));
			memcpy(tex_hdr.sig, "Texture", 7);
			tex_hdr.unknown_0 = 0x4000;
			tex_hdr.unknown_1 = 0x0;
			tex_hdr.unknown_2 = 0x0;
			tex_hdr.unknown_3 = 0x810;
			tex_hdr.width = png_data.width;
			tex_hdr.height = png_data.height;
		}
	}
	else
	{
		in_file = fopen(in_filename.c_str(), "rb");
		if (!in_file) return 2; // Failed to read source file

		fseek(in_file, 0, SEEK_END);
		raw_size = ftell(in_file);
		fseek(in_file, 0, SEEK_SET);

		raw_data = new BYTE[raw_size];
		fread(raw_data, raw_size, 1, in_file);
		fclose(in_file);
		out_filename = str.replaceFilename(in_filename, ".lz7");
	}
	
	// Compress LZ77
	lz77.Compress(raw_data, raw_size, &lz77_hdr, comp_opt);
	lz77_data = lz77_hdr.compress_data;
	lz_size = lz77_hdr.compress_size;
	delete[]raw_data;

	// Write LZ77-Texture file
	out_file = fopen(out_filename.c_str(), "wb");
	if (!out_file)
	{
		delete[]lz77_data;
		return 3; // Failed to read target file
	}

	if (toDMMFlag)
	{
		padd_len = 4;
		dmm_tex_hdr.content_size = lz_size + lz77.lz77_hdrsize;
		dmm_tex_hdr.data_size = dmm_tex_hdr.content_size + sizeof(dmm_tex_hdr.sig) + sizeof(dmm_tex_hdr.data_size) + dmm_tex_infosize;
		bin.endianConvD(lz77_hdr.total_len);
		bin.endianConvD(lz77_hdr.flag_count);
		bin.endianConvD(lz77_hdr.data_offset);
		bin.endianConvD(dmm_tex_hdr.content_size);
		bin.endianConvD(dmm_tex_hdr.data_size);
		switch (conv_mod)
		{
		case 0:
			fwrite(&dmm_tex_hdr, sizeof(dmm_tex_hdr.sig) + sizeof(dmm_tex_hdr.data_size), 1, out_file);
		case 1:
			fwrite(&dmm_tex_hdr.unknown, dmm_tex_infosize, 1, out_file);
		}
	}
	else
	{
		tex_hdr.content_size = lz_size + lz77.lz77_hdrsize;
		tex_hdr.data_size = tex_hdr.content_size + sizeof(tex_hdr.sig) + sizeof(tex_hdr.data_size) + tex_infosize;
		switch (conv_mod)
		{
		case 0:
			fwrite(&tex_hdr, sizeof(tex_hdr.sig) + sizeof(tex_hdr.data_size), 1, out_file);
		case 1:
			fwrite(&tex_hdr.unknown_0, tex_infosize, 1, out_file);
		}
	}

	fwrite(&lz77_hdr, lz77.lz77_hdrsize, 1, out_file);
	fwrite(lz77_data, lz_size, 1, out_file);
	delete[]lz77_data;
	if (conv_mod == 0)
	{
		bin.fileZerofill(out_file, bin.getPaddlen(lz_size + lz77.lz77_hdrsize, padd_len));
		errvalue = writeParts2File(out_file, parts_filename, png_data, toDMMFlag);
		fclose(out_file);
		if (errvalue != 0) return errvalue;
	}
	else fclose(out_file);

	if (comp_opt >= 0) msg.echoConvert(out_filename);
	return 0;
}

int AquaTexture::updatePartsfile(string in_filename, int comp_opt)
{
	int total_len, padd_len, anime_offset, org_count, index, count, width, height, i, j;
	float width_f, height_f;
	double width_d, height_d;
	char hdrbuffer[0x15]{ 0 }, alert_msg[0x100];
	bool isDMMFile;
	float* float_array;
	BYTE* raw_data, * anime_array;
	DWORD* rgba_table, sect_rgba, vec_idx, sect_buff, x, y;
	string parts_filename;
	FILE* parts_file;
	vector<DWORD> rgba_vec;
	vector<DWORD>::iterator index_vec;
	vector<WORD> x_st_vec, x_ed_vec, y_st_vec, y_ed_vec;

	pic_data png_data;
	parts_struct parts_data;
	parts_float float_data;
	anime_struct anime_data{ "", sizeof(anime_struct), 0 };
	dmm_parts_struct dmm_parts_data;
	dmm_parts_float dmm_float_data;

	isDMMFile = false;
	padd_len = 8;
	org_count = count = 0;
	anime_array = nullptr;
	memset(&float_data, 0, sizeof(parts_float));
	memset(&dmm_parts_data, 0, sizeof(dmm_parts_struct));
	memset(&dmm_float_data, 0, sizeof(dmm_parts_float));

	// Open parts file
	parts_filename = str.replaceExtension(in_filename, "");
	parts_file = fopen(parts_filename.c_str(), "rb");
	if (!parts_file) return 3; // Failed to read target file

	// Check Parts type
	fread(hdrbuffer, 0x14, 1, parts_file);
	if (strncmp(hdrbuffer, "Parts", 5) != 0) return 1; // Unable conversion
	else if (strncmp(&hdrbuffer[8], "    ", 4) != 0) isDMMFile = true;

	// Load png data
	int errvalue = png.readPNGFile(in_filename, &png_data);
	if (errvalue != 0)
	{
		fclose(parts_file);
		return 2; // Failed to read source file
	}
	width_f = width_d = width = png_data.width;
	height_f = height_d = height = png_data.height;

	// Parsing Parts - Anime data
	fseek(parts_file, 0, SEEK_END);
	total_len = ftell(parts_file);
	fseek(parts_file, 0, SEEK_SET);

	if (!isDMMFile)
	{
		fread(&parts_data, sizeof(parts_struct), 1, parts_file);
		org_count = parts_data.count;
		anime_offset = parts_data.total_len;
		anime_offset += bin.getPaddlen(anime_offset, padd_len);
		memset(&hdrbuffer, 0, 0x15);
		fseek(parts_file, anime_offset, SEEK_SET);
		fread(&hdrbuffer, 0x14, 1, parts_file);
		if (strncmp(hdrbuffer, "Anime", 5) != 0)
		{
			if (comp_opt >= 0)
			{
				sprintf(alert_msg, "\"%s\" not found 'Anime' section.", parts_filename.c_str());
				msg.echoAlert(alert_msg);
				msg.echoAlert(" --> Runs in default 'Anime' write mode.");
			}
			memset(&anime_data.sig, 0x20, sizeof(anime_data.sig));
			memcpy(&anime_data.sig, "Anime", 5);
			anime_array = new BYTE[anime_data.total_len];
			memcpy(anime_array, &anime_data, sizeof(anime_struct));
		}
		else
		{
			fseek(parts_file, anime_offset, SEEK_SET);
			fread(&anime_data, sizeof(anime_struct), 1, parts_file);
			anime_array = new BYTE[anime_data.total_len];
			fseek(parts_file, anime_offset, SEEK_SET);
			fread(anime_array, anime_data.total_len, 1, parts_file);
		}
	}
	else
	{
		if (comp_opt >= 0) msg.echoDMMconvert();
		padd_len = 4;
		fread(&dmm_parts_data, sizeof(dmm_parts_struct), 1, parts_file);
		bin.endianConvD(dmm_parts_data.count);
		org_count = dmm_parts_data.count;
	}
	fclose(parts_file);
	
	// get array from png image
	raw_data = png_data.rgba;
	index = width * height;
	memset(&sect_buff, 0xFF, 4);
	count = x = y = 0;
	if (comp_opt >= 0) msg.echoWait();
	for (i = 0; i < index; i++)
	{
		memcpy(&sect_rgba, &raw_data[i * 4], 4);
		if (sect_rgba != 0)
		{
			memcpy(&sect_buff, &sect_rgba, 3);
			index_vec = find(rgba_vec.begin(), rgba_vec.end(), sect_buff);
			if (index_vec == rgba_vec.end())
			{
				memcpy(&sect_buff, &sect_rgba, 3);
				rgba_vec.push_back(sect_buff);
				x_st_vec.push_back(x);
				x_ed_vec.push_back(x);
				y_st_vec.push_back(y);
				y_ed_vec.push_back(y);
				count++;
			}
			else
			{
				vec_idx = index_vec - rgba_vec.begin();

				if (x_ed_vec[vec_idx] < x) x_ed_vec[vec_idx] = x;
				else if (x_st_vec[vec_idx] > x) x_st_vec[vec_idx] = x;

				if (y_ed_vec[vec_idx] < y) y_ed_vec[vec_idx] = y;
				else if (y_st_vec[vec_idx] > y) y_st_vec[vec_idx] = y;
			}
		}
		if (++x == width) { x = 0; y++; }
	}
	index = rgba_vec.end() - rgba_vec.begin();
	rgba_table = bin.genRGBATable(count);
	if ((count != org_count) && (comp_opt >= 0))
	{

		sprintf(alert_msg, "Index count between 'origin Parts' - 'new PNG' is different. (%d -> %d)", org_count, count);
		msg.echoAlert(alert_msg);
	}

	// make new array
	org_count = count;
	if (isDMMFile) float_array = new float[count * sizeof(dmm_parts_float)];
	else float_array = new float[count * sizeof(parts_float)];
	for (j = i = 0; i < count; i++)
	{
		memcpy(&sect_buff, &rgba_table[i], 3);
		index_vec = find(rgba_vec.begin(), rgba_vec.end(), sect_buff);
		vec_idx = index_vec - rgba_vec.begin();
		if (index != vec_idx)
		{
			if (isDMMFile)
			{
				dmm_float_data.width = x_ed_vec[vec_idx] - x_st_vec[vec_idx] + 1;
				dmm_float_data.height = y_ed_vec[vec_idx] - y_st_vec[vec_idx] + 1;
				dmm_float_data.start_x = double(double(x_st_vec[vec_idx]) + 0.5) / width_d;
				dmm_float_data.start_y = double(double(y_st_vec[vec_idx]) + 0.5) / height_d;
				dmm_float_data.end_x = double(double(x_ed_vec[vec_idx] + 1) + 0.5) / width_d;
				dmm_float_data.end_y = double(double(y_ed_vec[vec_idx] + 1) + 0.5) / height_d;
				bin.endianConvF(dmm_float_data.width);
				bin.endianConvF(dmm_float_data.height);
				bin.endianConvF(dmm_float_data.start_x);
				bin.endianConvF(dmm_float_data.start_y);
				bin.endianConvF(dmm_float_data.end_x);
				bin.endianConvF(dmm_float_data.end_y);
				memcpy(&float_array[j++ * 6], &dmm_float_data, sizeof(dmm_parts_float));
			}
			else
			{
				float_data.width = x_ed_vec[vec_idx] - x_st_vec[vec_idx] + 1;
				float_data.height = y_ed_vec[vec_idx] - y_st_vec[vec_idx] + 1;
				float_data.target_width = int(float_data.width) / 2;
				float_data.target_height = int(float_data.height) / 2;
				float_data.start_x = float(x_st_vec[vec_idx]) / width_f;
				float_data.start_y = float(y_st_vec[vec_idx]) / height_f;
				float_data.end_x = float(x_ed_vec[vec_idx] + 1) / width_f;
				float_data.end_y = float(y_ed_vec[vec_idx] + 1) / height_f;
				memcpy(&float_array[j++ * 8], &float_data, sizeof(parts_float));
			}
		}
		else
		{
			if (comp_opt >= 0)
			{
				sprintf(alert_msg, "parts.png has not [Index: %d|#%06X] Color", i, sect_buff);
				msg.echoError(alert_msg);
			}
			org_count--;
		}
	}
	if (org_count <= 1)
	{
		if (isDMMFile)
		{
			dmm_float_data.width = width;
			dmm_float_data.height = height;
			dmm_float_data.start_x = 0.5 / width_d;
			dmm_float_data.start_y = 0.5 / height_d;
			dmm_float_data.end_x = double(width_d + 0.5) / width_d;
			dmm_float_data.end_y = double(height_d + 0.5) / height_d;
			bin.endianConvF(dmm_float_data.width);
			bin.endianConvF(dmm_float_data.height);
			bin.endianConvF(dmm_float_data.start_x);
			bin.endianConvF(dmm_float_data.start_y);
			bin.endianConvF(dmm_float_data.end_x);
			bin.endianConvF(dmm_float_data.end_y);
			memcpy(&float_array[j++ * 6], &float_data, sizeof(dmm_parts_float));
		}
		else
		{
			float_data.target_width = width / 2;
			float_data.target_height = height / 2;
			float_data.width = width;
			float_data.height = height;
			float_data.start_x = 0;
			float_data.start_y = 0;
			float_data.end_x = 1;
			float_data.end_y = 1;
			memcpy(float_array, &float_data, sizeof(parts_float));
		}
		org_count = 1;
	}

	// Clear used data
	delete[]raw_data;
	rgba_vec.clear(); x_st_vec.clear(); x_ed_vec.clear(); y_st_vec.clear(); y_ed_vec.clear();
	rgba_vec.shrink_to_fit(); x_st_vec.shrink_to_fit(); x_ed_vec.shrink_to_fit(); y_st_vec.shrink_to_fit(); y_ed_vec.shrink_to_fit();

	// Write new parts file
	parts_file = fopen(parts_filename.c_str(), "wb");
	if (!parts_file)
	{
		delete[]float_array;
		if (!isDMMFile) delete[]anime_array;
		return 3; // Failed to read target file
	}

	if (isDMMFile)
	{
		dmm_parts_data.count = org_count;
		dmm_parts_data.total_len = sizeof(dmm_parts_struct) + sizeof(dmm_parts_float) * org_count;
		bin.endianConvD(dmm_parts_data.count);
		bin.endianConvD(dmm_parts_data.total_len);
		fwrite(&dmm_parts_data, sizeof(dmm_parts_struct), 1, parts_file);
		fwrite(float_array, sizeof(dmm_parts_float) * org_count, 1, parts_file);
	}
	else
	{
		parts_data.count = org_count;
		parts_data.total_len = sizeof(parts_struct) + sizeof(parts_float) * org_count;
		fwrite(&parts_data, sizeof(parts_struct), 1, parts_file);
		fwrite(float_array, sizeof(parts_float) * org_count, 1, parts_file);
		bin.fileZerofill(parts_file, bin.getPaddlen(parts_data.total_len, padd_len));
		fwrite(anime_array, anime_data.total_len, 1, parts_file);
		bin.fileZerofill(parts_file, bin.getPaddlen(anime_data.total_len, padd_len));
		delete[]anime_array;
	}

	delete[]float_array;
	bin.fileZerofill(parts_file, padd_len);
	fclose(parts_file);
	if (comp_opt >= 0) msg.echoConvert(parts_filename);
	return 0;
}