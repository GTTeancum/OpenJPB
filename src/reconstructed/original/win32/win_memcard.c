/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\win32\win_memcard.c.
 * PDB module: 0106
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\win_memcard.obj
 * Primary source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 * Compiler language: c
 * Emitted procedures: 14
 */

#include "jpb/win_memcard.h"

#include "jpb/console.h"

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>

static char buf[256];
static WIN32_FIND_DATAA mc_fd;
static HANDLE mc_fdhandle;

static int memcard_is_dot_entry(const char *name)
{
    return name[0] == '.' &&
           (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

/* 0x12F040, 51 bytes, global, 2 named locals
 * cardfile
 * PDB type: char* (int, char*)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
char *cardfile(int card, char *filename)
{
    if (card == -1) {
        card = console_currentmemcard;
    }
    sprintf(buf, "c:\\katavo\\winver\\memcard\\%d\\%s", card, filename);
    return buf;
}

/* 0x12F080, 3 bytes, global, 2 named locals
 * kmMemcard_Delete
 * PDB type: void (int, char*)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
void kmMemcard_Delete(int card, char *ptrFileName)
{
    (void)card;
    (void)ptrFileName;
}

/* 0x12F090, 3 bytes, global, 0 named locals
 * kmMemcard_Exit
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
void kmMemcard_Exit(void)
{
}

/* 0x12F0A0, 3 bytes, global, 2 named locals
 * kmMemcard_Load
 * PDB type: void (int, char*)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
void kmMemcard_Load(int card, char *ptrFileName)
{
    (void)card;
    (void)ptrFileName;
}

/* 0x12F0B0, 3 bytes, global, 4 named locals
 * kmMemcard_Save
 * PDB type: void (int, char*, void*, long)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
void kmMemcard_Save(
    int card, char *ptrFileName, void *ptrData, long size)
{
    (void)card;
    (void)ptrFileName;
    (void)ptrData;
    (void)size;
}

/* 0x12F0C0, 3 bytes, global, 0 named locals
 * kmMemcard_Update
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
void kmMemcard_Update(void)
{
}

/* 0x12F0D0, 57 bytes, global, 2 named locals
 * memcard_DeleteFile
 * PDB type: void (int, char*)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
void memcard_DeleteFile(int card, char *filename)
{
    DeleteFileA(cardfile(card, filename));
}

/* 0x12F110, 86 bytes, global, 3 named locals
 * memcard_FileExists
 * PDB type: int (int, char*)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
int memcard_FileExists(int card, char *filename)
{
    FILE *file = fopen(cardfile(card, filename), "rb");
    if (file != NULL) {
        fclose(file);
        return 1;
    }
    return 0;
}

/* 0x12F170, 210 bytes, global, 1 named locals
 * memcard_FindFirstFile
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
char *memcard_FindFirstFile(int card)
{
    mc_fdhandle = FindFirstFileA(cardfile(card, "*"), &mc_fd);
    if (mc_fdhandle != NULL) {
        while (memcard_is_dot_entry(mc_fd.cFileName)) {
            if (!FindNextFileA(mc_fdhandle, &mc_fd)) {
                FindClose(mc_fdhandle);
                return NULL;
            }
        }
        return mc_fd.cFileName;
    }
    return NULL;
}

/* 0x12F250, 200 bytes, global, 1 named locals
 * memcard_FindNextFile
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
char *memcard_FindNextFile(int card)
{
    (void)card;
    if (mc_fdhandle != NULL) {
        if (FindNextFileA(mc_fdhandle, &mc_fd)) {
            while (memcard_is_dot_entry(mc_fd.cFileName)) {
                if (!FindNextFileA(mc_fdhandle, &mc_fd)) {
                    FindClose(mc_fdhandle);
                    return NULL;
                }
            }
            return mc_fd.cFileName;
        }
        FindClose(mc_fdhandle);
    }
    return NULL;
}

/* 0x12F320, 364 bytes, global, 8 named locals
 * memcard_LoadFile
 * PDB type: int (int, char*, unsigned char**...
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
int memcard_LoadFile(
    int card, char *filename, unsigned char **data, unsigned long *size)
{
    FILE *file = fopen(cardfile(card, filename), "rb");
    int rc = 0;
    unsigned char *memory = NULL;
    long file_size;

    if (file == NULL) {
        console_Printf("Can't open %s\n", buf);
        return -4;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (file_size == 0) {
        console_Printf("%s is empty!\n", buf);
        rc = -3;
    } else {
        memory = (unsigned char *)malloc((size_t)file_size);
        if (memory == NULL) {
            console_Printf(
                "Can't alloc %d bytes to load %s\n", (int)file_size, buf);
            rc = -2;
        } else if ((long)fread(memory, 1, (size_t)file_size, file) !=
                   file_size) {
            console_Printf(
                "Error reading %d bytes from %s\n", (int)file_size, buf);
            rc = -1;
        } else {
            *data = memory;
            *size = (unsigned long)file_size;
        }
    }

    fclose(file);
    fclose(file);
    if (memory != NULL && rc != 0) {
        free(memory);
    }
    return rc;
}

/* 0x12F490, 171 bytes, global, 5 named locals
 * memcard_SaveFile
 * PDB type: void (int, char*, unsigned char*...
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
void memcard_SaveFile(
    int card, char *filename, unsigned char *data, unsigned long size)
{
    FILE *file = fopen(cardfile(card, filename), "wb");
    if (file != NULL) {
        size_t written = fwrite(data, 1, size, file);
        console_Printf(
            written == size ? "wrote %d bytes to %s\n"
                            : "error writing %d bytes to %s\n",
            size,
            buf);
        fclose(file);
    }
}

/* 0x12F540, 3 bytes, global, 0 named locals
 * memcard_off
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
void memcard_off(void)
{
}

/* 0x12F550, 3 bytes, global, 0 named locals
 * memcard_on
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\win32\win_memcard.c
 */
void memcard_on(void)
{
}
