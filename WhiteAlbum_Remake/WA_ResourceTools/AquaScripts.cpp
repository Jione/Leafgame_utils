#include "AquaScripts.h"

void AquaScripts::convertMes2utf8(string scriptname, BYTE*& raw_data, DWORD& raw_len, csv_struct& csv_data, int& row, int& col, bool isDMMFile, bool isCustom)
{
    //initialize variables
    int i, idx, char_buff, slen;
    char* s_buff, index_buff[0x10]{ 0 };
    bool zero_flag;
    string select_text, text_buff;
    select_text = "/Select/";
    i = idx = 0;
    zero_flag = false;
    s_buff = new char[0x1000];

    while (i < raw_len)
    {
        char_buff = raw_data[i++];
        switch (char_buff)
        {
        case 0xF:
            i += 7;
            memset(s_buff, 0, 0x1000);
            slen = strlen((const char*)&raw_data[i]);
            memcpy(s_buff, &raw_data[i], slen);
            i += slen + 1;
            if (!zero_flag) i++;
            if (isDMMFile)
                text_buff = str.convertUTF8SJISEndian(s_buff, true, isCustom);
            else text_buff = s_buff;
            sprintf(index_buff, "%d", idx++);
            csv.setValue(csv_data, row, col++, scriptname, true);
            csv.setValue(csv_data, row, col++, index_buff, true);
            if (text_buff != "h")
                csv.setValue(csv_data, row++, col, text_buff, true);
            else csv.setValue(csv_data, row++, col, select_text, true);
            col = 0;
            zero_flag = false;
            break;
        case 0x2E:
            char_buff = raw_data[i++] - 1;
            i += (char_buff * 5);
            char_buff = raw_data[i];
            while (char_buff != 0)
                char_buff = raw_data[i++];
            break;
        case 0x42:
            i++;
            zero_flag = true;
            break;
        case 0:
            i += 4;
            if (i == 5) i++;
            break;
        case 0xFF:
            while (char_buff != 0)
                char_buff = raw_data[i++];
            break;
        default:
            char_buff = raw_data[i++];
            i += (char_buff * 5);
            break;
        }
    }

    delete[]s_buff;
}

void AquaScripts::updateMesStrings(string scriptname, BYTE*& raw_data, DWORD& raw_len, map<string, map<int, string>>& csvMap, bool toDMMFile, bool isConv, bool isCustom)
{
    // initialize variables
    int i, j, idx, char_buff, conv_len;
    char* s_buff;
    bool zero_flag;
    BYTE* conv_data;
    WORD w_buff;
    DWORD d_buff;
    string text_buff;
    const char* select_text = "/Select/";
    vector<string> chg_vec;

    zero_flag = false;
    s_buff = new char[0x1000];
    i = idx = 0;

    // make new array
    conv_len = raw_len * 3;
    conv_data = new BYTE[conv_len];
    memset(conv_data, 0, conv_len);
    conv_len = 0;

    // write new array
    while (i < raw_len)
    {
        char_buff = raw_data[i++];
        switch (char_buff)
        {
        case 0xF:
            conv_data[conv_len++] = char_buff;
            memcpy(&d_buff, &raw_data[i], 4);
            if (isConv) bin.endianConvD(d_buff);
            memcpy(&conv_data[conv_len], &d_buff, 4);
            i += 4; conv_len += 4;
            memcpy(&w_buff, &raw_data[i], 2);
            if (isConv) bin.endianConvW(w_buff);
            memcpy(&conv_data[conv_len], &w_buff, 2);
            i += 2; conv_len += 2;
            conv_data[conv_len++] = raw_data[i++];
            memset(s_buff, 0, 0x1000);
            j = strlen((const char*)&raw_data[i]);
            memcpy(s_buff, &raw_data[i], j);
            i += j + 1;
            if (!zero_flag) i++;
            text_buff = csvMap[scriptname][idx++];
            if (text_buff.length() > 0)
            {
                j = text_buff.length();
                memset(s_buff, 0, 0x1000);
                memcpy(s_buff, &text_buff[0], j);
                if (toDMMFile)
                    text_buff = str.convertUTF8SJISEndian(s_buff, false, isCustom);
                else text_buff = s_buff;
            }
            if (strcmp(text_buff.c_str(), select_text) == 0)
                text_buff = "h";
            j = text_buff.length();
            memcpy(&conv_data[conv_len], &text_buff[0], j);
            conv_len += j + 1;
            if (!zero_flag) conv_len++;
            zero_flag = false;
            break;
        case 0x2E:
            conv_data[conv_len++] = char_buff;
            char_buff = conv_data[conv_len++] = raw_data[i++];
            for (j = 1; j < char_buff; j++)
            {
                conv_data[conv_len++] = raw_data[i++];
                memcpy(&d_buff, &raw_data[i], 4);
                if (isConv) bin.endianConvD(d_buff);
                memcpy(&conv_data[conv_len], &d_buff, 4);
                i += 4;
                conv_len += 4;
            }
            char_buff = conv_data[conv_len++] = raw_data[i++];
            while (char_buff != 0)
                char_buff = conv_data[conv_len++] = raw_data[i++];
            break;
        case 0x42:
            conv_data[conv_len++] = char_buff;
            conv_data[conv_len++] = raw_data[i++];
            zero_flag = true;
            break;
        case 0:
            conv_len++;
            memcpy(&d_buff, &raw_data[i], 4);
            if (isConv) bin.endianConvD(d_buff);
            memcpy(&conv_data[conv_len], &d_buff, 4);
            conv_len += 4;
            i += 4;
            if (i == 5) conv_data[conv_len++] = raw_data[i++];
            break;
        case 0xFF:
            conv_data[conv_len++] = char_buff;
            while (char_buff != 0)
                char_buff = conv_data[conv_len++] = raw_data[i++];
            break;
        default:
            conv_data[conv_len++] = char_buff;
            char_buff = conv_data[conv_len++] = raw_data[i++];
            for (j = 0; j < char_buff; j++)
            {
                conv_data[conv_len++] = raw_data[i++];
                memcpy(&d_buff, &raw_data[i], 4);
                if (isConv) bin.endianConvD(d_buff);
                memcpy(&conv_data[conv_len], &d_buff, 4);
                i += 4;
                conv_len += 4;
            }
            break;
        }
    }
    csvMap[scriptname].clear();
    delete[]raw_data;
    raw_data = conv_data;
    raw_len = conv_len;
    delete[]s_buff;
}

