#include "jpb/camera.h"
#include "jpb/cube.h"
#include "jpb/game.h"
#include "jpb/jonny.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/projection.h"
#include "jpb/world.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr, "CHECK failed at %s:%d: %s\n",                       \
                __FILE__, __LINE__, #condition);                             \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int test_lerp(void)
{
    CHECK(camLerp(10, 30, 0.0) == 10);
    CHECK(camLerp(10, 30, 0.5) == 20);
    CHECK(camLerp(10, 30, 1.0) == 30);
    CHECK(camLerp(30, 10, 0.25) == 25);
    CHECK(camLerp(-10, 11, 0.5) == 0);
    CHECK(camLerp(10, 30, -0.5) == 0);
    CHECK(camLerp(10, 30, 2.0) == 50);
    CHECK(camLerp(0, 1, NAN) == 0);
    CHECK(camLerp(0, INT16_MAX, 2.0) == -2);
    CHECK(lerp(10.0, 30.0, 0.0) == 10.0);
    CHECK(lerp(10.0, 30.0, 0.5) == 20.0);
    CHECK(lerp(10.0, 30.0, 1.0) == 30.0);
    CHECK(map(5, 0, 10, 100, 200) == 150);
    CHECK(map(-5, 0, 10, 100, 200) == 50);
    CHECK(mapClamped(-5, 0, 10, 100, 200) == 100);
    CHECK(mapClamped(15, 0, 10, 100, 200) == 200);
    CHECK(mapClamped(-5, 0, 10, 200, 100) == 200);
    CHECK(mapClamped(15, 0, 10, 200, 100) == 100);
    CHECK(mapClamped(5, 0, 10, 123, 123) == 123);
    CHECK(mapDouble(5.0, 0.0, 10.0, 100.0, 200.0) == 150.0);
    return 0;
}

static int test_get_camera(void)
{
    VECTOR angle = {0, 0, 0, 1234};
    _svector focus = {0, 0, 0, 123};

    memset(&gCamera, 0, sizeof(gCamera));
    gCamera.angle.vx = 101;
    gCamera.angle.vy = -202;
    gCamera.angle.vz = 303;
    gCamera.angle.pad = 404;
    gCamera.focus.vx = 11;
    gCamera.focus.vy = -22;
    gCamera.focus.vz = 33;
    gCamera.focus.pad = 44;

    CHECK(camera_GetCamera(&angle, &focus) == &gCamera);
    CHECK(angle.vx == 101);
    CHECK(angle.vy == -202);
    CHECK(angle.vz == 303);
    CHECK(angle.pad == 1234);
    CHECK(focus.vx == 11);
    CHECK(focus.vy == -22);
    CHECK(focus.vz == 33);
    CHECK(focus.pad == 123);
    return 0;
}

static int test_location(void)
{
    VECTOR location = {901, 902, 903, 904};

    memset(&gCamera, 0, sizeof(gCamera));
    camera_GetLocation(&location);
    CHECK(location.vx == 901);
    CHECK(location.vy == 902);
    CHECK(location.vz == 903);
    CHECK(location.pad == 904);

    gCamera.viewType = JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gCamera.focus.vx = -101;
    gCamera.focus.vy = 202;
    gCamera.focus.vz = -303;
    camera_GetLocation(&location);
    CHECK(location.vx == -101);
    CHECK(location.vy == 202);
    CHECK(location.vz == -303);
    CHECK(location.pad == 904);

    gCamera.viewType =
        JPB_CAMERA_VIEW_RELATIVE_FOCUS | JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gCamera.campos.vx = 4;
    gCamera.campos.vy = -5;
    gCamera.campos.vz = 6;
    camera_GetLocation(&location);
    CHECK(location.vx == -105);
    CHECK(location.vy == 207);
    CHECK(location.vz == -309);
    CHECK(location.pad == 904);
    return 0;
}

