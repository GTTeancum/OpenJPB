#ifndef JPB_PWRUP_H
#define JPB_PWRUP_H

#include "jpb/fmath.h"
#include "jpb/list.h"
#include "jpb/vectors.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_POWERUP_CAPACITY = 128,
    JPB_POWERUP_CHECKPOINT_CAPACITY = 32,
    JPB_POWERUP_DISK_RECORD_SIZE = 12,
    JPB_POWERUP_TYPE_CHECKPOINT = 5,
    JPB_POWERUP_COLLECTED_FLAG = 0x8000
};

/* Exact matched-PC PDB type 0x70A9. */
typedef struct powerPoop {
    Node *node;
    _svector pos;
} powerPoop;

/*
 * Portable realization seam for exact PDB procedure DrawPowerUp. The
 * recovered gameplay owner still supplies its original position, type,
 * rotation, fixed-point scale, and offset; a host renderer may consume that
 * request without introducing a graphics-library dependency into pwrup.c.
 */
typedef void (*JPBPowerupDrawHook)(
    void *user_data,
    _svector *position,
    unsigned type,
    _svector *rotation,
    VECTOR *scale,
    _svector *offset);

extern int32_t maxCheckPoints;
extern int32_t usedCheckPoints;
extern _svector aCheckPoints[JPB_POWERUP_CHECKPOINT_CAPACITY];
extern List poopList[2];
extern powerPoop *poopArray;
extern int32_t gPoopMode;
extern unsigned powColorLimit;
extern int32_t powerUpScales[17];
extern CVECTOR pwrIcons[17];
extern int32_t mRandomPower[9];
extern int32_t cheat_currentCheckPoint;
extern char *powerUpNames[17];
extern char *powerUpFiles[17];

void DrawPowerUp(
    _svector *position,
    unsigned type,
    _svector *rotation,
    VECTOR *scale,
    _svector *offset);
void cheat_nextCheckPoint(void);
unsigned fixPowColor(unsigned color);
int kmAudioSFX_DumpBank(int bankID);
int mrktng_GoToNextCheckpoint(void);
void pwrup_CheckPowerUps(void);
void pwrup_Init(void);
int pwrup_JumpCheckPoint(void);
void pwrup_LevelEnd(void);
void pwrup_LevelStart(void);
void pwrup_LoadPoop(void);

void jpb_PwrupSetDrawHook(
    JPBPowerupDrawHook hook, void *user_data);

/* Inferred dependency-light boundaries around the 12-byte loadPoop stream. */
int jpb_PwrupLoadData(const void *data, size_t size);
size_t jpb_PwrupLoadedCount(void);
void jpb_PwrupReleaseData(void);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_PWRUP_STATIC_ASSERT(condition, message) \
    static_assert(condition, message)
#else
#define JPB_PWRUP_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)
#endif

JPB_PWRUP_STATIC_ASSERT(
    offsetof(powerPoop, node) == 0,
    "powerPoop.node layout changed");
JPB_PWRUP_STATIC_ASSERT(
    offsetof(powerPoop, pos) == sizeof(void *),
    "powerPoop.pos layout changed");
#if UINTPTR_MAX == UINT64_MAX
JPB_PWRUP_STATIC_ASSERT(
    sizeof(powerPoop) == 16,
    "x64 powerPoop must match PDB type 0x70A9");
#elif UINTPTR_MAX == UINT32_MAX
JPB_PWRUP_STATIC_ASSERT(
    sizeof(powerPoop) == 12,
    "32-bit powerPoop must compact to twelve bytes");
#else
#error Unsupported pointer width for powerPoop
#endif

#undef JPB_PWRUP_STATIC_ASSERT

#endif