int AquaScripts::convertSdat2Csv(string in_filename, int comp_opt, string tbl_filename)
{
    FILE* in_file, * out_file, * csv_file;
    DWORD total_size, data_size, read_length, index_size, * offset_array, d_buff, read_offset[2]{ 0 };
    string csv_filename, org_filename, pack_name;
    BYTE* file_data;
    bool isDMMFile, isCustom;
    char hdrbuffer[0x15]{ 0 };
    char* name_array, * file_char;
    int padd_len, col, row;
    pck_name_struct pck_name_hdr;
    pck_pack_struct pck_pack_hdr;
    dmm_pck_name_struct dmm_pck_name_hdr;
    dmm_pck_pack_struct dmm_pck_pack_hdr;
    csv_struct csv_data;

    csv_filename = str.replaceFilename(in_filename, ".csv");
    org_filename = str.replaceFilename(in_filename, ".org");;
    csv_data = csv.init(1, 3);
    isCustom = false;
    col = 0;
    row = 1;

    // open sdat file
    in_file = fopen(in_filename.c_str(), "rb");
    if (!in_file) return 2; // Failed to read source file

    // check out file access
    csv_file = fopen(csv_filename.c_str(), "wb");
    if (!csv_file)
    {
        fclose(in_file);
        return 3; // Failed to read target file
    }

    out_file = fopen(org_filename.c_str(), "wb");
    if (!out_file)
    {
        fclose(in_file);
        return 3; // Failed to read target file
    }

    // copy sdat to org file (backup binary)
    fseek(in_file, 0, SEEK_END);
    total_size = ftell(in_file);
    fseek(in_file, 0, SEEK_SET);
    file_data = new BYTE[total_size];
    fread(file_data, total_size, 1, in_file);
    fwrite(file_data, total_size, 1, out_file);
    fclose(out_file);
    fseek(in_file, 0, SEEK_SET);
    delete[]file_data;

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
            padd_len = 4;
            fread(&dmm_pck_name_hdr, sizeof(dmm_pck_name_struct), 1, in_file);
            bin.endianConvD(dmm_pck_name_hdr.length);
            data_size = dmm_pck_name_hdr.length - sizeof(dmm_pck_name_struct);
        }
        else
        {
            padd_len = 8;
            fread(&pck_name_hdr, sizeof(pck_name_struct), 1, in_file);
            data_size = pck_name_hdr.length - sizeof(pck_name_struct);
        }
    }
    else
    {
        fclose(in_file);
        return 1;
    }
    name_array = new char[data_size];
    fread(name_array, sizeof(char), data_size, in_file);

    //get Pack header
    fseek(in_file, bin.getPaddlen(data_size, padd_len), SEEK_CUR);
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
    fseek(in_file, bin.getPaddlen(index_size, padd_len), SEEK_CUR);

    if (
        (isDMMFile && (strncmp(hdrbuffer, "Pack", 4) != 0)) ||
        (!isDMMFile && (strncmp(hdrbuffer, "Pack                ", 0x14) != 0))
        )
    {
        delete[]name_array;
        delete[]offset_array;
        fclose(in_file);
        return 1;
    }

    // save csv metadata
    csv_data[0][0] = "File";
    csv_data[0][1] = "Index";
    csv_data[0][2] = "Text";
    csv.writeStream(csv_file, csv_data, 0, 0, true);

    // check DMM::Non-japanese scripts

    if (isDMMFile && ((total_size < 3100000) || (3200000 < total_size)) && (str.pathChecker(tbl_filename) == 0))
    {
        str.setTableFile(tbl_filename);
        isCustom = true;
        if (comp_opt >= 0)
            msg.echoAlert("Not origin JP Scripts. Table convert mode is enabled.");
    }

    // fill csv data
    for (int i = 0; i < data_size; i++)
    {
        row = col = 0;
        memcpy(read_offset, &name_array[i * 4], 4);
        if (isDMMFile) bin.endianConvD(read_offset[0]);
        file_char = name_array + read_offset[0];
        pack_name = static_cast<string>(reinterpret_cast<const char*>(file_char));
        memcpy(read_offset, offset_array + i * 2, 8);
        if (isDMMFile) { bin.endianConvD(read_offset[0]); bin.endianConvD(read_offset[1]); }
        read_length = read_offset[1];
        fseek(in_file, read_offset[0], SEEK_SET);
        file_data = new BYTE[read_length];
        memset(file_data, 0, read_length);
        fread(file_data, 1, read_length, in_file);
        if (strncmp(str.toLower(str.getExtension(pack_name)).c_str(), ".flag", 5) != 0)
        {
            if (comp_opt >= 0)
            {
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
                printf("\r[*] Process  :  %s", str.getFilename(pack_name).c_str());
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
            }
            convertMes2utf8(str.getFilename2(pack_name), file_data, read_length, csv_data, row, col, isDMMFile, isCustom);
            csv.writeStream(csv_file, csv_data, 0, --row, false);
        }
        delete[]file_data;
    }
    fclose(csv_file);
    fclose(in_file);

    //csv.writeFile(csv_data, csv_filename);
    csv.allClear(csv_data);

    delete[]name_array;
    delete[]offset_array;

    if (comp_opt >= 0) msg.echoConvert(csv_filename);
    return 0;
}

