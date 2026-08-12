#ifndef JPB_BRAINUTL_H
#define JPB_BRAINUTL_H

#include "jpb/player.h"
#include "jpb/collision.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact PDB type `NodeScale` (20 bytes). */
typedef struct NodeScale {
    int32_t NodeID;
    VECTOR Scale;
} NodeScale;

enum {
    JPB_BRAINUTL_CHEAT_BIG_HEAD = 0x01,
    JPB_BRAINUTL_CHEAT_BIG_FEET_AND_SABER = 0x02,
    JPB_BRAINUTL_CHEAT_SMALL_MODE = 0x04
};

/*
 * Portable boundary for the original SDL keyboard-state queries. The core
 * consumes complete chord state and remains independent of SDL/Win32.
 */
typedef uint32_t (*JPBBrainutlCheatChordProvider)(void *user_data);

extern NodeScale BigHandsFeetNodeScale[8];
extern int32_t cheat_bigHeadPressed[2];
extern int32_t cheat_bigHeadKeyPressed;
extern int32_t cheat_smallModeKeyPressed;
extern int32_t cheat_bigHead[2];
extern int32_t cheat_smallModePressed[2];
extern int32_t cheat_smallMode[2];
extern int32_t cheat_bigFeetAndSaberPressed[2];
extern int32_t cheat_bigFeetAndSaberKeyPressed;
extern int32_t cheat_bigFeetAndSaber[2];

void jpb_BrainutlSetCheatChordProvider(
    JPBBrainutlCheatChordProvider provider,
    void *user_data);
int brainutl_AddSabreEdge(int playerID, int w0, int w1);
void brainutl_ConformGeomNodes(playerObject *player);

uint32_t brainutl_ElapsedTime(
    uint32_t time, uint32_t duration);
uint32_t brainutl_DeltaTime(uint32_t time);
int brainutil_PauseControl(
    int32_t *cpad, playerObject *player);
int brainutl_FindLSB(uint32_t flag);
int brainutl_FindLSB_LV(uint32_t flag);
void brainutl_HeldPad(
    playerObject *player, int32_t *cpad);
objectRoot *brainutl_gGetNearestTarget(
    objectRoot *object0, int type);
void brainutl_Land(playerObject *player);
void brainutl_MultiPad(
    playerObject *player, int32_t *cpad);
void brainutl_PlayMotionSound(
    int playernum, char *sound, int delay);
int brainutil_PlotMaulTrajectory(
    int32_t *cpad, playerObject *player);
int brainutil_PlotTrajectory(
    int32_t *cpad, playerObject *player);
int brainutil_ReverseCheck(playerObject *player);
void brainutil_limitRange(
    int *input, int min, int max);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
static_assert(sizeof(NodeScale) == 20, "NodeScale layout changed");
#else
_Static_assert(sizeof(NodeScale) == 20, "NodeScale layout changed");
#endif

#endif