static int test_state_and_snap(void)
{
    Camera destination;

    memset(&gCamera, 0, sizeof(gCamera));
    memset(&destination, 0x5a, sizeof(destination));

    camera_SetViewType(0x12345678);
    CHECK(camera_GetViewType() == 0x12345678);
    camera_SetShake(-77);
    CHECK(screenshakeamplitude == -77);
    CHECK(screenshake == 0x100);

    gCamera.angleDest.vx = 1001;
    gCamera.angleDest.vy = -1002;
    gCamera.angleDest.vz = 1003;
    gCamera.angleDest.pad = 1004;
    gCamera.focusDest.vx = -201;
    gCamera.focusDest.vy = 202;
    gCamera.focusDest.vz = -203;
    gCamera.focusDest.pad = 204;

    camera_SnapCamera(&destination);
    CHECK(destination.angle.vx == 1001);
    CHECK(destination.angle.vy == -1002);
    CHECK(destination.angle.vz == 1003);
    CHECK(destination.angleDest.vx == 1001);
    CHECK(destination.angleDest.vy == -1002);
    CHECK(destination.angleDest.vz == 1003);
    CHECK(destination.focus.vx == -201);
    CHECK(destination.focus.vy == 202);
    CHECK(destination.focus.vz == -203);
    CHECK(destination.focusDest.vx == -201);
    CHECK(destination.focusDest.vy == 202);
    CHECK(destination.focusDest.vz == -203);

    /* The reference stores only xyz and leaves both padding words intact. */
    CHECK(destination.angle.pad == INT32_C(0x5a5a5a5a));
    CHECK(destination.angleDest.pad == INT32_C(0x5a5a5a5a));
    CHECK(destination.focus.pad == INT16_C(0x5a5a));
    CHECK(destination.focusDest.pad == INT16_C(0x5a5a));

    camera_SnapCamera(&gCamera);
    CHECK(gCamera.angle.vx == gCamera.angleDest.vx);
    CHECK(gCamera.angle.vy == gCamera.angleDest.vy);
    CHECK(gCamera.angle.vz == gCamera.angleDest.vz);
    CHECK(gCamera.focus.vx == gCamera.focusDest.vx);
    CHECK(gCamera.focus.vy == gCamera.focusDest.vy);
    CHECK(gCamera.focus.vz == gCamera.focusDest.vz);
    return 0;
}

static int test_scroll_camera(void)
{
    WorldData world;
    WorldData *saved_world = gpWorld;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    world.currentDolly = 7;
    world.aDolly[7].offx = 10;
    world.aDolly[7].offz = -20;
    camera_ScrollCamera(0, 0);
    CHECK(world.aDolly[7].flags == 0);
    CHECK(world.aDolly[7].offx == 10);
    CHECK(world.aDolly[7].offz == -20);
    camera_ScrollCamera(5, -7);
    CHECK(
        world.aDolly[7].flags ==
        (UINT32_C(0x8) | UINT32_C(0x10)));
    CHECK(world.aDolly[7].offx == 15);
    CHECK(world.aDolly[7].offz == -27);
    world.aDolly[7].offx = INT16_MAX;
    camera_ScrollCamera(1, 0);
    CHECK(world.aDolly[7].offx == INT16_MIN);
    gpWorld = saved_world;
    return 0;
}

typedef struct CameraGameplayFixture {
    WorldData world;
    playerObject players[2];
    sceneObject scenes[2];
    physicsObject physics[2];
    int32_t map_storage[6];
} CameraGameplayFixture;

