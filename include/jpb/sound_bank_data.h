#ifndef JPB_SOUND_BANK_DATA_H
#define JPB_SOUND_BANK_DATA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    AUDIO_SFX_BANK_COUNT = 43,
    AUDIO_SFX_BANK_NAME_CAPACITY = 62
};

/* Exact matched-PC PDB type: 80-byte tAudioSFX_Bank. */
typedef struct tAudioSFX_Bank {
    char bankName[AUDIO_SFX_BANK_NAME_CAPACITY];
    char **ptrSFXNames;
    int numSFXs;
} tAudioSFX_Bank;

extern tAudioSFX_Bank
    audioSFX_aSFXBanks[AUDIO_SFX_BANK_COUNT];

#ifdef __cplusplus
}
#endif

#endif
