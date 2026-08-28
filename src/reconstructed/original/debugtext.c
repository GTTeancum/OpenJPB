/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\debugtext.c.
 *
 * All 32 emitted procedures and the module-owned lookup tables are checked
 * against the matched PDB and shipped executable. Focused regressions cover
 * formatting quirks, clipping, atlas coordinates, 2D and 3D projection,
 * rectangle depths, icon advances, and both renderer submission paths.
 *
 * PDB module: 0029
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\debugtext.obj
 * Primary source: W:\SWJediPowerBattles\work\debugtext.c
 * Compiler language: c
 * Emitted procedures: 32
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/debugtext.h"
#include "jpb/fmath.h"
#include "jpb/game.h"
#include "jpb/resources.h"
#include "jpb/scene.h"
#include "jpb/text.h"
#include "jpb/texture.h"
#include "jpb/whook.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

static JPBDraw3dTextHook jpb_draw3d_text_hook;
static void *jpb_draw3d_text_user_data;

/* Exact PDB globals at matched-PC RVAs 0x4B8AE0 and 0x51C090. */
float textclipRect[4] = {280.0f, 304.0f, 381.0f, 357.0f};
unsigned textClipping;

/* Exact module-local PDB owners at matched-PC RVAs 0x4B8AF0..0x51C2AF. */
static char *debugfont = "a_dbfont.tga";
static _Material *debugtext;
static float cursorx;
static float cursory;
static char text[512];
static _Material *flatmat;

/* Exact PDB global at matched-PC RVA 0x51C098. */
_Material *font;
/* Exact initialized PDB global at matched-PC RVA 0x4B7AE0. */
float Size_Bold[256] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0234f, 0.0547f, 0.0f, 0.0703f, 0.1133f, 0.0f, 0.0234f,
    0.0391f, 0.0352f, 0.0508f, 0.0f, 0.0195f, 0.0391f, 0.0234f, 0.043f,
    0.0664f, 0.0469f, 0.0664f, 0.0664f, 0.0742f, 0.0703f, 0.0703f, 0.0664f,
    0.0664f, 0.0664f, 0.0234f, 0.0234f, 0.0f, 0.0703f, 0.0f, 0.0781f,
    0.0f, 0.0938f, 0.082f, 0.0859f, 0.0859f, 0.0742f, 0.0703f, 0.0938f,
    0.0781f, 0.0234f, 0.0664f, 0.0859f, 0.0742f, 0.0977f, 0.0781f, 0.0938f,
    0.0781f, 0.0977f, 0.0859f, 0.0781f, 0.082f, 0.082f, 0.0898f, 0.125f,
    0.082f, 0.0898f, 0.082f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0664f, 0.0742f, 0.0664f, 0.0703f, 0.0664f, 0.0508f, 0.0742f,
    0.0703f, 0.0234f, 0.0352f, 0.0664f, 0.0273f, 0.1055f, 0.0664f, 0.0781f,
    0.0742f, 0.0703f, 0.0469f, 0.0664f, 0.0469f, 0.0664f, 0.0742f, 0.1055f,
    0.0703f, 0.0703f, 0.0625f
};

/* Exact initialized PDB globals at matched-PC RVAs 0x4B72E0..0x4B82E0. */
float U_Bold[256] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.6523f, 0.0977f, 0.0f, 0.7188f, 0.875f, 0.0f, 0.6914f,
    0.207f, 0.2578f, 0.4648f, 0.0f, 0.0625f, 0.1641f, 0.0156f, 0.5156f,
    0.0039f, 0.082f, 0.1563f, 0.2305f, 0.3008f, 0.3789f, 0.4531f, 0.5313f,
    0.6055f, 0.6797f, 0.3086f, 0.3477f, 0.0f, 0.3906f, 0.0f, 0.5625f,
    0.0f, 0.0039f, 0.1094f, 0.2031f, 0.3008f, 0.3984f, 0.4883f, 0.5664f,
    0.6758f, 0.7734f, 0.8047f, 0.8867f, 0.0078f, 0.0898f, 0.2031f, 0.2969f,
    0.4023f, 0.4883f, 0.5977f, 0.6914f, 0.7773f, 0.8672f, 0.0039f, 0.0938f,
    0.2188f, 0.3008f, 0.3906f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0039f, 0.082f, 0.1602f, 0.2383f, 0.3203f, 0.3906f, 0.4414f,
    0.5273f, 0.6133f, 0.6406f, 0.6914f, 0.7656f, 0.8047f, 0.0039f, 0.082f,
    0.168f, 0.25f, 0.3359f, 0.3828f, 0.457f, 0.5117f, 0.5859f, 0.6602f,
    0.7695f, 0.8438f, 0.918f
};

