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

/* Exact PDB types used by the legacy three-slot memory-card owner. */
typedef struct miscSaveGameStruct {
    uint8_t pos[16][16];
} miscSaveGameStruct;

typedef struct MEMFILEHEADER {
    uint32_t saveChecksum;
    uint32_t optionChecksum;
    optionstruct OptionStruct;
    miscSaveGameStruct msg;
} MEMFILEHEADER;

typedef struct SAVEGAMEBLOCK {
    MEMFILEHEADER header;
    saveGameStruct saveGame[3];
} SAVEGAMEBLOCK;

extern SAVEGAMEBLOCK cardLoadBuffer;

#if defined(__cplusplus)
#define JPB_SAVEGAME_STATIC_ASSERT static_assert
#else
#define JPB_SAVEGAME_STATIC_ASSERT _Static_assert
#endif

JPB_SAVEGAME_STATIC_ASSERT(
    sizeof(miscSaveGameStruct) == 256,
    "miscSaveGameStruct must match the PDB layout");
JPB_SAVEGAME_STATIC_ASSERT(
    sizeof(MEMFILEHEADER) == 320,
    "MEMFILEHEADER must match the PDB layout");
JPB_SAVEGAME_STATIC_ASSERT(
    offsetof(MEMFILEHEADER, OptionStruct) == 8,
    "MEMFILEHEADER.OptionStruct offset changed");
JPB_SAVEGAME_STATIC_ASSERT(
    offsetof(MEMFILEHEADER, msg) == 64,
    "MEMFILEHEADER.msg offset changed");
JPB_SAVEGAME_STATIC_ASSERT(
    sizeof(SAVEGAMEBLOCK) == 14192,
    "SAVEGAMEBLOCK must match PDB type 0x7585");
JPB_SAVEGAME_STATIC_ASSERT(
    offsetof(SAVEGAMEBLOCK, saveGame) == 320,
    "SAVEGAMEBLOCK.saveGame offset changed");

#undef JPB_SAVEGAME_STATIC_ASSERT

/* Exact PDB-named raw-payload helpers at RVAs 0x127C70/0x128580. */
void deserializeGameStruct(const void *data);
void *serializeGameStruct(void);
void deserializeOptionStruct(const void *data, optionstruct *options);
void *serializeOptionsStruct(const optionstruct *options);

/*
 * Portable file boundary for the matched PC payloads. Game is exactly one
 * saveGameStruct (4,624 bytes); Options is exactly one optionstruct (56).
 */
JPBSaveResult jpb_SaveGameWriteFile(const char *path);
JPBSaveResult jpb_SaveGameReadFile(const char *path);
JPBSaveResult jpb_SaveOptionsWriteFile(const char *path);
JPBSaveResult jpb_SaveOptionsWriteFileData(
    const char *path, const optionstruct *options);
JPBSaveResult jpb_SaveOptionsReadFile(const char *path);

#ifdef __cplusplus
}
#endif

#endif
