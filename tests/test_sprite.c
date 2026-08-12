#include "jpb/sprite.h"

#include "jpb/alloc.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/debugtext.h"
#include "jpb/game.h"
#include "jpb/scene.h"
#include "jpb/whook.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int draw_text_calls;
static float draw_text_x;
static float draw_text_y;
static float draw_text_z;
static float draw_text_scale;
static uint32_t draw_text_color;
static char draw_text_text[64];

typedef struct SpriteDrawTrace {
    int texture_calls;
    _Material *texture;
    SCREENRECT destination;
    SCREENRECT source;
    CVECTOR color;
    float depth;
    int polygon_calls;
    int vertex_count;
    int no_scale;
    JPBScreenPolyVertex vertices[4];
} SpriteDrawTrace;

static void capture_texture(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth)
{
    SpriteDrawTrace *trace = (SpriteDrawTrace *)user_data;

    ++trace->texture_calls;
    trace->texture = texture;
    trace->destination = *destination;
    trace->source = *source;
    trace->color = color;
    trace->depth = layer_depth;
}

static void capture_polygon(
    void *user_data,
    _Material *material,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale)
{
    SpriteDrawTrace *trace = (SpriteDrawTrace *)user_data;

    ++trace->polygon_calls;
    trace->texture = material;
    trace->vertex_count = vertex_count;
    trace->no_scale = no_scale;
    if (vertex_count == 4) {
        memcpy(trace->vertices, vertices, sizeof(trace->vertices));
    }
}

static int keep_sprite_alive(int32_t *sprite)
{
    (void)sprite;
    return 0;
}

static void capture_draw3d_text(
    float x,
    float y,
    float z,
    float scale,
    uint32_t color,
    const char *text,
    void *user_data)
{
    (void)user_data;
    ++draw_text_calls;
    draw_text_x = x;
    draw_text_y = y;
    draw_text_z = z;
    draw_text_scale = scale;
    draw_text_color = color;
    (void)snprintf(
        draw_text_text, sizeof(draw_text_text), "%s", text);
}

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                               \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static void reset_sprite_runtime(void)
{
    meminit();
    sprite_gInitSprites();
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    OptionStruct.FunFactor &= UINT8_C(0xfe);
    gGlobalFrameRate = 4096;
    framerate = 1.0f;
    mDrawingSurfaceId = 0;
    jpb_DebugTextSetDraw3dHook(
        capture_draw3d_text, NULL);
}

static int test_hide_scb(void)
{
    SCB scb;

    memset(&scb, 0, sizeof(scb));
    scb.scb_flags = 0x21;
    sprite_gHideSCB(&scb);
    CHECK(scb.scb_flags == 0x61);
    return 0;
}

static int test_display_sprite(void)
{
    _Material material;
    SCB *scb;
    SCB *same;

    reset_sprite_runtime();
    memset(&material, 0, sizeof(material));
    effects1Handle[40] = &material;

    scb = sprite_DisplaySprite(
        NULL, 40, 10, 20, -30, 40, 8);
    CHECK(scb != NULL);
    CHECK(mSCBDraw[0].head == (Node *)scb);
    CHECK(mSCBDraw[0].tail == (Node *)scb);
    CHECK(scb->scb_Texture == &material);
    CHECK((scb->scb_flags & 2) != 0);
    CHECK(scb->scb_vertex0.vx == 10.0f);
    CHECK(scb->scb_vertex0.vy == 20.0f);
    CHECK(scb->scb_vertex0.pad == -1);
    CHECK(scb->scb_vertex1.vx == -20.0f);
    CHECK(scb->scb_vertex2.vy == 60.0f);
    CHECK(scb->scb_vertex3.vx == -20.0f);
    CHECK(scb->scb_vertex3.vy == 60.0f);
    CHECK(scb->scb_cvertex.pad == 8);

    same = sprite_DisplaySprite(
        scb, 40, 1, 2, 3, 4, 7);
    CHECK(same == scb);
    CHECK(mSCBDraw[0].head == (Node *)scb);
    CHECK(mSCBDraw[0].tail == (Node *)scb);
    CHECK(scb->scb_vertex3.vx == 4.0f);
    CHECK(scb->scb_vertex3.vy == 6.0f);
    CHECK(scb->scb_cvertex.pad == 7);
    return 0;
}

