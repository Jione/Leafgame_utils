#include "AquaPackfile.h"

vector<string> WCSortFilename = {
	"42px[0-9]\.fnt$", "46px[0-9]\.fnt$", "25px[0-9]\.fnt$", //font.fnt
	"\.lz7$", "tex_all\.tex", "layername\.bin", "height\.txt", "\.amb$", "\.exl$", "face\.uca\.bin", "uvrect\.bin", "default_fade\.txt", "lipsync[0-9]\.bin", "config\.txt" //live2d file
};

bool compareWAStrings(const string& str_a, const string& str_b)
{
	strconv str;
	string a, b;
	int res_a, res_b;
	a = str.removeUnderbar(str.toLower(str_a));
	b = str.removeUnderbar(str.toLower(str_b));
	res_a = str.wildcardVectorSearch(WCSortFilename, a);
	res_b = str.wildcardVectorSearch(WCSortFilename, b);
	if ((res_a < 0) || (res_b < 0) || (res_a == res_b)) return a.compare(b) < 0;
	else if (res_a < res_b) return true;
	else return false;
}

int AquaPack::pckUnpackFile(string in_filename, int comp_opt)
{
	FILE* in_file, * out_file;
	DWORD total_size, data_size, read_length, index_size, d_buff, * offset_array, * read_offset;
	string out_filename, out_folder, csv_filename, conv_out_folder, conv_out_filename, buffer_text, tbl_filename;
	BYTE* file_data;
	bool isDMMFile;
	char hdrbuffer[0x15]{ 0 };
	unsigned char* name_array, * file_char;
	int paddlen;
	pck_name_struct pck_name_hdr;
	pck_pack_struct pck_pack_hdr;
	dmm_pck_name_struct dmm_pck_name_hdr;
	dmm_pck_pack_struct dmm_pck_pack_hdr;

	//open pck file
	in_file = fopen(in_filename.c_str(), "rb");
	if (!in_file) return 2; // Failed to read source file

	fseek(in_file, 0, SEEK_END);
	total_size = ftell(in_file);
	fseek(in_file, 0, SEEK_SET);

	//get Filename header
	fread(hdrbuffer, 0x14, 1, in_file);
	fseek(in_file, 0, SEEK_SET);
	isDMMFile = false;
	if (strncmp(hdrbuffer, "Filename", 8) == 0)
	{
		if (strncmp(&hdrbuffer[8], "    ", 4) != 0)
		{
			isDMMFile = true;
			if (comp_opt >= 0) msg.echoDMMconvert();
			paddlen = 4;
			fread(&dmm_pck_name_hdr, sizeof(dmm_pck_name_struct), 1, in_file);
			bin.endianConvD(dmm_pck_name_hdr.length);
			data_size = dmm_pck_name_hdr.length - sizeof(dmm_pck_name_struct);
		}
		else
		{
			paddlen = 8;
			fread(&pck_name_hdr, sizeof(pck_name_struct), 1, in_file);
			data_size = pck_name_hdr.length - sizeof(pck_name_struct);
		}
	}
	else
	{
		fclose(in_file);
		return 1;
	}
	name_array = new unsigned char[data_size];
	fread(name_array, sizeof(char), data_size, in_file);

	//get Pack header
	fseek(in_file, bin.getPaddlen(data_size, paddlen), SEEK_CUR);
	if (isDMMFile)
	{
		fread(&dmm_pck_pack_hdr, sizeof(dmm_pck_pack_struct), 1, in_file);
		bin.endianConvD(dmm_pck_pack_hdr.index_size);
		bin.endianConvD(dmm_pck_pack_hdr.part_count);
		data_size = dmm_pck_pack_hdr.part_count;
		index_size = dmm_pck_pack_hdr.index_size;
		memcpy(hdrbuffer, dmm_pck_pack_hdr.sig, 8);
	}
	else
	{
		fread(&pck_pack_hdr, sizeof(pck_pack_struct), 1, in_file);
		data_size = pck_pack_hdr.part_count;
		index_size = pck_pack_hdr.index_size;
		memcpy(hdrbuffer, pck_pack_hdr.sig, 0x14);
	}
	d_buff = data_size * 2;
	offset_array = new DWORD[d_buff];
	fread(offset_array, sizeof(DWORD), d_buff, in_file);
	fseek(in_file, bin.getPaddlen(index_size, paddlen), SEEK_CUR);

	if	(
			(isDMMFile && (strncmp(hdrbuffer, "Pack", 4) != 0)) ||
			(!isDMMFile && (strncmp(hdrbuffer, "Pack                ", 0x14) != 0))
		)
	{
		delete[]name_array;
		delete[]offset_array;
		fclose(in_file);
		return 1;
	}

	// Extract Pack files
	read_offset = new DWORD[2];
	out_folder = str.replaceFilename(in_filename, "");
	str.mkdir(out_folder);

	for (int i = 0; i < data_size; i++)
	{
		memcpy(read_offset, &name_array[i * 4], 4);
		if (isDMMFile) bin.endianConvD(read_offset[0]);
		file_char = name_array + read_offset[0];
		out_filename = static_cast<string>(reinterpret_cast<const char*>(file_char));
		out_filename = out_folder + "\\" + out_filename;
		memcpy(read_offset, offset_array + i * 2, 8);
		if (isDMMFile) { bin.endianConvD(read_offset[0]); bin.endianConvD(read_offset[1]); }
		read_length = read_offset[1];

		fseek(in_file, read_offset[0], SEEK_SET);
		file_data = new BYTE[read_length];
		memset(file_data, 0, read_length);
		fread(file_data, 1, read_length, in_file);
		out_file = fopen(out_filename.c_str(), "wb");
		if (!out_file)
		{
			fclose(in_file);
			return 3; // Failed to read target file
		}
		fwrite(file_data, read_length, 1, out_file);
		fclose(out_file);
		delete[]file_data;
	}
	if (comp_opt >= 0) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	fclose(in_file);
	delete[]name_array;
	delete[]offset_array;
	delete[]read_offset;

	if (comp_opt >= 0) msg.echoConvert(out_folder);
	return 0;
}

