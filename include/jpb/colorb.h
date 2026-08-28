#ifndef JPB_COLORB_H
#define JPB_COLORB_H

#include "jpb/list.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DVECTOR {
    int16_t vx;
    int16_t vy;
} DVECTOR;

typedef struct cb_header {
    Node node;
    int id;
} cb_header;

typedef struct cb_circle {
    Node node;
    int id;
    int x1;
    int y1;
    int w;
    int h;
    int color;
} cb_circle;

typedef struct cb_line2d {
    Node node;
    int id;
    int x1;
    int y1;
    int x2;
    int y2;
    int color;
} cb_line2d;

typedef struct cb_move {
    Node node;
    int id;
    int x1;
    int y1;
    int color;
} cb_move;

typedef struct cb_point {
    Node node;
    int id;
    int x1;
    int y1;
    int w;
    int h;
    int color;
} cb_point;

extern List cb_list;
extern DVECTOR pen;
extern int penColor;

void cb_DrawList(void);
void cb_FreeList(void);
void cb_InitColorBasic(void);
int console_CircleCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float);
int console_LineCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float);
int console_MoveCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float);
int console_PointCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float);
int console_ScreenClearCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_COLORB_STATIC_ASSERT static_assert
#else
#define JPB_COLORB_STATIC_ASSERT _Static_assert
#endif

JPB_COLORB_STATIC_ASSERT(sizeof(DVECTOR) == 4, "DVECTOR PDB layout changed");
JPB_COLORB_STATIC_ASSERT(sizeof(cb_header) == 16,
                         "cb_header PDB layout changed");
JPB_COLORB_STATIC_ASSERT(offsetof(cb_circle, color) == 28,
                         "cb_circle.color PDB offset changed");
JPB_COLORB_STATIC_ASSERT(sizeof(cb_circle) == 32,
                         "cb_circle PDB layout changed");
JPB_COLORB_STATIC_ASSERT(sizeof(cb_line2d) == 32,
                         "cb_line2d PDB layout changed");
JPB_COLORB_STATIC_ASSERT(sizeof(cb_move) == 24,
                         "cb_move PDB layout changed");
JPB_COLORB_STATIC_ASSERT(sizeof(cb_point) == 32,
                         "cb_point PDB layout changed");

#undef JPB_COLORB_STATIC_ASSERT

#endif
