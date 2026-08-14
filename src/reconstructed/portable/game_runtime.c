/*
 * Portable live-scene integration loop.
 *
 * This is the portable PC gameplay-field integration loop. It loads original
 * JPX geometry, consumes the recovered game-owned pad state, runs the exact
 * recovered player controller when authored motion data is present, advances
 * the reviewed physics actor path, constructs the exact gameplay
 * world-to-screen matrix, and draws to a caller-owned software framebuffer.
 */

#include "jpb/game_runtime.h"
#include "jpb/ai.h"
#include "jpb/alloc.h"
#include "jpb/alltext.h"
#include "jpb/animctrl.h"
#include "jpb/brain.h"
#include "jpb/brainutl.h"
#include "jpb/boss.h"
#include "jpb/bullet.h"
#include "jpb/combo.h"
#include "jpb/cube.h"
#include "jpb/debugtext.h"
#include "jpb/enemy.h"
#include "jpb/effects.h"
#include "jpb/filesys.h"
#include "jpb/fx.h"
#include "jpb/game.h"
#include "jpb/generic_hook.h"
#include "jpb/globalarrays.h"
#include "jpb/input.h"
#include "jpb/intersec.h"
#include "jpb/io.h"
#include "jpb/jedi.h"
#include "jpb/jonny.h"
#include "jpb/level.h"
#include "jpb/level_world.h"
#include "jpb/loader.h"
#include "jpb/memory.h"
#include "jpb/menu.h"
#include "jpb/portable_text.h"
#include "jpb/pwrup.h"
#include "jpb/resources.h"
#include "jpb/settings.h"
#include "jpb/shaolin.h"
#include "jpb/sprite.h"
#include "jpb/tga.h"
#include "jpb/text.h"
#include "jpb/texture.h"
#include "jpb/whook.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double game_runtime_wall_seconds(void)
{
    struct timespec now;

    timespec_get(&now, TIME_UTC);
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
}

enum {
    JPB_GAME_RUNTIME_PATH_CAPACITY = 1024,
    JPB_GAME_RUNTIME_MODEL_TEXTURE_CAPACITY =
        JPB_BMD_NODE_CAPACITY,
    JPB_GAME_RUNTIME_ENEMY_CLASS_CAPACITY =
        JPB_ACTOR_NAME_COUNT
};

static const char *game_runtime_last_failure_stage = "none";

static void game_runtime_set_failure_stage(const char *stage)
{
    game_runtime_last_failure_stage = stage != NULL ? stage : "unknown";
}

const char *jpb_GameRuntimeLastFailureStage(void)
{
    return game_runtime_last_failure_stage;
}

static int game_runtime_fail(
    JPBGameRuntime *runtime, const char *stage, int result)
{
    game_runtime_set_failure_stage(stage);
    if (runtime != NULL) {
        jpb_GameRuntimeShutdown(runtime);
    }
    return result;
}

typedef struct JPBGameRuntimeTexture {
    char name[sizeof(((geomData *)0)->t.Texture)];
    uint8_t *fileData;
    uint32_t *pixels;
    JPBSoftwareTexture texture;
} JPBGameRuntimeTexture;

static void game_runtime_capture_draw_texture(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth)
{
    JPBGameRuntime *runtime =
        (JPBGameRuntime *)user_data;
    JPBGameRuntimeScreenDraw *draw;

    if (runtime == NULL || destination == NULL) {
        return;
    }
    if (runtime->screenDrawCount >=
        JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY) {
        ++runtime->screenDrawDroppedCount;
        return;
    }
    draw = &runtime->screenDraws[
        runtime->screenDrawCount++];
    draw->order = runtime->drawOrder++;
    draw->texture = texture;
    draw->destination = *destination;
    draw->textureWidth = texture != NULL ? texture->iw : 0;
    draw->textureHeight = texture != NULL ? texture->ih : 0;
    draw->color = color;
    draw->layerDepth = layer_depth;
    draw->hasSource = source != NULL;
    draw->isPlayerHudTile = 0;
    if (source != NULL) {
        draw->source = *source;
    } else {
        memset(&draw->source, 0, sizeof(draw->source));
    }
}

static void game_runtime_capture_player_tile(
    void *user_data,
    const FVECTOR *position,
    float width,
    float height,
    uint32_t color,
    float projection_depth)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    JPBGameRuntimeScreenDraw *draw;
    float depth;
    float scale_x;
    float scale_y;

    if (runtime == NULL || position == NULL ||
        OptionStruct.ScreenWidth == 0 ||
        OptionStruct.ScreenHeight == 0) {
        return;
    }
    depth = position->vz >= 1.0f
        ? position->vz
        : projection_depth;
    if (!(depth >= 1.0f)) {
        return;
    }
    if (runtime->screenDrawCount >=
        JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY) {
        ++runtime->screenDrawDroppedCount;
        ++runtime->playerHudTileDroppedCount;
        return;
    }

    scale_x = (float)OptionStruct.ScreenWidth / 640.0f;
    scale_y = (float)OptionStruct.ScreenHeight / 480.0f;
    draw = &runtime->screenDraws[runtime->screenDrawCount++];
    draw->order = runtime->drawOrder++;
    draw->texture = NULL;
    draw->destination.left = (int32_t)(
        (position->vx * 460.0f / depth + 320.0f) * scale_x);
    draw->destination.top = (int32_t)(
        (position->vy * 460.0f / depth + 240.0f) * scale_y);
    draw->destination.right = (int32_t)(
        ((position->vx + width + 1.0f) * 460.0f / depth +
         320.0f) * scale_x);
    draw->destination.bottom = (int32_t)(
        ((position->vy + height + 1.0f) * 460.0f / depth +
         240.0f) * scale_y);
    draw->color.r = (uint8_t)(color >> 16);
    draw->color.g = (uint8_t)(color >> 8);
    draw->color.b = (uint8_t)color;
    draw->color.cd = (uint8_t)(color >> 24);
    draw->layerDepth = position->vz;
    draw->hasSource = 0;
    draw->isPlayerHudTile = 1;
    memset(&draw->source, 0, sizeof(draw->source));
    ++runtime->playerHudTileDrawCount;
}

static void game_runtime_capture_bar(
    void *user_data,
    int x,
    int y,
    int width,
    int height,
    uint32_t color)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    JPBGameRuntimeScreenDraw *draw;

    if (runtime == NULL || width <= 0 || height <= 0) {
        return;
    }
    if (runtime->screenDrawCount >=
        JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY) {
        ++runtime->screenDrawDroppedCount;
        return;
    }
    draw = &runtime->screenDraws[runtime->screenDrawCount++];
    draw->order = runtime->drawOrder++;
    draw->texture = NULL;
    draw->destination.left = x;
    draw->destination.top = y;
    draw->destination.right = x + width;
    draw->destination.bottom = y + height;
    draw->textureWidth = 0;
    draw->textureHeight = 0;
    draw->color.r = (uint8_t)(color >> 16);
    draw->color.g = (uint8_t)(color >> 8);
    draw->color.b = (uint8_t)color;
    draw->color.cd = (uint8_t)(color >> 24);
    draw->layerDepth = 0.0f;
    draw->hasSource = 0;
    draw->isPlayerHudTile = 0;
    memset(&draw->source, 0, sizeof(draw->source));
}

static void game_runtime_capture_screen_glow(
    void *user_data,
    const _svector *start,
    const _svector *end,
    int width,
    uint32_t color)
{
    JPBGameRuntime *runtime =
        (JPBGameRuntime *)user_data;
    JPBGameRuntimeGlowDraw *draw;

    if (runtime == NULL || start == NULL || end == NULL) {
        return;
    }
    if (runtime->glowDrawCount >=
        JPB_GAME_RUNTIME_GLOW_DRAW_CAPACITY) {
        ++runtime->glowDrawDroppedCount;
        return;
    }
    draw = &runtime->glowDraws[runtime->glowDrawCount++];
    draw->start = *start;
    draw->end = *end;
    draw->radius = width;
    draw->color = color;
}

static void game_runtime_capture_cylinder(
    void *user_data,
    const VECTOR *location,
    const _svector *rotation,
    float radius1,
    float radius2,
    float height1,
    float height2,
    uint32_t color1,
    uint32_t color2)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    MATRIX matrix;
    _svector start;
    _svector end;
    float radius = radius1 > radius2 ? radius1 : radius2;

    (void)color2;
    if (runtime == NULL || location == NULL || rotation == NULL) {
        return;
    }
    fRotMatrix((_svector *)(void *)rotation, &matrix);
    start.vx = (int16_t)(
        (float)location->vx + matrix.m[0][1] * height1);
    start.vy = (int16_t)(
        (float)location->vy + matrix.m[1][1] * height1);
    start.vz = (int16_t)(
        (float)location->vz + matrix.m[2][1] * height1);
    start.pad = 0;
    end.vx = (int16_t)(
        (float)location->vx + matrix.m[0][1] * height2);
    end.vy = (int16_t)(
        (float)location->vy + matrix.m[1][1] * height2);
    end.vz = (int16_t)(
        (float)location->vz + matrix.m[2][1] * height2);
    end.pad = 0;
    if (radius < 0.0f) {
        radius = -radius;
    }
    ++runtime->cylinderDrawCount;
    game_runtime_capture_screen_glow(
        runtime,
        &start,
        &end,
        (int)(radius + 0.5f),
        color1);
}

static int game_runtime_capture_text(
    void *user_data,
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    float scale_adjustment,
    int font_style,
    const wchar_t *text)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    JPBGameRuntimeTextDraw *draw;
    size_t length;

    if (runtime == NULL || text == NULL) {
        return 0;
    }
    if (runtime->textDrawCount >=
        JPB_GAME_RUNTIME_TEXT_DRAW_CAPACITY) {
        ++runtime->textDrawDroppedCount;
        return 0;
    }
    draw = &runtime->textDraws[runtime->textDrawCount++];
    draw->order = runtime->drawOrder++;
    draw->tint = tint;
    draw->alpha = alpha;
    draw->mode = mode;
    draw->x = x;
    draw->y = y;
    draw->scale = scale;
    draw->scaleAdjustment = scale_adjustment;
    draw->fontStyle = font_style;
    draw->clipEnabled = jpb_TextGetClipRect(
        &draw->clipLeft,
        &draw->clipTop,
        &draw->clipRight,
        &draw->clipBottom);
    draw->compositePixels = 0;
    length = wcslen(text);
    if (length >= JPB_GAME_RUNTIME_TEXT_CAPACITY) {
        length = JPB_GAME_RUNTIME_TEXT_CAPACITY - 1;
    }
    wmemcpy(draw->text, text, length);
    draw->text[length] = L'\0';
    return (int)length;
}

static void game_runtime_capture_draw3d_text(
    float x,
    float y,
    float z,
    float scale,
    uint32_t color,
    const char *text,
    void *user_data)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    JPBGameRuntimeDraw3dText *draw;

    if (runtime == NULL || text == NULL) {
        return;
    }
    if (runtime->draw3dTextDrawCount >=
        JPB_GAME_RUNTIME_DRAW3D_TEXT_CAPACITY) {
        ++runtime->draw3dTextDroppedCount;
        return;
    }
    draw = &runtime->draw3dTextDraws[
        runtime->draw3dTextDrawCount++];
    draw->order = runtime->drawOrder++;
    draw->x = x;
    draw->y = y;
    draw->z = z;
    draw->scale = scale;
    draw->color = color;
    (void)snprintf(
        draw->text,
        sizeof(draw->text),
        "%s",
        text);
}

static void game_runtime_capture_sprite_display(
    void *user_data,
    int type,
    int x,
    int y,
    int w,
    int h,
    int clut,
    const _Material *material)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    JPBGameRuntimeSpriteDisplay *draw;

    if (runtime == NULL) {
        return;
    }
    if (runtime->spriteDisplayDrawCount >=
        JPB_GAME_RUNTIME_SPRITE_DISPLAY_CAPACITY) {
        ++runtime->spriteDisplayDroppedCount;
        return;
    }
    draw = &runtime->spriteDisplayDraws[
        runtime->spriteDisplayDrawCount++];
    draw->order = runtime->drawOrder++;
    draw->type = type;
    draw->x = x;
    draw->y = y;
    draw->width = w;
    draw->height = h;
    draw->clut = clut;
    draw->material = material;
    draw->textureWidth = material != NULL ? material->iw : 0;
    draw->textureHeight = material != NULL ? material->ih : 0;
}

static int game_runtime_capture_psx_texture(
    void *user_data,
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int red,
    int green,
    int blue)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    wchar_t digit[2];

    if (runtime != NULL) {
        if (runtime->psxTextureDrawCount >=
            JPB_GAME_RUNTIME_PSX_TEXTURE_DRAW_CAPACITY) {
            ++runtime->psxTextureDrawDroppedCount;
        } else {
            JPBGameRuntimePsxTextureDraw *draw =
                &runtime->psxTextureDraws[
                    runtime->psxTextureDrawCount++];
            draw->order = runtime->drawOrder;
            draw->texture = texture;
            draw->x = x;
            draw->y = y;
            draw->width = width;
            draw->height = height;
            draw->transparency = transparency;
            draw->red = red;
            draw->green = green;
            draw->blue = blue;
        }
    }
    if (texture >= UINT32_C(0xe8) &&
        texture <= UINT32_C(0xf1)) {
        digit[0] = (wchar_t)(L'0' + texture - UINT32_C(0xe8));
    } else if (texture >= UINT32_C(0xb5) &&
               texture <= UINT32_C(0xbe)) {
        digit[0] = (wchar_t)(L'0' + texture - UINT32_C(0xb5));
    } else {
        return 0;
    }
    digit[1] = L'\0';
    return game_runtime_capture_text(
        user_data,
        11,
        (int)(transparency & UINT32_C(0xff)),
        0,
        (int)x,
        (int)y,
        2.0f,
        scaleAdjustment,
        2,
        digit);
}

/*
 * Dependency-light PC realization of DrawPowerUp's recovered submission
 * boundary. The original owner still determines type, transform, culling,
 * and lifetime; this renderer draws a small world-space glint until the
 * power-up BMD/immediate-mode path itself is reconstructed.
 */
static void game_runtime_capture_powerup_draw(
    void *user_data,
    _svector *position,
    unsigned type,
    _svector *rotation,
    VECTOR *scale,
    _svector *offset)
{
    JPBGameRuntime *runtime =
        (JPBGameRuntime *)user_data;
    _svector center;
    _svector start;
    _svector end;
    uint32_t color;
    int radius;

    (void)rotation;
    if (runtime == NULL || position == NULL ||
        scale == NULL || offset == NULL || type >= 17) {
        return;
    }
    ++runtime->powerupDrawCount;
    center.vx = (int16_t)(position->vx + offset->vx);
    center.vy = (int16_t)(position->vy + offset->vy);
    center.vz = (int16_t)(position->vz + offset->vz);
    center.pad = 0;
    radius = scale->vx >= JPB_FIXED_ONE ? 24 : 16;
    color = UINT32_C(0xff000000) |
        ((uint32_t)pwrIcons[type].r << 16) |
        ((uint32_t)pwrIcons[type].g << 8) |
        (uint32_t)pwrIcons[type].b;

    start = center;
    end = center;
    start.vx = (int16_t)(start.vx - radius);
    end.vx = (int16_t)(end.vx + radius);
    game_runtime_capture_screen_glow(
        runtime, &start, &end, 3, color);
    start = center;
    end = center;
    start.vy = (int16_t)(start.vy - radius);
    end.vy = (int16_t)(end.vy + radius);
    game_runtime_capture_screen_glow(
        runtime, &start, &end, 3, color);
    start = center;
    end = center;
    start.vz = (int16_t)(start.vz - radius);
    end.vz = (int16_t)(end.vz + radius);
    game_runtime_capture_screen_glow(
        runtime, &start, &end, 3, color);
}

static uint32_t game_runtime_blend_rgba(
    uint32_t destination,
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t alpha)
{
    uint32_t inverse_alpha = UINT32_C(255) - alpha;
    uint32_t output_red =
        ((uint32_t)red * alpha +
         ((destination >> 16) & UINT32_C(0xff)) *
             inverse_alpha) /
        UINT32_C(255);
    uint32_t output_green =
        ((uint32_t)green * alpha +
         ((destination >> 8) & UINT32_C(0xff)) *
             inverse_alpha) /
        UINT32_C(255);
    uint32_t output_blue =
        ((uint32_t)blue * alpha +
         (destination & UINT32_C(0xff)) *
             inverse_alpha) /
        UINT32_C(255);

    return (output_red << 16) |
        (output_green << 8) |
        output_blue;
}

static uint32_t game_runtime_blend_screen_color(
    uint32_t destination, CVECTOR color)
{
    return game_runtime_blend_rgba(
        destination,
        color.r,
        color.g,
        color.b,
        color.cd);
}

static uint32_t game_runtime_sample_screen_texture(
    const JPBSoftwareTexture *texture,
    float source_x,
    float source_y)
{
    int x0;
    int y0;
    int x1;
    int y1;
    float fx;
    float fy;
    uint32_t c00;
    uint32_t c10;
    uint32_t c01;
    uint32_t c11;
    uint32_t result = 0;
    int channel;

    if (source_x < 0.0f) source_x = 0.0f;
    if (source_y < 0.0f) source_y = 0.0f;
    if (source_x > (float)(texture->width - 1u)) {
        source_x = (float)(texture->width - 1u);
    }
    if (source_y > (float)(texture->height - 1u)) {
        source_y = (float)(texture->height - 1u);
    }
    x0 = (int)source_x;
    y0 = (int)source_y;
    x1 = x0 + 1;
    y1 = y0 + 1;
    if (x1 >= (int)texture->width) x1 = x0;
    if (y1 >= (int)texture->height) y1 = y0;
    fx = source_x - (float)x0;
    fy = source_y - (float)y0;
    c00 = texture->pixels[
        (size_t)y0 * texture->stridePixels + (size_t)x0];
    c10 = texture->pixels[
        (size_t)y0 * texture->stridePixels + (size_t)x1];
    c01 = texture->pixels[
        (size_t)y1 * texture->stridePixels + (size_t)x0];
    c11 = texture->pixels[
        (size_t)y1 * texture->stridePixels + (size_t)x1];
    for (channel = 0; channel < 4; ++channel) {
        unsigned shift = (unsigned)channel * 8u;
        float v00 = (float)((c00 >> shift) & UINT32_C(0xff));
        float v10 = (float)((c10 >> shift) & UINT32_C(0xff));
        float v01 = (float)((c01 >> shift) & UINT32_C(0xff));
        float v11 = (float)((c11 >> shift) & UINT32_C(0xff));
        float top = v00 + (v10 - v00) * fx;
        float bottom = v01 + (v11 - v01) * fx;
        int value = (int)(top + (bottom - top) * fy + 0.5f);

        if (value < 0) value = 0;
        if (value > 255) value = 255;
        result |= (uint32_t)value << shift;
    }
    return result;
}

static int game_runtime_screen_draw_is_item_hud_texture(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        draw->texture->filename[0] == '\0') {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_detonator") != NULL ||
        strstr(filename, "a_bolt") != NULL ||
        strstr(filename, "a_battery") != NULL ||
        strstr(filename, "a_shield") != NULL;
}

static int game_runtime_screen_draw_is_credit_hud_texture(
    const JPBGameRuntimeScreenDraw *draw)
{
    if (draw == NULL || draw->texture == NULL ||
        draw->texture->filename[0] == '\0') {
        return 0;
    }
    return strstr(draw->texture->filename, "a_credit") != NULL;
}

static int game_runtime_screen_draw_is_rescue_hud_texture(
    const JPBGameRuntimeScreenDraw *draw)
{
    if (draw == NULL || draw->texture == NULL ||
        draw->texture->filename[0] == '\0') {
        return 0;
    }
    return strstr(draw->texture->filename, "a_maiden") != NULL ||
        strstr(draw->texture->filename, "a_pilot") != NULL;
}

static void game_runtime_flush_screen_draw_range(
    JPBGameRuntime *runtime,
    JPBSoftwareFramebuffer *framebuffer,
    size_t first_draw,
    size_t draw_count)
{
    size_t draw_order[JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY];
    size_t draw_index;

    if (runtime == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL ||
        framebuffer->width <= 0 ||
        framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width) {
        return;
    }
    if (first_draw >= runtime->screenDrawCount) {
        return;
    }
    if (draw_count > runtime->screenDrawCount - first_draw) {
        draw_count = runtime->screenDrawCount - first_draw;
    }
    /*
     * _DrawTexture submits screen quads to the same depth-tested bucket as
     * the retail renderer. Larger menu layer values are farther away: draw
     * them first, while retaining submission order for equal-depth tiles.
     * Capturing and replaying raw call order incorrectly put the 0.2 blue
     * character overlay on top of the selected portrait at depth 0.1.
     */
    for (draw_index = 0;
         draw_index < draw_count;
         ++draw_index) {
        size_t insertion = draw_index;

        draw_order[draw_index] = first_draw + draw_index;
        while (insertion > 0) {
            size_t previous = draw_order[insertion - 1];
            size_t current = draw_order[insertion];

            if (runtime->screenDraws[previous].layerDepth >=
                runtime->screenDraws[current].layerDepth) {
                break;
            }
            draw_order[insertion - 1] = current;
            draw_order[insertion] = previous;
            --insertion;
        }
    }
    for (draw_index = 0;
         draw_index < draw_count;
         ++draw_index) {
        const JPBGameRuntimeScreenDraw *draw =
            &runtime->screenDraws[draw_order[draw_index]];
        int left = draw->destination.left;
        int top = draw->destination.top;
        int right = draw->destination.right;
        int bottom = draw->destination.bottom;
        int destination_width = right - left;
        int destination_height = bottom - top;
        const JPBSoftwareTexture *texture = NULL;
        int item_hud_texture =
            game_runtime_screen_draw_is_item_hud_texture(draw);
        int credit_hud_texture =
            game_runtime_screen_draw_is_credit_hud_texture(draw);
        int rescue_hud_texture =
            game_runtime_screen_draw_is_rescue_hud_texture(draw);
        int y;

        if (draw->texture != NULL &&
            draw->texture->texture != NULL) {
            const JPBSoftwareTexture *candidate =
                (const JPBSoftwareTexture *)draw->texture->texture;

            if (candidate->pixels != NULL &&
                candidate->width > 0 &&
                candidate->height > 0 &&
                candidate->width <= UINT32_C(16384) &&
                candidate->height <= UINT32_C(16384) &&
                candidate->stridePixels >= candidate->width) {
                texture = candidate;
            }
        }

        if (left < 0) left = 0;
        if (top < 0) top = 0;
        if (right > framebuffer->width) {
            right = framebuffer->width;
        }
        if (bottom > framebuffer->height) {
            bottom = framebuffer->height;
        }
        if (left >= right || top >= bottom) {
            continue;
        }
        for (y = top; y < bottom; ++y) {
            uint32_t *row =
                framebuffer->pixels +
                (size_t)y *
                    (size_t)framebuffer->stridePixels;
            int x;

            for (x = left; x < right; ++x) {
                if (texture != NULL &&
                    destination_width != 0 &&
                    destination_height != 0) {
                    int source_left = draw->hasSource
                        ? draw->source.left
                        : 0;
                    int source_top = draw->hasSource
                        ? draw->source.top
                        : 0;
                    int source_right = draw->hasSource
                        ? draw->source.right
                        : (int)texture->width;
                    int source_bottom = draw->hasSource
                        ? draw->source.bottom
                        : (int)texture->height;
                    float source_x = (float)source_left +
                        ((float)(source_right - source_left) *
                         ((float)(x - draw->destination.left) + 0.5f)) /
                            (float)destination_width - 0.5f;
                    float source_y = (float)source_top +
                        ((float)(source_bottom - source_top) *
                         ((float)(y - draw->destination.top) + 0.5f)) /
                            (float)destination_height - 0.5f;
                    uint32_t sample;
                    uint32_t sample_alpha;
                    uint8_t alpha;
                    uint8_t red;
                    uint8_t green;
                    uint8_t blue;

                    sample = game_runtime_sample_screen_texture(
                        texture, source_x, source_y);
                    sample_alpha = sample >> 24;
                    red = (uint8_t)(
                        (sample >> 16) & UINT32_C(0xff));
                    green = (uint8_t)(
                        (sample >> 8) & UINT32_C(0xff));
                    blue = (uint8_t)(
                        sample & UINT32_C(0xff));
                    /*
                     * Front-end PNG/TGA assets carry real alpha. The level
                     * select foreground uses opaque black as a mask, so the
                     * compositor must not treat black RGB as transparency.
                     */
                    if (sample_alpha == 0) {
                        continue;
                    }
                    if (draw->color.r != 0 &&
                        draw->color.g != 0 &&
                        draw->color.b != 0 &&
                        draw->color.cd != 0) {
                        red = (uint8_t)(
                            (uint32_t)red *
                                draw->color.r /
                            UINT32_C(255));
                        green = (uint8_t)(
                            (uint32_t)green *
                                draw->color.g /
                            UINT32_C(255));
                        blue = (uint8_t)(
                            (uint32_t)blue *
                                draw->color.b /
                            UINT32_C(255));
                        sample_alpha =
                            sample_alpha *
                            (uint32_t)draw->color.cd /
                            UINT32_C(255);
                        if (draw->color.cd < UINT8_C(255)) {
                            ++runtime->
                                screenDrawTextureAlphaModulatedPixelCount;
                            if (item_hud_texture) {
                                ++runtime->
                                    itemHudTextureAlphaModulatedPixelCount;
                            }
                            if (credit_hud_texture) {
                                ++runtime->
                                    creditHudTextureAlphaModulatedPixelCount;
                            }
                            if (rescue_hud_texture) {
                                ++runtime->
                                    rescueHudTextureAlphaModulatedPixelCount;
                            }
                        }
                    }
                    if (sample_alpha == 0) {
                        continue;
                    }
                    alpha = (uint8_t)sample_alpha;
                    row[x] = game_runtime_blend_rgba(
                        row[x], red, green, blue, alpha);
                } else {
                    row[x] =
                        game_runtime_blend_screen_color(
                            row[x], draw->color);
                }
                ++runtime->screenDrawCompositePixelCount;
                if (draw->isPlayerHudTile) {
                    ++runtime->playerHudTileCompositePixelCount;
                }
            }
        }
    }
}

