/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\camera.c.
 *
 * Reviewed bodies cover the Camera state accessors, authored-dolly selection,
 * gameplay-camera placement, transition visibility checks, focused and
 * multiplayer camera modes, and screen-shake update listed below.
 *
 * Provenance:
 *   direct     - names/signatures from the exact PDB; Camera layout from TPI
 *                type 0x121A; gCamera and shake globals from linked symbols.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - field accesses, truncation, unchanged padding, and copy
 *                direction checked at the exact procedure RVAs.
 *
 * PDB module: 0013
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\camera.obj
 * Primary source: W:\SWJediPowerBattles\Work\camera.c
 * Compiler language: c
 * Emitted procedures: 23
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/camera.h"
#include "jpb/cube.h"
#include "jpb/flex.h"
#include "jpb/game.h"
#include "jpb/intersec.h"
#include "jpb/jonny.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/wrender.h"
#include "jpb/world.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/*
 * The matched PDB retains the accessor names but not names for their two
 * backing data objects at RVAs 0x547C08 and 0x547C10. These descriptive
 * module-local names therefore remain explicitly inferred.
 */
static Camera *jpb_current_camera;
static int32_t jpb_current_camera_type;
static VECTOR *jpb_focused_destination;

/* Exact file-local camera state at matched-PC RVAs 0x4F1AC4..0x4F1AE3. */
static char streetsending;
static _svector lead;
static int32_t cameralead;
static int32_t streetsendcampos;
static int32_t streetsendcamang;
static int32_t zeroBSSCheck;
static uint32_t camera_shake_start;

/* Exact global from boss.c used by camera types 5/6 and level 12. */
extern _svector gJarJarPos;

