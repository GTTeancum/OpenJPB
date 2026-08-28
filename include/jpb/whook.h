#ifndef JPB_WHOOK_H
#define JPB_WHOOK_H

#include "jpb/fmath.h"
#include "jpb/game.h"
#include "jpb/material.h"
#include "jpb/sdl_abi.h"
#include "jpb/theoraplay.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact matched-PC PDB type 0x69A2. */
typedef struct SCREENRECT {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} SCREENRECT;

/*
 * Portable realization seam for the original platform renderer's textured
 * screen rectangle. Gameplay retains exact _DrawTexture; each platform owns
 * how the queued rectangle is presented.
 */
typedef void (*JPBDrawTextureHook)(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth);

typedef void (*JPBDrawTextureClippedHook)(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth,
    const SCREENRECT *scissor);

typedef void (*JPBDrawUITextUTF16Hook)(
    void *user_data,
    const uint16_t *text,
    const SCREENRECT *destination,
    int font_style,
    int point_size,
    CVECTOR color,
    int depth_enabled,
    float depth);

typedef void (*JPBDrawUITextUTF163DHook)(
    void *user_data,
    const uint16_t *text,
    float x,
    float y,
    float z,
    int font_style,
    int point_size,
    uint32_t color);

/*
 * Dependency-light realization seam for exact PDB procedure
 * debug_drawsphere. The matched release body performs no visible draw, so
 * PC and later platform renderers may consume the authored debug primitive
 * without coupling gameplay to a graphics API.
 */
typedef void (*JPBDebugSphereHook)(
    void *user_data,
    int32_t x,
    int32_t y,
    int32_t z,
    int32_t radius,
    uint32_t color);

typedef void (*JPBClearWindowHook)(void *user_data);
typedef void (*JPBRenderLoadHook)(void *user_data);
typedef void (*JPBGetWindowSizeHook)(
    int *width, int *height, void *user_data);

/*
 * Exact _SetVert payload retained at the platform boundary. The original
 * renderer builder owns the same x/y/z, diffuse-color, and UV fields; this
 * fixed record lets dependency-light renderers consume completed polygons.
 */
typedef struct JPBScreenPolyVertex {
    float x;
    float y;
    float z;
    uint32_t argb;
    float tu;
    float tv;
} JPBScreenPolyVertex;

/* Exact matched-PC PDB type FRONTENDVERT (32 bytes). */
typedef struct FRONTENDVERT {
    float x;
    float y;
    float u;
    float v;
    int r;
    int g;
    int b;
    int color;
} FRONTENDVERT;

enum { JPB_SCREEN_POLY_VERTEX_CAPACITY = 4 };

/*
 * Portable mirror of the two constant-buffer publications made by exact
 * PDB procedure _ApplyLevelTransformation.
 */
typedef struct JPBLevelTransformation {
    float world[4][4];
    float scale[4];
} JPBLevelTransformation;

typedef void (*JPBScreenPolyHook)(
    void *user_data,
    _Material *material,
    uint32_t material_flags,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale);

typedef struct ufbx_scene ufbx_scene;
typedef void (*JPBInitFBXLevelDataHook)(
    void *user_data, ufbx_scene *scene);

void jpb_WHookSetDrawTextureHook(
    JPBDrawTextureHook hook, void *user_data);
void jpb_WHookSetDrawTextureClippedHook(
    JPBDrawTextureClippedHook hook, void *user_data);
void jpb_WHookSetDrawUITextUTF16Hook(
    JPBDrawUITextUTF16Hook hook, void *user_data);
void jpb_WHookSetDrawUITextUTF163DHook(
    JPBDrawUITextUTF163DHook hook, void *user_data);
void jpb_WHookSetDebugSphereHook(
    JPBDebugSphereHook hook, void *user_data);
void jpb_WHookSetClearWindowHook(
    JPBClearWindowHook hook, void *user_data);
void jpb_WHookSetRenderLoadHook(
    JPBRenderLoadHook hook, void *user_data);
void jpb_WHookSetGetWindowSizeHook(
    JPBGetWindowSizeHook hook, void *user_data);
void jpb_WHookSetScreenPolyHook(
    JPBScreenPolyHook hook, void *user_data);
void jpb_WHookSetInitFBXLevelDataHook(
    JPBInitFBXLevelDataHook hook, void *user_data);
