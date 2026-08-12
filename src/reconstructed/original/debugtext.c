/*
 * GENERATED RECONSTRUCTION SHELL - no function bodies recovered here.
 * PDB module: 0029
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\debugtext.obj
 * Primary source: W:\SWJediPowerBattles\work\debugtext.c
 * Compiler language: c
 * Emitted procedures: 32
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/debugtext.h"
#include "jpb/text.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

static JPBDraw3dTextHook jpb_draw3d_text_hook;
static void *jpb_draw3d_text_user_data;

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

/* 0x43920, 108 bytes, global, 8 named locals
 * DebugStringSize
 * PDB type: void (int*, int*, char*)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x43990, 126 bytes, global, 5 named locals
 * DebugText
 * PDB type: void (float, float*, float*, uns...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x43A10, 195 bytes, global, 8 named locals
 * DebugTextSize
 * PDB type: void (int*, int*, char*, <no typ...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x43AE0, 192 bytes, global, 5 named locals
 * Draw2dBox
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x43BA0, 588 bytes, global, 18 named locals
 * Draw3dText
 * PDB type: void (float, float, float, float...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 *
 * Partial renderer boundary: formatting and the exact public call surface are
 * live; projection/font emission is supplied by the current PC backend hook.
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
    char text[256];
    va_list args;

    if (jpb_draw3d_text_hook == NULL || Format == NULL) {
        return;
    }
    va_start(args, Format);
    (void)vsnprintf(text, sizeof(text), Format, args);
    va_end(args);
    jpb_draw3d_text_hook(
        x,
        y,
        z,
        scale,
        color,
        text,
        jpb_draw3d_text_user_data);
}

/* 0x43DF0, 362 bytes, global, 5 named locals
 * DrawRectangle
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x43F60, 1323 bytes, global, 25 named locals
 * DrawString
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x44490, 1560 bytes, global, 34 named locals
 * DrawString2D
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x44AB0, 1323 bytes, global, 25 named locals
 * DrawString3D
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x44FE0, 359 bytes, global, 5 named locals
 * DrawZRectangle
 * PDB type: int (float, float, float, float,...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45150, 22 bytes, global, 0 named locals
 * FreeFont
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45170, 159 bytes, global, 11 named locals
 * GetStringSize
 * PDB type: void (float, int*, int*, char*)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45210, 254 bytes, global, 11 named locals
 * GetTextSize
 * PDB type: void (float, int*, int*, char*, ...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45310, 52 bytes, global, 1 named locals
 * InitFont
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

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
    char formatted[256];
    wchar_t wide[256];
    va_list arguments;

    (void)z;
    if (format == NULL) {
        return 0;
    }
    va_start(arguments, format);
    (void)vsnprintf(
        formatted, sizeof(formatted), format, arguments);
    va_end(arguments);
    formatted[sizeof(formatted) - 1] = '\0';
    if (mbstowcs(
            wide,
            formatted,
            sizeof(wide) / sizeof(wide[0])) == (size_t)-1) {
        size_t index;

        for (index = 0;
             index + 1 < sizeof(wide) / sizeof(wide[0]) &&
             formatted[index] != '\0';
             ++index) {
            wide[index] = (unsigned char)formatted[index];
        }
        wide[index] = L'\0';
    } else {
        wide[sizeof(wide) / sizeof(wide[0]) - 1] = L'\0';
    }
    (void)SDLTextWriteScale(
        11,
        (int)(color >> 24),
        0,
        (int)x,
        (int)y,
        scale * 3.0f,
        2,
        L"%ls",
        wide);
    return 0;
}

/* 0x45410, 11 bytes, global, 0 named locals
 * clearTextClip
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

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

/* 0x455D0, 611 bytes, global, 9 named locals
 * console_text
 * PDB type: void (float, float, unsigned lon...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45840, 21 bytes, global, 0 named locals
 * debugReset
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45860, 16 bytes, global, 1 named locals
 * scr_debugPrintf
 * PDB type: void (char*, <no type>)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45870, 172 bytes, global, 2 named locals
 * scr_debugPrintfRed
 * PDB type: void (char*, <no type>)
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45920, 194 bytes, global, 6 named locals
 * scr_debugPrintfXY
 * PDB type: void (int, int, char*, <no type>...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x459F0, 192 bytes, global, 7 named locals
 * scr_debugPrintfXYC
 * PDB type: void (int, int, long, char*, <no...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45AB0, 194 bytes, global, 6 named locals
 * scr_debugPrintfXYRed
 * PDB type: void (int, int, char*, <no type>...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45B80, 441 bytes, global, 15 named locals
 * scr_debugPrintfXYZ
 * PDB type: void (int, int, int, char*, <no ...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
void scr_debugPrintfXYZ(
    int x,
    int y,
    int z,
    char *format,
    ...)
{
    char text[256];
    va_list arguments;

    if (format == NULL) {
        return;
    }
    va_start(arguments, format);
    (void)vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);
    text[sizeof(text) - 1] = '\0';
    Draw3dText(
        (float)x,
        (float)y,
        (float)z,
        1.0f,
        UINT32_C(0xff8090a0),
        "%s",
        text);
}

/* 0x45D40, 161 bytes, global, 7 named locals
 * scr_debugPrintfXYZRed
 * PDB type: void (int, int, int, char*, <no ...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45DF0, 86 bytes, global, 4 named locals
 * setTextClip
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45E50, 69 bytes, global, 4 named locals
 * setTextClipF
 * PDB type: void (float*, float*, float*, fl...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45EA0, 61 bytes, global, 4 named locals
 * setTextClipFAllSides
 * PDB type: void (float*, float*, float*, fl...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */

/* 0x45EE0, 526 bytes, global, 12 named locals
 * textClip
 * PDB type: int (float*, float*, float*, flo...
 * Source: W:\SWJediPowerBattles\work\debugtext.c
 */