static int32_t camera_trunc_double_to_i32(double value)
{
    if (!(value >= -2147483648.0 && value < 2147483648.0)) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int16_t camera_low_i16(int32_t value)
{
    uint16_t low = (uint16_t)(uint32_t)value;

    if (low <= INT16_MAX) {
        return (int16_t)low;
    }
    return (int16_t)((int32_t)low - 65536);
}

static int32_t camera_i32_from_bits(uint32_t bits)
{
    int32_t value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int32_t camera_wrap_add(int32_t left, int32_t right)
{
    return camera_i32_from_bits((uint32_t)left + (uint32_t)right);
}

static int32_t camera_wrap_multiply(int32_t left, int32_t right)
{
    return camera_i32_from_bits((uint32_t)left * (uint32_t)right);
}

static int32_t camera_wrap_subtract(int32_t left, int32_t right)
{
    return camera_i32_from_bits((uint32_t)left - (uint32_t)right);
}

static int32_t camera_arithmetic_shift_right(int32_t value, unsigned bits)
{
    uint32_t shifted = (uint32_t)value >> bits;

    if (value < 0) {
        shifted |= UINT32_MAX << (32U - bits);
    }
    return camera_i32_from_bits(shifted);
}

static int32_t camera_trunc_divide_power_two(
    int32_t value, unsigned bits)
{
    int32_t bias = (int32_t)((UINT32_C(1) << bits) - 1U);

    if (value < 0) {
        value = camera_wrap_add(value, bias);
    }
    return camera_arithmetic_shift_right(value, bits);
}

static int32_t camera_flexmul(int32_t left, int32_t right)
{
    return camera_trunc_divide_power_two(
        camera_wrap_multiply(left, right), 12);
}

static int32_t camera_average(int32_t left, int32_t right)
{
    int32_t sum = camera_wrap_add(left, right);

    return sum >= 0 ? sum / 2 : -((-sum) / 2);
}

static int32_t camera_clamp(
    int32_t value, int32_t minimum, int32_t maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static int camera_player_active(playerObject *player)
{
    return
        player != NULL &&
        player->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &player->playerRoot, 0, UINT32_C(0x20)) == 0 &&
        (player->playerRoot.flags & UINT32_C(0x40200)) == 0;
}

static physicsObject *camera_player_physics(playerObject *player)
{
    sceneObject *scene;

    if (player == NULL || player->playerRoot.pParent == NULL) {
        return NULL;
    }
    scene = (sceneObject *)player->playerRoot.pParent;
    return (physicsObject *)scene->pPhysics;
}

static int32_t camera_i16_clamped(int32_t value)
{
    return camera_clamp(value, -0x7fff, 0x8000);
}

static int camera_ascii_equal(const char *left, const char *right)
{
    if (left == NULL || right == NULL) {
        return 0;
    }
    while (*left != '\0' && *right != '\0') {
        unsigned char a = (unsigned char)*left++;
        unsigned char b = (unsigned char)*right++;

        if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
        if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
        if (a != b) return 0;
    }
    return *left == *right;
}

static int32_t camera_wrapped_angle_delta(
    int32_t current, int32_t destination)
{
    int32_t delta = destination - current;

    if (delta > 0x800) {
        delta -= 0x1000;
    } else if (delta < -0x800) {
        delta += 0x1000;
    }
    return delta;
}

static void camera_CameraSlide(void);
static int camera_StuffCamera(
    int camera_type,
    Camera *camera,
    BAP_CAMERADOLLY *dolly,
    int changed_camera,
    VECTOR *position,
    int leading);

/* 0x23700, 42 bytes, global, 4 named locals
 * camLerp
 * PDB type: short (short, short, double)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
int16_t camLerp(int16_t input, int16_t target, double amount)
{
    double value = ((double)target - (double)input) * amount;

    value += (double)input;
    return camera_low_i16(camera_trunc_double_to_i32(value));
}

/* 0x23730, 279 bytes, global, 3 named locals
 * camera_Camera2ViewVector
 * PDB type: void (Camera*, sceneGeometryEnv*...
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
void camera_Camera2ViewVector(
    Camera *camera, sceneGeometryEnv *environment)
{
    camera->angle.vx &= 0x0ffe;
    camera->angle.vz &= 0x7fff;
    if ((camera->viewType & 0x0100) != 0) {
        camera_CameraSlide();
    }
    if ((camera->viewType & JPB_CAMERA_VIEW_RELATIVE_FOCUS) != 0) {
        environment->pos.vx =
            camera_low_i16(camera->campos.vx - (int32_t)camera->focus.vx);
        environment->pos.vy =
            camera_low_i16(camera->campos.vy - (int32_t)camera->focus.vy);
        environment->pos.vz =
            camera_low_i16(camera->campos.vz - (int32_t)camera->focus.vz);
        environment->angle.vx = camera_low_i16(
            ratan2(camera->angle.vz, camera->campos.vy + 1) + 0x400);
        environment->angle.vy = camera_low_i16(
            ratan2(-camera->campos.vx, camera->campos.vz + 1) - 0x800);
        environment->angle.vz = 0;
        return;
    }
    if ((camera->viewType & JPB_CAMERA_VIEW_ABSOLUTE_FOCUS) != 0) {
        environment->pos.vx = camera_low_i16(-(int32_t)camera->focus.vx);
        environment->pos.vy = camera_low_i16(-(int32_t)camera->focus.vy);
        environment->pos.vz = camera_low_i16(-(int32_t)camera->focus.vz);
        environment->angle.vx = camera_low_i16(camera->angle.vx);
        environment->angle.vy = camera_low_i16(camera->angle.vy);
        environment->angle.vz = 0;
        environment->posDest.vx =
            camera_low_i16(-(int32_t)camera->focusDest.vx);
        environment->posDest.vy =
            camera_low_i16(-(int32_t)camera->focusDest.vy);
        environment->posDest.vz =
            camera_low_i16(-(int32_t)camera->focusDest.vz);
        environment->angleDest.vx =
            camera_low_i16(camera->angleDest.vx);
        environment->angleDest.vy =
            camera_low_i16(camera->angleDest.vy);
        environment->angleDest.vz = 0;
    }
}

/* 0x23850, 578 bytes, local, 5 named locals
 * camera_CameraSlide
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
static void camera_CameraSlide(void)
{
    int32_t speed = gGlobalFrameRate != 0 ? gGlobalFrameRate : 1;
    int32_t delta;
    int32_t magnitude;
    int32_t step;

    delta = camera_wrapped_angle_delta(
        gCamera.angle.vx, gCamera.angleDest.vx);
    magnitude = delta < 0 ? -delta : delta;
    if (magnitude < 0x11) {
        step = delta / 2;
    } else {
        step = camera_flexmul(
            camera_trunc_divide_power_two(delta, 4), speed);
    }
    gCamera.angle.vx =
        camera_wrap_add(gCamera.angle.vx, step) & 0x0fff;

    delta = camera_wrapped_angle_delta(
        gCamera.angle.vy, gCamera.angleDest.vy);
    magnitude = delta < 0 ? -delta : delta;
    if (magnitude < 0x11) {
        step = delta / 2;
    } else {
        step = camera_flexmul(
            camera_trunc_divide_power_two(delta, 4), speed);
    }
    gCamera.angle.vy =
        camera_wrap_add(gCamera.angle.vy, step) & 0x0fff;

    delta = gCamera.angleDest.vz - gCamera.angle.vz;
    step = camera_flexmul(
        camera_trunc_divide_power_two(delta, 4), speed);
    gCamera.angle.vz = camera_wrap_add(gCamera.angle.vz, step);

    if ((gCamera.viewType & 0x2000) == 0) {
        delta = (int32_t)gCamera.focusDest.vx -
                (int32_t)gCamera.focus.vx;
        step = camera_flexmul(
            camera_trunc_divide_power_two(delta, 4), speed);
        gCamera.focus.vx =
            camera_low_i16((int32_t)gCamera.focus.vx + step);

        delta = (int32_t)gCamera.focusDest.vy -
                (int32_t)gCamera.focus.vy;
        step = camera_flexmul(
            camera_trunc_divide_power_two(delta, 4), speed);
        gCamera.focus.vy =
            camera_low_i16((int32_t)gCamera.focus.vy + step);

        delta = (int32_t)gCamera.focusDest.vz -
                (int32_t)gCamera.focus.vz;
        step = camera_flexmul(
            camera_trunc_divide_power_two(delta, 4), speed);
        gCamera.focus.vz =
            camera_low_i16((int32_t)gCamera.focus.vz + step);
        return;
    }

    step = camera_flexmul(0x20, speed);
    {
        int16_t short_step = camera_low_i16(step);
        int32_t focus_delta =
            camera_low_i16(
                (int32_t)gCamera.focusDest.vx -
                (int32_t)gCamera.focus.vx);

        if (focus_delta < -step) {
            gCamera.focus.vx = camera_low_i16(
                (int32_t)gCamera.focus.vx - (int32_t)short_step);
        } else if (focus_delta > step) {
            gCamera.focus.vx = camera_low_i16(
                (int32_t)gCamera.focus.vx + (int32_t)short_step);
        }

        focus_delta =
            camera_low_i16(
                (int32_t)gCamera.focusDest.vy -
                (int32_t)gCamera.focus.vy);
        if (focus_delta < -step) {
            gCamera.focus.vy = camera_low_i16(
                (int32_t)gCamera.focus.vy - (int32_t)short_step);
        } else if (focus_delta > step) {
            gCamera.focus.vy = camera_low_i16(
                (int32_t)gCamera.focus.vy + (int32_t)short_step);
        }

        focus_delta =
            camera_low_i16(
                (int32_t)gCamera.focusDest.vz -
                (int32_t)gCamera.focus.vz);
        if (focus_delta < -step) {
            gCamera.focus.vz = camera_low_i16(
                (int32_t)gCamera.focus.vz - (int32_t)short_step);
        } else if (focus_delta > step) {
            gCamera.focus.vz = camera_low_i16(
                (int32_t)gCamera.focus.vz + (int32_t)short_step);
        }
    }
}

/* 0x23AA0, 66 bytes, global, 2 named locals
 * camera_GetCamera
 * PDB type: Camera* (VECTOR*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
Camera *camera_GetCamera(VECTOR *angle, _svector *focus)
{
    angle->vx = gCamera.angle.vx;
    angle->vy = gCamera.angle.vy;
    angle->vz = gCamera.angle.vz;
    focus->vx = gCamera.focus.vx;
    focus->vy = gCamera.focus.vy;
    focus->vz = gCamera.focus.vz;
    return &gCamera;
}

/* 0x23AF0, 7 bytes, global, 0 named locals
 * camera_GetCurrentCameraType
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */

/* 0x23B00, 96 bytes, global, 2 named locals
 * camera_GetLocation
 * PDB type: void (VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
int camera_GetCurrentCameraType(void)
{
    return jpb_current_camera_type;
}
void camera_GetLocation(VECTOR *location)
{
    if ((gCamera.viewType & JPB_CAMERA_VIEW_RELATIVE_FOCUS) != 0) {
        location->vx = (int32_t)gCamera.focus.vx - gCamera.campos.vx;
        location->vy = (int32_t)gCamera.focus.vy - gCamera.campos.vy;
        location->vz = (int32_t)gCamera.focus.vz - gCamera.campos.vz;
        return;
    }
    if ((gCamera.viewType & JPB_CAMERA_VIEW_ABSOLUTE_FOCUS) != 0) {
        location->vx = (int32_t)gCamera.focus.vx;
        location->vy = (int32_t)gCamera.focus.vy;
        location->vz = (int32_t)gCamera.focus.vz;
    }
}

/* 0x23B60, 7 bytes, global, 0 named locals
 * camera_GetViewType
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
int camera_GetViewType(void)
{
    return gCamera.viewType;
}

/* 0x23B70, 331 bytes, global, 4 named locals
 * camera_RestoreCameras
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
void camera_RestoreCameras(void)
{
    int old_camera_type;
    int player0_active;
    int player1_active;

    if (gpWorld == NULL) {
        return;
    }
    memcpy(
        gpWorld->aDolly,
        gpWorld->aBkDolly,
        sizeof(gpWorld->aDolly));
    gCamera.viewType &= ~0x1000;
    uberZRange = 0;
    uberXRange = 0;
    if ((int)(int8_t)LevelSelect == 14) {
        uberPos.vx = 0x3b00;
        uberPos.vz = -0x5000;
        uberZRange = 0x5cc;
        uberXRange = 0x500;
    }
    uberLock = 0;
    gpWorld->overRideDolly = 0;
    newcameraflag = 1;

    if (GameStruct.NumPlayers != 2) {
        return;
    }
    old_camera_type = jpb_current_camera_type;
    player0_active = camera_player_active(gpWorld->player0);
    player1_active = camera_player_active(gpWorld->player1);
    if (!player0_active) {
        if (player1_active) {
            jpb_current_camera_type = 2;
        }
    } else {
        jpb_current_camera_type = player1_active ? 0 : 1;
    }
    if (old_camera_type != jpb_current_camera_type) {
        camera_SetCameras();
    }
}

/* 0x23CC0, 99 bytes, global, 3 named locals
 * camera_ScrollCamera
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
void camera_ScrollCamera(int dx, int dz)
{
    BAP_CAMERADOLLY *dolly =
        &gpWorld->aDolly[gpWorld->currentDolly];

    if (dx != 0 && (dolly->flags & UINT32_C(0x8)) == 0) {
        dolly->flags |= UINT32_C(0x8);
    }
    if (dz != 0 && (dolly->flags & UINT32_C(0x10)) == 0) {
        dolly->flags |= UINT32_C(0x10);
    }
    dolly->offx = camera_low_i16(
        (int32_t)dolly->offx + dx);
    dolly->offz = camera_low_i16(
        (int32_t)dolly->offz + dz);
}

/* 0x23D30, 2093 bytes, global, 31 named locals
 * camera_SetCameraPos
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
int camera_SetCameraPos(int camera_type)
{
    playerObject *player0;
    playerObject *player1;
    playerObject *selected0;
    playerObject *selected1;
    physicsObject *physics0;
    physicsObject *physics1;
    VECTOR *position0;
    VECTOR *position1;
    VECTOR old_location;
    VECTOR target;
    VECTOR high_point;
    _jheightstuff height_stuff;
    int candidate_camera;
    int changed_camera = 0;
    int camera_accepted = 1;

    if (gpWorld == NULL || gpWorld->player0 == NULL ||
        gpWorld->player1 == NULL) {
        return 0;
    }
    player0 = gpWorld->player0;
    player1 = gpWorld->player1;
    position0 = physics_gGetPosition(&player0->playerRoot);
    position1 = physics_gGetPosition(&player1->playerRoot);
    if (position0 == NULL || position1 == NULL) {
        return 0;
    }

    selected0 = camera_type == 2 ? player1 : player0;
    selected1 = camera_type == 1 ? player0 : player1;
    position0 = physics_gGetPosition(&selected0->playerRoot);
    position1 = physics_gGetPosition(&selected1->playerRoot);
    physics0 = camera_player_physics(selected0);
    physics1 = camera_player_physics(selected1);
    if (position0 == NULL || position1 == NULL ||
        physics0 == NULL || physics1 == NULL) {
        return 0;
    }

    old_location.vx = gpWorld->location.vx;
    old_location.vy = gpWorld->location.vy;
    old_location.vz = gpWorld->location.vz;
    old_location.pad = 0;
    if (camera_type == 0) {
        float ground0 = physics0->airGround;
        float ground1 = physics1->airGround;
        float minimum0 = (float)position0->vy - 512.0f;
        float minimum1 = (float)position1->vy - 512.0f;
        int32_t vertical_sum;

        if (ground0 < minimum0) ground0 = minimum0;
        if (ground1 < minimum1) ground1 = minimum1;
        target.vx = camera_average(position0->vx, position1->vx);
        vertical_sum =
            (int32_t)ground0 + (int32_t)ground1 +
            position0->vy + position1->vy;
        target.vy = camera_trunc_divide_power_two(vertical_sum, 2);
        target.vz = camera_average(position0->vz, position1->vz);
    } else {
        target.vx = camera_average(position0->vx, position1->vx);
        target.vy = camera_average(position0->vy, position1->vy);
        target.vz = camera_average(position0->vz, position1->vz);
        if (camera_type == 5) {
            if (camera_player_active(selected0)) {
                gpWorld->location.vx = (int32_t)physics0->pos.vx;
                gpWorld->location.vy = (int32_t)physics0->pos.vy;
                gpWorld->location.vz = (int32_t)physics0->pos.vz;
            }
            if (camera_player_active(selected1)) {
                gpWorld->location.vx = (int32_t)physics1->pos.vx;
                gpWorld->location.vy = (int32_t)physics1->pos.vy;
                gpWorld->location.vz = (int32_t)physics1->pos.vz;
            }
            target.vx = gpWorld->location.vx;
            target.vy = gpWorld->location.vy;
            target.vz = gpWorld->location.vz;
        }
    }
    target.pad = 0;
    if ((int)(int8_t)LevelSelect == 12) {
        target.vx = gJarJarPos.vx;
        target.vy = gJarJarPos.vy;
        target.vz = gJarJarPos.vz;
    }

    high_point = target;
    high_point.vy += 0x100;
    memset(&height_stuff, 0, sizeof(height_stuff));
    {
        int ground = intersec_FindWalkHeight(
            &high_point,
            NULL,
            (objectRoot *)(void *)&height_stuff,
            1);

        if (height_stuff.poly == NULL || target.vy - ground >= 0x40) {
            candidate_camera = gpWorld->currentDolly;
        } else {
            const uint8_t *camera_record =
                (const uint8_t *)(const void *)height_stuff.poly;
            uint32_t polygon_word = (uint32_t)*height_stuff.poly;

            if ((polygon_word & UINT32_C(0x3c000000)) == 0 &&
                leveldata != NULL) {
                int32_t record =
                    (int32_t)((polygon_word >> 14) & UINT32_C(0xff)) * 9 +
                    (leveldata[-4] >> 11);

                const uint8_t *resolved_record =
                    (const uint8_t *)(const void *)(leveldata + record);

                /* Library-style floor records can encode an offset beyond
                 * the finite Jonny payload.  Arena first exposed this at the
                 * portable boundary; Ruins, Mini3, and Mini4 contain the
                 * same form.  Preserve the current dolly when the authored
                 * record does not belong to the relocated level archive. */
                if (!jpb_LevelDataContains(resolved_record, 8)) {
                    camera_record = NULL;
                } else {
                    camera_record = resolved_record;
                }
            }
            candidate_camera = camera_record != NULL
                ? camera_record[7] & 0x7f
                : gpWorld->currentDolly;
            if ((int)(int8_t)LevelSelect == 9 &&
                candidate_camera == 0x2a) {
                candidate_camera = 0;
            }
        }
    }
    if (gpWorld->overRideDolly > 0) {
        candidate_camera = gpWorld->overRideDolly;
    }
    candidate_camera = camera_clamp(candidate_camera, 0, 0xff);

    if (gpWorld->currentDolly != candidate_camera ||
        newcameraflag != 0) {
        Camera test_camera;
        sceneGeometryEnv local_environment;
        MATRIX test_matrix;
        VECTOR camera_position;
        FVECTOR4 new_frustum[6];
        FVECTOR4 box[6];
        int distances0[5] = {0};
        int distances1[5] = {0};
        int height;
        int box_mask;
        int offscreen = 0;
        int radius = camera_type == 0 ? -0xa0 : -0x54;

        newcameraflag = 0;
        memset(&test_camera, 0, sizeof(test_camera));
        memset(&local_environment, 0, sizeof(local_environment));
        test_camera.viewType = JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
        PushMatrix();
        (void)camera_StuffCamera(
            camera_type,
            &test_camera,
            &gpWorld->aDolly[candidate_camera],
            0,
            &target,
            0);
        camera_Camera2ViewVector(
            &test_camera, &local_environment);
        (void)scene_UpdateWorld2ScreenMatrix(&local_environment);
        twatcameramatrix(&local_environment.matrix, &test_matrix);
        camera_position.vx = test_camera.focus.vx;
        camera_position.vy = test_camera.focus.vy;
        camera_position.vz = test_camera.focus.vz;
        camera_position.pad = 0;
        buildfrustrum(
            &test_matrix,
            new_frustum,
            &camera_position,
            90.0f,
            320.0f,
            180.0f);
        if (camera_type == 0) {
            height = (int32_t)(
                (physics0->validairground +
                 physics1->validairground) *
                0.5f);
        } else if (camera_type == 2) {
            height = (int32_t)physics1->validairground;
        } else {
            height = (int32_t)physics0->validairground;
        }
        box_mask = CalcNewBox(height, new_frustum, box);
        if (camera_player_active(player0)) {
            offscreen =
                cliptofrustrum(
                    new_frustum,
                    &maPhysicsData[0].pos,
                    radius,
                    distances0) &
                box_mask &
                0x0f;
        }
        if (camera_player_active(player1)) {
            offscreen |=
                cliptofrustrum(
                    new_frustum,
                    &maPhysicsData[1].pos,
                    radius,
                    distances1) &
                box_mask &
                0x0f;
        }
        PopMatrix();

        camera_accepted =
            camera_type == 5 ||
            candidate_camera >= 0x80 ||
            offscreen == 0 ||
            (ch_pad(UINT16_C(0x400)) != 0 &&
             ch_pad(UINT16_C(0x40)) != 0 &&
             ch_pad(UINT16_C(0x8000)) != 0 &&
             ch_pad(UINT16_C(0x200)) != 0);
        if ((int)(int8_t)LevelSelect == 10 &&
            candidate_camera == 0x0f &&
            (maPhysicsData[0].pos.vy < 9600.0f ||
             maPhysicsData[1].pos.vy < 9600.0f)) {
            camera_accepted = 0;
        }
        if (camera_accepted) {
            gpWorld->currentDolly =
                camera_low_i16(candidate_camera);
            changed_camera = 1;
        } else {
            target = old_location;
        }
    }

    if (camera_type == 6) {
        gpWorld->location.vx = gJarJarPos.vx;
        gpWorld->location.vy = gJarJarPos.vy;
        gpWorld->location.vz = gJarJarPos.vz;
    } else {
        gpWorld->location.vx = target.vx;
        gpWorld->location.vy = target.vy;
        gpWorld->location.vz = target.vz;
    }
    return camera_StuffCamera(
        camera_type,
        &gCamera,
        &gpWorld->aDolly[gpWorld->currentDolly],
        changed_camera,
        &target,
        1);
}

