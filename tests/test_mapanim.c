#include "jpb/mapanim.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

extern int gGlobalFrameRate;
extern WorldData *gpWorld;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",              \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static WorldData world;
static wsl_BT_ANIMDEF definition;

static void setup_definition(int entries)
{
    memset(&world, 0, sizeof(world));
    memset(&definition, 0, sizeof(definition));
    definition.numFrames = 0x4000;
    definition.numNodes = 1;
    definition.aNodes[0].num = 0;
    definition.aNodes[0].numEntries = entries;
    definition.aNodes[0].iParent = -1;
    definition.aNodes[0].iChild = -1;
    definition.aNodes[0].iSibling = -1;
    world.animDef[0] = &definition;
    gpWorld = &world;
}

static void setup_animation(wsl_BT_ANIMMAP *animation)
{
    memset(animation, 0, sizeof(*animation));
    animation->defNum = 0;
    animation->numNodes = 1;
    animation->numFrames = 0x2000;
    animation->fps = 0x1000;
    animation->on = 1;
    animation->aNodes[0].pivot.vx = 10;
    animation->aNodes[0].pivot.vy = 20;
    animation->aNodes[0].pivot.vz = 30;
}

static int check_interpolation_and_matrix(void)
{
    wsl_BT_ANIMMAP animation;
    MATRIX *matrix;

    setup_definition(2);
    definition.aNodes[0].aEntry[0].frame = 0;
    definition.aNodes[0].aEntry[0].xyz.vx = 100;
    definition.aNodes[0].aEntry[0].xyz.vy = 200;
    definition.aNodes[0].aEntry[0].xyz.vz = 300;
    definition.aNodes[0].aEntry[1].frame = 0x4000;
    definition.aNodes[0].aEntry[1].xyz.vx = 200;
    definition.aNodes[0].aEntry[1].xyz.vy = 400;
    definition.aNodes[0].aEntry[1].xyz.vz = 700;

    setup_animation(&animation);
    animation.type = 0;
    animation.fps = 0;
    animation.currFrame = 0x2000;
    gGlobalFrameRate = 0x800;
    animsPaused = 0;
    manim_UpdateMapAnim(&animation);

    matrix = &animation.aNodes[0].mat.matrix;
    CHECK(animation.aNodes[0].currFrame == 0x2000);
    CHECK(fabsf(matrix->m[0][0] - 4096.0f) < 0.01f);
    CHECK(fabsf(matrix->m[1][1] - 4096.0f) < 0.01f);
    CHECK(fabsf(matrix->m[2][2] - 4096.0f) < 0.01f);
    CHECK(matrix->t[0] == 160);
    CHECK(matrix->t[1] == 320);
    CHECK(matrix->t[2] == 786);
    return 0;
}

static int check_update_modes(void)
{
    wsl_BT_ANIMMAP animation;

    setup_definition(1);
    gGlobalFrameRate = 0x800;
    animsPaused = 0;

    setup_animation(&animation);
    animation.type = 0;
    animation.currFrame = 0x1c00;
    manim_UpdateMapAnim(&animation);
    CHECK(animation.currFrame == 0x400);
    CHECK(animation.kDTime == 0x800);

    setup_animation(&animation);
    animation.type = 1;
    animation.state = 0;
    animation.delayTime = 0x400;
    manim_UpdateMapAnim(&animation);
    CHECK(animation.state == 2 && animation.timer1 == 0x800);

    setup_animation(&animation);
    animation.type = 2;
    animation.state = 3;
    animation.currFrame = 0x1000;
    manim_UpdateMapAnim(&animation);
    CHECK(animation.currFrame == 0x800 && animation.state == 3);

    setup_animation(&animation);
    animation.type = 3;
    animation.state = 1;
    animation.currFrame = 0x1c00;
    manim_UpdateMapAnim(&animation);
    CHECK(animation.currFrame == 0x2000 && animation.state == 2);

    setup_animation(&animation);
    animation.type = 4;
    animation.state = 1;
    animation.currFrame = 0x1c00;
    manim_UpdateMapAnim(&animation);
    CHECK(animation.currFrame == 0x1c00 && animation.state == 2);

    setup_animation(&animation);
    animation.type = 5;
    animation.state = 1;
    animation.currFrame = 0x1c00;
    manim_UpdateMapAnim(&animation);
    CHECK(animation.currFrame == 0x2000 && animation.state == 2);
    CHECK(animation.on == 0);

    setup_animation(&animation);
    animation.type = 6;
    animation.state = 4;
    animation.currFrame = 0x400;
    manim_UpdateMapAnim(&animation);
    CHECK(animation.currFrame == 0 && animation.state == 5);
    CHECK(animation.timer1 == 0);

    setup_animation(&animation);
    animation.type = 7;
    animation.currFrame = 0x1800;
    manim_UpdateMapAnim(&animation);
    CHECK(animation.currFrame == 0);

    animsPaused = 1;
    animation.currFrame = 123;
    animation.kDTime = 456;
    manim_UpdateMapAnim(&animation);
    CHECK(animation.currFrame == 123 && animation.kDTime == 456);
    animsPaused = 0;
    return 0;
}

int main(void)
{
    CHECK(check_interpolation_and_matrix() == 0);
    CHECK(check_update_modes() == 0);
    CHECK(manim_InitAnim(99) == 1);
    bapmanim_Activate(3);
    bapmanim_DeActivate(3);
    manim_HandleMapAnims();
    puts("Map animation tests passed");
    return 0;
}