static int test_hide_sprite(void)
{
    SCB scb;
    Sprite sprite;

    memset(&scb, 0, sizeof(scb));
    memset(&sprite, 0, sizeof(sprite));
    sprite.sp_SCB = &scb;
    scb.scb_flags = 0x80;

    sprite_gHideSprite(&sprite);
    CHECK(scb.scb_flags == 0xc0);
    sprite_gHideSprite(&sprite);
    CHECK(scb.scb_flags == 0xc0);
    sprite_gUnHideSprite(&sprite);
    CHECK(scb.scb_flags == 0x80);
    return 0;
}

static int test_free_scb(void)
{
    SCB scb;

    sprite_gFreeSCB(NULL);
    memset(&scb, 0, sizeof(scb));
    sprite_gFreeSCB(&scb);
    CHECK(scb.scb_flags == 0);
    scb.scb_flags = 0x40;
    sprite_gFreeSCB(&scb);
    CHECK(scb.scb_flags == 0x41);
    sprite_gFreeSCB(&scb);
    CHECK(scb.scb_flags == 0x41);
    return 0;
}

static int test_free_sprite(void)
{
    SCB scb;
    Sprite sprite;

    sprite_gFreeSprite(NULL);
    memset(&scb, 0, sizeof(scb));
    memset(&sprite, 0, sizeof(sprite));
    sprite.sp_SCB = &scb;

    sprite_gFreeSprite(&sprite);
    CHECK(sprite.sp_Flags == 1);
    CHECK(scb.scb_flags == 0);

    sprite.sp_Flags = 0x20;
    scb.scb_flags = 0x40;
    sprite_gFreeSprite(&sprite);
    CHECK(sprite.sp_Flags == 0x21);
    CHECK(scb.scb_flags == 0x41);

    sprite.sp_SCB = NULL;
    sprite_gFreeSprite(&sprite);
    CHECK(sprite.sp_Flags == 0x21);
    return 0;
}

static int test_render_sprite_paths(void)
{
    SpriteDrawTrace trace;
    _Material material;
    SCB scb;
    MATRIX matrix;

    memset(&trace, 0, sizeof(trace));
    memset(&material, 0, sizeof(material));
    memset(&scb, 0, sizeof(scb));
    memset(&matrix, 0, sizeof(matrix));
    material.iw = 32;
    material.ih = 16;
    scb.scb_Texture = &material;
    scb.scb_flags = 2;
    scb.scb_vertex0.vx = 30.0f;
    scb.scb_vertex0.vy = 40.0f;
    scb.scb_vertex0.pad = 0x7f;
    scb.scb_vertex1.vx = 10.0f;
    scb.scb_vertex2.vy = 20.0f;
    scb.scb_cvertex.pad = 8;
    jpb_WHookSetDrawTextureHook(capture_texture, &trace);
    _RenderSprite(&matrix, &scb);
    CHECK(trace.texture_calls == 1);
    CHECK(trace.texture == &material);
    CHECK(trace.destination.left == 10);
    CHECK(trace.destination.top == 20);
    CHECK(trace.destination.right == 30);
    CHECK(trace.destination.bottom == 40);
    CHECK(trace.source.left == 32);
    CHECK(trace.source.top == 16);
    CHECK(trace.source.right == 0);
    CHECK(trace.source.bottom == 0);
    CHECK(trace.color.r == 0xff);
    CHECK(trace.color.g == 0xff);
    CHECK(trace.color.b == 0xff);
    CHECK(trace.color.cd == 0x7f);
    CHECK(trace.depth == 0.001f);
    jpb_WHookSetDrawTextureHook(NULL, NULL);

    memset(&trace, 0, sizeof(trace));
    memset(&scb.scb_vertex0, 0, sizeof(_sfvector) * 4);
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    matrix.t[0] = 5;
    matrix.t[1] = 6;
    matrix.t[2] = 100;
    scb.scb_flags = 4;
    scb.scb_vertex0.vx = -2.0f;
    scb.scb_vertex0.vy = -3.0f;
    scb.scb_vertex1.vx = 2.0f;
    scb.scb_vertex1.vy = -3.0f;
    scb.scb_vertex2.vx = -2.0f;
    scb.scb_vertex2.vy = 3.0f;
    scb.scb_vertex3.vx = 2.0f;
    scb.scb_vertex3.vy = 3.0f;
    scb.scb_vertex0.pad = 0x80;
    scb.scb_cvertex.pad = 0;
    jpb_WHookSetScreenPolyHook(capture_polygon, &trace);
    _RenderSprite(&matrix, &scb);
    CHECK(trace.polygon_calls == 1);
    CHECK(trace.vertex_count == 4);
    CHECK(trace.no_scale == 1);
    CHECK(trace.vertices[0].x == 3.0f);
    CHECK(trace.vertices[0].y == 3.0f);
    CHECK(trace.vertices[0].z == 45.0f);
    CHECK(trace.vertices[3].x == 7.0f);
    CHECK(trace.vertices[3].y == 9.0f);
    CHECK(trace.vertices[0].tu == 0.0f);
    CHECK(trace.vertices[1].tu == 1.0f);
    CHECK(trace.vertices[2].tv == 1.0f);
    CHECK(trace.vertices[0].argb == UINT32_C(0x8030f838));
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    return 0;
}