/* 0x24560, 832 bytes, global, 15 named locals
 * camera_SetCameras
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
void camera_SetCameras(void)
{
    switch (jpb_current_camera_type) {
    case 0:
    case 1:
    case 2:
    case 5:
    case 6:
        (void)camera_SetCameraPos(jpb_current_camera_type);
        break;
    case 3:
        if (gpWorld != NULL && gpWorld->player0 != NULL &&
            gpWorld->player1 != NULL) {
            VECTOR *position0 = physics_gGetPosition(
                &gpWorld->player0->playerRoot);
            VECTOR *position1 = physics_gGetPosition(
                &gpWorld->player1->playerRoot);

            if (position0 != NULL && position1 != NULL) {
                int32_t delta_x = position0->vx - position1->vx;
                int32_t delta_z = position0->vz - position1->vz;
                int32_t destination_yaw;
                int32_t delta;

                if (delta_x == 0) delta_x = 1;
                destination_yaw =
                    (ratan2(delta_z, delta_x) - 0x400) & 0x0fff;
                gCamera.focusDest.vx =
                    camera_low_i16(position0->vx);
                gCamera.focusDest.vy =
                    camera_low_i16(position0->vy + 0x100);
                gCamera.focusDest.vz =
                    camera_low_i16(position0->vz);
                delta = camera_wrapped_angle_delta(
                    destination_yaw, gCamera.angle.vy);
                if ((delta < 0 ? -delta : delta) > 0x400) {
                    destination_yaw =
                        (destination_yaw - 0x800) & 0x0fff;
                }
                gCamera.angleDest.vx = -0x1e2;
                gCamera.angleDest.vy = destination_yaw;
                gCamera.angleDest.vz = 0x200;
                if ((gCamera.viewType & UINT32_C(0x100)) == 0) {
                    gCamera.angle = gCamera.angleDest;
                    gCamera.focus = gCamera.focusDest;
                }
            }
        }
        break;
    case 4:
        if (gCamera.cameraTimer < (uint32_t)totalframes) {
            gCamera.userData = 0;
            jpb_focused_destination = NULL;
        }
        if (gCamera.userData != 0) {
            const VECTOR *destination =
                jpb_focused_destination != NULL
                    ? jpb_focused_destination
                    : (const VECTOR *)(uintptr_t)gCamera.userData;

            gCamera.focusDest.vx =
                camera_low_i16(destination->vx);
            gCamera.focusDest.vy =
                camera_low_i16(destination->vy);
            gCamera.focusDest.vz =
                camera_low_i16(destination->vz);
            gCamera.angleDest.vz = 0x200;
            if ((gCamera.viewType & UINT32_C(0x100)) == 0) {
                gCamera.angle = gCamera.angleDest;
                gCamera.focus = gCamera.focusDest;
            }
            break;
        }
        jpb_current_camera_type = 1;
        jpb_current_camera = &gCamera;
        break;
    default:
        break;
    }

    if (screenshake > 0) {
        if (screenshake == 0x100) {
            camera_shake_start = (uint32_t)totalframes;
        }
        gCamera.focus.vy = camera_low_i16(
            (int32_t)gCamera.focus.vy +
            (int32_t)(
                sin((double)((uint32_t)totalframes - camera_shake_start)) *
                (double)screenshakeamplitude *
                (double)(screenshake / 0x20)));
        screenshake -= 6;
    }
}

/* 0x248A0, 97 bytes, global, 2 named locals
 * camera_SetCurrentCameraType
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */

/* 0x24910, 36 bytes, global, 3 named locals
 * camera_SetFocusedCameraFocus
 * PDB type: void (int, VECTOR*, int)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
void camera_SetFocusedCameraFocus(
    int camera_type, VECTOR *destination, int time)
{
    gCamera.cameraTimer =
        (uint32_t)totalframes + (uint32_t)(time * 0x200);
    jpb_current_camera_type = camera_type;
    gCamera.userData = (uint32_t)(uintptr_t)destination;
    jpb_focused_destination = destination;
}

/* 0x24940, 17 bytes, global, 1 named locals
 * camera_SetShake
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
void camera_SetCurrentCameraType(int type)
{
    if (jpb_current_camera_type != type) {
        jpb_current_camera_type = type;
        if (type == 5) {
            uint32_t player_y_bits;

            /*
             * The reference copies the raw pos.vy word into VECTOR.vy,
             * rather than performing a float-to-integer conversion.
             */
            memcpy(
                &player_y_bits,
                &maPhysicsData[0].pos.vy,
                sizeof(player_y_bits));
            Camera *camera =
                jpb_current_camera != NULL
                    ? jpb_current_camera
                    : &gCamera;

            streetcampos.vx =
                (int32_t)camera->focus.vx - 0x100;
            streetcampos.vy = (int32_t)player_y_bits;
            streetcampos.vz =
                (int32_t)camera->focus.vz;
        }
    }
    jpb_current_camera = &gCamera;
}
void camera_SetShake(int amplitude)
{
    screenshakeamplitude = amplitude;
    screenshake = 0x100;
}