static void camera_gameplay_fixture_init(
    CameraGameplayFixture *fixture)
{
    int index;

    memset(fixture, 0, sizeof(*fixture));
    fixture->world.player0 = &fixture->players[0];
    fixture->world.player1 = &fixture->players[1];
    fixture->world.currentDolly = 1;
    fixture->world.overRideDolly = 1;
    for (index = 0; index < 2; ++index) {
        fixture->players[index].playerRoot.objectID = index;
        fixture->players[index].playerRoot.pParent =
            &fixture->scenes[index].sceneRoot;
        fixture->scenes[index].pScene =
            &fixture->scenes[index].sceneRoot;
        fixture->scenes[index].pPhysics =
            &fixture->physics[index].physicsRoot;
        fixture->scenes[index].pPlayer =
            &fixture->players[index].playerRoot;
        fixture->physics[index].physicsRoot.objectID = index;
        fixture->physics[index].physicsRoot.pParent =
            &fixture->scenes[index].sceneRoot;
    }
    fixture->physics[0].vpos.vx = 1000;
    fixture->physics[0].vpos.vy = 2000;
    fixture->physics[0].vpos.vz = 3000;
    fixture->physics[0].pos.vx = 1000.0f;
    fixture->physics[0].pos.vy = 2000.0f;
    fixture->physics[0].pos.vz = 3000.0f;
    fixture->physics[0].airGround = 1800.0f;
    fixture->physics[0].validairground = 1800.0f;
    fixture->physics[1].vpos.vx = -1000;
    fixture->physics[1].vpos.vy = 2200;
    fixture->physics[1].vpos.vz = -3000;
    fixture->physics[1].pos.vx = -1000.0f;
    fixture->physics[1].pos.vy = 2200.0f;
    fixture->physics[1].pos.vz = -3000.0f;
    fixture->physics[1].airGround = 1900.0f;
    fixture->physics[1].validairground = 1900.0f;
    fixture->map_storage[0] = 0;
    fixture->map_storage[1] = 0;
}

static int test_gameplay_camera_owner(void)
{
    CameraGameplayFixture fixture;
    WorldData *saved_world = gpWorld;
    int32_t *saved_leveldata = leveldata;
    gamestruct saved_game = GameStruct;
    char saved_level = LevelSelect;
    BAP_CAMERADOLLY *dolly;

    camera_gameplay_fixture_init(&fixture);
    gpWorld = &fixture.world;
    leveldata = &fixture.map_storage[2];
    LevelSelect = 1;
    memset(&gCamera, 0, sizeof(gCamera));
    gCamera.viewType = JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gGlobalFrameRate = 0;
    GameStruct.NumPlayers = 1;
    dolly = &fixture.world.aDolly[1];
    dolly->pitch = 4;
    dolly->yaw = 0x123;
    dolly->offset.vx = 100;
    dolly->offset.vy = 200;
    dolly->offset.vz = 300;

    camera_SetCurrentCameraType(1);
    CHECK(camera_SetCameraPos(1) == 1);
    CHECK(fixture.world.location.vx == 1000);
    CHECK(fixture.world.location.vy == 2000);
    CHECK(fixture.world.location.vz == 3000);
    CHECK(gCamera.angle.vx == 0x20);
    CHECK(gCamera.angle.vy == 0x123);
    CHECK(gCamera.angle.vz == 0);
    CHECK(gCamera.focus.vx == 1100);
    CHECK(gCamera.focus.vy == 2200);
    CHECK(gCamera.focus.vz == 3300);
    CHECK((gCamera.viewType & 0x1000) != 0);
    CHECK(cameraYaw == 0x123);
    CHECK(cameraFacing.vx == -rsin(0x123));
    CHECK(cameraFacing.vy == 0);
    CHECK(cameraFacing.vz == -rcos(0x123));

    dolly->flags =
        UINT32_C(0x8) |
        UINT32_C(0x10) |
        UINT32_C(0x1000);
    dolly->slackx = 10;
    dolly->slacky = 20;
    dolly->slackz = 30;
    dolly->offx = 50;
    dolly->offy = -50;
    dolly->offz = 100;
    CHECK(camera_SetCameraPos(1) == 1);
    CHECK(gCamera.focusDest.vx == 110);
    CHECK(gCamera.focusDest.vy == 220);
    CHECK(gCamera.focusDest.vz == 330);

    /*
     * camera_StuffCamera samples and normalizes the prior lead before it
     * advances the vector. With yaw zero, sustained +Z movement therefore
     * feeds the second frame's forward lead back into its acceleration.
     */
    memset(dolly, 0, sizeof(*dolly));
    fixture.physics[0].mov.vz = 100.0f;
    gGlobalFrameRate = 0x800;
    CHECK(camera_SetCameraPos(1) == 1);
    CHECK(gCamera.focusDest.vx == 1000);
    CHECK(gCamera.focusDest.vy == 2000);
    CHECK(gCamera.focusDest.vz == 3216);
    CHECK(camera_SetCameraPos(1) == 1);
    CHECK(gCamera.focusDest.vx == 1000);
    CHECK(gCamera.focusDest.vy == 2000);
    CHECK(gCamera.focusDest.vz == 3612);

    fixture.world.aBkDolly[3].flags = UINT32_C(0xaabbccdd);
    fixture.world.aDolly[3].flags = 0;
    uberXRange = 22;
    uberZRange = 33;
    uberLock = 1;
    fixture.players[1].playerRoot.objectID = -1;
    GameStruct.NumPlayers = 2;
    camera_RestoreCameras();
    CHECK(fixture.world.aDolly[3].flags == UINT32_C(0xaabbccdd));
    CHECK(uberXRange == 0);
    CHECK(uberZRange == 0);
    CHECK(uberLock == 0);
    CHECK(fixture.world.overRideDolly == 0);
    CHECK(newcameraflag == 1);
    CHECK(camera_GetCurrentCameraType() == 1);

    gpWorld = saved_world;
    leveldata = saved_leveldata;
    GameStruct = saved_game;
    LevelSelect = saved_level;
    gGlobalFrameRate = 0;
    return 0;
}