static int test_sprite_rot_scale_and_work(void)
{
    SpriteDrawTrace trace;
    _Material material;
    Sprite *sptr;
    SCB *scb;
    MATRIX matrix;

    reset_sprite_runtime();
    memset(&trace, 0, sizeof(trace));
    memset(&material, 0, sizeof(material));
    memset(&matrix, 0, sizeof(matrix));
    material.iw = 10;
    material.ih = 6;
    sptr = sprite_gAllocSprite(8);
    CHECK(sptr != NULL);
    scb = sptr->sp_SCB;
    CHECK(scb != NULL);
    scb->scb_Texture = &material;
    sptr->sp_Pos.vx = 100.0f;
    sptr->sp_Pos.vy = 200.0f;
    sptr->sp_Pos.vz = 300.0f;
    sprite_SpriteRotScale(sptr, 0, 0, 0);
    CHECK(scb->scb_vertex0.vx == 95.0f);
    CHECK(scb->scb_vertex0.vy == 197.0f);
    CHECK(scb->scb_vertex0.vz == 300.0f);
    CHECK(scb->scb_vertex3.vx == 105.0f);
    CHECK(scb->scb_vertex3.vy == 203.0f);
    CHECK(scb->scb_vertex3.vz == 300.0f);

    sptr->sp_Func = keep_sprite_alive;
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    jpb_WHookSetScreenPolyHook(capture_polygon, &trace);
    sprite_SpriteWork(&matrix);
    CHECK(numSprite == 1);
    CHECK(numSCB == 1);
    CHECK(mCurSCBList == 1);
    CHECK(gGTEMATRIX.m[0][0] == 1.0f);
    CHECK(scb->scb_vertex0.vx == -20.0f);
    CHECK(scb->scb_vertex0.vy == -12.0f);
    CHECK(scb->scb_vertex3.vx == 20.0f);
    CHECK(scb->scb_vertex3.vy == 12.0f);
    CHECK(scb->scb_cvertex.vx == 100.0f);
    CHECK(scb->scb_cvertex.vy == 200.0f);
    CHECK(scb->scb_cvertex.vz == 300.0f);
    CHECK(trace.polygon_calls == 1);
    CHECK(trace.vertices[0].x == 80.0f);
    CHECK(trace.vertices[0].y == 188.0f);
    CHECK(trace.vertices[0].z == 245.0f);

    sprite_gFreeSprite(sptr);
    sprite_SpriteWork(&matrix);
    CHECK(numSprite == 0);
    CHECK(numSCB == 0);
    CHECK(mCurSCBList == 0);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    return 0;
}

static int test_double_buffer_selector_recovery(void)
{
    MATRIX matrix;

    reset_sprite_runtime();
    memset(&matrix, 0, sizeof(matrix));
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    mCurSCBList = 6;
    sprite_SpriteWork(&matrix);
    CHECK(jpb_sprite_list_recovery_count == 1);
    CHECK(jpb_sprite_last_invalid_selector == 6);
    CHECK(mCurSCBList == 1);
    return 0;
}