void CleanupLevelData(void);
void ClearWindow(void);
void GetWindowSize(int *width, int *height);
void PresentWindow(void);
void __RenderLoad(int endframe);
void WaitVBlank(void);
void RenderUIText(int x, int y, int width, int height, SDL_Surface *surface);
void RenderUITexture(
    _Material *material,
    SDL_Rect destination,
    SDL_Rect source,
    uint8_t alpha,
    int red,
    int green,
    int blue,
    int flip);
#if defined(JPB_WHOOK_TESTING)
typedef struct JPBWHookSDLTestHooks {
    void *(*create_texture_from_surface)(void *renderer, void *surface);
    int (*render_copy)(
        void *renderer,
        void *texture,
        const void *source,
        const void *destination);
    void (*destroy_texture)(void *texture);
    void (*free_surface)(void *surface);
    int (*render_set_clip_rect)(void *renderer, const void *rectangle);
    int (*set_texture_color_mod)(
        void *texture, uint8_t red, uint8_t green, uint8_t blue);
    int (*set_texture_alpha_mod)(void *texture, uint8_t alpha);
    int (*set_texture_blend_mode)(void *texture, int blend_mode);
    int (*render_copy_ex)(
        void *renderer,
        void *texture,
        const void *source,
        const void *destination,
        double angle,
        const void *center,
        int flip);
    char *(*get_base_path)(void);
} JPBWHookSDLTestHooks;

typedef struct JPBWHookWinMainTestHooks {
    void (*update_menus)(void);
    void (*load_options_data)(void);
    void (*set_width_height)(uint32_t width, uint32_t height);
    int (*steam_api_init)(void);
    int32_t (*create_application)(void *instance, char *command_line);
    int (*steam_api_restart_app_if_necessary)(uint32_t app_id);
    void *(*create_achievements)(void *achievements, int count);
    void *(*create_game_manager)(void);
    void *(*create_rich_presence)(void);
    int (*is_steam_running_on_steam_deck)(void);
    void (*initialize_main)(void);
    void (*steam_api_shutdown)(void);
    void (*destroy_achievements)(void *achievements);
    void (*report_create_error)(
        uint32_t result,
        char *message,
        uint32_t line,
        char *file);
    void (*print_fatal_error)(const char *message);
} JPBWHookWinMainTestHooks;

void jpb_WHookSetSDLTestHooks(const JPBWHookSDLTestHooks *hooks);
void jpb_WHookSetWinMainTestHooks(
    const JPBWHookWinMainTestHooks *hooks);
void jpb_WHookVideoDestinationForTest(
    uint32_t screen_width,
    uint32_t screen_height,
    SCREENRECT *destination);
void jpb_WHookText3DQuadForTest(
    uint32_t left,
    uint32_t top,
    uint32_t right,
    uint32_t bottom,
    float atlas_width,
    float atlas_height,
    float scale,
    float maximum_height,
    float x,
    float y,
    float z,
    uint32_t color,
    JPBScreenPolyVertex vertices[4],
    float *next_x);
float jpb_WHookText3DLineAdvanceForTest(
    float y, float maximum_height);
typedef struct JPBWHookText2DDrawTest {
    SCREENRECT source;
    SCREENRECT destination;
    float color[4];
    float depth;
    SCREENRECT scissor;
    int has_scissor;
    int sampler_type;
} JPBWHookText2DDrawTest;
void jpb_WHookText2DDrawForTest(
    int minimum_x,
    int maximum_x,
    int maximum_y,
    uint32_t left,
    uint32_t top,
    uint32_t right,
    uint32_t bottom,
    int maximum_height,
    int x,
    int y,
    CVECTOR color,
    int clipping,
    SCREENRECT scissor,
    int depth_enabled,
    float depth,
    JPBWHookText2DDrawTest *draw,
    int *next_x);
int jpb_WHookTextControllerIconForTest(uint16_t character);
int jpb_WHookTextTagIconForTest(uint16_t character, int *alpha);
void jpb_WHookSetAudioQueueForTest(AudioQueue *head, AudioQueue *tail);
AudioQueue *jpb_WHookAudioQueueForTest(void);
AudioQueue *jpb_WHookAudioQueueTailForTest(void);
void jpb_WHookAudioCallbackForTest(
    void *userdata, unsigned char *stream, int length);