static int test_camera_console_and_focus(void)
{
    WorldData world;
    WorldData *saved_world = gpWorld;
    int32_t saved_totalframes = totalframes;
    char *shake_arguments[2] = {"ShAkE", "7"};
    char *set_arguments[2] = {"SET", "12"};
    int integer_arguments[2] = {0, 7};
    VECTOR focus = {11, 22, 33, 0};

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    screenshake = 0;
    CHECK(console_CamerasCommand(
              2,
              shake_arguments,
              integer_arguments,
              NULL) == 0);
    CHECK(screenshake == 0x100);
    CHECK(screenshakeamplitude == 7);
    integer_arguments[1] = 12;
    CHECK(console_CamerasCommand(
              2,
              set_arguments,
              integer_arguments,
              NULL) == 0);
    CHECK(world.overRideDolly == 12);

    totalframes = 100;
    camera_SetFocusedCameraFocus(4, &focus, 2);
    CHECK(camera_GetCurrentCameraType() == 4);
    CHECK(gCamera.cameraTimer == 1124U);
    camera_SetCameras();
    CHECK(gCamera.focusDest.vx == 11);
    CHECK(gCamera.focusDest.vy == 22);
    CHECK(gCamera.focusDest.vz == 33);
    CHECK(screenshake == 250);

    gpWorld = saved_world;
    totalframes = saved_totalframes;
    screenshake = 0;
    return 0;
}

