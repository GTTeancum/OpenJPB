#ifndef JPB_DEBUGTEXT_H
#define JPB_DEBUGTEXT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*JPBDraw3dTextHook)(
    float x,
    float y,
    float z,
    float scale,
    uint32_t color,
    const char *text,
    void *user_data);

extern float Size_Bold[256];
extern float U_Bold[256];
extern float V_Bold[256];
extern float U_System[256];
extern float V_System[256];
extern unsigned textClipping;
extern float textclipRect[4];

/* Passive observation seam; Draw3dText always executes its canonical path. */
void jpb_DebugTextSetDraw3dHook(
    JPBDraw3dTextHook hook, void *user_data);
void console_loadfont(void);
void Draw3dText(
    float x,
    float y,
    float z,
    float scale,
    uint32_t color,
    char *Format,
    ...);
void scr_debugPrintfXYZ(
    int x,
    int y,
    int z,
    char *format,
    ...);
void GetStringSize(
    float scale, int *width, int *height, char *text);
void GetTextSize(
    float scale, int *width, int *height, char *format, ...);
void DebugStringSize(int *width, int *height, char *ignored_text);
void DebugString(
    float start_x,
    float *x,
    float *y,
    unsigned color,
    char *string);
void DebugText(
    float start_x,
    float *x,
    float *y,
    unsigned color,
    char *format,
    ...);
void DebugTextSize(int *width, int *height, char *format, ...);
int Draw2dBox(
    float x, float y, float width, float height, long color);
int DrawRectangle(
    float x, float y, float width, float height, long color);
int DrawString(
    float x,
    float y,
    float z,
    float scale,
    unsigned long color,
    char *text);
int DrawString2D(
    float x,
    float y,
    float z,
    float scale,
    unsigned long color,
    char *format,
    ...);
int DrawString3D(
    float x,
    float y,
    float z,
    float scale,
    unsigned long color,
    char *text);
int DrawZRectangle(
    float x, float y, float width, float height, long color);
int console_rectangle(
    float x, float y, float width, float height, long color);
void console_text(
    float x, float y, unsigned long color, char *buffer);
void InitFont(void);
void FreeFont(void);
void clearTextClip(void);
void debugReset(void);
void scr_debugPrintf(char *format, ...);
void scr_debugPrintfRed(char *format, ...);
void scr_debugPrintfXY(int x, int y, char *format, ...);
void scr_debugPrintfXYC(
    int x, int y, long color, char *format, ...);
void scr_debugPrintfXYRed(int x, int y, char *format, ...);
void scr_debugPrintfXYZ(
    int x, int y, int z, char *format, ...);
void scr_debugPrintfXYZRed(
    int x, int y, int z, char *format, ...);
void setTextClip(
    unsigned x, unsigned y, unsigned width, unsigned height);
void setTextClipF(
    float *x, float *y, float *width, float *height);
void setTextClipFAllSides(
    float *left, float *top, float *right, float *bottom);
int textClip(
    float *x,
    float *y,
    float *width,
    float *height,
    float *u,
    float *u_width,
    float *v,
    float *v_height);
int _DrawText(
    float x,
    float y,
    float z,
    float scale,
    uint32_t color,
    char *format,
    ...);
int debug_printf(char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
