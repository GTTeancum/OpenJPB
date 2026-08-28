/*
 * Reviewed reconstruction of W:\\SWJediPowerBattles\\work\\bucket.c.
 *
 * PDB symbols and types establish the complete public surface, global data,
 * and the 4,419-entry bucket registry. Control flow and constants were
 * checked directly against game.exe RVAs 0x218B0 through 0x2277A.
 */

#include "jpb/bucket.h"

#include "jpb/console.h"
#include "jpb/filesys.h"
#include "jpb/game.h"
#include "jpb/loader.h"
#include "jpb/memory.h"
#include "jpb/resources.h"
#include "jpb/sound.h"
#include "jpb/text.h"
#include "jpb/texture.h"
#include "jpb/world.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

enum {
    JPB_BUCKET_MAGIC = 0x4B55424A,
    JPB_BUCKET_COMPRESSED_MAGIC = 0x50434C52,
    JPB_BUCKET_ENTRY_SIZE = 0x108,
    JPB_BUCKET_ENTRY_NAME_OFFSET = 0x10,
    JPB_BUCKET_COMPRESSED_HEADER_SIZE = 0x10
};

char *bucketCom[JPB_BUCKET_COMMAND_COUNT] = {
    "?", "levels", "jedi", "front", "all", "save", "stats",
    "read", "write", "off", "test", "debugon", "debugoff"
};

uint16_t bucketModels[JPB_BUCKET_MODEL_COUNT] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x35, 0x36, 0x1A, 0x1C, 0x0F, 0x11, 0x12, 0x15, 0x1E,
    0x24, 0x25, 0x29, 0x30, 0x31, 0x32, 0x33
};

#include "bucket_list.inc"

uint8_t bucketDebug;
uint8_t *bucketLoadFP;
uint32_t bucketLoadOffset;
uint32_t bucketLoaded;
uint32_t bucketWrapper;
uint32_t bucketUncomp1;
uint32_t bucketUncomp2;
char bucketSubName[JPB_BUCKET_SUBNAME_CAPACITY];
uint32_t bucketCompChecksum;
uint32_t bucketUncompChecksum;
uint32_t bucketSubCompressedSize;
uint32_t bucketSubUncompressedSize;
uint32_t bucketSubCompressed;
uint32_t bucketSubCurrentFP;
uint8_t *bucketSubaddress;
uint32_t bucketSubsize;
char lastBukLoaded[JPB_BUCKET_LAST_NAME_CAPACITY];
FILE *bucketFP;

static uint32_t bucket_read_u32(const uint8_t *bytes)
{
    uint32_t value;

    memcpy(&value, bytes, sizeof(value));
    return value;
}

/* Reference RVA 0x218B0, 398 bytes. */
void bucketCopy(uint8_t *dst, unsigned offset, unsigned size)
{
    if (bucketSubCompressed == 0) {
        memcpy(dst, bucketSubaddress + offset, size);
        return;
    }

    if (offset == 0 && size >= bucketSubUncompressedSize) {
        ++bucketUncomp1;
        bucketRLDecompress(bucketSubaddress, dst, bucketSubCompressedSize);
        return;
    }

    ++bucketUncomp2;
    bucketDecompressRLRandom(bucketSubaddress, dst,
        bucketSubUncompressedSize, offset, size);
}

/* Reference RVA 0x21A40, 234 bytes. */
void bucketDecompressRLRandom(uint8_t *bufferIn, uint8_t *bufferOut,
    unsigned fsize, unsigned rdOffset, unsigned rdSize)
{
    uint8_t *endPtr = bufferIn + fsize;
    unsigned total = 0;
    unsigned wrtotal = 0;

    while (bufferIn != endPtr && total < rdSize) {
        unsigned count = (*bufferIn & 0x7Fu) + 1u;
        unsigned packetEnd = wrtotal + count;
        uint8_t *payload = bufferIn + 1;

        if ((int8_t)*bufferIn < 0) {
            if (packetEnd < rdOffset) {
                wrtotal = packetEnd;
                bufferIn += 2;
            } else {
                unsigned index;

                for (index = 0; index < count; ++index) {
                    if (wrtotal >= rdOffset && total < rdSize) {
                        bufferOut[total++] = *payload;
                    }
                    ++wrtotal;
                }
                bufferIn += 2;
            }
        } else {
            if (packetEnd >= rdOffset) {
                unsigned index;

                packetEnd = wrtotal;
                for (index = 0; index < count; ++index) {
                    if (packetEnd >= rdOffset && total < rdSize) {
                        bufferOut[total++] = payload[index];
                    }
                    ++packetEnd;
                }
            }
            bufferIn = payload + count;
            wrtotal = packetEnd;
        }
    }
}