int AquaScripts::convertCsv2Sdat(string in_filename, int comp_opt, string tbl_filename)
{
    FILE* in_file, * out_file;
    DWORD total_size, data_size, read_length, index_size, write_buff, d_buff, * offset_array, read_offset[2]{ 0 };
    string sdat_filename, org_filename, pack_name;
    BYTE* file_data;
    bool toDMMFile, isEcho, isDMMFile, isCustom, isConv;
    char hdrbuffer[0x15]{ 0 };
    char* name_array, * file_char;
    int padd_len, padd_len2, errvalue;
    unsigned int struct_offset, name_offset, pack_offset, file_offset, name_length;
    pck_name_struct pck_name_hdr;
    pck_pack_struct pck_pack_hdr;
    dmm_pck_name_struct dmm_pck_name_hdr;
    dmm_pck_pack_struct dmm_pck_pack_hdr;
    map<string, map<int, string>> csvMap;

    sdat_filename = str.replaceExtension(in_filename, ".sdat");
    org_filename = str.replaceExtension(in_filename, ".org");;
    isCustom = false;
    padd_len = 0x8;
    padd_len2 = 0x10;
    name_offset = 0;
    isEcho = comp_opt >= 0;
    toDMMFile = (comp_opt & 4) != 0;
    if (!isEcho) toDMMFile = !toDMMFile;
    if ((comp_opt >= 0) && toDMMFile) msg.echoDMMconvert();

    // open org file
    in_file = fopen(org_filename.c_str(), "rb");
    if (!in_file) return 2; // Failed to read source file

    // open csv file
    errvalue = csv.readFile2Map(in_filename, csvMap);
    if (!errvalue) return 2; // Failed to read source file

    // check out file access
    out_file = fopen(sdat_filename.c_str(), "wb");
    if (!out_file)
    {
        fclose(in_file);
        return 3; // Failed to read target file
    }

    // load file data (backuped binary)
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
            padd_len = 4;
            fread(&dmm_pck_name_hdr, sizeof(dmm_pck_name_struct), 1, in_file);
            bin.endianConvD(dmm_pck_name_hdr.length);
            data_size = dmm_pck_name_hdr.length - sizeof(dmm_pck_name_struct);
        }
        else
        {
            padd_len = 8;
            fread(&pck_name_hdr, sizeof(pck_name_struct), 1, in_file);
            data_size = pck_name_hdr.length - sizeof(pck_name_struct);
        }
    }
    else
    {
        fclose(in_file);
        return 1;
    }
    name_array = new char[data_size];
    fread(name_array, sizeof(char), data_size, in_file);

    //get Pack header
    fseek(in_file, bin.getPaddlen(data_size, padd_len), SEEK_CUR);
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
    fseek(in_file, bin.getPaddlen(index_size, padd_len), SEEK_CUR);

    if (
        (isDMMFile && (strncmp(hdrbuffer, "Pack", 4) != 0)) ||
        (!isDMMFile && (strncmp(hdrbuffer, "Pack                ", 0x14) != 0))
        )
    {
        delete[]name_array;
        delete[]offset_array;
        fclose(in_file);
        return 1;
    }

    // Check convert binary option
    isConv = (toDMMFile ^ isDMMFile);
    if (isConv)
    {
        msg.echoAlert("Option-Script File version Mismatch. Check need to exclude '/P' option.");
        if (!isDMMFile)
        {
            memcpy(&dmm_pck_name_hdr.sig, &pck_name_hdr.sig, 8);
            dmm_pck_name_hdr.length = pck_name_hdr.length - 0xC;
            memcpy(dmm_pck_pack_hdr.sig, pck_pack_hdr.sig, 8);
            dmm_pck_pack_hdr.index_size = pck_pack_hdr.index_size - 0xC;
            dmm_pck_pack_hdr.part_count = pck_pack_hdr.part_count;
        }
        else
        {
            memset(&pck_name_hdr.sig, 0x20, 0x14);
            memcpy(&pck_name_hdr.sig, &dmm_pck_name_hdr.sig, 8);
            pck_name_hdr.length = dmm_pck_name_hdr.length + 0xC;
            memset(&pck_pack_hdr.sig, 0x20, 0x14);
            memcpy(pck_pack_hdr.sig, dmm_pck_pack_hdr.sig, 8);
            pck_pack_hdr.index_size = dmm_pck_pack_hdr.index_size + 0xC;
            pck_pack_hdr.part_count = dmm_pck_pack_hdr.part_count;
        }
    }

    // Write Header
    if (toDMMFile)
    {
        padd_len = 4;
        struct_offset = sizeof(dmm_pck_name_struct);
        name_offset += struct_offset + dmm_pck_pack_hdr.part_count * 4;
        pack_offset = dmm_pck_name_hdr.length;
        pack_offset += bin.getPaddlen(pack_offset, padd_len);
        file_offset = pack_offset + dmm_pck_pack_hdr.index_size;
        file_offset += bin.getPaddlen(file_offset, padd_len2);
        bin.fileZerofill(out_file, file_offset);
        fseek(out_file, 0, SEEK_SET);
        bin.endianConvD(dmm_pck_name_hdr.length);
        bin.endianConvD(dmm_pck_pack_hdr.index_size);
        bin.endianConvD(dmm_pck_pack_hdr.part_count);
        fwrite(&dmm_pck_name_hdr, sizeof(dmm_pck_name_struct), 1, out_file);
        fseek(out_file, pack_offset, SEEK_SET);
        fwrite(&dmm_pck_pack_hdr, sizeof(dmm_pck_pack_struct), 1, out_file);
        pack_offset += sizeof(dmm_pck_pack_struct);
        fseek(out_file, file_offset, SEEK_SET);
    }
    else
    {
        padd_len = 8;
        struct_offset = sizeof(pck_name_struct);
        name_offset = struct_offset + pck_pack_hdr.part_count * 4;
        pack_offset = pck_name_hdr.length;
        pack_offset += bin.getPaddlen(pack_offset, padd_len);
        file_offset = pack_offset + pck_pack_hdr.index_size;
        file_offset += bin.getPaddlen(file_offset, padd_len);
        bin.fileZerofill(out_file, file_offset);
        fseek(out_file, 0, SEEK_SET);
        fwrite(&pck_name_hdr, sizeof(pck_name_struct), 1, out_file);
        fseek(out_file, pack_offset, SEEK_SET);
        fwrite(&pck_pack_hdr, sizeof(pck_pack_struct), 1, out_file);
        pack_offset += sizeof(pck_pack_struct);
        fseek(out_file, file_offset, SEEK_SET);
    }

    // check DMM::Non-japanese scripts
    if (toDMMFile && (str.checkSJIS(csvMap["P00110"][0])) && ((str.pathChecker(tbl_filename)) == 0))
    {
        str.setTableFile(tbl_filename);
        isCustom = true;
        if (comp_opt >= 0)
            msg.echoAlert("Custom table convert mode is enabled.");
    }

    // edit and repack file data
    for (int i = 0; i < data_size; i++)
    {
        // read packed file
        memcpy(read_offset, &name_array[i * 4], 4);
        if (isDMMFile) bin.endianConvD(read_offset[0]);
        file_char = name_array + read_offset[0];
        pack_name = static_cast<string>(reinterpret_cast<const char*>(file_char));
        if (toDMMFile) pack_name = str.toUpper(pack_name);
        else pack_name = str.toUpperFirst(pack_name);
        memcpy(read_offset, offset_array + i * 2, 8);
        if (isDMMFile) { bin.endianConvD(read_offset[0]); bin.endianConvD(read_offset[1]); }
        read_length = read_offset[1];
        fseek(in_file, read_offset[0], SEEK_SET);
        file_data = new BYTE[read_length];
        memset(file_data, 0, read_length);
        fread(file_data, 1, read_length, in_file);

        // edit file
        if (strncmp(str.toLower(str.getExtension(pack_name)).c_str(), ".flag", 5) != 0)
        {
            if (comp_opt >= 0)
            {
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
                printf("\r[*] Process  :  %s", str.getFilename(pack_name).c_str());
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
            }
            updateMesStrings(&str.getFilename2(pack_name)[0], file_data, read_length, csvMap, toDMMFile, isConv, isCustom);
        }

        // write filename offset
        fseek(out_file, struct_offset + i * 4, SEEK_SET);
        name_length = pack_name.length();
        write_buff = name_offset - struct_offset;
        if (toDMMFile) bin.endianConvD(write_buff);
        fwrite(&write_buff, 4, 1, out_file);

        // write filename
        fseek(out_file, name_offset, SEEK_SET);
        const char* name_array = pack_name.c_str();
        fwrite(name_array, sizeof(char), name_length, out_file);
        name_offset += pack_name.length() + 1;

        // write file offset
        fseek(out_file, pack_offset + i * 8, SEEK_SET);
        write_buff = file_offset;
        if (toDMMFile) bin.endianConvD(write_buff);
        fwrite(&write_buff, 4, 1, out_file);
        write_buff = read_length;
        if (toDMMFile) bin.endianConvD(write_buff);
        fwrite(&write_buff, 4, 1, out_file);

        // write file
        fseek(out_file, 0, SEEK_END);
        fwrite(file_data, read_length, 1, out_file);

        // write padding
        bin.fileZerofill(out_file, bin.getPaddlen(read_length, padd_len2));
        file_offset = ftell(out_file);

        delete[]file_data;
    }
    padd_len = bin.getPaddlen(file_offset - padd_len, 0x10);
    bin.fileZerofill(out_file, padd_len);

    fclose(in_file);
    fclose(out_file);

    delete[]name_array;
    delete[]offset_array;
    csvMap.clear();

    if (comp_opt >= 0) msg.echoConvert(sdat_filename);
    return 0;
}