float V_Bold[256] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    0.8828f, 0.8828f, 0.8828f, 0.8828f, 0.8828f, 0.8828f, 0.8828f, 0.8828f,
    0.8828f, 0.8828f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.5313f, 0.5313f, 0.5313f, 0.5313f, 0.5313f, 0.5313f, 0.5313f,
    0.5313f, 0.5313f, 0.5313f, 0.5313f, 0.4141f, 0.4141f, 0.4141f, 0.4141f,
    0.4141f, 0.4141f, 0.4141f, 0.4141f, 0.4141f, 0.4141f, 0.2969f, 0.2969f,
    0.2969f, 0.2969f, 0.2969f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.7656f, 0.7656f, 0.7656f, 0.7656f, 0.7656f, 0.7656f, 0.7656f,
    0.7656f, 0.7656f, 0.7656f, 0.7656f, 0.7656f, 0.7656f, 0.6484f, 0.6484f,
    0.6484f, 0.6484f, 0.6484f, 0.6484f, 0.6484f, 0.6484f, 0.6484f, 0.6484f,
    0.6484f, 0.6484f, 0.6484f
};

float U_System[256] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.3867f, 0.0703f, 0.0f, 0.4453f, 0.5039f, 0.0f, 0.4219f,
    0.1367f, 0.168f, 0.2891f, 0.0f, 0.0469f, 0.1016f, 0.0156f, 0.3203f,
    0.543f, 0.5703f, 0.6016f, 0.6328f, 0.6641f, 0.6953f, 0.7266f, 0.7578f,
    0.7891f, 0.8203f, 0.2031f, 0.2344f, 0.0f, 0.2578f, 0.0f, 0.3516f,
    0.0f, 0.0078f, 0.0391f, 0.0703f, 0.1016f, 0.1328f, 0.1641f, 0.1953f,
    0.2266f, 0.2617f, 0.2891f, 0.3203f, 0.3516f, 0.3828f, 0.4141f, 0.4453f,
    0.4766f, 0.5078f, 0.5391f, 0.5703f, 0.6016f, 0.6328f, 0.6641f, 0.6953f,
    0.7266f, 0.7539f, 0.7891f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0078f, 0.0391f, 0.0703f, 0.1016f, 0.1328f, 0.1641f, 0.1953f,
    0.2266f, 0.2578f, 0.293f, 0.3203f, 0.3516f, 0.3828f, 0.4141f, 0.4453f,
    0.4766f, 0.5078f, 0.5391f, 0.5703f, 0.6016f, 0.6328f, 0.6641f, 0.6953f,
    0.7266f, 0.7578f, 0.7891f
};

float V_System[256] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.1641f, 0.1641f, 0.0f, 0.1641f, 0.1641f, 0.0f, 0.1641f,
    0.1641f, 0.1641f, 0.1641f, 0.0f, 0.1641f, 0.1641f, 0.1641f, 0.1641f,
    0.1641f, 0.1641f, 0.1641f, 0.1641f, 0.1641f, 0.1641f, 0.1641f, 0.1641f,
    0.1641f, 0.1641f, 0.1641f, 0.1641f, 0.0f, 0.1641f, 0.0f, 0.1641f,
    0.0f, 0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f,
    0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f,
    0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f, 0.0469f,
    0.0469f, 0.0469f, 0.0469f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f,
    0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f,
    0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f, 0.1055f,
    0.1055f, 0.1055f, 0.1055f
};

void jpb_DebugTextSetDraw3dHook(
    JPBDraw3dTextHook hook, void *user_data)
{
    jpb_draw3d_text_hook = hook;
    jpb_draw3d_text_user_data = user_data;
}

