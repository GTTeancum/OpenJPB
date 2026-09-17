/* Exact portable screen geometry from reconstructed original/wHook.cpp. */
#include "jpb/whook.h"
#include "jpb/menu.h"
#include <stdint.h>
#include <string.h>
static int jpb_whook_fixed_product(int left, int right)
{
    uint32_t bits = (uint32_t)left * (uint32_t)right;

    if ((bits & UINT32_C(0x80000000)) != 0) {
        bits = ~(~bits >> 12);
    } else {
        bits >>= 12;
    }
    int32_t result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

int cliptoscreen(short *pos)
{
    enum {
        LEFT = 1,
        RIGHT = 2,
        TOP = 4,
        BOTTOM = 8,
        MIN_X = 0x18,
        MAX_X = 0x768,
        MIN_Y = 8,
        MAX_Y = 0x430
    };
    int original_x = pos[0];
    int original_y = pos[1];
    int x = original_x;
    int y = original_y;
    int clipcode = 0;
    int alpha = 0xff;
    int center_delta_x;
    int center_delta_y;
    int y_over_x = 0;
    int x_over_y = 0;
    int vertical_distance;
    int horizontal_distance;

    if (x < MIN_X) {
        clipcode |= LEFT;
    }
    if (x > MAX_X) {
        clipcode |= RIGHT;
    }
    if (y < MIN_Y) {
        clipcode |= TOP;
    }
    if (y > MAX_Y) {
        clipcode |= BOTTOM;
    }
    if (clipcode == 0) {
        return alpha;
    }

    center_delta_y = (OptionStruct.ScreenHeight >> 1) - y;
    center_delta_x = (OptionStruct.ScreenWidth >> 1) - x;
    if (center_delta_x != 0) {
        y_over_x = (center_delta_y * 0x1000) / center_delta_x;
    }

    vertical_distance =
        (clipcode & TOP) != 0 ? MIN_Y - y : y - MAX_Y;
    horizontal_distance =
        (clipcode & LEFT) != 0 ? MIN_X - x : x - MAX_X;
    if (vertical_distance < (horizontal_distance >> 1)) {
        vertical_distance = horizontal_distance >> 1;
    }
    alpha = vertical_distance < 0x80
        ? 0xff - vertical_distance * 2
        : 0;

    if (center_delta_y != 0) {
        x_over_y = (center_delta_x * 0x1000) / center_delta_y;
    }
    if ((clipcode & (LEFT | RIGHT)) != 0 && center_delta_y == 0) {
        x = center_delta_x > 0 ? MIN_X : MAX_X;
        pos[0] = (short)x;
        pos[1] = (short)y;
        return alpha;
    }
    if ((clipcode & (TOP | BOTTOM)) != 0 && center_delta_x == 0) {
        y = center_delta_y > 0 ? MIN_Y : MAX_Y;
        pos[0] = (short)x;
        pos[1] = (short)y;
        return alpha;
    }

    switch (clipcode) {
    case LEFT:
        y += jpb_whook_fixed_product(MIN_X - x, y_over_x);
        x = MIN_X;
        break;
    case RIGHT:
        y += jpb_whook_fixed_product(MAX_X - x, y_over_x);
        x = MAX_X;
        break;
    case TOP:
        y = MIN_Y;
        x = original_x +
            jpb_whook_fixed_product(MIN_Y - original_y, x_over_y);
        break;
    case LEFT | TOP:
        y += jpb_whook_fixed_product(MIN_X - x, y_over_x);
        x = MIN_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MIN_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MIN_Y - original_y, x_over_y);
        }
        break;
    case RIGHT | TOP:
        y += jpb_whook_fixed_product(MAX_X - x, y_over_x);
        x = MAX_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MIN_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MIN_Y - original_y, x_over_y);
        }
        break;
    case BOTTOM:
        y = MAX_Y;
        x = original_x +
            jpb_whook_fixed_product(MAX_Y - original_y, x_over_y);
        break;
    case LEFT | BOTTOM:
        y += jpb_whook_fixed_product(MIN_X - x, y_over_x);
        x = MIN_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MAX_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MAX_Y - original_y, x_over_y);
        }
        break;
    case RIGHT | BOTTOM:
        y += jpb_whook_fixed_product(MAX_X - x, y_over_x);
        x = MAX_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MAX_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MAX_Y - original_y, x_over_y);
        }
        break;
    default:
        break;
    }
    pos[0] = (short)x;
    pos[1] = (short)y;
    return alpha;
}

void frontEndPoly(
    _Material *material,
    int vertex_count,
    FRONTENDVERT *vertices,
    float depth)
{
    int color = vertices[0].color;
    int index;

    _StartPoly(vertex_count, material);
    for (index = 0; index < vertex_count; ++index) {
        _SetVert(
            index,
            vertices[index].x,
            vertices[index].y,
            depth,
            (uint32_t)color,
            vertices[index].u,
            vertices[index].v);
    }
    _EndPoly();
}
