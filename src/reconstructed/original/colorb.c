/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\work\colorb.c.
 * PDB module: 0015
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\colorb.obj
 * Primary source: W:\SWJediPowerBattles\work\colorb.c
 * Compiler language: c
 * Emitted procedures: 8
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/colorb.h"

#include "jpb/console.h"

#include <stdlib.h>

List cb_list;
DVECTOR pen;
int penColor;

/* 0x26670, 3 bytes, global, 0 named locals
 * cb_DrawList
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\colorb.c
 */
void cb_DrawList(void)
{
}

/* 0x26680, 51 bytes, global, 1 named locals
 * cb_FreeList
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\colorb.c
 */
void cb_FreeList(void)
{
    cb_header *node;

    while ((node = (cb_header *)list_RemoveHead(&cb_list)) != NULL) {
        free(node);
    }
}

/* 0x266C0, 12 bytes, global, 0 named locals
 * cb_InitColorBasic
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\colorb.c
 */
void cb_InitColorBasic(void)
{
    list_InitList(&cb_list);
}

/* 0x266D0, 184 bytes, global, 5 named locals
 * console_CircleCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\colorb.c
 */
int console_CircleCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    cb_circle *circle;

    (void)arg_str;
    (void)arg_float;
    if (narg == 4) {
        circle = (cb_circle *)malloc(sizeof(*circle));
        if (circle != NULL) {
            circle->id = 4;
            list_AddTail(&cb_list, &circle->node);
            circle->x1 = arg_int[0];
            circle->y1 = arg_int[1];
            circle->w = arg_int[2];
            circle->h = arg_int[2];
            circle->color = arg_int[3];
        }
    } else if (narg == 3) {
        circle = (cb_circle *)malloc(sizeof(*circle));
        if (circle != NULL) {
            circle->id = 4;
            list_AddTail(&cb_list, &circle->node);
            circle->x1 = arg_int[0];
            circle->y1 = arg_int[1];
            circle->w = arg_int[2];
            circle->h = arg_int[2];
            circle->color = penColor;
        }
    }
}

/* 0x26790, 225 bytes, global, 5 named locals
 * console_LineCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\colorb.c
 */
int console_LineCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    cb_line2d *line;

    (void)arg_str;
    (void)arg_float;
    if (narg == 5) {
        line = (cb_line2d *)malloc(sizeof(*line));
        if (line != NULL) {
            line->id = 1;
            list_AddTail(&cb_list, &line->node);
            line->x1 = arg_int[0];
            line->y1 = arg_int[1];
            line->x2 = arg_int[2];
            line->y2 = arg_int[3];
            line->color = arg_int[4];
        }
    } else if (narg == 4) {
        line = (cb_line2d *)malloc(sizeof(*line));
        if (line != NULL) {
            line->id = 1;
            list_AddTail(&cb_list, &line->node);
            line->x1 = arg_int[0];
            line->y1 = arg_int[1];
            line->x2 = arg_int[2];
            line->y2 = arg_int[3];
            line->color = penColor;
        }
    } else {
        console_Printf("line x1 y1 x2 y2 clut\n");
        console_Printf("line x1 y1 x2 y2\n");
    }
}

/* 0x26880, 201 bytes, global, 5 named locals
 * console_MoveCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\colorb.c
 */
int console_MoveCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    cb_move *move;

    (void)arg_str;
    (void)arg_float;
    if (narg == 3) {
        move = (cb_move *)malloc(sizeof(*move));
        if (move != NULL) {
            move->id = 5;
            list_AddTail(&cb_list, &move->node);
            move->x1 = arg_int[0];
            move->y1 = arg_int[1];
            move->color = arg_int[2];
        }
    } else if (narg == 2) {
        move = (cb_move *)malloc(sizeof(*move));
        if (move != NULL) {
            move->id = 5;
            list_AddTail(&cb_list, &move->node);
            move->x1 = arg_int[0];
            move->y1 = arg_int[1];
            move->color = penColor;
        }
    } else {
        console_Printf("move deltaX deltaY clut\n");
        console_Printf("move deltaX deltaY\n");
    }
}

/* 0x26950, 434 bytes, global, 5 named locals
 * console_PointCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\colorb.c
 */
int console_PointCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    cb_point *point;

    (void)arg_str;
    (void)arg_float;
    if (narg == 5) {
        point = (cb_point *)malloc(sizeof(*point));
        if (point != NULL) {
            point->id = 2;
            list_AddTail(&cb_list, &point->node);
            point->x1 = arg_int[0];
            point->y1 = arg_int[1];
            point->w = arg_int[2];
            point->h = arg_int[3];
            point->color = arg_int[4];
        }
    } else if (narg == 4) {
        point = (cb_point *)malloc(sizeof(*point));
        if (point != NULL) {
            point->id = 2;
            list_AddTail(&cb_list, &point->node);
            point->x1 = arg_int[0];
            point->y1 = arg_int[1];
            point->w = arg_int[2];
            point->h = arg_int[3];
            point->color = penColor;
        }
    } else if (narg == 3) {
        point = (cb_point *)malloc(sizeof(*point));
        if (point != NULL) {
            point->id = 2;
            list_AddTail(&cb_list, &point->node);
            point->x1 = arg_int[0];
            point->y1 = arg_int[1];
            point->w = 1;
            point->h = 1;
            point->color = arg_int[2];
        }
    } else if (narg == 2) {
        point = (cb_point *)malloc(sizeof(*point));
        if (point != NULL) {
            point->id = 2;
            list_AddTail(&cb_list, &point->node);
            point->x1 = arg_int[0];
            point->y1 = arg_int[1];
            point->w = 1;
            point->h = 1;
            point->color = penColor;
        }
    } else {
        console_Printf("point x1 y1 w h clut\n");
        console_Printf("point x1 y1 w h\n");
        console_Printf("point x1 y1 clut\n");
        console_Printf("point x1 y1\n");
    }
}

/* 0x26B10, 83 bytes, global, 5 named locals
 * console_ScreenClearCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\colorb.c
 */
int console_ScreenClearCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;
    if (narg == 0) {
        cb_FreeList();
    } else {
        console_Printf("scls: clears the draw list\n");
    }
}
