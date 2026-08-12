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

/*
 * Inferred dependency-free PC renderer seam. Draw3dText retains its exact
 * PDB-facing API; the selected host renderer receives already formatted text.
 */
void jpb_DebugTextSetDraw3dHook(
    JPBDraw3dTextHook hook, void *user_data);
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