static int test_add_normal_effect(void)
{
    EffectData effect;
    _Material material;
    VECTOR loc = {100, 200, 300, 0};
    _svector inherited_velocity = {7, 8, 9, 0};
    Sprite **result;
    Sprite *sptr;
    SCB *scb;

    reset_sprite_runtime();
    memset(&effect, 0, sizeof(effect));
    memset(&material, 0, sizeof(material));
    material.iw = 10;
    material.ih = 6;
    effects1Handle[2] = &material;
    effect.bank = 1;
    effect.type = 2;
    effect.delay = 3;
    effect.vel.vx = 4;
    effect.vel.vy = 5;
    effect.vel.vz = 6;
    effect.vel.pad = 7;
    effect.acc.vx = 1;
    effect.acc.vy = 2;
    effect.acc.vz = 3;
    effect.bright.init = 100;
    effect.bright.vel = 10;
    effect.bright.acc = 2;
    effect.bright.limit = 200;
    effect.scale.init = 4096;
    effect.scale.vel = 20;
    effect.scale.acc = 3;
    effect.scale.limit = 5000;
    effect.rx = 9;
    effect.rvx = 11;
    effect.pos.vx = 2;
    effect.pos.vy = 3;
    effect.pos.vz = 4;
    effect.flags = UINT32_C(0x10);

    result = sprite_AddSpriteEffect(
        &effect, 1, &loc, &inherited_velocity);
    CHECK(result != NULL);
    CHECK(result[0] != NULL);
    sptr = result[0];
    scb = sptr->sp_SCB;
    CHECK(scb != NULL);
    CHECK(mSCBDraw[0].head == (Node *)scb);
    CHECK(mSCBDraw[0].tail == (Node *)scb);
    CHECK(scb->scb_Texture == &material);
    CHECK(sptr->sp_Pos.vx == 102.0f);
    CHECK(sptr->sp_Pos.vy == 203.0f);
    CHECK(sptr->sp_Pos.vz == 304.0f);
    CHECK(sptr->sp_Vel.vx == 11.0f);
    CHECK(sptr->sp_Vel.vy == 13.0f);
    CHECK(sptr->sp_Vel.vz == 15.0f);
    CHECK(sptr->sp_Acc.vx == 1.0f);
    CHECK(sptr->sp_Acc.vy == 2.0f);
    CHECK(sptr->sp_Acc.vz == 3.0f);
    CHECK(sptr->sp_Rot.vx == 9);
    CHECK(sptr->sp_RVel.vx == 11);
    CHECK(sptr->sp_Func == sprite_MainCallBack);
    CHECK(sptr->sp_Delay == 3);
    CHECK(sptr->sp_cBright.init == 100);
    CHECK(sptr->sp_cScale.limit == 5000);
    CHECK((uint32_t)sptr->sp_Flags == UINT32_C(0x00100000));
    CHECK((uint32_t)scb->scb_flags == UINT32_C(0x00100208));
    CHECK(scb->scb_vertex0.vx == 97.0f);
    CHECK(scb->scb_vertex0.vy == 200.0f);
    CHECK(scb->scb_vertex0.vz == 304.0f);
    CHECK(scb->scb_vertex3.vx == 107.0f);
    CHECK(scb->scb_vertex3.vy == 206.0f);
    CHECK(scb->scb_vertex3.vz == 304.0f);
    CHECK(scb->scb_cvertex.vx == 9.0f);
    CHECK(scb->scb_cvertex.vz == 7.0f);
    return 0;
}

