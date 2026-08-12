#ifndef JPB_LEVEL_H
#define JPB_LEVEL_H

#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { JPB_SPARK_ROOM_ARC_COUNT = 5 };
enum { JPB_LEVEL_BOUND_CORNER_COUNT = 4 };

/* Exact PDB globals, type VECTOR[5]. */
extern VECTOR maSpark1[JPB_SPARK_ROOM_ARC_COUNT];
extern VECTOR maSpark2[JPB_SPARK_ROOM_ARC_COUNT];
extern FVECTOR g_levelUVScroll;
extern _svector FedBounds[JPB_LEVEL_BOUND_CORNER_COUNT];
extern _svector CorusBounds[JPB_LEVEL_BOUND_CORNER_COUNT];
extern _svector palaceExitBounds[JPB_LEVEL_BOUND_CORNER_COUNT];

void level_Arena(void);
void level_Corus(void);
void level_CountDown(int time, int kill, int score);
void level_Fed(void);
void level_Hangar(void);
void level_Mini4(void);
void level_Palace(void);
void level_Palace_KillOffscreenBoss(int enemy_index);
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
