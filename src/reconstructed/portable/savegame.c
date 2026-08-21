/*
 * Dependency-light persistence boundary for the matched PC save payloads.
 * The on-disk sizes and filenames come from PDB procedures LoadGameData,
 * SaveGameData, LoadOptionsData, and SaveSettingsData in wHook.cpp.
 */

#include "jpb/savegame.h"

#include "jpb/extracharacters.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void deserializeGameStruct(const void *data)
{
    if (data != NULL) {
        memcpy(&SaveGameStruct, data, sizeof(SaveGameStruct));
    }
}

void *serializeGameStruct(void)
{
    void *data = malloc(sizeof(SaveGameStruct));

    if (data != NULL) {
        memcpy(data, &SaveGameStruct, sizeof(SaveGameStruct));
    }
    return data;
}

static JPBSaveResult jpb_save_write_payload(
    const char *path, const void *data, size_t size)
{
    FILE *file;
    size_t written;
    int flush_result;
    int close_result;

    if (path == NULL || path[0] == '\0' || data == NULL) {
        return JPB_SAVE_BAD_ARGUMENT;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        return JPB_SAVE_IO_ERROR;
    }
    written = fwrite(data, 1, size, file);
    flush_result = fflush(file);
    close_result = fclose(file);
    if (written != size || flush_result != 0 || close_result != 0) {
        return JPB_SAVE_IO_ERROR;
    }
    return JPB_SAVE_OK;
}

static JPBSaveResult jpb_save_read_payload(
    const char *path, void *data, size_t size)
{
    FILE *file;
    size_t read_count;
    int trailing;

    if (path == NULL || path[0] == '\0' || data == NULL) {
        return JPB_SAVE_BAD_ARGUMENT;
    }
    errno = 0;
    file = fopen(path, "rb");
    if (file == NULL) {
        return errno == ENOENT
            ? JPB_SAVE_NOT_FOUND
            : JPB_SAVE_IO_ERROR;
    }
    read_count = fread(data, 1, size, file);
    trailing = read_count == size ? fgetc(file) : EOF;
    if (ferror(file) != 0) {
        (void)fclose(file);
        return JPB_SAVE_IO_ERROR;
    }
    if (fclose(file) != 0) {
        return JPB_SAVE_IO_ERROR;
    }
    if (read_count != size || trailing != EOF) {
        return JPB_SAVE_INVALID_DATA;
    }
    return JPB_SAVE_OK;
}

JPBSaveResult jpb_SaveGameWriteFile(const char *path)
{
    size_t index;

    UpdateSaveGameStruct();
    SaveGameStruct.saveFileVer = 0;
    SaveGameStruct.validFlag = 1;
    SaveGameStruct.lastlevel = GameStruct.CurrentLevel;
    SaveGameStruct.secretBits = secretBits;
    SaveGameStruct.players[0] =
        (uint8_t)GameStruct.ModelSelect[0];
    SaveGameStruct.players[1] =
        (uint8_t)GameStruct.ModelSelect[1];
    SaveGameStruct.unlockedExtraCharacters = 0;
    for (index = 0; index < ExtraCharactersSize; ++index) {
        if (ExtraCharacters[index].Unlocked != 0) {
            SaveGameStruct.unlockedExtraCharacters |=
                (uint16_t)(UINT16_C(1) << index);
        }
    }
    return jpb_save_write_payload(
        path, &SaveGameStruct, sizeof(SaveGameStruct));
}

JPBSaveResult jpb_SaveGameReadFile(const char *path)
{
    saveGameStruct loaded;
    JPBSaveResult result = jpb_save_read_payload(
        path, &loaded, sizeof(loaded));

    if (result != JPB_SAVE_OK) {
        return result;
    }
    if (loaded.saveFileVer != 0 ||
        (loaded.validFlag == 0 && loaded.continueAble == 0)) {
        return JPB_SAVE_INVALID_DATA;
    }
    SaveGameStruct = loaded;
    ApplySaveGameData();
    secretBits = SaveGameStruct.secretBits;
    GameStruct.ModelSelect[0] = SaveGameStruct.players[0];
    GameStruct.ModelSelect[1] = SaveGameStruct.players[1];
    return JPB_SAVE_OK;
}

JPBSaveResult jpb_SaveOptionsWriteFile(const char *path)
{
    OptionStruct.saveFileVer = 0;
    return jpb_SaveOptionsWriteFileData(path, &OptionStruct);
}

JPBSaveResult jpb_SaveOptionsWriteFileData(
    const char *path, const optionstruct *options)
{
    return jpb_save_write_payload(path, options, sizeof(*options));
}

JPBSaveResult jpb_SaveOptionsReadFile(const char *path)
{
    optionstruct loaded;
    JPBSaveResult result = jpb_save_read_payload(
        path, &loaded, sizeof(loaded));

    if (result != JPB_SAVE_OK) {
        return result;
    }
    if (loaded.saveFileVer != 0) {
        return JPB_SAVE_INVALID_DATA;
    }
    OptionStruct = loaded;
    return JPB_SAVE_OK;
}
