#include "jpb/colorb.h"

#include <stdio.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int check_commands(void)
{
    int values[5] = {11, 22, 33, 44, 55};
    cb_circle *circle;
    cb_line2d *line;
    cb_move *move;
    cb_point *point;

    cb_InitColorBasic();
    penColor = 91;
    (void)console_CircleCommand(3, NULL, values, NULL);
    (void)console_LineCommand(5, NULL, values, NULL);
    (void)console_MoveCommand(2, NULL, values, NULL);
    (void)console_PointCommand(3, NULL, values, NULL);

    circle = (cb_circle *)(void *)cb_list.head;
    CHECK(circle != NULL);
    CHECK(circle->id == 4 && circle->x1 == 11 && circle->y1 == 22);
    CHECK(circle->w == 33 && circle->h == 33 && circle->color == 91);

    line = (cb_line2d *)(void *)circle->node.next;
    CHECK(line != NULL);
    CHECK(line->id == 1 && line->x1 == 11 && line->y1 == 22);
    CHECK(line->x2 == 33 && line->y2 == 44 && line->color == 55);

    move = (cb_move *)(void *)line->node.next;
    CHECK(move != NULL);
    CHECK(move->id == 5 && move->x1 == 11 && move->y1 == 22);
    CHECK(move->color == 91);

    point = (cb_point *)(void *)move->node.next;
    CHECK(point != NULL);
    CHECK(point->id == 2 && point->x1 == 11 && point->y1 == 22);
    CHECK(point->w == 1 && point->h == 1 && point->color == 33);
    CHECK(point->node.next == NULL);
    CHECK(cb_list.tail == &point->node);

    cb_DrawList();
    CHECK(cb_list.head == &circle->node);
    (void)console_ScreenClearCommand(0, NULL, NULL, NULL);
    CHECK(cb_list.head == NULL && cb_list.tail == NULL);
    return 0;
}

static int check_alternate_forms(void)
{
    int values[5] = {-1, -2, -3, -4, -5};
    cb_circle *circle;
    cb_line2d *line;
    cb_move *move;
    cb_point *point;

    cb_InitColorBasic();
    penColor = 72;
    (void)console_CircleCommand(4, NULL, values, NULL);
    (void)console_LineCommand(4, NULL, values, NULL);
    (void)console_MoveCommand(3, NULL, values, NULL);
    (void)console_PointCommand(2, NULL, values, NULL);
    (void)console_PointCommand(4, NULL, values, NULL);
    (void)console_PointCommand(5, NULL, values, NULL);

    circle = (cb_circle *)(void *)cb_list.head;
    line = (cb_line2d *)(void *)circle->node.next;
    move = (cb_move *)(void *)line->node.next;
    point = (cb_point *)(void *)move->node.next;
    CHECK(circle->color == -4);
    CHECK(line->color == 72);
    CHECK(move->color == -3);
    CHECK(point->w == 1 && point->h == 1 && point->color == 72);
    point = (cb_point *)(void *)point->node.next;
    CHECK(point->w == -3 && point->h == -4 && point->color == 72);
    point = (cb_point *)(void *)point->node.next;
    CHECK(point->w == -3 && point->h == -4 && point->color == -5);

    cb_FreeList();
    CHECK(cb_list.head == NULL && cb_list.tail == NULL);
    return 0;
}

int main(void)
{
    CHECK(check_commands() == 0);
    CHECK(check_alternate_forms() == 0);
    puts("Color Basic command tests passed");
    return 0;
}