static int test_ring_effects(void)
{
    RingData data;
    VECTOR pos = {10, 20, 30, 0};
    Ring *ring;
    Ring *cursor;
    int count;

    reset_sprite_runtime();
    memset(&data, 0, sizeof(data));
    data.rot.vx = 1;
    data.rot.vy = 2;
    data.rot.vz = 3;
    data.b.init = 4;
    data.b.acc = 5;
    data.r1.init = 6;
    data.r1.acc = 7;
    data.r2.init = 8;
    data.r2.acc = 9;
    data.h1.init = 10;
    data.h1.acc = 11;
    data.h2.init = 12;
    data.h2.acc = 13;
    data.time = 14;
    gGlobalTimer = 100;

    ring = sprite_FireRing(&data, &pos);
    CHECK(ring != NULL);
    CHECK(ring->sp_Type == -1);
    CHECK(ring->pos.vx == 10);
    CHECK(ring->pos.vy == 20);
    CHECK(ring->pos.vz == 30);
    CHECK(ring->rot.vx == 1);
    CHECK(ring->rot.vy == 2);
    CHECK(ring->rot.vz == 3);
    CHECK(ring->rot.pad == 0);
    CHECK(ring->b1 == 4);
    CHECK(ring->b1v == 5);
    CHECK(ring->rad1 == 6);
    CHECK(ring->rad2 == 8);
    CHECK(ring->h1 == 10);
    CHECK(ring->h2 == 12);
    CHECK(ring->time == 114);
    CHECK(ring->pRingData == &data);

    reset_sprite_runtime();
    memset(&data, 0, sizeof(data));
    data.rot.pad = 0x40;
    data.r1.init = 64;
    ring = sprite_FireRing(&data, &pos);
    CHECK(ring != NULL);
    CHECK(ring->rad1 == 0);
    CHECK(ring->rad2 == 1);
    CHECK(ring->h1 == 16);
    CHECK(ring->h2 == 15);
    count = 0;
    cursor = ring;
    while (cursor != NULL) {
        ++count;
        if (cursor->sp_Next == NULL) {
            CHECK(cursor->h2 == 0);
        }
        cursor = (Ring *)cursor->sp_Next;
    }
    CHECK(count == 16);
    return 0;
}

static int test_add_effect_at_node(void)
{
    EffectData effect;
    _Material material;
    Mnode node;
    Sprite **result;

    reset_sprite_runtime();
    coll_ResetCollisionSystem();
    memset(&effect, 0, sizeof(effect));
    memset(&material, 0, sizeof(material));
    memset(&node, 0, sizeof(node));
    node.id = (modelNodeId)(NODE_DYNAMIC | 6);
    node.v3RotCenter.vx = 30;
    node.v3RotCenter.vy = 40;
    node.v3RotCenter.vz = 50;
    node.v3Velocity.vx = 3;
    node.v3Velocity.vy = 4;
    node.v3Velocity.vz = 5;
    coll_gRegisterNode(2, &node);
    effects1Handle[0] = &material;
    effect.bank = 1;
    effect.type = 0;
    effect.vel.vx = 7;
    effect.vel.vy = 8;
    effect.vel.vz = 9;

    result = sprite_AddSpriteEffectAtNode(
        &effect, 1, 2, 6);
    CHECK(result != NULL);
    CHECK(result[0] != NULL);
    CHECK(result[0]->sp_Pos.vx == 30.0f);
    CHECK(result[0]->sp_Pos.vy == 40.0f);
    CHECK(result[0]->sp_Pos.vz == 50.0f);
    CHECK(result[0]->sp_Vel.vx == 10.0f);
    CHECK(result[0]->sp_Vel.vy == 12.0f);
    CHECK(result[0]->sp_Vel.vz == 14.0f);
    return 0;
}