int AquaPack::pckRepackFile(string in_folder, int comp_opt)
{
	// Initialize various
	bool fnt_flg, sdat_flg, dmm_flg, echo_flg;
	unsigned int file_count, pack_offset, file_offset, in_filesize, fname_length, struct_offset, name_offset, pack_hdr_size, paddlen, paddlen2;
	vector<string> file_list, write_list;
	FILE* in_file, * out_file;
	BYTE* file_buffer;
	DWORD write_buff;
	string out_filename, in_filename, file_ext, write_name;
	pck_name_struct pck_name_hdr{};
	pck_pack_struct pck_pack_hdr{};
	dmm_pck_name_struct dmm_pck_name_hdr{};
	dmm_pck_pack_struct dmm_pck_pack_hdr{};
	write_buff = 0;
	fnt_flg = sdat_flg = false;
	fname_length = 0;
	paddlen2 = 0x10;
	echo_flg = comp_opt >= 0;
	dmm_flg = (comp_opt & 4) != 0;
	if (!echo_flg) dmm_flg = !dmm_flg;

	if (dmm_flg)
	{
		if (echo_flg) msg.echoDMMconvert();
		struct_offset = sizeof(dmm_pck_name_struct);
		pack_hdr_size = sizeof(dmm_pck_pack_struct);
		paddlen = 4;
	}
	else
	{
		struct_offset = sizeof(pck_name_struct);
		pack_hdr_size = sizeof(pck_pack_struct);
		paddlen = 8;
	}
	name_offset = struct_offset;

	// Serch Allowed extension files
	for (const auto& fname : fs::directory_iterator(in_folder))
	{
		if (fs::is_regular_file(fname))
		{
			file_ext = fname.path().extension().string();
			transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);
			if (find(allowedExtensions.begin(), allowedExtensions.end(), file_ext) != allowedExtensions.end())
			{
				if (file_ext == ".fnt") fnt_flg = true;
				else if (file_ext == ".flag") sdat_flg = true;
				file_list.push_back(fname.path().filename().string());
			}
		}
	}
	file_count = file_list.size();
	if (file_count == 0) return -1;

	// make pck structs
	if (sdat_flg) out_filename = in_folder + ".sdat";
	else out_filename = in_folder + ".pck";
	out_file = fopen(out_filename.c_str(), "wb");
	if (!out_file) return 3; // Failed to read target file

	if (dmm_flg)
	{
		memcpy(dmm_pck_name_hdr.sig, "Filename", 8);
		memcpy(dmm_pck_pack_hdr.sig, "Pack    ", 8);
	}
	else
	{
		memset(pck_name_hdr.sig, 0x20, 0x14);
		memcpy(pck_name_hdr.sig, "Filename", 8);
		memset(pck_pack_hdr.sig, 0x20, 0x14);
		memcpy(pck_pack_hdr.sig, "Pack", 4);
	}

	if (file_list.size() > 0xFF) sort(file_list.begin(), file_list.end());
	else sort(file_list.begin(), file_list.end(), compareWAStrings);

	for (string file_name : file_list)
	{
		write_name = str.removeUnderbar(file_name);
		if (dmm_flg) write_name = str.toUpper(write_name);
		write_list.push_back(write_name);
		fname_length += write_name.length();
		fname_length++;
	}

	// Filename, Pack data initialize
	pack_offset = fname_length + struct_offset + file_count * 4;
	name_offset += file_count * 4;
	if (dmm_flg)
	{
		dmm_pck_name_hdr.length = pack_offset;
		dmm_pck_pack_hdr.index_size = pack_hdr_size + file_count * 8;
		dmm_pck_pack_hdr.part_count = file_count;
		pack_offset += bin.getPaddlen(pack_offset, paddlen);
		file_offset = pack_offset + dmm_pck_pack_hdr.index_size;
		if (str.getTotalsize(in_folder, file_list) > 0x2000000) paddlen2 = 0x800;
		file_offset += bin.getPaddlen(file_offset, paddlen2);
		bin.endianConvD(dmm_pck_name_hdr.length);
		bin.endianConvD(dmm_pck_pack_hdr.index_size);
		bin.endianConvD(dmm_pck_pack_hdr.part_count);
	}
	else
	{
		pck_name_hdr.length = pack_offset;
		pck_pack_hdr.index_size = pack_hdr_size + file_count * 8;
		pck_pack_hdr.part_count = file_count;
		pack_offset += bin.getPaddlen(pack_offset, paddlen);
		file_offset = pack_offset + pck_pack_hdr.index_size + bin.getPaddlen(pack_hdr_size, paddlen);
	}

	// write pck file header
	bin.fileZerofill(out_file, file_offset);
	fseek(out_file, 0, SEEK_SET);
	if (dmm_flg) fwrite(&dmm_pck_name_hdr, struct_offset, 1, out_file);
	else fwrite(&pck_name_hdr, struct_offset, 1, out_file);
	fseek(out_file, pack_offset, SEEK_SET);
	if (dmm_flg) fwrite(&dmm_pck_pack_hdr, pack_hdr_size, 1, out_file);
	else fwrite(&pck_pack_hdr, pack_hdr_size, 1, out_file);
	pack_offset += pack_hdr_size;

	// struct_offset, name_offset, pack_offset, file_offset, write_list, file_list
	for (int i = 0; i < file_count; i++)
	{
		// write filename offset
		fseek(out_file, struct_offset + i * 4, SEEK_SET);
		write_name = write_list[i];
		fname_length = write_name.length();
		write_buff = name_offset - struct_offset;
		if (dmm_flg) bin.endianConvD(write_buff);
		fwrite(&write_buff, 4, 1, out_file);

		// write filename
		fseek(out_file, name_offset, SEEK_SET);
		const char* name_array = write_name.c_str();
		fwrite(name_array, sizeof(char), fname_length, out_file);
		name_offset += write_name.length() + 1;

		// open input file
		in_filename = in_folder + "\\" + file_list[i].c_str();
		in_file = fopen(in_filename.c_str(), "rb");
		if (!in_file) return 2; // Failed to read source file

		fseek(in_file, 0, SEEK_END);
		in_filesize = ftell(in_file);
		fseek(in_file, 0, SEEK_SET);
		file_buffer = new BYTE[in_filesize];
		fread(file_buffer, in_filesize, 1, in_file);
		fclose(in_file);

		// write file offset
		fseek(out_file, pack_offset + i * 8, SEEK_SET);
		write_buff = file_offset;
		if (dmm_flg) bin.endianConvD(write_buff);
		fwrite(&write_buff, 4, 1, out_file);
		write_buff = in_filesize;
		if (dmm_flg) bin.endianConvD(write_buff);
		fwrite(&write_buff, 4, 1, out_file);

		// write file
		fseek(out_file, 0, SEEK_END);
		fwrite(file_buffer, in_filesize, 1, out_file);

		// write padding
		bin.fileZerofill(out_file, bin.getPaddlen(in_filesize, paddlen2));
		file_offset = ftell(out_file);

		delete[]file_buffer;
	}
	paddlen = bin.getPaddlen(ftell(out_file) - paddlen, 0x10);
	bin.fileZerofill(out_file, paddlen);

	fclose(out_file);

	if (echo_flg) msg.echoConvert(out_filename);
	return 0;
}