/* Reference RVA 0x21B30, 455 bytes. */
int bucketFindFile(char *name, unsigned *bukNameIndex)
{
    unsigned index = bucketWrapper;
    unsigned passes = 2;
    int result = 0;
    char *filename = (char *)malloc(strlen(name) + 13u);

    strcpy(filename, "@bucket\\");
    strcat(filename, name);
    strcat(filename, ".buk");
    *bukNameIndex = 0;

    for (;;) {
        if (result != 0) {
            return result;
        }
        if (strcmp("lastfile", bucketList[index]) == 0) {
            --passes;
            index = 0;
        }
        if (bucketList[index][0] == '@') {
            *bukNameIndex = index;
        }
        if (strcmp(filename, bucketList[index]) == 0) {
            result = 1;
        }
        ++index;

        if (passes == 0) {
            char *extension;

            if (result != 0) {
                return result;
            }
            extension = strchr(filename, '.');
            if (strcmp(extension, ".adx") == 0 ||
                strcmp(extension, ".sfx") == 0 ||
                strcmp(extension, ".jpx") == 0) {
                return 0;
            }
            return result;
        }
    }
}

/* Reference RVA 0x21D00, 438 bytes. */
int bucketFindSubname(char *name, uint8_t *loadaddress,
    unsigned *size, uint8_t **address)
{
    unsigned filecount = bucket_read_u32(loadaddress + 4);
    unsigned index;

    strcpy(bucketSubName, name);
    for (index = 0; index < filecount; ++index) {
        size_t record = (size_t)index * JPB_BUCKET_ENTRY_SIZE;

        if (strcmp(name, (char *)loadaddress +
                JPB_BUCKET_ENTRY_NAME_OFFSET + record) == 0) {
            uint8_t *payload;

            *size = bucket_read_u32(loadaddress + 8 + record);
            payload = loadaddress + bucket_read_u32(loadaddress + 12 + record);
            *address = payload;
            bucketSubCurrentFP = 0;

            if (bucket_read_u32(payload) == JPB_BUCKET_COMPRESSED_MAGIC) {
                bucketSubCompressed = 1;
                *address += JPB_BUCKET_COMPRESSED_HEADER_SIZE;
                bucketSubUncompressedSize = bucket_read_u32(payload + 4);
                bucketUncompChecksum = bucket_read_u32(payload + 8);
                bucketCompChecksum = bucket_read_u32(payload + 12);
                bucketSubCompressedSize =
                    *size - JPB_BUCKET_COMPRESSED_HEADER_SIZE;
            } else {
                bucketSubCompressed = 0;
            }
            return 1;
        }
    }
    return 0;
}

/* Reference RVA 0x21EC0, 42 bytes. */
void bucketFront(void)
{
    console_Printf("bucket front starting.\n");
    bucketFP = (FILE *)(uintptr_t)UINT32_MAX;
    console_Printf("bucket front done.\n");
}

/* Reference RVA 0x21EF0, 6 bytes. */
int bucketHandleCommand(unsigned command)
{
    (void)command;
    return 1;
}

/* Reference RVA 0x21F00, 6 bytes. */
int bucketHandler(int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)narg;
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;
    return 1;
}

/* Reference RVA 0x21F10, 19 bytes. */
void bucketJedi(void)
{
    LevelSelect = 3;
    console_Printf("bucket writing disabled\n");
}