/* 0x24960, 7 bytes, global, 1 named locals
 * camera_SetViewType
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
void camera_SetViewType(int viewType)
{
    gCamera.viewType = viewType;
}

/* 0x24970, 93 bytes, global, 3 named locals
 * camera_SnapCamera
 * PDB type: void (Camera*)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
void camera_SnapCamera(Camera *camera)
{
    camera->angle.vx = gCamera.angleDest.vx;
    camera->angle.vy = gCamera.angleDest.vy;
    camera->angle.vz = gCamera.angleDest.vz;
    camera->angleDest.vx = gCamera.angleDest.vx;
    camera->angleDest.vy = gCamera.angleDest.vy;
    camera->angleDest.vz = gCamera.angleDest.vz;
    camera->focus.vx = gCamera.focusDest.vx;
    camera->focus.vy = gCamera.focusDest.vy;
    camera->focus.vz = gCamera.focusDest.vz;
    camera->focusDest.vx = gCamera.focusDest.vx;
    camera->focusDest.vy = gCamera.focusDest.vy;
    camera->focusDest.vz = gCamera.focusDest.vz;
}

/* 0x249D0, 2785 bytes, local, 45 named locals
 * camera_StuffCamera
 * PDB type: int (int, Camera*, BAP_CAMERADOL...
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */

/* 0x254C0, 5 bytes, global, 1 named locals
 * camera_gGetLocation
 * PDB type: void (VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
static int camera_StuffCamera(
    int camera_type,
    Camera *camera,
    BAP_CAMERADOLLY *dolly,
    int changed_camera,
    VECTOR *position,
    int leading)
{
    playerObject *player0;
    playerObject *player1;
    playerObject *selected_player;
    physicsObject *physics;
    physicsObject *physics2;
    uint32_t flags;
    int32_t pitch;
    int32_t yaw;
    int32_t focus_x;
    int32_t focus_y;
    int32_t focus_z;
    int level = (int)(int8_t)LevelSelect;

    if (camera == NULL || dolly == NULL || position == NULL ||
        gpWorld == NULL) {
        return 0;
    }
    player0 = gpWorld->player0;
    player1 = gpWorld->player1;
    selected_player = camera_type == 2 ? player1 : player0;
    physics = camera_player_physics(selected_player);
    physics2 = camera_player_physics(player1);
    flags = dolly->flags;
    pitch = (int32_t)dolly->pitch + 0x1c;
    yaw = (int32_t)dolly->yaw & 0x0fff;
    mCameraAngleDest = -(int32_t)dolly->yaw;

    if (leading != 0) {
        _svector camera_direction = {
            camera_low_i16(rsin(yaw)),
            0,
            camera_low_i16(rcos(yaw)),
            0
        };
        _svector normalized_lead;

        /*
         * The matched owner samples the existing lead before advancing it.
         * normalize_svector also preserves its exact in-place overflow
         * reduction side effect. The prior reconstruction sampled the
         * already-updated vector and transposed sine/cosine, which caused
         * sustained movement to bias the gameplay framing.
         */
        (void)normalize_svector(&lead, &normalized_lead);
        cameralead = DOT12(&camera_direction, &normalized_lead);
    }

    if (leading != 0 && physics != NULL &&
        gGlobalFrameRate != 0 && level != 12) {
        _svector movement;
        int32_t speed;
        int32_t lead_speed = camera_flexmul(0x24a, gGlobalFrameRate);
        int32_t lead_slowdown = camera_flexmul(0x100, gGlobalFrameRate);
        int32_t decay =
            0x1000 - camera_flexmul(0x18c, gGlobalFrameRate);

        if (cameralead > 0) {
            lead_speed += camera_trunc_divide_power_two(cameralead, 4);
        }
        movement.vx = camera_low_i16((int32_t)physics->mov.vx);
        movement.vy = camera_low_i16((int32_t)physics->mov.vy);
        movement.vz = camera_low_i16((int32_t)physics->mov.vz);
        if (camera_type == 0 && physics2 != NULL) {
            movement.vx = camera_low_i16(camera_average(
                movement.vx, (int32_t)physics2->mov.vx));
            movement.vy = camera_low_i16(camera_average(
                movement.vy, (int32_t)physics2->mov.vy));
            movement.vz = camera_low_i16(camera_average(
                movement.vz, (int32_t)physics2->mov.vz));
        }
        speed = normalize(
            movement.vx, movement.vy, movement.vz, &movement);
        movement.vy = 0;
        if (lead_slowdown != 0 && speed != 0) {
            int32_t scale =
                camera_wrap_multiply(speed, lead_speed) /
                lead_slowdown;

            lead.vx = camera_low_i16(
                (int32_t)lead.vx +
                camera_flexmul(movement.vx, scale));
            lead.vz = camera_low_i16(
                (int32_t)lead.vz +
                camera_flexmul(movement.vz, scale));
        }
        lead.vy = 0;
        lead.vx = camera_low_i16(camera_flexmul(lead.vx, decay));
        lead.vz = camera_low_i16(camera_flexmul(lead.vz, decay));
    }

    if ((flags & UINT32_C(0x400)) != 0) {
        focus_x = dolly->offset.vx + dolly->slackx;
        focus_y = dolly->offset.vy + dolly->offx;
        focus_z = dolly->offset.vz + dolly->slackz;
        gpWorld->location = dolly->offset;
    } else if (camera_type == 5) {
        int32_t maximum_x = INT32_MIN;

        pitch = 0x200;
        yaw = 0x400;
        focus_y = streetcampos.vy +
            (GameStruct.NumPlayers == 2 ? 0x4e8 : 0x4d0);
        if (camera_player_active(player0) && physics != NULL) {
            maximum_x = (int32_t)physics->pos.vx;
        }
        if (camera_player_active(player1) && physics2 != NULL &&
            (int32_t)physics2->pos.vx > maximum_x) {
            maximum_x = (int32_t)physics2->pos.vx;
        }
        focus_x = camera_wrap_add(maximum_x, 0x29c);
        focus_z = streetcampos.vz;

        if (zeroBSSCheck != zerobss_levelReset) {
            zeroBSSCheck = zerobss_levelReset;
            zerobss_levelReset = 0;
            streetsendcamang = 0;
            streetsending = 0;
            streetsendcampos = 0;
        }
        if ((jpb_CubeRuntimeFlags & UINT32_C(8)) != 0) {
            if (streetsending == 0) {
                streetsending = 1;
                streetsendcampos = focus_x;
                streetsendcamang =
                    0x200 - camera_flexmul(8, gGlobalFrameRate);
            } else {
                if (streetsendcamang > 0x140) {
                    streetsendcamang -=
                        camera_flexmul(4, gGlobalFrameRate);
                }
                streetsendcampos -=
                    camera_flexmul(0x24, gGlobalFrameRate);
                focus_x = streetsendcampos;
                pitch = streetsendcamang;
            }
        }
        focus_z = camera_low_i16(
            ((((camera_low_i16(
                    (int32_t)((uint32_t)(focus_z + 0x7f80) >> 8)) +
                8) & 0xfff0) -
              0x7f) *
             0x100) +
            0x80);
    } else {
        if (camera_type == 6) {
            position->vx = gJarJarPos.vx;
            position->vy = gJarJarPos.vy;
            position->vz = gJarJarPos.vz;
        }
        focus_x = position->vx +
            ((flags & UINT32_C(0x8)) == 0
                 ? dolly->offset.vx
                 : 0);
        focus_y = position->vy +
            ((flags & UINT32_C(0x1000)) == 0
                 ? dolly->offset.vy
                 : 0);
        focus_z = position->vz +
            ((flags & UINT32_C(0x10)) == 0
                 ? dolly->offset.vz
                 : 0);
        if (uberLock != 0) {
            focus_x = position->vx;
            focus_z = position->vz;
        }
        if (leading != 0 && gGlobalFrameRate != 0 && level != 12) {
            focus_x += lead.vx;
            focus_z += lead.vz;
        }
        if (camera_type == 6 && level == 12) {
            switch (gpWorld->currentDolly) {
            case 0x10:
                focus_x += 0x500;
                focus_y -= 0xc0;
                focus_z += 0x100;
                break;
            case 0x11:
                pitch += 0xa0;
                focus_x += 0x500;
                focus_y += 0x100;
                break;
            case 0x17:
                focus_y += 0x80;
                focus_z += 0x200;
                break;
            default:
                focus_x += 0x500;
                focus_y -= 0x40;
                focus_z += 0x100;
                break;
            }
        }

        if (uberLock == 0 && camera_type != 5 && level != 12) {
            if ((flags & UINT32_C(0x8)) != 0) {
                int32_t candidate = focus_x + dolly->offx;
                int32_t minimum =
                    (flags & UINT32_C(0x100)) == 0
                        ? dolly->offset.vx - dolly->slackx
                        : candidate;
                int32_t maximum =
                    (flags & UINT32_C(0x80)) == 0
                        ? dolly->offset.vx + dolly->slackx
                        : candidate;
                focus_x = camera_clamp(candidate, minimum, maximum);
            }
            if ((flags & UINT32_C(0x1000)) != 0) {
                focus_y = camera_clamp(
                    focus_y + dolly->offy,
                    dolly->offset.vy - dolly->slacky,
                    dolly->offset.vy + dolly->slacky);
            }
            if ((flags & UINT32_C(0x10)) != 0) {
                int32_t candidate = focus_z + dolly->offz;
                int32_t minimum =
                    (flags & UINT32_C(0x20)) == 0
                        ? dolly->offset.vz - dolly->slackz
                        : candidate;
                int32_t maximum =
                    (flags & UINT32_C(0x40)) == 0
                        ? dolly->offset.vz + dolly->slackz
                        : candidate;
                focus_z = camera_clamp(candidate, minimum, maximum);
            }
        } else if (uberLock != 0) {
            if (uberXRange != 0) {
                focus_x = camera_clamp(
                    focus_x,
                    dolly->offset.vx + uberPos.vx - uberXRange,
                    dolly->offset.vx + uberPos.vx + uberXRange);
            }
            if ((flags & UINT32_C(0x1000)) != 0) {
                focus_y = camera_clamp(
                    focus_y + dolly->offy,
                    dolly->offset.vy - dolly->slacky,
                    dolly->offset.vy + dolly->slacky);
            }
            if (uberZRange != 0) {
                focus_z = camera_clamp(
                    focus_z,
                    dolly->offset.vz + uberPos.vz - uberZRange,
                    dolly->offset.vz + uberPos.vz + uberZRange);
            }
        }
    }

    if ((flags & UINT32_C(0x2000)) != 0) {
        int32_t dx = camera_low_i16(focus_x - position->vx);
        int32_t dz = camera_low_i16(focus_z - position->vz);
        _svector ignored_normal;

        (void)normalize(
            dx,
            camera_low_i16(focus_y - position->vy),
            dz,
            &ignored_normal);
        yaw = ratan2(dx, dz) & 0x0fff;
        mCameraAngleDest = -yaw;
    }

    if (level == 25 && player0 != NULL && player1 != NULL) {
        int32_t average_x = camera_average(
            (int32_t)maPhysicsData[0].pos.vx,
            (int32_t)maPhysicsData[1].pos.vx);
        int32_t range = physics_gGetRange(
            &player0->playerRoot, &player1->playerRoot);

        focus_x = camera_clamp(
            average_x + 0x100 + range, 0x1838, 0x1a90);
    }

    focus_x = camera_i16_clamped(focus_x);
    focus_y = camera_i16_clamped(focus_y);
    focus_z = camera_i16_clamped(focus_z);
    if (camera_type == 5 ||
        (((flags & UINT32_C(4)) != 0) && changed_camera != 0) ||
        (camera->viewType & UINT32_C(0x1000)) == 0 ||
        level == 12) {
        camera->angle.vx = pitch;
        camera->angle.vy = yaw;
        camera->angle.vz = 0;
        camera->focus.vx = camera_low_i16(focus_x);
        camera->focus.vy = camera_low_i16(focus_y);
        camera->focus.vz = camera_low_i16(focus_z);
        camera->angleDest.vx = pitch;
        camera->angleDest.vy = yaw;
        camera->angleDest.vz = 0;
        camera->focusDest = camera->focus;
        camera->viewType |= UINT32_C(0x1000);
    } else {
        camera->angleDest.vx = pitch;
        camera->angleDest.vy = yaw;
        camera->angleDest.vz = 0;
        camera->focusDest.vx = camera_low_i16(focus_x);
        camera->focusDest.vy = camera_low_i16(focus_y);
        camera->focusDest.vz = camera_low_i16(focus_z);
    }

    {
        VECTOR view = {0, 0, 0, 0};

        camera_GetLocation(&view);
        cameraLocation.vx = camera_low_i16(view.vx);
        cameraLocation.vy = camera_low_i16(view.vy);
        cameraLocation.vz = camera_low_i16(view.vz);
    }
    cameraYaw = yaw;
    cameraFacing.vx = camera_low_i16(-rsin(yaw));
    cameraFacing.vy = 0;
    cameraFacing.vz = camera_low_i16(-rcos(yaw));
    return 1;
}
void camera_gGetLocation(VECTOR *location)
{
    camera_GetLocation(location);
}