static void game_runtime_flush_screen_draws(
    JPBGameRuntime *runtime,
    JPBSoftwareFramebuffer *framebuffer)
{
    if (runtime == NULL) {
        return;
    }
    game_runtime_flush_screen_draw_range(
        runtime,
        framebuffer,
        0,
        runtime->screenDrawCount);
}

static void game_runtime_clear_framebuffer(
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color)
{
    int y;

    if (framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width) {
        return;
    }
    for (y = 0; y < framebuffer->height; ++y) {
        uint32_t *row =
            framebuffer->pixels +
            (size_t)y *
                (size_t)framebuffer->stridePixels;
        int x;

        for (x = 0; x < framebuffer->width; ++x) {
            row[x] = clear_color;
        }
    }
}

/* Compact dependency-light glyphs used by the recovered in-game HUD. */
static uint8_t game_runtime_glyph_row(wchar_t glyph, int row)
{
    static const uint8_t digits[10][7] = {
        {14, 17, 19, 21, 25, 17, 14},
        {4, 12, 4, 4, 4, 4, 14},
        {14, 17, 1, 2, 4, 8, 31},
        {30, 1, 1, 14, 1, 1, 30},
        {2, 6, 10, 18, 31, 2, 2},
        {31, 16, 16, 30, 1, 1, 30},
        {6, 8, 16, 30, 17, 17, 14},
        {31, 1, 2, 4, 8, 8, 8},
        {14, 17, 17, 14, 17, 17, 14},
        {14, 17, 17, 15, 1, 2, 12}
    };
    static const uint8_t plus[7] = {0, 4, 4, 31, 4, 4, 0};
    static const uint8_t minus[7] = {0, 0, 0, 31, 0, 0, 0};

    if (row < 0 || row >= 7) {
        return 0;
    }
    if (glyph >= L'0' && glyph <= L'9') {
        return digits[glyph - L'0'][row];
    }
    if (glyph == L'+') {
        return plus[row];
    }
    if (glyph == L'-') {
        return minus[row];
    }
    if (glyph == L' ') {
        return 0;
    }
    return row == 0 || row == 6 ? 31 : 17;
}

static void game_runtime_flush_text_draw(
    JPBGameRuntime *runtime,
    JPBSoftwareFramebuffer *framebuffer,
    JPBGameRuntimeTextDraw *draw)
{
    JPBPortableTextMetrics metrics;
    size_t length;
    float scaled_size;
    int pixel_size;
    int text_width;
    int origin_x;
    int alpha;
    uint32_t tint;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    size_t glyph_index;

    if (runtime == NULL || framebuffer == NULL || draw == NULL ||
        framebuffer->pixels == NULL ||
        framebuffer->width <= 0 ||
        framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width) {
        return;
    }
    length = wcslen(draw->text);
    scaled_size = draw->scale * draw->scaleAdjustment;
    pixel_size = (int)(scaled_size + 0.5f);
    origin_x = draw->x;
    alpha = draw->alpha;
    tint = jpb_PortableTextTint(draw->tint);
    red = (uint8_t)(tint & UINT32_C(0xff));
    green = (uint8_t)((tint >> 8) & UINT32_C(0xff));
    blue = (uint8_t)((tint >> 16) & UINT32_C(0xff));

    if (jpb_PortableTextDraw(
            draw->text,
            draw->tint,
            draw->alpha,
            draw->mode,
            draw->x,
            draw->y,
            draw->scale,
            draw->fontStyle,
            OptionStruct.Language,
            draw->scaleAdjustment,
            draw->clipEnabled,
            draw->clipLeft,
            draw->clipTop,
            draw->clipRight,
            draw->clipBottom,
            framebuffer,
            &metrics)) {
        ++runtime->textTrueTypeDrawCount;
        runtime->textDrawCompositePixelCount +=
            metrics.blendedPixels;
        draw->compositePixels +=
            metrics.blendedPixels;
        if (metrics.pointSize >
            runtime->maximumTextPointSize) {
            runtime->maximumTextPointSize = metrics.pointSize;
        }
        if (metrics.width >
            runtime->maximumTextMeasuredWidth) {
            runtime->maximumTextMeasuredWidth = metrics.width;
        }
        if (metrics.height >
            runtime->maximumTextMeasuredHeight) {
            runtime->maximumTextMeasuredHeight = metrics.height;
        }
        return;
    }
    ++runtime->textFallbackDrawCount;
    {
        int point_size = jpb_PortableTextPointSize(
            draw->scale, draw->scaleAdjustment);

        if (point_size > runtime->maximumTextPointSize) {
            runtime->maximumTextPointSize = point_size;
        }
    }
    if (pixel_size < 1) pixel_size = 1;
    if (pixel_size > 16) pixel_size = 16;
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;
    text_width = length != 0
        ? (int)(length * (size_t)(6 * pixel_size) -
                (size_t)pixel_size)
        : 0;
    if (text_width > runtime->maximumTextMeasuredWidth) {
        runtime->maximumTextMeasuredWidth = text_width;
    }
    if (7 * pixel_size > runtime->maximumTextMeasuredHeight) {
        runtime->maximumTextMeasuredHeight = 7 * pixel_size;
    }
    if ((draw->mode & 0x7f) == 1) {
        origin_x -= text_width;
    } else if ((draw->mode & 0x7f) == 2) {
        origin_x -= text_width / 2;
    }

    for (glyph_index = 0;
         glyph_index < length;
         ++glyph_index) {
        int glyph_x = origin_x +
            (int)glyph_index * 6 * pixel_size;
        int glyph_y;

        for (glyph_y = 0; glyph_y < 7; ++glyph_y) {
            uint8_t bits = game_runtime_glyph_row(
                draw->text[glyph_index], glyph_y);
            int glyph_column;

            for (glyph_column = 0;
                 glyph_column < 5;
                 ++glyph_column) {
                int block_x;
                int block_y;

                if ((bits & (uint8_t)(
                        1u << (4 - glyph_column))) == 0) {
                    continue;
                }
                for (block_y = 0;
                     block_y < pixel_size;
                     ++block_y) {
                    int output_y = draw->y +
                        glyph_y * pixel_size + block_y;
                    uint32_t *row;

                    if (output_y < 0 ||
                        output_y >= framebuffer->height) {
                        continue;
                    }
                    row = framebuffer->pixels +
                        (size_t)output_y *
                            (size_t)framebuffer->stridePixels;
                    for (block_x = 0;
                         block_x < pixel_size;
                         ++block_x) {
                        int output_x = glyph_x +
                            glyph_column * pixel_size +
                            block_x;

                        if (output_x < 0 ||
                            output_x >= framebuffer->width) {
                            continue;
                        }
                        row[output_x] = game_runtime_blend_rgba(
                            row[output_x],
                            red,
                            green,
                            blue,
                            (uint8_t)alpha);
                        ++runtime->textDrawCompositePixelCount;
                        ++draw->compositePixels;
                    }
                }
            }
        }
    }
}

static void game_runtime_flush_text_draws(
    JPBGameRuntime *runtime,
    JPBSoftwareFramebuffer *framebuffer)
{
    size_t draw_index;

    if (runtime == NULL) {
        return;
    }
    for (draw_index = 0;
         draw_index < runtime->textDrawCount;
         ++draw_index) {
        game_runtime_flush_text_draw(
            runtime,
            framebuffer,
            &runtime->textDraws[draw_index]);
    }
}

static void game_runtime_flush_ordered_title_draws(
    JPBGameRuntime *runtime,
    JPBSoftwareFramebuffer *framebuffer)
{
    size_t screen_index = 0;
    size_t text_index = 0;

    if (runtime == NULL) {
        return;
    }
    while (screen_index < runtime->screenDrawCount ||
           text_index < runtime->textDrawCount) {
        int has_next_text = text_index < runtime->textDrawCount;
        uint32_t next_text_order = has_next_text
            ? runtime->textDraws[text_index].order
            : UINT32_MAX;
        size_t screen_start = screen_index;

        while (screen_index < runtime->screenDrawCount &&
               (!has_next_text ||
                runtime->screenDraws[screen_index].order <
                    next_text_order)) {
            ++screen_index;
        }
        if (screen_index > screen_start) {
            game_runtime_flush_screen_draw_range(
                runtime,
                framebuffer,
                screen_start,
                screen_index - screen_start);
        }
        if (has_next_text) {
            game_runtime_flush_text_draw(
                runtime,
                framebuffer,
                &runtime->textDraws[text_index]);
            ++text_index;
        }
    }
}

static void game_runtime_flush_glow_draws(
    JPBGameRuntime *runtime,
    MATRIX *view,
    JPBSoftwareFramebuffer *framebuffer,
    const JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats)
{
    size_t draw_index;
    size_t pixels_before = stats != NULL ? stats->pixels : 0;
    size_t rejected_before = stats != NULL
        ? stats->glowDepthRejectedPixels
        : 0;

    if (runtime == NULL || view == NULL || framebuffer == NULL) {
        return;
    }
    for (draw_index = 0;
         draw_index < runtime->glowDrawCount;
         ++draw_index) {
        const JPBGameRuntimeGlowDraw *draw =
            &runtime->glowDraws[draw_index];

        (void)jpb_SoftwareDrawGlowLine(
            &draw->start,
            &draw->end,
            draw->radius,
            draw->color,
            view,
            framebuffer,
            depth_buffer,
            stats);
    }
    if (stats != NULL) {
        runtime->glowDrawCompositePixelCount =
            stats->pixels - pixels_before;
        runtime->glowDrawDepthRejectedPixelCount =
            stats->glowDepthRejectedPixels - rejected_before;
    }
}

struct JPBGameRuntimeTextureCache {
    char directory[JPB_GAME_RUNTIME_PATH_CAPACITY];
    int levelIndex;
    JPBGameRuntimeTexture *textures;
    size_t textureCount;
    size_t textureCapacity;
    size_t loadedTextureCount;
};

struct JPBGameRuntimeSecondPlayerState {
    uint8_t *cadStorage;
    uint8_t *bmdStorage;
    uint8_t *comboStorage;
    JPBCadView cadView;
    JPBBmdView bmdView;
    JPBGameRuntimeTextureCache *textureCache;
    modelObject model;
    animObject *animation;
    int authoredMotionReady;
    int authoredFrameReady;
    int authoredPoseReady;
    uint32_t decodedFrameCount;
    size_t renderedTriangles;
    size_t renderedPixels;
};

static int game_runtime_resolve_texture(
    void *user_data,
    const char *texture_name,
    JPBSoftwareTexture *texture);
static void *game_runtime_load_material_texture(
    void *user_data,
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height);
static void game_runtime_scene_after_animations(
    void *user_data, MATRIX *view);
static void game_runtime_scene_after_world(
    void *user_data, MATRIX *view);
static void game_runtime_scene_after_models(
    void *user_data, MATRIX *view);
static void game_runtime_scene_before_player_process(
    void *user_data, MATRIX *view);
static void game_runtime_scene_level_owner(
    void *user_data,
    int level,
    int argument0,
    int argument1,
    int argument2);
static void game_runtime_capture_screen_poly(
    void *user_data,
    _Material *material,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale);

static void *game_runtime_load_resident_texture(
    const char *path,
    int texture_type,
    uint32_t option)
{
    return _LoadTexture(
        (char *)(void *)path,
        (TT_TEXTYPE)texture_type,
        option);
}

typedef struct JPBGameRuntimeEnemyClass {
    int actorNum;
    int modelId;
    const char *modelName;
    const char *animationName;
    uint8_t *cadStorage;
    uint8_t *bmdStorage;
    JPBCadView cadView;
    JPBBmdView bmdView;
    JPBGameRuntimeTextureCache *textureCache;
    uint8_t *aiStorage[JPB_AI_LEVEL_CAPACITY];
    size_t aiStorageSize[JPB_AI_LEVEL_CAPACITY];
    aiData *aiDataByLevel[JPB_AI_LEVEL_CAPACITY];
    char aiCadPath[JPB_GAME_RUNTIME_PATH_CAPACITY];
    int wasActive;
    int wasRendered;
} JPBGameRuntimeEnemyClass;

typedef struct JPBGameRuntimeEnemyActor {
    objectRoot actorRoot;
    sceneObject *scene;
    modelObject model;
    physicsObject *physics;
    animObject *animation;
    playerObject *player;
    wsl_ENEMY *enemy;
    JPBGameRuntimeEnemyClass *assetClass;
    int authoredMotionReady;
    int authoredFrameReady;
    int authoredPoseReady;
    uint32_t decodedFrameCount;
    uint32_t mainCallbackFrameCount;
    uint32_t kungfuSchedulerFrameCount;
    uint32_t authoredOpcodeFrameCount;
    uint32_t opcodeBoundaryFrameCount;
    uint16_t opcodeBoundary;
    uint32_t damageProcessedCount;
    uint32_t reactionMotionFrameCount;
    uint32_t recoilReactionCount;
    int16_t lastReactionMotion;
    int16_t lastDamageMotion;
    int16_t animationMotion;
    int16_t aiLevel;
    int16_t initialEnergy;
    int16_t minimumEnergy;
    float lastRecoil;
    uint8_t schedulerHitBefore;
    uint8_t schedulerPendingHit;
    int schedulerAuthoredHitReaction;
    float schedulerRecoilBefore;
    size_t renderedTriangles;
    size_t renderedPixels;
} JPBGameRuntimeEnemyActor;

static int game_runtime_build_model(
    JPBBmdView *view,
    const char *name,
    int object_id,
    modelObject *destination,
    JPBGameRuntimeTextureCache *texture_cache)
{
    objectRoot scene_root;
    modelObject *registered;

    if (view == NULL || view->payload == NULL ||
        name == NULL || destination == NULL) {
        return 0;
    }
    scene_root = destination->modelRoot;
    jpb_TextureSetPlatformHooks(
        texture_cache != NULL
            ? game_runtime_load_material_texture
            : NULL,
        NULL,
        texture_cache);
    jpb_ModelSetGeometryBounds(
        view->payload, view->payload_size);
    registered = model_gInitModelRoot(
        (geomData *)(void *)view->payload,
        (char *)(void *)name,
        object_id);
    if (registered == NULL || registered->pRootNode == NULL) {
        return 0;
    }
    if (!mReuseModel) {
        view->geometry_streams_relocated = 1;
        view->material_handles_relocated =
            jpb_TextureHasLoadHook();
    }
    *destination = *registered;
    destination->modelRoot = scene_root;
    return 1;
}

struct JPBGameRuntimeEnemyState {
    JPBGameRuntimeEnemyActor
        actors[JPB_GAME_RUNTIME_ENEMY_CAPACITY];
    JPBGameRuntimeEnemyClass
        classes[JPB_GAME_RUNTIME_ENEMY_CLASS_CAPACITY];
    size_t classCount;
};

static void game_runtime_observe_player_process(
    JPBPlayerProcessPhase phase,
    int index,
    playerObject *player,
    int AI_ON,
    const int32_t *cpad,
    void *user_data);

static void game_runtime_observe_bullet_launch(
    void *user_data,
    const Projectile *projectile,
    const playerObject *player,
    const VECTOR *start,
    const VECTOR *target);

typedef struct JPBGameRuntimeEnemyClassSpec {
    const char *actorStem;
    const char *modelName;
    const char *animationName;
    int modelId;
} JPBGameRuntimeEnemyClassSpec;

/* Build a portable view of the exact executable-owned BAF/model/animation
 * tables instead of maintaining a second hand-selected class list. */
static int game_runtime_enemy_class_spec(
    size_t actor_index,
    JPBGameRuntimeEnemyClassSpec *spec)
{
    const modelAnimConnect *connection;

    if (spec == NULL || actor_index >= JPB_ACTOR_NAME_COUNT) {
        return 0;
    }
    connection = &model_anim_table[actor_index];
    if (connection->modelID >= JPB_MODEL_NAME_COUNT ||
        connection->poolID >= JPB_ANIMATION_NAME_COUNT ||
        sObiNames[actor_index] == NULL ||
        sModelNames[connection->modelID] == NULL ||
        sAnimNames[connection->poolID] == NULL) {
        return 0;
    }
    spec->actorStem = sObiNames[actor_index];
    spec->modelName = sModelNames[connection->modelID];
    spec->animationName = sAnimNames[connection->poolID];
    spec->modelId = connection->modelID;
    return 1;
}

static int game_runtime_archive_memory_initialized;
static char game_runtime_effect_directory[
    JPB_GAME_RUNTIME_PATH_CAPACITY];
static char game_runtime_resource_path[
    JPB_GAME_RUNTIME_PATH_CAPACITY];

static void *game_runtime_resolve_bmd_geometry_stream(
    const JPBBmdView *view,
    const geomData *geometry,
    int pointer_type)
{
    JPBBmdGeometryView geometry_view;
    uintptr_t payload_address;
    uintptr_t geometry_address;
    uintptr_t payload_end;

    if (view == NULL || view->payload == NULL ||
        geometry == NULL) {
        return NULL;
    }
    payload_address = (uintptr_t)view->payload;
    geometry_address = (uintptr_t)geometry;
    if ((uintptr_t)view->payload_size >
            UINTPTR_MAX - payload_address) {
        return NULL;
    }
    payload_end =
        payload_address + (uintptr_t)view->payload_size;
    if (geometry_address < payload_address ||
        geometry_address > payload_end ||
        sizeof(*geometry) >
            (size_t)(payload_end - geometry_address) ||
        jpb_BmdGetGeometry(
            view, geometry, &geometry_view) != JPB_BMD_OK) {
        return NULL;
    }

    switch (pointer_type) {
    case JPB_POINTER_ARRAY_VERTEX:
        return (void *)geometry_view.packed_vertices;
    case JPB_POINTER_ARRAY_NORMAL:
        return (void *)geometry_view.packed_normals;
    case JPB_POINTER_ARRAY_UV:
        return (void *)geometry_view.face_uvs;
    case JPB_POINTER_ARRAY_COLOR:
        return (void *)geometry_view.colors;
    case JPB_POINTER_ARRAY_INDEX:
        if (geometry_view.face_encoding ==
            JPB_BMD_FACE_SIGNED_16) {
            return (void *)geometry_view.faces;
        }
        return (void *)geometry_view.packed_faces;
    default:
        return NULL;
    }
}

static void *game_runtime_resolve_geometry_stream(
    const geomData *geometry,
    int pointer_type,
    void *user_data)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    void *resolved;
    size_t class_index;

    if (runtime == NULL) {
        return NULL;
    }
    resolved = game_runtime_resolve_bmd_geometry_stream(
        &runtime->bmdView, geometry, pointer_type);
    if (resolved != NULL) {
        return resolved;
    }
    if (runtime->enemyState == NULL) {
        return NULL;
    }
    for (class_index = 0;
         class_index < runtime->enemyState->classCount;
         ++class_index) {
        resolved = game_runtime_resolve_bmd_geometry_stream(
            &runtime->enemyState->classes[class_index].bmdView,
            geometry,
            pointer_type);
        if (resolved != NULL) {
            return resolved;
        }
    }
    return NULL;
}

static int game_runtime_is_separator(char value)
{
    return value == '/' || value == '\\';
}

static int game_runtime_ascii_lower(int value)
{
    if (value >= 'A' && value <= 'Z') {
        return value + ('a' - 'A');
    }
    return value;
}

static int game_runtime_path_stem_equals(
    const char *path,
    const char *expected_stem)
{
    const char *base;
    const char *slash;
    const char *backslash;
    const char *extension;
    size_t stem_bytes;
    size_t index;

    if (path == NULL || expected_stem == NULL) {
        return 0;
    }
    slash = strrchr(path, '/');
    backslash = strrchr(path, '\\');
    base = path;
    if (slash != NULL) {
        base = slash + 1;
    }
    if (backslash != NULL && backslash + 1 > base) {
        base = backslash + 1;
    }
    extension = strrchr(base, '.');
    stem_bytes =
        extension != NULL
            ? (size_t)(extension - base)
            : strlen(base);
    if (stem_bytes != strlen(expected_stem)) {
        return 0;
    }
    for (index = 0; index < stem_bytes; ++index) {
        if (game_runtime_ascii_lower(
                (unsigned char)base[index]) !=
            game_runtime_ascii_lower(
                (unsigned char)expected_stem[index])) {
            return 0;
        }
    }
    return 1;
}

static int game_runtime_set_effect_directory(
    const char *asset_path)
{
    size_t path_bytes = strlen(asset_path);
    size_t index;
    const char *effects = "effects";

    for (index = 0; index + 4 < path_bytes; ++index) {
        const char *candidate = asset_path + index;

        if (game_runtime_is_separator(candidate[0]) &&
            game_runtime_ascii_lower(candidate[1]) == 'r' &&
            game_runtime_ascii_lower(candidate[2]) == 'e' &&
            game_runtime_ascii_lower(candidate[3]) == 's' &&
            game_runtime_is_separator(candidate[4])) {
            size_t prefix_bytes = index + 5;
            size_t effects_bytes = strlen(effects);
            char base_path[JPB_RESOURCE_PATH_CAPACITY];

            if (index + 2 > sizeof(base_path)) {
                return 0;
            }
            memcpy(base_path, asset_path, index + 1);
            base_path[index + 1] = '\0';
            if (!jpb_ResourceSetBasePath(base_path)) {
                return 0;
            }

            if (prefix_bytes + effects_bytes + 2 >
                sizeof(game_runtime_effect_directory)) {
                return 0;
            }
            memcpy(
                game_runtime_effect_directory,
                asset_path,
                prefix_bytes);
            memcpy(
                game_runtime_effect_directory + prefix_bytes,
                effects,
                effects_bytes);
            game_runtime_effect_directory[
                prefix_bytes + effects_bytes] =
                candidate[4];
            game_runtime_effect_directory[
                prefix_bytes + effects_bytes + 1] =
                '\0';
            return 1;
        }
    }
    return 0;
}

static const char *game_runtime_resolve_resource(
    const char *resource_name,
    int resource_type,
    const char *extension)
{
    size_t directory_bytes;
    size_t name_bytes;

    (void)extension;
    if (resource_type == JPB_RESOURCE_EFFECT_TEXTURE) {
        return resource_getPath(
            resource_name,
            JPB_RESOURCE_EFFECT_TEXTURE);
    }
    if (resource_type != JPB_RESOURCE_EFFECT ||
        game_runtime_effect_directory[0] == '\0') {
        return resource_name;
    }
    directory_bytes =
        strlen(game_runtime_effect_directory);
    name_bytes = strlen(resource_name);
    if (directory_bytes + name_bytes + 1 >
        sizeof(game_runtime_resource_path)) {
        return NULL;
    }
    memcpy(
        game_runtime_resource_path,
        game_runtime_effect_directory,
        directory_bytes);
    memcpy(
        game_runtime_resource_path + directory_bytes,
        resource_name,
        name_bytes + 1);
    return game_runtime_resource_path;
}