/* Reference RVA 0x21F30, 113 bytes. */
void bucketLevelLoad(unsigned level)
{
    char oldlevel = LevelSelect;

    LevelSelect = (char)level;
    GameStruct.NumPlayers = 1;
    initDataArrays();
    texture_Flush(UINT32_C(0xFFFFFFBF));
    game_InitGameSystems();
    sound_FreeBank(3);
    sound_FreeBank(2);
    sound_FreeBank(1);
    loader_LevelLoad();
    initDataArrays();
    texture_Flush(UINT32_C(0xFFFFFFBF));
    game_InitGameSystems();
    LevelSelect = oldlevel;
}

/* Reference RVA 0x21FB0, 12 bytes. */
void bucketLevels(void)
{
    console_Printf("bucket writing disabled\n");
}

/* Reference RVA 0x21FC0, 347 bytes. */
int bucketLoadBuk(unsigned bucketIndex, char *subname)
{
    char *name = bucketList[bucketIndex];
    unsigned result = 0;

    if (bucket_read_u32((uint8_t *)mem_pool_3a) == JPB_BUCKET_MAGIC) {
        result = (unsigned)bucketFindSubname(subname,
            (uint8_t *)mem_pool_3a, &bucketSubsize, &bucketSubaddress);
        if (result != 0) {
            return (int)result;
        }
        if (strcmp(name, lastBukLoaded) != 0) {
            bucketLoaded = 0;
        } else if (bucketLoaded != 0) {
            goto find_subname;
        }
    } else {
        bucketLoaded = 0;
    }

    (void)file_LoadFile(name, mem_pool_3a);
    bucketLoaded = 1;
    strcpy(lastBukLoaded, name);
    debugBucket("loaded bucket %s\n", name);

find_subname:
    if (bucket_read_u32((uint8_t *)mem_pool_3a) == JPB_BUCKET_MAGIC) {
        result = (unsigned)bucketFindSubname(subname,
            (uint8_t *)mem_pool_3a, &bucketSubsize, &bucketSubaddress);
    } else {
        bucketLoaded = 0;
        debugBucket("bad bucket %s %x %x %x\n", name,
            bucket_read_u32((uint8_t *)mem_pool_3a),
            JPB_BUCKET_MAGIC, (unsigned)(uintptr_t)mem_pool_3a);
    }
    return (int)result;
}

/* Reference RVA 0x22120, 130 bytes. */
void bucketRLDecompress(uint8_t *bufferIn, uint8_t *bufferOut, unsigned fsize)
{
    uint8_t *endPtr = bufferIn + fsize;

    while (bufferIn != endPtr) {
        unsigned count = (*bufferIn & 0x7Fu) + 1u;
        uint8_t *payload = bufferIn + 1;

        if ((int8_t)*bufferIn < 0) {
            memset(bufferOut, *payload, count);
            bufferIn += 2;
        } else {
            memcpy(bufferOut, payload, count);
            bufferIn = payload + count;
        }
        bufferOut += count;
    }
}

/* Reference RVA 0x221B0, 72 bytes. */
void bucketStringToCaps(char *s)
{
    unsigned index = 0;

    while (s[index] != '\0') {
        s[index] = (char)toupper((int)s[index]);
        ++index;
    }
}

/* Reference RVA 0x22200, 12 bytes. */
unsigned checkBucketFP(unsigned val)
{
    return val == JPB_BUCKET_MAGIC;
}

/* Reference RVA 0x22210, 3 bytes. */
void closeBucketLog(void)
{
}

/* Reference RVA 0x22220, 137 bytes. */
void debugBucket(char *string, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, string);
    vsprintf(buffer, string, args);
    va_end(args);
    printf("BUCKET: %s\n", buffer);
}

/* Reference RVA 0x222B0, 148 bytes. */
int gammaHandler(int narg, char **arg_str, int *arg_int, float *arg_float)
{
    float gamma;

    (void)arg_int;
    if (narg == 0) {
        console_Printf("Current gamma:%f %d\n", frontGamma, frontRGBoff);
        return 0;
    }
    if (arg_str[0][0] == '?') {
        console_Printf("USE:gamma 1.0\n");
        return 0;
    }

    gamma = arg_float[0];
    frontGamma = gamma;
    if (gamma > 1.0f) {
        gamma = 1.0f;
        frontGamma = gamma;
    }
    console_Printf("gamma:%1.4f\n", gamma);
    text_gInitialise(0);
    return 0;
}

