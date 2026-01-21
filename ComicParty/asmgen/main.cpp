/*
    Project: ComicKR Patcher
    Environment: Visual Studio 2022 (C++)
    Target: 32-bit PE File (ImageBase: 0x400000)
*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <map>

// =============================================================
// [¼³Á¤ ¹× ·ÎÁ÷ ¿µ¿ª]
// =============================================================

#define TARGET_FILENAME  "Comic_v7.exe"
#define OUTPUT_FILENAME  "Comic_v8.exe"
#define IMAGE_BASE       0x400000

// ÆÄÀÏ µ¥ÀÌÅÍ¸¦ ÀúÀåÇÒ Àü¿ª ¹öÆÛ
std::vector<uint8_t> fileBuffer;

// C ¼Ò½º ÄÚµåÀÇ º¯¼ö ÁÖ¼Ò(key)¿Í EXE ³»ºÎÀÇ È®Á¤µÈ °¡»ó ÁÖ¼Ò(value)¸¦ ¸ÅÇÎ
std::map<void*, uint32_t> addressMap;

static int alignSize = 0;
static int checkString = 1;

// °¡»ó ÁÖ¼Ò(VA)¸¦ ÆÄÀÏ ¿ÀÇÁ¼Â(Offset)À¸·Î º¯È¯
uint32_t VAToOffset(uint32_t va) {
    return va - IMAGE_BASE;
}

// ¹®ÀÚ¿­ º¯È¯/°Ë»ç ÇÔ¼ö (¿äÃ»»çÇ× ¹Ý¿µ)
// input: ¿øº» ¹®ÀÚ¿­, output: ÆÄÀÏ¿¡ ¾µ ¹öÆÛ, size: ¹öÆÛ Å©±â
void TransformString(const char* input, char* output, int maxSize) {
    // ¿¹½Ã: ´Ü¼øÈ÷ º¹»ç (ÇÊ¿ä ½Ã ÀÎÄÚµù º¯È¯ ·ÎÁ÷ Ãß°¡ °¡´É)
    // CP949 -> UTF8 º¯È¯ µîÀ» ¼öÇàÇÒ ¼ö ÀÖ´Â °ø°£ÀÔ´Ï´Ù.
    int len = strlen(input);
    strcpy(output, input);
    if (checkString) {
        for (int i = 0; i < len; i++) {
            unsigned char c = input[i];
            if (0x7F < c) {
                if ((c < 0xA1) || (c > 0xDF)) {
                    printf("Check String: %s\n", input);
                    break;
                }
                c = input[++i];
                if (c < 0xA1) {
                    printf("Check String: %s\n", input);
                    break;
                }
            }
        }
    }
    unsigned char c = output[(len - 1)];
    if ((c < 0x80) && (c != 0x0) && !(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))) {
        output[(len - 1)] = (char)((c >> 4) | 0xB0);
        output[len] = (char)((c & 0xF) | 0x70);
//        output[(len + 1)] = '\1';
//        output[(len + 2)] = '\0';
        output[(len + 1)] = '\0';
    }
}

// 1. ¹®ÀÚ¿­ ¹­À½ Ã³¸® ÇÔ¼ö (¹®ÀÚ¿­ ¹èÄ¡ ¹× ÁÖ¼Ò µî·Ï)
void ProcessStringBlock(uint32_t startVA, int maxBlockSize, char** stringList, int count) {
    uint32_t currentOffset = VAToOffset(startVA);
    uint32_t startOffset = currentOffset;
    uint32_t endOffsetLimit = startOffset + maxBlockSize;

    printf("[Block 0x%X] Processing %d strings...\n", startVA, count);

    for (int i = 0; i < count; i++) {
        // ÇöÀç ¹®ÀÚ¿­ÀÌ ¾²ÀÏ EXE ³»ÀÇ °¡»ó ÁÖ¼Ò °è»ê
        uint32_t currentVA = IMAGE_BASE + currentOffset;

        // ¸ÅÇÎ Å×ÀÌºí¿¡ µî·Ï (¼Ò½ºÄÚµåÀÇ unk_ º¯¼ö ÁÖ¼Ò -> EXEÀÇ VA)
        addressMap[stringList[i]] = currentVA;

        // º¯È¯ ¹× ¹öÆÛ ÁØºñ
        char tempBuffer[8192] = { 0 };
        TransformString(stringList[i], tempBuffer, sizeof(tempBuffer));
        size_t len = strlen(tempBuffer) + 1; // Null Æ÷ÇÔ
        if ((0 < alignSize) && (alignSize < len)) {
            printf("\nCheck Index: %d", i);
        }
        if ((0 < alignSize) && (len % alignSize)) {
            len += alignSize - (len % alignSize);
        }
        // ¹üÀ§ °Ë»ç
        if (currentOffset + len > endOffsetLimit) {
            printf("Error: Block size exceeded at 0x%X! (Index:%d)\n", startVA, i);
            exit(1);
        }

        // ÆÄÀÏ ¹öÆÛ¿¡ ¾²±â
        memcpy(&fileBuffer[currentOffset], tempBuffer, len);
        currentOffset += len;
    }

    // ³²Àº °ø°£ 0À¸·Î Ã¤¿ì±â (Padding)
    if (currentOffset < endOffsetLimit) {
        memset(&fileBuffer[currentOffset], 0, endOffsetLimit - currentOffset);
    }
}

// 2. È£Ãâ ÁÖ¼Ò(Æ÷ÀÎÅÍ) ¾÷µ¥ÀÌÆ® ÇÔ¼ö
void UpdateReferences(uint32_t tableStartVA, char** targetList, int count) {
    uint32_t currentOffset = VAToOffset(tableStartVA);

    printf("[Ref 0x%X] Updating %d pointers...\n", tableStartVA, count);

    for (int i = 0; i < count; i++) {
        char* targetVar = targetList[i];

        // ¸ÅÇÎ Å×ÀÌºí¿¡¼­ ÇØ´ç ¹®ÀÚ¿­ÀÇ »õ·Î¿î ÁÖ¼Ò °Ë»ö
        if (addressMap.find(targetVar) == addressMap.end()) {
            printf("Error: String reference not found in map! Index: %d\n", i);
            // ¾ÈÀüÀ» À§ÇØ °è¼Ó ÁøÇàÇÏ°Å³ª Á¾·á
            currentOffset += 4;
            continue;
        }

        uint32_t newVA = addressMap[targetVar];

        // EXE ÆÄÀÏ ³»ÀÇ Æ÷ÀÎÅÍ °ª ¾÷µ¥ÀÌÆ® (Little Endian)
        // 32ºñÆ® Æ÷ÀÎÅÍ ¾²±â
        uint32_t* ptrLoc = (uint32_t*)&fileBuffer[currentOffset];
        *ptrLoc = newVA;

        currentOffset += 4; // Æ÷ÀÎÅÍ Å©±â¸¸Å­ ÀÌµ¿
    }
}

// =============================================================
// DATA AREA
// =============================================================

static char unk_461764[] = "£¿£¿£¿";
static char unk_46176C[] = "³îÀÌ°ø¿ø 2";
static char unk_461778[] = "°Å¸®";
static char unk_46177C[] = "ºÒ²É³îÀÌ¡¡¹ã";
static char unk_461788[] = "ÇÏ´Ã¡¡¹ã¡¡´«";
static char unk_461794[] = "ÇÏ´Ã¡¡³·";
static char unk_46179C[] = "¿¡ÀÌ¹ÌÀÇ ¹æ¡¡¹ã";
static char unk_4617AC[] = "¿ø·ë ¸Ç¼Ç¡¡¹ã¡¡´«(»ç´Ù¸® ÀÖÀ½)";
static char unk_4617D8[] = "¿ø·ë ¸Ç¼Ç¡¡¹ã¡¡´«";
static char unk_4617F8[] = "¸¸È­ Ä«Æä";
static char unk_461804[] = "´Ù¹æ";
static char unk_46180C[] = "ÁÖÀÎ°ø ¼­Å¬¡¡ÀÌº¥Æ® Áß¡¡Ã¥»ó£«¼Õ´Ô";
static char unk_461834[] = "ÁÖÀÎ°ø ¼­Å¬¡¡ÁØºñ Áß¡¡Ã¥»ó¸¸";
static char unk_461858[] = "ÁÖÀÎ°ø ¹æ(¾îµÎ¿ò)";
static char unk_461870[] = "ÁÖÀÎ°ø ¹æ(¹ã)";
static char unk_461884[] = "ÄÚ¹ÌÄÉ È¸Àå£¯Åë·Î¡¡ÁØºñ Áß¡¡¾Æ¹«µµ ¾øÀ½";
static char unk_4618AC[] = "ÄÚ¹ÌÄÉ È¸Àå£¯ÄÚ½ºÇÁ·¹ °ø°£¡¡ÁØºñ Áß¡¡µå¹®µå¹® ÁØºñÇÏ´Â »ç¶÷";
static char unk_4618E8[] = "ÄÚ¹ÌÄÉ È¸Àå£¯¼ÅÅÍ ¾Õ¡¡ÁØºñ Áß¡¡Ã¥»ó+µå¹®µå¹® ÁØºñÇÏ´Â »ç¶÷";
static char unk_461924[] = "ÄÚ¹ÌÄÉ È¸Àå£¯¼­Å¬¡¡ÁØºñ Áß¡¡Ã¥»ó+µå¹®µå¹® ÁØºñÇÏ´Â »ç¶÷";
static char unk_46195C[] = "ÄÚ¹ÌÄÉ È¸Àå£¯¼­Å¬¡¡Àü³¯ ¼³Ä¡¡¡µå¹®µå¹® ÁØºñÇÏ´Â »ç¶÷";
static char unk_461990[] = "Èò»ö";
static char unk_461994[] = "°ËÀº»ö";
static char unk_461998[] = "ÄÚ¹ÌÄÉ È¸Àå£¯¼ÅÅÍ ¾Õ¡¡ÀÌº¥Æ® Áß¡¡¼Õ´Ô ÆÐÅÏ 2";
static char unk_4619CC[] = "ÄÚ¹ÌÄÉ È¸Àå£¯¼ÅÅÍ ¾Õ¡¡ÀÌº¥Æ® Áß¡¡¼Õ´Ô ÆÐÅÏ 1";
static char unk_461A04[] = "¹ÌÁîÅ°ÀÇ ¹æ";
static char unk_461A10[] = "¿ÂÃµ ¿©°ü ¿Ü°ü";
static char unk_461A20[] = "ÀÎ¼â¼Ò ¿Ü°ü";
static char unk_461A2C[] = "ÄÚ¹ÌÄÉ È¸Àå£¯Åë·Î¡¡ÀÌº¥Æ® Áß¡¡¼Õ´Ô ÀÖÀ½";
static char unk_461A54[] = "½Å»ç";
static char unk_461A5C[] = "ÄÚ¹ÌÄÉ È¸Àå£¯ÄÚ½ºÇÁ·¹ °ø°£¡¡ÀÌº¥Æ® Áß¡¡ÄÚ½º ÇÃ·¹ÀÌ¾î";
static char unk_461A98[] = "³îÀÌ°ø¿ø ³»ºÎ";
static char unk_461AA4[] = "µµÄì µ¼¡¤°í¶óÄí¿£ ³îÀÌ°ø¿ø";
static char unk_461AC0[] = "¼îÇÎ¸ô";
static char unk_461AD8[] = "¿¡ÀÌ¹ÌÀÇ ¹æ";
static char unk_461AE4[] = "¿À»çÄ« °Å¸® Ç³°æ";
static char unk_461AF4[] = "¾ß½ÃÀå";
static char unk_461AFC[] = "´«±æ";
static char unk_461B04[] = "¿ÂÃµ ¿©°ü °´½Ç(ÀåÁö¹® °ãÄ§)¡¡¹ã";
static char unk_461B24[] = "¿ÂÃµ ¿©°ü °´½Ç";
static char unk_461B34[] = "´ëÇÐ °­ÀÇ½Ç";
static char unk_461B40[] = "¿ø·ë ¸Ç¼Ç¡¡Àú³á";
static char unk_461B5C[] = "¿ø·ë ¸Ç¼Ç¡¡¹ã¡¡ºñ";
static char unk_461B7C[] = "¿ø·ë ¸Ç¼Ç¡¡¹ã";
static char unk_461B98[] = "¿ø·ë ¸Ç¼Ç¡¡³·¡¡ºñ";
static char unk_461BB8[] = "¿ø·ë ¸Ç¼Ç¡¡³·";
static char unk_461BD4[] = "¿ª¾Õ ±¤Àå¡¡Å©¸®½º¸¶½º(´«)";
static char unk_461BF0[] = "¿ª¾Õ ±¤Àå¡¡Àú³á";
static char unk_461C00[] = "¿ª¾Õ ±¤Àå¡¡¹ã";
static char unk_461C10[] = "¿ª¾Õ ±¤Àå¡¡³·¡¡ºñ";
static char unk_461C24[] = "¿ª¾Õ ±¤Àå¡¡³·";
static char unk_461C34[] = "¹Ù´Ù";
static char unk_461C38[] = "È£ÅÚ °´½Ç ¾È";
static char unk_461C48[] = "°ø¿ø ¾È¡¡°Ü¿ï¡¡Àú³á";
static char unk_461C5C[] = "°ø¿ø ¾È¡¡°¡À»¡¡Àú³á";
static char unk_461C70[] = "°ø¿ø ¾È¡¡º½¿©¸§¡¡Àú³á";
static char unk_461C84[] = "°ø¿ø ¾È¡¡°Ü¿ï¡¡¹ã";
static char unk_461C94[] = "°ø¿ø ¾È¡¡°¡À»¡¡¹ã";
static char unk_461CA4[] = "°ø¿ø ¾È¡¡º½¿©¸§¡¡¹ã";
static char unk_461CB8[] = "°ø¿ø ¾È¡¡°Ü¿ï¡¡³·";
static char unk_461CC8[] = "°ø¿ø ¾È¡¡°¡À»¡¡³·";
static char unk_461CD8[] = "°ø¿ø ¾È¡¡º½¿©¸§¡¡³·";
static char unk_461CEC[] = "¿ª ½Â°­Àå¡¡¹ã";
static char unk_461CFC[] = "¿ª ½Â°­Àå¡¡³·¡¡ºñ";
static char unk_461D10[] = "¿ª ½Â°­Àå¡¡³·";
static char unk_461D20[] = "ÀÎ¼â¼Ò ³»ºÎ";
static char unk_461D2C[] = "°ÔÀÓ¼¾ÅÍ ¸ÅÀå ¾È";
static char unk_461D40[] = "´ëÇÐ ÃàÁ¦ Ç³°æ";
static char unk_461D4C[] = "´ëÇÐ ±³³»";
static char unk_461D58[] = "ÄÚ¹ÌÄÉ ÁØºñÈ¸ »ç¹«¼Ò";
static char unk_461D6C[] = "Ä³¸¯ÅÍ ¼¥ ¸ÅÀå ¾È";
static char unk_461D88[] = "È­¹æ ¸ÅÀå ¾È";
static char unk_461D94[] = "ÄÚ¹ÌÄÉ È¸Àå£¯¼­Å¬¡¡ÀÌº¥Æ® Áß¡¡¼Õ´Ô ÆÐÅÏ 2";
static char unk_461DC4[] = "ÄÚ¹ÌÄÉ È¸Àå£¯¼­Å¬¡¡ÀÌº¥Æ® Áß¡¡¼Õ´Ô ÆÐÅÏ 1";
static char unk_461DF4[] = "ÄÚ¹ÌÄÉ È¸Àå(Áß)¡¡Àú³á";
static char unk_461E0C[] = "ÄÚ¹ÌÄÉ È¸Àå(Áß)¡¡ºñ";
static char unk_461E24[] = "ÄÚ¹ÌÄÉ È¸Àå(Áß)";
static char unk_461E38[] = "ÄÚ¹ÌÄÉ È¸Àå(´ë)¡¡Àú³á";
static char unk_461E50[] = "ÄÚ¹ÌÄÉ È¸Àå(´ë)";
static char unk_461E64[] = "ÁÖÀÎ°øÀÇ ¹æ";
static char unk_461E74[] = "È÷·Î½º¿¡ÀÇ ¾ó±¼";
static int size_461764 = 0x71C;
static char* strings_461764[] = {
    unk_461764, unk_46176C, unk_461778, unk_46177C, unk_461788, unk_461794, unk_46179C, unk_4617AC,
    unk_4617D8, unk_4617F8, unk_461804, unk_46180C, unk_461834, unk_461858, unk_461870, unk_461884,
    unk_4618AC, unk_4618E8, unk_461924, unk_46195C, unk_461990, unk_461994, unk_461998, unk_4619CC,
    unk_461A04, unk_461A10, unk_461A20, unk_461A2C, unk_461A54, unk_461A5C, unk_461A98, unk_461AA4,
    unk_461AC0, unk_461AD8, unk_461AE4, unk_461AF4, unk_461AFC, unk_461B04, unk_461B24, unk_461B34,
    unk_461B40, unk_461B5C, unk_461B7C, unk_461B98, unk_461BB8, unk_461BD4, unk_461BF0, unk_461C00,
    unk_461C10, unk_461C24, unk_461C34, unk_461C38, unk_461C48, unk_461C5C, unk_461C70, unk_461C84,
    unk_461C94, unk_461CA4, unk_461CB8, unk_461CC8, unk_461CD8, unk_461CEC, unk_461CFC, unk_461D10,
    unk_461D20, unk_461D2C, unk_461D40, unk_461D4C, unk_461D58, unk_461D6C, unk_461D88, unk_461D94,
    unk_461DC4, unk_461DF4, unk_461E0C, unk_461E24, unk_461E38, unk_461E50, unk_461E64, unk_461E74
};
static char* def_4610E8[] = {
    unk_461E74, unk_461E64, unk_461E50, unk_461E38, unk_461E24, unk_461E0C, unk_461DF4, unk_461DC4,
    unk_461D94, unk_461D88, unk_461D6C, unk_461D58, unk_461D4C, unk_461D40, unk_461D2C, unk_461D20,
    unk_461D10, unk_461CFC, unk_461CEC, unk_461CD8, unk_461CC8, unk_461CB8, unk_461CA4, unk_461C94,
    unk_461C84, unk_461C70, unk_461C5C, unk_461C48, unk_461C38, unk_461C34, unk_461C24, unk_461C10,
    unk_461C00, unk_461BF0, unk_461BD4, unk_461BB8, unk_461B98, unk_461B7C, unk_461B5C, unk_461B40,
    unk_461B34, unk_461B24, unk_461B04, unk_461AFC, unk_461AF4, unk_461AE4, unk_461AD8, unk_461AC0,
    unk_461AA4, unk_461A98, unk_461A5C, unk_461A54, unk_461A2C, unk_461A20, unk_461A10, unk_461A04,
    unk_4619CC, unk_461998, unk_461994, unk_461990, unk_46195C, unk_461924, unk_4618E8, unk_4618AC,
    unk_461884, unk_461870, unk_461858, unk_461834, unk_46180C, unk_461804, unk_4617F8, unk_4617D8,
    unk_4617AC, unk_46179C, unk_461794, unk_461788, unk_46177C, unk_461778, unk_46176C, unk_461764,
    unk_461764, unk_461764, unk_461764
};



static int size_46B840 = 0xC;
static char* strings_46B840[] = { "KoS.pak" };
static int size_46B864 = 0xC;
static char* strings_46B864[] = { "KoR.pak" };

static char unk_462AA0[] = "Ãµ»ç¿Í ¾Ç¸¶ÀÇ ÁÂ´ãÈ¸";
static char unk_462AB4[] = "¼¼±â¸» ÆÐ¿Õ? ";
static char unk_462AC4[] = "¿Â±â";
static char unk_462AD0[] = "³Ê¸¸Àº ÁöÅ°°í ½Í¾î";
static char unk_462AE8[] = "Àú, ¹èÅÍ¸®°¡ ´Ù µÉ °Í °°¾Æ¿ä¡¦";
static char unk_462B04[] = "£í£ù £ç£ò£á£ä£õ£á£ô£é£ï£î";
static char unk_462B20[] = "¼ö°íÇß¾î";
static char unk_462B30[] = "»õ¿ìµî ÇÏÀÌ Á¡ÇÁ! ";
static char unk_462B4C[] = "ÈÇ¸¢ÇÑ ¹èÆ®³×¿ä";
static char unk_462B64[] = "ÁÙ¹«´Ì ÆÒÆ¼·Î ¾á¾á¾á";
static char unk_462B84[] = "ÀÛÀº ¿ë±â";
static char unk_462B90[] = "»©¾ÑÀº ÀÔ¼ú";
static char unk_462B9C[] = "ÀÌ°Ô ¼Ò¹®³­ ÇÑ ¹úÀÌÁÒ. ";
static char unk_462BB4[] = "³Î Ä¡À¯ÇØ ÁÖ°í ½Í¾î";
static char unk_462BC8[] = "ÀÌ ¿ìÀ¯, ¾È ³ì´Â±º¿ä. ";
static char unk_462BE8[] = "£Í£Ï£Ï£Î";
static char unk_462BF4[] = "¾È Ãß¿ö¿ä. Àú, °¡³­ÇÏ´Ï±î¿ä. ";
static char unk_462C14[] = "¾Æ¿ì¿ì¢¦";
static char unk_462C20[] = "ÂøÇÑ ¾ÆÀÌÀÇ µµ¿ì¹Ì";
static char unk_462C34[] = "Àú, Àú¡¦ ÀÌ·± °÷ Ã³À½ÀÌ¿¡¿ä. ";
static char unk_462C5C[] = "»ç¶û µµ½Ã¶ôÀº °ö»©±â·Î";
static char unk_462C70[] = "Ä¡»ç¸¦ Ã£¾Æ¶ó! ";
static char unk_462C80[] = "¾Æ¾ß¾ß, Çã¸® »ß¾ú¾î. ";
static char unk_462C9C[] = "£Â£å £È£á£ð£ð£ù";
static char unk_462CAC[] = "ºûÀÌ ºñÄ¡´Â ÂÊÀ¸·Î";
static char unk_462CBC[] = "¿ì¸® »ç¶ûÀÇ ÁõÇ¥";
static char unk_462CCC[] = "¼Õ¿¡ ³ÖÀº ¼ÒÁßÇÑ °Í";
static char unk_462CE4[] = "ÀÒ¾î¹ö¸± »·ÇÑ ¼ÒÁßÇÑ °Í";
static char unk_462CFC[] = "ÄÚ½ºÇÁ·¹ Âü°¡ÀÚ·Î¼­ÀÇ ÀÚ°¢";
static char unk_462D18[] = "³ª¡¦¡¦ ÁÁ¾ÆÇÏ´Â »ç¶÷ÀÌ¡¦¡¦";
static char unk_462D34[] = "·¹ÀÌÄÚÀÇ ºñ¹Ð!? ";
static char unk_462D48[] = "¶ß°Å¿öÁ®¶ó";
static char unk_462D54[] = "ÁßÈ­ ¼Ò³à";
static char unk_462D5C[] = "ÀçÈ¸´Â ¿¡·ÎÆ½ÇÏ°Ô";
static char unk_462D70[] = "¸¸³²Àº È£È­·Ó°Ô";
static char unk_462D88[] = "´ç½Å°ú ÇÔ²²¡¦";
static char unk_462D9C[] = "±×·£µå ÇÇ³¯·¹";
static char unk_462DB0[] = "Àå°©Àº ¾ÆÀÌµ¹ÀÇ ¿¹ÀÇ";
static char unk_462DCC[] = "Ãã µ¿ÀÛÀº ÀÌ·¸°Ô·Á³ª? ";
static char unk_462DEC[] = "¿ì¼± ¹ß¼º ¿¬½ÀºÎÅÍ¾ß";
static char unk_462E00[] = "¾È³çÀº ³ë·§¼Ò¸®¿¡ ½Ç¾î¼­";
static char unk_462E18[] = "Àú¹«´Â ¼®¾ç ¼Ó¿¡¼­";
static char unk_462E2C[] = "Å©¸®½º¸¶½º, °ø¿ø¿¡¼­¡¦";
static char unk_462E44[] = "°¡±õ°íµµ ¸Õ µÎ »ç¶÷";
static char unk_462E54[] = "¾Æ»çÈ÷°¡ ³ë·¡ÇÕ´Ï´Ù! ";
static char unk_462E68[] = "¾ÆÀÌµ¹ £Ë£É£Ó£Ó";
static char unk_462E7C[] = "ÀÇ¿Ü·Î ¸ÀÀÖ¾î¢¦";
static char unk_462E8C[] = "¾ðÁ¨°¡ÀÇ ¸Þ¸® Å©¸®½º¸¶½º";
static char unk_462EA8[] = "Á¨Àå! ÆÐµå¶ó¸é ¾È Áú ÅÙµ¥. ";
static char unk_462ED0[] = "°¨ÀÚÆ¢±èÀº ¾î¶°¼¼¿ä? ";
static char unk_462EE4[] = "ÈÞ½Ä";
static char unk_462EF0[] = "³»°¡ ºû³¯ Â÷·Ê´Ù, ÄíÈ¥ºÎÃ÷! ";
static char unk_462F08[] = "ÀÌ°Å ¹ÌÆÃÀÌ¾ß? ";
static char unk_462F1C[] = "»ý¾ó ¾Æ»çÈ÷Â¯";
static char unk_462F30[] = "µÑÀÌ¼­ µµÇÇÇà°¢";
static char unk_462F40[] = "¿À, ¿À, ¿À, ¿À, ¿À·»Áö ÁÖ, ½º. ";
static char unk_462F68[] = "¿µ¿øÀÇ Çàº¹À»¡¦¡¦";
static char unk_462F7C[] = "¸Â´êÀº ¸¶À½";
static char unk_462F8C[] = "¸¶¹«¸®´Â ±â½ÂÀ§·Î";
static char unk_462FA0[] = "¾Æ¾ß, Á÷±¸ ½ÂºÎ";
static char unk_462FB0[] = "³ª, ¹þ¾îµµ ´ë´ÜÇØ¿ä";
static char unk_462FCC[] = "µî ³Ê¸Ó·Î ÀüÇØÁö´Â ¼¾Æ¼¸àÅÐ";
static char unk_462FE8[] = "£Ò£Á£É£Î £×£Ï£Í£Á£Î";
static char unk_463000[] = "¾Æ¹öÁöÀÇ ¼Ò¿ø, ¾Æ¾ßÀÇ ¸¶À½";
static char unk_463014[] = "£Ö£É£Ò£Ç£É£Î £Ë£É£Ó£Ó";
static char unk_46302C[] = "´ÜÇ³ ¼Ó¿¡¼­";
static char unk_463038[] = "¾Ñ¡¦";
static char unk_463040[] = "¿ÃÇØ´Â ÁÁÀº ÀÏ ÀÖÀ»±î³Ä¢¦? ";
static char unk_463060[] = "ÀÌ ´Ù¸®´Â ²ÎÀÌ³×, ÀÌ°Ç ±³Ã¼´Ù. ";
static char unk_463084[] = "ÀÌ°Ç ¸ÔÀ» ¼ö ¾ø½À´Ï´Ù, ÀÌ°Íµµ ¾È µË´Ï´Ù. ";
static char unk_4630B0[] = "ÁøÁöÇÑ ½ÂºÎ";
static char unk_4630BC[] = "ÈÊ¡¦¡¦";
static char unk_4630C8[] = "±×·¸°Ô ³ª¸¦ ÀÇ½ÄÇÏ±â ½ÃÀÛÇÏ´Â °Å¾ß. ";
static char unk_4630E8[] = "½Å°æ ¾²ÀÌ´Â ±× ¾ÆÀÌ";
static char unk_4630F8[] = "ºÒ½ÖÇÑ ³ª";
static char unk_463104[] = "¿µ±¤À¸·Î °¡´Â ±æ";
static char unk_463110[] = "±× °¨µ¿À» ´Ù½Ã ÇÑ ¹ø";
static char unk_463124[] = "ÀüÂ÷µµ";
static char unk_46312C[] = "¾î¼­ ¿À½Ã°Ô";
static char unk_463138[] = "ÆÄ¶õ ¼Ó¿ÊÀº ¾î¸¥ÀÇ Áõ¸í";
static char unk_463150[] = "£Ó£É£Ì£Å£Î£Ô £Ã£È£Ò£É£Ó£Ô£Í£Á£Ó";
static char unk_463174[] = "À¯¸í ÀÛ°¡ÀÇ Áõ¸í";
static char unk_463184[] = "¾ÆÅ¸Å¸Å¸Å¸! ";
static char unk_463194[] = "º¸¶ó! ÀÌ ¿ì¾ÆÇÏ°í È­·ÁÇÑ Ææ³î¸²À»! ";
static char unk_4631BC[] = "¾Æ¹öÁöÀÇ ¿ø¼ö´Ù¾Æ¾Æ!! ";
static char unk_4631CC[] = "ÀÜÈ¤ÇÑ °á¸»";
static char unk_4631DC[] = "±àÁö¸¦ Áö´Ñ ¶óÀÌ¹ú";
static char unk_4631EC[] = "¿¡ÀÌ¹Ì¿Í ¹°°í±â";
static char unk_4631F8[] = "£Ù£Ï£Õ £Á£Ò£Å £Ó£È£Ï£Ã£Ë! ";
static char unk_463218[] = "¹ÌºÐ ÀûºÐ ±âºÐ ÁÁ³×!? ";
static char unk_463230[] = "±× ¾Ö´Â ¼¼·ÃµÈ ¾Æ°¡¾¾";
static char unk_463248[] = "ÇÏ±³±æ »û±æ¿¡¼­";
static char unk_46325C[] = "¹«Å°ÀÌ!!! ";
static char unk_46326C[] = "¿äÁò, ¾î¶§? ";
static char unk_46327C[] = "¶ó¸®¶ó¸®¢¦";
static char unk_46328C[] = "±×·³, ºÎ¸ð´Ô²² ÀÎ»çÇÏ·¯ °¥±î? ";
static char unk_4632A8[] = "ÆãÆã ½ð´Ù! ";
static char unk_4632BC[] = "±Ù½Ã´Â °¨µµ°¡ ÁÁ´Ù´øµ¥";
static char unk_4632DC[] = "³ª, °Å±â ¾àÇÏ´Ü ¸»ÀÌ¾ß¡¦";
static char unk_4632F4[] = "ÀÌ°Ç ¾È µÇ´Â °Å¾ß? ";
static char unk_46330C[] = "°Å±â Çü¾¾, ¾î¶§¿ä? ";
static char unk_463328[] = "ÈÉÃÄº¸±â¶û º¸ÄÉ¡¤ÃýÄÚ¹Ì´Â Ä­»çÀÌ »ç¶÷ÀÇ Æ¯±â¾ß";
static char unk_463350[] = "°«£ÇÆæ! ";
static char unk_463364[] = "ºÒÅ¸¶ó £ÇÆæ";
static char unk_463374[] = "°áÀü! °ÝÀü!! ´ë³­Àü!!! ";
static char unk_463390[] = "°áÅõ! °ÝÅõ!! ´ë³­Åõ!!! ";
static char unk_4633AC[] = "Àß ÀÖ¾î À¯¿ì! ¸¶½ºÅÍ ¿ÀÅ¸Äí ¼³»ê¿¡¼­ Á×´Ù? ";
static char unk_4633D4[] = "£Í£É£Ä£Î£É£Ç£È£Ô £Ò£Õ£Î";
static char unk_4633F0[] = "ÁøÂ¥ ÃýÄÚ¹Ì ³Ö¾îÁÙ±î? ";
static char unk_46340C[] = "¡¦³ª´Â ½ÎÁö ¾Ê´Ù±¸";
static char unk_463420[] = "ÇÑ ÀÜ ¾î¶§¿ä? ";
static char unk_463430[] = "ÀÌ Ç³¼±, ¾ó¸¶ÀÏ±î? ";
static char unk_46344C[] = "»¡°£ ½ÅÈ£, Ä­»çÀÌ »ç¶÷Àº ¾È ¼­¿ä";
static char unk_46346C[] = "³ª´Ï¿Í Àå»ç²Û, ¿ì½À°Ô º¸¸é ¾È µÅ¿ä¢¦";
static char unk_463494[] = "£Ä£ò£å£á£í £Á£ç£á£é£î";
static char unk_4634AC[] = "£Î£å£ö£å£ò £ó£á£ù £Ç£ï£ï£ä£â£ù£å";
static char unk_4634D0[] = "¿ª½Ã ¿¬ÇÏ°¡ ÁÁ¾Æ";
static char unk_4634E8[] = "Á¥º´ °øÁÖ º»Æí";
static char unk_4634FC[] = "Àá±ñ¸¸ ±â´Ù·ÁÁà";
static char unk_463510[] = "±â¸ð³ë ¹ÌÀÎ";
static char unk_46351C[] = "°í¹é";
static char unk_463524[] = "¹ÝÁö¿¡ ´ã±ä ¸¶À½";
static char unk_46353C[] = "¹Ì³ª¹Ì ¾¾´Â Á¥º´ Àß ¹°¾î";
static char unk_463554[] = "ÁøÇà ´ã´ç ¹Ì³ª¹Ì";
static char unk_463560[] = "¿ÀÀÏ Á» ¹ß¶ó ÁÖÁö ¾ÊÀ»·¡¿ä? ";
static char unk_46357C[] = "¾É¾Æ ÀÖ´Â ¹Ì³ª¹Ì";
static char unk_463588[] = "»õÄ§ÇÑ ¹Ì³ª¹Ì";
static char unk_463594[] = "È­Ã¢ÇÑ ³¯¾¾¿¡ ÀÌ²ø·Á";
static char unk_4635A4[] = "³ÊÀÇ ´«µ¿ÀÚ´Â 1 ±â°¡ º¼Æ®";
static char unk_4635BC[] = "°è¼Ó ¹Ù¶óº¸°í ½Í¾î";
static char unk_4635CC[] = "¾ÆÄ§Àº ¹¹¿´´õ¶ó";
static char unk_4635E4[] = "´©±¸¿¡°Ô³ª ÇÐ»ý ½ÃÀýÀº ÀÖ¾ú´Ù";
static char unk_463600[] = "·°Å° Âù½º´Â ºñ ¼Ó¿¡¼­";
static char unk_46361C[] = "¿·Áý ´©´Ô";
static char unk_463630[] = "¿¹»Û ´©´Ô";
static char unk_463644[] = "µÑÀÇ Ãâ¹ß";
static char unk_463650[] = "£Ì£Ï£Ö£Å £É£Ó¡¦";
static char unk_463664[] = "¸¸Á·ÇÏÁö ¸øÇØ¡¦";
static char unk_463674[] = "º¸Åë ÀÚ±ØÀ¸·Î´Â¡¦";
static char unk_463684[] = "ÀþÀº µÎ »ç¶÷Àº¡¦";
static char unk_463694[] = "ÀÌºÒ µµµÏ";
static char unk_4636A4[] = "°í¸¿½À´Ï´ç¢¦";
static char unk_4636BC[] = "£È£Á£Ò£Ä £×£Ï£Ò£Ë´Â ³¡³ªÁö ¾Ê¾Æ";
static char unk_4636DC[] = "½º¸¶ÀÏ ½º¸¶ÀÏ";
static char unk_4636F0[] = "¼Õ¼ö ¸¸µç ¿ä¸®¸¸ ÇÑ °Ç ¾ø´Ù";
static char unk_463708[] = "ÀÚ, ¸Ô¾îºÁ";
static char unk_463718[] = "»ß- Çá¶óµÕµÕ";
static char unk_46372C[] = "°©ÀÛ½º·± £Ë£É£Ó£Ó";
static char unk_46373C[] = "³Ê¸¦ ÀüºÎ ²ø¾î¾È°í ½Í¾î";
static char unk_463750[] = "¶°¿À¸£´Â °ú°Å";
static char unk_463760[] = "´«¹°ÀÇ ÀÌÀ¯´Â¡¦";
static char unk_463770[] = "²¿¸¶ Ä³¸¯ÅÍµµ Àß ¾î¿ï¸³´Ï´Ù";
static char unk_463790[] = "ÀÍ»çº¸´Ù Áú½Ä»ç";
static char unk_4637A4[] = "ÀÌ¹ø¿¡´Â ºñÄ¡¹ß¸®ºÎ";
static char unk_4637BC[] = "£Ð£Ï£×£Ä£Å£Ò £Ó£Î£Ï£×";
static char unk_4637D4[] = "³ª, ÀÌÁ¦ Á¤¸» ¾È µÉÁöµµ¡¦¡¦";
static char unk_4637F0[] = "ÇÇÄ¡ÇÇÄ¡ ÂýÂý";
static char unk_463808[] = "³ª Å×´Ï½ººÎ";
static int size_462AA0 = 0xD78;
static char* strings_462AA0[] = {
    unk_462AA0, unk_462AB4, unk_462AC4, unk_462AD0, unk_462AE8, unk_462B04, unk_462B20, unk_462B30,
    unk_462B4C, unk_462B64, unk_462B84, unk_462B90, unk_462B9C, unk_462BB4, unk_462BC8, unk_462BE8,
    unk_462BF4, unk_462C14, unk_462C20, unk_462C34, unk_462C5C, unk_462C70, unk_462C80, unk_462C9C,
    unk_462CAC, unk_462CBC, unk_462CCC, unk_462CE4, unk_462CFC, unk_462D18, unk_462D34, unk_462D48,
    unk_462D54, unk_462D5C, unk_462D70, unk_462D88, unk_462D9C, unk_462DB0, unk_462DCC, unk_462DEC,
    unk_462E00, unk_462E18, unk_462E2C, unk_462E44, unk_462E54, unk_462E68, unk_462E7C, unk_462E8C,
    unk_462EA8, unk_462ED0, unk_462EE4, unk_462EF0, unk_462F08, unk_462F1C, unk_462F30, unk_462F40,
    unk_462F68, unk_462F7C, unk_462F8C, unk_462FA0, unk_462FB0, unk_462FCC, unk_462FE8, unk_463000,
    unk_463014, unk_46302C, unk_463038, unk_463040, unk_463060, unk_463084, unk_4630B0, unk_4630BC,
    unk_4630C8, unk_4630E8, unk_4630F8, unk_463104, unk_463110, unk_463124, unk_46312C, unk_463138,
    unk_463150, unk_463174, unk_463184, unk_463194, unk_4631BC, unk_4631CC, unk_4631DC, unk_4631EC,
    unk_4631F8, unk_463218, unk_463230, unk_463248, unk_46325C, unk_46326C, unk_46327C, unk_46328C,
    unk_4632A8, unk_4632BC, unk_4632DC, unk_4632F4, unk_46330C, unk_463328, unk_463350, unk_463364,
    unk_463374, unk_463390, unk_4633AC, unk_4633D4, unk_4633F0, unk_46340C, unk_463420, unk_463430,
    unk_46344C, unk_46346C, unk_463494, unk_4634AC, unk_4634D0, unk_4634E8, unk_4634FC, unk_463510,
    unk_46351C, unk_463524, unk_46353C, unk_463554, unk_463560, unk_46357C, unk_463588, unk_463594,
    unk_4635A4, unk_4635BC, unk_4635CC, unk_4635E4, unk_463600, unk_46361C, unk_463630, unk_463644,
    unk_463650, unk_463664, unk_463674, unk_463684, unk_463694, unk_4636A4, unk_4636BC, unk_4636DC,
    unk_4636F0, unk_463708, unk_463718, unk_46372C, unk_46373C, unk_463750, unk_463760, unk_463770,
    unk_463790, unk_4637A4, unk_4637BC, unk_4637D4, unk_4637F0, unk_463808
};
static char* def_4627D0[] = {
    unk_463808, unk_4637F0, unk_4637D4, unk_4637BC, unk_4637A4, unk_463790, unk_463770, unk_463760,
    unk_463750, unk_46373C, unk_46372C, unk_463718, unk_463708, unk_4636F0, unk_4636DC, unk_4636BC,
    unk_4636A4, unk_463694, unk_463684, unk_463674, unk_463664, unk_463650, unk_463644, unk_463630,
    unk_46361C, unk_463600, unk_4635E4, unk_4635CC, unk_4635BC, unk_4635A4, unk_463594, unk_463588,
    unk_46357C, unk_463560, unk_463554, unk_46353C, unk_463524, unk_46351C, unk_463510, unk_4634FC,
    unk_4634E8, unk_4634D0, unk_4634AC, unk_463494, unk_46346C, unk_46344C, unk_463430, unk_463420,
    unk_46340C, unk_4633F0, unk_4633D4, unk_4633AC, unk_463390, unk_463374, unk_463364, unk_463350,
    unk_463328, unk_46330C, unk_4632F4, unk_4632DC, unk_4632BC, unk_4632A8, unk_46328C, unk_46327C,
    unk_46326C, unk_46325C, unk_463248, unk_463230, unk_463218, unk_4631F8, unk_4631EC, unk_4631DC,
    unk_4631CC, unk_4631BC, unk_463194, unk_463184, unk_463174, unk_463150, unk_463138, unk_46312C,
    unk_463124, unk_463110, unk_463104, unk_4630F8, unk_4630E8, unk_4630C8, unk_4630BC, unk_4630B0,
    unk_463084, unk_463060, unk_463040, unk_463038, unk_46302C, unk_463014, unk_463000, unk_462FE8,
    unk_462FCC, unk_462FB0, unk_462FA0, unk_462F8C, unk_462F7C, unk_462F68, unk_462F40, unk_462F30,
    unk_462F1C, unk_462F08, unk_462EF0, unk_462EE4, unk_462ED0, unk_462EA8, unk_462E8C, unk_462E7C,
    unk_462E68, unk_462E54, unk_462E44, unk_462E2C, unk_462E18, unk_462E00, unk_462DEC, unk_462DCC,
    unk_462DB0, unk_462D9C, unk_462D88, unk_462D70, unk_462D5C, unk_462D54, unk_462D48, unk_462D34,
    unk_462D18, unk_462CFC, unk_462CE4, unk_462CCC, unk_462CBC, unk_462CAC, unk_462C9C, unk_462C80,
    unk_462C70, unk_462C5C, unk_462C34, unk_462C20, unk_462C14, unk_462BF4, unk_462BE8, unk_462BC8,
    unk_462BB4, unk_462B9C, unk_462B90, unk_462B84, unk_462B64, unk_462B4C, unk_462B30, unk_462B20,
    unk_462B04, unk_462AE8, unk_462AD0, unk_462AC4, unk_462AB4, unk_462AA0
};


static char unk_463E10[] = "¡¸ÀÌ¾ß~ ¼ö°íÇß´Ù ¿µ¸Ç!! Ã¹ ÄÚ¹ÌÆÄ´Â ¹«»çÈ÷ ³¡³µ±º. ÀÚ, µ¿Áö¿©. ÀÌ¹ø Ã¥ÀÇ ¸Å»ó°ú ³»¿ë ¸»ÀÎµ¥¡¦¡¹";
static char unk_463E74[] = "ÀÚ ±×·³¡¦";
static char unk_463E80[] = "¾îÈÞ¡¦¡¦ ¿©ÀüÈ÷ ÀÜ¶à ¶°µé¾î ´ë´Ù°¡ µ¹¾Æ°¡ ¹ö¸®³×¡¦ ¹Ù»Û ³à¼®ÀÌ±¸¸¸.";
static char unk_463ECC[] = "¡¸±×·³ ¶Ç º¸ÀÚ ¿µ¸Ç!! ÀÛº°ÀÌ´Ù!!¡¹";
static char unk_463EFC[] = "¡¸ÀÌ¾ß~ ¼ö°íÇß´Ù ¿µ¸Ç!! ÀÌ¹ø ÄÚ¹ÌÆÄµµ ¹«»çÈ÷ ³¡³µ±º. ÀÚ, µ¿Áö¿©. ÀÌ¹ø Ã¥ÀÇ ¸Å»ó°ú ³»¿ë ¸»ÀÎµ¥¡¦¡¹";
static char unk_463F64[] = "ÀüÃ¼ÀûÀ¸·Î´Â ¿Ï¼ºµµ°¡ ¸Å¿ì ³ô°Ô ³ª¿ÔÁö.";
static char unk_463F94[] = "ÀüÃ¼ÀûÀ¸·Î´Â ²Ï ¿¹»Ú°Ô ¸¶¹«¸®µÈ °Í °°³×.";
static char unk_463FBC[] = "ÀüÃ¼ÀûÀ¸·Î´Â Å« Èì ¾øÀÌ Àß Á¤¸®µÅ ÀÖ¾úÁö.";
static char unk_463FE4[] = "ÀüÃ¼ÀûÀ¸·Î´Â ¸¶¹«¸®°¡ Á¶±Ý °ÅÄ£ ÆíÀÌ¾úÁö.";
static char unk_464010[] = "ÀüÃ¼ÀûÀ¸·Î´Â ¸¶¹«¸®°¡ ¾û¸ÁÀÌ¶ó ¿Ï¼ºµµ°¡ ³·¾ÒÁö.";
static char unk_464040[] = "½ºÅä¸®´Â ÈÇ¸¢ÇÏ°í °¨µ¿ÀûÀÎ ÁÁÀº ÀÌ¾ß±â¿´´Ù.";
static char unk_464068[] = "½ºÅä¸®´Â ²Ï ¸ÀÀÌ »ì¾Æ ÀÖ´Â ±íÀÌ ÀÖ´Â ³»¿ëÀÌ¾ú´Ù.";
static char unk_464090[] = "½ºÅä¸®´Â ³ª¸§´ë·Î ¹«³­ÇÏ°Ô Àß Á¤¸®µÅ ÀÖ¾ú´Ù.";
static char unk_4640BC[] = "½ºÅä¸®´Â Á¶±Ý ÁøºÎÇØ¼­ ¿µ º°·Î¿´´Ù.";
static char unk_4640EC[] = "½ºÅä¸®´Â °ÅÀÇ ¾ø´Ù°í ÇØµµ µÉ Á¤µµÀÇ ¼öÁØÀÌ¾ú´Ù.";
static char unk_464124[] = "±×¸²Àº ³ª¹«¶ö µ¥ ¾øÀÌ ¿Ïº®Çß°í,";
static char unk_46413C[] = "±×¸²Àº »ó´çÈ÷ ´Ã¾î¼­ ²Ï Àß ±×¸®°Ô µÇ¾ú°í,";
static char unk_464164[] = "±×¸²Àº Á¦¹ý ±¦ÂúÀº ´À³¦ÀÌ°í,";
static char unk_464180[] = "±×¸²Àº Á¶±Ý °ÅÄ¥°í,";
static char unk_464198[] = "±×¸²Àº ¾ÆÁ÷µµ ¿µ ¾û¸ÁÀÌ°í,";
static char unk_4641B4[] = "µ¿ÀÎÀÇ ½Å";
static char unk_4641C0[] = "µ¿ÀÎ¿Õ";
static char unk_4641C8[] = "½Ç·ÂÆÄ µ¿ÀÎ ÀÛ°¡";
static char unk_4641D8[] = "³ë·ÃÇÑ µ¿ÀÎ ÀÛ°¡";
static char unk_4641EC[] = "Áß°ß µ¿ÀÎ ÀÛ°¡";
static char unk_4641FC[] = "½ÅÀÎ µ¿ÀÎ ÀÛ°¡";
static char unk_46420C[] = "Ç²³»±â µ¿ÀÎ ÀÛ°¡";
static int size_463E10 = 0x410;
static char* strings_463E10[] = {
    unk_463E10, unk_463E74, unk_463E80, unk_463ECC, unk_463EFC, unk_463F64, unk_463F94, unk_463FBC,
    unk_463FE4, unk_464010, unk_464040, unk_464068, unk_464090, unk_4640BC, unk_4640EC, unk_464124,
    unk_46413C, unk_464164, unk_464180, unk_464198, unk_4641B4, unk_4641C0, unk_4641C8, unk_4641D8,
    unk_4641EC, unk_4641FC, unk_46420C
};
static char* def_463DA4[] = {
    unk_46420C, unk_4641FC, unk_4641EC, unk_4641D8, unk_4641C8, unk_4641C0, unk_4641B4, unk_464198,
    unk_464180, unk_464164, unk_46413C, unk_464124, unk_4640EC, unk_4640BC, unk_464090, unk_464068,
    unk_464040, unk_464010, unk_463FE4, unk_463FBC, unk_463F94, unk_463F64, unk_463EFC, unk_463ECC,
    unk_463E80, unk_463E74, unk_463E10
};


static char unk_464220[] = "¡¸ÁÁ¾Æ!! ÀÌ¹ø °øÀûÀ¸·Î ³Ê¸¦ %s ±ÞÀ¸·Î ÀÎÁ¤ÇØ ÁÖÁö¡Ú¡¹";
static char unk_464258[] = "¡¸%s¡¹";
static char unk_464260[] = "¡¸ÀÌ¹ø ¸¸È­´Â %s %s¡¹";
static char unk_464278[] = "¡¸¡¦´ëÃæ ÀÌ·± ´À³¦ÀÌ´Ù¡¹";
static char unk_464294[] = "%sÀ»(¸¦) ÁÖ¿ü½À´Ï´Ù";
static char unk_4642A4[] = "ÀÀ? ¹¹¾ß ÀÌ°Ç?";
static char unk_4642B8[] = "%s ´ÔÀÇ µ¿ÀÎÁö¸¦ ¼Õ¿¡ ³Ö¾ú½À´Ï´Ù.";
static int size_464220 = 0xBC;
static char* strings_464220[] = {
    unk_464220, unk_464258, unk_464260, unk_464278, unk_464294, unk_4642A4, unk_4642B8
};
static char* def_40A9A0[] = { unk_464220 };
static char* def_40A946[] = { unk_464258 };
static char* def_40A922[] = { unk_464260 };
static char* def_40A8F2[] = { unk_464278 };
static char* def_40AD19[] = { unk_464294 };
static char* def_40AD0A[] = { unk_4642A4 };
static char* def_40ACD0[] = { unk_4642B8 };


static char unk_464590[] = "Ã¹ Àå ÄÃ·¯";
static char unk_46459C[] = "¹Ú ÀÎ¼â";
static char unk_4645A4[] = "Àå½ÄÁö";
static char unk_4645AC[] = "Ç® ÄÃ·¯";
static char unk_4645B8[] = "2»ö ÀÎ¼â";
static char unk_4645C4[] = "´Ü»ö";
static char unk_4645CC[] = "¿É¼Â ÀÎ¼â 84ÆäÀÌÁö";
static char unk_4645E4[] = "¿É¼Â ÀÎ¼â 72ÆäÀÌÁö";
static char unk_4645FC[] = "¿É¼Â ÀÎ¼â 60ÆäÀÌÁö";
static char unk_464614[] = "¿É¼Â ÀÎ¼â 48ÆäÀÌÁö";
static char unk_46462C[] = "¿É¼Â ÀÎ¼â 36ÆäÀÌÁö";
static char unk_464644[] = "¿É¼Â ÀÎ¼â 24ÆäÀÌÁö";
static char unk_46465C[] = "º¹»çº» 12ÆäÀÌÁö";
static int size_464590 = 0xE0;
static char* strings_464590[] = {
  unk_464590, unk_46459C, unk_4645A4, unk_4645AC, unk_4645B8, unk_4645C4, unk_4645CC, unk_4645E4,
  unk_4645FC, unk_464614, unk_46462C, unk_464644, unk_46465C
};
static char* def_4643BC[] = {
    unk_46465C, unk_464644, unk_46462C, unk_464614, unk_4645FC, unk_4645E4, unk_4645CC
};
static char* def_4644B8[] = {
    unk_4645C4, unk_4645B8, unk_4645AC
};
static char* def_464524[] = {
    unk_4645A4, unk_46459C, unk_464590
};


static char unk_46AEC8[] = "%6s %-22s¡¡ÆÇ¸Å ºÎ¼ö %s";
static char unk_46AEE4[] = "%4s¿ù";
static char unk_46AEEC[] = "°á¡¡°ú¡¡¹ß¡¡Ç¥";
static int size_46AEC8 = 0x34;
static char* strings_46AEC8[] = {
    unk_46AEC8, unk_46AEE4, unk_46AEEC
};
static char* def_411BC6[] = { unk_46AEC8 };
static char* def_411B9C[] = { unk_46AEE4 };
static char* def_411B3C[] = { unk_46AEEC };


static char unk_46B204[] = "¾î¸Ó´Ï";
static char unk_46B210[] = "¾Æ¹öÁö";
static char unk_46B21C[] = "¹æ¼Û";
static char unk_46B224[] = "ÀÎ¼â¼Ò Á÷¿ø";
static char unk_46B230[] = "¼±¹è";
static char unk_46B238[] = "¿ª¹«¿ø";
static char unk_46B240[] = "¿©ÀÚ";
static char unk_46B248[] = "¾ö¸¶";
static char unk_46B250[] = "¾Æºü";
static char unk_46B258[] = "À¯Ä«";
static char unk_46B260[] = "¸¶À¯";
static char unk_46B268[] = "¹ÌÈ£";
static char unk_46B270[] = "ÄÚ½ºÇÃ·¹ÀÌ¾î";
static char unk_46B280[] = "¿©ÀÚ ¸ñ¼Ò¸®";
static char unk_46B28C[] = "¸®Æ÷ÅÍ";
static char unk_46B298[] = "Çì¸ðÇì¸ð";
static char unk_46B2A4[] = "¸¶È÷·ç";
static char unk_46B2AC[] = "³²ÀÚ ¸ñ¼Ò¸®";
static char unk_46B2B4[] = "¸ð¸ð";
static char unk_46B2BC[] = "¿©°ü Á¾¾÷¿ø";
static char unk_46B2C4[] = "¿þÀÌÆ®¸®½º";
static char unk_46B2D4[] = "ÆÇ¸Å¿ø";
static char unk_46B2DC[] = "ÇØ¼³ÀÚ";
static char unk_46B2E4[] = "»çÈ¸ÀÚ";
static char unk_46B2EC[] = "Á¡¿ø";
static char unk_46B2F4[] = "¼Ò³à";
static char unk_46B2FC[] = "¾î¸Ó´Ô";
static char unk_46B304[] = "´©´Ô";
static char unk_46B310[] = "¾ÆÀú¾¾";
static char unk_46B31C[] = "¾ÆÀÌ";
static char unk_46B324[] = "Á¾¾÷¿ø";
static char unk_46B32C[] = "£¿";
static char unk_46B330[] = "ÀüÈ­";
static char unk_46B338[] = "¼Õ´Ô";
static char unk_46B33C[] = "¼Ò³à ¸ñ¼Ò¸® £Ã";
static char unk_46B34C[] = "¼Ò³à ¸ñ¼Ò¸® £Â";
static char unk_46B35C[] = "¼Ò³à ¸ñ¼Ò¸® £Á";
static char unk_46B36C[] = "¼Ò³à ¸ñ¼Ò¸®";
static char unk_46B378[] = "½ºÅÂÇÁ";
static char unk_46B384[] = "»ïÀÎÁ¶";
static char unk_46B38C[] = "¿ÀÅ¸Äí";
static char unk_46B394[] = "ÆíÁýÀå";
static char unk_46B39C[] = "¿î¼Û¾÷ÀÚ";
static char unk_46B3A8[] = "¼ö¼ö²²³¢ ³²ÀÚ";
static char unk_46B3B0[] = "Å¸ÀÌ½Ã";
static char unk_46B3B8[] = "ÀÌÄí¹Ì";
static char unk_46B3C0[] = "Ä¡»ç";
static char unk_46B3C8[] = "·¹ÀÌÄÚ";
static char unk_46B3D0[] = "¾Æ»çÈ÷";
static char unk_46B3D8[] = "¾Æ¾ß";
static char unk_46B3DC[] = "¿¡ÀÌ¹Ì";
static char unk_46B3E4[] = "À¯¿ì";
static char unk_46B3EC[] = "¹Ì³ª¹Ì";
static char unk_46B3F0[] = "¹ÌÁîÅ°";
static char unk_46B3F8[] = "ÁÖÀÎ°ø";
static int size_46B204 = 0x1FC;
static char* strings_46B204[] = {
    unk_46B204, unk_46B210, unk_46B21C, unk_46B224, unk_46B230, unk_46B238, unk_46B240, unk_46B248,
    unk_46B250, unk_46B258, unk_46B260, unk_46B268, unk_46B270, unk_46B280, unk_46B28C, unk_46B298,
    unk_46B2A4, unk_46B2AC, unk_46B2B4, unk_46B2BC, unk_46B2C4, unk_46B2D4, unk_46B2DC, unk_46B2E4,
    unk_46B2EC, unk_46B2F4, unk_46B2FC, unk_46B304, unk_46B310, unk_46B31C, unk_46B324, unk_46B32C,
    unk_46B330, unk_46B338, unk_46B33C, unk_46B34C, unk_46B35C, unk_46B36C, unk_46B378, unk_46B384,
    unk_46B38C, unk_46B394, unk_46B39C, unk_46B3A8, unk_46B3B0, unk_46B3B8, unk_46B3C0, unk_46B3C8,
    unk_46B3D0, unk_46B3D8, unk_46B3DC, unk_46B3E4, unk_46B3EC, unk_46B3F0, unk_46B3F8
};
static char* def_46B0D0[] = {
    unk_46B3F8, unk_46B3F0, unk_46B3EC, unk_46B3E4, unk_46B3DC, unk_46B3D8, unk_46B3D0, unk_46B3C8,
    unk_46B3C0, unk_46B3B8, unk_46B3B0, unk_46B3A8, unk_46B39C, unk_46B394, unk_46B38C, unk_46B384,
    unk_46B378, unk_46B36C, unk_46B35C, unk_46B34C, unk_46B33C, unk_46B338, unk_46B330, unk_46B32C,
    unk_46B324, unk_46B31C, unk_46B310, unk_46B304, unk_46B2FC, unk_46B2F4, unk_46B2EC, unk_46B2E4,
    unk_46B2DC, unk_46B2D4, unk_46B2C4, unk_46B2BC, unk_46B2B4, unk_46B2AC, unk_46B2A4, unk_46B298,
    unk_46B28C, unk_46B280, unk_46B270, unk_46B384, unk_46B268, unk_46B260, unk_46B258, unk_46B250,
    unk_46B248, unk_46B240, unk_46B238, unk_46B230, unk_46B224, unk_46B21C, unk_46B210, unk_46B204
};
static char* def_416642[] = { unk_46B204 };
static char* def_4166A8[] = { unk_46B204 };
static char* def_4166EE[] = { unk_46B204 };
static char* def_448B31[] = { unk_46B3C0 };
static char* def_448B56[] = { unk_46B3D8 };


static char unk_46B5D8[] = "ÀÌ ÆÄÀÏ¿¡ µ¤¾î¾µ±î¿ä?";
static char unk_46B5F8[] = "ÀÌ ÆÄÀÏÀ» ºÒ·¯¿Ã±î¿ä?";
static int size_46B5D8 = 0x40;
static char* strings_46B5D8[] = { unk_46B5D8, unk_46B5F8 };
static char* def_46B5D0[] = { unk_46B5F8, unk_46B5D8 };


static char unk_46B630[] = "¾Æ´Ï¿À";
static char unk_46B638[] = "¡¡³×¡¡";
static int size_46B630 = 0x10;
static char* strings_46B630[] = { unk_46B630, unk_46B638 };
static char* def_46B628[] = { unk_46B638, unk_46B630 };


static char unk_46BF68[] = "³×";
static int size_46BF68 = 0x8;
static char* strings_46BF68[] = { unk_46BF68 };
static char* def_46BE6C[] = { unk_46BF68, unk_46B630};


static char unk_46B650[] = "%s¿ù %sÀÏ";
static char unk_46B65C[] = "ÆÄÀÏÀÌ ¾ø½À´Ï´Ù";
static char unk_46B674[] = "ÆÄÀÏ %s";
static int size_46B650 = 0x30;
static char* strings_46B650[] = { unk_46B650, unk_46B65C, unk_46B674 };
static char* def_4192EE[] = { unk_46B650 };
static char* def_4191E7[] = { unk_46B65C };
static char* def_4191B9[] = { unk_46B674 };


static char unk_46BA6C[] = "¼¼ÀÌºê";
static char unk_46BA74[] = "·Îµå";
static int size_46BA6C = 0x10;
static char* strings_46BA6C[] = { unk_46BA6C, unk_46BA74 };
static char* def_46BA64[] = { unk_46BA74, unk_46BA6C };


static char unk_46BAA8[] = "%s ÇÒ ÆÄÀÏÀ» ¼±ÅÃÇÏ¼¼¿ä";
static int size_46BAA8 = 0x28;
static char* strings_46BAA8[] = { unk_46BAA8 };
static char* def_4196E0[] = { unk_46BAA8 };


static char unk_46BB10[] = "°ÔÀÓ Á¾·á";
static char unk_46BB1C[] = "À½¾Ç µè±â";
static char unk_46BB28[] = "¾Ù¹üÀ» º»´Ù";
static char unk_46BB38[] = "ºÒ·¯¿À±â";
static char unk_46BB44[] = "Ã³À½ºÎÅÍ ½ÃÀÛ";
static int size_46BB10 = 0x37;
static char* strings_46BB10[] = { unk_46BB10, unk_46BB1C, unk_46BB28, unk_46BB38, unk_46BB44 };
static char* def_46BAEC[] = {
    unk_46BB44, unk_46BB38, unk_46BB28, unk_46BB1C, unk_46BB10, unk_46BB44, unk_46BB38, unk_46BB28,
    unk_46BB10
};


static char unk_46BB80[] = "ÇÁ·Ñ·Î±×¸¦ ÁøÇàÇÏ½Ã°Ú½À´Ï±î?";
static int size_46BB80 = 0x20;
static char* strings_46BB80[] = { unk_46BB80 };
static char* def_41A746[] = { unk_46BB80 };


static char unk_46BBC0[] = "ÇÁ·Î µ¥ºß ¿À¸®Áö³Î";
static char unk_46BBD8[] = "¼ö¶óÀå ¸ðµå·Î ÁøÀÔÇÒ±î¿ä?";
static char unk_46BBF4[] = "¾ÆÀÌÅÛÀ» »ç¿ëÇÒ±î¿ä?";
static int size_46BBC0 = 0x50;
static char* strings_46BBC0[] = { unk_46BBC0, unk_46BBD8, unk_46BBF4 };
static char* def_41B37D[] = { unk_46BBC0 };
static char* def_428B98[] = { unk_46BBC0 };
static char* def_41C24F[] = { unk_46BBD8 };
static char* def_41CECF[] = { unk_46BBF4 };


static char unk_46BC38[] = "±¼¸²Ã¼";
static char unk_46BC48[] = "ÆùÆ® ¸ñ·Ï Ç¥½Ã¿¡ ÇÊ¿äÇÑ ¸Þ¸ð¸®°¡ ºÎÁ·ÇÕ´Ï´Ù";
static int size_46BC38 = 0x40;
static char* strings_46BC38[] = { unk_46BC38, unk_46BC48 };
static char* def_41E4AE[] = { unk_46BC38 };
static char* def_41E504[] = { unk_46BC38 };
static char* def_41E5D7[] = { unk_46BC48 };
static char* def_41E6E9[] = { unk_46BC48 };

static char unk_46BE0C[] = "£Ê£á£í£í£é£î£ç £Â£ï£ï£ë £Ó£ô£ï£ò£å";
static int size_46BE0C = 0x24;
static char* strings_46BE0C[] = { unk_46BE0C };
static char unk_46BE30[] = "¸ÅÄÞ´ç";
static int size_46BE30 = 0x8;
static char* strings_46BE30[] = { unk_46BE30 };
static char unk_46BE38[] = "£Ã£Á£Ô £Á £Ä£Á£Ó£È";
static int size_46BE38 = 0x14;
static char* strings_46BE38[] = { unk_46BE38 };


static char unk_46C05C[] = "ÀÌ\xb2\x70³»¿ë´ë·Î ÁøÇàÇÒ±î¿ä?";
static int size_46C05C = 0x1C;
static char* strings_46C05C[] = { unk_46C05C };


static char unk_46C09C[] = "84 ÆäÀÌÁö";
static char unk_46C0A8[] = "72 ÆäÀÌÁö";
static char unk_46C0B4[] = "60 ÆäÀÌÁö";
static char unk_46C0C0[] = "48 ÆäÀÌÁö";
static char unk_46C0CC[] = "36 ÆäÀÌÁö";
static char unk_46C0D8[] = "24 ÆäÀÌÁö";
static char unk_46C0E4[] = "Ç® ÄÃ·¯ ÀÎ¼â";
static char unk_46C0F4[] = " ´Ù»ö ÀÎ¼â";
static char unk_46C104[] = " ´Ü»ö ÀÎ¼â";
static int size_46C09C = 0x78;
static char* strings_46C09C[] = {
    unk_46C09C, unk_46C0A8, unk_46C0B4, unk_46C0C0, unk_46C0CC, unk_46C0D8, unk_46C0E4, unk_46C0F4,
    unk_46C104
};
static char* def_46C078[] = {
    unk_46C104, unk_46C0F4, unk_46C0E4, unk_46C0D8, unk_46C0CC, unk_46C0C0, unk_46C0B4, unk_46C0A8,
    unk_46C09C
};
static char* def_46F2C8[] = { unk_46C104, unk_46C0F4, unk_46C0E4 };


static char unk_46C1DC[] = "%s ´ÔÀÌ ¿ø°í ÀÇ·Ú¸¦ °ÅÀýÇß½À´Ï´Ù";
static char unk_46C1FC[] = "%s ´Ô²² ¿ø°í¸¦ ÀÇ·ÚÇß½À´Ï´Ù";
static char unk_46C218[] = "¿ø°í¸¦ ÀÇ·ÚÇÒ »ç¶÷ÀÌ ¾ø½À´Ï´Ù";
static int size_46C1DC = 0x5C;
static char* strings_46C1DC[] = { unk_46C1DC, unk_46C1FC, unk_46C218 };
static char* def_424688[] = { unk_46C1DC };
static char* def_42464E[] = { unk_46C1FC };
static char* def_424509[] = { unk_46C218 };


static char unk_46C244[] = "Ã¢ÀÛ";
static char unk_46C24C[] = "°ÔÀÓ";
static char unk_46C254[] = "¾Ö´Ï";
static int size_46C244 = 0x18;
static char* strings_46C244[] = { unk_46C244, unk_46C24C, unk_46C254 };
static char* def_46C238[] = { unk_46C254, unk_46C24C, unk_46C244 };


static char unk_46C448[] = "¿ÀÈÄ Çàµ¿À» ¼±ÅÃÇØ ÁÖ¼¼¿ä";
static char unk_46C468[] = "¿ÀÀü Çàµ¿À» ¼±ÅÃÇØ ÁÖ¼¼¿ä";
static char unk_46C488[] = "³²¿¡°Ô ÀÇÁöÇÏÁö ¸¿½Ã´Ù";
static int size_46C448 = 0x58;
static char* strings_46C448[] = { unk_46C448, unk_46C468, unk_46C488 };
static char* def_42783D[] = { unk_46C448 };
static char* def_427836[] = { unk_46C468 };
static char* def_428BDB[] = { unk_46C488 };


static char unk_46C540[] = "¿ª½Ã, ±×¸¸µÑ±î";
static char unk_46C554[] = "¡¦±Ùµ¥ µ·ÀÌ ¾ø³×";
static char unk_46C570[] = "¾î¶ó, ÀÌ·± °Ô È­¹æ¿¡?";
static char unk_46C590[] = "¹¹ ÁÁÀº °Å ¾øÀ»±î?";
static char unk_46C5AC[] = "Çï½ºÀå";
static char unk_46C5B4[] = "È­¹æ";
static char unk_46C5BC[] = "%s ±¸ÀÔÀ» ¿Ï·áÇß½À´Ï´Ù";
static int size_46C540 = 0x90;
static char* strings_46C540[] = { unk_46C540, unk_46C554, unk_46C570, unk_46C590, unk_46C5AC, unk_46C5B4, unk_46C5BC };
static char* def_46C530[] = { unk_46C590, unk_46C570, unk_46C554, unk_46C540 };
static char* def_42CD67[] = { unk_46C5AC };
static char* def_42CD5C[] = { unk_46C5B4 };
static char* def_42D64A[] = { unk_46C5BC };


static char unk_46C678[] = "»ç¿ëÇÒ ¾ÆÀÌÅÛÀ» ¼±ÅÃÇØ ÁÖ¼¼¿ä";
static char unk_46C69C[] = "±¸¸ÅÇÒ ¾ÆÀÌÅÛÀ» ¼±ÅÃÇØ ÁÖ¼¼¿ä";
static int size_46C678 = 0x48;
static char* strings_46C678[] = { unk_46C678, unk_46C69C };
static char* def_46C670[] = { unk_46C69C, unk_46C678 };


static char unk_46C978[] = "ÀÛ°î¡¡¸¶Ã÷¿ÀÄ« ÁØ¾ß";
static char unk_46C98C[] = "ÀÛ°î¡¡¿ä³×¹«¶ó Å¸Ä«È÷·Î";
static char unk_46C9A0[] = "ÀÛ°î¡¡ÀÌ½ÃÄ«¿Í ½Å¾ß";
static char unk_46C9B4[] = "ÀÛ°î¡¡³ªÄ«°¡¹Ì Ä«ÁîÈ÷µ¥";
static char unk_46C9C8[] = "ÀÛ°î¡¡½Ã¸ðÄ«¿Í ³ª¿À¾ß";
static char unk_46C9DC[] = "»ç¶û¾ÎÀÌ";
static char unk_46C9E8[] = "£Ç£ò£á£ä£õ£á£ì¡¡£È£á£ð£ð£é£î£å£ó£ó";
static char unk_46CA0C[] = "¼¼»óÀÌ ³¯ ºÎ¸¥´Ù";
static char unk_46CA24[] = "ÇØÇÇ ÇØÇÇ ¶óÀÌÇÁ";
static char unk_46CA3C[] = "£¹£¹£¥ ¹«Ã·°¡ ¼Ò³à";
static char unk_46CA50[] = "ÇÏ´Ãºû ³ªÆÈ²É";
static char unk_46CA64[] = "´ÞÀÇ µ¿È­";
static char unk_46CA70[] = "ÇÁ¸°¼¼½º Å¸ÀÓ";
static char unk_46CA84[] = "±×°Ô ±×·¸ÀÝ¾Æ¡Ù";
static char unk_46CA9C[] = "½£ÀÇ Æ¼ÆÄÆ¼";
static char unk_46CAA8[] = "¹Ù¶÷Àº ´Ã º½¹Ù¶÷";
static char unk_46CABC[] = "£Ô£è£ò£é£ì£ì£é£î£ç";
static char unk_46CAD0[] = "£Ò£ï£í£á£î£ô£é£ã";
static char unk_46CAE4[] = "£Ï£õ£ô£ó£é£ä£å";
static char unk_46CAF4[] = "¿ì´çÅÁ£Ñ";
static char unk_46CB04[] = "£Ó£õ£î£ó£è£é£î£å¡¡£Æ£ï£ò£å£ö£å£ò";
static char unk_46CB28[] = "ÇÇÇÒ ¼ö ¾ø´Â ºñ±Ø";
static char unk_46CB38[] = "½½ÇÄÀÇ È÷·ÎÀÎ";
static char unk_46CB4C[] = "ÄÚ¹Í ÆÄ¢¦Æ¼¢¦";  // 46F7A0¿¡¼­ Ãß°¡ È£Ãâ
static char unk_46CB60[] = "£Ä£á£é£ì£ù¡¡£Í£õ£ó£é£ã";
static char unk_46CB78[] = "¸ÞÀÎ Å×¸¶";
static char unk_46CB88[] = "£Á£ó¡¡£ô£é£í£å¡¡£ç£ï£å£ó¡¡£â£ù";
static char unk_46CBA8[] = "À½¾Ç ÁßÁö";
static char unk_46CBB4[] = "ÀÛ»ç¡¡½ºÅ¸´Ï ³ª¿ÀÄÚ";
static char unk_46CBC8[] = "À½¾ÇÀ» ¼±ÅÃÇØ ÁÖ¼¼¿ä";
static int size_46C978 = 0x268;
static char* strings_46C978[] = {
    unk_46C978, unk_46C98C, unk_46C9A0, unk_46C9B4, unk_46C9C8, unk_46C9DC, unk_46C9E8, unk_46CA0C,
    unk_46CA24, unk_46CA3C, unk_46CA50, unk_46CA64, unk_46CA70, unk_46CA84, unk_46CA9C, unk_46CAA8,
    unk_46CABC, unk_46CAD0, unk_46CAE4, unk_46CAF4, unk_46CB04, unk_46CB28, unk_46CB38, unk_46CB4C,
    unk_46CB60, unk_46CB78, unk_46CB88, unk_46CBA8, unk_46CBB4, unk_46CBC8
};
static char* def_46C8B4[] = {
    unk_46CB88, unk_46CB78, unk_46CB60, unk_46CB4C, unk_46CB38, unk_46CB28, unk_46CB04, unk_46CAF4,
    unk_46CAE4, unk_46CAD0, unk_46CABC, unk_46CAA8, unk_46CA9C, unk_46CA84, unk_46CA70, unk_46CA64,
    unk_46CA50, unk_46CA3C, unk_46CA24, unk_46CA0C, unk_46C9E8, unk_46C9DC, unk_46C9C8, unk_46C9B4,
    unk_46C9A0, unk_46C98C, unk_46C978
};
static char* def_4359C8[] = { unk_46CBA8 };
static char* def_43596C[] = { unk_46CBB4 };
static char* def_435873[] = { unk_46CBC8 };


static char unk_46CC28[] = "¼­Å¬¸í£º¡¡";
static char unk_46CC38[] = "ÇÊ¡¡¸í£º¡¡";
static char unk_46CC48[] = "ÀÌ¡¡¸§£º¡¡";
static char unk_46CC58[] = "¡¡¼º¡¡£º¡¡";
static char unk_46CC68[] = "ÀÌ ¼³Á¤À¸·Î ÁøÇàÇÏ°Ú½À´Ï±î?";
static char unk_46CC88[] = "º¯°æÇÒ Ç×¸ñÀ» ¼±ÅÃÇØ ÁÖ¼¼¿ä";
static int size_46CC28 = 0x84;
static char* strings_46CC28[] = { unk_46CC28, unk_46CC38, unk_46CC48, unk_46CC58, unk_46CC68, unk_46CC88};
static char* def_46CBE0[] = { unk_46CC88, unk_46CC68 };
static char* def_46CBF0[] = { unk_46B638, unk_46B630, unk_46CC58, unk_46CC48, unk_46CC38, unk_46CC28 };


static char unk_46CD7C[] = ""; // ªóÇà
static char unk_46CD84[] = ""; // ªòÇà
static char unk_46CD8C[] = ""; // ªïÇà
static char unk_46CDC0[] = ""; // ªíÇà
static char unk_46CE04[] = ""; // ªìÇà
static char unk_46CE54[] = ""; // ªëÇà
static char unk_46CE6C[] = ""; // ªêÇà
static char unk_46CF00[] = ""; // ªéÇà
static char unk_46CF34[] = ""; // ªèÇà
static char unk_46CF98[] = ""; // ªæÇà
static char unk_46CFE8[] = ""; // ªäÇà
static char unk_46D01C[] = ""; // ªâÇà
static char unk_46D05C[] = ""; // ªáÇà
static char unk_46D088[] = ""; // ªàÇà
static char unk_46D0A8[] = ""; // ªßÇà
static char unk_46D0D4[] = ""; // ªÞÇà
static char unk_46D124[] = ""; // ªÛÇà
static char unk_46D1FC[] = ""; // ªØÇà
static char unk_46D254[] = ""; // ªÕÇà
static char unk_46D2FC[] = ""; // ªÒÇà
static char unk_46D3C0[] = ""; // ªÏÇà
static char unk_46D4C8[] = ""; // ªÎÇà
static char unk_46D4F0[] = ""; // ªÍÇà
static char unk_46D514[] = ""; // ªÌÇà
static char unk_46D520[] = ""; // ªËÇà
static char unk_46D550[] = ""; // ªÊÇà
static char unk_46D584[] = ""; // ªÈÇà
// ªÆÇà & ¤¹Çà
static char unk_46D68C[] = "Â¥Â¦Â§Â¨Â©ÂªÂ«Â¬Â­Â®Â¯Â°Â±Â²Â³Â´ÂµÂ¶Â·Â¸rÂ¹ÂºÂ»Â¼Â½Â¾Â¿ÂÀÂÁÂÂÂÃÂÄÂÅÂÆÂÇÂÈÂÉÂÊÂËÂÌrÂÍÂÎÂÏÂÐÂÑÂÒÂÓÂÔÂÕÂÖÂ×ÂØÂÙÂÚÂÛÂÜÂÝÂÞÂßÂàrÂáÂâÂãÂäÂåÂæÂçÂèÂéÂêÂëÂìÂíÂîÂïÂðÂñÂòÂóÂôrÂõÂön";
// ªÄÇà & ¤¶Çà
static char unk_46D724[] = "½Î½Ï½Ð½Ñ½Ò½Ó½Ô½Õ½Ö½×½Ø½Ù½Ú½Û½Ü½Ý½Þ½ß½à½ár½â½ã½ä½å½æ½ç½è½é½ê½ë½ì½í½î½ï½ð½ñ½ò½ó½ô½õr½ö½÷½ø½ù½ú½û½ü½ý½þ¾¡¾¢¾£¾¤¾¥¾¦¾§¾¨¾©¾ª¾«r¾¬¾­¾®¾¯¾°¾±¾²¾³¾´¾µ¾¶¾·¾¸¾¹¾º¾»¾¼¾½¾¾¾¿r¾À¾Á¾Â¾Ã¾Ä¾Ån";
// ªÁÇà & ¤³Çà
static char unk_46D770[] = "ºüºýºþ»¡»¢»£»¤»¥»¦»§»¨»©»ª»«»¬»­»®»¯»°»±r»²»³»´»µ»¶»·»¸»¹»º»»»¼»½»¾»¿»À»Á»Â»Ã»Ä»År»Æ»Ç»È»É»Ê»Ë»Ì»Í»Î»Ï»Ð»Ñ»Ò»Ó»Ô»Õ»Ö»×»Ø»Ùr»Ú»Û»Ü»Ý»Þ»ß»à»á»â»ã»ä»å»æn";
// ª¿Çà & ¤¨Çà
static char unk_46D834[] = "µûµüµýµþ¶¡¶¢¶£¶¤¶¥¶¦¶§¶¨¶©¶ª¶«¶¬¶­¶®¶¯¶°r¶±¶²¶³¶´¶µ¶¶¶·¶¸¶¹¶º¶»¶¼¶½¶¾¶¿¶À¶Á¶Â¶Ã¶Är¶Å¶Æ¶Ç¶È¶É¶Ê¶Ë¶Ì¶Í¶Î¶Ï¶Ð¶Ñ¶Ò¶Ó¶Ô¶Õ¶Ö¶×¶Ør¶Ù¶Ú¶Û¶Ü¶Ý¶Þ¶ß¶à¶á¶â¶ã¶ä¶å¶æ¶ç¶è¶é¶ê¶ë¶ìr¶í¶î¶ï¶ð¶ñ¶òn";
// ª½Çà & ¤¢Çà
static char unk_46D924[] = "±î±ï±ð±ñ±ò±ó±ô±õ±ö±÷±ø±ù±ú±û±ü±ý±þ²¡²¢²£r²¤²¥²¦²§²¨²©²ª²«²¬²­²®²¯²°²±²²²³²´²µ²¶²·r²¸²¹²º²»²¼²½²¾²¿²À²Á²Â²Ã²Ä²Å²Æ²Ç²È²É²Ê²Ër²Ì²Í²Î²Ï²Ð²Ñ²Ò²Ó²Ô²Õ²Ö²×²Ø²Ù²Ú²Û²Ü²Ý²Þ²ßr²à²á²â²ã²ä²å²æ²ç²è²é²ê²ë²ì²í²î²ï²ð²ñ²ò²ór²ô²õ²ö²÷²ø²ù²ú²û²ü²ý²þ³¡³¢³£³¤³¥³¦³§³¨³©n";
// ª»Çà & ¤¾Çà
static char unk_46DA00[] = "ÇÏÇÐÇÑÇÒÇÓÇÔÇÕÇÖÇ×ÇØÇÙÇÚÇÛÇÜÇÝÇÞÇßÇàÇáÇârÇãÇäÇåÇæÇçÇèÇéÇêÇëÇìÇíÇîÇïÇðÇñÇòÇóÇôÇõÇörÇ÷ÇøÇùÇúÇûÇüÇýÇþÈ¡È¢È£È¤È¥È¦È§È¨È©ÈªÈ«È¬rÈ­È®È¯È°È±È²È³È´ÈµÈ¶È·È¸È¹ÈºÈ»È¼È½È¾È¿ÈÀrÈÁÈÂÈÃÈÄÈÅÈÆÈÇÈÈÈÉÈÊÈËÈÌÈÍÈÎÈÏÈÐÈÑÈÒÈÓÈÔrÈÕÈÖÈ×ÈØÈÙÈÚÈÛÈÜÈÝÈÞÈßÈàÈáÈâÈãÈäÈåÈæÈçÈèrÈéÈêÈëÈìÈíÈîÈïÈðÈñÈòÈóÈôÈõÈöÈ÷ÈøÈùÈúÈûÈürÈýÈþn";
// ª¹Çà & ¤½Çà
static char unk_46DAF8[] = "ÆÄÆÅÆÆÆÇÆÈÆÉÆÊÆËÆÌÆÍÆÎÆÏÆÐÆÑÆÒÆÓÆÔÆÕÆÖÆ×rÆØÆÙÆÚÆÛÆÜÆÝÆÞÆßÆàÆáÆâÆãÆäÆåÆæÆçÆèÆéÆêÆërÆìÆíÆîÆïÆðÆñÆòÆóÆôÆõÆöÆ÷ÆøÆùÆúÆûÆüÆýÆþÇ¡rÇ¢Ç£Ç¤Ç¥Ç¦Ç§Ç¨Ç©ÇªÇ«Ç¬Ç­Ç®Ç¯Ç°Ç±Ç²Ç³Ç´ÇµrÇ¶Ç·Ç¸Ç¹ÇºÇ»Ç¼Ç½Ç¾Ç¿ÇÀÇÁÇÂÇÃÇÄÇÅÇÆÇÇÇÈÇÉrÇÊÇËÇÌÇÍÇÎn";
// ª·Çà & ¤¼Çà
static char unk_46DB58[] = "Å¸Å¹ÅºÅ»Å¼Å½Å¾Å¿ÅÀÅÁÅÂÅÃÅÄÅÅÅÆÅÇÅÈÅÉÅÊÅËrÅÌÅÍÅÎÅÏÅÐÅÑÅÒÅÓÅÔÅÕÅÖÅ×ÅØÅÙÅÚÅÛÅÜÅÝÅÞÅßrÅàÅáÅâÅãÅäÅåÅæÅçÅèÅéÅêÅëÅìÅíÅîÅïÅðÅñÅòÅórÅôÅõÅöÅ÷ÅøÅùÅúÅûÅüÅýÅþÆ¡Æ¢Æ£Æ¤Æ¥Æ¦Æ§Æ¨Æ©rÆªÆ«Æ¬Æ­Æ®Æ¯Æ°Æ±Æ²Æ³Æ´ÆµÆ¶Æ·Æ¸Æ¹ÆºÆ»Æ¼Æ½rÆ¾Æ¿ÆÀÆÁÆÂÆÃn";
// ªµÇà & ¤»Çà
static char unk_46DEA4[] = "Ä«Ä¬Ä­Ä®Ä¯Ä°Ä±Ä²Ä³Ä´ÄµÄ¶Ä·Ä¸Ä¹ÄºÄ»Ä¼Ä½Ä¾rÄ¿ÄÀÄÁÄÂÄÃÄÄÄÅÄÆÄÇÄÈÄÉÄÊÄËÄÌÄÍÄÎÄÏÄÐÄÑÄÒrÄÓÄÔÄÕÄÖÄ×ÄØÄÙÄÚÄÛÄÜÄÝÄÞÄßÄàÄáÄâÄãÄäÄåÄærÄçÄèÄéÄêÄëÄìÄíÄîÄïÄðÄñÄòÄóÄôÄõÄöÄ÷ÄøÄùÄúrÄûÄüÄýÄþÅ¡Å¢Å£Å¤Å¥Å¦Å§Å¨Å©ÅªÅ«Å¬Å­Å®Å¯Å°rÅ±Å²Å³Å´ÅµÅ¶Å·n";
// ª³Çà & ¤ºÇà
static char unk_46DF98[] = "Â÷ÂøÂùÂúÂûÂüÂýÂþÃ¡Ã¢Ã£Ã¤Ã¥Ã¦Ã§Ã¨Ã©ÃªÃ«Ã¬rÃ­Ã®Ã¯Ã°Ã±Ã²Ã³Ã´ÃµÃ¶Ã·Ã¸Ã¹ÃºÃ»Ã¼Ã½Ã¾Ã¿ÃÀrÃÁÃÂÃÃÃÄÃÅÃÆÃÇÃÈÃÉÃÊÃËÃÌÃÍÃÎÃÏÃÐÃÑÃÒÃÓÃÔrÃÕÃÖÃ×ÃØÃÙÃÚÃÛÃÜÃÝÃÞÃßÃàÃáÃâÃãÃäÃåÃæÃçÃèrÃéÃêÃëÃìÃíÃîÃïÃðÃñÃòÃóÃôÃõÃöÃ÷ÃøÃùÃúÃûÃürÃýÃþÄ¡Ä¢Ä£Ä¤Ä¥Ä¦Ä§Ä¨Ä©Äªn";
// ª±Çà & ¤¸Çà
static char unk_46E108[] = "ÀÚÀÛÀÜÀÝÀÞÀßÀàÀáÀâÀãÀäÀåÀæÀçÀèÀéÀêÀëÀìÀírÀîÀïÀðÀñÀòÀóÀôÀõÀöÀ÷ÀøÀùÀúÀûÀüÀýÀþÁ¡Á¢Á£rÁ¤Á¥Á¦Á§Á¨Á©ÁªÁ«Á¬Á­Á®Á¯Á°Á±Á²Á³Á´ÁµÁ¶Á·rÁ¸Á¹ÁºÁ»Á¼Á½Á¾Á¿ÁÀÁÁÁÂÁÃÁÄÁÅÁÆÁÇÁÈÁÉÁÊÁËrÁÌÁÍÁÎÁÏÁÐÁÑÁÒÁÓÁÔÁÕÁÖÁ×ÁØÁÙÁÚÁÛÁÜÁÝÁÞÁßrÁàÁáÁâÁãÁäÁåÁæÁçÁèÁéÁêÁëÁìÁíÁîÁïÁðÁñÁòÁórÁôÁõÁöÁ÷ÁøÁùÁúÁûÁüÁýÁþÂ¡Â¢Â£Â¤n";
// ª¯Çà & ¤·Çà
static char unk_46E200[] = "¾Æ¾Ç¾È¾É¾Ê¾Ë¾Ì¾Í¾Î¾Ï¾Ð¾Ñ¾Ò¾Ó¾Ô¾Õ¾Ö¾×¾Ø¾Ùr¾Ú¾Û¾Ü¾Ý¾Þ¾ß¾à¾á¾â¾ã¾ä¾å¾æ¾ç¾è¾é¾ê¾ë¾ì¾ír¾î¾ï¾ð¾ñ¾ò¾ó¾ô¾õ¾ö¾÷¾ø¾ù¾ú¾û¾ü¾ý¾þ¿¡¿¢¿£r¿¤¿¥¿¦¿§¿¨¿©¿ª¿«¿¬¿­¿®¿¯¿°¿±¿²¿³¿´¿µ¿¶¿·r¿¸¿¹¿º¿»¿¼¿½¿¾¿¿¿À¿Á¿Â¿Ã¿Ä¿Å¿Æ¿Ç¿È¿É¿Ê¿Ër¿Ì¿Í¿Î¿Ï¿Ð¿Ñ¿Ò¿Ó¿Ô¿Õ¿Ö¿×¿Ø¿Ù¿Ú¿Û¿Ü¿Ý¿Þ¿ßr¿à¿á¿â¿ã¿ä¿å¿æ¿ç¿è¿é¿ê¿ë¿ì¿í¿î¿ï¿ð¿ñ¿ò¿ór¿ô¿õ¿ö¿÷¿ø¿ù¿ú¿û¿ü¿ý¿þÀ¡À¢À£À¤À¥À¦À§À¨À©rÀªÀ«À¬À­À®À¯À°À±À²À³À´ÀµÀ¶À·À¸À¹ÀºÀ»À¼À½rÀ¾À¿ÀÀÀÁÀÂÀÃÀÄÀÅÀÆÀÇÀÈÀÉÀÊÀËÀÌÀÍÀÎÀÏÀÐÀÑrÀÒÀÓÀÔÀÕÀÖÀ×ÀØÀÙn";
// ª­Çà & ¤µÇà
static char unk_46E270[] = "»ç»è»é»ê»ë»ì»í»î»ï»ð»ñ»ò»ó»ô»õ»ö»÷»ø»ù»úr»û»ü»ý»þ¼¡¼¢¼£¼¤¼¥¼¦¼§¼¨¼©¼ª¼«¼¬¼­¼®¼¯¼°r¼±¼²¼³¼´¼µ¼¶¼·¼¸¼¹¼º¼»¼¼¼½¼¾¼¿¼À¼Á¼Â¼Ã¼Är¼Å¼Æ¼Ç¼È¼É¼Ê¼Ë¼Ì¼Í¼Î¼Ï¼Ð¼Ñ¼Ò¼Ó¼Ô¼Õ¼Ö¼×¼Ør¼Ù¼Ú¼Û¼Ü¼Ý¼Þ¼ß¼à¼á¼â¼ã¼ä¼å¼æ¼ç¼è¼é¼ê¼ë¼ìr¼í¼î¼ï¼ð¼ñ¼ò¼ó¼ô¼õ¼ö¼÷¼ø¼ù¼ú¼û¼ü¼ý¼þ½¡½¢r½£½¤½¥½¦½§½¨½©½ª½«½¬½­½®½¯½°½±½²½³½´½µ½¶r½·½¸½¹½º½»½¼½½½¾½¿½À½Á½Â½Ã½Ä½Å½Æ½Ç½È½É½Êr½Ë½Ì½Ín";
// ª«Çà & ¤²Çà
static char unk_46E3F8[] = "¹Ù¹Ú¹Û¹Ü¹Ý¹Þ¹ß¹à¹á¹â¹ã¹ä¹å¹æ¹ç¹è¹é¹ê¹ë¹ìr¹í¹î¹ï¹ð¹ñ¹ò¹ó¹ô¹õ¹ö¹÷¹ø¹ù¹ú¹û¹ü¹ý¹þº¡º¢rº£º¤º¥º¦º§º¨º©ºªº«º¬º­º®º¯º°º±º²º³º´ºµº¶rº·º¸º¹ººº»º¼º½º¾º¿ºÀºÁºÂºÃºÄºÅºÆºÇºÈºÉºÊrºËºÌºÍºÎºÏºÐºÑºÒºÓºÔºÕºÖº×ºØºÙºÚºÛºÜºÝºÞrºßºàºáºâºãºäºåºæºçºèºéºêºëºìºíºîºïºðºñºòrºóºôºõºöº÷ºøºùºúºûn";
// ªªÇà & ¤±Çà
static char unk_46E5F4[] = "¸¶¸·¸¸¸¹¸º¸»¸¼¸½¸¾¸¿¸À¸Á¸Â¸Ã¸Ä¸Å¸Æ¸Ç¸È¸Ér¸Ê¸Ë¸Ì¸Í¸Î¸Ï¸Ð¸Ñ¸Ò¸Ó¸Ô¸Õ¸Ö¸×¸Ø¸Ù¸Ú¸Û¸Ü¸Ýr¸Þ¸ß¸à¸á¸â¸ã¸ä¸å¸æ¸ç¸è¸é¸ê¸ë¸ì¸í¸î¸ï¸ð¸ñr¸ò¸ó¸ô¸õ¸ö¸÷¸ø¸ù¸ú¸û¸ü¸ý¸þ¹¡¹¢¹£¹¤¹¥¹¦¹§r¹¨¹©¹ª¹«¹¬¹­¹®¹¯¹°¹±¹²¹³¹´¹µ¹¶¹·¹¸¹¹¹º¹»r¹¼¹½¹¾¹¿¹À¹Á¹Â¹Ã¹Ä¹Å¹Æ¹Ç¹È¹É¹Ê¹Ë¹Ì¹Í¹Î¹Ïr¹Ð¹Ñ¹Ò¹Ó¹Ô¹Õ¹Ö¹×¹Øn";
// ª¨Çà & ¤©Çà
static char unk_46E64C[] = "¶ó¶ô¶õ¶ö¶÷¶ø¶ù¶ú¶û¶ü¶ý¶þ·¡·¢·£·¤·¥·¦·§·¨r·©·ª·«·¬·­·®·¯·°·±·²·³·´·µ·¶···¸·¹·º·»·¼r·½·¾·¿·À·Á·Â·Ã·Ä·Å·Æ·Ç·È·É·Ê·Ë·Ì·Í·Î·Ï·Ðr·Ñ·Ò·Ó·Ô·Õ·Ö·×·Ø·Ù·Ú·Û·Ü·Ý·Þ·ß·à·á·â·ã·är·å·æ·ç·è·é·ê·ë·ì·í·î·ï·ð·ñ·ò·ó·ô·õ·ö·÷·ør·ù·ú·û·ü·ý·þ¸¡¸¢¸£¸¤¸¥¸¦¸§¸¨¸©¸ª¸«¸¬¸­¸®r¸¯¸°¸±¸²¸³¸´¸µn";
// ª¦Çà & ¤§Çà
static char unk_46E6D0[] = "´Ù´Ú´Û´Ü´Ý´Þ´ß´à´á´â´ã´ä´å´æ´ç´è´é´ê´ë´ìr´í´î´ï´ð´ñ´ò´ó´ô´õ´ö´÷´ø´ù´ú´û´ü´ý´þµ¡µ¢rµ£µ¤µ¥µ¦µ§µ¨µ©µªµ«µ¬µ­µ®µ¯µ°µ±µ²µ³µ´µµµ¶rµ·µ¸µ¹µºµ»µ¼µ½µ¾µ¿µÀµÁµÂµÃµÄµÅµÆµÇµÈµÉµÊrµËµÌµÍµÎµÏµÐµÑµÒµÓµÔµÕµÖµ×µØµÙµÚµÛµÜµÝµÞrµßµàµáµâµãµäµåµæµçµèµéµêµëµìµíµîµïµðµñµòrµóµôµõµöµ÷µøµùµún";
// ª¤Çà & ¤¤Çà
static char unk_46E718[] = "³ª³«³¬³­³®³¯³°³±³²³³³´³µ³¶³·³¸³¹³º³»³¼³½r³¾³¿³À³Á³Â³Ã³Ä³Å³Æ³Ç³È³É³Ê³Ë³Ì³Í³Î³Ï³Ð³Ñr³Ò³Ó³Ô³Õ³Ö³×³Ø³Ù³Ú³Û³Ü³Ý³Þ³ß³à³á³â³ã³ä³år³æ³ç³è³é³ê³ë³ì³í³î³ï³ð³ñ³ò³ó³ô³õ³ö³÷³ø³ùr³ú³û³ü³ý³þ´¡´¢´£´¤´¥´¦´§´¨´©´ª´«´¬´­´®´¯r´°´±´²´³´´´µ´¶´·´¸´¹´º´»´¼´½´¾´¿´À´Á´Â´Ãr´Ä´Å´Æ´Ç´È´É´Ê´Ë´Ì´Í´Î´Ï´Ð´Ñ´Ò´Ó´Ô´Õ´Ö´×r´Øn";
// ª¢Çà & ¤¡Çà
static char unk_46E7A0[] = "°¡°¢°£°¤°¥°¦°§°¨°©°ª°«°¬°­°®°¯°°°±°²°³°´r°µ°¶°·°¸°¹°º°»°¼°½°¾°¿°À°Á°Â°Ã°Ä°Å°Æ°Ç°Èr°É°Ê°Ë°Ì°Í°Î°Ï°Ð°Ñ°Ò°Ó°Ô°Õ°Ö°×°Ø°Ù°Ú°Û°Ür°Ý°Þ°ß°à°á°â°ã°ä°å°æ°ç°è°é°ê°ë°ì°í°î°ï°ðr°ñ°ò°ó°ô°õ°ö°÷°ø°ù°ú°û°ü°ý°þ±¡±¢±£±¤±¥±¦r±§±¨±©±ª±«±¬±­±®±¯±°±±±²±³±´±µ±¶±·±¸±¹±ºr±»±¼±½±¾±¿±À±Á±Â±Ã±Ä±Å±Æ±Ç±È±É±Ê±Ë±Ì±Í±Îr±Ï±Ð±Ñ±Ò±Ó±Ô±Õ±Ö±×±Ø±Ù±Ú±Û±Ü±Ý±Þ±ß±à±á±âr±ã±ä±å±æ±ç±è±é±ê±ë±ì±ín";
static char unk_46E804[] = {   // ¿µ¼ýÀÚ
    0x82, 0x4F, 0x82, 0x50, 0x82, 0x51, 0x82, 0x52, 0x82, 0x53, 0x82, 0x54, 0x82, 0x55, 0x82, 0x56,
    0x82, 0x57, 0x82, 0x58, 0x72, 0x82, 0x60, 0x82, 0x61, 0x82, 0x62, 0x82, 0x63, 0x82, 0x64, 0x82,
    0x65, 0x82, 0x66, 0x82, 0x67, 0x82, 0x68, 0x82, 0x69, 0x82, 0x6A, 0x82, 0x6B, 0x82, 0x6C, 0x82,
    0x6D, 0x82, 0x6E, 0x82, 0x6F, 0x82, 0x70, 0x82, 0x71, 0x82, 0x72, 0x82, 0x73, 0x72, 0x82, 0x74,
    0x82, 0x75, 0x82, 0x76, 0x82, 0x77, 0x82, 0x78, 0x82, 0x79, 0x72, 0x82, 0x81, 0x82, 0x82, 0x82,
    0x83, 0x82, 0x84, 0x82, 0x85, 0x82, 0x86, 0x82, 0x87, 0x82, 0x88, 0x82, 0x89, 0x82, 0x8A, 0x82,
    0x8B, 0x82, 0x8C, 0x82, 0x8D, 0x82, 0x8E, 0x82, 0x8F, 0x82, 0x90, 0x82, 0x91, 0x82, 0x92, 0x82,
    0x93, 0x82, 0x94, 0x72, 0x82, 0x95, 0x82, 0x96, 0x82, 0x97, 0x82, 0x98, 0x82, 0x99, 0x82, 0x9A,
    0x72, 0x81, 0x49, 0x81, 0x48, 0x81, 0x95, 0x81, 0x45, 0x81, 0x5E, 0x81, 0x75, 0x81, 0x76, 0x81,
    0x69, 0x81, 0x6A, 0x81, 0x77, 0x81, 0x78, 0x81, 0x99, 0x81, 0x9B, 0x81, 0x9D, 0x81, 0x44, 0x6E,
    0x00
};
static char unk_46E8A8[] = {   // Ä«Å¸Ä«³ª
    0x83, 0x41, 0x83, 0x43, 0x83, 0x45, 0x83, 0x47, 0x83, 0x49, 0x20, 0x83, 0x4A, 0x83, 0x4C, 0x83,
    0x4E, 0x83, 0x50, 0x83, 0x52, 0x20, 0x83, 0x54, 0x83, 0x56, 0x83, 0x58, 0x83, 0x5A, 0x83, 0x5C,
    0x20, 0x83, 0x5E, 0x83, 0x60, 0x83, 0x63, 0x83, 0x65, 0x83, 0x67, 0x72, 0x83, 0x69, 0x83, 0x6A,
    0x83, 0x6B, 0x83, 0x6C, 0x83, 0x6D, 0x20, 0x83, 0x6E, 0x83, 0x71, 0x83, 0x74, 0x83, 0x77, 0x83,
    0x7A, 0x20, 0x83, 0x7D, 0x83, 0x7E, 0x83, 0x80, 0x83, 0x81, 0x83, 0x82, 0x20, 0x83, 0x84, 0x20,
    0x83, 0x86, 0x20, 0x83, 0x88, 0x72, 0x83, 0x89, 0x83, 0x8A, 0x83, 0x8B, 0x83, 0x8C, 0x83, 0x8D,
    0x20, 0x83, 0x8F, 0x20, 0x83, 0x92, 0x20, 0x83, 0x93, 0x72, 0x83, 0x4B, 0x83, 0x4D, 0x83, 0x4F,
    0x83, 0x51, 0x83, 0x53, 0x20, 0x83, 0x55, 0x83, 0x57, 0x83, 0x59, 0x83, 0x5B, 0x83, 0x5D, 0x20,
    0x83, 0x5F, 0x83, 0x61, 0x83, 0x64, 0x83, 0x66, 0x83, 0x68, 0x20, 0x83, 0x6F, 0x83, 0x72, 0x83,
    0x75, 0x83, 0x78, 0x83, 0x7B, 0x72, 0x83, 0x70, 0x83, 0x73, 0x83, 0x76, 0x83, 0x79, 0x83, 0x7C,
    0x20, 0x83, 0x40, 0x83, 0x42, 0x83, 0x44, 0x83, 0x46, 0x83, 0x48, 0x20, 0x83, 0x62, 0x83, 0x83,
    0x83, 0x85, 0x83, 0x87, 0x83, 0x8E, 0x20, 0x83, 0x95, 0x83, 0x96, 0x83, 0x94, 0x81, 0x5B, 0x6E,
    0x00
};
static char unk_46E96C[] = {   // È÷¶ó°¡³ª
    0x82, 0xA0, 0x82, 0xA2, 0x82, 0xA4, 0x82, 0xA6, 0x82, 0xA8, 0x20, 0x82, 0xA9, 0x82, 0xAB, 0x82,
    0xAD, 0x82, 0xAF, 0x82, 0xB1, 0x20, 0x82, 0xB3, 0x82, 0xB5, 0x82, 0xB7, 0x82, 0xB9, 0x82, 0xBB,
    0x20, 0x82, 0xBD, 0x82, 0xBF, 0x82, 0xC2, 0x82, 0xC4, 0x82, 0xC6, 0x72, 0x82, 0xC8, 0x82, 0xC9,
    0x82, 0xCA, 0x82, 0xCB, 0x82, 0xCC, 0x20, 0x82, 0xCD, 0x82, 0xD0, 0x82, 0xD3, 0x82, 0xD6, 0x82,
    0xD9, 0x20, 0x82, 0xDC, 0x82, 0xDD, 0x82, 0xDE, 0x82, 0xDF, 0x82, 0xE0, 0x20, 0x82, 0xE2, 0x20,
    0x82, 0xE4, 0x20, 0x82, 0xE6, 0x72, 0x82, 0xE7, 0x82, 0xE8, 0x82, 0xE9, 0x82, 0xEA, 0x82, 0xEB,
    0x20, 0x82, 0xED, 0x20, 0x82, 0xF0, 0x20, 0x82, 0xF1, 0x72, 0x82, 0xAA, 0x82, 0xAC, 0x82, 0xAE,
    0x82, 0xB0, 0x82, 0xB2, 0x20, 0x82, 0xB4, 0x82, 0xB6, 0x82, 0xB8, 0x82, 0xBA, 0x82, 0xBC, 0x20,
    0x82, 0xBE, 0x82, 0xC0, 0x82, 0xC3, 0x82, 0xC5, 0x82, 0xC7, 0x20, 0x82, 0xCE, 0x82, 0xD1, 0x82,
    0xD4, 0x82, 0xD7, 0x82, 0xDA, 0x72, 0x82, 0xCF, 0x82, 0xD2, 0x82, 0xD5, 0x82, 0xD8, 0x82, 0xDB,
    0x20, 0x82, 0x9F, 0x82, 0xA1, 0x82, 0xA3, 0x82, 0xA5, 0x82, 0xA7, 0x20, 0x82, 0xC1, 0x82, 0xE1,
    0x82, 0xE3, 0x82, 0xE5, 0x82, 0xEC, 0x20, 0x81, 0x5B, 0x6E, 0x00
};
static char unk_46EA28[] = "¤¡¤¤¤§¤©¤±¤²¤µ¤·¤¸¤º¤»¤¼¤½¤¾r¤¢ ¤¨  ¤³¤¶ ¤¹n"; // ÇÑÀÚ
static int size_46CD7C = 0x1D18;
static char* strings_46CD7C[] = {
    unk_46CD7C, unk_46CD84, unk_46CD8C, unk_46CDC0, unk_46CE04, unk_46CE54, unk_46CE6C, unk_46CF00,
    unk_46CF34, unk_46CF98, unk_46CFE8, unk_46D01C, unk_46D05C, unk_46D088, unk_46D0A8, unk_46D0D4,
    unk_46D124, unk_46D1FC, unk_46D254, unk_46D2FC, unk_46D3C0, unk_46D4C8, unk_46D4F0, unk_46D514,
    unk_46D520, unk_46D550, unk_46D584, unk_46D68C, unk_46D724, unk_46D770, unk_46D834, unk_46D924,
    unk_46DA00, unk_46DAF8, unk_46DB58, unk_46DEA4, unk_46DF98, unk_46E108, unk_46E200, unk_46E270,
    unk_46E3F8, unk_46E5F4, unk_46E64C, unk_46E6D0, unk_46E718, unk_46E7A0, unk_46E804, unk_46E8A8,
    unk_46E96C, unk_46EA28
};
static char* def_46CCB4[] = {
    unk_46EA28, unk_46E96C, unk_46E8A8, unk_46E804, unk_46E7A0, unk_46E718, unk_46E6D0, unk_46E64C,
    unk_46E5F4, unk_46E3F8, unk_46E270, unk_46E200, unk_46E108, unk_46DF98, unk_46DEA4, unk_46DB58,
    unk_46DAF8, unk_46DA00, unk_46D924, unk_46D834, unk_46D770, unk_46D724, unk_46D68C, unk_46D584,
    unk_46D550, unk_46D520, unk_46D514, unk_46D4F0, unk_46D4C8, unk_46D3C0, unk_46D2FC, unk_46D254,
    unk_46D1FC, unk_46D124, unk_46D0D4, unk_46D0A8, unk_46D088, unk_46D05C, unk_46D01C, unk_46CFE8,
    unk_46CF98, unk_46CF34, unk_46CF00, unk_46CE6C, unk_46CE54, unk_46CE04, unk_46CDC0, unk_46CD8C,
    unk_46CD84, unk_46CD7C
};


static char unk_46C12C[] = "%s %s";
static int size_46C12C = 0x8;
static char* strings_46C12C[] = { unk_46C12C };


static char unk_46F0CC[] = "ÀÌ¾ß, ÀÌ·± ÁÁÀº ÀÏµµ ÀÖ³×";
static char unk_46F0F0[] = "¹¹¹¹,¡ºÄ£ÇØÁø ±â³äÀ¸·Î ÀÌ ¿ø°í¸¦ ¹Þ¾Æ Áà¡»";
static char unk_46F124[] = "ºê¶ó´õ£²";
static char unk_46F130[] = "¼¾µµ\xb2\x70Ä«ÁîÅ°";
static char unk_46F13C[] = "Ä«ÁîÅ°";
static char unk_46F144[] = "¼¾µµ";
static char unk_46F14C[] = "%s ´ÔÀÇ ¿ø°í°¡ ¿Ã¶ó¿Ô½À´Ï´Ù";
static char unk_46F168[] = "ÄÃ·¯ ±â¼úÀÌ %s±îÁö ¿Ã¶ú½À´Ï´Ù";
static char unk_46F184[] = "ÃâÁ¡ ¼¾½º°¡ %s±îÁö ¿Ã¶ú½À´Ï´Ù";
static char unk_46F1A0[] = "Ã¢ÀÇ·ÂÀÌ %s±îÁö ¿Ã¶ú½À´Ï´Ù";
static char unk_46F1B8[] = "±×¸² ½Ç·ÂÀÌ %s±îÁö ¿Ã¶ú½À´Ï´Ù";
static char unk_46F1D0[] = "¸¶¹«¸® ±â¼úÀÌ %s±îÁö ¿Ã¶ú½À´Ï´Ù";
static char unk_46F1EC[] = "ÄÜÆ¼°¡ ³¡³µ½À´Ï´Ù";
static char unk_46F204[] = "ÆæÅÍÄ¡°¡ ³¡³µ½À´Ï´Ù";
static char unk_46F21C[] = "¿ø°í°¡ ¿Ï¼ºµÇ¾ú½À´Ï´Ù";
static char unk_46F230[] = "Ã¥ÀÇ ÁúÀÌ ÁÁ¾ÆÁ³½À´Ï´Ù";
static char unk_46F248[] = "Ã¥ÀÇ ÁúÀÌ Á¶±Ý ÁÁ¾ÆÁ³½À´Ï´Ù";
static char unk_46F264[] = "±Ý¼Õ¿¡ ÇÑ °ÉÀ½ ´Ù°¡°¬½À´Ï´Ù";
static char unk_46F27C[] = "Á¶±Ý Áñ°Å¿öÁ³½À´Ï´Ù";
static char unk_46F294[] = "%s ´Ô¿¡°Ô¼­ ¿ø°í°¡ µµÂøÇß½À´Ï´Ù";
static char unk_46F2B0[] = "¾Æ¹« ÀÏµµ ÀÏ¾î³ªÁö ¾Ê¾Ò½À´Ï´Ù";
static int size_46F0CC = 0x1FC;
static char* strings_46F0CC[] = {
    unk_46F0CC, unk_46F0F0, unk_46F124, unk_46F130, unk_46F13C, unk_46F144, unk_46F14C, unk_46F168,
    unk_46F184, unk_46F1A0, unk_46F1B8, unk_46F1D0, unk_46F1EC, unk_46F204, unk_46F21C, unk_46F230,
    unk_46F248, unk_46F264, unk_46F27C, unk_46F294, unk_46F2B0
};
static char* def_46F0C4[] = { unk_46F0F0, unk_46F0CC };
static char* def_46EFB0[] = { unk_46F144, unk_46F13C, unk_46F130, unk_46F124 };
static char* def_4403A7[] = { unk_46F14C };
static char* def_4405DD[] = { unk_46F168 };
static char* def_440BE7[] = { unk_46F168 };
static char* def_440D33[] = { unk_46F168 };
static char* def_440A2D[] = { unk_46F184 };
static char* def_440A98[] = { unk_46F1A0 };
static char* def_440E6F[] = { unk_46F1A0 };
static char* def_440B07[] = { unk_46F1B8 };
static char* def_441001[] = { unk_46F1B8 };
static char* def_440B77[] = { unk_46F1D0 };
static char* def_4411BF[] = { unk_46F1D0 };
static char* def_440E18[] = { unk_46F1EC };
static char* def_440FA9[] = { unk_46F204 };
static char* def_44114F[] = { unk_46F21C };
static char* def_44146A[] = { unk_46F230 };
static char* def_4413EC[] = { unk_46F264 };
static char* def_4412D7[] = { unk_46F27C };
static char* def_441570[] = { unk_46F294 };
static char* def_441524[] = { unk_46F2B0 };
static char* def_44153E[] = { unk_46F2B0 };


static char unk_46F2F0[] = "µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù";
static int size_46F2F0 = 0x18;
static char* strings_46F2F0[] = { unk_46F2F0 };


static char unk_46F3A8[] = "±×·³, ÈÄµü ½ÅÃ»ÇØ ¹ö¸®ÀÚ.";
static char unk_46F3D8[] = "ÀÀ? ³×°¡ Á÷Á¢ ½ÅÃ»ÇÏ°í ½Í´Ù°í? ÀÌºÁ, ³Í ³»°¡ ¸»ÇÏ´Â ´ë·Î¸¸ ¾²¸é µÅ. ¾Ë°Ú¾î?";
static char unk_46F430[] = "³Êµµ ³ªÃ³·³ ÃÊ´ëÇü µ¿ÀÎ ÀÛ°¡°¡ µÇ°í ½ÍÁö? ±×·³, ³» ¸»¸¸ µè´Â °Å¾ß.";
static char unk_46F498[] = "³» ¿¹»óÀ¸·Ð, ÀÌ ±ÙÃ³°¡ ³ë¸²¼ö¾ß. ºÐ¸í ÀÌ ¼ÒÀç°¡ ¶ã °Å¾ß.";
static char unk_46F4E8[] = "ÀÌ·± ·ù¶ó¸é¡¦¡¦ ¹¹, ³»¿ëÀº ÀÌ Á¤µµÁö. ³» °ßº»´ë·Î ±×¸®¸é ±×·°Àú·° µÉ °Å¾ß.";
static char unk_46F544[] = "Ç¥Áö·Î ¸ÕÀú ¿ÀÅ¸Äí ¸¶À½À» Àâ¾Æ¾ß ÇÏ´Ï±î ÈûÁà¼­ ±×·Á.";
static char unk_46F594[] = "ÀÌ°Í¸¸ÀÌ¶óµµ º¸±â ÁÁ°Ô ¸¸µé¸é »ç ÁÙ °Å¾ß.";
static char unk_46F5CC[] = "ÆäÀÌÁö´Â ÀÌ Á¤µµ·Á³ª? ¾Æ¹«¸® Ç²³»±âÀÎ ³Ê¶óµµ ÀÌ Á¤µµ¸¦ ¸ø ±×¸± ¸®´Â ¾øÁö?";
static char unk_46F62C[] = "½ÇÆÐÇÏ¸é ³ª±îÁö ¸Á½Å´çÇÏ´Ï±î, Á×¾îµµ ÇØ³»¾ß ÇØ.";
static char unk_46F668[] = "Áú¹®Àº¡¦¡¦ ¾ø°Ú±º. ¹¹? ³× ÀÇ°ßÀº ¾È µé¾î. ¾îÂ÷ÇÇ ¾µ¸ð ¾øÀ» °Å´Ï±î, Á¶¿ëÈ÷ ±×¸®±â³ª ÇØ.";
static char unk_46F6D4[] = "ÀÚ, ¾î¼­ ´ë»ç¶óµµ Â¥. ÇÑ°¡ÇÏ¸é »óÅÂ º¸·¯ ¿Ã°Ô.";
static int size_46F3A8 = 0x36C;
static char* strings_46F3A8[] = {
    unk_46F3A8, unk_46F3D8, unk_46F430, unk_46F498, unk_46F4E8, unk_46F544, unk_46F594, unk_46F5CC,
    unk_46F62C, unk_46F668, unk_46F6D4
};
static char* def_4234F2[] = { unk_46F3A8 };
static char* def_42351F[] = { unk_46F3D8 };
static char* def_42354A[] = { unk_46F430 };
static char* def_42428E[] = { unk_46F498 };
static char* def_4499AE[] = { unk_46F4E8 };
static char* def_41044C[] = { unk_46F544 };
static char* def_410471[] = { unk_46F594 };
static char* def_43B09C[] = { unk_46F5CC };
static char* def_43B0C1[] = { unk_46F62C };
static char* def_4229B2[] = { unk_46F668 };
static char* def_4229C4[] = { unk_46F6D4 };


static char unk_473A80[] = "°¡¿Í´Ù À¯¡¡´ÙÃ¤·Î¿î Ä³¸¯ÅÍ·Î ½ÂºÎÇÏ´Â ¿øÈ­°¡. ¿äÁòÀº ºóÀ¯ Ä³¸¯ÅÍ ÃëÇâÀÎ ¸ð¾çÀÌ´Ù.";
static char unk_473ACC[] = "³×³ëÃ÷Å°£¦·Î¹Â¡¡´ÙÅ©ÇÑ ³×³ëÃ÷Å°¿Í ÆËÇÑ ·Î¹Â, µÑÀÇ °³¼ºÀÌ ºû³ª´Â À¯´Ö.";
static char unk_473B18[] = "»çÄí¶õ¡¡¼³Á¤¿¡ ÁýÂøÇÏ´Â ÀÌ·ÐÆÄ. ÇÏÁö¸¸ ÅÂÅ¬À» ¹ÞÀ¸¸é µ¥¹ÌÁö¸¦ Å©°Ô ÀÔ´Â Å¸ÀÔÀÎ ¸ð¾ç.";
static char unk_473B64[] = "Ä­´©Å° ¿ä¾ÆÄÉ¡¡Æ÷±ÙÇÑ ºÐÀ§±âÀÇ ÀÛÇ³. ÀÎ°£°ü°èÀÇ ½Ã²ô·¯¿î ´ÙÅù¿¡ ¾àÇÑ ¼¶¼¼ÇÑ »ç¶÷.";
static char unk_473BB0[] = "ÇÏ±â¾ß¸¶ »çÄ«°Ô¡¡±Í¿©¿î ±×¸²Ã¼°¡ Æ¯±â. ¾Ë¸ö ¾ÕÄ¡¸¶¸¦ ¹«Ã´ ÁÁ¾ÆÇÏ´Â ¸ð¾çÀÌ´Ù.";
static char unk_473C00[] = "»ï°¢ÇüÀÇ ³× ¹øÂ° º¯¡¡½´¸£ÇÑ ÀÛÇ³ÀÌ ¹®ÇÐÀûÀÌ´Ù. ¿ø°íÀÇ ¿Ü°¢À» °Ë°Ô Ä¥ÇÏ´Â °É ÁÁ¾ÆÇÑ´Ù.";
static char unk_473C4C[] = "¹Ì³ªÁîÅ° Åä¿À·ç¡¡¹Ùº¸ Ä³¸¯ÀÌ Æ¯±â¶ó°í ¸»ÇÏÁö¸¸, ±×°Í¸¸ÀÌ ¾Æ´Ï¶ó´Â °Ç ³Î¸® ¾Ë·ÁÁø »ç½Ç.";
static char unk_473C98[] = "´ÙÄ«ÇÏ½Ã Å¸Ã÷¾ß¡¡ÀÌ¾ß±â ±¸¼º¿¡ Á¤Æò. ¾î¶² Å×¸¶µç ¹èÆ² Àå¸é¿¡ ÁýÂøÇÏ´Â °Ô ½ÅÁ¶.";
static char unk_473CE8[] = "¿À¿À³ë Å×Ã÷¾ß¡¡°³¼º ÀÖ´Â ±Í¿©¿î Ä³¸¯ÅÍ¿Í Æ÷±ÙÇÑ ÀÌ¾ß±â ¸¸µé±â·Î ÀÎ±â.";
static char unk_473D2C[] = "È÷¶óÅ° ³ª¿ÀÅä½Ã¡¡±Í¿©¿î Ä³¸¯ÅÍ¿Í´Â ¹Ý´ë·Î ÇÏµåÇÑ ÀÛÇ³µµ Á¦´ë·Î º¸¿© ÁØ´Ù.";
static char unk_473D6C[] = "Áø³ëÁ¶£¦Äí·Îº¸½Ã ÄÚ¿ìÇÏÄí¡¡¼­·Î ´Ù¸¥ »öÀÌ À¶ÇÕµÇ¾î, ½ÂÈ­µÈ ÀÎ±â À¯´Ö.";
static char unk_473DB0[] = "¹ÌÁîÅ¸´Ï Åä¿À·ç¡¡°ÔÀÓ ¿øÈ­¸¦ °ÅÃÄ Áö±Ýµµ °è¼Ó ¶ß´Â ÀÎ±â ÀÛ°¡.";
static char unk_473DF0[] = "ÄÚ¹«·Î ÄÉÀÌ½ºÄÉ¡¡¼¼·ÃµÈ ÆæÅÍÄ¡¿Í ±ò²ûÇÑ ±×¸²Ã¼°¡ ¸Å·Â.";
static char unk_473E24[] = "Å°¹«¶ó¾ß ·á¡¡Ä³¸¯ÅÍº¸´Ù Ã¤»ö ±× ÀÚÃ¼¿¡ ÁýÂøÇÏ´Â ÀåÀÎÇü £Ã£ÇÀÛ°¡.";
static char unk_473E6C[] = "Å¸ÄÉ¿ìÄ¡ ¿ä½Ã¹Ì¡¡¸¶ÀÌ³ÊÇÑ ¼ÒÀç¸¦ ÁÁ¾ÆÇÏ¸ç ÀÚ±â ÆäÀÌ½º·Î È°µ¿À» ÀÌ¾î°¡´Â Áß.";
static char unk_473EB0[] = "Ä­·ÎÁÖ¡¡µÎ±ÙµÎ±Ù ºÎ²ô·¯¿î Á¤¼® ½ºÅä¸®°¡ Æ¯±â.";
static char unk_473EE4[] = "¹ÌÃ÷¹Ì ¹Ì»çÅä¡¡³ë·¡µµ Ããµµ ÄÚ½ºÇÁ·¹µµ °¡´ÉÇÑ ÀÎ±â ¿©¼º µ¿ÀÎ ÀÛ°¡.";
static char unk_473F20[] = "£Ã£È£Á£Ò£Í¡¡±Í¿©¿î Ä³¸¯ÅÍ¸¦ ¸¸µå´Â °Í¿¡ ÁýÂøÇÏ´Â £Ã£ÇÀÛ°¡.";
static char unk_473F64[] = "¹Ý·ÎÈ£¡¡ÁÙ¹«´Ì ÆÒÆ¼ ÁýÂøÀº ¾÷°è ÃÖ°í ¼öÁØ.";
static char unk_473FA0[] = "»ê¼î ³ª³ª¹Ì¡¡±Í¿©¿î ±×¸²Ã¼ÀÇ Ä³¸¯ÅÍ°¡ Æ¯±âÀÎ £Ã£ÇÀÛ°¡.";
static char unk_473FE4[] = "£Ã£È£Ï£Ã£Ï¡¡¼¼·ÃµÈ µðÀÚÀÎÀº µ¿ÀÎ°è ÃÖ°í¶ó´Â ¼Ò¹®.";
static char unk_47401C[] = "È÷¶óÅ° ³ª¿À¸®£¦±«±¤¼±£¦¿ä½Ã´Ù °íÅÙ¡¡¼¼ »ç¶÷ÀÇ ¼­·Î ´Ù¸¥ »öÀÌ À¶ÇÕµÇ¾î ½ÂÈ­µÈ ÀÎ±â À¯´Ö.";
static char unk_47406C[] = "³ª³ª¼¼ ¾Æ¿ÀÀÌ¡¡¼¼·ÃµÈ Ä³¸¯ÅÍ´Â ´©±¸µç ¸Å·áµÈ´Ù.";
static char unk_4740A4[] = "£ö£ï£ç£õ£å¡¡¼ö¸¹Àº °ÔÀÓ ¿øÈ­¸¦ ¸Ã¾Æ ¿Â º£Å×¶û ÀÛ°¡.";
static char unk_4740E0[] = "¾ÆÀÚ¶ó½Ã Ä«Áî¸¶¡¡Áü½Â ¼Ò³à¿¡ ´ëÇÑ °íÁýÀ» ³¡±îÁö ÁöÄÑ ¿Â ¿øÈ­°¡.";
static char unk_47411C[] = "´Ï½ÃÅ° ½Ã·Î¡¡±Í¿©¿î ±×¸²Ã¼¿Í ´ÙÃ¤·Î¿î ÀÌ¾ß±â ±¸¼ºÀ¸·Î ÀÐÈ÷´Â ÈûÀÌ ÀÖ´Ù.";
static char unk_474150[] = "Å°Ä¡Äí È÷·ÎÄÚ¡¡±Í¿©¿î ±×¸²Ã¼¿¡ ½´¸£ÇÑ °³±×¸¦ µ¶ÇÏ°Ô ¾ñÀº ¸À.";
static char unk_474188[] = "Á¦Äí¿ì Åä¿À·ç¡¡ÆÄ±«·Â ÀÖ´Â °³±×¿Í ¸Å´Ï¾ÇÇÑ ¼ÒÀç·Î Á¤¸é½ÂºÎ.";
static char unk_4741C0[] = "ÀÚ·Î¿ì ¾ÆÅ°¶ó¡¡±Í¿©¿î ±×¸²Ã¼¿Í ´ÙÃ¤·Î¿î ÀÌ¾ß±â ¸¸µé±â·Î ÀÎ±â¸ôÀÌ Áß.";
static char unk_4741F8[] = "ÀÌÃ÷Å°£¦ÄËÅ¸¡¡ÁÖ·Î £Ã£ÇÂÊ¿¡¼­ È°¾à Áß. Ä³¸¯ÅÍÀÇ ±Í¿©¿òÀÌ ºû³­´Ù.";
static char unk_474234[] = "¿ä½ÃÀÚ¿Í Åä¸ð¾ÆÅ°£¦¿À¿ÀÃ÷Å° ·áÅ°¡¡°ÔÀÓ ¿øÈ­°¡¿Í ½Ã³ª¸®¿À ¶óÀÌÅÍÀÇ °­·Â ÅÂ±×.";
static char unk_47427C[] = "»çÀÌÅä¿ì Ã÷Ä«»ç¡¡±Í¿©¿î ±×¸²Ã¼¿Í Æ÷±ÙÇÑ ÀÛÇ³À¸·Î Á¤ÆòÀÌ ÀÖ´Ù.";
static char unk_4742C0[] = "¹ÌÄ«ÁîÅ° ¾ÆÅ°¶ó£¡¡¡¹Ì¼Ò³à °ÔÀÓ Ä³¸¯ÅÍ £Ã£Ç°¡ Æ¯±âÀÎ £Ã£ÇÀÛ°¡.";
static char unk_474304[] = "Áø½Å¡¡µ¶Æ¯ÇÑ Ä³¸¯ÅÍ¿Í ¼¾½º·Î »Ñ¸® ±íÀº ÀÎ±â¸¦ ÀÚ¶ûÇÏ´Â £Ã£ÇÀÛ°¡.";
static char unk_474344[] = "»çÅ° Ä«¿À¸®¡¡£Ã£ÇÄÚ¹ÍÀÇ °³Ã´ÀÚ °°Àº Á¸Àç. ÀÌ¾ß±â ±¸¼º¿¡ Á¤Æò.";
static char unk_47437C[] = "¾ÆÅ°¿ä½Ã ¿ä½Ã¾ÆÅ°¡¡¿äÁò °¨¼ºÀÇ È£°¨Çü ±×¸²Ã¼¿¡, µ¶ÇÑ ¸À ½ºÅä¸®¸¦ ¼ûÀº ¾ç³äÀ¸·Î.";
static char unk_4743C8[] = "°ÕÃ÷ ¹ÌÄ«¹Ì¡¡±Í¿©¿î ±×¸²Ã¼¿Í Æ÷±ÙÇÑ ÀÛÇ³À¸·Î ¸¶À½ÀÌ Ç®¸°´Ù.";
static char unk_474404[] = "Ã÷·ç±â ¾ß½ºÀ¯Å°¡¡±Í¿©¿î ±×¸²Ã¼¿Í Æ÷±ÙÇÑ ÀÛÇ³ÀÌ ¿äÁò ´À³¦.";
static char unk_47443C[] = "¾Æ¸®¸¶ ÄÉÀÌÅ¸·Î¡¡¶Ù¾î³­ ÀÌ¾ß±â ±¸¼º°ú ³ÑÄ¡´Â È°·ÂÀÌ ºú¾î³»´Â ¸À.";
static char unk_474478[] = "¾Æ¶óÀÌ Ä«Áî»çÅ°¡¡°ÔÀÓ ¿øÈ­·Î ´ëºê·¹ÀÌÅ©. ¸¸È­¿¡¼­´Â ½´¸£ÇÑ ¸Àµµ ³½´Ù.";
static char unk_4744B8[] = "¾ÆÀÌÀÚ¿Í È÷·Î½Ã¡¡º»ÀÎµµ Áï¸ÅÈ¸¸¦ ¿î¿µ. È°·ÂÀÌ ³ÑÄ¡´Â ÀÎ±â ¿©¼º µ¿ÀÎ ÀÛ°¡.";
static char unk_474504[] = "·ù³ë½ºÄÉ£¦Â÷³ë¹Ìº¸¿ìÁî¡¡ÇÏÀÌÅÙ¼Ç °³±×·Î ÆøÁÖÇÏ´Â ³²¸Å À¯´Ö.";
static char unk_474548[] = "³ªµ¥¾Æ¶ó Å¸ÄÉ¿ä½Ã¡¡£Ð£Ï£ÐÄ³¸¯ÅÍ¿Í ¼±ÁøÀû µðÀÚÀÎ ¼¾½ºÀÇ À¶ÇÕ.";
static char unk_474588[] = "¹ÌÁîÅ° Å¸Äí¾ß¡¡¼Ò³à¸¸È­Ç³ ¿ä¼Ò¸¦ °¡Áø Ä³¸¯ÅÍ°¡ °ÔÀÓ°è¿¡¼­´Â ½Å¼±ÇÏ´Ù.";
static char unk_4745D4[] = "Â¡Â¡¡¡±Í¿©¿î ±×¸²Ã¼¿Í Æ÷±ÙÇÑ ÀÛÇ³ÀÌ µ¶Æ¯ÇÏ´Ù.";
static char unk_47460C[] = "Å¸Ã÷³×ÄÚ¡¡¼¶¼¼ÇÑ ÅÍÄ¡·Î ±×¸° Ä³¸¯ÅÍ°¡ µ¶Æ¯ÇÑ ºÐÀ§±â¸¦ ¸¸µç´Ù.";
static char unk_474658[] = "È£¸£¸ó ·»Áö·Î£¦£Í£Á£Ò£Ã£Ùµ¶¡¡±Í¿©¿î Ä³¸¯ÅÍ¿Í´Â ¾È ¾î¿ï¸®´Â Á÷±¸ ½ÂºÎ°¡ ½Ã¿øÇÏ´Ù.";
static char unk_4746A8[] = "ÀÌ¼Ò¿ìµµ¿ì¹ÙÄí¡¡±Í¿©¿î ±×¸²Ã¼¿Í ´ÙÃ¤·Î¿î ÀÌ¾ß±â ±¸¼ºÀÌ ¸¸µé¾î ³»´Â ÆÄ¿ö.";
static char unk_4746E0[] = "¾ß³ë Å¸Äí¹Ì£¦Ä«°¡¿Í Åä¸ð³ëºÎ¡¡µÑÀÇ ¼­·Î ´Ù¸¥ »öÀÌ À¶ÇÕµÇ¾î, ½ÂÈ­µÈ ÀÎ±â À¯´Ö.";
static char unk_474728[] = "¾ÆÅ°¹Ù ³ª±â¡¡ÀÐÈ÷´Â ½ºÅä¸®°¡ È£Æò. ¿ï¸®´Â ÀÌ¾ß±â ¸¸µé±â´Â ÀÏÇ°.";
static char unk_474768[] = "´Ï½Ã¸¶Å¸ ¾Æ¿ÀÀÌ¡¡À¯Çà ¼ÒÀç¸¦ Àý´ë ³õÄ¡Áö ¾Ê´Â ÀÎ±â ¿©¼º µ¿ÀÎ ÀÛ°¡.";
static char unk_474798[] = "Ä­ÀÚÅ° È÷·Î¡¡ÀþÀº ½ÅÀÎ µ¿ÀÎ ÀÛ°¡. ÀÎÅÍ³Ý¿¡¼­µµ ´ëÀÎ±â.";
static char unk_4747D8[] = "³×±âÅ¸¸¶£¦Ã÷³ë´Ù È÷»ç½Ã¡¡±Í¿©¿î Ä³¸¯ÅÍÀÇ ¸Å·ÂÀº ÀÌÁ¦ ÃÊÁ¤¼®.";
static char unk_474814[] = "ÀÌ½º¡¡±Í¿©¿î Ä³¸¯ÅÍ ¼¾½º¿Í´Â ¹Ý´ë·Î ¸¸È­´Â ÀÇ¿Ü·Î ¸Å´Ï¾Ç.";
static char unk_474858[] = "È÷Ä«¿Í ÇìÅ°·ç¡¡Âü½ÅÇÑ °³±×¿Í ¸Å´Ï¾ÇÇÑ ¼ÒÀç·Î ½ÂºÎ.";
static char unk_47488C[] = "ÇòÆ÷ÄÚ±º¡¡±Í¿©¿î ±×¸²Ã¼¿Í Æ÷±ÙÇÑ ÀÛÇ³ÀÌ µ¶ÀÚÀûÀÎ ¼¼°è¸¦ ¸¸µç´Ù.";
static char unk_4748D4[] = "£Ó£È£À£Ò£Ð¡¡½ºÅ¸ÀÏ¸®½ÃÇÑ ¼¾½º°¡ ÀüÆí¿¡ °ÉÃÄ ¹ßÈÖµÈ´Ù.";
static char unk_474910[] = "Å¸¸ð¸® Å¸´ÙÁö¡¡±Í¿±Áö¸¸ »À´ë ÀÖ´Â Ä³¸¯ÅÍ ¹¦»ç°¡ ½Å¼±ÇÏ´Ù.";
static char unk_47494C[] = "q¹Ì·¡´Â Âù¶õÇÔÀ¸·Î °¡µæÇÏ´Ùn";
static char unk_474968[] = "qÀü¼³ÀÇ ¸¸È­°¡ÀÇ º×n";
static char unk_474980[] = "qÀü¼³ÀÇ ¸¸È­°¡ÀÇ ¸ðÀÚn";
static char unk_474998[] = "q°ú°Å´Â È¯»ó, ÁøÁ¤ÇÑ ¹Ì·¡°¡ Çö½ÇÀÌ µÈ´Ùn";
static char unk_4749BC[] = "q½ÅºñÇÑ ÆíÁö°¡ ¿ø°í¸¦ ²ø¾î´ç±ä´Ùn";
static char unk_4749E0[] = "q±Ý¼ÕÀ¸·Î °¡´Â Áö¸§±æÀÌ µÈ´Ùn";
static char unk_4749F4[] = "qÇ¥Áö°¡ ¼ú¼ú ¿Ï¼ºµÈ´Ùn";
static char unk_474A10[] = "qÁÁ¾ÆÇÏ´Â ¾ÆÀÌ¿¡°Ô »ç¶û¹Þ´Â ³²ÀÚ°¡ µÉ ¼ö ÀÖ´Ùn";
static char unk_474A30[] = "qÆæÀÌ ´õ Àß ³ª°£´Ùn";
static char unk_474A48[] = "qÁ¶±Ý ´õ Áñ°Å¿öÁø´Ùn";
static char unk_474A5C[] = "qÃ¼·ÂÀÌ È¸º¹µÈ´Ùn";
static char unk_474A70[] = "£Ó£Ó";
static char unk_474A78[] = "£Ó";
static char unk_474A7C[] = "£Á";
static char unk_474A80[] = "£Â";
static char unk_474A84[] = "£Ã";
static char unk_474A88[] = "½ÃÁîÄí";
static char unk_474A8C[] = "¸®ÇÁ ÆÄÀÌÆ®";
static char unk_474A9C[] = "Å°Áî¾ÆÅä";
static char unk_474AA0[] = "¸®ÇÁ ½ºÅÂÇÁ";
static char unk_474AB0[] = "£Ô£ï¡¡£È£å£á£ò£ô";
static char unk_474AC4[] = "³ª°¡¼¼";
static char unk_474ACC[] = "£×£è£é£ô£å¡¡£Á£ì£â£õ£í";
static char unk_474AE4[] = "°³±×";
static char unk_474AEC[] = "£Ó£Æ";
static char unk_474AF4[] = "¿ÀÄÃÆ®";
static char unk_474B00[] = "¿¬¾Ö";
static char unk_474B08[] = "¿ª»ç";
static char unk_474B10[] = "½ºÆ÷Ã÷ ±Ù¼º";
static char unk_474B1C[] = "¹Ì¼Ò³à °ÔÀÓ";
static char unk_474B28[] = "³ëº§";
static char unk_474B30[] = "½Ã¹Ä·¹ÀÌ¼Ç";
static char unk_474B44[] = "À°¼º";
static char unk_474B4C[] = "·ÑÇÃ·¹À×";
static char unk_474B60[] = "¾îµåº¥Ã³";
static char unk_474B70[] = "´ëÀü °ÝÅõ";
static char unk_474B7C[] = "½ºÆ÷Ã÷";
static char unk_474B88[] = "Ãß¸®";
static char unk_474B90[] = "ÄÚ¹Ìµð";
static char unk_474B9C[] = "¾×¼Ç";
static char unk_474BA8[] = "È÷¾î·Î";
static char unk_474BB4[] = "¸¶¹ý¼Ò³à";
static char unk_474BC0[] = "ÆÇÅ¸Áö";
static char unk_474BD0[] = "·Îº¿";
static char unk_474BDC[] = "¸®ÇÁ";
static int size_473A80 = 0x1164;
static char* strings_473A80[] = {
    unk_473A80, unk_473ACC, unk_473B18, unk_473B64, unk_473BB0, unk_473C00, unk_473C4C, unk_473C98,
    unk_473CE8, unk_473D2C, unk_473D6C, unk_473DB0, unk_473DF0, unk_473E24, unk_473E6C, unk_473EB0,
    unk_473EE4, unk_473F20, unk_473F64, unk_473FA0, unk_473FE4, unk_47401C, unk_47406C, unk_4740A4,
    unk_4740E0, unk_47411C, unk_474150, unk_474188, unk_4741C0, unk_4741F8, unk_474234, unk_47427C,
    unk_4742C0, unk_474304, unk_474344, unk_47437C, unk_4743C8, unk_474404, unk_47443C, unk_474478,
    unk_4744B8, unk_474504, unk_474548, unk_474588, unk_4745D4, unk_47460C, unk_474658, unk_4746A8,
    unk_4746E0, unk_474728, unk_474768, unk_474798, unk_4747D8, unk_474814, unk_474858, unk_47488C,
    unk_4748D4, unk_474910, unk_47494C, unk_474968, unk_474980, unk_474998, unk_4749BC, unk_4749E0,
    unk_4749F4, unk_474A10, unk_474A30, unk_474A48, unk_474A5C, unk_474A70, unk_474A78, unk_474A7C,
    unk_474A80, unk_474A84, unk_474A88, unk_474A8C, unk_474A9C, unk_474AA0, unk_474AB0, unk_474AC4,
    unk_474ACC, unk_474AE4, unk_474AEC, unk_474AF4, unk_474B00, unk_474B08, unk_474B10, unk_474B1C,
    unk_474B28, unk_474B30, unk_474B44, unk_474B4C, unk_474B60, unk_474B70, unk_474B7C, unk_474B88,
    unk_474B90, unk_474B9C, unk_474BA8, unk_474BB4, unk_474BC0, unk_474BD0, unk_474BDC
};
static char* def_46F714[] = {
    unk_46C254, unk_46C24C, unk_46C244, unk_474BDC, unk_474BD0, unk_474BC0, unk_474BB4, unk_474BA8,
    unk_474B9C, unk_474B90, unk_474B88, unk_474B7C, unk_474B70, unk_474B60, unk_474B4C, unk_474B44,
    unk_474B30, unk_474B28, unk_474B9C, unk_474B1C, unk_474B10, unk_474B08, unk_474B00, unk_474AF4,
    unk_474AEC, unk_474B9C, unk_474BC0, unk_474AE4, unk_474ACC, unk_474AC4, unk_474AB0, unk_474AA0,
    unk_474A9C, unk_474A8C, unk_474A88, unk_46CB4C
};
static char* def_471744[] = { unk_474A84, unk_474A80, unk_474A7C, unk_474A78, unk_474A70 };
static char* def_473438[] = {
    unk_474A5C, unk_474A48, unk_474A30, unk_474A10, unk_4749F4, unk_4749E0, unk_4749BC, unk_474998,
    unk_474980, unk_474968, unk_47494C, unk_474910, unk_4748D4, unk_47488C, unk_474858, unk_474814,
    unk_4747D8, unk_474798, unk_474768, unk_474728, unk_4746E0, unk_4746A8, unk_474658, unk_47460C,
    unk_4745D4, unk_474588, unk_474548, unk_474504, unk_4744B8, unk_474478, unk_47443C, unk_474404,
    unk_4743C8, unk_47437C, unk_474344, unk_474304, unk_4742C0, unk_47427C, unk_474234, unk_4741F8,
    unk_4741C0, unk_474188, unk_474150, unk_47411C, unk_4740E0, unk_4740A4, unk_47406C, unk_47401C,
    unk_473FE4, unk_473FA0, unk_473F64, unk_473F20, unk_473EE4, unk_473EB0, unk_473E6C, unk_473E24,
    unk_473DF0, unk_473DB0, unk_473D6C, unk_473D2C, unk_473CE8, unk_473C98, unk_473C4C, unk_473C00,
    unk_473BB0, unk_473B64, unk_473B18, unk_473ACC, unk_473A80
};


static char unk_475074[] = "¿À´ÃÀÇ Çàµ¿À» ¼±ÅÃÇØ ÁÖ¼¼¿ä";
static int size_475074 = 0x20;
static char* strings_475074[] = { unk_475074 };
static char* def_447E84[] = { unk_475074 };


static char unk_475630[] = "ÆòÀÏÀÇ ÀÏÁ¤À» ÀÔ·ÂÇØ ÁÖ¼¼¿ä";
static int size_475630 = 0x20;
static char* strings_475630[] = { unk_475630 };
static char* def_44EB38[] = { unk_475630 };


static char unk_47515C[] = "¾ÆÄ§";
static char unk_475160[] = "ÀüÃ¶ °æÀû¡¤±Þºê·¹ÀÌÅ© ¼Ò¸®";
static char unk_475178[] = "ÀüÃ¶ °æÀû";
static char unk_475184[] = "¹Ù´Ù ¼Ò¸®";
static char unk_47518C[] = "¾Ë¶÷½Ã°è";
static char unk_475198[] = "¹Ú¼ö";
static char unk_4751A0[] = "Àü¿ø Â÷´Ü";
static char unk_4751AC[] = "ÀüÈ­°¡ ¿Â´Ù";
static char unk_4751BC[] = "Àû¸·ÀÌ Èå¸¥´Ù";
static char unk_4751CC[] = "ÀüÈ­¸¦ °Ç´Ù";
static char unk_4751DC[] = "ÀüÈ­°¡ ²÷°å´Ù";
static char unk_4751EC[] = "»À°¡ ¿ìµÎµÏ";
static char unk_4751F8[] = "Áö¸éÀÌ ¿ï¸°´Ù";
static char unk_475200[] = "»þ¿ö ¼Ò¸®";
static char unk_475210[] = "ºÎ¾ý ¿ä¸®";
static char unk_47521C[] = "ºÎ¾ý ¼³°ÅÁö";
static char unk_47522C[] = "»ç¸Á";
static char unk_475234[] = "ÆÎÆÄ·¹";
static char unk_475244[] = "½ºÀ§Ä¡ ¼Ò¸®";
static char unk_475250[] = "¹è¿¡¼­ ²¿¸£¸¤";
static char unk_475260[] = "µÚÀû°Å¸°´Ù";
static char unk_47526C[] = "Ã¥ÀåÀ» ³Ñ±ä´Ù";
static char unk_475278[] = "¿ø°í¸¦ ±×¸°´Ù";
static char unk_475284[] = "ÃÊÀÎÁ¾";
static char unk_47528C[] = "»´¶§¸®±â";
static char unk_475294[] = "¹®À» ¼¼°Ô µÎµå¸°´Ù";
static char unk_4752A4[] = "°¡º­¿î ¹°°ÇÀ» ¶³¾î¶ß¸°´Ù";
static char unk_4752B8[] = "ºÎµúÈù´Ù";
static char unk_4752C4[] = "¶°¹Î´Ù";
static char unk_4752D0[] = "¹°°ÇÀ» ¶³¾î¶ß¸°´Ù";
static char unk_4752DC[] = "½É¸®¹¦»ç/Ãæ°Ý";
static char unk_4752EC[] = "½É¸®¹¦»ç/¼îÅ©";
static char unk_475300[] = "°ü³» È£Ãâ";
static char unk_475310[] = "´Þ¸®´Â ¼Ò¸®";
static char unk_475318[] = "¶§¸®´Â ¼Ò¸®£³";
static char unk_475324[] = "¶§¸®´Â ¼Ò¸®£²";
static char unk_475330[] = "¶§¸®´Â ¼Ò¸®£±";
static char unk_47533C[] = "¼öÈ­±â¸¦ ³»·Á³õ´Â´Ù";
static char unk_47534C[] = "¼öÈ­±â¸¦ µç´Ù";
static char unk_47535C[] = "¹®À» ´Ý´Â´Ù";
static char unk_47536C[] = "¹®À» ¿¬´Ù";
static char unk_47537C[] = "ÀüÀÚÀ½(ÈÞ´ëÆù ¹öÆ°, PC ¼Ò¸®)";
static char unk_4753A0[] = "ÀüÃ¶ÀÌ °ð Ãâ¹ßÇÕ´Ï´Ù";
static char unk_4753BC[] = "ÈÞ´ëÀüÈ­°¡ ¿ï¸°´Ù";
static char unk_4753CC[] = "´Ù¹æ¿¡ µé¾î°£´Ù";
static char unk_4753DC[] = "ºñ";
static char unk_4753E0[] = "msk";
static char unk_4753E4[] = " ¾¾";
static char unk_4753EC[] = " ±º";
static char unk_4753F4[] = "´Ô¾Æ";
static char unk_4753FC[] = "CN";
static char unk_475400[] = "Ä¡";
static char unk_475404[] = " ¿Àºü";
static char unk_475410[] = "¾Æ¾ß Â¯";
static char unk_47541C[] = "Ä¡»ç Â¯";
static char unk_475428[] = "Ä¡-Â¯";
static int size_47515C = 0x2D8;
static char* strings_47515C[] = {
    unk_47515C, unk_475160, unk_475178, unk_475184, unk_47518C, unk_475198, unk_4751A0, unk_4751AC,
    unk_4751BC, unk_4751CC, unk_4751DC, unk_4751EC, unk_4751F8, unk_475200, unk_475210, unk_47521C,
    unk_47522C, unk_475234, unk_475244, unk_475250, unk_475260, unk_47526C, unk_475278, unk_475284,
    unk_47528C, unk_475294, unk_4752A4, unk_4752B8, unk_4752C4, unk_4752D0, unk_4752DC, unk_4752EC,
    unk_475300, unk_475310, unk_475318, unk_475324, unk_475330, unk_47533C, unk_47534C, unk_47535C,
    unk_47536C, unk_47537C, unk_4753A0, unk_4753BC, unk_4753CC, unk_4753DC, unk_4753E0, unk_4753E4,
    unk_4753EC, unk_4753F4, unk_4753FC, unk_475400, unk_475404, unk_475410, unk_47541C, unk_475428
};
static char* def_4750A4[] = {
    unk_4753DC, unk_4753CC, unk_4753BC, unk_4753A0, unk_47537C, unk_47536C, unk_47535C, unk_47534C,
    unk_47533C, unk_475330, unk_475324, unk_475318, unk_475310, unk_475300, unk_4752EC, unk_4752DC,
    unk_4752D0, unk_4752C4, unk_4752B8, unk_4752A4, unk_475294, unk_47528C, unk_475284, unk_475278,
    unk_47526C, unk_475260, unk_475250, unk_475244, unk_475234, unk_47522C, unk_47521C, unk_475210,
    unk_475200, unk_4751F8, unk_4751EC, unk_4751DC, unk_4751CC, unk_4751BC, unk_4751AC, unk_4751A0,
    unk_475198, unk_47518C, unk_475184, unk_475178, unk_475160, unk_47515C
};
static char* def_448461[] = { unk_4753E0 };
static char* def_448DBE[] = { unk_4753E4 };
static char* def_448D19[] = { unk_4753EC };
static char* def_448D6D[] = { unk_4753EC };
static char* def_448CC5[] = { unk_4753F4 };
static char* def_448C45[] = { unk_4753FC };
static char* def_448BA9[] = { unk_475400 };
static char* def_448B77[] = { unk_475404 };
static char* def_448B60[] = { unk_475410 };
static char* def_448B3F[] = { unk_47541C };
static char* def_448B27[] = { unk_475428 };




static char unk_46F7A4[] = "°­Ã¶ÀÇ Áß±âº´";
static char unk_46F7BB[] = "ÃÊ! ³¡³»ÁÖ´Â ·Îº¿";
static char unk_46F7D2[] = "¿ë±â¾ß¸»·Î ÆÄ¿ö";
static char unk_46F7E9[] = "¿­¹Ý°Ô¸®¿Â";
static char unk_46F800[] = "³Ê¾ß¸»·Î ¿ë»ç´Ù!";
static char unk_46F817[] = "´ëµµ! ¸¶¹ý ·Îº¿ÀÇ ºñ¹Ð";
static char unk_46F82E[] = "ÄçÄç £Ë£É£Î£Ç";
static char unk_46F845[] = "¾ÆÅ°ÇÏ¹Ù¶ó ÀÎÅÍ³Ý´Ü";
static char unk_46F85C[] = "ÃÊ¼ÒÇü ·Îº¿ °¡¸£³ª³í";
static char unk_46F873[] = "¿ìÁÖÀüÇÔ ¾Æ½ºÄ«";
static char unk_46F88A[] = "¿ÀÆÛ·¹ÀÌÅÍ ´öÈÄ";
static char unk_46F8A1[] = "³¶ÀÎ¿¡°Ô ¹ÙÄ¡´Â ¹ß¶óµå";
static char unk_46F8B8[] = "µ¿ÂÊ ¿ë»ç¿Í ¼­ÂÊ ¸¶¿Õ";
static char unk_46F8CF[] = "¾Æ¼­ÀÇ °Ë";
static char unk_46F8E6[] = "µå·¡°ï ÇÐ¿ø";
static char unk_46F8FD[] = "¸¶¾²ÀÚÄ«¿ë Å°·Î 2000¿£";
static char unk_46F914[] = "Áü½Â °øÁÖ";
static char unk_46F92B[] = "¿¤ÇÁ ±Í Çõ¸í";
static char unk_46F942[] = "È¯»ó ¼º¾ß";
static char unk_46F959[] = "¸¶¿°ÀÇ ¿Õ";
static char unk_46F970[] = "ÇØÇÇ Äù½ºÆ®";
static char unk_46F987[] = "ÆÈ¶óµò ·Îµå";
static char unk_46F99E[] = "ÈÄ´ÏÈÄ´Ï Æ÷Çª·ç";
static char unk_46F9B5[] = "»ç¶ûÀÇ ¸¶¹ýÀ» Ç®Áö ¸¶";
static char unk_46F9CC[] = "¸¶¹ýÀ¸·Î ¾ÆÀÌµ¹!!";
static char unk_46F9E3[] = "·Î¸®·Î¸®º»";
static char unk_46F9FA[] = "¸¶¹ýÁÖºÎ ¸ùÅ°¸¶¸¶";
static char unk_46FA11[] = "¸¶¹ýÇÐÀÚ ½º¹Ì°¡¿Í À¯¹Ì";
static char unk_46FA28[] = "ÇÇÄ¡! ÇÇ¡ªÄ¡!";
static char unk_46FA3F[] = "ÆäÆÛ¹ÎÆ® ¸ÅÁ÷¡Ù";
static char unk_46FA56[] = "ÇÏ³Ä¢¦¿ùµå";
static char unk_46FA6D[] = "ÃòÅ°ÃòÅ° ÆÄ¿ö";
static char unk_46FA84[] = "·¯ºí¸® ÀÓÆÑÆ®";
static char unk_46FA9B[] = "¾Ç¡¤Áï¡¤Âü!!";
static char unk_46FAB2[] = "ºÒ²ÉÀÇ ½´ÆÛÈ÷¾î·Î";
static char unk_46FAC9[] = "£Ë£Å£Ù £Í£Á£Î";
static char unk_46FAE0[] = "£Ð£Ï£×£Å£Ò£Æ£Õ£Ì£Ì¡Ù";
static char unk_46FAF7[] = "°Å¸¸ÇÑ Çü»ç";
static char unk_46FB0E[] = "£³£í£é£î£é£ô£ó!!";
static char unk_46FB25[] = "ÆÄÆ®Å¸ÀÓ È÷¾î·Î";
static char unk_46FB3C[] = "º¯½Å!";
static char unk_46FB53[] = "³Ê¸¸À» ÁöÅ°°í ½Í¾î";
static char unk_46FB6A[] = "¿©ÀÚ¾Öµµ Èû³»´Â °Å¾ß!";
static char unk_46FB81[] = "Á¤ÀÇÀÇ ºû¡¤¾ÇÀÇ ²É";
static char unk_46FB98[] = "ÇÏ¸®£¦Æø½º";
static char unk_46FBAF[] = "£Í£É£Ó£Ó£É£Ï£Î";
static char unk_46FBC6[] = "º£ÀÌ»çÀÌµå ½ºÅä¸®";
static char unk_46FBDD[] = "£Ç£Ï¡ª£Ç£Ï¡ªÄ¸Æ¾";
static char unk_46FBF4[] = "£Â£É£Ç£Â£Ï£Ó£Ó";
static char unk_46FC0B[] = "Ç®ÇÃ·§";
static char unk_46FC22[] = "¸®¸¶Å· Å³·¯";
static char unk_46FC39[] = "ÀÎºñÀúºí ½ºÀ§ÆÛ";
static char unk_46FC50[] = "£Ä£Ï£Õ£Â£Ô!!";
static char unk_46FC67[] = "±âµ¿Ãµ»ç ¸¶¸®¾Æ";
static char unk_46FC7E[] = "±×³à¿¡°Ô ¸Ã°Ü!!";
static char unk_46FC95[] = "»çÄ«ÀÌ ±º°ú ´Ï½Ã´Ù ¾¾";
static char unk_46FCAC[] = "´ÏÆ÷Æ÷";
static char unk_46FCC3[] = "±×·± µÎ »ç¶÷ÀÇ °ü°è";
static char unk_46FCDA[] = "¿ì¶ó¿ì¶ó ºí·¢";
static char unk_46FCF1[] = "Åä´Ù ¸ñÁ¶°¡Á·£²";
static char unk_46FD08[] = "·¯ºê¡ÙÄÚ¹Ìµð¾ð";
static char unk_46FD1F[] = "½ÃÇè¿¡ ¾È ³ª¿Í!!";
static char unk_46FD36[] = "¿©±ä ÇÐ¿øÀå";
static char unk_46FD4D[] = "ÇÐ»ýÈ¸ÀÇ º°³­ ³à¼®µé";
static char unk_46FD64[] = "¾Æºü¿¡°Ô ÅÐÀÌ ³µ´Ù£¡";
static char unk_46FD7B[] = "³ªÀÇ º¹½½º¹½½´Ô";
static char unk_46FD92[] = "¸íÅ½Á¤Àº ÃÊµîÇÐ»ý?";
static char unk_46FDA9[] = "¸íÅ½Á¤ »ìÀÎ»ç°Ç";
static char unk_46FDC0[] = "µµ³­´çÇÑ Ãß¸®";
static char unk_46FDD7[] = "¹Ì¼Ò³à Å½Á¤ ·ç¹Ì";
static char unk_46FDEE[] = "¹ÌÄ¡³ëÄí ¿ÂÃµ »ìÀÎ»ç°Ç";
static char unk_46FE05[] = "£Î.£Ù ÄÁº¥¼Å³Î";
static char unk_46FE1C[] = "¸ÞÀÌÇÃ ¾ÆÁÜ¸¶ÀÇ Ãß¸®";
static char unk_46FE33[] = "±î³õ°í ¹üÀÎÀº ³ª´Ù£¡";
static char unk_46FE4A[] = "¹üÀÎÀº ÀÌ ¾È¿¡ ÀÖ¾î£¡";
static char unk_46FE61[] = "±èÀüÀÏ ¼Ò³à";
static char unk_46FE78[] = "¼È·Ï 3¼¼";
static char unk_46FE8F[] = "¸¶±¸µµ";
static char unk_46FEA6[] = "Ä®Àº ÇÏ·ç¸¸¿¡ ¾È µÈ´Ù";
static char unk_46FEBD[] = "±Í½Å ÄÚÄ¡ »ç¶ûÀÇ °ÝÁ¤";
static char unk_46FED4[] = "½ºÆ÷Ã÷ £Ó£Ù£Ï¡ª£Î£Å£Î";
static char unk_46FEEB[] = "¿ÞÆÈÀÇ ¸Í¼¼";
static char unk_46FF02[] = "º¸ÀÌÁö ¾Ê´Â °ñ´ë";
static char unk_46FF19[] = "°áÁ¤ÇØ¶ó! ¹ø°³ ½¸";
static char unk_46FF30[] = "ºÒÅë ¾ÆºüÀÇ ½ºÆÄ¸£Å¸µµ";
static char unk_46FF47[] = "³»ÀÏÀ» À§ÇÑ ±× Ã¹ ¹øÂ°";
static char unk_46FF5E[] = "µµ½ºÄÚÀÌ ·»Áö·Î";
static char unk_46FF75[] = "ÁÁ¾ÆÁÁ¾Æ ¸Å´ÏÀú";
static char unk_46FF8C[] = "£Æ£É£Î£É£Ó£È £È£É£Í£¡";
static char unk_46FFA3[] = "À¯¹ß ÄÞº¸ ¸¶´Ï¾Æ";
static char unk_46FFBA[] = "ÆÝÄ¡ ÆÝÄ¡ Å±";
static char unk_46FFD1[] = "ÁÖ¸ÔÀ¸·Î ¸»ÇØ!!";
static char unk_46FFE8[] = "¶ß°Å¿î ÇÇÀÇ ¹°°á";
static char unk_46FFFF[] = "ÄË´Ô Çì¸ðÇì¸ðÃ¥";
static char unk_470016[] = "±× »ç¶÷À» ¸¸³ª°í ½Í¾î";
static char unk_47002D[] = "ºó°ï°ÝÅõ°¡ÀÇ ¼Ö·Î ¿©Çà";
static char unk_470044[] = "½Î¿òÀÇ È¥";
static char unk_47005B[] = "£Ð£Ï£×£Å£Ò£¦£Æ£Ï£Ò£Ã£Å";
static char unk_470072[] = "£Æ£å£ô£é£ó£è £Ç£á£í£å";
static char unk_470089[] = "£²£´½Ã°£ÀÇ ¾Ç¸ù";
static char unk_4700A0[] = "´ÙÀÌ¾ó£Ñ¸¦ µ¹·Á¶ó!!";
static char unk_4700B7[] = "£Ð£Ò£É£Ó£Ï£Î";
static char unk_4700CE[] = "£Á£Ó£Ó£Á£Ó£Ó£É£Î";
static char unk_4700E5[] = "£Ä£Å£Á£Ô£È¡¤£Ú£Ï£Î£Å";
static char unk_4700FC[] = "°ËÀº ¼Ó³» ¸¶À»ÀÇ ±âÀû";
static char unk_470113[] = "£Å£Ó£Ã£Á£Ð£Å!!";
static char unk_47012A[] = "½Ã°£Àº µ¹°í µ¹¾Æ";
static char unk_470141[] = "¸ð³ë¸®½º";
static char unk_470158[] = "µÑÀÌ¼­ ´ë¸ðÇè";
static char unk_47016F[] = "Ãµ°øµµ½Ã";
static char unk_470186[] = "¹Ù±×³ÊÀÇ ±ÍÈ¯";
static char unk_47019D[] = "È²±Ý °Ë°ú Àº ¹æÆÐ";
static char unk_4701B4[] = "¿À¿À ¿ë»ç¿©!!";
static char unk_4701CB[] = "»þ¸Õ ½ºÅä¸®";
static char unk_4701E2[] = "Æ®·¹Àú£Â£Ï£Ø";
static char unk_4701F9[] = "°Ý·Ä ·¯ºê µå·¡°ïÃ¥";
static char unk_470210[] = "¸ðÁö¸ðÁö Äù½ºÆ®";
static char unk_470227[] = "´ëµµµÏ Æ®·¦";
static char unk_47023E[] = "ÄÚ°¼·ç Äù½ºÆ®";
static char unk_470255[] = "£Æ£á£î£ô£á£ó£ô£é£ã";
static char unk_47026C[] = "¼º½º·¯¿î ¿ë»ç";
static char unk_470283[] = "¾Æ¹öÁö²²¡¦";
static char unk_47029A[] = "£±£°£°¸¸ÀÇ µþµé¿¡°Ô¡¦";
static char unk_4702B1[] = "´©±¸¿¡°Ôµµ ÁöÁö ¾Ê¾Æ£¡";
static char unk_4702C8[] = "Æä¾î¸® £Í£Á£Ë£Å£Ò";
static char unk_4702DF[] = "¿À´Ãµµ ½ß½ßÇÏ³×!";
static char unk_4702F6[] = "¸¶ÃÊ¿¡ ¹«ÃÊ";
static char unk_47030D[] = "¸¶ÀÌ¡ÙÆ¼Ã³";
static char unk_470324[] = "»þÀÌ´× ·Îµå";
static char unk_47033B[] = "È¥³»ÁÙ °Å¾ß!";
static char unk_470352[] = "£Í£Á£Ó£Ô£Å£Ò ·Îµå";
static char unk_470369[] = "Ä¿½ºÅÍ¸¶ÀÌÁî¡¤°É";
static char unk_470380[] = "£Ä£õ£å£ì£Ô£á£ã£ô£é£ã£ó";
static char unk_470397[] = "±º»ç´Â ÀÌ·¸°Ô ¸»Çß´Ù";
static char unk_4703AE[] = "º´»ç ÇÑ ¹«´õ±â 100¿£";
static char unk_4703C5[] = "ÆÐ¹Ð¸® ·¹½ºÅä¶û £Ç£Ï£¡";
static char unk_4703DC[] = "¿Õ±¹ÀÇ °á´Ü";
static char unk_4703F3[] = "ÃÊ Á¦·Î Àü±â";
static char unk_47040A[] = "ÀºÇÏ ÇãÁ¢ ¿­Àü";
static char unk_470421[] = "ÃÊº¸ ½ÃÀå ºÐÅõ±â";
static char unk_470438[] = "»ç¸·ÀÇ Áã";
static char unk_47044F[] = "£Ï£ó£á£ë£á £×£á£ò£ó";
static char unk_470466[] = "³ª´Â £Ç£Ï£Ä!";
static char unk_47047D[] = "Çª¸¥ ´ÞÀÇ ¹ã¿¡´Â";
static char unk_470494[] = "°¢¼ºÇÏ´Â °Í";
static char unk_4704AB[] = "½É¿¬";
static char unk_4704C2[] = "»ç½ÅÀÇ ¹ã";
static char unk_4704D9[] = "´©¿¡°¡ ¿î´Ù";
static char unk_4704F0[] = "¹«Áö°³ ¾ð´ö";
static char unk_470507[] = "¾ðÁ¨°¡ ¾îµò°¡¿¡¼­";
static char unk_47051E[] = "³ë½ºÅÅÁö¾î";
static char unk_470535[] = "¹Ù½Ç¸®½ºÅ©";
static char unk_47054C[] = "Èð¾îÁö´Â µµ½Ã";
static char unk_470563[] = "£Î£ï£é£ó£å";
static char unk_47057A[] = "¹ö¼¸ ÇüÁ¦";
static char unk_470591[] = "¿ø´õ °í°í!";
static char unk_4705A8[] = "Àü»ç¿¡°Ô ²É´Ù¹ßÀ»";
static char unk_4705BF[] = "£Å£Í£Ð£É£Ò£Å";
static char unk_4705D6[] = "Ã­ÇÇ";
static char unk_4705ED[] = "´Þ·Á¶ó ½º³×ÀÌÅ©";
static char unk_470604[] = "£Ê£Õ£Í£Ð!!";
static char unk_47061B[] = "£Ì£á£ó£ô £Æ£é£ç£è£ô";
static char unk_470632[] = "£Ó£Ï£Ì£Ä£É£Å£Ò£Ó";
static char unk_470649[] = "£Ç£Õ£Î ¸¶´Ï¾Æ";
static char unk_470660[] = "£Ä£Á£Î£Ç£Å£Ò £Í£Å£Î";
static char unk_470677[] = "·¯ºê·¯ºê¡¤Æ÷¿¡¹ö";
static char unk_47068E[] = "¾È°æ Àü¼³";
static char unk_4706A5[] = "¹æ°ú ÈÄ ÆÄ¶ó´ÙÀÌ½º";
static char unk_4706BC[] = "³ª¿Í ÇÔ²²¡Ù";
static char unk_4706D3[] = "È¥³»ÁÙ °Å¾ß!";
static char unk_4706EA[] = "£Ç£á£ì£ó£Ð£ò£ï£ê£å£ã£ô";
static char unk_470701[] = "¿¬¾Ö Àü¶õ";
static char unk_470718[] = "¹«Áö°³ ³Ê¸Ó";
static char unk_47072F[] = "¸ÞÀÌµå £é£î £Ê£á£ð£á£î";
static char unk_470746[] = "Ä³·µ ÄÉÀÌÅ©";
static char unk_47075D[] = "ºóÀ¯ £È£Ï£Õ£Ó£Å";
static char unk_470774[] = "¿ïº¸ ½ºÆ®¶óÀÌÄ¿";
static char unk_47078B[] = "¼øÁ¤ ½½·¯°Å";
static char unk_4707A2[] = "Ãà±¸ ¼Ò³à";
static char unk_4707B9[] = "¹é±¸¸¦ ÃÄ¶ó";
static char unk_4707D0[] = "ÇÏÆ®·Î £Ã£Á£Ô£Ã£È";
static char unk_4707E7[] = "½º¸Å½Ã!!";
static char unk_4707FE[] = "¿ªÁÖÇà ÀÌ·¹ºì";
static char unk_470815[] = "ÃÊ°í¼Ó ½ºÇÁ¸°ÅÍ";
static char unk_47082C[] = "¹Ì½ºÅÍ¡¤¹öÅÍÇÃ¶óÀÌ";
static char unk_470843[] = "ÅÍÇÁ¸¦ ³»´Þ·Á¶ó!!";
static char unk_47085A[] = "Å¸ÀÌ ºê·¹ÀÌÅ©";
static char unk_470871[] = "¼¼Å°°¡ÇÏ¶ó ¿µ¿õÀü";
static char unk_470888[] = "¼ºÀ» ½×Àº ³²ÀÚ";
static char unk_47089F[] = "½Å¼±Á¶ ÀÌ¹®";
static char unk_4708B6[] = "ÃòÅ°ÃòÅ° È÷ÁöÄ«Å¸´Ô";
static char unk_4708CD[] = "Á¤¼º°ø ÀÌ¾ß±â";
static char unk_4708E4[] = "¿À¿¡µµ ´Ñ¹ý ¹«¿¹Ã¸";
static char unk_4708FB[] = "ÃãÃß´Â ´ë ¿¡µµ ¼ö»ç¼±";
static char unk_470912[] = "¼ÒÀÎ°ú ³ëºÎ³ª°¡";
static char unk_470929[] = "°Ý¡¤»ï±¹È­°Ý´Ü";
static char unk_470940[] = "´Á´ëµéÀÇ Àü¼³";
static char unk_470957[] = "¿ëÀÇ ±º´Ü";
static char unk_47096E[] = "ÇÑ¹ø ´õ £Ë£É£Ó£Ó¸¦";
static char unk_470985[] = "¼­Å÷ ³×°¡ ÁÁ¾Æ";
static char unk_47099C[] = "¾ðÁ¦³ª ²À ¾È¾ÆÁà";
static char unk_4709B3[] = "ÇÏ´Ãºû ÆÇÅ¸Áö";
static char unk_4709CA[] = "£È£á£ð£ð£ù £Ì£é£æ£å";
static char unk_4709E1[] = "µÎ ¹øÂ° ´ÜÃß";
static char unk_4709F8[] = "£Ì£ï£ö£å£ò£ó£Á£ç£á£é£î";
static char unk_470A0F[] = "±×³à¿¡°Ô £±£²£°£¥";
static char unk_470A26[] = "¶ó½ºÆ® ³ªÀÌÆ® ¸Þ¸ð¸®";
static char unk_470A3D[] = "¿©°í»ý µÎ±ÙµÎ±Ù ³ªÀÌÆ®";
static char unk_470A54[] = "³ª¸¸ÀÇ Æ÷ÄÏ";
static char unk_470A6B[] = "°ËÀº Å¹·ù";
static char unk_470A82[] = "¿¢¼Ò½Ã½ºÆ®´Ü";
static char unk_470A99[] = "£Ä£Å£Á£Ô£È¡¡£Í£Á£Ó£Ë";
static char unk_470AB0[] = "°ËÀº ¹ìÀÇ °ü";
static char unk_470AC7[] = "¿ì´Â ¾Æ±â";
static char unk_470ADE[] = "£Î£É£Ç£È£Ô£Í£Á£Ò£Å";
static char unk_470AF5[] = "±«±â Àü½Â";
static char unk_470B0C[] = "´ÙÅ© »çÀÌÆ®";
static char unk_470B23[] = "À½¾ç»ç";
static char unk_470B3A[] = "Ã¥Àå¿¡ ´Ã¾î¼± ±â¾ï";
static char unk_470B51[] = "£Ð£ó£ù£ã£è£ï£Â£ò£á£é£î";
static char unk_470B68[] = "¿ìÁÖ±Ø ½ºÆäÀÌ½º ¿ÀÆä¶ó";
static char unk_470B7F[] = "ÀºÇÏ ¼±Àå";
static char unk_470B96[] = "Â÷¿øÀÇ Æ´»õ¿¡ £¶¼¾Æ®";
static char unk_470BAD[] = "¾È³ç ±Ý¼ºÀÎ ¾¾¡Ù";
static char unk_470BC4[] = "È­¼ºÀÎÀº º¸¾Ò´Ù";
static char unk_470BDB[] = "¸í¿Õ¼º »ç´Ü";
static char unk_470BF2[] = "¾ÈÅ¸·¹½ºÀÇ ¹«¹ýÀÚ";
static char unk_470C09[] = "ÀÎ°ø ÁøÈ­ÀÇ µµÀü";
static char unk_470C20[] = "°¥»ö Çý¼ºÀÇ Áö¹èÀÚ";
static char unk_470C37[] = "ÀºÇÏ°èÀÇ £·´ë ¼ö¼ö²²³¢";
static char unk_470C4E[] = "½ÅµéÀÇ È²È¥";
static char unk_470C65[] = "´õ¡¤Æ®·¹Àú ÇåÅÍ";
static char unk_470C7C[] = "ºÁ¶ó ¸¶! ¹øÀåÁ¶";
static char unk_470C93[] = "½ºÆÄÀÌ ´ëÀÛÀü";
static char unk_470CAA[] = "±â¶ó ½½·¡½Ã";
static char unk_470CC1[] = "ºí·¢¾Æ¿ô";
static char unk_470CD8[] = "´Ù¿îÅ¸¿î È÷¾î·ÎÁî";
static char unk_470CEF[] = "£Â£Õ£Ó£Ô£Å£Ò¡Ù¹Ì³ª·ç";
static char unk_470D06[] = "È÷Ä«·çÀÇ ÁÖ¸Ô";
static char unk_470D1D[] = "¹ÙÀÌ¿Ã·±½º £²£°£°£Ø";
static char unk_470D34[] = "¿¡ÀÌÀüÆ® £Á£Ë£É£Ò£Á";
static char unk_470D4B[] = "´õ¡¤¼­¹ÙÀÌ¹ö";
static char unk_470D62[] = "¸¶¹ý»ç ÆÄ¾ßÆÄ¾ß";
static char unk_470D79[] = "¼º·ÉÀÇ °è°î";
static char unk_470D90[] = "¸¶ÃµÈ¸¶û";
static char unk_470DA7[] = "ÀÌ»óÇÑ ³ª¶ó ÀÌ¾ß±â";
static char unk_470DBE[] = "¾Ç¸¶±â»çÀÇ ºÎ¸§";
static char unk_470DD5[] = "°ËÀÇ Ãã";
static char unk_470DEC[] = "ÁÖ¹® ´ëÀü";
static char unk_470E03[] = "£Ó£ð£å£ì£ì £Ó£÷£ï£ò£ä";
static char unk_470E1A[] = "½Ã±Þ £¸£µ£°¿£ È÷·ÎÀÎ";
static char unk_470E31[] = "´ë¸¶µµ»ç ½ºÅÚ¶ó";
static char unk_470E48[] = "²¿¸¶¿ë»çÀÇ ´ë¸¶¿Õ Åä¹ú";
static char unk_470E5F[] = "ÅÝÄ¡¸® ±º";
static char unk_470E76[] = "¸»½é²Ù·¯±â ¾î¸°ÀÌµé";
static char unk_470E8D[] = "µµµ¥½ºÄ« µ§½ºÄÉ";
static char unk_470EA4[] = "»Ç¿ë";
static char unk_470EBB[] = "¿øÃò!!!!!!";
static char unk_470ED2[] = "ÃÌ½º·± ±¹¿Õ";
static char unk_470EE9[] = "ÄÚ½Ã³ªµ¥ ÄÃ·º¼Ç";
static char unk_470F00[] = "³» ÀÌ¸§Àº µµ»çºÎ·Î";
static char unk_470F17[] = "¿ì´çÅÁ ±×¶ûÇÁ¸®";
static char unk_470F2E[] = "ÁÁ¾ÆÁÁ¾Æ¸Ó½Å ¸ð¸®Å¸ ±º";
static char unk_470F45[] = "¹Ýµí¹Ýµí ¿¡µµ¶¯ ±º";
static char unk_470F5C[] = "À¯Å° £Æ£ï£ò£å£ö£å£ò";
static char unk_470F73[] = "·¯ºê·¯ºê¡Ù¸®³ªÂ¯";
static char unk_470F8A[] = "¸¶³ª¡¤¸¶³ª?";
static char unk_470FA1[] = "¾ÆÀÌµ¹ÀÌ¶óµµ Èûµé¾î";
static char unk_470FB8[] = "¿ô¾îÁà¿ä ¾ß¿äÀÌ ¾¾";
static char unk_470FCF[] = "»ó³ÉÇÏ°í ¾ÖÆ¶ÇÑ";
static char unk_470FE6[] = "£×£é£î£ô£å£ò£Ô£á£ì£å£ó";
static char unk_470FFD[] = "¹Ì»çÅ° ¼±¹èÀÇ ¸ð¿¡¸ð¿¡";
static char unk_471014[] = "¿¬¿¹°è £Ì£ï£ö£å£ò£ó";
static char unk_47102B[] = "¿À°¡Å¸ °¡ÀÇ »ç¶÷µé";
static char unk_471042[] = "°Ü¿ïÀÇ ¸Þ¸ð¸®¾ó";
static char unk_471059[] = "¼¼¹Ù½ºÂùÀÇ ´ë¸ðÇè";
static char unk_471070[] = "¸ÀÀÖ´Â Ä¿ÇÇ ³»¸®´Â ¹ý";
static char unk_471087[] = "¼¼¹Ù½ºÂù £±£¹£´£µ";
static char unk_47109E[] = "Çü»ç ³ª°¡¼¼ »ç°Ç¼öÃ¸";
static char unk_4710B5[] = "¸ÖÆ¼¸¦ ¸¸µç ³²ÀÚµé";
static char unk_4710CC[] = "°¡¸£ÃÄÁà ¹Ì½ºÅÍ ³ª°¡¼¼";
static char unk_4710E3[] = "º¯È­¹«½Ö ³ª°¡¼¼";
static char unk_4710FA[] = "°¥!!";
static char unk_471111[] = "³ª°¡¼¼ÀÇ Ç÷Á·";
static char unk_471128[] = "ÁÖÇàµî";
static char unk_47113F[] = "ÀÌ¿ôÁýÀÇ ³ª°¡¼¼ ¾¾";
static char unk_471156[] = "¾ÆÄ«¸µ°ú ÇÔ²²!";
static char unk_47116D[] = "¾È°æ À§¿øÈ¸ ÆÄÆ® £²";
static char unk_471184[] = "ÃÊ´É·Â ¿©°í»ý";
static char unk_47119B[] = "ÃòÅ°ÃòÅ°¡Ù¸ÖÆ¼";
static char unk_4711B2[] = "¿ù°£ ½ÃÈ£ Â¯ ´º½º";
static char unk_4711C9[] = "ÇÑ¹ãÀÇ ºñ¹Ð À¯Èñ";
static char unk_4711E0[] = "Èû³»¶ó ¾Æ¿ÀÀÌ Â¯!";
static char unk_4711F7[] = "Ãß¾ïÀÇ ±× ¾ÆÀÌ";
static char unk_47120E[] = "£Ò£é£ï¡¤£Ò£é£ï¡¤£Ò£é£ï";
static char unk_471225[] = "º½ÀÌ ¿À¸é";
static char unk_47123C[] = "º¢²ÉÀÇ °èÀý";
static char unk_471253[] = "³ëº§¿Õ ´ÙÄ«ÇÏ½Ã";
static char unk_47126A[] = "¹Ì³ªÁîÅ° ¾Æ¹öÁö";
static char unk_471281[] = "¶ó¢¦¡Ù£Ù£Ï£Õ";
static char unk_471298[] = "£Ä¡¤£Ï¡¤£Ú¡¤£Á";
static char unk_4712AF[] = "À½¾ÇÀÌ¶ó¸é ¸Ã°ÜÁà¡Ù";
static char unk_4712C6[] = "Èû³»¶ó ½Ã¸ðÄ«¿Í Àü¹«£¡";
static char unk_4712DD[] = "½Î¿ö¶ó!! ¿ì´Ù¸£¸Ç";
static char unk_4712F4[] = "Ãà¡Ù£Ð£ÓÆÇ";
static char unk_47130B[] = "°£»çÀÌÀÎÀÇ ÆÐ±â´Ù¢¦£¡";
static char unk_471322[] = "¿ï°í ¿ô°í ½Î¿ì°í";
static char unk_471339[] = "¿©·¯ºÐ ´öºÐÀÔ´Ï´Ù";
static char unk_471350[] = "Ä¡Áî·ç ¾¾ÀÇ ½ÄÄ®";
static char unk_471367[] = "Ä«¿¡µ¥¡¤Ä«¿¡µ¥¡¤Ä«¿¡µ¥";
static char unk_47137E[] = "´ÙÅ©»çÀÌµå ÇÏÃ÷³×";
static char unk_471395[] = "ÀÎ¿¬";
static char unk_4713AC[] = "À§¼±ÀÚ";
static char unk_4713C3[] = "¿¤Å©ÀÇ ºÎ¸§";
static char unk_4713DA[] = "³¡¿¡¼­ ½ÃÀÛÀ¸·Î";
static char unk_4713F1[] = "ÇÏÃ÷³× Â¯°ú ºÒ²É³îÀÌ";
static char unk_471408[] = "°¡Ã­ÇÉ°ú ÇÔ²²";
static char unk_47141F[] = "ºÓÀº ´Þ";
static char unk_471436[] = "¹ö¼¸ ÆÐ´Ð!!";
static char unk_47144D[] = "¸®ÇÁ ¿Ã½ºÅ¸Áî";
static char unk_471464[] = "²ÞÀÇ °ø¿¬!!";
static char unk_47147B[] = "°áÁ¤ÇØ¶ó ÇÕÃ¼±â!";
static char unk_471492[] = "±¸Ä³¸¯¿¡ »ç¶ûÀÇ ¼Õ±æÀ»";
static char unk_4714A9[] = "´Ù½Ã ¸¸³ª¼­ ¹Ý°¡¿ü¾î";
static char unk_4714C0[] = "Àö¡Ù³ªÀÌÆ® ¸¶ÀÛ±Í";
static char unk_4714D7[] = "ºû³ª¶ó ÇÊ½º ¼Òµå";
static char unk_4714EE[] = "Ã÷·çÅ°¾ß Àß ºÎÅ¹ÇØ";
static char unk_471505[] = "¹Ù¶÷ÀÌ¿© ¹°ÀÌ¿© ºÒÀÌ¿©";
static char unk_47151C[] = "µ¶ÀüÆÄ Ãß°¡";
static char unk_471533[] = "¸ÖÆ¼µµ Èû³»!!";
static char unk_47154A[] = "»ç¿À¸µ ºÎ·ç¸¶";
static char unk_471561[] = "ÀüÆÄ°¡¢¦¢¦";
static char unk_471578[] = "¾È°æ À§¿øÈ¸ ÆÄÆ® £±";
static char unk_47158F[] = "ÀüÆÄ ´ê¾Ò¾î?";
static char unk_4715A6[] = "µ¶ÀüÆÄ ÀÏ±â";
static char unk_4715BD[] = "Ãß¾ïÀÇ ¿À¸£°ñ";
static char unk_4715D4[] = "Èû³»¶ó Ã÷Å°½Ã¸¶ ³²¸Å";
static char unk_4715EB[] = "¾Æ½ºÆ®¶ö ¹ö½ºÅÍÁî";
static char unk_471602[] = "¿Àºü¿Í ¿©µ¿»ý";
static char unk_471619[] = "£Â£Á£Ä £Ä£Å£Ó£É£Ò£Å";
static char unk_471630[] = "·ç¸®ÄÚ ¾¾¡¦";
static char unk_471647[] = "ÄÚ¹ÌÆÄ¿¡¼­ Æù";
static char unk_47165E[] = "ÃÊ¡ÙºýÄ£ ¿©ÀÚ¾ÆÀÌ";
static char unk_471675[] = "¸ÛÃ»¸ÛÃ» ´©´Ô";
static char unk_47168C[] = "³­ µ¿ÀÎ°èÀÇ ÆÐ¿Õ!";
static char unk_4716A3[] = "´ú··ÀÌ Ä¡»ç Â¯¡Ù";
static char unk_4716BA[] = "¸¶°¨ £Ï£Ë ±âºÐÀº £Î£Ç";
static char unk_4716D1[] = "¸¸È­ °°Àº »ç¶ûµµ £Ï£Ë";
static char unk_4716E8[] = "£Ä£Å£Å£ÐÀ¸·Î °¡ÀÚ";
static char unk_4716FF[] = "³ªÀÇ »öÀ¸·Î ¹°µé¾î!";
static char unk_471716[] = "¸¶´Ï¾ÆÀÇ ±æ";
static char unk_47172D[] = "°õÀÌ ÆÎÆÎÇØ?";
static int size_46F7A4 = 0x1FA0;
static char* strings_46F7A4[] = {
    unk_46F7A4, unk_46F7BB, unk_46F7D2, unk_46F7E9, unk_46F800, unk_46F817, unk_46F82E, unk_46F845,
    unk_46F85C, unk_46F873, unk_46F88A, unk_46F8A1, unk_46F8B8, unk_46F8CF, unk_46F8E6, unk_46F8FD,
    unk_46F914, unk_46F92B, unk_46F942, unk_46F959, unk_46F970, unk_46F987, unk_46F99E, unk_46F9B5,
    unk_46F9CC, unk_46F9E3, unk_46F9FA, unk_46FA11, unk_46FA28, unk_46FA3F, unk_46FA56, unk_46FA6D,
    unk_46FA84, unk_46FA9B, unk_46FAB2, unk_46FAC9, unk_46FAE0, unk_46FAF7, unk_46FB0E, unk_46FB25,
    unk_46FB3C, unk_46FB53, unk_46FB6A, unk_46FB81, unk_46FB98, unk_46FBAF, unk_46FBC6, unk_46FBDD,
    unk_46FBF4, unk_46FC0B, unk_46FC22, unk_46FC39, unk_46FC50, unk_46FC67, unk_46FC7E, unk_46FC95,
    unk_46FCAC, unk_46FCC3, unk_46FCDA, unk_46FCF1, unk_46FD08, unk_46FD1F, unk_46FD36, unk_46FD4D,
    unk_46FD64, unk_46FD7B, unk_46FD92, unk_46FDA9, unk_46FDC0, unk_46FDD7, unk_46FDEE, unk_46FE05,
    unk_46FE1C, unk_46FE33, unk_46FE4A, unk_46FE61, unk_46FE78, unk_46FE8F, unk_46FEA6, unk_46FEBD,
    unk_46FED4, unk_46FEEB, unk_46FF02, unk_46FF19, unk_46FF30, unk_46FF47, unk_46FF5E, unk_46FF75,
    unk_46FF8C, unk_46FFA3, unk_46FFBA, unk_46FFD1, unk_46FFE8, unk_46FFFF, unk_470016, unk_47002D,
    unk_470044, unk_47005B, unk_470072, unk_470089, unk_4700A0, unk_4700B7, unk_4700CE, unk_4700E5,
    unk_4700FC, unk_470113, unk_47012A, unk_470141, unk_470158, unk_47016F, unk_470186, unk_47019D,
    unk_4701B4, unk_4701CB, unk_4701E2, unk_4701F9, unk_470210, unk_470227, unk_47023E, unk_470255,
    unk_47026C, unk_470283, unk_47029A, unk_4702B1, unk_4702C8, unk_4702DF, unk_4702F6, unk_47030D,
    unk_470324, unk_47033B, unk_470352, unk_470369, unk_470380, unk_470397, unk_4703AE, unk_4703C5,
    unk_4703DC, unk_4703F3, unk_47040A, unk_470421, unk_470438, unk_47044F, unk_470466, unk_47047D,
    unk_470494, unk_4704AB, unk_4704C2, unk_4704D9, unk_4704F0, unk_470507, unk_47051E, unk_470535,
    unk_47054C, unk_470563, unk_47057A, unk_470591, unk_4705A8, unk_4705BF, unk_4705D6, unk_4705ED,
    unk_470604, unk_47061B, unk_470632, unk_470649, unk_470660, unk_470677, unk_47068E, unk_4706A5,
    unk_4706BC, unk_4706D3, unk_4706EA, unk_470701, unk_470718, unk_47072F, unk_470746, unk_47075D,
    unk_470774, unk_47078B, unk_4707A2, unk_4707B9, unk_4707D0, unk_4707E7, unk_4707FE, unk_470815,
    unk_47082C, unk_470843, unk_47085A, unk_470871, unk_470888, unk_47089F, unk_4708B6, unk_4708CD,
    unk_4708E4, unk_4708FB, unk_470912, unk_470929, unk_470940, unk_470957, unk_47096E, unk_470985,
    unk_47099C, unk_4709B3, unk_4709CA, unk_4709E1, unk_4709F8, unk_470A0F, unk_470A26, unk_470A3D,
    unk_470A54, unk_470A6B, unk_470A82, unk_470A99, unk_470AB0, unk_470AC7, unk_470ADE, unk_470AF5,
    unk_470B0C, unk_470B23, unk_470B3A, unk_470B51, unk_470B68, unk_470B7F, unk_470B96, unk_470BAD,
    unk_470BC4, unk_470BDB, unk_470BF2, unk_470C09, unk_470C20, unk_470C37, unk_470C4E, unk_470C65,
    unk_470C7C, unk_470C93, unk_470CAA, unk_470CC1, unk_470CD8, unk_470CEF, unk_470D06, unk_470D1D,
    unk_470D34, unk_470D4B, unk_470D62, unk_470D79, unk_470D90, unk_470DA7, unk_470DBE, unk_470DD5,
    unk_470DEC, unk_470E03, unk_470E1A, unk_470E31, unk_470E48, unk_470E5F, unk_470E76, unk_470E8D,
    unk_470EA4, unk_470EBB, unk_470ED2, unk_470EE9, unk_470F00, unk_470F17, unk_470F2E, unk_470F45,
    unk_470F5C, unk_470F73, unk_470F8A, unk_470FA1, unk_470FB8, unk_470FCF, unk_470FE6, unk_470FFD,
    unk_471014, unk_47102B, unk_471042, unk_471059, unk_471070, unk_471087, unk_47109E, unk_4710B5,
    unk_4710CC, unk_4710E3, unk_4710FA, unk_471111, unk_471128, unk_47113F, unk_471156, unk_47116D,
    unk_471184, unk_47119B, unk_4711B2, unk_4711C9, unk_4711E0, unk_4711F7, unk_47120E, unk_471225,
    unk_47123C, unk_471253, unk_47126A, unk_471281, unk_471298, unk_4712AF, unk_4712C6, unk_4712DD,
    unk_4712F4, unk_47130B, unk_471322, unk_471339, unk_471350, unk_471367, unk_47137E, unk_471395,
    unk_4713AC, unk_4713C3, unk_4713DA, unk_4713F1, unk_471408, unk_47141F, unk_471436, unk_47144D,
    unk_471464, unk_47147B, unk_471492, unk_4714A9, unk_4714C0, unk_4714D7, unk_4714EE, unk_471505,
    unk_47151C, unk_471533, unk_47154A, unk_471561, unk_471578, unk_47158F, unk_4715A6, unk_4715BD,
    unk_4715D4, unk_4715EB, unk_471602, unk_471619, unk_471630, unk_471647, unk_47165E, unk_471675,
    unk_47168C, unk_4716A3, unk_4716BA, unk_4716D1, unk_4716E8, unk_4716FF, unk_471716, unk_47172D
};

static char unk_471A98[] = "Å¸¸ð¸® Å¸´ÙÁö";
static char unk_471ABD[] = "£â£é£í£ï£ô£á";
static char unk_471B00[] = "£Ó£È£À£Ò£Ð";
static char unk_471B25[] = "½Å¹Ù ¿ì·ÕÂ÷";
static char unk_471B68[] = "ÇòÆ÷ÄÚ±º";
static char unk_471B8D[] = "£Ó£Ô£Õ£Ä£É£Ï È£Çª³ª ÇØ¹æÀü¼±";
static char unk_471BD0[] = "È÷Ä«¿Í ÇìÅ°·ç";
static char unk_471BF5[] = "½ºÆÄÀÌ½Ã ´ëÀÛÀü";
static char unk_471C38[] = "ÀÌ½º";
static char unk_471C5D[] = "Å¸Å×¿ÀÄ« µµÀå";
static char unk_471CA0[] = "³×±âÅ¸¸¶£¦Ã÷³ë´Ù È÷»ç½Ã";
static char unk_471CC5[] = "Ä¡Äí¿ÍÀÇ ¸¶À½";
static char unk_471D08[] = "Ä­ÀÚÅ° È÷·Î";
static char unk_471D2D[] = "¾ßÅ°´ÏÄí Á¤½Ä";
static char unk_471D70[] = "´Ï½Ã¸¶Å¸ ¾Æ¿ÀÀÌ";
static char unk_471D95[] = "£Ê£Ï£Ë£Å£Ò¡¡£Ô£Ù£Ð£Å";
static char unk_471DD8[] = "¾ÆÅ°¹Ù ³ª±â";
static char unk_471DFD[] = "¿À·¹»ç¸¶ ¸Þ¸ð¸®¾ó";
static char unk_471E40[] = "¾ß³ë Å¸Äí¹Ì£¦Ä«°¡¿Í Åä¸ð³ëºÎ";
static char unk_471E65[] = "½ºÄ«Æùµµ";
static char unk_471EA8[] = "ÀÌ¼Ò¿ìµµ¿ì¹ÙÄí";
static char unk_471ECD[] = "£Í£Ç¡¡£×£Ï£Ò£Ë£Ó";
static char unk_471F10[] = "È£¸£¸ó ·»Áö·Î£¦£Í£Á£Ò£Ã£Ùµ¶";
static char unk_471F35[] = "Á÷µµ°ü";
static char unk_471F78[] = "Å¸Ã÷³×ÄÚ";
static char unk_471F9D[] = "ÄÚÅ¸Ã÷Áý";
static char unk_471FE0[] = "Â¡Â¡";
static char unk_472005[] = "Â¡Â¡";
static char unk_472048[] = "¹ÌÁîÅ° Å¸Äí¾ß";
static char unk_47206D[] = "¹ÌÁî¸ð Å¬·´";
static char unk_4720B0[] = "³ªµ¥¾Æ¶ó Å¸ÄÉ¿ä½Ã";
static char unk_4720D5[] = "£Ú£­£ö£å£ã£ô£ï£ò";
static char unk_472118[] = "·ù³ë½ºÄÉ£¦Â÷³ë¹Ìº¸¿ìÁî";
static char unk_47213D[] = "Â÷Â÷Á¶";
static char unk_472180[] = "¾ÆÀÌÀÚ¿Í È÷·Î½Ã";
static char unk_4721A5[] = "£È£é£ç£è£Ò£é£ó£ë£Ò£å£ö£ï£ì£õ£ô£é£ï£î";
static char unk_4721E8[] = "¾Æ¶óÀÌ Ä«Áî»çÅ°";
static char unk_47220D[] = "¸¶·ç¾Æ¶óÀÌ";
static char unk_472250[] = "¾Æ¸®¸¶ ÄÉÀÌÅ¸·Î";
static char unk_472275[] = "ÀÏº» ¿Í¸£¿Í¸£ µ¿¸Í";
static char unk_4722B8[] = "Ã÷·ç±â ¾ß½ºÀ¯Å°";
static char unk_4722DD[] = "ÇÐ¿ø ¿ë»çºÎ";
static char unk_472320[] = "°ÕÃ÷ ¹ÌÄ«¹Ì";
static char unk_472345[] = "°ÕÃ÷ Á¦ÀÛ¼Ò";
static char unk_472388[] = "¾ÆÅ°¿ä½Ã ¿ä½Ã¾ÆÅ°";
static char unk_4723AD[] = "£Á£î£ï£ò£á£ë¡¡£Ð£ï£ó£ô";
static char unk_4723F0[] = "»çÅ° Ä«¿À¸®";
static char unk_472415[] = "£¹£¸Å¬·´";
static char unk_472458[] = "Áø½Å";
static char unk_47247D[] = "°ð µ¹¾Æ¿É´Ï´Ù";
static char unk_4724C0[] = "¹ÌÄ«ÁîÅ° ¾ÆÅ°¶ó£¡";
static char unk_4724E5[] = "£Ô£Ò£É£­£Í£Ï£Ï£Î£¡";
static char unk_472528[] = "»çÀÌÅä¿ì Ã÷Ä«»ç";
static char unk_47254D[] = "ºù°í ¸¸¾ß";
static char unk_472590[] = "¿ä½ÃÀÚ¿Í Åä¸ð¾ÆÅ°£¦¿À¿ÀÃ÷Å° ·áÅ°";
static char unk_4725B5[] = "£Ð£ó£ù£­£×£á£ì£ë£å£î";
static char unk_4725F8[] = "ÀÌÃ÷Å°£¦ÄËÅ¸";
static char unk_47261D[] = "ºñÇÁ ½ºÅ×ÀÌÅ©";
static char unk_472660[] = "ÀÚ·Î¿ì ¾ÆÅ°¶ó";
static char unk_472685[] = "½ºÆ©µð¿À ²ÞÈ¥";
static char unk_4726C8[] = "Á¦Äí¿ì Åä¿À·ç";
static char unk_4726ED[] = "°Ë¼º ÆÐ¿Õ »óÈ¸";
static char unk_472730[] = "Å°Ä¡Äí È÷·ÎÄÚ";
static char unk_472755[] = "Å°Ä¡Å°Ä¡ Å¬·´";
static char unk_472798[] = "´Ï½ÃÅ° ½Ã·Î";
static char unk_4727BD[] = "£Í£Á£Ú£Å£­£Å£Ø";
static char unk_472800[] = "¾ÆÀÚ¶ó½Ã Ä«Áî¸¶";
static char unk_472825[] = "£Ë¡¯£ó¡¡£Æ£Á£Ã£Ô£Ï£Ò£Ù";
static char unk_472868[] = "£ö£ï£ç£õ£å";
static char unk_47288D[] = "£Ö£Ï£Ç£Õ£Å";
static char unk_4728D0[] = "³ª³ª¼¼ ¾Æ¿ÀÀÌ";
static char unk_4728F5[] = "ÆÄ¿ö ±×¶óµ¥ÀÌ¼Ç";
static char unk_472938[] = "È÷¶óÅ° ³ª¿À¸®£¦±«±¤¼±£¦¿ä½Ã´Ù °íÅÙ";
static char unk_47295D[] = "°ÜÀÚ ¸í¶õ";
static char unk_4729A0[] = "£Ã£È£Ï£Ã£Ï";
static char unk_4729C5[] = "ÃÊÄÝ¸´¡¤¼¥";
static char unk_472A08[] = "»ê¼î ³ª³ª¹Ì";
static char unk_472A2D[] = "£Ú£å£ò£ï¡¡£Ð£ì£ï£ô£ô£å£ò";
static char unk_472A70[] = "¹Ý·ÎÈ£";
static char unk_472A95[] = "°ËÀº ¼Ó³» ÇüÁ¦";
static char unk_472AD8[] = "£Ã£È£Á£Ò£Í";
static char unk_472AFD[] = "£Ã£è£á£ò£í£é£î£ç¡¡£Ó£ï£æ£ô£÷£á£ò£å";
static char unk_472B40[] = "¹ÌÃ÷¹Ì ¹Ì»çÅä";
static char unk_472B65[] = "£Ã£Õ£Ô¡¡£Á¡¡£Ä£Á£Ó£È£¡£¡";
static char unk_472BA8[] = "Ä­·ÎÁÖ";
static char unk_472BCD[] = "£Â£ì£á£ú£å£ò¡¡£Ï£î£å";
static char unk_472C10[] = "Å¸ÄÉ¿ìÄ¡ ¿ä½Ã¹Ì";
static char unk_472C35[] = "¾´¾¦";
static char unk_472C78[] = "Å°¹«¶ó¾ß ·á";
static char unk_472C9D[] = "£Ô£å£á£í ÅäÄ«Ä¡";
static char unk_472CE0[] = "ÄÚ¹«·Î ÄÉÀÌ½ºÄÉ";
static char unk_472D05[] = "£Ó£Ô£Õ£Ä£É£Ï ¾ÆÀÎ·ù";
static char unk_472D48[] = "¹ÌÁîÅ¸´Ï Åä¿À·ç";
static char unk_472D6D[] = "Å©·©Å©¡¤ÀÎ";
static char unk_472DB0[] = "Áø³ëÁ¶£¦Äí·Îº¸½Ã ÄÚ¿ìÇÏÄí";
static char unk_472DD5[] = "ÄõÅÍºä";
static char unk_472E18[] = "È÷¶óÅ° ³ª¿ÀÅä½Ã";
static char unk_472E3D[] = "ÁöÀ¯°¡¿ÀÄ« »óÁ¡È¸";
static char unk_472E80[] = "¿À¿À³ë Å×Ã÷¾ß";
static char unk_472EA5[] = "£Ð£õ£ó£ó£ù¡¤£Ã£Á£Ô";
static char unk_472EE8[] = "´ÙÄ«ÇÏ½Ã Å¸Ã÷¾ß";
static char unk_472F0D[] = "ÀÙ»ç±Í Å¬·´";
static char unk_472F50[] = "¹Ì³ªÁîÅ° Åä¿À·ç";
static char unk_472F75[] = "½ºÆ©µð¿À ÀÙ»ç±Í";
static char unk_472FB8[] = "»ï°¢ÇüÀÇ ³× ¹øÂ° º¯";
static char unk_472FDD[] = "ÇÏ¶ó´Ù ¿ì´Ù·ç";
static char unk_473020[] = "ÇÏ±â¾ß¸¶ »çÄ«°Ô";
static char unk_473045[] = "ÆÄ¿îÆ¾ ½ºÄù¾î";
static char unk_473088[] = "Ä­´©Å° ¿ä¾ÆÄÉ";
static char unk_4730AD[] = "°í¾çÀÌ ¹Ù³ª³ª";
static char unk_4730F0[] = "»çÄí¶õ";
static char unk_473115[] = "¸¶Ä«·Î´Ï ÀüÇÏ";
static char unk_473158[] = "³×³ëÃ÷Å° À¯Å°½Ã·Î£¦·Î¹Â";
static char unk_47317D[] = "³×³ëÃ÷Å°£¦·Î¹Â";
static char unk_4731C0[] = "°¡¿Í´Ù À¯";
static char unk_4731E5[] = "±â¸§Áý";

static char unk_473228[] = "È²Á¦ ¾×±â½º";
static char unk_473258[] = "¸¸È­ ´ÜÇàº»";
static char unk_473288[] = "½´ÆÛ ÆæÃË";
static char unk_4732B8[] = "Á¦¿Õ ¾×±â½º";
static char unk_4732E8[] = "½´ÆÛ ¸¶Ä¿";
static char unk_473318[] = "´ÞÀÎÀÇ Ã¥";
static char unk_473348[] = "ÀÌ»óÇÑ ÆíÁö";
static char unk_473378[] = "È¯»óÀÇ °ú°Å";
static char unk_4733A8[] = "Àü¼³ÀÇ ¸ðÀÚ";
static char unk_4733D8[] = "Àü¼³ÀÇ º×";
static char unk_473408[] = "¿µ±¤ÀÇ ¹Ì·¡";

static char* strings_471A98[] = { unk_471A98 };
static char* strings_471ABD[] = { unk_471ABD };
static char* strings_471B00[] = { unk_471B00 };
static char* strings_471B25[] = { unk_471B25 };
static char* strings_471B68[] = { unk_471B68 };
static char* strings_471B8D[] = { unk_471B8D };
static char* strings_471BD0[] = { unk_471BD0 };
static char* strings_471BF5[] = { unk_471BF5 };
static char* strings_471C38[] = { unk_471C38 };
static char* strings_471C5D[] = { unk_471C5D };
static char* strings_471CA0[] = { unk_471CA0 };
static char* strings_471CC5[] = { unk_471CC5 };
static char* strings_471D08[] = { unk_471D08 };
static char* strings_471D2D[] = { unk_471D2D };
static char* strings_471D70[] = { unk_471D70 };
static char* strings_471D95[] = { unk_471D95 };
static char* strings_471DD8[] = { unk_471DD8 };
static char* strings_471DFD[] = { unk_471DFD };
static char* strings_471E40[] = { unk_471E40 };
static char* strings_471E65[] = { unk_471E65 };
static char* strings_471EA8[] = { unk_471EA8 };
static char* strings_471ECD[] = { unk_471ECD };
static char* strings_471F10[] = { unk_471F10 };
static char* strings_471F35[] = { unk_471F35 };
static char* strings_471F78[] = { unk_471F78 };
static char* strings_471F9D[] = { unk_471F9D };
static char* strings_471FE0[] = { unk_471FE0 };
static char* strings_472005[] = { unk_472005 };
static char* strings_472048[] = { unk_472048 };
static char* strings_47206D[] = { unk_47206D };
static char* strings_4720B0[] = { unk_4720B0 };
static char* strings_4720D5[] = { unk_4720D5 };
static char* strings_472118[] = { unk_472118 };
static char* strings_47213D[] = { unk_47213D };
static char* strings_472180[] = { unk_472180 };
static char* strings_4721A5[] = { unk_4721A5 };
static char* strings_4721E8[] = { unk_4721E8 };
static char* strings_47220D[] = { unk_47220D };
static char* strings_472250[] = { unk_472250 };
static char* strings_472275[] = { unk_472275 };
static char* strings_4722B8[] = { unk_4722B8 };
static char* strings_4722DD[] = { unk_4722DD };
static char* strings_472320[] = { unk_472320 };
static char* strings_472345[] = { unk_472345 };
static char* strings_472388[] = { unk_472388 };
static char* strings_4723AD[] = { unk_4723AD };
static char* strings_4723F0[] = { unk_4723F0 };
static char* strings_472415[] = { unk_472415 };
static char* strings_472458[] = { unk_472458 };
static char* strings_47247D[] = { unk_47247D };
static char* strings_4724C0[] = { unk_4724C0 };
static char* strings_4724E5[] = { unk_4724E5 };
static char* strings_472528[] = { unk_472528 };
static char* strings_47254D[] = { unk_47254D };
static char* strings_472590[] = { unk_472590 };
static char* strings_4725B5[] = { unk_4725B5 };
static char* strings_4725F8[] = { unk_4725F8 };
static char* strings_47261D[] = { unk_47261D };
static char* strings_472660[] = { unk_472660 };
static char* strings_472685[] = { unk_472685 };
static char* strings_4726C8[] = { unk_4726C8 };
static char* strings_4726ED[] = { unk_4726ED };
static char* strings_472730[] = { unk_472730 };
static char* strings_472755[] = { unk_472755 };
static char* strings_472798[] = { unk_472798 };
static char* strings_4727BD[] = { unk_4727BD };
static char* strings_472800[] = { unk_472800 };
static char* strings_472825[] = { unk_472825 };
static char* strings_472868[] = { unk_472868 };
static char* strings_47288D[] = { unk_47288D };
static char* strings_4728D0[] = { unk_4728D0 };
static char* strings_4728F5[] = { unk_4728F5 };
static char* strings_472938[] = { unk_472938 };
static char* strings_47295D[] = { unk_47295D };
static char* strings_4729A0[] = { unk_4729A0 };
static char* strings_4729C5[] = { unk_4729C5 };
static char* strings_472A08[] = { unk_472A08 };
static char* strings_472A2D[] = { unk_472A2D };
static char* strings_472A70[] = { unk_472A70 };
static char* strings_472A95[] = { unk_472A95 };
static char* strings_472AD8[] = { unk_472AD8 };
static char* strings_472AFD[] = { unk_472AFD };
static char* strings_472B40[] = { unk_472B40 };
static char* strings_472B65[] = { unk_472B65 };
static char* strings_472BA8[] = { unk_472BA8 };
static char* strings_472BCD[] = { unk_472BCD };
static char* strings_472C10[] = { unk_472C10 };
static char* strings_472C35[] = { unk_472C35 };
static char* strings_472C78[] = { unk_472C78 };
static char* strings_472C9D[] = { unk_472C9D };
static char* strings_472CE0[] = { unk_472CE0 };
static char* strings_472D05[] = { unk_472D05 };
static char* strings_472D48[] = { unk_472D48 };
static char* strings_472D6D[] = { unk_472D6D };
static char* strings_472DB0[] = { unk_472DB0 };
static char* strings_472DD5[] = { unk_472DD5 };
static char* strings_472E18[] = { unk_472E18 };
static char* strings_472E3D[] = { unk_472E3D };
static char* strings_472E80[] = { unk_472E80 };
static char* strings_472EA5[] = { unk_472EA5 };
static char* strings_472EE8[] = { unk_472EE8 };
static char* strings_472F0D[] = { unk_472F0D };
static char* strings_472F50[] = { unk_472F50 };
static char* strings_472F75[] = { unk_472F75 };
static char* strings_472FB8[] = { unk_472FB8 };
static char* strings_472FDD[] = { unk_472FDD };
static char* strings_473020[] = { unk_473020 };
static char* strings_473045[] = { unk_473045 };
static char* strings_473088[] = { unk_473088 };
static char* strings_4730AD[] = { unk_4730AD };
static char* strings_4730F0[] = { unk_4730F0 };
static char* strings_473115[] = { unk_473115 };
static char* strings_473158[] = { unk_473158 };
static char* strings_47317D[] = { unk_47317D };
static char* strings_4731C0[] = { unk_4731C0 };
static char* strings_4731E5[] = { unk_4731E5 };

static char* strings_473228[] = { unk_473228 };
static char* strings_473258[] = { unk_473258 };
static char* strings_473288[] = { unk_473288 };
static char* strings_4732B8[] = { unk_4732B8 };
static char* strings_4732E8[] = { unk_4732E8 };
static char* strings_473318[] = { unk_473318 };
static char* strings_473348[] = { unk_473348 };
static char* strings_473378[] = { unk_473378 };
static char* strings_4733A8[] = { unk_4733A8 };
static char* strings_4733D8[] = { unk_4733D8 };
static char* strings_473408[] = { unk_473408 };


#define writeStringBlock(a) ProcessStringBlock( 0x ## a, size_ ## a, strings_ ## a, (sizeof(strings_ ## a) / sizeof(char*)))
#define writeString_0x20(a) ProcessStringBlock( 0x ## a, 0x20, strings_ ## a, (sizeof(strings_ ## a) / sizeof(char*)))
#define writeString_0x25(a) ProcessStringBlock( 0x ## a, 0x25, strings_ ## a, (sizeof(strings_ ## a) / sizeof(char*)))
#define writeString_0x27(a) ProcessStringBlock( 0x ## a, 0x27, strings_ ## a, (sizeof(strings_ ## a) / sizeof(char*)))
#define updateRef(a) UpdateReferences( 0x ## a, def_ ## a, (sizeof(def_ ## a) / sizeof(char*)))

static void ApplyPatches() {

    // KoS, KoR Àû¿ë
    writeStringBlock(46B840);
    writeStringBlock(46B864);

    writeStringBlock(461764);
    writeStringBlock(462AA0);
    writeStringBlock(463E10);
    writeStringBlock(464220);
    writeStringBlock(464590);
    writeStringBlock(46AEC8);
    writeStringBlock(46B204);
    writeStringBlock(46B5D8);
    writeStringBlock(46B630);
    writeStringBlock(46BF68);
    writeStringBlock(46B650);
    writeStringBlock(46BA6C);
    writeStringBlock(46BAA8);
    writeStringBlock(46BB10);
    writeStringBlock(46BB80);
    writeStringBlock(46BBC0);
    writeStringBlock(46BC38);
    writeStringBlock(46BE0C);
    writeStringBlock(46BE30);
    writeStringBlock(46BE38);
    writeStringBlock(46C05C);
    writeStringBlock(46C09C);
    writeStringBlock(46C1DC);
    writeStringBlock(46C244);
    writeStringBlock(46C448);
    writeStringBlock(46C540);
    writeStringBlock(46C678);
    writeStringBlock(46C978);
    writeStringBlock(46CC28);

    checkString = 0;

    // ÀÌ¸§ ÀÔ·Â ±¸°£
    writeStringBlock(46CD7C);

    // ¼º+ÀÌ¸§, ÀÛÇ° Á¾·ù Ç¥½Ã
    // writeStringBlock(46C12C);

    checkString = 1;

    writeStringBlock(46F0CC);
    writeStringBlock(46F2F0);
    writeStringBlock(46F3A8);
    writeStringBlock(473A80);
    writeStringBlock(475074);
    writeStringBlock(47515C);
    writeStringBlock(475630);

    alignSize = 0x17;
    writeStringBlock(46F7A4);

    alignSize = 0x25;
    writeString_0x25(471A98);
    writeString_0x25(471B00);
    writeString_0x25(471B68);
    writeString_0x25(471BD0);
    writeString_0x25(471C38);
    writeString_0x25(471CA0);
    writeString_0x25(471D08);
    writeString_0x25(471D70);
    writeString_0x25(471DD8);
    writeString_0x25(471E40);
    writeString_0x25(471EA8);
    writeString_0x25(471F10);
    writeString_0x25(471F78);
    writeString_0x25(471FE0);
    writeString_0x25(472048);
    writeString_0x25(4720B0);
    writeString_0x25(472118);
    writeString_0x25(472180);
    writeString_0x25(4721E8);
    writeString_0x25(472250);
    writeString_0x25(4722B8);
    writeString_0x25(472320);
    writeString_0x25(472388);
    writeString_0x25(4723F0);
    writeString_0x25(472458);
    writeString_0x25(4724C0);
    writeString_0x25(472528);
    writeString_0x25(472590);
    writeString_0x25(4725F8);
    writeString_0x25(472660);
    writeString_0x25(4726C8);
    writeString_0x25(472730);
    writeString_0x25(472798);
    writeString_0x25(472800);
    writeString_0x25(472868);
    writeString_0x25(4728D0);
    writeString_0x25(472938);
    writeString_0x25(4729A0);
    writeString_0x25(472A08);
    writeString_0x25(472A70);
    writeString_0x25(472AD8);
    writeString_0x25(472B40);
    writeString_0x25(472BA8);
    writeString_0x25(472C10);
    writeString_0x25(472C78);
    writeString_0x25(472CE0);
    writeString_0x25(472D48);
    writeString_0x25(472DB0);
    writeString_0x25(472E18);
    writeString_0x25(472E80);
    writeString_0x25(472EE8);
    writeString_0x25(472F50);
    writeString_0x25(472FB8);
    writeString_0x25(473020);
    writeString_0x25(473088);
    writeString_0x25(4730F0);
    writeString_0x25(473158);
    writeString_0x25(4731C0);

    alignSize = 0x27;
    writeString_0x27(471ABD);
    writeString_0x27(471B25);
    writeString_0x27(471B8D);
    writeString_0x27(471BF5);
    writeString_0x27(471C5D);
    writeString_0x27(471CC5);
    writeString_0x27(471D2D);
    writeString_0x27(471D95);
    writeString_0x27(471DFD);
    writeString_0x27(471E65);
    writeString_0x27(471ECD);
    writeString_0x27(471F35);
    writeString_0x27(471F9D);
    writeString_0x27(472005);
    writeString_0x27(47206D);
    writeString_0x27(4720D5);
    writeString_0x27(47213D);
    writeString_0x27(4721A5);
    writeString_0x27(47220D);
    writeString_0x27(472275);
    writeString_0x27(4722DD);
    writeString_0x27(472345);
    writeString_0x27(4723AD);
    writeString_0x27(472415);
    writeString_0x27(47247D);
    writeString_0x27(4724E5);
    writeString_0x27(47254D);
    writeString_0x27(4725B5);
    writeString_0x27(47261D);
    writeString_0x27(472685);
    writeString_0x27(4726ED);
    writeString_0x27(472755);
    writeString_0x27(4727BD);
    writeString_0x27(472825);
    writeString_0x27(47288D);
    writeString_0x27(4728F5);
    writeString_0x27(47295D);
    writeString_0x27(4729C5);
    writeString_0x27(472A2D);
    writeString_0x27(472A95);
    writeString_0x27(472AFD);
    writeString_0x27(472B65);
    writeString_0x27(472BCD);
    writeString_0x27(472C35);
    writeString_0x27(472C9D);
    writeString_0x27(472D05);
    writeString_0x27(472D6D);
    writeString_0x27(472DD5);
    writeString_0x27(472E3D);
    writeString_0x27(472EA5);
    writeString_0x27(472F0D);
    writeString_0x27(472F75);
    writeString_0x27(472FDD);
    writeString_0x27(473045);
    writeString_0x27(4730AD);
    writeString_0x27(473115);
    writeString_0x27(47317D);
    writeString_0x27(4731E5);

    alignSize = 0x20;
    writeString_0x20(473228);
    writeString_0x20(473258);
    writeString_0x20(473288);
    writeString_0x20(4732B8);
    writeString_0x20(4732E8);
    writeString_0x20(473318);
    writeString_0x20(473348);
    writeString_0x20(473378);
    writeString_0x20(4733A8);
    writeString_0x20(4733D8);
    writeString_0x20(473408);
    //writeStringBlock(000000);
    

    // 461764
    updateRef(4610E8);
    // 462AA0
    updateRef(4627D0);
    // 463E10
    updateRef(463DA4);
    // 464220
    updateRef(40A9A0);
    updateRef(40A946);
    updateRef(40A922);
    updateRef(40A8F2);
    updateRef(40AD19);
    updateRef(40AD0A);
    updateRef(40ACD0);
    // 464590
    updateRef(4643BC);
    updateRef(4644B8);
    updateRef(464524);
    // 46AEC8
    updateRef(411BC6);
    updateRef(411B9C);
    updateRef(411B3C);
    // 46B204
    updateRef(46B0D0);
    updateRef(416642);
    updateRef(4166A8);
    updateRef(4166EE);
    updateRef(448B31);
    updateRef(448B56);
    // 46B5D8
    updateRef(46B5D0);
    // 46B630
    updateRef(46B628);
    // 46BF68
    updateRef(46BE6C);
    // 46B650
    updateRef(4192EE);
    updateRef(4191E7);
    updateRef(4191B9);
    // 46BA6C
    updateRef(46BA64);
    // 46BAA8
    updateRef(4196E0);
    // 46BB10
    updateRef(46BAEC);
    // 46BB80
    updateRef(41A746);
    // 46BBC0
    updateRef(41B37D);
    updateRef(428B98);
    updateRef(41C24F);
    updateRef(41CECF);
    // 46BC38
    updateRef(41E4AE);
    updateRef(41E504);
    updateRef(41E5D7);
    updateRef(41E6E9);
    // 46C09C
    updateRef(46C078);
    updateRef(46F2C8);
    // 46C1DC
    updateRef(424688);
    updateRef(42464E);
    updateRef(424509);
    // 46C244
    updateRef(46C238);
    // 46C448
    updateRef(42783D);
    updateRef(427836);
    updateRef(428BDB);
    // 46C540
    updateRef(46C530);
    updateRef(42CD67);
    updateRef(42CD5C);
    updateRef(42D64A);
    // 46C678
    updateRef(46C670);
    // 46C978
    updateRef(46C8B4);
    updateRef(4359C8);
    updateRef(43596C);
    updateRef(435873);
    // 46CC28
    updateRef(46CBE0);
    updateRef(46CBF0);
    // 46CD7C
    updateRef(46CCB4);
    // 46F0CC
    updateRef(46F0C4);
    updateRef(46EFB0);
    updateRef(4403A7);
    updateRef(4405DD);
    updateRef(440BE7);
    updateRef(440D33);
    updateRef(440A2D);
    updateRef(440A98);
    updateRef(440E6F);
    updateRef(440B07);
    updateRef(441001);
    updateRef(440B77);
    updateRef(4411BF);
    updateRef(440E18);
    updateRef(440FA9);
    updateRef(44114F);
    updateRef(44146A);
    updateRef(4413EC);
    updateRef(4412D7);
    updateRef(441570);
    updateRef(441524);
    updateRef(44153E);
    // 46F3A8
    updateRef(4234F2);
    updateRef(42351F);
    updateRef(42354A);
    updateRef(42428E);
    updateRef(4499AE);
    updateRef(41044C);
    updateRef(410471);
    updateRef(43B09C);
    updateRef(43B0C1);
    updateRef(4229B2);
    updateRef(4229C4);
    // 473A80
    updateRef(46F714);
    updateRef(471744);
    updateRef(473438);
    // 475074
    updateRef(447E84);
    // 47515C
    updateRef(4750A4);
    updateRef(448461);
    updateRef(448DBE);
    updateRef(448D19);
    updateRef(448D6D);
    updateRef(448CC5);
    updateRef(448C45);
    updateRef(448BA9);
    updateRef(448B77);
    updateRef(448B60);
    updateRef(448B3F);
    updateRef(448B27);
    // 475630
    updateRef(44EB38);

}

// =============================================================
// [¸ÞÀÎ ÇÔ¼ö]
// =============================================================

int main() {
    FILE* fp = fopen(TARGET_FILENAME, "rb");
    if (!fp) {
        printf("Failed to open %s\n", TARGET_FILENAME);
        return 1;
    }

    // ÆÄÀÏ Å©±â ÃøÁ¤ ¹× ¹öÆÛ ·Îµå
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fileBuffer.resize(fileSize);
    fread(fileBuffer.data(), 1, fileSize, fp);
    fclose(fp);

    printf("File loaded. Size: %ld bytes\n", fileSize);

    // ÆÐÄ¡ Àû¿ë
    ApplyPatches();

    // °á°ú ÀúÀå
    fp = fopen(OUTPUT_FILENAME, "wb");
    if (!fp) {
        printf("Failed to write %s\n", OUTPUT_FILENAME);
        return 1;
    }
    fwrite(fileBuffer.data(), 1, fileSize, fp);
    fclose(fp);

    printf("Done. Saved as %s\n", OUTPUT_FILENAME);
    system("pause");
    return 0;
}