const char *jpb_WHookLocalizedVideoPathForTest(const char *path);
#endif
void LoadGameData(void);
void LoadOptionsData(void);
void SaveGameData(void);
void SaveSettingsData(optionstruct options);
extern int refreshFontAtlasFlag;
extern uint8_t resolutionUpdated;
extern int32_t newWidth;
extern int32_t newHeight;
extern int32_t newWindowMode;
extern _Material *whitemat;
extern _Material *whitematAdd;
void MarkFontAtlasForRefresh(void);
void RefreshFontAtlas(void);
void UpdateResolution(int width, int height, int window_mode);
void SetInMenu(int in_menu);
void clearzerobss(void);
void initXAstuff(void);
void _StoreDescriptorHeapOffsetsEnd(void);
void _StoreDescriptorHeapOffsetsStart(void);
void texture_GarbageCollect(void);
void whook_RestoreTextures(void);
bool IsNullTerminated(const char *text);
char *ModifyFilename(const char *filename);
void SetFilenameExtension(
    char *source, char *destination, char *extension);
void UpdateValidResolutions(void);
void OutputTextXY(int x, int y, char *format, ...);
void __PCTrace(char *format, ...);
void dbgprintf(char *format, ...);
int debug_printf1(char *format, ...);
char *GetAchNameFromIndex(int index);
const JPBLevelTransformation *jpb_WHookLevelTransformation(void);
void _ApplyLevelTransformation(
    MATRIX *matrix, float x_scale, float y_scale, float z_scale);
void _ApplyProjection(FVECTOR *vertices);
void _DrawTexture(
    _Material *texture,
    SCREENRECT destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth);
void _DrawTextureClipped(
    _Material *texture,
    SCREENRECT destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth,
    SCREENRECT scissor);
void _DrawUITextUTF16(
    uint16_t *text,
    SCREENRECT destination,
    int font_style,
    int point_size,
    CVECTOR color);
void _DrawUITextUTF16Depth(
    uint16_t *text,
    SCREENRECT destination,
    int font_style,
    int point_size,
    CVECTOR color,
    float depth);
void _DrawUITextUTF16_3D(
    uint16_t *text,
    float x,
    float y,
    float z,
    int font_style,
    int point_size,
    uint32_t color);
void debug_drawsphere(
    int x, int y, int z, int radius, uint32_t color);
void debug_box(_svector *top_left, _svector *bottom_right, uint32_t color);
void debug_drawline(
    int x1,
    int y1,
    int z1,
    int x2,
    int y2,
    int z2,
    uint32_t color);
void debug_drawpoint(
    int x, int y, int z, int width, int height, uint32_t color);
void debug_drawpoint2d(
    int x, int y, int width, int height, uint32_t color);
void debug_vectoroffset(
    int x1,
    int y1,
    int z1,
    int x2,
    int y2,
    int z2,
    int shift,
    uint32_t color);
void _EndPoly(void);
void _InitFBXLevelData(ufbx_scene *scene);
void _NoScaleEndPoly(void);
void _SetVert(
    int vertex,
    float x,
    float y,
    float z,
    unsigned long argb,
    float tu,
    float tv);
void _StartPoly(int vertex_count, _Material *material);
void frontEndPoly(
    _Material *material,
    int vertex_count,
    FRONTENDVERT *vertices,
    float depth);
int cliptoscreen(short *position);
int getDefaultResolutionIndex(void);
int KeyPressed(int key);
int KeyHeld(int key);
int KeyReleased(int key);
int LastKey(void);
int ShiftKeyDown(void);
int CtrlKeyDown(void);
void SDL_ResetClipRect(void);
void SDL_SetClip(int x, int y, int width, int height);
void jpb_WHookHandleKeyEvent(
    int virtual_key, int scan_code, int pressed);
void jpb_WHookEndInputFrame(void);
void jpb_WHookClearKeyState(void);
int seecull(FVECTOR4 *point, FVECTOR4 *planes);

#if defined(__cplusplus)
#define JPB_WHOOK_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_WHOOK_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_WHOOK_STATIC_ASSERT(
    sizeof(SCREENRECT) == 16,
    "SCREENRECT must match PDB type 0x69A2");
JPB_WHOOK_STATIC_ASSERT(
    offsetof(SCREENRECT, bottom) == 12,
    "SCREENRECT.bottom layout changed");
JPB_WHOOK_STATIC_ASSERT(
    sizeof(JPBScreenPolyVertex) == 24,
    "JPBScreenPolyVertex layout changed");

#undef JPB_WHOOK_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