static int test_main_callback_motion(void)
{
    Sprite *sptr;
    SCB *scb;
    int result;

    reset_sprite_runtime();
    sptr = sprite_gAllocSprite(8);
    CHECK(sptr != NULL);
    scb = sptr->sp_SCB;
    CHECK(scb != NULL);
    sptr->sp_Pos.vx = 10.0f;
    sptr->sp_Pos.vy = 20.0f;
    sptr->sp_Pos.vz = 30.0f;
    scb->scb_vertex0.vx = 10.0f;
    scb->scb_vertex0.vy = 20.0f;
    scb->scb_vertex0.vz = 30.0f;
    sptr->sp_Vel.vx = 4.0f;
    sptr->sp_Vel.vy = 6.0f;
    sptr->sp_Vel.vz = 8.0f;
    sptr->sp_Acc.vx = 2.0f;
    sptr->sp_Acc.vy = 2.0f;
    sptr->sp_Acc.vz = 2.0f;
    sptr->sp_RVel.vx = 3;
    sptr->sp_Delay = 0;
    sptr->sp_cScale.init = 100;
    sptr->sp_cScale.vel = 10;
    sptr->sp_cScale.acc = 2;
    sptr->sp_cBright.init = 80;
    sptr->sp_cBright.vel = -10;
    sptr->sp_cBright.limit = 100;

    result = sprite_MainCallBack((int32_t *)sptr);
    CHECK(result == 100);
    CHECK(sptr->sp_Time == 1);
    CHECK(sptr->sp_Rot.vx == 3);
    CHECK(sptr->sp_Vel.vx == 6.0f);
    CHECK(sptr->sp_Vel.vy == 8.0f);
    CHECK(sptr->sp_Vel.vz == 10.0f);
    CHECK(sptr->sp_Pos.vx == 13.0f);
    CHECK(sptr->sp_Pos.vy == 24.0f);
    CHECK(sptr->sp_Pos.vz == 35.0f);
    CHECK(sptr->sp_cScale.init == 110);
    CHECK(sptr->sp_cScale.vel == 12);
    CHECK(sptr->sp_cBright.init == 70);
    CHECK(scb->scb_vertex0.pad == 80);
    CHECK((sptr->sp_Flags & 1) == 0);
    return 0;
}

static int test_add_callback_recursion(void)
{
    EffectHeader child;
    _Material material;
    Sprite *launcher;
    SCB *spawned_scb;

    reset_sprite_runtime();
    memset(&child, 0, sizeof(child));
    memset(&material, 0, sizeof(material));
    child.num = 1;
    child.aEffects[0].bank = 1;
    child.aEffects[0].type = 0;
    child.aEffects[0].pos.vx = 5;
    effects1Handle[0] = &material;
    paEffects[7] = &child;

    launcher = sprite_gAllocSprite(0);
    CHECK(launcher != NULL);
    launcher->sp_Type = 7;
    launcher->sp_Pos.vx = 100.0f;
    launcher->sp_Pos.vy = 200.0f;
    launcher->sp_Pos.vz = 300.0f;
    launcher->sp_Vel.vx = 10.0f;
    launcher->sp_Delay = 1;
    launcher->sp_cBright.limit = 0;

    CHECK(sprite_AddCallBack((int32_t *)launcher) == 0);
    CHECK((launcher->sp_Flags & 1) != 0);
    CHECK(mSCBDraw[0].head != NULL);
    spawned_scb = (SCB *)mSCBDraw[0].head;
    CHECK(spawned_scb->scb_Texture == &material);
    CHECK(spawned_scb->scb_vertex0.vx == 115.0f);
    CHECK(spawned_scb->scb_vertex0.vy == 200.0f);
    CHECK(spawned_scb->scb_vertex0.vz == 300.0f);
    return 0;
}

static int test_comments_sprite(void)
{
    char text[] = "JEDI POWER BATTLE ON!";
    VECTOR pos = {100, 200, 300, 0};
    _svector vel = {1, 2, 3, 0};
    Sprite *sptr;
    int random_x;
    int random_z;

    reset_sprite_runtime();
    gGlobalTimer = UINT32_C(0x1000);
    srand(7);
    random_x = rand() % 16 - 8;
    random_z = rand() % 16 - 8;
    srand(7);
    sptr = sprite_GetCommentsSprite(
        text,
        &pos,
        &vel,
        UINT32_C(0x7fffffff));
    CHECK(sptr != NULL);
    CHECK(sptr->sp_Pos.vx ==
          (float)(pos.vx + random_x));
    CHECK(sptr->sp_Pos.vy == 216.0f);
    CHECK(sptr->sp_Pos.vz ==
          (float)(pos.vz + random_z));
    CHECK(sptr->sp_Vel.vx == 1.0f);
    CHECK(sptr->sp_Vel.vy == 2.0f);
    CHECK(sptr->sp_Vel.vz == 3.0f);
    CHECK((char *)(void *)sptr->sp_Anim == text);
    CHECK((uint32_t)(uintptr_t)sptr->sp_PAnim ==
          UINT32_C(0x7fffffff));
    CHECK((uint32_t)(uintptr_t)sptr->sp_User ==
          UINT32_C(0x3000));

    draw_text_calls = 0;
    sprite_CommentsCallBack((int32_t *)(void *)sptr);
    CHECK(draw_text_calls == 1);
    CHECK(draw_text_x ==
          (float)(pos.vx + random_x));
    CHECK(draw_text_y == 216.0f);
    CHECK(draw_text_z ==
          (float)(pos.vz + random_z));
    CHECK(draw_text_scale == 1.5f);
    CHECK(draw_text_color ==
          UINT32_C(0x7fffffff));
    CHECK(strcmp(draw_text_text, text) == 0);
    CHECK((sptr->sp_Flags & 1) == 0);

    gGlobalTimer = UINT32_C(0x3001);
    sprite_CommentsCallBack((int32_t *)(void *)sptr);
    CHECK((sptr->sp_Flags & 1) != 0);
    return 0;
}

