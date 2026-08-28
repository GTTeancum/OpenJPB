#ifndef JPB_LEVEL_H
#define JPB_LEVEL_H

#include "jpb/fmath.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct playerObject playerObject;

enum { JPB_SPARK_ROOM_ARC_COUNT = 5 };
enum { JPB_LEVEL_BOUND_CORNER_COUNT = 4 };

/* Exact PDB globals, type VECTOR[5]. */
extern VECTOR maSpark1[JPB_SPARK_ROOM_ARC_COUNT];
extern VECTOR maSpark2[JPB_SPARK_ROOM_ARC_COUNT];
extern FVECTOR g_levelUVScroll;
extern _svector FedBounds[JPB_LEVEL_BOUND_CORNER_COUNT];
extern _svector CorusBounds[JPB_LEVEL_BOUND_CORNER_COUNT];
extern _svector palaceExitBounds[JPB_LEVEL_BOUND_CORNER_COUNT];

void BigPinkPulsatingShaft(
    int x,
    int z,
    int width,
    uint32_t color,
    int section,
    int zpush_value);
int bigcheck(int index);
void calcboxcoord(playerObject *player, int *x, int *y);
void corecheck0(playerObject *player, _svector *position);
void corecheck1(playerObject *player, _svector *position);
int corecheck2(
    playerObject *player,
    _svector *position1,
    _svector *position2);
void corecheck3(playerObject *player, _svector *points);
int corefloorglow(uint32_t *color, int position);
void core_specials(void);
void drawbigpoly(uint32_t color, int sort_offset);
void glowdeath(void);
void level_CoreWater(void);
void level_InitSpecials(int level);
void oldlevel_Mini3(void);
void plotmaze(
    unsigned maze1,
    unsigned maze2,
    unsigned color1,
    unsigned color2,
    int transition);
void plotnode(
    _svector *node,
    int direction1,
    int direction2,
    int x_offset,
    int y_offset,
    uint32_t color1,
    uint32_t color2,
    int transition,
    int length);
void level_Arena(void);
void level_Corus(void);
void level_CountDown(int time, int kill, int score);
void level_Fed(void);
void level_Hangar(void);
void level_Mini1(int time, int kill);
void level_Mini2(void);
void level_Mini3(void);
void level_Mini4(void);
void level_Palace(void);
void level_Palace_KillOffscreenBoss(int enemy_index);
void level_Ruins(void);
void level_SparkRoom(void);
void level_Theed(void);
void drawsomecrappywater(
    _svector *water,
    int count,
    float factor1,
    float factor2,
    int speed1,
    int speed2,
    uint32_t color1,
    uint32_t color2);
int standingonit(
    int player_index,
    _svector *upper,
    _svector *lower);

#ifdef __cplusplus
}
#endif

#endif
