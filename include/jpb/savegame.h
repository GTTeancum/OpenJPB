#ifndef JPB_SAVEGAME_H
#define JPB_SAVEGAME_H

#include "jpb/game.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum JPBSaveResult {
    JPB_SAVE_OK = 0,
    JPB_SAVE_NOT_FOUND = 1,
    JPB_SAVE_INVALID_DATA = 2,
    JPB_SAVE_IO_ERROR = 3,
    JPB_SAVE_BAD_ARGUMENT = 4
} JPBSaveResult;

/* Exact PDB-named raw-payload helpers at RVAs 0x127C70/0x128580. */
void deserializeGameStruct(const void *data);
void *serializeGameStruct(void);

/*
 * Portable file boundary for the matched PC payloads. Game is exactly one
 * saveGameStruct (4,624 bytes); Options is exactly one optionstruct (56).
 */
JPBSaveResult jpb_SaveGameWriteFile(const char *path);
JPBSaveResult jpb_SaveGameReadFile(const char *path);
JPBSaveResult jpb_SaveOptionsWriteFile(const char *path);
JPBSaveResult jpb_SaveOptionsReadFile(const char *path);

#ifdef __cplusplus
}
#endif

#endif