static int test_points_sprite(void)
{
    VECTOR pos = {100, 200, 300, 0};
    _svector vel = {1, 2, 3, 0};
    Sprite *sptr;
    int random_x;
    int random_z;

    reset_sprite_runtime();
    gGlobalTimer = UINT32_C(0x1000);
    srand(11);
    random_x = rand() % 8 - 4;
    random_z = rand() % 8 - 4;
    srand(11);
    sptr = sprite_GetPointsSprite(
        125,
        &pos,
        &vel,
        UINT32_C(0x00112233),
        0);
    CHECK(sptr != NULL);
    CHECK(sptr->sp_Num == 125);
    CHECK(sptr->sp_Pos.vx ==
          (float)(pos.vx + random_x));
    CHECK(sptr->sp_Pos.vy == 208.0f);
    CHECK(sptr->sp_Pos.vz ==
          (float)(pos.vz + random_z));
    CHECK(sptr->sp_Vel.vx == 1.0f);
    CHECK(sptr->sp_Vel.vy == 2.0f);
    CHECK(sptr->sp_Vel.vz == 3.0f);
    CHECK((uint32_t)(uintptr_t)sptr->sp_PAnim ==
          UINT32_C(0x00112233));
    CHECK((uint32_t)(uintptr_t)sptr->sp_User ==
          UINT32_C(0x9000));

    draw_text_calls = 0;
    draw_text_text[0] = '\0';
    sprite_PointsCallBack((int32_t *)(void *)sptr);
    CHECK(draw_text_calls == 1);
    CHECK(strcmp(draw_text_text, "+125") == 0);
    CHECK(draw_text_scale == 1.5f);
    CHECK(draw_text_color == UINT32_C(0xff112233));
    CHECK(sptr->sp_Pos.vx ==
          (float)(pos.vx + random_x + 1));
    CHECK(sptr->sp_Pos.vy == 210.0f);
    CHECK(sptr->sp_Pos.vz ==
          (float)(pos.vz + random_z + 3));

    gGlobalTimer = UINT32_C(0x9000);
    sprite_PointsCallBack((int32_t *)(void *)sptr);
    CHECK((sptr->sp_Flags & 1) != 0);
    return 0;
}

int main(void)
{
    if (test_hide_scb() != 0) {
        return 1;
    }
    if (test_display_sprite() != 0) {
        return 1;
    }
    if (test_hide_sprite() != 0) {
        return 1;
    }
    if (test_free_scb() != 0) {
        return 1;
    }
    if (test_free_sprite() != 0) {
        return 1;
    }
    if (test_render_sprite_paths() != 0) {
        return 1;
    }
    if (test_sprite_rot_scale_and_work() != 0) {
        return 1;
    }
    if (test_double_buffer_selector_recovery() != 0) {
        return 1;
    }
    if (test_add_normal_effect() != 0) {
        return 1;
    }
    if (test_ring_effects() != 0) {
        return 1;
    }
    if (test_add_effect_at_node() != 0) {
        return 1;
    }
    if (test_main_callback_motion() != 0) {
        return 1;
    }
    if (test_add_callback_recursion() != 0) {
        return 1;
    }
    if (test_comments_sprite() != 0) {
        return 1;
    }
    if (test_points_sprite() != 0) {
        return 1;
    }

    puts("sprite tests passed");
    return 0;
}
