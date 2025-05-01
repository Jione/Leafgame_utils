#include "AquaLZ77.h"

void LZ77::kmp_match(BYTE*& raw_data, int& total_len, int& code_pos, bool& comp_flg)
{
    //initialize kmp variables
    int opt_len = 1;
    int kmp_index1 = 1, kmp_index2 = 0;
    int lookahead_len = total_len - code_pos + 1;
    if (lookahead_len > kmp_max)
    {
        lookahead_len = kmp_max;
    }
    else if (lookahead_len < 0)
    {
        lookahead_len = 0;
    }
    for (int i = 0; i < kmp_max; i++) kmp_table[i] = NULL;

    // make kmp searching table
    while (kmp_index1 < lookahead_len)
    {
        if (raw_data[(code_pos + kmp_index1)] == raw_data[(code_pos + kmp_index2)])
        {
            kmp_table[kmp_index1++] = ++kmp_index2;
        }
        else if (kmp_index2 > 0)
        {
            kmp_index2 = kmp_table[(kmp_index2 - 1)];
        }
        else
        {
            kmp_table[kmp_index1++] = 0;
        }
    }

    // search window buffer
    if (code_pos < window) kmp_index1 = code_pos;
    else kmp_index1 = window;
    kmp_best_pos = kmp_best_len = kmp_index2 = 0;

    //while (kmp_index1 > 0)
    if (comp_flg) opt_len = lookahead_len;
    while (kmp_index1 > (1 - opt_len))
    {
        if (raw_data[(code_pos + kmp_index2)] == raw_data[(code_pos - kmp_index1)])
        {
            kmp_index1--;
            kmp_index2++;
            if (kmp_index2 > kmp_best_len)
            {
                kmp_best_pos = kmp_index1;
                kmp_best_len = kmp_index2;
                if (kmp_index2 >= lookahead_len)
                    kmp_index2 = ~kmp_index1 - 1;
            }
        }
        else if (kmp_index2 > 0)
        {
            kmp_index2 = kmp_table[(kmp_index2 - 1)];
        }
        else
        {
            kmp_index1--;
        }

        if ((kmp_index1 + kmp_index2) <= 0)
            kmp_index1 = ~opt_len;
    }
    return;
}

// LZ77 none compress mode
void LZ77::NoCompress(BYTE* raw_data, int raw_size, lz77_struct* output_struct)
{
    int flag_length = raw_size >> 3;
    BYTE* lz77_data = new BYTE[flag_length + raw_size];
    memset(lz77_data, 0, flag_length + raw_size);
    memcpy(lz77_data + flag_length, raw_data, raw_size);

    memcpy(output_struct->sig, "LZ77", 4);
    output_struct->total_len = raw_size;
    output_struct->flag_count = raw_size;
    output_struct->data_offset = flag_length + lz77_hdrsize;
    output_struct->compress_data = lz77_data;
    output_struct->compress_size = raw_size + flag_length + lz77_hdrsize;
}

/*
    flgopt: 0 = Default mode, 1 = Optimized mode, 2 = No Compress mode
    If flgopt is a negative value, proceed to echo-off mode.
*/
void LZ77::Compress(BYTE* raw_data, int raw_size, lz77_struct* output_struct, int flgopt)
{
    // parsing flag option
    bool echo_flg, comp_flg, nocomp_flg;
    echo_flg = flgopt >= 0;
    comp_flg = (flgopt & 1) != 0;
    nocomp_flg = (flgopt & 2) != 0;
    if (!echo_flg) { comp_flg = !comp_flg; nocomp_flg = !nocomp_flg; }
    if (nocomp_flg) { NoCompress(raw_data, raw_size, output_struct); return; }

    // initialize lz77 data variables
    int progress, percent, i;
    int code_pos = 0;
    int data_length = 0;
    int flag_count = 0;
    int flag_length = 0;
    BYTE flags = 0;
    BYTE* data_buff = new BYTE[raw_size];
    BYTE* flag_buff = new BYTE[(raw_size >> 3)];
    memset(data_buff, 0, raw_size);
    memset(flag_buff, 0, (raw_size >> 3));
    memset(kmp_table, 0, kmp_max);

    if (raw_size > 102400) progress = raw_size / 100;
    else progress = raw_size;

    if (echo_flg) printf("\r0%% Compressed...");
    while (code_pos < raw_size)
    {
        flag_count++;
        kmp_match(raw_data, raw_size, code_pos, comp_flg);
        if (kmp_best_len > kmp_min)
        {
            data_buff[data_length++] = kmp_best_pos + kmp_best_len;
            data_buff[data_length++] = kmp_best_len - kmp_min - 1;
            code_pos += kmp_best_len;
            flags++;
        }
        else
            data_buff[data_length++] = raw_data[code_pos++];

        if ((flag_count % 8) == 0)
        {
            flag_buff[flag_length++] = flags;
            flags = 0;
        }
        else
            flags = flags << 1;

        if (echo_flg)
        {
            if ((code_pos % progress) == 0)
            {
                percent = code_pos / progress;
                printf("\r%d%% Compressed...", percent);
            }
        }
    }
    if (echo_flg) printf("\r100%% Compressed...\r");

    if ((flag_count % 8) != 0)
    {
        for (i = flag_count % 8; i < 7; i++) flags = flags << 1;
        flag_buff[flag_length++] = flags;
    }

    BYTE* lz77_data = new BYTE[flag_length + data_length];
    memset(lz77_data, 0, flag_length + data_length);
    memcpy(lz77_data, flag_buff, flag_length);
    memcpy(&lz77_data[flag_length], data_buff, data_length);

    memcpy(output_struct->sig, "LZ77", 4);
    output_struct->total_len = raw_size;
    output_struct->flag_count = flag_count;
    output_struct->data_offset = flag_length + lz77_hdrsize;
    output_struct->compress_data = lz77_data;
    output_struct->compress_size = data_length + flag_length;

    delete[]flag_buff;
    delete[]data_buff;
}

void LZ77::Decompress(lz77_struct* input_struct, BYTE*& raw_data)
{
    BYTE* lz77_data = input_struct->compress_data;
    int lz77_len = input_struct->compress_size;
    int raw_size = input_struct->total_len;
    raw_data = new BYTE[raw_size];
    memset(raw_data, 0, raw_size);

    //Initalize decompress buffer various
    BYTE flag = 0;
    int flag_seek = 0;
    int data_seek = input_struct->data_offset - lz77_hdrsize;
    int raw_seek = 0;
    int rewind_pos, match_len;
    DWORD flag_count = input_struct->flag_count;

    //Decompress LZ77
    for (int i = 0; i < flag_count; i++)
    {
        if (i % 8 == 0)
        {
            flag = lz77_data[i % 8 + flag_seek++];
        }

        if (flag & 0x80)
        {
            rewind_pos = lz77_data[data_seek++];
            for (match_len = lz77_data[data_seek++] + 3; match_len > 0; match_len--)
                raw_data[raw_seek] = raw_data[raw_seek++ - rewind_pos];
        }
        else raw_data[raw_seek++] = lz77_data[data_seek++];

        flag = flag << 1;

        if (data_seek >= lz77_len) i = flag_count;
    }
}