static int test_camera_to_view(void)
{
    sceneGeometryEnv environment;

    memset(&gCamera, 0, sizeof(gCamera));
    memset(&environment, 0x4b, sizeof(environment));
    gCamera.viewType = JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gCamera.angle.vx = 0x1001;
    gCamera.angle.vy = -2;
    gCamera.angle.vz = 0x18001;
    gCamera.focus.vx = 100;
    gCamera.focus.vy = -200;
    gCamera.focus.vz = 300;
    gCamera.angleDest.vx = 501;
    gCamera.angleDest.vy = -502;
    gCamera.focusDest.vx = -50;
    gCamera.focusDest.vy = 60;
    gCamera.focusDest.vz = -70;

    camera_Camera2ViewVector(&gCamera, &environment);
    CHECK(gCamera.angle.vx == 0);
    CHECK(gCamera.angle.vz == 1);
    CHECK(environment.angle.vx == 0);
    CHECK(environment.angle.vy == -2);
    CHECK(environment.angle.vz == 0);
    CHECK(environment.pos.vx == -100);
    CHECK(environment.pos.vy == 200);
    CHECK(environment.pos.vz == -300);
    CHECK(environment.angleDest.vx == 501);
    CHECK(environment.angleDest.vy == -502);
    CHECK(environment.angleDest.vz == 0);
    CHECK(environment.posDest.vx == 50);
    CHECK(environment.posDest.vy == -60);
    CHECK(environment.posDest.vz == 70);
    CHECK(environment.pos.pad == INT16_C(0x4b4b));
    CHECK(environment.angle.pad == INT16_C(0x4b4b));

    memset(&environment, 0, sizeof(environment));
    gCamera.viewType = JPB_CAMERA_VIEW_RELATIVE_FOCUS;
    gCamera.campos.vx = 10;
    gCamera.campos.vy = 20;
    gCamera.campos.vz = 30;
    gCamera.angle.vz = 40;
    gCamera.focus.vx = 1;
    gCamera.focus.vy = 2;
    gCamera.focus.vz = 3;
    camera_Camera2ViewVector(&gCamera, &environment);
    CHECK(environment.pos.vx == 9);
    CHECK(environment.pos.vy == 18);
    CHECK(environment.pos.vz == 27);
    CHECK(environment.angle.vx == ratan2(40, 21) + 0x400);
    CHECK(environment.angle.vy == ratan2(-10, 31) - 0x800);
    CHECK(environment.angle.vz == 0);
    return 0;
}

static int test_camera_slide(void)
{
    sceneGeometryEnv environment;

    memset(&gCamera, 0, sizeof(gCamera));
    memset(&environment, 0, sizeof(environment));
    gGlobalFrameRate = 4096;
    gCamera.viewType = 0x0100 | JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gCamera.angleDest.vx = 16;
    gCamera.angleDest.vy = 0x0ff0;
    gCamera.angleDest.vz = 160;
    gCamera.focusDest.vx = 160;
    gCamera.focusDest.vy = -160;
    gCamera.focusDest.vz = 320;

    camera_Camera2ViewVector(&gCamera, &environment);
    CHECK(gCamera.angle.vx == 8);
    CHECK(gCamera.angle.vy == 0x0ff8);
    CHECK(gCamera.angle.vz == 10);
    CHECK(gCamera.focus.vx == 10);
    CHECK(gCamera.focus.vy == -10);
    CHECK(gCamera.focus.vz == 20);

    gCamera.viewType =
        0x0100 | 0x2000 | JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gCamera.focus.vx = 0;
    gCamera.focus.vy = 0;
    gCamera.focus.vz = 0;
    gCamera.focusDest.vx = 64;
    gCamera.focusDest.vy = -64;
    gCamera.focusDest.vz = 32;
    camera_Camera2ViewVector(&gCamera, &environment);
    CHECK(gCamera.focus.vx == 32);
    CHECK(gCamera.focus.vy == -32);
    CHECK(gCamera.focus.vz == 0);

    /*
     * At the executable's initialized half-rate cadence, the required
     * even-pitch mask precedes CameraSlide. A one-unit slide therefore
     * returns to the same odd result on the following call; no caller in
     * the matched frame path carries that odd value past the next mask.
     */
    memset(&gCamera, 0, sizeof(gCamera));
    gGlobalFrameRate = 0x800;
    gCamera.viewType =
        0x0100 | JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gCamera.angle.vx = 184;
    gCamera.angleDest.vx = 246;
    camera_Camera2ViewVector(&gCamera, &environment);
    CHECK(gCamera.angle.vx == 185);
    camera_Camera2ViewVector(&gCamera, &environment);
    CHECK(gCamera.angle.vx == 185);

    gGlobalFrameRate = 0;
    return 0;
}