static int game_runtime_collision_path(
    const char *jpx_path,
    int level_index,
    char *path,
    size_t path_capacity)
{
    const char *cursor = NULL;
    size_t path_bytes;
    size_t index;
    size_t prefix_bytes;
    size_t level_bytes;
    const char *directory = "W3D/";
    const char *extension = ".j3d";
    char separator;

    if (level_index < 0 ||
        level_index >= JPB_LEVEL_COUNT) {
        return 0;
    }
    path_bytes = strlen(jpx_path);
    for (index = 0; index + 4 < path_bytes; ++index) {
        const char *candidate = jpx_path + index;

        if (game_runtime_is_separator(candidate[0]) &&
            game_runtime_ascii_lower(
                (unsigned char)candidate[1]) == 'j' &&
            game_runtime_ascii_lower(
                (unsigned char)candidate[2]) == 'p' &&
            game_runtime_ascii_lower(
                (unsigned char)candidate[3]) == 'x' &&
            game_runtime_is_separator(candidate[4])) {
            cursor = candidate;
            break;
        }
    }
    if (cursor == NULL) {
        return 0;
    }

    prefix_bytes = (size_t)(cursor - jpx_path + 1);
    level_bytes = strlen(sLevelNames[level_index]);
    if (prefix_bytes + strlen(directory) + level_bytes +
            strlen(extension) + 1 >
        path_capacity) {
        return 0;
    }
    separator = *cursor;
    memcpy(path, jpx_path, prefix_bytes);
    memcpy(
        path + prefix_bytes,
        directory,
        strlen(directory));
    if (separator == '\\') {
        path[prefix_bytes + strlen(directory) - 1] = '\\';
    }
    memcpy(
        path + prefix_bytes + strlen(directory),
        sLevelNames[level_index],
        level_bytes);
    memcpy(
        path + prefix_bytes + strlen(directory) + level_bytes,
        extension,
        strlen(extension) + 1);
    return 1;
}

static int game_runtime_camera_path(
    const char *jpx_path,
    int level_index,
    char *path,
    size_t path_capacity)
{
    const char *cursor = NULL;
    const char *directory = "CAMERAS/";
    const char *extension = ".cam";
    size_t path_bytes;
    size_t index;
    size_t prefix_bytes;
    size_t level_bytes;
    char separator;

    if (level_index < 0 ||
        level_index >= JPB_LEVEL_COUNT) {
        return 0;
    }
    path_bytes = strlen(jpx_path);
    for (index = 0; index + 4 < path_bytes; ++index) {
        const char *candidate = jpx_path + index;

        if (game_runtime_is_separator(candidate[0]) &&
            game_runtime_ascii_lower(candidate[1]) == 'j' &&
            game_runtime_ascii_lower(candidate[2]) == 'p' &&
            game_runtime_ascii_lower(candidate[3]) == 'x' &&
            game_runtime_is_separator(candidate[4])) {
            cursor = candidate;
            break;
        }
    }
    if (cursor == NULL) {
        return 0;
    }
    prefix_bytes = (size_t)(cursor - jpx_path + 1);
    level_bytes = strlen(sLevelNames[level_index]);
    if (prefix_bytes + strlen(directory) + level_bytes +
            strlen(extension) + 1 >
        path_capacity) {
        return 0;
    }
    separator = *cursor;
    memcpy(path, jpx_path, prefix_bytes);
    memcpy(
        path + prefix_bytes,
        directory,
        strlen(directory));
    if (separator == '\\') {
        path[prefix_bytes + strlen(directory) - 1] = '\\';
    }
    memcpy(
        path + prefix_bytes + strlen(directory),
        sLevelNames[level_index],
        level_bytes);
    memcpy(
        path + prefix_bytes + strlen(directory) + level_bytes,
        extension,
        strlen(extension) + 1);
    return 1;
}

static int game_runtime_load_authored_cameras(
    JPBGameRuntime *runtime,
    const char *jpx_path,
    int level_index)
{
    char path[JPB_GAME_RUNTIME_PATH_CAPACITY];
    uint64_t size;
    int32_t loaded;

    if (runtime == NULL || runtime->world == NULL ||
        !game_runtime_camera_path(
            jpx_path,
            level_index,
            path,
            sizeof(path))) {
        return 0;
    }
    size = file_getFileSize(path);
    if (size != sizeof(runtime->world->aDolly)) {
        return 0;
    }
    loaded = file_LoadFile(
        path, runtime->world->aDolly);
    if (loaded !=
        (int32_t)sizeof(runtime->world->aDolly)) {
        return 0;
    }
    memcpy(
        runtime->world->aBkDolly,
        runtime->world->aDolly,
        sizeof(runtime->world->aDolly));
    runtime->world->currentDolly = 0;
    runtime->authoredCameraDolly = 0;
    return 1;
}

static int game_runtime_prepare_archive_memory(void)
{
    int pool;

    if (!game_runtime_archive_memory_initialized) {
        if (memory_InitMemorySystem() != 0) {
            return 0;
        }
        game_runtime_archive_memory_initialized = 1;
    } else {
        for (pool = 0; pool < MEMORY_POOL_COUNT; ++pool) {
            (void)memory_FlushMemoryPool(pool);
        }
        pointerRegistry_Reset();
    }
    return 1;
}

static int game_runtime_load_collision(
    JPBGameRuntime *runtime,
    const char *jpx_path,
    int level_index)
{
    char path[JPB_GAME_RUNTIME_PATH_CAPACITY];
    JPBFileHandle file = 0;
    uint64_t file_size;
    uint8_t *end_cursor;

    if (!game_runtime_collision_path(
            jpx_path,
            level_index,
            path,
            sizeof(path))) {
        return 0;
    }
    if (!file_OPEN(path, &file)) {
        return 0;
    }
    file_size = file_GETSIZE(&file);
    if (file_size == 0 || file_size > SIZE_MAX ||
        file_size > INT32_MAX) {
        (void)file_CLOSE(&file);
        return 0;
    }

    runtime->collisionStorage =
        (uint8_t *)malloc((size_t)file_size);
    runtime->world =
        (WorldData *)calloc(1, sizeof(*runtime->world));
    if (runtime->collisionStorage == NULL ||
        runtime->world == NULL) {
        (void)file_CLOSE(&file);
        return 0;
    }
    if (file_READ(
            &file,
            (char *)runtime->collisionStorage,
            (int32_t)file_size,
            JPB_FILE_READ_STREAM) != file_size) {
        (void)file_CLOSE(&file);
        return 0;
    }
    (void)file_CLOSE(&file);

    runtime->collisionStorageSize = (size_t)file_size;
    gpWorld = runtime->world;
    if (file_RelocateChunks(
            runtime->collisionStorage,
            runtime->collisionStorageSize,
            &end_cursor) != JPB_CHUNKS_OK ||
        end_cursor !=
            runtime->collisionStorage +
                runtime->collisionStorageSize ||
        leveldata == NULL) {
        return 0;
    }
    runtime->collisionReady = 1;
    return 1;
}

static int game_runtime_texture_directory(
    const char *bmd_path,
    char *directory,
    size_t directory_capacity)
{
    const char *slash = strrchr(bmd_path, '/');
    const char *backslash = strrchr(bmd_path, '\\');
    const char *separator = slash;
    const char suffix_forward[] = "tga/";
    const char suffix_backward[] = "tga\\";
    const char *suffix;
    size_t prefix_bytes;
    size_t suffix_bytes;

    if (separator == NULL ||
        (backslash != NULL && backslash > separator)) {
        separator = backslash;
    }
    prefix_bytes =
        separator != NULL
            ? (size_t)(separator - bmd_path + 1)
            : 0;
    suffix =
        separator != NULL && *separator == '\\'
            ? suffix_backward
            : suffix_forward;
    suffix_bytes = strlen(suffix);
    if (prefix_bytes + suffix_bytes + 1 >
        directory_capacity) {
        return 0;
    }
    if (prefix_bytes != 0) {
        memcpy(directory, bmd_path, prefix_bytes);
    }
    memcpy(
        directory + prefix_bytes,
        suffix,
        suffix_bytes + 1);
    return 1;
}

static int game_runtime_file_directory(
    const char *path,
    char *directory,
    size_t directory_capacity)
{
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *separator = slash;
    size_t directory_bytes;

    if (separator == NULL ||
        (backslash != NULL && backslash > separator)) {
        separator = backslash;
    }
    directory_bytes =
        separator != NULL
            ? (size_t)(separator - path + 1)
            : 0;
    if (directory_bytes + 1 > directory_capacity) {
        return 0;
    }
    if (directory_bytes != 0) {
        memcpy(directory, path, directory_bytes);
    }
    directory[directory_bytes] = '\0';
    return 1;
}

static int game_runtime_sibling_asset_path(
    const char *reference_path,
    const char *stem,
    const char *extension,
    char *path,
    size_t path_capacity)
{
    const char *slash;
    const char *backslash;
    const char *separator;
    size_t directory_bytes;
    size_t stem_bytes;
    size_t extension_bytes;

    if (reference_path == NULL ||
        stem == NULL ||
        extension == NULL ||
        path == NULL) {
        return 0;
    }
    slash = strrchr(reference_path, '/');
    backslash = strrchr(reference_path, '\\');
    separator = slash;
    if (separator == NULL ||
        (backslash != NULL && backslash > separator)) {
        separator = backslash;
    }
    directory_bytes =
        separator != NULL
            ? (size_t)(separator - reference_path + 1)
            : 0;
    stem_bytes = strlen(stem);
    extension_bytes = strlen(extension);
    if (directory_bytes + stem_bytes +
            extension_bytes + 1 >
        path_capacity) {
        return 0;
    }
    if (directory_bytes != 0) {
        memcpy(path, reference_path, directory_bytes);
    }
    memcpy(path + directory_bytes, stem, stem_bytes);
    memcpy(
        path + directory_bytes + stem_bytes,
        extension,
        extension_bytes + 1);
    return 1;
}

