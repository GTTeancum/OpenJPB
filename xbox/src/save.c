/* Xbox persistence adapter: the shared PC payloads (options and campaign
   progress) are stored in this title's E:\UDATA\<title id>\ folder. */
#include "jpb/savegame.h"
#include "jpb/game.h"
#include "jpb/alltext.h"
#include <nxdk/mount.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "save.h"

static char options_path[96];
static char game_path[96];
static int save_ready;
static int valid_game_save;
volatile int jpb_XboxSaveLastResult;
volatile unsigned jpb_XboxSaveWriteCount;

/* FATX writes stay in the kernel cache after fclose; a console switched off
   before the lazy writer runs loses them. Flush each completed file. */
static void flush_path(const char *path)
{
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    IO_STATUS_BLOCK status;
    NtFlushBuffersFile(file, &status);
    CloseHandle(file);
}

static void write_meta(const char *path, const char *text)
{
    /* Dashboard metadata files are UTF-16LE with a byte-order mark. */
    FILE *file;
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return;
    file = fopen(path, "wb");
    if (!file) return;
    fputc(0xff, file);
    fputc(0xfe, file);
    for (; *text; ++text) {
        fputc((unsigned char)*text, file);
        fputc(0, file);
    }
    fclose(file);
    flush_path(path);
}

int jpb_XboxSaveInit(void)
{
    char title[48], save[64], path[96];
    /* XBE header at 0x10000: +0x118 holds the certificate address and the
       certificate's title ID is at +8. */
    const uint8_t *certificate = *(const uint8_t *const *)(uintptr_t)0x10118u;
    uint32_t title_id = *(const uint32_t *)(certificate + 8);

    if (!nxIsDriveMounted('E') &&
        !nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\"))
        return 0;
    snprintf(title, sizeof(title), "E:\\UDATA\\%08x", (unsigned)title_id);
    snprintf(save, sizeof(save), "%s\\SAVEDATA0", title);
    CreateDirectoryA("E:\\UDATA", NULL);
    CreateDirectoryA(title, NULL);
    CreateDirectoryA(save, NULL);
    if (GetFileAttributesA(save) == INVALID_FILE_ATTRIBUTES) return 0;
    snprintf(path, sizeof(path), "%s\\TitleMeta.xbx", title);
    write_meta(path, "TitleName=OpenJPB\r\n");
    snprintf(path, sizeof(path), "%s\\SaveMeta.xbx", save);
    write_meta(path, "Name=Jedi Power Battles\r\n");
    snprintf(options_path, sizeof(options_path), "%s\\options.dat", save);
    snprintf(game_path, sizeof(game_path), "%s\\savegame.dat", save);
    save_ready = 1;
    return 1;
}

void jpb_XboxSaveLoadOptions(void)
{
    if (!save_ready) return;
    jpb_XboxSaveLastResult = jpb_SaveOptionsReadFile(options_path);
    if (jpb_XboxSaveLastResult == JPB_SAVE_OK)
        generateAllText(OptionStruct.Language);
}

void jpb_XboxSaveLoadGame(void)
{
    if (!save_ready) return;
    jpb_XboxSaveLastResult = jpb_SaveGameReadFile(game_path);
    valid_game_save = jpb_XboxSaveLastResult == JPB_SAVE_OK;
}

int jpb_XboxSaveHasGame(void)
{
    return valid_game_save;
}

void jpb_XboxSaveWriteGame(void)
{
    if (!save_ready) return;
    jpb_XboxSaveLastResult = jpb_SaveGameWriteFile(game_path);
    flush_path(game_path);
    if (jpb_XboxSaveLastResult == JPB_SAVE_OK) {
        valid_game_save = 1;
        ++jpb_XboxSaveWriteCount;
    }
}

void jpb_XboxSaveWriteOptions(const optionstruct *options)
{
    if (!save_ready || !options) return;
    jpb_XboxSaveLastResult = jpb_SaveOptionsWriteFileData(options_path, options);
    flush_path(options_path);
    if (jpb_XboxSaveLastResult == JPB_SAVE_OK) ++jpb_XboxSaveWriteCount;
}
