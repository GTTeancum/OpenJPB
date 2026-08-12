#ifndef JPB_SOUND_BANK_DATA_H
#define JPB_SOUND_BANK_DATA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { JPB_SOUND_BANK_TABLE_COUNT = 43 };

typedef struct JPBSoundBankData {
    const char *directory;
    const char *const *paths;
    int count;
} JPBSoundBankData;

extern const JPBSoundBankData
    jpb_soundBankTable[JPB_SOUND_BANK_TABLE_COUNT];

#ifdef __cplusplus
}
#endif

#endif