static int matrix_equal(const MATRIX *left, const MATRIX *right)
{
    return memcmp(left, right, sizeof(*left)) == 0;
}

static int test_scene_matrix_and_projection(void)
{
    Camera camera;
    sceneGeometryEnv environment;
    FVECTOR world = {0.0f, 0.0f, -10.0f};
    FVECTOR screen;
    MATRIX *result;

    memset(&camera, 0, sizeof(camera));
    memset(&environment, 0, sizeof(environment));
    camera.viewType = JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    result = scene_UpdateWorld2ScreenMatrix(&environment);
    CHECK(result == &environment.matrix);
    CHECK(matrix_equal(
        &environment.matrix, &environment.matrixRaw));
    CHECK(matrix_equal(&environment.matrix, &gGTEMATRIX));
    CHECK(v3Translate.vx == 0);
    CHECK(v3Translate.vy == 0);
    CHECK(v3Translate.vz == 0);

    gSceneGeometryEnv = environment;
    CHECK(scene_GetSceneMatrix() == &gSceneGeometryEnv.matrix);
    CHECK(scene_GetRawSceneMatrix() == &gSceneGeometryEnv.matrixRaw);
    CHECK(scene_GetViewPos() == &gSceneGeometryEnv.pos);

    CHECK(jpb_ProjectCameraToViewport(
              &camera,
              &environment,
              &world,
              640.0f,
              480.0f,
              &screen) == 0);
    CHECK(screen.vx == 320.0f);
    CHECK(fabsf(screen.vy - 240.0f) < 0.01f);
    CHECK(fabsf(screen.vz - 10.0f / 10240.0f) < 0.000001f);
    CHECK(jpb_ProjectCameraToViewport(
              NULL,
              &environment,
              &world,
              640.0f,
              480.0f,
              &screen) == JPB_PROJECTION_DEGENERATE_CAMERA);
    return 0;
}

static int test_twatted_camera_matrix(void)
{
    MATRIX source = {
        {
            {1.0f, 2.0f, 3.0f},
            {4.0f, 5.0f, 6.0f},
            {7.0f, 8.0f, 9.0f}
        },
        {11, 22, 33}
    };
    MATRIX original = source;
    MATRIX result;
    const float expected[3][3] = {
        {-1.0f, 4.0f, 7.0f},
        {3.0f, -6.0f, -9.0f},
        {-2.0f, -5.0f, -8.0f}
    };

    memset(&gCamera, 0, sizeof(gCamera));
    gCamera.viewType = JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gCamera.focus.vx = 100;
    gCamera.focus.vy = 200;
    gCamera.focus.vz = 300;
    memset(&result, 0, sizeof(result));

    twatcameramatrix(&source, &result);
    CHECK(memcmp(&source, &original, sizeof(source)) == 0);
    CHECK(memcmp(result.m, expected, sizeof(expected)) == 0);
    CHECK(result.t[0] == 0x8000 - 100 + 0x100);
    CHECK(result.t[1] == 300 + 0x8000 - 0x100);
    CHECK(result.t[2] == 200);
    return 0;
}

int main(void)
{
    CHECK(test_lerp() == 0);
    CHECK(test_get_camera() == 0);
    CHECK(test_location() == 0);
    CHECK(test_state_and_snap() == 0);
    CHECK(test_scroll_camera() == 0);
    CHECK(test_gameplay_camera_owner() == 0);
    CHECK(test_camera_console_and_focus() == 0);
    CHECK(test_camera_to_view() == 0);
    CHECK(test_camera_slide() == 0);
    CHECK(test_scene_matrix_and_projection() == 0);
    CHECK(test_twatted_camera_matrix() == 0);
    puts("camera tests passed");
    return 0;
}