/* 0x254D0, 171 bytes, global, 4 named locals
 * console_CamerasCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */

/* 0x25580, 28 bytes, global, 3 named locals
 * lerp
 * PDB type: double (double, double, double)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
int console_CamerasCommand(
    int argument_count,
    char **string_arguments,
    int *integer_arguments,
    float *float_arguments)
{
    (void)float_arguments;

    if (argument_count == 2 &&
        string_arguments != NULL &&
        integer_arguments != NULL) {
        if (camera_ascii_equal(string_arguments[0], "shake")) {
            camera_SetShake(integer_arguments[1]);
            return 0;
        }
        if (camera_ascii_equal(string_arguments[0], "set") &&
            gpWorld != NULL) {
            gpWorld->overRideDolly =
                camera_low_i16(integer_arguments[1]);
            return 0;
        }
    }
    /* The original tail only presents usage text through console_Printf. */
    return 0;
}
double lerp(double x, double y, double amount)
{
    return (1.0 - amount) * x + y * amount;
}

/* 0x255A0, 23 bytes, global, 5 named locals
 * map
 * PDB type: int (int, int, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
int map(
    int x,
    int input_minimum,
    int input_maximum,
    int output_minimum,
    int output_maximum)
{
    int32_t numerator = camera_wrap_multiply(
        camera_wrap_subtract(output_maximum, output_minimum),
        camera_wrap_subtract(x, input_minimum));

    return camera_wrap_add(
        numerator /
            camera_wrap_subtract(
                input_maximum, input_minimum),
        output_minimum);
}

/* 0x255C0, 67 bytes, global, 6 named locals
 * mapClamped
 * PDB type: int (int, int, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
int mapClamped(
    int x,
    int input_minimum,
    int input_maximum,
    int output_minimum,
    int output_maximum)
{
    int value;

    if (output_minimum == output_maximum) {
        return output_minimum;
    }
    value = map(
        x,
        input_minimum,
        input_maximum,
        output_minimum,
        output_maximum);
    if (output_minimum < output_maximum) {
        if (value < output_minimum) {
            return output_minimum;
        }
        if (value > output_maximum) {
            return output_maximum;
        }
        return value;
    }
    if (value > output_minimum) {
        return output_minimum;
    }
    if (value < output_maximum) {
        return output_maximum;
    }
    return value;
}

/* 0x25610, 31 bytes, global, 5 named locals
 * mapDouble
 * PDB type: double (double, double, double, ...
 * Source: W:\SWJediPowerBattles\Work\camera.c
 */
double mapDouble(
    double x,
    double input_minimum,
    double input_maximum,
    double output_minimum,
    double output_maximum)
{
    return
        ((x - input_minimum) *
         (output_maximum - output_minimum)) /
            (input_maximum - input_minimum) +
        output_minimum;
}