static JPBGameRuntimeTextureCache *
game_runtime_create_texture_cache(
    size_t capacity, int level_index)
{
    JPBGameRuntimeTextureCache *cache;

    if (capacity == 0 ||
        capacity >
            SIZE_MAX / sizeof(JPBGameRuntimeTexture)) {
        return NULL;
    }
    cache =
        (JPBGameRuntimeTextureCache *)calloc(
            1, sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->textures =
        (JPBGameRuntimeTexture *)calloc(
            capacity, sizeof(*cache->textures));
    if (cache->textures == NULL) {
        free(cache);
        return NULL;
    }
    cache->levelIndex = level_index;
    cache->textureCapacity = capacity;
    return cache;
}

static void game_runtime_free_texture_cache(
    JPBGameRuntimeTextureCache *cache)
{
    size_t texture;

    if (cache == NULL) {
        return;
    }
    for (texture = 0;
         texture < cache->textureCount;
         ++texture) {
        free(cache->textures[texture].pixels);
        free(cache->textures[texture].fileData);
    }
    free(cache->textures);
    free(cache);
}

static int game_runtime_texture_path(
    const JPBGameRuntimeTextureCache *cache,
    const char *texture_name,
    char *path,
    size_t path_capacity)
{
    const char *slash = strrchr(texture_name, '/');
    const char *backslash = strrchr(texture_name, '\\');
    const char *base_name = texture_name;
    const char *dot;
    size_t directory_bytes = strlen(cache->directory);
    size_t stem_bytes;

    if (slash != NULL) base_name = slash + 1;
    if (backslash != NULL && backslash + 1 > base_name) {
        base_name = backslash + 1;
    }
    dot = strrchr(base_name, '.');
    stem_bytes =
        dot != NULL
            ? (size_t)(dot - base_name)
            : strlen(base_name);
    if (stem_bytes == 0 ||
        directory_bytes + stem_bytes + sizeof(".tga") >
            path_capacity) {
        return 0;
    }
    memcpy(path, cache->directory, directory_bytes);
    memcpy(path + directory_bytes, base_name, stem_bytes);
    memcpy(
        path + directory_bytes + stem_bytes,
        ".tga",
        sizeof(".tga"));
    return 1;
}

static int game_runtime_load_texture(
    JPBGameRuntimeTextureCache *cache,
    JPBGameRuntimeTexture *entry,
    const char *texture_name)
{
    char path[JPB_GAME_RUNTIME_PATH_CAPACITY];
    JPBFileHandle file = 0;
    uint64_t file_size;
    JPBTgaView tga;
    _Material material;
    size_t pixel_count;
    size_t path_bytes;
    size_t path_index;
    size_t name_bytes = strlen(texture_name);

    if (name_bytes >= sizeof(entry->name) ||
        !game_runtime_texture_path(
            cache,
            texture_name,
            path,
            sizeof(path))) {
        return 0;
    }
    memcpy(entry->name, texture_name, name_bytes + 1);
    if (!file_OPEN(path, &file)) {
        return 0;
    }
    file_size = file_GETSIZE(&file);
    if (file_size == 0 || file_size > INT32_MAX) {
        (void)file_CLOSE(&file);
        return 0;
    }
    entry->fileData = (uint8_t *)malloc((size_t)file_size);
    if (entry->fileData == NULL) {
        (void)file_CLOSE(&file);
        return 0;
    }
    if (file_READ(
            &file,
            (char *)entry->fileData,
            (int32_t)file_size,
            JPB_FILE_READ_STREAM) != file_size) {
        (void)file_CLOSE(&file);
        free(entry->fileData);
        entry->fileData = NULL;
        return 0;
    }
    (void)file_CLOSE(&file);
    if (jpb_TgaInspect(
            entry->fileData,
            (size_t)file_size,
            &tga) != JPB_TGA_OK) {
        free(entry->fileData);
        entry->fileData = NULL;
        return 0;
    }
    pixel_count = (size_t)tga.width * (size_t)tga.height;
    if (pixel_count > SIZE_MAX / sizeof(uint32_t)) {
        free(entry->fileData);
        entry->fileData = NULL;
        return 0;
    }
    entry->pixels =
        (uint32_t *)malloc(pixel_count * sizeof(uint32_t));
    if (entry->pixels == NULL ||
        jpb_TgaDecodeA8R8G8B8(
            &tga,
            entry->pixels,
            pixel_count,
            tga.width) != JPB_TGA_OK) {
        free(entry->pixels);
        free(entry->fileData);
        entry->pixels = NULL;
        entry->fileData = NULL;
        return 0;
    }
    entry->texture.pixels = entry->pixels;
    entry->texture.width = tga.width;
    entry->texture.height = tga.height;
    entry->texture.stridePixels = tga.width;
    memset(&material, 0, sizeof(material));
    material.samplerType = TEXTURESAMPLER_LINEARCLAMP;
    material.colorOverride = -1;
    path_bytes = strlen(path);
    if (path_bytes >= sizeof(material.filename)) {
        free(entry->pixels);
        free(entry->fileData);
        entry->pixels = NULL;
        entry->fileData = NULL;
        return 0;
    }
    memcpy(material.filename, path, path_bytes + 1);
    for (path_index = 0; path_index < path_bytes; ++path_index) {
        if (material.filename[path_index] == '\\') {
            material.filename[path_index] = '/';
        }
    }
    SetTextureColorOverride(cache->levelIndex, &material);
    entry->texture.materialFlags = material.flags;
    entry->texture.samplerType = material.samplerType;
    entry->texture.colorOverride = material.colorOverride;
    return 1;
}

static int game_runtime_resolve_texture(
    void *user_data,
    const char *texture_name,
    JPBSoftwareTexture *texture)
{
    JPBGameRuntimeTextureCache *cache =
        (JPBGameRuntimeTextureCache *)user_data;
    JPBGameRuntimeTexture *entry;
    size_t index;

    if (cache == NULL || texture_name == NULL ||
        texture == NULL || texture_name[0] == '\0') {
        return 0;
    }
    for (index = 0; index < cache->textureCount; ++index) {
        entry = &cache->textures[index];
        if (strcmp(entry->name, texture_name) == 0) {
            if (entry->texture.pixels == NULL) {
                return 0;
            }
            *texture = entry->texture;
            return 1;
        }
    }
    if (cache->textureCount >=
        cache->textureCapacity) {
        return 0;
    }
    entry = &cache->textures[cache->textureCount++];
    memset(entry, 0, sizeof(*entry));
    if (!game_runtime_load_texture(
            cache, entry, texture_name)) {
        return 0;
    }
    ++cache->loadedTextureCount;
    *texture = entry->texture;
    return 1;
}

static void *game_runtime_load_material_texture(
    void *user_data,
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height)
{
    JPBGameRuntimeTextureCache *cache =
        (JPBGameRuntimeTextureCache *)user_data;
    JPBSoftwareTexture texture;
    const char *base_name;
    const char *slash;
    const char *backslash;
    size_t index;

    (void)option;
    if (cache == NULL || filename == NULL ||
        width == NULL || height == NULL) {
        return NULL;
    }
    slash = strrchr(filename, '/');
    backslash = strrchr(filename, '\\');
    base_name = filename;
    if (slash != NULL) {
        base_name = slash + 1;
    }
    if (backslash != NULL && backslash + 1 > base_name) {
        base_name = backslash + 1;
    }
    if (!game_runtime_resolve_texture(
            cache, base_name, &texture)) {
        return NULL;
    }
    for (index = 0; index < cache->textureCount; ++index) {
        JPBGameRuntimeTexture *entry = &cache->textures[index];

        if (strcmp(entry->name, base_name) == 0) {
            if (entry->texture.width > INT16_MAX ||
                entry->texture.height > INT16_MAX) {
                return NULL;
            }
            *width = (int16_t)entry->texture.width;
            *height = (int16_t)entry->texture.height;
            entry->texture.materialType = material_type;
            return &entry->texture;
        }
    }
    return NULL;
}

static int16_t game_runtime_i16(float value)
{
    if (value < -32768.0f) return -32768;
    if (value > 32767.0f) return 32767;
    return (int16_t)value;
}

static int32_t game_runtime_radians_to_angle(float radians)
{
    const float fixed_units_per_radian =
        4096.0f / 6.28318530717958647692f;

    return (int32_t)(radians * fixed_units_per_radian);
}

static int game_runtime_build_authored_camera(
    JPBGameRuntime *runtime)
{
    if (runtime == NULL || runtime->world == NULL ||
        !runtime->collisionReady) {
        return 0;
    }
    gCamera = runtime->camera;
    camera_SetCameras();
    runtime->camera = gCamera;
    runtime->authoredCameraDolly =
        runtime->world->currentDolly;
    runtime->cameraCollisionFraction = 1.0f;
    ++runtime->authoredCameraFrameCount;
    return 1;
}

/*
 * Mini2's recovered Kaadu owner (ai_Kadu) publishes gJarJarPos and selects
 * camera type 6 every gameplay tick. The portable player bridge does not yet
 * instantiate the two Kaadu mount actors, so preserve that exact camera
 * ownership with the live rider position instead of leaving the PDB global
 * at its zero-initialized address. The start table gives both Mini2 riders
 * the same x/z position; once the mount actors are present ai_Kadu supersedes
 * this boundary with its player-pair/maPhysicsData[2..3] calculation.
 */
static void game_runtime_publish_level_camera_owner(
    JPBGameRuntime *runtime)
{
    if (runtime == NULL || runtime->physics == NULL ||
        (int)(int8_t)LevelSelect != 12) {
        return;
    }
    gJarJarPos.vx = (int16_t)(int32_t)runtime->physics->pos.vx;
    gJarJarPos.vy = (int16_t)(int32_t)runtime->physics->pos.vy;
    gJarJarPos.vz = (int16_t)(int32_t)runtime->physics->pos.vz;
    camera_SetCurrentCameraType(6);
}

static int game_runtime_build_camera(JPBGameRuntime *runtime)
{
    enum {
        GAME_RUNTIME_CAMERA_FOCUS_HEIGHT = 0,
        GAME_RUNTIME_CAMERA_COLLISION_PADDING = 96
    };
    float horizontal = cosf(runtime->orbitPitch) * runtime->orbitDistance;
    FVECTOR camera_focus = {
        runtime->targetX,
        runtime->targetY +
            (float)GAME_RUNTIME_CAMERA_FOCUS_HEIGHT,
        runtime->targetZ
    };
    FVECTOR desired_eye;
    FVECTOR clipped_eye;
    float collision_fraction = 1.0f;
    float eye_x =
        camera_focus.vx +
        cosf(runtime->orbitYaw) * horizontal;
    float eye_y =
        camera_focus.vy +
        sinf(runtime->orbitPitch) * runtime->orbitDistance;
    float eye_z =
        camera_focus.vz +
        sinf(runtime->orbitYaw) * horizontal;
    float forward_x;
    float forward_y;
    float forward_z;
    float planar_distance =
        0.0f;

    /* The portable VS bridge still lacks the retail Arena camera setup that
     * precedes camera_SetCameraPos. Keep its dependency-light midpoint orbit
     * isolated here; ordinary gameplay remains owned by the authored camera. */
    if (!(GameStruct.versusModeFlag != 0 &&
          runtime->secondPlayerState != NULL) &&
        game_runtime_build_authored_camera(runtime)) {
        return 1;
    }
    desired_eye.vx = eye_x;
    desired_eye.vy = eye_y;
    desired_eye.vz = eye_z;
    if (jpb_SoftwareClipCameraToJpx(
            &runtime->scene,
            &camera_focus,
            &desired_eye,
            (float)GAME_RUNTIME_CAMERA_COLLISION_PADDING,
            &clipped_eye,
            &collision_fraction) !=
        JPB_SOFTWARE_RENDER_OK) {
        return 0;
    }
    runtime->cameraCollisionFraction =
        collision_fraction;
    if (collision_fraction < 1.0f) {
        ++runtime->cameraCollisionFrameCount;
    }
    eye_x = clipped_eye.vx;
    eye_y = clipped_eye.vy;
    eye_z = clipped_eye.vz;
    forward_x = camera_focus.vx - eye_x;
    forward_y = camera_focus.vy - eye_y;
    forward_z = camera_focus.vz - eye_z;
    planar_distance =
        sqrtf(forward_x * forward_x + forward_z * forward_z);

    memset(&runtime->camera, 0, sizeof(runtime->camera));
    runtime->camera.viewType = JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    runtime->camera.focus.vx = game_runtime_i16(eye_x);
    runtime->camera.focus.vy = game_runtime_i16(eye_y);
    runtime->camera.focus.vz = game_runtime_i16(eye_z);
    runtime->camera.focusDest = runtime->camera.focus;
    runtime->camera.angle.vx = game_runtime_radians_to_angle(
        -atan2f(forward_y, planar_distance));
    runtime->camera.angle.vy = game_runtime_radians_to_angle(
        atan2f(-forward_x, -forward_z));
    runtime->camera.angleDest = runtime->camera.angle;
    /* camera_StuffCamera publishes movement-relative yaw with the opposite
     * sign from Camera.angleDest.vy. The Arena bridge has no retail dolly
     * owner yet, so it must preserve that exact global relationship here. */
    mCameraAngleDest = -runtime->camera.angleDest.vy;
    return 1;
}

static int game_runtime_companion_path(
    const char *cad_path,
    const char *file_name,
    char *path,
    size_t path_capacity)
{
    const char *slash = strrchr(cad_path, '/');
    const char *backslash = strrchr(cad_path, '\\');
    const char *separator = slash;
    size_t directory_bytes;
    size_t file_name_bytes = strlen(file_name);

    if (separator == NULL ||
        (backslash != NULL && backslash > separator)) {
        separator = backslash;
    }
    directory_bytes =
        separator != NULL
            ? (size_t)(separator - cad_path + 1)
            : 0;
    if (directory_bytes + file_name_bytes + 1 >
        path_capacity) {
        return 0;
    }
    if (directory_bytes != 0) {
        memcpy(path, cad_path, directory_bytes);
    }
    memcpy(
        path + directory_bytes,
        file_name,
        file_name_bytes + 1);
    return 1;
}

static int game_runtime_ai_path(
    const char *cad_path,
    int ai_level,
    char *path,
    size_t path_capacity)
{
    const char *res = NULL;
    const char *file_name;
    const char *extension;
    size_t path_bytes = strlen(cad_path);
    size_t index;
    size_t prefix_bytes;
    size_t stem_bytes;
    char separator;
    const char *directory = "AI";

    if (ai_level < 0 ||
        ai_level >= JPB_AI_LEVEL_CAPACITY) {
        return 0;
    }
    for (index = 0; index + 4 < path_bytes; ++index) {
        const char *candidate = cad_path + index;

        if (game_runtime_is_separator(candidate[0]) &&
            game_runtime_ascii_lower(candidate[1]) == 'r' &&
            game_runtime_ascii_lower(candidate[2]) == 'e' &&
            game_runtime_ascii_lower(candidate[3]) == 's' &&
            game_runtime_is_separator(candidate[4])) {
            res = candidate;
            break;
        }
    }
    if (res == NULL) {
        return 0;
    }
    file_name = strrchr(cad_path, '/');
    {
        const char *backslash = strrchr(cad_path, '\\');

        if (file_name == NULL ||
            (backslash != NULL && backslash > file_name)) {
            file_name = backslash;
        }
    }
    file_name =
        file_name == NULL ? cad_path : file_name + 1;
    extension = strrchr(file_name, '.');
    if (extension == NULL || extension == file_name) {
        return 0;
    }
    prefix_bytes = (size_t)(res - cad_path + 5);
    stem_bytes = (size_t)(extension - file_name);
    if (prefix_bytes + strlen(directory) + 1 +
            stem_bytes + 3 + 1 >
        path_capacity) {
        return 0;
    }
    separator = res[4];
    memcpy(path, cad_path, prefix_bytes);
    memcpy(
        path + prefix_bytes,
        directory,
        strlen(directory));
    path[prefix_bytes + strlen(directory)] =
        separator;
    memcpy(
        path + prefix_bytes + strlen(directory) + 1,
        file_name,
        stem_bytes);
    path[prefix_bytes + strlen(directory) + 1 +
         stem_bytes] = '.';
    path[prefix_bytes + strlen(directory) + 1 +
         stem_bytes + 1] = '0';
    path[prefix_bytes + strlen(directory) + 1 +
         stem_bytes + 2] = (char)('1' + ai_level);
    path[prefix_bytes + strlen(directory) + 1 +
         stem_bytes + 3] = '\0';
    return 1;
}

static int game_runtime_load_huffman(
    JPBGameRuntime *runtime, const char *cad_path)
{
    char table_path[JPB_GAME_RUNTIME_PATH_CAPACITY];
    char value_path[JPB_GAME_RUNTIME_PATH_CAPACITY];
    char option_path[JPB_GAME_RUNTIME_PATH_CAPACITY];

    if (!game_runtime_companion_path(
            cad_path,
            "huffman.tab",
            table_path,
            sizeof(table_path)) ||
        !game_runtime_companion_path(
            cad_path,
            "huffman.val",
            value_path,
            sizeof(value_path)) ||
        !game_runtime_companion_path(
            cad_path,
            "huffman.opt",
            option_path,
            sizeof(option_path))) {
        return 0;
    }
    runtime->huffmanTables =
        (JPBHuffmanTableSet *)malloc(
            sizeof(*runtime->huffmanTables));
    if (runtime->huffmanTables == NULL) {
        return 0;
    }
    if (jpb_HuffmanLoadFiles(
            table_path,
            value_path,
            option_path,
            runtime->huffmanTables) != JPB_HUFFMAN_OK) {
        return 0;
    }
    jpb_HuffmanUseTables(runtime->huffmanTables);
    return 1;
}

static int game_runtime_publish_authored_frame(
    JPBGameRuntime *runtime, int32_t previous_index)
{
    _animFrame *decoded_frame;

    if (!runtime->authoredMotionReady ||
        runtime->animation->pCurrentAnimSeq == NULL) {
        return 1;
    }
    decoded_frame = runtime->animation->pCurrentAnimFrame;
    if (decoded_frame == NULL) {
        return 0;
    }
    if (decoded_frame ==
        &runtime->animation->tweenAnimFrame) {
        ++runtime->authoredTweenFrameCount;
    }
    if (runtime->actorModel.pRootNode != NULL &&
        jpb_ModelPublishAnimFrame(
            &runtime->actorModel,
            decoded_frame,
            &runtime->actorRoot) != JPB_MODEL_POSE_OK) {
        return 0;
    }
    if (runtime->actorModel.pRootNode != NULL) {
        size_t node_index;
        uint32_t hot_nodes = 0;

        runtime->authoredPoseReady = 1;
        for (node_index = 0;
             node_index < runtime->bmdView.node_count;
             ++node_index) {
            Mnode *node = coll_GetNode(
                0, (unsigned)node_index);

            if (node != NULL &&
                (node->flags & JPB_COLLISION_FLAG_HOT) != 0) {
                ++hot_nodes;
            }
        }
        if (hot_nodes != 0) {
            ++runtime->authoredHotFrameCount;
            if (hot_nodes >
                runtime->authoredHotNodePeak) {
                runtime->authoredHotNodePeak = hot_nodes;
            }
        }
    }
    runtime->authoredFrameReady = 1;
    if (runtime->animation->animFrameIndex != previous_index) {
        ++runtime->decodedFrameCount;
    }
    return 1;
}

static int game_runtime_publish_second_player_frame(
    JPBGameRuntime *runtime, int32_t previous_index)
{
    JPBGameRuntimeSecondPlayerState *state;
    _animFrame *decoded_frame;

    if (runtime == NULL || runtime->secondPlayerState == NULL) {
        return 1;
    }
    state = runtime->secondPlayerState;
    if (!state->authoredMotionReady ||
        state->animation == NULL ||
        state->animation->pCurrentAnimSeq == NULL) {
        return 1;
    }
    decoded_frame = state->animation->pCurrentAnimFrame;
    if (decoded_frame == NULL) {
        return 0;
    }
    if (state->model.pRootNode != NULL &&
        jpb_ModelPublishAnimFrame(
            &state->model,
            decoded_frame,
            &runtime->inactivePlayerActorRoot) !=
            JPB_MODEL_POSE_OK) {
        return 0;
    }
    state->authoredPoseReady =
        state->model.pRootNode != NULL;
    state->authoredFrameReady = 1;
    if (state->animation->animFrameIndex != previous_index) {
        ++state->decodedFrameCount;
    }
    return 1;
}

static int game_runtime_publish_enemy_frame(
    JPBGameRuntimeEnemyActor *actor, int32_t previous_index)
{
    _animFrame *decoded_frame;

    if (!actor->authoredMotionReady ||
        actor->animation->pCurrentAnimSeq == NULL) {
        return 1;
    }
    decoded_frame = actor->animation->pCurrentAnimFrame;
    if (decoded_frame == NULL) {
        return 0;
    }
    if (actor->model.pRootNode != NULL &&
        jpb_ModelPublishAnimFrame(
            &actor->model,
            decoded_frame,
            &actor->actorRoot) != JPB_MODEL_POSE_OK) {
        return 0;
    }
    if (actor->model.pRootNode != NULL) {
        actor->authoredPoseReady = 1;
    }
    actor->authoredFrameReady = 1;
    if (actor->animation->animFrameIndex != previous_index) {
        ++actor->decodedFrameCount;
    }
    return 1;
}

static int game_runtime_enemy_is_active(
    const wsl_ENEMY *enemy)
{
    const Node *node;

    if (enemy == NULL) {
        return 0;
    }
    for (node = enemyList[mCurEnemyList].head;
         node != NULL;
         node = node->next) {
        if (node == &enemy->node) {
            return 1;
        }
    }
    return 0;
}

static int game_runtime_prepare_depth_buffer(
    JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer)
{
    size_t required;
    float *resized;

    if (runtime == NULL || framebuffer == NULL ||
        depth_buffer == NULL ||
        framebuffer->width <= 0 ||
        framebuffer->height <= 0 ||
        framebuffer->stridePixels <
            framebuffer->width ||
        (size_t)framebuffer->height >
            SIZE_MAX /
                (size_t)framebuffer->stridePixels) {
        return 0;
    }
    required =
        (size_t)framebuffer->height *
        (size_t)framebuffer->stridePixels;
    if (required > runtime->renderDepthCapacity) {
        if (required > SIZE_MAX / sizeof(float)) {
            return 0;
        }
        resized =
            (float *)realloc(
                runtime->renderDepthBuffer,
                required * sizeof(float));
        if (resized == NULL) {
            return 0;
        }
        runtime->renderDepthBuffer = resized;
        runtime->renderDepthCapacity = required;
    }
    depth_buffer->values =
        runtime->renderDepthBuffer;
    depth_buffer->width =
        (size_t)framebuffer->width;
    depth_buffer->height =
        (size_t)framebuffer->height;
    depth_buffer->strideValues =
        (size_t)framebuffer->stridePixels;
    return 1;
}

static int game_runtime_snapshot_glow_depth_buffer(
    JPBGameRuntime *runtime,
    const JPBSoftwareDepthBuffer *source,
    JPBSoftwareDepthBuffer *snapshot)
{
    size_t required;
    float *resized;

    if (runtime == NULL ||
        source == NULL ||
        snapshot == NULL ||
        source->values == NULL ||
        source->width == 0 ||
        source->height == 0 ||
        source->strideValues < source->width ||
        source->height > SIZE_MAX / source->strideValues) {
        return 0;
    }
    required = source->height * source->strideValues;
    if (required > runtime->glowDepthCapacity) {
        if (required > SIZE_MAX / sizeof(float)) {
            return 0;
        }
        resized =
            (float *)realloc(
                runtime->glowDepthBuffer,
                required * sizeof(float));
        if (resized == NULL) {
            return 0;
        }
        runtime->glowDepthBuffer = resized;
        runtime->glowDepthCapacity = required;
    }
    memcpy(
        runtime->glowDepthBuffer,
        source->values,
        required * sizeof(float));
    snapshot->values = runtime->glowDepthBuffer;
    snapshot->width = source->width;
    snapshot->height = source->height;
    snapshot->strideValues = source->strideValues;
    return 1;
}

static void game_runtime_measure_hot_node_distance(
    JPBGameRuntime *runtime,
    const JPBGameRuntimeEnemyActor *actor)
{
    int attacker_index;

    if (runtime->player == NULL ||
        actor == NULL ||
        actor->player == NULL ||
        runtime->player->paNodesSizes == NULL ||
        actor->player->paNodesSizes == NULL) {
        return;
    }
    for (attacker_index = 0;
         attacker_index < runtime->player->numCollisionNodes;
         ++attacker_index) {
        CollisionData *attacker_data =
            &runtime->player->paNodesSizes[attacker_index];
        Mnode *attacker_node = coll_GetNode(
            runtime->player->playernum,
            (unsigned)(uint8_t)attacker_data->id);
        Mnode *hot_node =
            attacker_data->parentid == -1
                ? attacker_node
                : coll_GetNode(
                      runtime->player->playernum,
                      (unsigned)(uint8_t)
                          attacker_data->parentid);
        int target_index;

        if (attacker_node == NULL ||
            hot_node == NULL ||
            attacker_data->radius1 <= 0 ||
            (hot_node->flags &
             JPB_COLLISION_FLAG_HOT) == 0) {
            continue;
        }
        for (target_index = 0;
             target_index <
                 actor->player->numCollisionNodes;
            ++target_index) {
            CollisionData *target_data =
                &actor->player
                     ->paNodesSizes[target_index];
            Mnode *target_node = coll_GetNode(
                actor->player->playernum,
                (unsigned)(uint8_t)target_data->id);
            uint32_t distance;
            int32_t radius;
            unsigned attacker_node_id =
                (unsigned)(uint8_t)attacker_data->id;

            if (target_node == NULL ||
                target_data->radius1 <= 0) {
                continue;
            }
            distance = vec_DistanceLV(
                &attacker_node->v3RotCenter,
                &target_node->v3RotCenter);
            radius =
                ((int32_t)attacker_data->radius1 *
                     runtime->player->fScale / JPB_FIXED_ONE +
                 (int32_t)target_data->radius1 *
                     actor->player->fScale /
                     JPB_FIXED_ONE) /
                2;
            if (runtime->closestHotTargetNodeDistance == 0 ||
                distance <
                    runtime->closestHotTargetNodeDistance) {
                runtime->closestHotTargetNodeDistance =
                    distance;
                runtime->closestHotTargetCollisionRadius =
                    radius;
                runtime->closestHotNodeId =
                    attacker_data->id;
                runtime->closestTargetNodeId =
                    target_data->id;
            }
            if (attacker_node_id <
                    JPB_COLLISION_NODE_CAPACITY &&
                (runtime->
                         closestHotTargetDistanceByNode[
                             attacker_node_id] == 0 ||
                 distance <
                     runtime->
                         closestHotTargetDistanceByNode[
                             attacker_node_id])) {
                runtime->closestHotTargetDistanceByNode[
                    attacker_node_id] = distance;
                runtime->hotTargetCollisionRadiusByNode[
                    attacker_node_id] = radius;
                runtime->closestTargetNodeByHotNode[
                    attacker_node_id] = target_data->id;
            }
        }
    }
}

int jpb_GameRuntimeInit(
    JPBGameRuntime *runtime, const char *jpx_path)
{
    return jpb_GameRuntimeInitWithCad(
        runtime, jpx_path, NULL);
}

int jpb_GameRuntimeInitWithCad(
    JPBGameRuntime *runtime,
    const char *jpx_path,
    const char *cad_path)
{
    return jpb_GameRuntimeInitWithAssets(
        runtime, jpx_path, cad_path, NULL);
}

int jpb_GameRuntimeInitWithAssets(
    JPBGameRuntime *runtime,
    const char *jpx_path,
    const char *cad_path,
    const char *bmd_path)
{
    return jpb_GameRuntimeInitWithPlayerAssets(
        runtime, jpx_path, cad_path, bmd_path, 0);
}

int jpb_GameRuntimeInitWithPlayerAssets(
    JPBGameRuntime *runtime,
    const char *jpx_path,
    const char *cad_path,
    const char *bmd_path,
    int player_model_id)
{
    JPBJpxLoadConfig config;
    float span_x;
    float span_y;
    float span_z;
    float span;
    const char *resident_sprite_path;
    const char *default_texture_path;
    int level_index;
    int result;

    if (runtime == NULL || jpx_path == NULL ||
        player_model_id < 0 ||
        player_model_id >= JPB_MODEL_NAME_COUNT) {
        game_runtime_set_failure_stage("init:invalid-arguments");
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    game_runtime_set_failure_stage("none");
    memset(runtime, 0, sizeof(*runtime));
    runtime->meshStorage =
        (uint8_t *)malloc(JPB_JPX_REFERENCE_WORLD_CAPACITY);
    if (runtime->meshStorage == NULL) {
        game_runtime_set_failure_stage("init:mesh-storage");
        return JPB_GAME_RUNTIME_OUT_OF_MEMORY;
    }
    memset(&config, 0, sizeof(config));
    config.storage = runtime->meshStorage;
    config.storageCapacity = JPB_JPX_REFERENCE_WORLD_CAPACITY;
    level_index = jpb_LevelIndexFromPath(jpx_path);
    if (level_index != JPB_LEVEL_INDEX_NONE) {
        config.levelName = sLevelNames[level_index];
    }
    result = jpx_LoadFile(jpx_path, &config, &runtime->meshView);
    if (result != JPB_JPX_OK) {
        return game_runtime_fail(
            runtime, "init:jpx-load", JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    runtime->worldTextureCache =
        game_runtime_create_texture_cache(
            runtime->meshView.numMaterials,
            level_index);
    if (runtime->worldTextureCache == NULL) {
        return game_runtime_fail(
            runtime, "init:world-texture-cache",
            JPB_GAME_RUNTIME_OUT_OF_MEMORY);
    }
    if (!game_runtime_file_directory(
            jpx_path,
            runtime->worldTextureCache->directory,
            sizeof(
                runtime->worldTextureCache
                    ->directory))) {
        return game_runtime_fail(
            runtime, "init:world-texture-directory",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    result = jpb_SoftwarePrepareJpxLevelScene(
        &runtime->meshView, level_index, &runtime->scene);
    if (result != JPB_SOFTWARE_RENDER_OK) {
        return game_runtime_fail(
            runtime, "init:prepare-level-scene",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    if (!game_runtime_prepare_archive_memory() ||
        !game_runtime_set_effect_directory(jpx_path)) {
        return game_runtime_fail(
            runtime, "init:archive-or-effect-directory",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    if (level_index != JPB_LEVEL_INDEX_NONE) {
        LevelSelect = (char)level_index;
        cube_InitVisibility();
        ClearCachedTextureIndices();
        if (!game_runtime_load_collision(
                runtime, jpx_path, level_index) ||
            !game_runtime_load_authored_cameras(
                runtime, jpx_path, level_index)) {
            return game_runtime_fail(
                runtime, "init:collision-or-cameras",
                JPB_GAME_RUNTIME_LOAD_FAILED);
        }
    }

    if (level_index != JPB_LEVEL_INDEX_NONE) {
        /*
         * player_RefreshPlayer converts the exact startPos table into fixed
         * game units in this order. Starting the runtime here also provides
         * an independent visual check of the recovered FBX/JPX placement.
         */
        runtime->targetX =
            (128.0f - (float)startPos[level_index][0].vx) * 256.0f;
        runtime->targetY =
            (float)startPos[level_index][0].vz * 256.0f;
        runtime->targetZ =
            ((float)startPos[level_index][0].vy - 127.0f) * 256.0f;
    } else {
        runtime->targetX =
            (runtime->scene.minX + runtime->scene.maxX) * 0.5f;
        runtime->targetY =
            (runtime->scene.minY + runtime->scene.maxY) * 0.5f;
        runtime->targetZ =
            (runtime->scene.minZ + runtime->scene.maxZ) * 0.5f;
    }
    span_x = runtime->scene.maxX - runtime->scene.minX;
    span_y = runtime->scene.maxY - runtime->scene.minY;
    span_z = runtime->scene.maxZ - runtime->scene.minZ;
    span = span_x;
    if (span_y > span) span = span_y;
    if (span_z > span) span = span_z;
    if (span < 1.0f) span = 1.0f;
    if (level_index != JPB_LEVEL_INDEX_NONE) {
        /*
         * Camera.focus is an exact 16-bit game type. A whole-level orbit no
         * longer fits after applying the real 256-unit scale, so use a
         * gameplay-sized inspection orbit around the exact player spawn.
         */
        runtime->orbitYaw = 2.3f;
        runtime->orbitPitch = 0.35f;
        runtime->orbitDistance = 4000.0f;
        runtime->minimumOrbitDistance = 1024.0f;
        runtime->maximumOrbitDistance = 12000.0f;
    } else {
        runtime->orbitYaw = -0.85f;
        runtime->orbitPitch = 0.45f;
        runtime->orbitDistance = span * 1.3f;
        runtime->minimumOrbitDistance = span * 0.2f;
        if (runtime->minimumOrbitDistance < 8.0f) {
            runtime->minimumOrbitDistance = 8.0f;
        }
        runtime->maximumOrbitDistance = span * 4.0f;
        if (runtime->maximumOrbitDistance < 64.0f) {
            runtime->maximumOrbitDistance = 64.0f;
        }
    }
    memset(&runtime->environment, 0, sizeof(runtime->environment));
    scene_gInitRoot();
    runtime->uiTextureCache =
        game_runtime_create_texture_cache(
            JPB_RESIDENT_SPRITE_COUNT,
            level_index);
    resident_sprite_path = resource_getPath(
        "a_credit.tga",
        JPB_RESOURCE_EFFECT_TEXTURE);
    if (runtime->uiTextureCache == NULL ||
        resident_sprite_path == NULL ||
        !game_runtime_file_directory(
            resident_sprite_path,
            runtime->uiTextureCache->directory,
            sizeof(runtime->uiTextureCache->directory))) {
        return game_runtime_fail(
            runtime, "init:ui-texture-directory",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    runtime->defaultTextureCache =
        game_runtime_create_texture_cache(4, level_index);
    default_texture_path = resource_getPath(
        "a_water.tga", JPB_RESOURCE_DEFAULT);
    if (runtime->defaultTextureCache == NULL ||
        default_texture_path == NULL ||
        !game_runtime_file_directory(
            default_texture_path,
            runtime->defaultTextureCache->directory,
            sizeof(runtime->defaultTextureCache->directory))) {
        return game_runtime_fail(
            runtime, "init:default-texture-directory",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    sprite_gInitSprites();
    gFileNotFound = 0;
    jpb_TextureSetPlatformHooks(
        game_runtime_load_material_texture,
        NULL,
        runtime->uiTextureCache);
    file_SetChunkLoadHooks(
        game_runtime_resolve_resource, NULL);
    file_SetTextureLoadHook(
        game_runtime_load_resident_texture);
    file_LoadEffects();
    file_LoadResidentSprites();
    file_SetTextureLoadHook(NULL);
    file_SetChunkLoadHooks(NULL, NULL);
    if (gFileNotFound != 0 || paEffects[0] == NULL ||
        effects1Handle[40] == NULL ||
        effects1Handle[45] == NULL ||
        effects1Handle[49] == NULL) {
        return game_runtime_fail(
            runtime, "init:effects-or-resident-sprites",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    bullet_InitProjectilePool();
    ClearInput();
    maskPadBits(0);
    scene_gInitScenes(0);
    physics_gInitObjects(0);
    anim_InitAnimations(0);
    player_gInitPlayers(0);
    model_InitModels();
    runtime->actorScene = scene_gGetNewSceneObject(0);
    runtime->physics = physics_gGetNewObject(0);
    runtime->player = NULL;
    runtime->inactivePlayerScene =
        scene_gGetNewSceneObject(1);
    runtime->inactivePlayerPhysics =
        physics_gGetNewObject(1);
    runtime->inactivePlayer =
        player_gGetNewPlayerObject(1);
    if (runtime->actorScene == NULL ||
        runtime->physics == NULL ||
        runtime->inactivePlayerScene == NULL ||
        runtime->inactivePlayerPhysics == NULL ||
        runtime->inactivePlayer == NULL) {
        return game_runtime_fail(
            runtime, "init:actor-or-player-slot",
            JPB_GAME_RUNTIME_OUT_OF_MEMORY);
    }
    runtime->actorRoot.objectID = 0;
    memcpy(
        runtime->actorRoot.objectName,
        "ACTOR",
        sizeof("ACTOR"));
    runtime->actorModel.modelRoot.objectID = 0;
    memcpy(
        runtime->actorModel.modelRoot.objectName,
        "MODEL",
        sizeof("MODEL"));
    obj_gSetChildObject(
        runtime->actorScene, &runtime->actorRoot, 0);
    obj_gSetChildObject(
        runtime->actorScene, &runtime->actorModel.modelRoot, 1);
    obj_gSetChildObject(
        runtime->actorScene, &runtime->physics->physicsRoot, 2);
    /*
     * The exact AI target selectors always dereference both world player
     * slots. Preserve the original single-player invariant with a valid,
     * inactive actor in reserved object slot 1. Both the actor root and the
     * player component carry the original inactive flag so every owner,
     * including player_gProcessPlayers, rejects it before dereferencing
     * unbound animation state.
     */
    runtime->inactivePlayerActorRoot.objectID = 1;
    runtime->inactivePlayerActorRoot.flags =
        UINT32_C(0x20);
    runtime->inactivePlayer->playerRoot.flags =
        UINT32_C(0x20);
    memcpy(
        runtime->inactivePlayerActorRoot.objectName,
        "ACTOR",
        sizeof("ACTOR"));
    obj_gSetChildObject(
        runtime->inactivePlayerScene,
        &runtime->inactivePlayerActorRoot,
        0);
    obj_gSetChildObject(
        runtime->inactivePlayerScene,
        &runtime->inactivePlayerPhysics->physicsRoot,
        2);
    obj_gSetChildObject(
        runtime->inactivePlayerScene,
        &runtime->inactivePlayer->playerRoot,
        4);
    /*
     * The exact game-system owner populates the callback table before
     * loader_CreateCharacter invokes ai_InitPlayer.
     */
    game_setFuncArray();
    jpb_PhysicsSetGeometryStreamResolver(
        game_runtime_resolve_geometry_stream, runtime);
    if (bmd_path != NULL) {
        runtime->textureCache =
            game_runtime_create_texture_cache(
                JPB_GAME_RUNTIME_MODEL_TEXTURE_CAPACITY,
                level_index);
        if (runtime->textureCache == NULL) {
            return game_runtime_fail(
                runtime, "init:model-texture-cache",
                JPB_GAME_RUNTIME_OUT_OF_MEMORY);
        }
        if (!game_runtime_texture_directory(
                bmd_path,
                runtime->textureCache->directory,
                sizeof(runtime->textureCache->directory))) {
            return game_runtime_fail(
                runtime, "init:model-texture-directory",
                JPB_GAME_RUNTIME_LOAD_FAILED);
        }
        runtime->bmdStorage =
            (uint8_t *)malloc(JPB_BMD_REFERENCE_CAPACITY);
        if (runtime->bmdStorage == NULL) {
            return game_runtime_fail(
                runtime, "init:bmd-storage",
                JPB_GAME_RUNTIME_OUT_OF_MEMORY);
        }
        if (jpb_BmdLoadFile(
                bmd_path,
                runtime->bmdStorage,
                JPB_BMD_REFERENCE_CAPACITY,
                &runtime->bmdView) != JPB_BMD_OK ||
            !game_runtime_build_model(
                &runtime->bmdView,
                "PLAYER",
                0,
                &runtime->actorModel,
                runtime->textureCache)) {
            return game_runtime_fail(
                runtime, "init:bmd-load-or-build-player-model",
                JPB_GAME_RUNTIME_LOAD_FAILED);
        }
    }
    if (cad_path != NULL) {
        if (!game_runtime_load_huffman(runtime, cad_path)) {
            return game_runtime_fail(
                runtime, "init:huffman-load",
                JPB_GAME_RUNTIME_LOAD_FAILED);
        }
        runtime->cadStorage =
            (uint8_t *)malloc(JPB_CAD_REFERENCE_CAPACITY);
        if (runtime->cadStorage == NULL) {
            return game_runtime_fail(
                runtime, "init:cad-storage",
                JPB_GAME_RUNTIME_OUT_OF_MEMORY);
        }
        if (jpb_CadLoadFile(
                cad_path,
                runtime->cadStorage,
                JPB_CAD_REFERENCE_CAPACITY,
                &runtime->cadView) != JPB_CAD_OK ||
            runtime->cadView.sequence_count < 2 ||
            runtime->cadView.sequence_count > INT16_MAX) {
            return game_runtime_fail(
                runtime, "init:cad-load",
                JPB_GAME_RUNTIME_LOAD_FAILED);
        }
        runtime->animation = anim_CreateObject(
            runtime->actorScene,
            runtime->cadView.payload,
            NULL,
            0);
        if (runtime->animation == NULL) {
            return game_runtime_fail(
                runtime, "init:animation-create",
                JPB_GAME_RUNTIME_LOAD_FAILED);
        }
    } else {
        runtime->animation = &maAnimationData[0];
        runtime->animation->animRoot.objectID = 0;
        obj_gSetChildObject(
            runtime->actorScene,
            &runtime->animation->animRoot,
            3);
    }
    runtime->player = player_gCreateObject(
        runtime->actorScene,
        player_model_id,
        bmd_path != NULL
            ? jedi_InitPlayer
            : NULL);
    if (runtime->player == NULL) {
        return game_runtime_fail(
            runtime, "init:player-create",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    if (cad_path != NULL) {
        player_gConnectMotionData(
            runtime->player,
            runtime->cadView.payload);
        if (runtime->player->paMotions !=
                runtime->cadView.motions ||
            runtime->player->maxMotions !=
                (int16_t)runtime->cadView.sequence_count) {
            return game_runtime_fail(
                runtime, "init:connect-motion-data",
                JPB_GAME_RUNTIME_LOAD_FAILED);
        }
        runtime->player->oldmaxCMotions =
            (int16_t)runtime->cadView.sequence_count;
        runtime->authoredMotionReady = 1;
    }
    /*
     * Single-player still owns a valid inactive player-1 slot. Match the
     * original player-pair target invariant until combat/collision owners
     * select a live opponent; enemy asset creation is not a target selector.
     */
    runtime->player->target = runtime->inactivePlayer;
    runtime->inactivePlayer->target = runtime->player;
    memset(&GameStruct, 0, sizeof(GameStruct));
    game_setDefaultOptions();
    generateAllText(OptionStruct.Language);
    newGameGameInit();
    /*
     * Enter the same recovered title-selection ownership used by the retail
     * flow instead of writing player-count/model globals independently.
     */
    menu_setNumPlayers(1);
    menuVars.pplayers[0] = 0;
    menu_setPlayer(0, (unsigned)player_model_id);
    GameStruct.CurrentLevel =
        (uint8_t)(level_index == JPB_LEVEL_INDEX_NONE
                      ? 0
                      : level_index);
    if (level_index != JPB_LEVEL_INDEX_NONE) {
        pwrup_LoadPoop();
        pwrup_Init();
        runtime->powerupCount = jpb_PwrupLoadedCount();
        runtime->checkpointCount =
            (size_t)(maxCheckPoints > 0
                         ? maxCheckPoints - 1
                         : 0);
        if (runtime->powerupCount == 0) {
            return game_runtime_fail(
                runtime, "init:powerups",
                JPB_GAME_RUNTIME_LOAD_FAILED);
        }
    }
    /*
     * game_setDefaultOptions selects normal difficulty, after which exact
     * game_initPerLevel copies the current level's five authored tuning
     * bytes. Use that reviewed data path here instead of a runtime constant.
     */
    if (!jpb_game_ApplyLevelDifficulty(
            (unsigned)GameStruct.CurrentLevel, 1)) {
        return game_runtime_fail(
            runtime, "init:level-difficulty",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    gGlobalTimer = 0;
    totalframes = 0;
    /*
     * loader_LevelLoad enters the exact PDB-named refresh owner after the
     * character, motion data, level, and difficulty have all been selected.
     * That owner does considerably more than place the actor: it applies the
     * authored per-level facing, resets gameplay callbacks and transient
     * state, initializes energy/Force from the selected model, and seeds the
     * idle animation queue.  The former portable shortcut duplicated only
     * position and Motion[0], leaving FED facing backwards and starting the
     * controller from an invalid animation/reset state.
     */
    player_RefreshPlayer(runtime->player);
    runtime->targetX = runtime->physics->pos.vx;
    runtime->targetY = runtime->physics->pos.vy;
    runtime->targetZ = runtime->physics->pos.vz;
    result = jpb_PhysicsUpdateSceneObject(runtime->physics);
    if (result != JPB_PHYSICS_PARTIAL_OK) {
        return game_runtime_fail(
            runtime, "init:physics-update-player",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    runtime->player->playerPad.padnum = 0;
    /* Preserve player_gCreateObject's exact edge/held channel masks. */
    runtime->player->playerPad.mask0 = 0;
    runtime->player->playerPad.mask1 = UINT32_MAX;
    initialLevelPauseDelay = 0;
    memset(&runtime->camera, 0, sizeof(runtime->camera));
    /*
     * scene_middleRender calls camera_SetCameras before building the view
     * matrix. Publish the exact scene-init defaults here and let that owner
     * resolve the player's authored collision dolly on the first frame.
     */
    runtime->camera.viewType = UINT32_C(0x0901);
    gCamera = runtime->camera;
    newcameraflag = 1;
    camera_SetCurrentCameraType(1);
    gSCENE_READY = runtime->collisionReady != 0;
    /*
     * loader_LevelLoad owns fx_Init in the reference. The portable runtime
     * currently enters after that loader boundary, so install the matching
     * default-resource cache and perform the exact effect initialization
     * before the first live scene frame.
     */
    jpb_TextureSetPlatformHooks(
        game_runtime_load_material_texture,
        NULL,
        runtime->defaultTextureCache);
    fx_Init();
    if (!fx_DefaultTexturesReady()) {
        return game_runtime_fail(
            runtime, "init:fx-default-textures",
            JPB_GAME_RUNTIME_LOAD_FAILED);
    }
    jpb_WHookSetDrawTextureHook(
        game_runtime_capture_draw_texture, runtime);
    runtime->drawTextureHookReady = 1;
    jpb_WHookSetScreenPolyHook(
        game_runtime_capture_screen_poly, runtime);
    runtime->screenPolyHookReady = 1;
    jpb_GameSetBarHook(game_runtime_capture_bar, runtime);
    runtime->barHookReady = 1;
    jpb_PlayerSetTileHook(
        game_runtime_capture_player_tile, runtime);
    runtime->playerTileHookReady = 1;
    jpb_TextSetDrawHook(
        game_runtime_capture_text, runtime);
    jpb_TextSetPsxTextureHook(
        game_runtime_capture_psx_texture, runtime);
    runtime->textHookReady = 1;
    jpb_DebugTextSetDraw3dHook(
        game_runtime_capture_draw3d_text, runtime);
    runtime->draw3dTextHookReady = 1;
    jpb_SpriteSetDisplayHook(
        game_runtime_capture_sprite_display, runtime);
    runtime->spriteDisplayHookReady = 1;
    jpb_FxSetScreenGlowHook(
        game_runtime_capture_screen_glow, runtime);
    runtime->glowHookReady = 1;
    jpb_SpriteSetCylinderHook(
        game_runtime_capture_cylinder, runtime);
    runtime->cylinderHookReady = 1;
    jpb_PwrupSetDrawHook(
        game_runtime_capture_powerup_draw, runtime);
    runtime->powerupDrawHookReady = 1;
    jpb_PlayerSetProcessObserver(
        game_runtime_observe_player_process, runtime);
    runtime->playerProcessObserverReady = 1;
    jpb_BulletSetLaunchObserver(
        game_runtime_observe_bullet_launch, runtime);
    runtime->bulletLaunchObserverReady = 1;
    {
        JPBSceneMiddleRenderHooks hooks;

        memset(&hooks, 0, sizeof(hooks));
        hooks.afterAnimations =
            game_runtime_scene_after_animations;
        hooks.afterWorld = game_runtime_scene_after_world;
        hooks.renderModels = game_runtime_scene_after_models;
        hooks.beforePlayerProcess =
            game_runtime_scene_before_player_process;
        hooks.levelOwner = game_runtime_scene_level_owner;
        jpb_SceneSetMiddleRenderHooks(&hooks, runtime);
        runtime->sceneMiddleRenderHooksReady = 1;
    }
    game_runtime_set_failure_stage("none");
    return JPB_GAME_RUNTIME_OK;
}

static JPBGameRuntimeEnemyClass *
game_runtime_find_enemy_class(
    JPBGameRuntimeEnemyState *state,
    int actor_num)
{
    size_t index;

    if (state == NULL) {
        return NULL;
    }
    for (index = 0; index < state->classCount; ++index) {
        if (state->classes[index].actorNum == actor_num) {
            return &state->classes[index];
        }
    }
    return NULL;
}

int jpb_GameRuntimeEnemyClassModelId(
    const JPBGameRuntime *runtime,
    int actor_num)
{
    size_t index;

    if (runtime == NULL ||
        runtime->enemyState == NULL) {
        return -1;
    }
    for (index = 0;
         index < runtime->enemyState->classCount;
         ++index) {
        const JPBGameRuntimeEnemyClass *asset_class =
            &runtime->enemyState->classes[index];

        if (asset_class->actorNum == actor_num) {
            return asset_class->modelId;
        }
    }
    return -1;
}

int jpb_GameRuntimeEnemyClassWasActive(
    const JPBGameRuntime *runtime,
    int actor_num)
{
    size_t index;

    if (runtime == NULL ||
        runtime->enemyState == NULL) {
        return 0;
    }
    for (index = 0;
         index < runtime->enemyState->classCount;
         ++index) {
        const JPBGameRuntimeEnemyClass *asset_class =
            &runtime->enemyState->classes[index];

        if (asset_class->actorNum == actor_num) {
            return asset_class->wasActive;
        }
    }
    return 0;
}

static JPBGameRuntimeEnemyActor *
game_runtime_primary_enemy_actor(
    JPBGameRuntime *runtime)
{
    size_t index;

    if (runtime == NULL || runtime->enemyState == NULL) {
        return NULL;
    }
    if (runtime->player != NULL &&
        runtime->player->target != NULL) {
        for (index = 0;
             index < JPB_GAME_RUNTIME_ENEMY_CAPACITY;
             ++index) {
            JPBGameRuntimeEnemyActor *actor =
                &runtime->enemyState->actors[index];

            if (actor->authoredMotionReady &&
                actor->player == runtime->player->target) {
                return actor;
            }
        }
    }
    for (index = 0;
         index < JPB_GAME_RUNTIME_ENEMY_CAPACITY;
         ++index) {
        JPBGameRuntimeEnemyActor *actor =
            &runtime->enemyState->actors[index];

        if (actor->authoredMotionReady) {
            return actor;
        }
    }
    return NULL;
}

static JPBGameRuntimeEnemyActor *
game_runtime_enemy_actor_for_player(
    JPBGameRuntime *runtime,
    playerObject *player)
{
    size_t index;

    if (runtime == NULL ||
        runtime->enemyState == NULL ||
        player == NULL) {
        return NULL;
    }
    for (index = 0;
         index < JPB_GAME_RUNTIME_ENEMY_CAPACITY;
         ++index) {
        JPBGameRuntimeEnemyActor *actor =
            &runtime->enemyState->actors[index];

        if (actor->authoredMotionReady &&
            actor->player == player) {
            return actor;
        }
    }
    return NULL;
}

static int game_runtime_report_passive_motion(
    int32_t *cpad, playerObject *player)
{
    (void)cpad;
    if (player != NULL && player->pMotion != NULL &&
        *player->pMotion != NULL) {
        player->currentMotion =
            (int16_t)(*player->pMotion)->Seq;
    }
    return 0;
}

static void game_runtime_observe_bullet_launch(
    void *user_data,
    const Projectile *projectile,
    const playerObject *player,
    const VECTOR *start,
    const VECTOR *target)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    int slot;

    if (runtime == NULL || projectile == NULL || player == NULL ||
        start == NULL || target == NULL) {
        return;
    }
    slot = (int)player->playernum;
    if ((unsigned)slot >= 2U) {
        return;
    }
    ++runtime->playerProjectileLaunchCount[slot];
    runtime->lastPlayerProjectileType[slot] = projectile->pj_Type;
    runtime->lastPlayerProjectileFlags[slot] =
        (uint32_t)projectile->pj_Flags;
    runtime->lastPlayerProjectileStart[slot] = *start;
    runtime->lastPlayerProjectileTarget[slot] = *target;
}

static void game_runtime_observe_player_process(
    JPBPlayerProcessPhase phase,
    int index,
    playerObject *player,
    int AI_ON,
    const int32_t *cpad,
    void *user_data)
{
    JPBGameRuntime *runtime =
        (JPBGameRuntime *)user_data;
    JPBGameRuntimeEnemyActor *actor;
    int control_slot = -1;

    (void)index;
    if (runtime == NULL || player == NULL) {
        return;
    }
    if (phase == JPB_PLAYER_PROCESS_BEFORE_CONTROL &&
        AI_ON == 0 && player->pEnemy == NULL &&
        player->pMotionCallBack == NULL &&
        player->pMotion != NULL && *player->pMotion != NULL) {
        uint32_t held = cpad != NULL ? (uint32_t)cpad[1] : 0;
        uint32_t force_modifier =
            (uint32_t)gaButtonMap[
                OptionStruct.ControllerConfig[0]][4];

        /*
         * The retail animation scheduler reports even callback-free active
         * motions during the brain tick.  The portable scheduler processes
         * animation and player lists as separate stages, so install a
         * one-tick reporter through the recovered callback boundary.  This
         * makes the exact run-stop, running-block, and motion-gated input
         * branches observe the animation that is actually active without
         * changing brain_ControlPlayer itself.
         */
        if ((held & force_modifier) == 0) {
            player->pMotionCallBack =
                game_runtime_report_passive_motion;
            ++runtime->passiveMotionReportFrameCount;
        }
    } else if (phase == JPB_PLAYER_PROCESS_AFTER_CONTROL &&
               player->pMotionCallBack ==
                   game_runtime_report_passive_motion) {
        player->pMotionCallBack = NULL;
    }
    if (player == runtime->player) {
        control_slot = 0;
    } else if (player == runtime->inactivePlayer &&
               runtime->secondPlayerState != NULL) {
        control_slot = 1;
    }
    if (control_slot >= 0) {
        if (phase == JPB_PLAYER_PROCESS_BEFORE_CONTROL && cpad != NULL) {
            uint32_t pressed = (uint32_t)cpad[0];
            uint32_t held = (uint32_t)cpad[1];
            uint32_t released =
                runtime->controlPreviousHeldBits[control_slot] & ~held;

            if (pressed != 0) {
                ++runtime->controlPressedFrameCount[control_slot];
            }
            if (held != 0) {
                ++runtime->controlHeldFrameCount[control_slot];
            }
            if (released != 0) {
                ++runtime->controlReleaseEventCount[control_slot];
            }
            runtime->controlObservedPressedBits[control_slot] |= pressed;
            runtime->controlObservedHeldBits[control_slot] |= held;
            runtime->controlObservedReleasedBits[control_slot] |= released;
            runtime->controlPreviousHeldBits[control_slot] = held;
            if ((held & UINT32_C(0xf000)) != 0) {
                float axis_x = control_slot == 0 ? g_p1X : g_p2X;
                float axis_y = control_slot == 0 ? g_p1Y : g_p2Y;

                ++runtime->controlDirectionalFrameCount[control_slot];
                runtime->lastControlAxisX[control_slot] = axis_x;
                runtime->lastControlAxisY[control_slot] = axis_y;
                runtime->lastControlCameraAngle[control_slot] =
                    mCameraAngleDest;
                runtime->lastControlDesiredFacing[control_slot] =
                    jpb_BrainDirectionAngle(
                        axis_x, axis_y, mCameraAngleDest);
            }
        } else if (phase == JPB_PLAYER_PROCESS_AFTER_CONTROL) {
            uint8_t lock_active =
                (player->pFlags & UINT32_C(0x00400000)) != 0;

            if (runtime->controlLockInitialized[control_slot] != 0 &&
                runtime->controlLockActive[control_slot] != lock_active) {
                ++runtime->controlLockToggleCount[control_slot];
            }
            runtime->controlLockInitialized[control_slot] = 1;
            runtime->controlLockActive[control_slot] = lock_active;
            runtime->lastControlMotion[control_slot] =
                player->currentMotion;
            if (cpad != NULL &&
                ((uint32_t)cpad[1] & UINT32_C(0xf000)) != 0) {
                runtime->lastControlFacing[control_slot] =
                    physics_gGetFacing(&player->playerRoot);
            }
            if (player->currentMotion == 1 ||
                player->currentMotion == 2) {
                ++runtime->controlLocomotionFrameCount[control_slot];
                runtime->lastControlLocomotionMotion[control_slot] =
                    player->currentMotion;
            }
        }
    }
    if (player == runtime->player &&
        phase == JPB_PLAYER_PROCESS_BEFORE_CONTROL) {
        int attached = player->pEnemy != NULL;

        if (attached != runtime->playerAuthoredAiObserved) {
            if (attached) {
                ++runtime->playerAuthoredAiAttachCount;
                runtime->lastPlayerAuthoredAiEnemyId =
                    (int16_t)player->pEnemy->enemyID;
                runtime->lastPlayerAuthoredAiOwnerType =
                    (int16_t)player->pEnemy->ownerType;
            } else {
                ++runtime->playerAuthoredAiReleaseCount;
            }
            runtime->playerAuthoredAiObserved = attached;
        }
    }
    if (phase == JPB_PLAYER_PROCESS_AFTER_CONTROL &&
        player->pEnemy != NULL &&
        gpWorld != NULL &&
        gpWorld->currentDolly >= 0 &&
        gpWorld->currentDolly < 256 &&
        (gpWorld->aDolly[gpWorld->currentDolly].flags &
         UINT32_C(0x400)) != 0 &&
        player->pMotion != NULL &&
        *player->pMotion != NULL) {
        /*
         * braindmg_DamageControl returns the authored-camera sentinel 0x400
         * while a dolly owns the scene. brain_ControlPlayer consequently
         * publishes logical motion 1 even though it did not activate that
         * animation or its velocity. The retail loader's actor lifecycle
         * does not expose that transient mismatch to BAP AI; restore the
         * animation-owned motion for every BAP-driven player, including a
         * live P1/P2 temporarily attached by enemy_ParseOpcodes 0x60f.
         * Restricting this adapter to separately allocated enemy actors left
         * FED's player proxy unable to turn toward its authored waypoint.
         */
        player->currentMotion =
            (int16_t)(*player->pMotion)->Seq;
    }
    if (player == runtime->player) {
        if (phase == JPB_PLAYER_PROCESS_AFTER_CONTROL &&
            runtime->authoredMotionReady) {
            if (player->currentMotion == 1 ||
                player->currentMotion == 2) {
                ++runtime->authoredLocomotionMotionFrameCount;
                runtime->lastAuthoredLocomotionMotion =
                    player->currentMotion;
            }
            if (player->currentMotion >= 0 &&
                player->currentMotion < player->maxMotions &&
                player->paMotions[
                    player->currentMotion].Damage != 0) {
                ++runtime->authoredDamageMotionFrameCount;
                runtime->lastAuthoredDamageMotion =
                    player->currentMotion;
            }
            if (player->currentMotion >= 92 &&
                player->currentMotion <= 94) {
                ++runtime->authoredRunningAttackFrameCount;
                runtime->lastAuthoredRunningAttackMotion =
                    player->currentMotion;
            }
        }
        return;
    }

    actor = game_runtime_enemy_actor_for_player(
        runtime, player);
    if (actor == NULL) {
        return;
    }
    if (phase == JPB_PLAYER_PROCESS_BEFORE_CONTROL) {
        actor->schedulerPendingHit = player->hitNumber;
        actor->schedulerAuthoredHitReaction =
            player->hitMotion != NULL
                ? (int)player->hitMotion->hitReact
                : -1;
        actor->schedulerRecoilBefore =
            actor->physics != NULL
                ? actor->physics->constmov.vz
                : 0.0f;
        runtime->combatHitCount += (uint8_t)(
            actor->schedulerPendingHit -
            actor->schedulerHitBefore);
        if (player->pMainCallBack != NULL) {
            ++actor->mainCallbackFrameCount;
        }
        return;
    }

    {
        int processed_hit =
            actor->schedulerPendingHit != 0 &&
            player->hitNumber == 0;
        int enemy_energy =
            game_gGetEnergy(player->playernum);
        int motion_index = -1;
        int motion;

        if (processed_hit) {
            ++actor->damageProcessedCount;
            if (runtime->enemyDamageProcessedCount == 0) {
                runtime->enemyInitialEnergy =
                    actor->initialEnergy;
                runtime->enemyMinimumEnergy =
                    actor->initialEnergy;
            }
            ++runtime->enemyDamageProcessedCount;
        }
        if (enemy_energy < actor->minimumEnergy) {
            actor->minimumEnergy = (int16_t)enemy_energy;
        }
        if (processed_hit &&
            enemy_energy < runtime->enemyMinimumEnergy) {
            runtime->enemyMinimumEnergy =
                (int16_t)enemy_energy;
        }
        for (motion = 0;
             player->pMotion != NULL &&
             player->paMotions != NULL &&
             motion < player->maxMotions;
             ++motion) {
            if (*player->pMotion ==
                &player->paMotions[motion]) {
                motion_index = motion;
                break;
            }
        }
        actor->animationMotion = (int16_t)motion_index;
        if (processed_hit) {
            actor->lastDamageMotion =
                (int16_t)motion_index;
            runtime->lastEnemyDamageMotion =
                (int16_t)motion_index;
        }
        if (processed_hit && motion_index >= 0 &&
            (motion_index == 6 ||
             motion_index == 33 ||
             motion_index ==
                 actor->schedulerAuthoredHitReaction ||
             motion_index == 37 ||
             motion_index == 38 ||
             motion_index == 42 ||
             motion_index == mDrawingSurfaceId + 49)) {
            ++actor->reactionMotionFrameCount;
            actor->lastReactionMotion =
                (int16_t)motion_index;
            ++runtime->enemyReactionMotionFrameCount;
            runtime->lastEnemyReactionMotion =
                (int16_t)motion_index;
        }
        if (processed_hit && actor->physics != NULL &&
            actor->physics->constmov.vz !=
                actor->schedulerRecoilBefore) {
            ++actor->recoilReactionCount;
            actor->lastRecoil =
                actor->physics->constmov.vz -
                actor->schedulerRecoilBefore;
            ++runtime->enemyRecoilReactionCount;
            runtime->lastEnemyRecoil = actor->lastRecoil;
        }
    }
}

static void game_runtime_publish_primary_enemy(
    JPBGameRuntime *runtime)
{
    JPBGameRuntimeEnemyActor *actor;
    JPBGameRuntimeEnemyState *state;

    if (runtime == NULL || runtime->enemyState == NULL) {
        return;
    }
    state = runtime->enemyState;
    actor = game_runtime_primary_enemy_actor(runtime);
    if (actor == NULL) {
        actor = &state->actors[0];
    }
    runtime->enemyActorRoot = actor->actorRoot;
    runtime->enemyScene = actor->scene;
    runtime->enemyModel = actor->model;
    runtime->enemyPhysics = actor->physics;
    runtime->enemyAnimation = actor->animation;
    runtime->enemyPlayer = actor->player;
    runtime->enemy = actor->enemy;
    runtime->enemyAuthoredMotionReady =
        actor->authoredMotionReady;
    runtime->enemyAuthoredFrameReady =
        actor->authoredFrameReady;
    runtime->enemyAuthoredPoseReady =
        actor->authoredPoseReady;
    runtime->enemyDecodedFrameCount =
        actor->decodedFrameCount;
    runtime->enemyMainCallbackFrameCount =
        actor->mainCallbackFrameCount;
    runtime->enemyKungfuSchedulerFrameCount =
        actor->kungfuSchedulerFrameCount;
    runtime->enemyAuthoredOpcodeFrameCount =
        actor->authoredOpcodeFrameCount;
    runtime->enemyOpcodeBoundaryFrameCount =
        actor->opcodeBoundaryFrameCount;
    runtime->enemyOpcodeBoundary =
        actor->opcodeBoundary;
    runtime->enemyAnimationMotion =
        actor->animationMotion;
    runtime->enemyAiLevel = actor->aiLevel;
    if (runtime->enemyDamageProcessedCount == 0) {
        runtime->enemyInitialEnergy =
            actor->initialEnergy;
        runtime->enemyMinimumEnergy =
            actor->minimumEnergy;
    }
    runtime->enemyRenderedTriangles =
        actor->renderedTriangles;
    runtime->enemyRenderedPixels =
        actor->renderedPixels;
    if (actor->assetClass != NULL) {
        runtime->enemyCadStorage =
            actor->assetClass->cadStorage;
        runtime->enemyBmdStorage =
            actor->assetClass->bmdStorage;
        runtime->enemyCadView =
            actor->assetClass->cadView;
        runtime->enemyBmdView =
            actor->assetClass->bmdView;
        runtime->enemyTextureCache =
            actor->assetClass->textureCache;
    }
    if (actor->authoredMotionReady &&
        actor->assetClass != NULL &&
        (uint32_t)actor->aiLevel <
        JPB_AI_LEVEL_CAPACITY) {
        runtime->enemyAiStorage =
            actor->assetClass
                ->aiStorage[actor->aiLevel];
        runtime->enemyAiStorageSize =
            actor->assetClass
                ->aiStorageSize[actor->aiLevel];
    } else {
        runtime->enemyAiStorage = NULL;
        runtime->enemyAiStorageSize = 0;
    }
}

static aiData *game_runtime_get_enemy_ai(
    JPBGameRuntimeEnemyClass *asset_class,
    int level)
{
    char ai_path[JPB_GAME_RUNTIME_PATH_CAPACITY];
    aiData *data = NULL;

    if (asset_class == NULL ||
        (uint32_t)level >= JPB_AI_LEVEL_CAPACITY) {
        return NULL;
    }
    if (asset_class->aiDataByLevel[level] != NULL) {
        return asset_class->aiDataByLevel[level];
    }
    if (!game_runtime_ai_path(
            asset_class->aiCadPath,
            level,
            ai_path,
            sizeof(ai_path))) {
        return NULL;
    }
    asset_class->aiStorage[level] =
        (uint8_t *)malloc(JPB_AI_REFERENCE_CAPACITY);
    if (asset_class->aiStorage[level] == NULL) {
        return NULL;
    }
    if (jpb_AiLoadDataFile(
            ai_path,
            asset_class->aiStorage[level],
            JPB_AI_REFERENCE_CAPACITY,
            &data,
            &asset_class->aiStorageSize[level]) !=
            JPB_AI_OK ||
        !jpb_AiRegisterData(
            asset_class->actorNum, level, data)) {
        free(asset_class->aiStorage[level]);
        asset_class->aiStorage[level] = NULL;
        asset_class->aiStorageSize[level] = 0;
        return NULL;
    }
    asset_class->aiDataByLevel[level] = data;
    return data;
}

static void game_runtime_release_enemy_class(
    JPBGameRuntimeEnemyClass *asset_class)
{
    int level;

    if (asset_class == NULL) {
        return;
    }
    for (level = 0;
         level < JPB_AI_LEVEL_CAPACITY;
         ++level) {
        if (asset_class->aiDataByLevel[level] != NULL) {
            (void)jpb_AiRegisterData(
                asset_class->actorNum, level, NULL);
        }
        free(asset_class->aiStorage[level]);
    }
    free(asset_class->cadStorage);
    free(asset_class->bmdStorage);
    game_runtime_free_texture_cache(
        asset_class->textureCache);
    memset(asset_class, 0, sizeof(*asset_class));
}

static void game_runtime_release_enemy_classes(
    JPBGameRuntimeEnemyState *state)
{
    size_t index;

    if (state == NULL) {
        return;
    }
    for (index = 0; index < state->classCount; ++index) {
        game_runtime_release_enemy_class(
            &state->classes[index]);
    }
    state->classCount = 0;
}

static int game_runtime_find_enemy_actor_num(
    const JPBGameRuntime *runtime,
    const char *actor_stem)
{
    int index;

    if (runtime == NULL || runtime->world == NULL ||
        runtime->world->apActorNames == NULL) {
        return -1;
    }
    for (index = 0;
         index < runtime->world->nActor;
         ++index) {
        if (game_runtime_path_stem_equals(
                runtime->world->apActorNames[index],
                actor_stem)) {
            return index;
        }
    }
    return -1;
}

static int game_runtime_load_enemy_class(
    JPBGameRuntimeEnemyState *state,
    const JPBGameRuntimeEnemyClassSpec *spec,
    int actor_num,
    const char *cad_reference_path,
    const char *bmd_reference_path,
    int use_reference_files)
{
    JPBGameRuntimeEnemyClass *asset_class;
    char cad_path[JPB_GAME_RUNTIME_PATH_CAPACITY];
    char bmd_path[JPB_GAME_RUNTIME_PATH_CAPACITY];

    if (state == NULL || spec == NULL ||
        cad_reference_path == NULL ||
        bmd_reference_path == NULL ||
        actor_num < 0 ||
        actor_num >= JPB_ENEMY_MODEL_ACCOUNT_CAPACITY ||
        state->classCount >=
            JPB_GAME_RUNTIME_ENEMY_CLASS_CAPACITY) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    if (use_reference_files) {
        if (strlen(cad_reference_path) >= sizeof(cad_path) ||
            strlen(bmd_reference_path) >= sizeof(bmd_path)) {
            return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
        }
        memcpy(
            cad_path,
            cad_reference_path,
            strlen(cad_reference_path) + 1);
        memcpy(
            bmd_path,
            bmd_reference_path,
            strlen(bmd_reference_path) + 1);
    } else if (
        !game_runtime_sibling_asset_path(
            cad_reference_path,
            spec->animationName,
            ".cad",
            cad_path,
            sizeof(cad_path)) ||
        !game_runtime_sibling_asset_path(
            bmd_reference_path,
            spec->modelName,
            ".bmd",
            bmd_path,
            sizeof(bmd_path))) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }

    asset_class = &state->classes[state->classCount];
    memset(asset_class, 0, sizeof(*asset_class));
    asset_class->actorNum = actor_num;
    asset_class->modelId = spec->modelId;
    asset_class->modelName = spec->modelName;
    asset_class->animationName = spec->animationName;
    if (!game_runtime_sibling_asset_path(
            cad_reference_path,
            spec->modelName,
            ".cad",
            asset_class->aiCadPath,
            sizeof(asset_class->aiCadPath))) {
        game_runtime_release_enemy_class(asset_class);
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }

    asset_class->cadStorage =
        (uint8_t *)malloc(JPB_CAD_REFERENCE_CAPACITY);
    asset_class->bmdStorage =
        (uint8_t *)malloc(JPB_BMD_REFERENCE_CAPACITY);
    if (asset_class->cadStorage == NULL ||
        asset_class->bmdStorage == NULL) {
        game_runtime_release_enemy_class(asset_class);
        return JPB_GAME_RUNTIME_OUT_OF_MEMORY;
    }
    if (jpb_BmdLoadFile(
            bmd_path,
            asset_class->bmdStorage,
            JPB_BMD_REFERENCE_CAPACITY,
            &asset_class->bmdView) != JPB_BMD_OK ||
        jpb_CadLoadFile(
            cad_path,
            asset_class->cadStorage,
            JPB_CAD_REFERENCE_CAPACITY,
            &asset_class->cadView) != JPB_CAD_OK ||
        asset_class->cadView.sequence_count < 1 ||
        asset_class->cadView.sequence_count > INT16_MAX) {
        game_runtime_release_enemy_class(asset_class);
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    asset_class->textureCache =
        game_runtime_create_texture_cache(
            JPB_GAME_RUNTIME_MODEL_TEXTURE_CAPACITY,
            (int)(int8_t)LevelSelect);
    if (asset_class->textureCache == NULL) {
        game_runtime_release_enemy_class(asset_class);
        return JPB_GAME_RUNTIME_OUT_OF_MEMORY;
    }
    if (!game_runtime_texture_directory(
            bmd_path,
            asset_class->textureCache->directory,
            sizeof(asset_class->textureCache->directory))) {
        game_runtime_release_enemy_class(asset_class);
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    maModelID[actor_num][0] = spec->modelId;
    ++state->classCount;
    return JPB_GAME_RUNTIME_OK;
}

static void game_runtime_release_enemy_actor(
    JPBGameRuntimeEnemyActor *actor)
{
    int object_id = -1;

    if (actor == NULL) {
        return;
    }
    if (actor->scene != NULL) {
        object_id = actor->scene->sceneRoot.objectID;
    }
    if (actor->animation != NULL) {
        actor->animation->animRoot.objectID = -1;
        actor->animation->animRoot.pParent = NULL;
    }
    if (actor->player != NULL) {
        actor->player->playerRoot.objectID = -1;
        actor->player->playerRoot.pParent = NULL;
    }
    if (actor->physics != NULL) {
        actor->physics->physicsRoot.objectID = -1;
        actor->physics->physicsRoot.pParent = NULL;
    }
    actor->model.modelRoot.objectID = -1;
    actor->model.modelRoot.pParent = NULL;
    actor->actorRoot.objectID = -1;
    actor->actorRoot.pParent = NULL;
    if (actor->scene != NULL) {
        memset(actor->scene, 0, sizeof(*actor->scene));
        actor->scene->sceneRoot.objectID = -1;
    }
    if ((uint32_t)object_id < JPB_ANIMATION_CAPACITY) {
        jpb_AnimResetObjectSlot(object_id);
    }
    memset(actor, 0, sizeof(*actor));
    actor->actorRoot.objectID = -1;
    actor->model.modelRoot.objectID = -1;
}

static JPBGameRuntimeEnemyActor *
game_runtime_find_enemy_actor_slot(
    JPBGameRuntime *runtime)
{
    size_t index;

    for (index = 0;
         index < JPB_GAME_RUNTIME_ENEMY_CAPACITY;
         ++index) {
        JPBGameRuntimeEnemyActor *actor =
            &runtime->enemyState->actors[index];

        if (!actor->authoredMotionReady &&
            (actor->player == NULL ||
             actor->player->playerRoot.objectID == -1)) {
            game_runtime_release_enemy_actor(actor);
            return actor;
        }
    }
    return NULL;
}

static int game_runtime_prepare_enemy_actor_pools(
    JPBGameRuntimeEnemyActor *actor)
{
    int object_id;

    actor->scene = scene_gGetNewSceneObject(-1);
    actor->physics = physics_gGetNewObject(-1);
    actor->player = NULL;
    if (actor->scene == NULL ||
        actor->physics == NULL) {
        return 0;
    }
    object_id = actor->scene->sceneRoot.objectID;
    if (actor->physics->physicsRoot.objectID != object_id) {
        return 0;
    }

    memset(actor->scene, 0, sizeof(*actor->scene));
    actor->scene->sceneRoot.objectID = object_id;
    memset(actor->physics, 0, sizeof(*actor->physics));
    actor->physics->physicsRoot.objectID = object_id;
    actor->physics->turnspeed = 0x500;
    actor->physics->radius = 0x36;
    actor->physics->mass = 0x800;
    actor->physics->height = 0xdc;
    memcpy(
        actor->physics->physicsRoot.objectName,
        "PHYSICS",
        sizeof("PHYSICS"));
    jpb_AnimResetObjectSlot(object_id);
    return 1;
}

static int game_runtime_create_enemy(
    wsl_ENEMY *enemy, void *user_data)
{
    JPBGameRuntime *runtime =
        (JPBGameRuntime *)user_data;
    JPBGameRuntimeEnemyState *state;
    JPBGameRuntimeEnemyActor *actor;
    JPBGameRuntimeEnemyClass *asset_class;
    wsl_BAP_PLACEMENT *placement;
    aiData *enemy_ai;
    int object_id;

    if (runtime == NULL || runtime->enemyState == NULL ||
        enemy == NULL || enemy->pPlace == NULL) {
        return 0;
    }
    state = runtime->enemyState;
    placement = enemy->pPlace;
    asset_class = game_runtime_find_enemy_class(
        state, placement->actorNum);
    if (asset_class == NULL) {
        return 0;
    }
    enemy_ai =
        game_runtime_get_enemy_ai(
            asset_class, enemy->aiLevel);
    if (enemy_ai == NULL) {
        return 0;
    }
    actor = game_runtime_find_enemy_actor_slot(runtime);
    if (actor == NULL ||
        !game_runtime_prepare_enemy_actor_pools(actor)) {
        game_runtime_release_enemy_actor(actor);
        return 0;
    }
    object_id = actor->scene->sceneRoot.objectID;
    actor->assetClass = asset_class;
    actor->actorRoot.objectID = object_id;
    memcpy(
        actor->actorRoot.objectName,
        "ACTOR",
        sizeof("ACTOR"));
    actor->model.modelRoot.objectID = object_id;
    memcpy(
        actor->model.modelRoot.objectName,
        "MODEL",
        sizeof("MODEL"));
    obj_gSetChildObject(
        actor->scene, &actor->actorRoot, 0);
    obj_gSetChildObject(
        actor->scene, &actor->model.modelRoot, 1);
    obj_gSetChildObject(
        actor->scene,
        &actor->physics->physicsRoot,
        2);
    if (!game_runtime_build_model(
            &asset_class->bmdView,
            asset_class->modelName,
            object_id,
            &actor->model,
            asset_class->textureCache)) {
        game_runtime_release_enemy_actor(actor);
        return 0;
    }
    actor->animation = anim_CreateObject(
        actor->scene,
        asset_class->cadView.payload,
        NULL,
        0);
    if (actor->animation == NULL) {
        game_runtime_release_enemy_actor(actor);
        return 0;
    }
    actor->player = player_gCreateObject(
        actor->scene,
        asset_class->modelId,
        ai_InitPlayer);
    if (actor->player == NULL) {
        game_runtime_release_enemy_actor(actor);
        return 0;
    }
    player_gConnectMotionData(
        actor->player,
        asset_class->cadView.payload);
    if (actor->player->paMotions !=
            asset_class->cadView.motions ||
        actor->player->maxMotions !=
            (int16_t)asset_class->cadView.sequence_count) {
        game_runtime_release_enemy_actor(actor);
        return 0;
    }
    actor->player->oldmaxCMotions =
        (int16_t)asset_class->cadView.sequence_count;
    actor->player->paiMemory = enemy_ai;
    actor->player->paCombos = NULL;
    actor->player->maxCombos = 0;
    actor->player->target = runtime->player;
    enemy->actorNum = placement->actorNum;
    enemy->enemyNum = enemy->enemyID;
    enemy->pPlayer = actor->player;
    actor->player->pEnemy = enemy;
    actor->enemy = enemy;
    actor->aiLevel = (int16_t)enemy->aiLevel;
    actor->authoredMotionReady = 1;

    /*
     * loader_CreateEnemy delegates the complete authored spawn reset to
     * player_RefreshPlayer. Besides position, facing, and energy, that owner
     * seeds the animation queue and clears transient player state. Starting
     * motion zero directly leaves non-combat scene directors in their model's
     * motion 3, which prevents their BAP camera script from advancing.
     */
    player_RefreshPlayer(actor->player);

    /*
     * The dependency-light host assembles loader_CreateCharacter's component
     * pools directly. Publish the floor resolved by player_RefreshPlayer as
     * the actor's current ground before the same-frame physics scheduler can
     * classify the fresh object as airborne.
     */
    actor->physics->airGround =
        actor->physics->validairground;

    /*
     * Exact loader_CreateEnemy RVAs 0xBC2E2..0xBC387 read the recovered
     * wsl_ENEMY.movementMode field at offset 0xB0.  ownerType describes the
     * actor's gameplay role and is independent (scripted level machinery is
     * commonly owner 3 with ordinary movement mode 0).
     */
    switch (enemy->movementMode) {
    case 2:
        actor->physics->movemode = MOVE_HOVER;
        actor->physics->flags |= UINT32_C(0x2000);
        break;
    case 3:
        actor->physics->movemode = MOVE_HOVER3D;
        actor->physics->flags |= UINT32_C(0x2000);
        break;
    case 4:
        actor->physics->movemode = MOVE_FLY;
        actor->physics->flags |= UINT32_C(0x2000);
        break;
    default:
        actor->physics->movemode = MOVE_NORMAL;
        actor->physics->flags &= ~UINT32_C(0x2000);
        break;
    }
    actor->initialEnergy =
        GameStruct.aCharacterData[object_id].Energy;
    actor->minimumEnergy = actor->initialEnergy;
    ++runtime->enemyActorCount;
    ++runtime->enemySpawnCount;
    if (runtime->enemyActorCount >
        runtime->enemyActorPeakCount) {
        runtime->enemyActorPeakCount =
            runtime->enemyActorCount;
    }
    game_runtime_publish_primary_enemy(runtime);
    return 1;
}

int jpb_GameRuntimeAddEnemyAssets(
    JPBGameRuntime *runtime,
    const char *cad_path,
    const char *bmd_path)
{
    JPBGameRuntimeEnemyState *state;
    size_t spec_index;
    int index;
    int result;

    if (runtime == NULL || cad_path == NULL || bmd_path == NULL ||
        runtime->world == NULL || runtime->player == NULL ||
        runtime->world->apEnemy == NULL ||
        runtime->world->nEnemy <= 0 ||
        runtime->enemyState != NULL) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    if (strlen(cad_path) >= JPB_GAME_RUNTIME_PATH_CAPACITY ||
        strlen(bmd_path) >= JPB_GAME_RUNTIME_PATH_CAPACITY) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    memset(maModelID, 0, sizeof(maModelID));

    state =
        (JPBGameRuntimeEnemyState *)calloc(
            1, sizeof(*state));
    if (state == NULL) {
        return JPB_GAME_RUNTIME_OUT_OF_MEMORY;
    }
    runtime->enemyState = state;
    for (spec_index = 0;
         spec_index < JPB_ACTOR_NAME_COUNT;
         ++spec_index) {
        JPBGameRuntimeEnemyClassSpec spec;
        int actor_num =
            -1;

        if (!game_runtime_enemy_class_spec(
                spec_index, &spec)) {
            game_runtime_release_enemy_classes(state);
            free(state);
            runtime->enemyState = NULL;
            return JPB_GAME_RUNTIME_LOAD_FAILED;
        }
        actor_num = game_runtime_find_enemy_actor_num(
            runtime, spec.actorStem);

        if (actor_num < 0) {
            continue;
        }
        /*
         * sObiNames contains two exact "jarjar" records: the ordinary
         * level actor and the playable variant.  The level archive owns one
         * actor slot for that stem, and the original table lookup resolves
         * its first match.  Loading every matching table record instead
         * incorrectly requests jar_jar_playable.bmd (which is not an
         * installed asset) for Marsh and Mini3.
         */
        if (game_runtime_find_enemy_class(state, actor_num) != NULL) {
            continue;
        }
        result = game_runtime_load_enemy_class(
            state,
            &spec,
            actor_num,
            cad_path,
            bmd_path,
            spec.modelId == 17);
        if (result != JPB_GAME_RUNTIME_OK) {
            game_runtime_release_enemy_classes(state);
            free(state);
            runtime->enemyState = NULL;
            return result;
        }
    }
    if (state->classCount == 0) {
        game_runtime_release_enemy_classes(state);
        free(state);
        runtime->enemyState = NULL;
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    runtime->enemyLoadedClassCount = state->classCount;
    for (spec_index = 0;
         spec_index < state->classCount;
         ++spec_index) {
        for (index = 0;
             index < runtime->world->nEnemy;
             ++index) {
            wsl_BAP_PLACEMENT *candidate =
                runtime->world->apEnemy[index];

            if (candidate != NULL &&
                candidate->actorNum ==
                    state->classes[spec_index].actorNum) {
                ++runtime->enemyPlacedClassCount;
                break;
            }
        }
    }
    runtime->enemyCadStorage =
        state->classes[0].cadStorage;
    runtime->enemyBmdStorage =
        state->classes[0].bmdStorage;
    runtime->enemyCadView =
        state->classes[0].cadView;
    runtime->enemyBmdView =
        state->classes[0].bmdView;
    runtime->enemyTextureCache =
        state->classes[0].textureCache;

    enemy_InitEnemies();
    shaolin_InitKungfu();
    jpb_LoaderSetEnemyCreateProvider(
        game_runtime_create_enemy, runtime);
    /*
     * Loading assets must not manufacture an active actor. The exact
     * enemy_HandleEnemies -> _checkForNewEnemies owner activates authored
     * placements from activeFlags, aRange, and WorldData.location on the
     * first simulation frame.
     */
    game_runtime_publish_primary_enemy(runtime);
    return JPB_GAME_RUNTIME_OK;
}

int jpb_GameRuntimeAddPlayerComboData(
    JPBGameRuntime *runtime,
    const char *cmb_path)
{
    Combo *combos;
    int16_t combo_count;

    if (runtime == NULL || cmb_path == NULL ||
        runtime->player == NULL ||
        runtime->comboStorage != NULL) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    runtime->comboStorage =
        (uint8_t *)malloc(JPB_COMBO_REFERENCE_CAPACITY);
    if (runtime->comboStorage == NULL) {
        return JPB_GAME_RUNTIME_OUT_OF_MEMORY;
    }
    if (jpb_ComboLoadFile(
            cmb_path,
            runtime->comboStorage,
            JPB_COMBO_REFERENCE_CAPACITY,
            &combos,
            &combo_count) != JPB_COMBO_OK) {
        free(runtime->comboStorage);
        runtime->comboStorage = NULL;
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    runtime->player->paCombos = combos;
    runtime->player->maxCombos = combo_count;
    game_initPlayerStartCombos(
        (uint32_t)runtime->player->playernum);
    combo_InitComboData(runtime->player);
    return JPB_GAME_RUNTIME_OK;
}

int jpb_GameRuntimeActivateSecondPlayer(
    JPBGameRuntime *runtime,
    const char *cad_path,
    const char *bmd_path,
    int player_model_id)
{
    JPBGameRuntimeSecondPlayerState *state;
    playerObject *player;
    unsigned level_index;
    int result;

    if (runtime == NULL || cad_path == NULL || bmd_path == NULL ||
        runtime->inactivePlayerScene == NULL ||
        runtime->inactivePlayerPhysics == NULL ||
        runtime->inactivePlayer == NULL ||
        runtime->secondPlayerState != NULL ||
        player_model_id < 0 ||
        player_model_id >= JPB_MODEL_NAME_COUNT) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    state = (JPBGameRuntimeSecondPlayerState *)calloc(1, sizeof(*state));
    if (state == NULL) {
        return JPB_GAME_RUNTIME_OUT_OF_MEMORY;
    }
    runtime->secondPlayerState = state;
    level_index = (unsigned)(uint8_t)GameStruct.CurrentLevel;

    state->textureCache = game_runtime_create_texture_cache(
        JPB_GAME_RUNTIME_MODEL_TEXTURE_CAPACITY,
        level_index < JPB_LEVEL_NAME_COUNT ? (int)level_index : -1);
    state->bmdStorage = (uint8_t *)malloc(JPB_BMD_REFERENCE_CAPACITY);
    state->cadStorage = (uint8_t *)malloc(JPB_CAD_REFERENCE_CAPACITY);
    if (state->textureCache == NULL || state->bmdStorage == NULL ||
        state->cadStorage == NULL) {
        return JPB_GAME_RUNTIME_OUT_OF_MEMORY;
    }
    if (!game_runtime_texture_directory(
            bmd_path,
            state->textureCache->directory,
            sizeof(state->textureCache->directory))) {
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }

    state->model.modelRoot.objectID = 1;
    memcpy(
        state->model.modelRoot.objectName,
        "MODEL",
        sizeof("MODEL"));
    obj_gSetChildObject(
        runtime->inactivePlayerScene,
        &state->model.modelRoot,
        1);
    if (jpb_BmdLoadFile(
            bmd_path,
            state->bmdStorage,
            JPB_BMD_REFERENCE_CAPACITY,
            &state->bmdView) != JPB_BMD_OK ||
        !game_runtime_build_model(
            &state->bmdView,
            "PLAYER",
            1,
            &state->model,
            state->textureCache)) {
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    if (jpb_CadLoadFile(
            cad_path,
            state->cadStorage,
            JPB_CAD_REFERENCE_CAPACITY,
            &state->cadView) != JPB_CAD_OK ||
        state->cadView.sequence_count < 2 ||
        state->cadView.sequence_count > INT16_MAX) {
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    state->animation = anim_CreateObject(
        runtime->inactivePlayerScene,
        state->cadView.payload,
        NULL,
        1);
    if (state->animation == NULL) {
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }

    player = runtime->inactivePlayer;
    player->pMotion = &state->animation->pMotion;
    player->playernum = 1;
    player->playerID = (int16_t)player_model_id;
    player->fLife = 0;
    player->fStun = 0;
    player->fForce = 0;
    player->pFlags = 0;
    player->playerPad.oldbits0 = 0;
    player->playerPad.oldbits1 = 0;
    player->playerPad.mask0 = 0;
    player->playerPad.mask1 = UINT32_MAX;
    player->ACTION_LOCK = 0;
    (void)jedi_InitPlayer(player);
    scene_gSetSceneModelKeyFrame(
        1, state->animation->pCurrentAnimFrame);
    player_gConnectMotionData(player, state->cadView.payload);
    if (player->paMotions != state->cadView.motions ||
        player->maxMotions !=
            (int16_t)state->cadView.sequence_count) {
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    player->oldmaxCMotions =
        (int16_t)state->cadView.sequence_count;
    state->authoredMotionReady = 1;
    runtime->inactivePlayerActorRoot.flags &= ~UINT32_C(0x20);
    player->playerRoot.flags &= ~UINT32_C(0x20);
    player->target = runtime->player;
    runtime->player->target = player;
    player->playerPad.padnum = 1;
    player->playerPad.mask0 = 0;
    player->playerPad.mask1 = UINT32_MAX;
    player_RefreshPlayer(player);
    result = jpb_PhysicsUpdateSceneObject(
        runtime->inactivePlayerPhysics);
    if (result != JPB_PHYSICS_PARTIAL_OK) {
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    return JPB_GAME_RUNTIME_OK;
}

int jpb_GameRuntimeAddSecondPlayerComboData(
    JPBGameRuntime *runtime,
    const char *cmb_path)
{
    JPBGameRuntimeSecondPlayerState *state;
    Combo *combos;
    int16_t combo_count;

    if (runtime == NULL || cmb_path == NULL ||
        runtime->inactivePlayer == NULL ||
        runtime->secondPlayerState == NULL) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    state = runtime->secondPlayerState;
    if (state->comboStorage != NULL) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    state->comboStorage =
        (uint8_t *)malloc(JPB_COMBO_REFERENCE_CAPACITY);
    if (state->comboStorage == NULL) {
        return JPB_GAME_RUNTIME_OUT_OF_MEMORY;
    }
    if (jpb_ComboLoadFile(
            cmb_path,
            state->comboStorage,
            JPB_COMBO_REFERENCE_CAPACITY,
            &combos,
            &combo_count) != JPB_COMBO_OK) {
        free(state->comboStorage);
        state->comboStorage = NULL;
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    runtime->inactivePlayer->paCombos = combos;
    runtime->inactivePlayer->maxCombos = combo_count;
    game_initPlayerStartCombos(1);
    combo_InitComboData(runtime->inactivePlayer);
    return JPB_GAME_RUNTIME_OK;
}

int jpb_GameRuntimeSecondPlayerReady(
    const JPBGameRuntime *runtime)
{
    const JPBGameRuntimeSecondPlayerState *state;

    if (runtime == NULL || runtime->secondPlayerState == NULL ||
        runtime->inactivePlayer == NULL ||
        runtime->inactivePlayerScene == NULL) {
        return 0;
    }
    state = runtime->secondPlayerState;
    /* scene_middleRender poses and renders before player_gProcessPlayers.
     * A control transition can therefore select the next animation frame
     * after this frame's authored pose was successfully consumed. Requiring
     * those two pointers to remain equal misclassified exact walk-to-idle
     * transitions as an unready second player. */
    return state->authoredMotionReady &&
        state->authoredFrameReady &&
        state->authoredPoseReady &&
        state->animation != NULL &&
        state->model.pRootNode != NULL &&
        runtime->inactivePlayer->playernum == 1 &&
        (runtime->inactivePlayer->playerRoot.flags &
            UINT32_C(0x20)) == 0;
}

size_t jpb_GameRuntimeSecondPlayerRenderedTriangles(
    const JPBGameRuntime *runtime)
{
    return runtime != NULL && runtime->secondPlayerState != NULL
        ? runtime->secondPlayerState->renderedTriangles
        : 0;
}

size_t jpb_GameRuntimeSecondPlayerRenderedPixels(
    const JPBGameRuntime *runtime)
{
    return runtime != NULL && runtime->secondPlayerState != NULL
        ? runtime->secondPlayerState->renderedPixels
        : 0;
}

void jpb_GameRuntimeSetLevelRenderMesh(
    JPBGameRuntime *runtime,
    const JPBSoftwareLevelMesh *mesh)
{
    if (runtime != NULL) {
        runtime->levelRenderMesh = mesh;
    }
}

void jpb_GameRuntimeSetLevelRenderHook(
    JPBGameRuntime *runtime,
    JPBGameRuntimeLevelRenderHook hook,
    void *user_data)
{
    if (runtime != NULL) {
        runtime->levelRenderHook = hook;
        runtime->levelRenderUserData = user_data;
    }
}

void jpb_GameRuntimeSetModelRenderHooks(
    JPBGameRuntime *runtime,
    JPBGameRuntimeModelRenderBeginHook begin_hook,
    JPBSoftwareTriangleSink triangle_sink,
    JPBGameRuntimeModelRenderEndHook end_hook,
    void *user_data)
{
    if (runtime != NULL) {
        runtime->modelRenderBeginHook = begin_hook;
        runtime->modelTriangleSink = triangle_sink;
        runtime->modelRenderEndHook = end_hook;
        runtime->modelRenderUserData = user_data;
    }
}

void jpb_GameRuntimeSetScreenPolyRenderHooks(
    JPBGameRuntime *runtime,
    JPBGameRuntimeModelRenderBeginHook begin_hook,
    JPBSoftwareTriangleSink triangle_sink,
    JPBGameRuntimeModelRenderEndHook end_hook,
    void *user_data)
{
    if (runtime != NULL) {
        runtime->screenPolyRenderBeginHook = begin_hook;
        runtime->screenPolyTriangleSink = triangle_sink;
        runtime->screenPolyRenderEndHook = end_hook;
        runtime->screenPolyRenderUserData = user_data;
    }
}

void jpb_GameRuntimeSetTitleScreenDrawRenderHook(
    JPBGameRuntime *runtime,
    JPBGameRuntimeScreenDrawRenderHook hook,
    void *user_data)
{
    if (runtime != NULL) {
        runtime->titleScreenDrawRenderHook = hook;
        runtime->titleScreenDrawRenderUserData = user_data;
    }
}

void jpb_GameRuntimeSetGameplayCompositeHook(
    JPBGameRuntime *runtime,
    JPBGameRuntimeGameplayCompositeHook hook,
    void *user_data)
{
    if (runtime != NULL) {
        runtime->gameplayCompositeHook = hook;
        runtime->gameplayCompositeUserData = user_data;
    }
}

int jpb_GameRuntimeResolveLevelTexture(
    JPBGameRuntime *runtime,
    const char *texture_name,
    JPBSoftwareTexture *texture)
{
    return runtime != NULL
        ? game_runtime_resolve_texture(
              runtime->worldTextureCache, texture_name, texture)
        : 0;
}

static int game_runtime_update_enemy_actor(
    JPBGameRuntimeEnemyActor *actor,
    JPBEnemyOpcodeParseResult opcode_result,
    uint16_t opcode_boundary)
{
    if (opcode_result == JPB_ENEMY_OPCODE_PARSE_COMPLETE) {
        ++actor->authoredOpcodeFrameCount;
    } else {
        ++actor->opcodeBoundaryFrameCount;
        actor->opcodeBoundary = opcode_boundary;
    }
    if (actor->enemy->kungfu != NULL &&
        actor->enemy->kungfu->id ==
            &actor->player->playerRoot &&
        actor->enemy->kungfu->flags == 1) {
        ++actor->kungfuSchedulerFrameCount;
    }
    return 1;
}

typedef struct JPBGameRuntimeFrameContext {
    JPBGameRuntime *runtime;
    JPBSoftwareFramebuffer *framebuffer;
    JPBSoftwareRenderStats *stats;
    JPBSoftwareDepthBuffer depthBuffer;
    JPBSoftwareDepthBuffer glowDepthBuffer;
    int32_t previousAnimationIndices[JPB_ANIMATION_CAPACITY];
    int sharedDepthReady;
    int glowDepthReady;
    int result;
} JPBGameRuntimeFrameContext;

static JPBGameRuntimeFrameContext *game_runtime_active_frame;

static void game_runtime_capture_screen_poly(
    void *user_data,
    _Material *material,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    JPBGameRuntimeFrameContext *context =
        game_runtime_active_frame;
    size_t pixels_before;
    size_t pixels_added;
    int is_water;
    int result;
    JPBGameRuntimeScreenPolyDraw *draw = NULL;
    int copy_count = vertex_count;

    if (runtime == NULL || context == NULL ||
        context->runtime != runtime ||
        context->result != JPB_GAME_RUNTIME_OK) {
        return;
    }
    if (runtime->screenPolyDrawCount <
        JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY) {
        draw = &runtime->screenPolyDraws[
            runtime->screenPolyDrawCount];
        if (copy_count > JPB_SCREEN_POLY_VERTEX_CAPACITY) {
            copy_count = JPB_SCREEN_POLY_VERTEX_CAPACITY;
        }
        draw->texture = material;
        draw->vertexCount = vertex_count;
        draw->noScale = no_scale;
        draw->deferred = !context->sharedDepthReady ||
            runtime->modelRenderBeginHook != NULL;
        if (copy_count > 0 && vertices != NULL) {
            memcpy(
                draw->vertices,
                vertices,
                (size_t)copy_count *
                    sizeof(draw->vertices[0]));
        }
        if (copy_count < JPB_SCREEN_POLY_VERTEX_CAPACITY) {
            memset(
                &draw->vertices[copy_count],
                0,
                (size_t)(JPB_SCREEN_POLY_VERTEX_CAPACITY -
                         copy_count) *
                    sizeof(draw->vertices[0]));
        }
    } else {
        ++runtime->screenPolyDroppedCount;
    }
    ++runtime->screenPolyDrawCount;
    if (!context->sharedDepthReady ||
        runtime->modelRenderBeginHook != NULL) {
        return;
    }
    pixels_before = context->stats != NULL
        ? context->stats->pixels
        : 0;
    is_water = material != NULL &&
        game_runtime_path_stem_equals(
            material->filename, "a_water");
    result = jpb_SoftwareDrawScreenPoly(
        material,
        vertex_count,
        vertices,
        no_scale,
        context->framebuffer,
        &context->depthBuffer,
        context->stats);
    if (result != JPB_SOFTWARE_RENDER_OK) {
        context->result = JPB_GAME_RUNTIME_RENDER_FAILED;
        return;
    }
    if (context->stats != NULL) {
        pixels_added = context->stats->pixels - pixels_before;
        runtime->screenPolyCompositePixelCount += pixels_added;
    } else {
        pixels_added = 0;
    }
    if (is_water) {
        ++runtime->waterPolyDrawCount;
        runtime->waterPolyCompositePixelCount += pixels_added;
    }
}

static void game_runtime_flush_deferred_screen_polys(
    JPBGameRuntimeFrameContext *context)
{
    JPBGameRuntime *runtime;
    size_t draw_index;

    if (context == NULL || context->runtime == NULL ||
        context->framebuffer == NULL ||
        context->result != JPB_GAME_RUNTIME_OK ||
        !context->sharedDepthReady) {
        return;
    }
    runtime = context->runtime;
    if (runtime->screenPolyRenderBeginHook != NULL &&
        !runtime->screenPolyRenderBeginHook(
            runtime->screenPolyRenderUserData,
            context->framebuffer, &context->depthBuffer)) {
        context->result = JPB_GAME_RUNTIME_RENDER_FAILED;
        return;
    }
    for (draw_index = 0;
         draw_index < runtime->screenPolyDrawCount &&
         draw_index < JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY;
         ++draw_index) {
        JPBGameRuntimeScreenPolyDraw *draw =
            &runtime->screenPolyDraws[draw_index];
        size_t pixels_before;
        size_t pixels_added;
        int is_water;
        int result;

        if (!draw->deferred) {
            continue;
        }
        pixels_before = context->stats != NULL
            ? context->stats->pixels
            : 0;
        is_water = draw->texture != NULL &&
            game_runtime_path_stem_equals(
                draw->texture->filename, "a_water");
        result = runtime->screenPolyTriangleSink != NULL
            ? jpb_SoftwareDrawScreenPolyToSink(
                  draw->texture,
                  draw->vertexCount,
                  draw->vertices,
                  draw->noScale,
                  context->framebuffer,
                  &context->depthBuffer,
                  runtime->screenPolyTriangleSink,
                  runtime->screenPolyRenderUserData,
                  context->stats)
            : jpb_SoftwareDrawScreenPoly(
                  draw->texture,
                  draw->vertexCount,
                  draw->vertices,
                  draw->noScale,
                  context->framebuffer,
                  &context->depthBuffer,
                  context->stats);
        if (result != JPB_SOFTWARE_RENDER_OK) {
            context->result = JPB_GAME_RUNTIME_RENDER_FAILED;
            return;
        }
        draw->deferred = 0;
        if (context->stats != NULL) {
            pixels_added = context->stats->pixels - pixels_before;
            runtime->screenPolyCompositePixelCount += pixels_added;
        } else {
            pixels_added = 0;
        }
        if (is_water) {
            ++runtime->waterPolyDrawCount;
            runtime->waterPolyCompositePixelCount += pixels_added;
        }
    }
    if (runtime->screenPolyRenderEndHook != NULL &&
        !runtime->screenPolyRenderEndHook(
            runtime->screenPolyRenderUserData,
            context->framebuffer, &context->depthBuffer)) {
        context->result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
}

static void game_runtime_scene_after_animations(
    void *user_data, MATRIX *view)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    JPBGameRuntimeFrameContext *context =
        game_runtime_active_frame;

    (void)view;
    if (context == NULL || context->runtime != runtime ||
        context->result != JPB_GAME_RUNTIME_OK) {
        return;
    }
    if (runtime->authoredMotionReady) {
        int animation_index =
            runtime->animation->animRoot.objectID;

        if ((uint32_t)animation_index >=
                JPB_ANIMATION_CAPACITY ||
            !game_runtime_publish_authored_frame(
                runtime,
                context->previousAnimationIndices[
                    animation_index])) {
            context->result =
                JPB_GAME_RUNTIME_RENDER_FAILED;
            return;
        }
    }
    if (runtime->secondPlayerState != NULL &&
        runtime->secondPlayerState->authoredMotionReady) {
        int animation_index =
            runtime->secondPlayerState->animation->animRoot.objectID;

        if ((uint32_t)animation_index >=
                JPB_ANIMATION_CAPACITY ||
            !game_runtime_publish_second_player_frame(
                runtime,
                context->previousAnimationIndices[
                    animation_index])) {
            context->result =
                JPB_GAME_RUNTIME_RENDER_FAILED;
            return;
        }
    }
    if (runtime->enemyState != NULL) {
        size_t enemy_index;

        for (enemy_index = 0;
             enemy_index < JPB_GAME_RUNTIME_ENEMY_CAPACITY;
             ++enemy_index) {
            JPBGameRuntimeEnemyActor *actor =
                &runtime->enemyState->actors[enemy_index];
            int animation_index;

            if (!actor->authoredMotionReady ||
                !game_runtime_enemy_is_active(actor->enemy)) {
                continue;
            }
            animation_index =
                actor->animation->animRoot.objectID;
            if ((uint32_t)animation_index >=
                    JPB_ANIMATION_CAPACITY ||
                !game_runtime_publish_enemy_frame(
                    actor,
                    context->previousAnimationIndices[
                        animation_index])) {
                context->result =
                    JPB_GAME_RUNTIME_RENDER_FAILED;
                return;
            }
        }
        game_runtime_publish_primary_enemy(runtime);
    }
}

static void game_runtime_scene_after_world(
    void *user_data, MATRIX *view)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    JPBGameRuntimeFrameContext *context =
        game_runtime_active_frame;
    uint32_t clear_color;
    double started;

    if (context == NULL || context->runtime != runtime ||
        context->result != JPB_GAME_RUNTIME_OK) {
        return;
    }
    if (runtime->topView) {
        context->result = jpb_SoftwareRenderJpxWireframe(
            &runtime->scene,
            view,
            context->framebuffer,
            0,
            context->stats) == JPB_SOFTWARE_RENDER_OK
                ? JPB_GAME_RUNTIME_OK
                : JPB_GAME_RUNTIME_RENDER_FAILED;
        return;
    }
    clear_color = runtime->world != NULL
        ? ((uint32_t)runtime->world->bkColor.r << 16) |
          ((uint32_t)runtime->world->bkColor.g << 8) |
          (uint32_t)runtime->world->bkColor.b
        : 0;
    started = game_runtime_wall_seconds();
    if (!game_runtime_prepare_depth_buffer(
            runtime,
            context->framebuffer,
            &context->depthBuffer)) {
        context->result = JPB_GAME_RUNTIME_RENDER_FAILED;
        return;
    }
    context->result =
        (runtime->levelRenderMesh != NULL &&
                 runtime->levelRenderHook != NULL
             ? runtime->levelRenderHook(
                   runtime->levelRenderUserData,
                   runtime->levelRenderMesh,
                   &runtime->scene,
                   view,
                   context->framebuffer,
                   clear_color,
                   game_runtime_resolve_texture,
                   runtime->worldTextureCache,
                   &context->depthBuffer,
                   context->stats)
             : runtime->levelRenderMesh != NULL
             ? jpb_SoftwareRenderLevelMesh(
                   runtime->levelRenderMesh,
                   &runtime->scene,
                   view,
                   context->framebuffer,
                   clear_color,
                   game_runtime_resolve_texture,
                   runtime->worldTextureCache,
                   &context->depthBuffer,
                   context->stats)
             : jpb_SoftwareRenderJpxMaterialized(
                   &runtime->scene,
                   view,
                   context->framebuffer,
                   clear_color,
                   game_runtime_resolve_texture,
                   runtime->worldTextureCache,
                   &context->depthBuffer,
                   context->stats)) == JPB_SOFTWARE_RENDER_OK
            ? JPB_GAME_RUNTIME_OK
            : JPB_GAME_RUNTIME_RENDER_FAILED;
    context->sharedDepthReady =
        context->result == JPB_GAME_RUNTIME_OK;
    runtime->profileWorldSeconds +=
        game_runtime_wall_seconds() - started;
    if (context->sharedDepthReady) {
        runtime->worldLoadedTextures =
            runtime->worldTextureCache->loadedTextureCount;
        runtime->worldRenderedPixels =
            context->stats != NULL
                ? context->stats->pixels
                : 0;
    }
}

static int game_runtime_scene_render_model(
    JPBGameRuntimeFrameContext *context,
    const JPBBmdView *view,
    modelObject *model,
    const _animFrame *key_frame,
    const FVECTOR *position,
    int32_t yaw,
    JPBGameRuntimeTextureCache *texture_cache)
{
    JPBGameRuntime *runtime = context->runtime;
    int result = context->sharedDepthReady &&
            runtime->modelTriangleSink != NULL
        ? jpb_SoftwareRenderBmdMaterializedWithDepthToSink(
            view, model, key_frame, position, yaw,
            scene_GetSceneMatrix(), &runtime->scene,
            context->framebuffer, game_runtime_resolve_texture,
            texture_cache, &context->depthBuffer,
            runtime->modelTriangleSink,
            runtime->modelRenderUserData, context->stats)
        : context->sharedDepthReady
        ? jpb_SoftwareRenderBmdMaterializedWithDepth(
            view,
            model,
            key_frame,
            position,
            yaw,
            scene_GetSceneMatrix(),
            &runtime->scene,
            context->framebuffer,
            game_runtime_resolve_texture,
            texture_cache,
            &context->depthBuffer,
            context->stats)
        : jpb_SoftwareRenderBmdMaterialized(
            view,
            model,
            key_frame,
            position,
            yaw,
            scene_GetSceneMatrix(),
            &runtime->scene,
            context->framebuffer,
            game_runtime_resolve_texture,
            texture_cache,
            context->stats);

    return result == JPB_SOFTWARE_RENDER_OK;
}

static void game_runtime_scene_after_models(
    void *user_data, MATRIX *view)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    JPBGameRuntimeFrameContext *context =
        game_runtime_active_frame;
    double started;

    (void)view;
    if (context == NULL || context->runtime != runtime ||
        context->result != JPB_GAME_RUNTIME_OK) {
        return;
    }
    started = game_runtime_wall_seconds();
    if (runtime->modelRenderBeginHook != NULL &&
        !runtime->modelRenderBeginHook(
            runtime->modelRenderUserData,
            context->framebuffer, &context->depthBuffer)) {
        context->result = JPB_GAME_RUNTIME_RENDER_FAILED;
        return;
    }
    if (runtime->actorModel.pRootNode != NULL) {
        FVECTOR position = {
            runtime->physics->pos.vx,
            runtime->physics->pos.vy,
            runtime->physics->pos.vz
        };
        size_t triangles_before = context->stats != NULL
            ? context->stats->modelTriangles
            : 0;
        size_t pixels_before = context->stats != NULL
            ? context->stats->modelPixels
            : 0;

        if (!game_runtime_scene_render_model(
                context,
                &runtime->bmdView,
                &runtime->actorModel,
                runtime->actorScene != NULL
                    ? runtime->actorScene->pKeyFrameModel
                    : NULL,
                &position,
                runtime->physics->angle.vy,
                runtime->textureCache)) {
            context->result =
                JPB_GAME_RUNTIME_RENDER_FAILED;
            return;
        }
        runtime->playerRenderedTriangles =
            context->stats != NULL
                ? context->stats->modelTriangles -
                    triangles_before
                : 0;
        runtime->playerRenderedPixels =
            context->stats != NULL
                ? context->stats->modelPixels - pixels_before
                : 0;
        if (runtime->playerRenderedPixels != 0) {
            ++runtime->playerVisibleFrameCount;
        }
    }
    if (runtime->secondPlayerState != NULL &&
        runtime->secondPlayerState->model.pRootNode != NULL) {
        JPBGameRuntimeSecondPlayerState *state =
            runtime->secondPlayerState;
        FVECTOR position = {
            runtime->inactivePlayerPhysics->pos.vx,
            runtime->inactivePlayerPhysics->pos.vy,
            runtime->inactivePlayerPhysics->pos.vz
        };
        size_t triangles_before = context->stats != NULL
            ? context->stats->modelTriangles
            : 0;
        size_t pixels_before = context->stats != NULL
            ? context->stats->modelPixels
            : 0;

        if (!game_runtime_scene_render_model(
                context,
                &state->bmdView,
                &state->model,
                runtime->inactivePlayerScene != NULL
                    ? runtime->inactivePlayerScene->pKeyFrameModel
                    : NULL,
                &position,
                runtime->inactivePlayerPhysics->angle.vy,
                state->textureCache)) {
            context->result =
                JPB_GAME_RUNTIME_RENDER_FAILED;
            return;
        }
        state->renderedTriangles =
            context->stats != NULL
                ? context->stats->modelTriangles - triangles_before
                : 0;
        state->renderedPixels =
            context->stats != NULL
                ? context->stats->modelPixels - pixels_before
                : 0;
    }
    if (runtime->enemyState != NULL) {
        size_t index;

        for (index = 0;
             index < JPB_GAME_RUNTIME_ENEMY_CAPACITY;
             ++index) {
            JPBGameRuntimeEnemyActor *actor =
                &runtime->enemyState->actors[index];
            FVECTOR position;
            size_t triangles_before;
            size_t pixels_before;

            if (!actor->authoredMotionReady ||
                actor->model.pRootNode == NULL ||
                actor->assetClass == NULL) {
                continue;
            }
            position.vx = actor->physics->pos.vx;
            position.vy = actor->physics->pos.vy;
            position.vz = actor->physics->pos.vz;
            triangles_before = context->stats != NULL
                ? context->stats->modelTriangles
                : 0;
            pixels_before = context->stats != NULL
                ? context->stats->modelPixels
                : 0;
            if (!game_runtime_scene_render_model(
                    context,
                    &actor->assetClass->bmdView,
                    &actor->model,
                    actor->scene != NULL
                        ? actor->scene->pKeyFrameModel
                        : NULL,
                    &position,
                    actor->physics->angle.vy,
                    actor->assetClass->textureCache)) {
                context->result =
                    JPB_GAME_RUNTIME_RENDER_FAILED;
                return;
            }
            if (!actor->assetClass->wasRendered) {
                actor->assetClass->wasRendered = 1;
                ++runtime->enemyRenderedClassCount;
            }
            actor->renderedTriangles =
                context->stats != NULL
                    ? context->stats->modelTriangles -
                        triangles_before
                    : 0;
            actor->renderedPixels =
                context->stats != NULL
                    ? context->stats->modelPixels - pixels_before
                    : 0;
        }
    }
    if (runtime->modelRenderEndHook != NULL &&
        !runtime->modelRenderEndHook(
            runtime->modelRenderUserData,
            context->framebuffer, &context->depthBuffer)) {
        context->result = JPB_GAME_RUNTIME_RENDER_FAILED;
        return;
    }
    runtime->profileModelsSeconds +=
        game_runtime_wall_seconds() - started;
}

static void game_runtime_scene_before_player_process(
    void *user_data, MATRIX *view)
{
    JPBGameRuntime *runtime = (JPBGameRuntime *)user_data;
    size_t index;

    (void)view;
    if (runtime == NULL || runtime->enemyState == NULL ||
        runtime->player == NULL) {
        return;
    }
    for (index = 0;
         index < JPB_GAME_RUNTIME_ENEMY_CAPACITY;
         ++index) {
        JPBGameRuntimeEnemyActor *actor =
            &runtime->enemyState->actors[index];

        if (actor->authoredMotionReady &&
            actor->player != NULL) {
            game_runtime_measure_hot_node_distance(
                runtime, actor);
            actor->schedulerHitBefore =
                actor->player->hitNumber;
        }
    }
}

static void game_runtime_scene_level_owner(
    void *user_data,
    int level,
    int argument0,
    int argument1,
    int argument2)
{
    (void)user_data;
    (void)argument0;
    (void)argument1;
    (void)argument2;
    if (level == 15) {
        level_Mini4();
    }
}

void jpb_GameRuntimeShutdown(JPBGameRuntime *runtime)
{
    if (runtime == NULL) {
        return;
    }
    if (runtime->drawTextureHookReady) {
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        runtime->drawTextureHookReady = 0;
    }
    if (runtime->screenPolyHookReady) {
        jpb_WHookSetScreenPolyHook(NULL, NULL);
        runtime->screenPolyHookReady = 0;
    }
    if (runtime->barHookReady) {
        jpb_GameSetBarHook(NULL, NULL);
        runtime->barHookReady = 0;
    }
    if (runtime->playerTileHookReady) {
        jpb_PlayerSetTileHook(NULL, NULL);
        runtime->playerTileHookReady = 0;
    }
    if (runtime->textHookReady) {
        jpb_TextSetDrawHook(NULL, NULL);
        jpb_TextSetPsxTextureHook(NULL, NULL);
        runtime->textHookReady = 0;
    }
    if (runtime->draw3dTextHookReady) {
        jpb_DebugTextSetDraw3dHook(NULL, NULL);
        runtime->draw3dTextHookReady = 0;
    }
    if (runtime->spriteDisplayHookReady) {
        jpb_SpriteSetDisplayHook(NULL, NULL);
        runtime->spriteDisplayHookReady = 0;
    }
    jpb_PortableTextShutdown();
    if (runtime->glowHookReady) {
        jpb_FxSetScreenGlowHook(NULL, NULL);
        runtime->glowHookReady = 0;
    }
    if (runtime->cylinderHookReady) {
        jpb_SpriteSetCylinderHook(NULL, NULL);
        runtime->cylinderHookReady = 0;
    }
    if (runtime->powerupDrawHookReady) {
        jpb_PwrupSetDrawHook(NULL, NULL);
        runtime->powerupDrawHookReady = 0;
    }
    if (runtime->playerProcessObserverReady) {
        jpb_PlayerSetProcessObserver(NULL, NULL);
        runtime->playerProcessObserverReady = 0;
    }
    if (runtime->bulletLaunchObserverReady) {
        jpb_BulletSetLaunchObserver(NULL, NULL);
        runtime->bulletLaunchObserverReady = 0;
    }
    if (runtime->sceneMiddleRenderHooksReady) {
        jpb_SceneSetMiddleRenderHooks(NULL, NULL);
        runtime->sceneMiddleRenderHooksReady = 0;
    }
    jpb_LoaderSetEnemyCreateProvider(NULL, NULL);
    file_SetTextureLoadHook(NULL);
    file_SetChunkLoadHooks(NULL, NULL);
    jpb_PhysicsSetGeometryStreamResolver(NULL, NULL);
    jpb_PwrupReleaseData();
    texture_Flush((unsigned)TT_ANY);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    if (runtime->enemyState != NULL) {
        game_runtime_release_enemy_classes(
            runtime->enemyState);
    }
    if (runtime->world != NULL) {
        free(runtime->world->apEnemy);
        free(runtime->world->apAI);
        free(runtime->world->apActorNames);
    }
    if (gpWorld == runtime->world) {
        gSCENE_READY = 0;
        gpWorld = NULL;
        jonnylevel = NULL;
        leveldata = NULL;
        jpb_LevelDataClearBounds();
        texturebase = NULL;
        colorbase = NULL;
        vertbase = NULL;
        mapyend = 0;
    }
    /* Clear the exact model owner before releasing its archive-backed nodes. */
    model_InitModels();
    jpb_ModelSetGeometryBounds(NULL, 0);
    free(runtime->meshStorage);
    free(runtime->collisionStorage);
    free(runtime->cadStorage);
    free(runtime->bmdStorage);
    free(runtime->comboStorage);
    if (runtime->secondPlayerState != NULL) {
        free(runtime->secondPlayerState->cadStorage);
        free(runtime->secondPlayerState->bmdStorage);
        free(runtime->secondPlayerState->comboStorage);
        game_runtime_free_texture_cache(
            runtime->secondPlayerState->textureCache);
        free(runtime->secondPlayerState);
    }
    free(runtime->enemyState);
    free(runtime->huffmanTables);
    game_runtime_free_texture_cache(
        runtime->worldTextureCache);
    game_runtime_free_texture_cache(
        runtime->textureCache);
    game_runtime_free_texture_cache(
        runtime->uiTextureCache);
    game_runtime_free_texture_cache(
        runtime->defaultTextureCache);
    free(runtime->renderDepthBuffer);
    free(runtime->glowDepthBuffer);
    free(runtime->world);
    memset(runtime, 0, sizeof(*runtime));
}

int jpb_GameRuntimeFrame(
    JPBGameRuntime *runtime,
    float elapsed_seconds,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareRenderStats *stats)
{
    uint32_t controls;
    uint32_t held_controls;
    float direction_x = 0.0f;
    float direction_y = 0.0f;
    MATRIX *view = NULL;
    JPBGameRuntimeFrameContext context;
    int animation_index;
    int enemy_frame_processed;
    double effects_started;

    if (runtime == NULL || framebuffer == NULL) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    runtime->screenDrawCount = 0;
    runtime->drawOrder = 0;
    runtime->screenDrawDroppedCount = 0;
    runtime->screenDrawCompositePixelCount = 0;
    runtime->screenDrawTextureAlphaModulatedPixelCount = 0;
    runtime->itemHudTextureAlphaModulatedPixelCount = 0;
    runtime->creditHudTextureAlphaModulatedPixelCount = 0;
    runtime->rescueHudTextureAlphaModulatedPixelCount = 0;
    runtime->playerHudTileDrawCount = 0;
    runtime->playerHudTileDroppedCount = 0;
    runtime->playerHudTileCompositePixelCount = 0;
    runtime->textDrawCount = 0;
    runtime->textDrawDroppedCount = 0;
    runtime->textDrawCompositePixelCount = 0;
    runtime->textTrueTypeDrawCount = 0;
    runtime->textFallbackDrawCount = 0;
    runtime->maximumTextPointSize = 0;
    runtime->maximumTextMeasuredWidth = 0;
    runtime->maximumTextMeasuredHeight = 0;
    runtime->draw3dTextDrawCount = 0;
    runtime->draw3dTextDroppedCount = 0;
    runtime->spriteDisplayDrawCount = 0;
    runtime->spriteDisplayDroppedCount = 0;
    runtime->psxTextureDrawCount = 0;
    runtime->psxTextureDrawDroppedCount = 0;
    runtime->glowDrawCount = 0;
    runtime->glowDrawDroppedCount = 0;
    runtime->glowDrawCompositePixelCount = 0;
    runtime->glowDrawDepthRejectedPixelCount = 0;
    runtime->cylinderDrawCount = 0;
    runtime->screenPolyDrawCount = 0;
    runtime->screenPolyDroppedCount = 0;
    runtime->screenPolyCompositePixelCount = 0;
    runtime->waterPolyDrawCount = 0;
    runtime->waterPolyCompositePixelCount = 0;
    runtime->powerupDrawCount = 0;
    OptionStruct.ScreenWidth =
        (uint32_t)(framebuffer->width > 0
                       ? framebuffer->width
                       : 0);
    OptionStruct.ScreenHeight =
        (uint32_t)(framebuffer->height > 0
                       ? framebuffer->height
                       : 0);
    scaleAdjustment = getScaleAdjustment();
    scaleAdjustmentMM = getScaleAdjustmentMM();
    if (!(elapsed_seconds >= 0.0f)) {
        elapsed_seconds = 0.0f;
    }
    if (elapsed_seconds > 0.1f) {
        elapsed_seconds = 0.1f;
    }
    deltaTime = elapsed_seconds;
    /*
     * Camera preparation can inspect the host state without consuming the
     * original edge detector. player_gProcessPlayers samples and publishes
     * the authoritative Pad channels at the post-render ownership boundary.
     */
    controls = jpb_InputReadRawPad(
        (int)runtime->player->playerPad.padnum);
    held_controls = controls;
    /* ReadKeyboardInput emits LEFT for physical right and vice versa. */
    if ((held_controls & JPB_PAD_LEFT) != 0) direction_x += 1.0f;
    if ((held_controls & JPB_PAD_RIGHT) != 0) direction_x -= 1.0f;
    if ((held_controls & JPB_PAD_UP) != 0) direction_y -= 1.0f;
    if ((held_controls & JPB_PAD_DOWN) != 0) direction_y += 1.0f;

    if ((controls & JPB_PAD_ZOOM_IN) != 0) {
        runtime->orbitDistance *= 1.0f - elapsed_seconds;
    }
    if ((controls & JPB_PAD_ZOOM_OUT) != 0) {
        runtime->orbitDistance *= 1.0f + elapsed_seconds;
    }
    if (runtime->orbitDistance < runtime->minimumOrbitDistance) {
        runtime->orbitDistance = runtime->minimumOrbitDistance;
    }
    if (runtime->orbitDistance > runtime->maximumOrbitDistance) {
        runtime->orbitDistance = runtime->maximumOrbitDistance;
    }
    /*
     * The matched executable initializes these gameplay scales to 0.5 and
     * 0x800 and never writes them again. Wall-clock deltaTime remains live
     * for the PC front end, while gameplay advances at its authored 60 Hz
     * fixed step.
     */
    fGlobalFrameRate = 0.5f;
    gGlobalFrameRate = 0x800;
    if (runtime->authoredMotionReady && player1InputType == 0) {
        /*
         * Keyboard input has no analog provider, so derive its camera-relative
         * axes from the held direction bits.  XInput already published the
         * real stick axes in pc_read_pad; overwriting them here made right/left
         * movement select the wrong facing and discarded analog magnitude.
         */
        g_p1X = direction_x;
        g_p1Y = direction_y;
    }
    if (GameStruct.versusModeFlag != 0 &&
        runtime->secondPlayerState != NULL &&
        runtime->inactivePlayerPhysics != NULL) {
        runtime->targetX =
            (runtime->physics->pos.vx +
             runtime->inactivePlayerPhysics->pos.vx) * 0.5f;
        runtime->targetY =
            (runtime->physics->pos.vy +
             runtime->inactivePlayerPhysics->pos.vy) * 0.5f;
        runtime->targetZ =
            (runtime->physics->pos.vz +
             runtime->inactivePlayerPhysics->pos.vz) * 0.5f;
    }
    if (runtime->world != NULL && runtime->physics != NULL) {
        /*
         * The reference scene owner keeps WorldData.location synchronized
         * with the live gameplay focus before enemy activation/range tests.
         * The portable camera seam does not yet own that store, so publish
         * the active single-player position here.
         */
        runtime->world->location.vx = (int32_t)runtime->targetX;
        runtime->world->location.vy = (int32_t)runtime->targetY;
        runtime->world->location.vz = (int32_t)runtime->targetZ;
    }
    for (animation_index = 0;
         animation_index < JPB_ANIMATION_CAPACITY;
         ++animation_index) {
        context.previousAnimationIndices[animation_index] =
            maAnimationData[animation_index].animFrameIndex;
    }

    game_runtime_publish_level_camera_owner(runtime);
    if (!game_runtime_build_camera(runtime)) {
        return JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    gCamera = runtime->camera;
    memset(&context.depthBuffer, 0, sizeof(context.depthBuffer));
    memset(&context.glowDepthBuffer, 0, sizeof(context.glowDepthBuffer));
    context.runtime = runtime;
    context.framebuffer = framebuffer;
    context.stats = stats;
    context.sharedDepthReady = 0;
    context.glowDepthReady = 0;
    context.result = JPB_GAME_RUNTIME_OK;
    game_runtime_active_frame = &context;
    enemy_frame_processed =
        gSCENE_READY == 0 ||
        initialLevelPauseDelay < 2 ||
        (GameStruct.GameState & UINT32_C(0x02000000)) == 0;
    scene_middleRender(NULL);
    game_runtime_active_frame = NULL;
    if (context.result != JPB_GAME_RUNTIME_OK) {
        return context.result;
    }
    ++runtime->profileFrameCount;
    /*
     * Exact game_ProcessStatus ordering: scene/input processing completes,
     * then each live world player conforms its authored model nodes.
     */
    brainutl_ConformGeomNodes(runtime->player);
    if (GameStruct.NumPlayers == 2 &&
        runtime->secondPlayerState != NULL &&
        runtime->inactivePlayer != NULL) {
        brainutl_ConformGeomNodes(runtime->inactivePlayer);
    }
    view = scene_GetSceneMatrix();
    runtime->camera = gCamera;
    /*
     * scene_middleRender publishes the exact camera-derived environment in
     * the game-owned global. Keep the descriptive runtime snapshot aligned
     * with that owner for host diagnostics and platform presentation.
     */
    runtime->environment = gSceneGeometryEnv;
    runtime->targetX = runtime->physics->pos.vx;
    runtime->targetY = runtime->physics->pos.vy;
    runtime->targetZ = runtime->physics->pos.vz;

    if (runtime->enemyState != NULL) {
        int class_seen[
            JPB_GAME_RUNTIME_ENEMY_CLASS_CAPACITY] = {0};
        size_t active_count = 0;
        size_t active_class_count = 0;
        size_t index;
        uint16_t unsupported_opcode = 0;
        JPBEnemyOpcodeParseResult opcode_result =
            jpb_enemy_LastFrameResult(
                &unsupported_opcode);

        for (index = 0;
             index < JPB_GAME_RUNTIME_ENEMY_CAPACITY;
             ++index) {
            JPBGameRuntimeEnemyActor *actor =
                &runtime->enemyState->actors[index];

            if (!actor->authoredMotionReady) {
                continue;
            }
            if (!game_runtime_enemy_is_active(actor->enemy)) {
                game_runtime_release_enemy_actor(actor);
                continue;
            }
            ++active_count;
            if (actor->assetClass != NULL &&
                actor->assetClass >=
                    runtime->enemyState->classes &&
                actor->assetClass <
                    runtime->enemyState->classes +
                        runtime->enemyState->classCount) {
                size_t class_index =
                    (size_t)(actor->assetClass -
                        runtime->enemyState->classes);

                class_seen[class_index] = 1;
                if (!actor->assetClass->wasActive) {
                    actor->assetClass->wasActive = 1;
                    ++runtime->enemyActivatedClassCount;
                }
            }
            if (enemy_frame_processed) {
                (void)game_runtime_update_enemy_actor(
                    actor,
                    opcode_result,
                    unsupported_opcode);
            }
        }
        for (index = 0;
             index < runtime->enemyState->classCount;
             ++index) {
            active_class_count += class_seen[index] != 0;
        }
        runtime->enemyActorCount = active_count;
        runtime->enemyActiveClassCount = active_class_count;
        if (active_count > runtime->enemyActorPeakCount) {
            runtime->enemyActorPeakCount = active_count;
        }
        if (active_class_count >
            runtime->enemyActiveClassPeakCount) {
            runtime->enemyActiveClassPeakCount =
                active_class_count;
        }
        game_runtime_publish_primary_enemy(runtime);
    }
    if (runtime->powerupCount != 0 && poopArray != NULL) {
        size_t powerup_index;
        size_t collected_count = 0;

        for (powerup_index = 0;
             powerup_index < runtime->powerupCount;
             ++powerup_index) {
            collected_count +=
                ((uint16_t)poopArray[powerup_index].pos.pad &
                 JPB_POWERUP_COLLECTED_FLAG) != 0;
        }
        runtime->powerupCollectedCount = collected_count;
    }
    effects_started = game_runtime_wall_seconds();
    game_runtime_flush_deferred_screen_polys(&context);
    runtime->profileScreenPolySeconds +=
        game_runtime_wall_seconds() - effects_started;
    if (context.result != JPB_GAME_RUNTIME_OK) {
        return context.result;
    }
    if (context.sharedDepthReady &&
        runtime->gameplayCompositeHook == NULL) {
        double snapshot_started = game_runtime_wall_seconds();
        /*
         * PDB fx_screenGlow emits depth-tested immediate quads. Capture the
         * occluder surface after world, actors, enemies, and deferred
         * immediate polys have all contributed to the shared depth buffer.
         */
        context.glowDepthReady =
            game_runtime_snapshot_glow_depth_buffer(
                runtime,
                &context.depthBuffer,
                &context.glowDepthBuffer);
        if (!context.glowDepthReady) {
            return JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        runtime->profileDepthSnapshotSeconds +=
            game_runtime_wall_seconds() - snapshot_started;
    }
    enemy_CheckTeleport();
    {
        double hud_started = game_runtime_wall_seconds();

        game_runtime_flush_ordered_title_draws(runtime, framebuffer);
        runtime->profileHudSeconds +=
            game_runtime_wall_seconds() - hud_started;
    }
    if (runtime->gameplayCompositeHook != NULL) {
        size_t saved_screen_pixels =
            runtime->screenDrawCompositePixelCount;
        size_t saved_player_hud_pixels =
            runtime->playerHudTileCompositePixelCount;
        size_t saved_text_pixels =
            runtime->textDrawCompositePixelCount;
        size_t saved_alpha_pixels =
            runtime->screenDrawTextureAlphaModulatedPixelCount;
        size_t saved_item_alpha_pixels =
            runtime->itemHudTextureAlphaModulatedPixelCount;
        size_t saved_credit_alpha_pixels =
            runtime->creditHudTextureAlphaModulatedPixelCount;
        size_t saved_rescue_alpha_pixels =
            runtime->rescueHudTextureAlphaModulatedPixelCount;
        size_t saved_text_draw_pixels[
            JPB_GAME_RUNTIME_TEXT_DRAW_CAPACITY];
        size_t draw_index;
        double stage_started;

        for (draw_index = 0;
             draw_index < runtime->textDrawCount;
             ++draw_index) {
            saved_text_draw_pixels[draw_index] =
                runtime->textDraws[draw_index].compositePixels;
        }
        stage_started = game_runtime_wall_seconds();
        if (!runtime->gameplayCompositeHook(
                runtime->gameplayCompositeUserData,
                JPB_GAMEPLAY_COMPOSITE_HUD_BLACK,
                framebuffer, runtime->glowDraws,
                runtime->glowDrawCount, view, stats)) {
            return JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        runtime->profileCompositeUploadSeconds +=
            game_runtime_wall_seconds() - stage_started;
        stage_started = game_runtime_wall_seconds();
        game_runtime_clear_framebuffer(
            framebuffer, UINT32_C(0x00ffffff));
        game_runtime_flush_ordered_title_draws(runtime, framebuffer);
        runtime->profileHudReplaySeconds +=
            game_runtime_wall_seconds() - stage_started;
        stage_started = game_runtime_wall_seconds();
        if (!runtime->gameplayCompositeHook(
                runtime->gameplayCompositeUserData,
                JPB_GAMEPLAY_COMPOSITE_HUD_WHITE,
                framebuffer, runtime->glowDraws,
                runtime->glowDrawCount, view, stats)) {
            return JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        runtime->profileCompositeUploadSeconds +=
            game_runtime_wall_seconds() - stage_started;
        runtime->screenDrawCompositePixelCount = saved_screen_pixels;
        runtime->playerHudTileCompositePixelCount =
            saved_player_hud_pixels;
        runtime->textDrawCompositePixelCount = saved_text_pixels;
        runtime->screenDrawTextureAlphaModulatedPixelCount =
            saved_alpha_pixels;
        runtime->itemHudTextureAlphaModulatedPixelCount =
            saved_item_alpha_pixels;
        runtime->creditHudTextureAlphaModulatedPixelCount =
            saved_credit_alpha_pixels;
        runtime->rescueHudTextureAlphaModulatedPixelCount =
            saved_rescue_alpha_pixels;
        for (draw_index = 0;
             draw_index < runtime->textDrawCount;
             ++draw_index) {
            runtime->textDraws[draw_index].compositePixels =
                saved_text_draw_pixels[draw_index];
        }
        stage_started = game_runtime_wall_seconds();
        if (!runtime->gameplayCompositeHook(
                runtime->gameplayCompositeUserData,
                JPB_GAMEPLAY_COMPOSITE_FINISH,
                framebuffer, runtime->glowDraws,
                runtime->glowDrawCount, view, stats)) {
            return JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        runtime->profileCompositeFinishSeconds +=
            game_runtime_wall_seconds() - stage_started;
        runtime->glowDrawCompositePixelCount =
            runtime->glowDrawCount != 0 ? 1 : 0;
    } else {
        double glow_started = game_runtime_wall_seconds();

        game_runtime_flush_glow_draws(
            runtime,
            view,
            framebuffer,
            context.glowDepthReady ? &context.glowDepthBuffer : NULL,
            stats);
        runtime->profileGlowSeconds +=
            game_runtime_wall_seconds() - glow_started;
    }
    runtime->profileEffectsSeconds +=
        game_runtime_wall_seconds() - effects_started;
    /* Return post-frame state to its exact PDB-named scene owner. */
    scene_postRender();
    return JPB_GAME_RUNTIME_OK;
}

int jpb_GameRuntimeTitleFrame(
    JPBGameRuntime *runtime,
    JPBSoftwareFramebuffer *framebuffer)
{
    unsigned menu_mode;

    if (runtime == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width ||
        !runtime->textHookReady) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    runtime->textDrawCount = 0;
    runtime->textDrawDroppedCount = 0;
    runtime->textDrawCompositePixelCount = 0;
    runtime->textTrueTypeDrawCount = 0;
    runtime->textFallbackDrawCount = 0;
    runtime->maximumTextPointSize = 0;
    runtime->maximumTextMeasuredWidth = 0;
    runtime->maximumTextMeasuredHeight = 0;
    runtime->psxTextureDrawCount = 0;
    runtime->psxTextureDrawDroppedCount = 0;
    runtime->screenDrawCount = 0;
    runtime->drawOrder = 0;
    runtime->screenDrawDroppedCount = 0;
    runtime->screenDrawCompositePixelCount = 0;
    runtime->screenDrawTextureAlphaModulatedPixelCount = 0;
    runtime->itemHudTextureAlphaModulatedPixelCount = 0;
    runtime->creditHudTextureAlphaModulatedPixelCount = 0;
    runtime->rescueHudTextureAlphaModulatedPixelCount = 0;
    runtime->screenPolyDrawCount = 0;
    runtime->screenPolyDroppedCount = 0;
    runtime->screenPolyCompositePixelCount = 0;
    OptionStruct.ScreenWidth = (uint32_t)framebuffer->width;
    OptionStruct.ScreenHeight = (uint32_t)framebuffer->height;
    scaleAdjustment = getScaleAdjustment();
    scaleAdjustmentMM = getScaleAdjustmentMM();
    menu_mainLoop();

    if (runtime->titleScreenDrawRenderHook != NULL) {
        if (!runtime->titleScreenDrawRenderHook(
                runtime->titleScreenDrawRenderUserData,
                runtime->screenDraws,
                runtime->screenDrawCount,
                framebuffer)) {
            return JPB_GAME_RUNTIME_RENDER_FAILED;
        }
    } else {
        game_runtime_flush_screen_draws(runtime, framebuffer);
    }
    game_runtime_flush_text_draws(runtime, framebuffer);
    menu_mode = menuVars.menuMode[menuVars.menuModeSP & 7u];

    /*
     * State 0x66 is the recovered level-load handoff.  It deliberately has
     * no menu definition or draw owner: the platform host consumes it and
     * starts the selected level.  Treating that empty transition frame as a
     * rendering failure made an interactive selection exit cleanly with
     * status 5 before the host could complete the handoff.
     */
    return runtime->screenDrawDroppedCount == 0 &&
           runtime->textDrawDroppedCount == 0 &&
           (runtime->textDrawCount != 0 ||
            runtime->screenDrawCount != 0 ||
            menu_mode == 0x13 || menu_mode == 0x66)
        ? JPB_GAME_RUNTIME_OK
        : JPB_GAME_RUNTIME_RENDER_FAILED;
}