/* 0x43680, 671 bytes, global, 10 named locals
 * DebugString
 * PDB type: void (float, float*, float*, uns...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void DebugString(
    float start_x,
    float *x,
    float *y,
    unsigned color,
    char *string)
{
    signed char character;

    if (debugtext == NULL) {
        debugtext = _LoadTexture(
            (char *)resource_getPath(debugfont, JPB_RESOURCE_DEFAULT),
            TT_DEBUG,
            UINT32_C(0x02000001));
    }
    while ((character = (signed char)*string++) != 0) {
        float u;
        float v;

        if (character == 10 || character == 12) {
            *x = start_x;
            *y += 15.0f;
            continue;
        }
        if (character > 32) {
            u = U_System[character];
            v = V_System[character];
        } else {
            u = 0.8627451f;
            v = 0.058823526f;
        }

        _StartPoly(4, debugtext);
        _SetVert(0, *x, *y, 9.9e-05f, color, u, v);
        _SetVert(
            1, *x + 10.0f, *y, 9.9e-05f,
            color, u + 0.03137255f, v);
        _SetVert(
            2, *x, *y + 15.0f, 9.9e-05f,
            color, u, v - 0.047058824f);
        _SetVert(
            3, *x + 10.0f, *y + 15.0f, 9.9e-05f,
            color, u + 0.03137255f, v - 0.047058824f);
        _EndPoly();
        *x += 10.0f;
    }
}

/* 0x43920, 108 bytes, global, 8 named locals
 * DebugStringSize
 * PDB type: void (int*, int*, char*)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
static void debug_measure_shared_text(int *width, int *height)
{
    float line_width = 0.0f;
    float max_width = 0.0f;
    float total_height = 15.0f;
    const unsigned char *cursor = (const unsigned char *)text;

    while (*cursor != 0) {
        unsigned char shifted = (unsigned char)(*cursor - 10u);

        if ((shifted & 0xfdu) == 0) {
            line_width = 0.0f;
            total_height += 15.0f;
        } else {
            line_width = (float)((double)line_width + 10.0);
        }
        if (line_width > max_width) {
            max_width = line_width;
        }
        ++cursor;
    }
    *width = (int)max_width;
    *height = (int)total_height;
}

void DebugStringSize(int *width, int *height, char *ignored_text)
{
    (void)ignored_text;
    debug_measure_shared_text(width, height);
}

/* 0x43990, 126 bytes, global, 5 named locals
 * DebugText
 * PDB type: void (float, float*, float*, uns...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void DebugText(
    float start_x,
    float *x,
    float *y,
    unsigned color,
    char *format,
    ...)
{
    va_list arguments;

    va_start(arguments, format);
    (void)vsprintf(text, format, arguments);
    va_end(arguments);
    DebugString(start_x, x, y, color, text);
}

/* 0x43A10, 195 bytes, global, 8 named locals
 * DebugTextSize
 * PDB type: void (int*, int*, char*, <no typ...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void DebugTextSize(int *width, int *height, char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    (void)vsprintf(text, format, arguments);
    va_end(arguments);
    debug_measure_shared_text(width, height);
}

/* 0x43AE0, 192 bytes, global, 5 named locals
 * Draw2dBox
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
int Draw2dBox(
    float x, float y, float width, float height, long color)
{
    (void)DrawRectangle(x, y, width, 1.0f, color);
    (void)DrawRectangle(x, y + height, width, 1.0f, color);
    (void)DrawRectangle(x, y, 1.0f, height, color);
    return DrawRectangle(x + width, y, 1.0f, height, color);
}

/* 0x43BA0, 588 bytes, global, 18 named locals
 * Draw3dText
 * PDB type: void (float, float, float, float...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void Draw3dText(
    float x,
    float y,
    float z,
    float scale,
    uint32_t color,
    char *Format,
    ...)
{
    VECTOR screen_position;
    _svector position;
    FVECTOR float_screen_position;
    float maximum_width = 0.0f;
    float line_width = 0.0f;
    float text_height = scale * 30.0f;
    char *cursor;
    va_list arguments;

    if (OptionStruct.overlayMode == 0) {
        return;
    }
    va_start(arguments, Format);
    (void)vsprintf(text, Format, arguments);
    va_end(arguments);
    if (jpb_draw3d_text_hook != NULL) {
        jpb_draw3d_text_hook(
            x,
            y,
            z,
            scale,
            color,
            text,
            jpb_draw3d_text_user_data);
    }

    position.vx = (int16_t)(int)x;
    position.vy = (int16_t)(int)y;
    position.vz = (int16_t)(int)z;
    (void)fApplyMatrix(
        &CameraMatrix, &position, &screen_position);
    screen_position.vx = (int32_t)(
        (uint32_t)screen_position.vx +
        (uint32_t)CameraMatrix.t[0]);
    screen_position.vy = (int32_t)(
        (uint32_t)screen_position.vy +
        (uint32_t)CameraMatrix.t[1]);
    screen_position.vz = (int32_t)(
        (uint32_t)screen_position.vz +
        (uint32_t)CameraMatrix.t[2]);
    float_screen_position.vx = (float)screen_position.vx;
    float_screen_position.vy = (float)screen_position.vy;
    float_screen_position.vz = (float)screen_position.vz;
    if (float_screen_position.vz <= 460.0f) {
        return;
    }

    cursor = text;
    while (*cursor != '\0') {
        signed char character = (signed char)*cursor++;

        if (character == '\n') {
            line_width = 0.0f;
            text_height += scale * 30.0f;
        } else if (character == ' ') {
            line_width += scale * 12.0f;
        } else {
            line_width +=
                Size_Bold[character] * 255.0f * scale;
        }
        if (line_width > maximum_width) {
            maximum_width = line_width;
        }
    }
    (void)SDLTextWriteScale3D(
        color,
        0,
        float_screen_position.vx -
            (float)((int)maximum_width / 2),
        float_screen_position.vy - (float)(int)text_height,
        float_screen_position.vz,
        scale * 1.5f,
        2,
        text);
}

/* 0x43DF0, 362 bytes, global, 5 named locals
 * DrawRectangle
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
int DrawRectangle(
    float x, float y, float width, float height, long color)
{
    if (flatmat == NULL) {
        flatmat = _LoadTexture(NULL, TT_DEBUG, 1);
    }
    _StartPoly(4, flatmat);
    _SetVert(0, x, y, 9.68e-05f, (unsigned long)color, 0.0f, 0.0f);
    _SetVert(
        1, x + width + 1.0f, y, 9.68e-05f,
        (unsigned long)color, 0.0f, 0.0f);
    _SetVert(
        2, x, y + height + 1.0f, 9.68e-05f,
        (unsigned long)color, 0.0f, 0.0f);
    _SetVert(
        3, x + width + 1.0f, y + height + 1.0f, 9.68e-05f,
        (unsigned long)color, 0.0f, 0.0f);
    _EndPoly();
}

/* 0x43F60, 1323 bytes, global, 25 named locals
 * DrawString
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
static int debug_draw_string_poly(
    float x,
    float y,
    float z,
    float scale,
    unsigned long color,
    char *string,
    int no_scale)
{
    float start_x = x;
    float line_height = scale * 30.0f;
    unsigned char character;

    if (debugtext == NULL) {
        debugtext = _LoadTexture(
            (char *)resource_getPath(debugfont, JPB_RESOURCE_DEFAULT),
            TT_DEBUG,
            UINT32_C(0x02000001));
    }
    while ((character = (unsigned char)*string++) != 0) {
        float original_width;
        float draw_x;
        float draw_y;
        float draw_width;
        float draw_height;
        float u;
        float u_width;
        float v;
        float v_height;

        if (character == 10 || character == 12) {
            x = start_x;
            y += line_height;
            continue;
        }
        if (character == 32) {
            x += scale * 12.0f;
            continue;
        }
        if (character >= 128 && character <= 132) {
            x += (float)iDrawIcon(x, y, character, color, scale);
            continue;
        }

        original_width = Size_Bold[character] * 255.0f * scale;
        draw_x = x;
        draw_y = y;
        draw_width = original_width;
        draw_height = line_height;
        u = U_Bold[character];
        u_width = Size_Bold[character];
        v = V_Bold[character];
        v_height = 0.11764706f;
        if (textClip(
                &draw_x,
                &draw_y,
                &draw_width,
                &draw_height,
                &u,
                &u_width,
                &v,
                &v_height)) {
            _StartPoly(4, debugtext);
            _SetVert(0, draw_x, draw_y, z, color, u, v);
            _SetVert(
                1, draw_x + draw_width, draw_y, z,
                color, u + u_width, v);
            _SetVert(
                2, draw_x, draw_y + draw_height, z,
                color, u, v - v_height);
            _SetVert(
                3, draw_x + draw_width, draw_y + draw_height, z,
                color, u + u_width, v - v_height);
            if (no_scale) {
                _NoScaleEndPoly();
            } else {
                _EndPoly();
            }
        }
        x += original_width;
    }
    return (int)(x - start_x);
}

int DrawString(
    float x,
    float y,
    float z,
    float scale,
    unsigned long color,
    char *string)
{
    return debug_draw_string_poly(
        x, y, z, scale, color, string, 0);
}

/* 0x44490, 1560 bytes, global, 34 named locals
 * DrawString2D
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
int DrawString2D(
    float x,
    float y,
    float z,
    float scale,
    unsigned long color,
    char *format,
    ...)
{
    float start_x = x;
    float line_height = scale * 30.0f;
    unsigned char character;
    va_list arguments;

    va_start(arguments, format);
    (void)vsprintf(text, format, arguments);
    va_end(arguments);
    if (debugtext == NULL) {
        debugtext = _LoadTexture(
            (char *)resource_getPath(debugfont, JPB_RESOURCE_DEFAULT),
            TT_DEBUG,
            UINT32_C(0x02000001));
    }
    {
        char *cursor = text;

        while ((character = (unsigned char)*cursor++) != 0) {
            float original_width;
            float draw_x;
            float draw_y;
            float draw_width;
            float draw_height;
            float u;
            float u_width;
            float v;
            float v_height;

            if (character == 10 || character == 12) {
                x = start_x;
                y += line_height;
                continue;
            }
            if (character == 32) {
                x += scale * 12.0f;
                continue;
            }
            if (character >= 128 && character <= 132) {
                x += (float)iDrawIcon(x, y, character, color, scale);
                continue;
            }

            original_width = Size_Bold[character] * 255.0f * scale;
            draw_x = x;
            draw_y = y;
            draw_width = original_width;
            draw_height = line_height;
            u = U_Bold[character];
            u_width = Size_Bold[character];
            v = V_Bold[character];
            v_height = 0.11764706f;
            if (textClip(
                    &draw_x,
                    &draw_y,
                    &draw_width,
                    &draw_height,
                    &u,
                    &u_width,
                    &v,
                    &v_height)) {
                unsigned destination_scale_x =
                    OptionStruct.ScreenWidth / 640u;
                unsigned destination_scale_y =
                    OptionStruct.ScreenHeight / 480u;
                SCREENRECT destination;
                SCREENRECT source;
                CVECTOR draw_color;

                source.left = (int)roundf((float)debugtext->iw * u);
                source.top = (int)roundf((float)debugtext->ih * v);
                source.right = (int)roundf(
                    (float)debugtext->iw * (u + u_width));
                source.bottom = (int)roundf(
                    (float)debugtext->ih * (v - v_height));
                destination.left =
                    (int)draw_x * (int)destination_scale_x;
                destination.top =
                    (int)draw_y * (int)destination_scale_y;
                destination.right =
                    (int)(draw_x + draw_width) *
                    (int)destination_scale_x;
                destination.bottom =
                    (int)(draw_y + draw_height) *
                    (int)destination_scale_y;
                draw_color.r = (uint8_t)(color >> 16);
                draw_color.g = (uint8_t)(color >> 8);
                draw_color.b = (uint8_t)color;
                draw_color.cd = (uint8_t)(color >> 24);
                _DrawTexture(
                    debugtext,
                    destination,
                    &source,
                    draw_color,
                    z);
            }
            x += original_width;
        }
    }
    return (int)(x - start_x);
}

/* 0x44AB0, 1323 bytes, global, 25 named locals
 * DrawString3D
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
int DrawString3D(
    float x,
    float y,
    float z,
    float scale,
    unsigned long color,
    char *string)
{
    return debug_draw_string_poly(
        x, y, z, scale, color, string, 1);
}

/* 0x44FE0, 359 bytes, global, 5 named locals
 * DrawZRectangle
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
int DrawZRectangle(
    float x, float y, float width, float height, long color)
{
    if (flatmat == NULL) {
        flatmat = _LoadTexture(NULL, TT_DEBUG, 1);
    }
    _StartPoly(4, flatmat);
    _SetVert(0, x, y, frontZ, (unsigned long)color, 0.0f, 0.0f);
    _SetVert(
        1, x + width + 1.0f, y, frontZ,
        (unsigned long)color, 0.0f, 0.0f);
    _SetVert(
        2, x, y + height + 1.0f, frontZ,
        (unsigned long)color, 0.0f, 0.0f);
    _SetVert(
        3, x + width + 1.0f, y + height + 1.0f, frontZ,
        (unsigned long)color, 0.0f, 0.0f);
    _EndPoly();
}

/* 0x45150, 22 bytes, global, 0 named locals
 * FreeFont
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void FreeFont(void)
{
    if (debugtext != NULL) {
        debugtext = NULL;
    }
}

/* 0x45170, 159 bytes, global, 11 named locals
 * GetStringSize
 * PDB type: void (float, int*, int*, char*)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void GetStringSize(
    float scale, int *width, int *height, char *text)
{
    float line_width = 0.0f;
    float max_width = 0.0f;
    float line_height = scale * 30.0f;
    float total_height = line_height;
    unsigned char character;

    while ((character = (unsigned char)*text++) != 0) {
        if (character == '\n') {
            line_width = 0.0f;
            total_height += line_height;
        } else if (character == ' ') {
            line_width += 12.0f * scale;
        } else {
            line_width +=
                Size_Bold[(int8_t)character] * 255.0f * scale;
        }
        if (line_width > max_width) {
            max_width = line_width;
        }
    }
    *width = (int)max_width;
    *height = (int)total_height;
}

/* 0x45210, 254 bytes, global, 11 named locals
 * GetTextSize
 * PDB type: void (float, int*, int*, char*, ...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void GetTextSize(
    float scale, int *width, int *height, char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    (void)vsprintf(text, format, arguments);
    va_end(arguments);
    GetStringSize(scale, width, height, text);
}

/* 0x45310, 52 bytes, global, 1 named locals
 * InitFont
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void InitFont(void)
{
    debugtext = _LoadTexture(
        (char *)resource_getPath(debugfont, JPB_RESOURCE_DEFAULT),
        TT_DEBUG,
        UINT32_C(0x02000001));
}

/* 0x45350, 179 bytes, global, 6 named locals
 * _DrawText
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
int _DrawText(
    float x,
    float y,
    float z,
    float scale,
    uint32_t color,
    char *format,
    ...)
{
    va_list arguments;

    (void)z;
    va_start(arguments, format);
    (void)vsprintf(text, format, arguments);
    va_end(arguments);
    (void)SDLTextWriteScale(
        11,
        (int)(color >> 24),
        0,
        (int)x,
        (int)y,
        scale * 3.0f,
        2,
        "%s",
        text);
    return 0;
}

/* 0x45410, 11 bytes, global, 0 named locals
 * clearTextClip
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void clearTextClip(void)
{
    textClipping = 0;
}

/* 0x45420, 60 bytes, global, 1 named locals
 * console_loadfont
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45460, 362 bytes, global, 5 named locals
 * console_rectangle
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
int console_rectangle(
    float x, float y, float width, float height, long color)
{
    if (flatmat == NULL) {
        flatmat = _LoadTexture(NULL, TT_DEBUG, 1);
    }
    _StartPoly(4, flatmat);
    _SetVert(0, x, y, 9.36e-05f, (unsigned long)color, 0.0f, 0.0f);
    _SetVert(
        1, x + width + 1.0f, y, 9.36e-05f,
        (unsigned long)color, 0.0f, 0.0f);
    _SetVert(
        2, x, y + height + 1.0f, 9.36e-05f,
        (unsigned long)color, 0.0f, 0.0f);
    _SetVert(
        3, x + width + 1.0f, y + height + 1.0f, 9.36e-05f,
        (unsigned long)color, 0.0f, 0.0f);
    _EndPoly();
}

/* 0x455D0, 611 bytes, global, 9 named locals
 * console_text
 * PDB type: void (float, float, unsigned lon...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void console_text(
    float x, float y, unsigned long color, char *buffer)
{
    const char *font_path =
        resource_getPath("p_consle.tga", JPB_RESOURCE_DEFAULT);
    unsigned char character;

    if (font == NULL) {
        font = _LoadTexture((char *)font_path, TT_DEBUG, 2);
    }
    while ((character = (unsigned char)*buffer++) != 0) {
        unsigned glyph = (unsigned char)(character + 224u);
        float u;
        float v;

        if (glyph > 128u) {
            glyph = 128u;
        }
        u = (float)(glyph & 15u) * 8.0f * 0.0078125f;
        v = (float)(glyph >> 4) * 15.0f * 0.0078125f;
        _StartPoly(4, font);
        _SetVert(0, x, y, 8.72e-05f, color, u, v);
        _SetVert(
            1, x + 8.0f, y, 8.72e-05f,
            color, u + 0.0625f, v);
        _SetVert(
            2, x, y + 15.0f, 8.72e-05f,
            color, u, v + 0.1171875f);
        _SetVert(
            3, x + 8.0f, y + 15.0f, 8.72e-05f,
            color, u + 0.0625f, v + 0.1171875f);
        _EndPoly();
        x += 8.0f;
    }
}

/* 0x45840, 21 bytes, global, 0 named locals
 * debugReset
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void debugReset(void)
{
    cursorx = 20.0f;
    cursory = 40.0f;
}

/* 0x45860, 16 bytes, global, 1 named locals
 * scr_debugPrintf
 * PDB type: void (char*, <no type>)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void scr_debugPrintf(char *format, ...)
{
    (void)format;
}

/* 0x45870, 172 bytes, global, 2 named locals
 * scr_debugPrintfRed
 * PDB type: void (char*, <no type>)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void scr_debugPrintfRed(char *format, ...)
{
    char local_text[256];
    va_list arguments;

    va_start(arguments, format);
    (void)vsprintf(local_text, format, arguments);
    va_end(arguments);
    DebugText(
        20.0f,
        &cursorx,
        &cursory,
        UINT32_C(0xff80ff80),
        local_text);
}

/* 0x45920, 194 bytes, global, 6 named locals
 * scr_debugPrintfXY
 * PDB type: void (int, int, char*, <no type>...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void scr_debugPrintfXY(int x, int y, char *format, ...)
{
    char local_text[256];
    float x_position = (float)x;
    float y_position = (float)y;
    va_list arguments;

    va_start(arguments, format);
    (void)vsprintf(local_text, format, arguments);
    va_end(arguments);
    DebugText(
        (float)x,
        &x_position,
        &y_position,
        UINT32_C(0xff30e030),
        local_text);
}

/* 0x459F0, 192 bytes, global, 7 named locals
 * scr_debugPrintfXYC
 * PDB type: void (int, int, long, char*, <no...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void scr_debugPrintfXYC(
    int x, int y, long color, char *format, ...)
{
    char local_text[256];
    float x_position = (float)x;
    float y_position = (float)y;
    va_list arguments;

    va_start(arguments, format);
    (void)vsprintf(local_text, format, arguments);
    va_end(arguments);
    DebugText(
        (float)x,
        &x_position,
        &y_position,
        (unsigned)color,
        local_text);
}

/* 0x45AB0, 194 bytes, global, 6 named locals
 * scr_debugPrintfXYRed
 * PDB type: void (int, int, char*, <no type>...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void scr_debugPrintfXYRed(int x, int y, char *format, ...)
{
    char local_text[256];
    float x_position = (float)x;
    float y_position = (float)y;
    va_list arguments;

    va_start(arguments, format);
    (void)vsprintf(local_text, format, arguments);
    va_end(arguments);
    DebugText(
        (float)x,
        &x_position,
        &y_position,
        UINT32_C(0xff80e080),
        local_text);
}

/* 0x45B80, 441 bytes, global, 15 named locals
 * scr_debugPrintfXYZ
 * PDB type: void (int, int, int, char*, <no ...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void console_loadfont(void)
{
    char *path = (char *)resource_getPath(
        "p_consle.tga", JPB_RESOURCE_DEFAULT);

    if (font == NULL) {
        font = _LoadTexture(path, TT_DEBUG, 2);
    }
}
void scr_debugPrintfXYZ(
    int x,
    int y,
    int z,
    char *format,
    ...)
{
    _svector position;
    VECTOR transformed;
    FVECTOR screen;
    int text_width;
    int text_height;
    int32_t translated_x;
    int32_t translated_y;
    int32_t translated_z;
    float projection;
    va_list arguments;

    va_start(arguments, format);
    (void)vsprintf(text, format, arguments);
    va_end(arguments);
    debug_measure_shared_text(&text_width, &text_height);

    position.vx = (int16_t)x;
    position.vy = (int16_t)y;
    position.vz = (int16_t)z;
    position.pad = 0;
    (void)fApplyMatrix(&CameraMatrix, &position, &transformed);
    translated_x = (int32_t)(
        (uint32_t)transformed.vx + (uint32_t)CameraMatrix.t[0]);
    translated_y = (int32_t)(
        (uint32_t)transformed.vy + (uint32_t)CameraMatrix.t[1]);
    translated_z = (int32_t)(
        (uint32_t)transformed.vz + (uint32_t)CameraMatrix.t[2]);
    screen.vz = (float)translated_z;
    if (screen.vz <= 1.0f) {
        screen.vz = 1.0f;
    }
    projection = 460.0f / screen.vz;
    screen.vx =
        (float)translated_x * projection + 320.0f -
        (float)text_width * 0.5f;
    screen.vy =
        (float)translated_y * projection + 240.0f -
        (float)text_height * 0.5f;
    screen.vz /= 10240.0f;
    DebugString(
        screen.vx,
        &screen.vx,
        &screen.vy,
        UINT32_C(0xff8090a0),
        text);
}

/* 0x45D40, 161 bytes, global, 7 named locals
 * scr_debugPrintfXYZRed
 * PDB type: void (int, int, int, char*, <no ...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void scr_debugPrintfXYZRed(
    int x,
    int y,
    int z,
    char *format,
    ...)
{
    _svector position;
    FVECTOR screen;
    char buffer[128];
    va_list arguments;

    position.vx = (int16_t)x;
    position.vy = (int16_t)y;
    position.vz = (int16_t)z;
    position.pad = 0;
    va_start(arguments, format);
    (void)sprintf(buffer, format, arguments);
    va_end(arguments);
    (void)PerspectiveTransform(&CameraMatrix, &position, &screen);
    DebugText(
        screen.vx,
        &screen.vx,
        &screen.vy,
        UINT32_C(0xffa04030),
        buffer);
}

/* 0x45DF0, 86 bytes, global, 4 named locals
 * setTextClip
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void setTextClip(
    unsigned x, unsigned y, unsigned width, unsigned height)
{
    textClipping = 1;
    textclipRect[0] = (float)(uint64_t)x;
    textclipRect[1] = (float)(uint64_t)y;
    textclipRect[2] = (float)(uint64_t)(x + width);
    textclipRect[3] = (float)(uint64_t)(y + height);
}

/* 0x45E50, 69 bytes, global, 4 named locals
 * setTextClipF
 * PDB type: void (float*, float*, float*, fl...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void setTextClipF(
    float *x, float *y, float *width, float *height)
{
    textclipRect[0] = *x;
    textclipRect[1] = *y;
    textclipRect[2] = *x + *width;
    textclipRect[3] = *y + *height;
    textClipping = 1;
}

/* 0x45EA0, 61 bytes, global, 4 named locals
 * setTextClipFAllSides
 * PDB type: void (float*, float*, float*, fl...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void setTextClipFAllSides(
    float *left, float *top, float *right, float *bottom)
{
    textclipRect[0] = *left;
    textclipRect[1] = *top;
    textclipRect[2] = *right;
    textclipRect[3] = *bottom;
    textClipping = 1;
}

/* 0x45EE0, 526 bytes, global, 12 named locals
 * textClip
 * PDB type: int (float*, float*, float*, flo...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
int textClip(
    float *x,
    float *y,
    float *width,
    float *height,
    float *u,
    float *u_width,
    float *v,
    float *v_height)
{
    float right_state = *x + *width;
    float bottom_state = *y + *height;

    if (textClipping == 0) {
        return 1;
    }
    if (textclipRect[0] >= right_state ||
        textclipRect[1] >= bottom_state ||
        *x >= textclipRect[2] ||
        *y >= textclipRect[3]) {
        return 0;
    }
    if (*x >= textclipRect[0] &&
        textclipRect[2] >= right_state &&
        *y >= textclipRect[1] &&
        textclipRect[3] >= bottom_state) {
        return 1;
    }

    if (textclipRect[0] >= *x && right_state >= textclipRect[0]) {
        float clipped = textclipRect[0] - *x;
        float original_width;
        float source_clipped;

        right_state -= *x;
        original_width = right_state;
        *width -= clipped;
        source_clipped = *u_width / original_width * clipped;
        *x = textclipRect[0];
        *u += source_clipped;
        *u_width -= source_clipped;
    }
    else if (textclipRect[2] > *x && right_state > textclipRect[2]) {
        float clipped = right_state - textclipRect[2];
        float original_width = right_state - *x;
        float source_clipped = *u_width / original_width * clipped;

        *width -= clipped;
        *u_width -= source_clipped;
    }
    if (textclipRect[1] >= *y && bottom_state >= textclipRect[1]) {
        float clipped = textclipRect[1] - *y;
        float original_height;
        float source_clipped;

        bottom_state -= *y;
        original_height = bottom_state;
        *height -= clipped;
        source_clipped = *v_height / original_height * clipped;
        *y = textclipRect[1];
        *v -= source_clipped;
        *v_height -= source_clipped;
    }
    else if (textclipRect[3] > *y && bottom_state > textclipRect[3]) {
        float clipped = bottom_state - textclipRect[3];
        float original_height = bottom_state - *y;
        float source_clipped = *v_height / original_height * clipped;

        *height -= clipped;
        *v_height -= source_clipped;
    }
    return 1;
}