/* Reference RVA 0x22350, 12 bytes. */
int gdirHandler(int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)narg;
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;
    return console_Printf("NOT AVAILABLE UNDER WINDOZE\n");
}

/* Reference RVA 0x22360, 5 bytes. */
void gdirOutput(char *text)
{
    (void)console_Printf(text);
}

/* Reference RVA 0x22370, 42 bytes. */
void initBucket(unsigned flag)
{
    if (flag == 0) {
        bucketFP = (FILE *)(uintptr_t)UINT32_MAX;
        bucketLoadFP = NULL;
        bucketLoadOffset = 0;
        bucketLoaded = 0;
        lastBukLoaded[0] = '\0';
    }
}

/* Reference RVA 0x223A0, 173 bytes. */
int memConsoleHandler(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)narg;
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;

    console_Printf("memory Free:%dK biggest Free:%dK\n", 0, 0);
    console_Printf("Pools:\n");
    console_Printf("    Max  Used\n");
    console_Printf("0:%04dK  %04dK\n", 0x80,
        maMemoryBanks[0].memSize - maMemoryBanks[0].memFree);
    console_Printf("1:%04dK  %04dK\n", 0x2BF,
        maMemoryBanks[1].memSize - maMemoryBanks[1].memFree);
    console_Printf("2:%04dK  %04dK\n", 0x600,
        maMemoryBanks[2].memSize - maMemoryBanks[2].memFree);
    return console_Printf("3:%04dK  %04dK\n", 0x1400,
        maMemoryBanks[3].memSize - maMemoryBanks[3].memFree);
}

/* Reference RVA 0x22450, 3 bytes. */
unsigned openBucket(char *in_name, long *fd, unsigned *offset)
{
    (void)in_name;
    (void)fd;
    (void)offset;
    return 0;
}

/* Reference RVA 0x22460, 11 bytes. */
void openBucketLog(char *name)
{
    (void)name;
    bucketFP = (FILE *)(uintptr_t)UINT32_MAX;
}

/* Reference RVA 0x22470, 3 bytes. */
void pauseUnpauseBucket(void)
{
}

/* Reference RVA 0x224E0, 244 bytes. */
unsigned ramBukChecksum(uint8_t *buffer, unsigned count)
{
    unsigned index;
    unsigned total = 0;

    for (index = 0; index < count; ++index) {
        total += buffer[index];
    }
    return total;
}

/* Reference RVA 0x225F0, 347 bytes. */
void updateBucketWrapper(uint8_t *name)
{
    unsigned index;
    unsigned passes = 2;
    int result = 0;
    char *filename;

    bucketWrapper = 0;
    (void)resource_getPathWithExtension(
        (char *)name, JPB_RESOURCE_BUCKET, "buk");
    index = bucketWrapper;
    filename = (char *)malloc(strlen((char *)name) + 13u);
    strcpy(filename, "@bucket\\");
    strcat(filename, (char *)name);
    strcat(filename, ".buk");
    bucketWrapper = 0;

    for (;;) {
        if (result != 0) {
            return;
        }
        if (strcmp("lastfile", bucketList[index]) == 0) {
            --passes;
            index = 0;
        }
        if (bucketList[index][0] == '@') {
            bucketWrapper = index;
        }
        if (strcmp(filename, bucketList[index]) == 0) {
            result = 1;
        }
        ++index;

        if (passes == 0) {
            if (result == 0) {
                (void)strchr(filename, '.');
            }
            return;
        }
    }
}

/* Reference RVA 0x22750, 17 bytes. */
int vmemHandler(int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)narg;
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;
    return console_Printf("VRAM  free:%dK   Max free:%dK\n", 0, 0);
}

/* Reference RVA 0x22770, 11 bytes. */
void writeBucketFileLog(char *name)
{
    (void)name;
    bucketFP = (FILE *)(uintptr_t)UINT32_MAX;
}
