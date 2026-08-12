/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\scene.c.
 *
 * The reviewed subset owns gameplay view records, current/destination
 * world-to-screen matrices, scene transform publication, player death, and
 * post-render timing/double-buffer state, off-screen player warning
 * presentation, and the main render scheduler. Level-specific callbacks are
 * being restored individually from their owning modules.
 *
 * Provenance:
 *   direct     - names/signatures from the exact PDB; sceneGeometryEnv from
 *                TPI type 0x125B; v3Translate and gGTEMATRIX from symbols.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - hurtplayer gates, state masks, sound arguments, component
 *                offsets, one-based tank-enemy lookup and driver cleanup
 *                checked at RVA 0xF4EB0; off-screen projection, timers,
 *                warning/penalty sequence, exact four-vertex fan geometry,
 *                colors, and draw suppression checked at RVA 0xF5050;
 *                matrix stack identities, call order,
 *                copies, translations, return, and state side effect checked
 *                at RVA 0xF5660; transform accessors and no-op stubs checked
 *                at 0xF5610..0xF5D0F; post-render timer, strobe, scene-ready,
 *                and draw-surface transitions checked at 0xF6290..0xF6312.
 *
 * PDB module: 0076
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\scene.obj
 * Primary source: W:\SWJediPowerBattles\Work\scene.c
 * Compiler language: c
 * Emitted procedures: 25
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/scene.h"
#include "jpb/anim.h"
#include "jpb/boss.h"
#include "jpb/game.h"
#include "jpb/camera.h"
#include "jpb/console.h"
#include "jpb/cube.h"
#include "jpb/enemy.h"
#include "jpb/flex.h"
#include "jpb/force.h"
#include "jpb/model.h"
#include "jpb/linkstubs.h"
#include "jpb/level.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/prim.h"
#include "jpb/pwrup.h"
#include "jpb/render_nodes.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/texture.h"
#include "jpb/whook.h"
#include "jpb/world.h"
#include "jpb/wrender.h"

#include <string.h>

/* Direct global at RVA 0x9460C0; the executable bounds it at 20 records. */
sceneObject maSceneData[JPB_SCENE_CAPACITY];

extern int mDrawingSurfaceId;

static JPBSceneMiddleRenderHooks scene_middle_render_hooks;
static void *scene_middle_render_user_data;

/* 0xF4EB0, 408 bytes, global, 5 named locals
 * hurtplayer
 * PDB type: void (playerObject*, int)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void hurtplayer(playerObject *player, int mod)
{
    int index = player->playerRoot.objectID;
    wsl_ENEMY *tankenemy;
    int tank_index;
    sceneObject *scene;
    physicsObject *physics;
    modelObject *model;

    if (index == -1 ||
        obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) != 0 ||
        (player->pFlags & UINT32_C(0x40200)) != 0 ||
        index >= 2) {
        return;
    }

    game_gModEnergy(index, mod);
    if (game_gGetEnergy(index) > 0) {
        return;
    }

    if ((player->pFlags & UINT32_C(0x200)) == 0) {
        player->pFlags |= UINT32_C(0x200);
    }
    if (player->playernum < 2) {
        game_gSetGameFlags(
            UINT32_C(0x20) << ((uint16_t)player->playernum & 31));
        player_AfterLife(player);
        obj_gSetObjectFlag(&player->playerRoot, 0, 0x20);
    }

    player->hitNumber = 1;
    player->hitMotion = NULL;
    (void)sound_Play(NULL, 3, "jedihit", 3);

    scene = (sceneObject *)player->playerRoot.pParent;
    physics = (physicsObject *)scene->pPhysics;
    physics->falltimer = 0;
    physics->flags &= UINT32_C(0xffffff40);
    physics->falltimer = 0;
    physics->movemode = MOVE_NORMAL;
    player->pFlags &= UINT32_C(0xffffff7f);
    model = (modelObject *)scene->pModel;
    model->flags &= UINT32_C(0xffffffef);

    tank_index = playertankindex;
    if (tank_index == 0) {
        return;
    }
    tankenemy = gaPlayerData[tank_index - 1].pEnemy;
    if (player == tankdrivers[0]) {
        tankdrivers[0] = NULL;
        timesincetank[0] = 0x1e000;
    } else if (player == tankdrivers[1]) {
        tankdrivers[1] = NULL;
        timesincetank[1] = 0x1e000;
    } else {
        return;
    }

    playertankindex = 0;
    tankenemy->enemyFlags &=
        ~(UINT32_C(1) << ((index + 0x1a) & 31));
}

/* 0xF5050, 1458 bytes, global, 22 named locals
 * playerOffScreenArrow
 * PDB type: void (physicsObject*, unsigned)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void playerOffScreenArrow(physicsObject *p0, unsigned color)
{
    static int timeoffscreen[2];
    static int offscreenbleeps[2];
    static int zeroBSSCheck;
    static _Material *trans;
    union {
        int packed;
        int16_t component[2];
    } projected, original;
    sceneObject *scene;
    playerObject *player;
    _svector world_position;
    _svector screen_center;
    _svector original_screen_position;
    _svector direction;
    int player_index;
    int previous_time;
    int timeout;
    int alpha;
    int direction_x_scaled;
    int direction_y_scaled;
    int direction_x_quarter;
    int direction_y_quarter;
    int clipped_x;
    int clipped_y;
    int left_x;
    int left_y;
    int right_x;
    int right_y;
    int center_x;
    int center_y;
    uint32_t opaque_color;
    uint32_t point_color;

    scene = (sceneObject *)p0->physicsRoot.pParent;
    player = (playerObject *)scene->pPlayer;
    if (GameStruct.CurrentLevel == 0 ||
        gSCENE_READY == 0 ||
        player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) != 0 ||
        (player->pFlags & UINT32_C(0x40200)) != 0) {
        return;
    }

    if (zeroBSSCheck != zerobss_levelReset) {
        zeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        memset(timeoffscreen, 0, sizeof(timeoffscreen));
        memset(offscreenbleeps, 0, sizeof(offscreenbleeps));
    }

    world_position.vx = (int16_t)(int)p0->pos.vx;
    world_position.vy =
        (int16_t)((int)p0->pos.vy + 0x60);
    world_position.vz = (int16_t)(int)p0->pos.vz;
    world_position.pad = 0;
    (void)TransformPoints(&world_position, &projected.packed, 1);
    original.packed = projected.packed;
    alpha = cliptoscreen(projected.component);
    player_index = p0->physicsRoot.objectID;
    if (alpha > 0x6f) {
        playeronscreen[player_index] = 1;
        timeoffscreen[player_index] = 0;
        offscreenbleeps[player_index] = 0;
        return;
    }

    playeronscreen[player_index] = 0;
    timeout = offscreenbleeps[player_index] == 0
        ? 0x78000
        : 0x1e000;
    if ((gpWorld->aDolly[gpWorld->currentDolly].flags & 0x400) != 0) {
        timeoffscreen[player_index] = 0;
    }
    previous_time = timeoffscreen[player_index];
    timeoffscreen[player_index] += gGlobalFrameRate;
    if (LevelSelect != 12 &&
        timeoffscreen[player_index] > timeout) {
        if (previous_time <= timeout) {
            timeoffscreen[player_index] = 0;
            ++offscreenbleeps[player_index];
            if (offscreenbleeps[player_index] < 6) {
                (void)sound_Play(
                    &p0->vpos, 3, "xpointbp", 3);
            } else {
                /* The optimized retail body inlines this exact procedure. */
                hurtplayer(player, -255);
            }
        }
        color = UINT32_C(0xffffffff);
    }

    screen_center.vx = (int16_t)(OptionStruct.ScreenWidth >> 1);
    screen_center.vy = (int16_t)(OptionStruct.ScreenHeight >> 1);
    screen_center.vz = 0;
    screen_center.pad = 0;
    original_screen_position.vx = original.component[0];
    original_screen_position.vy = original.component[1];
    original_screen_position.vz = 0;
    original_screen_position.pad = 0;
    (void)vecsub(
        &original_screen_position, &screen_center, &direction);
    (void)normalize_svector(&direction, &direction);
    (void)vecscale(&direction, -32, &direction);

    direction_x_scaled = flexmul(direction.vx, 0x199a);
    direction_y_scaled = flexmul(direction.vy, 0x199a);
    /* Raw SAR instructions floor negative values instead of truncating. */
    direction_x_quarter = direction.vx >= 0
        ? direction.vx / 4
        : -((-(int)direction.vx + 3) / 4);
    direction_y_quarter = direction.vy >= 0
        ? direction.vy / 4
        : -((-(int)direction.vy + 3) / 4);

    clipped_x = projected.component[0];
    clipped_y = projected.component[1];
    left_x = clipped_x - direction_y_quarter + direction_x_scaled;
    left_y = clipped_y + direction_x_quarter + direction.vy;
    right_x = clipped_x + direction_y_quarter + direction_x_scaled;
    right_y = clipped_y - direction_x_quarter + direction.vy;
    center_x = (left_x + clipped_x + right_x) / 3;
    center_y = (left_y + clipped_y + right_y) / 3;

    if ((gpWorld->aDolly[gpWorld->currentDolly].flags & 0x400) != 0) {
        return;
    }
    if (trans == NULL) {
        trans = _LoadTexture(NULL, TT_SPRITE, 2);
    }
    _StartPoly(4, trans);
    opaque_color = (uint32_t)color | UINT32_C(0xff000000);
    _SetVert(
        0, (float)left_x, (float)left_y, 0.0001f,
        opaque_color, 0.0f, 0.0f);
    _SetVert(
        1, (float)center_x, (float)center_y, 0.0001f,
        0, 0.0f, 0.0f);
    point_color = color_interpolate4k(
        (uint32_t)color, UINT32_C(0xffffffff), 600);
    _SetVert(
        2, (float)clipped_x, (float)clipped_y, 0.0001f,
        point_color, 0.0f, 0.0f);
    _SetVert(
        3, (float)right_x, (float)right_y, 0.0001f,
        opaque_color, 0.0f, 0.0f);
    _EndPoly();
}

/* 0xF5610, 3 bytes, global, 2 named locals
 * scene_AspectCorrectMatrix
 * PDB type: void (MATRIX*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_AspectCorrectMatrix(MATRIX *matrix, VECTOR *position)
{
    /* The matched retail body is a three-byte return stub. */
    (void)matrix;
    (void)position;
}

/* 0xF5620, 3 bytes, global, 0 named locals
 * scene_DimScreen
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_DimScreen(void)
{
    /* The matched retail body is a three-byte return stub. */
}

/* 0xF5630, 8 bytes, global, 0 named locals
 * scene_GetRawSceneMatrix
 * PDB type: MATRIX* ()
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
MATRIX *scene_GetRawSceneMatrix(void)
{
    return &gSceneGeometryEnv.matrixRaw;
}

/* 0xF5640, 8 bytes, global, 0 named locals
 * scene_GetSceneMatrix
 * PDB type: MATRIX* ()
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
MATRIX *scene_GetSceneMatrix(void)
{
    return &gSceneGeometryEnv.matrix;
}

/* 0xF5650, 8 bytes, global, 0 named locals
 * scene_GetViewPos
 * PDB type: _svector* ()
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
_svector *scene_GetViewPos(void)
{
    return &gSceneGeometryEnv.pos;
}

/* 0xF5660, 349 bytes, global, 3 named locals
 * scene_UpdateWorld2ScreenMatrix
 * PDB type: MATRIX* (sceneGeometryEnv*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
MATRIX *scene_UpdateWorld2ScreenMatrix(sceneGeometryEnv *environment)
{
    _svector coordinate_rotation = {0x0800, 0, 0, 0};
    MATRIX conversion;

    fRotMatrix(&environment->angle, &environment->matrix);
    fRotMatrix(&coordinate_rotation, &conversion);
    fMulMatrix(&environment->matrix, &conversion);
    environment->matrixRaw = environment->matrix;
    fApplyMatrix(
        &environment->matrix, &environment->pos, &v3Translate);
    fTransMatrix(&environment->matrixRaw, &v3Translate);
    fTransMatrix(&environment->matrix, &v3Translate);

    /*
     * SetRotMatrix and SetTransMatrix both copy this complete MATRIX to the
     * exact gGTEMATRIX global. One assignment preserves their final state
     * without retaining redundant link-stub calls.
     */
    gGTEMATRIX = environment->matrix;

    fRotMatrix(&environment->angleDest, &environment->matrixDest);
    fRotMatrix(&coordinate_rotation, &conversion);
    fMulMatrix(&environment->matrixDest, &conversion);
    fApplyMatrix(
        &environment->matrixDest,
        &environment->posDest,
        &v3Translate);
    fTransMatrix(&environment->matrixDest, &v3Translate);
    return &environment->matrix;
}

/* 0xF57C0, 170 bytes, global, 6 named locals
 * scene_gCreateObject
 * PDB type: sceneObject* (char*, geomData*, ...
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */

/* 0xF5870, 90 bytes, global, 3 named locals
 * scene_gGetNewSceneObject
 * PDB type: sceneObject* (int)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
sceneObject *scene_gCreateObject(
    char *name,
    geomData *geometry,
    int id)
{
    sceneObject *scene = scene_gGetNewSceneObject(id);

    if (scene != NULL) {
        modelObject *model =
            model_gInitModelRoot(geometry, name, scene->sceneRoot.objectID);

        obj_gSetChildObject(scene, &model->modelRoot, 1);
        model->modelRoot.objectID = scene->sceneRoot.objectID;
        obj_gSetChildObject(scene, &scene->sceneRoot, 0);
        ++gCurrentSceneObject;
    }
    return scene;
}
sceneObject *scene_gGetNewSceneObject(int ID)
{
    int index;

    if (ID < 0) {
        for (index = 0; index < JPB_SCENE_CAPACITY; ++index) {
            sceneObject *scene = &maSceneData[index];

            if (scene->sceneRoot.objectID == -1) {
                scene->sceneRoot.objectID = index;
                return scene;
            }
        }
    } else if (ID < JPB_SCENE_CAPACITY &&
               maSceneData[ID].sceneRoot.objectID == -1) {
        maSceneData[ID].sceneRoot.objectID = ID;
        return &maSceneData[ID];
    }
    return NULL;
}

/* 0xF58D0, 67 bytes, global, 4 named locals
 * scene_gGetSceneModelMatrix
 * PDB type: void (int, _svector*, VECTOR*, V...
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gGetSceneModelMatrix(
    int id,
    _svector *angle,
    VECTOR *position,
    VECTOR *snapshot_position)
{
    sceneObject *scene = &maSceneData[id];

    (void)angle;
    position->vx = scene->v3WorldPosition.vx;
    position->vy = scene->v3WorldPosition.vy;
    position->vz = scene->v3WorldPosition.vz;
    snapshot_position->vx = scene->v3SnapShotPosition.vx;
    snapshot_position->vy = scene->v3SnapShotPosition.vy;
    snapshot_position->vz = scene->v3SnapShotPosition.vz;
}

/* 0xF5920, 115 bytes, global, 4 named locals
 * scene_gGetSceneModelMatrixFV
 * PDB type: void (int, _svector*, FVECTOR*, ...
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gGetSceneModelMatrixFV(
    int id,
    _svector *pv3Angle,
    FVECTOR *pv3Position,
    FVECTOR *pv3SnapShotPosition)
{
    sceneObject *scene = &maSceneData[id];

    (void)pv3Angle;
    pv3Position->vx =
        (float)(int32_t)scene->v3WorldPosition.vx;
    pv3Position->vy =
        (float)(int32_t)scene->v3WorldPosition.vy;
    pv3Position->vz =
        (float)(int32_t)scene->v3WorldPosition.vz;
    pv3SnapShotPosition->vx =
        (float)scene->v3SnapShotPosition.vx;
    pv3SnapShotPosition->vy =
        (float)scene->v3SnapShotPosition.vy;
    pv3SnapShotPosition->vz =
        (float)scene->v3SnapShotPosition.vz;
}

/* 0xF59A0, 33 bytes, global, 2 named locals
 * scene_gGetSnapShotPosition
 * PDB type: void (int, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gGetSnapShotPosition(int id, VECTOR *position)
{
    position->vx = maSceneData[id].v3SnapShotPosition.vx;
    /* Retail deliberately leaves vy and pad untouched. */
    position->vz = maSceneData[id].v3SnapShotPosition.vz;
}

/* 0xF59D0, 69 bytes, global, 0 named locals
 * scene_gInitRoot
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */

/* 0xF5A20, 196 bytes, global, 1 named locals
 * scene_gInitScenes
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gInitRoot(void)
{
    gSCENE_READY = 0;
    memset(&gSceneRoot, 0, sizeof(gSceneRoot));
    camera_SetViewType(0x901);
    gSceneRoot.paSceneModels = maSceneData;
    /* Retail's final DrawSync(0) target is itself a three-byte return stub. */
}
/*
 * Exact pool-reset block from scene_gInitScenes, RVAs 0xF5A24..0xF5A90.
 * The remaining original routine registers three developer-console commands;
 * that console integration stays pending rather than pulling placeholder
 * callbacks into the portable gameplay core.
 */
void jpb_SceneInitPool(int start)
{
    int index;

    if ((uint32_t)start >= JPB_SCENE_CAPACITY) {
        return;
    }
    for (index = start; index < JPB_SCENE_CAPACITY; ++index) {
        memset(&maSceneData[index], 0, sizeof(maSceneData[index]));
        maSceneData[index].sceneRoot.objectID = -1;
    }
}

void scene_gInitScenes(int start)
{
    jpb_SceneInitPool(start);
    (void)console_AddCommand(
        "cameras", "cam", console_CamerasCommand);
    (void)console_AddCommand(
        "anim", "anim", console_AnimCommand);
    (void)console_AddCommand(
        "enemy", "enemy", console_EnemyCommand);
}

/* 0xF5AF0, 11 bytes, global, 2 named locals
 * scene_gProject2Screen
 * PDB type: void (_svector*, int*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gProject2Screen(_svector *position, int *screen)
{
    (void)TransformPoints(position, screen, 1);
}

/* 0xF5B00, 22 bytes, global, 2 named locals
 * scene_gSetSceneModelKeyFrame
 * PDB type: void (int, _animFrame*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gSetSceneModelKeyFrame(
    int id, _animFrame *pKeyFrame)
{
    maSceneData[id].pKeyFrameModel = pKeyFrame;
}

/* 0xF5B20, 128 bytes, global, 3 named locals
 * scene_gSetSceneModelMatrix
 * PDB type: void (int, _svector*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gSetSceneModelMatrix(
    int id,
    _svector *angle,
    VECTOR *position)
{
    sceneObject *scene = &maSceneData[id];

    UpdatePublicVars(&maPhysicsData[id]);
    scene->v3WorldAngle.vx = angle->vx;
    scene->v3WorldAngle.vy = angle->vy;
    scene->v3WorldAngle.vz = angle->vz;
    scene->v3WorldPosition.vx = (int16_t)position->vx;
    scene->v3WorldPosition.vy = (int16_t)position->vy;
    scene->v3WorldPosition.vz = (int16_t)position->vz;
}

/* 0xF5BA0, 137 bytes, global, 3 named locals
 * scene_gSetSceneModelMatrixFV
 * PDB type: void (int, VECTOR*, FVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gSetSceneModelMatrixFV(
    int index, VECTOR *angle, FVECTOR *position)
{
    sceneObject *scene = &maSceneData[index];

    UpdatePublicVars(&maPhysicsData[index]);
    scene->v3WorldAngle.vx = (int16_t)angle->vx;
    scene->v3WorldAngle.vy = (int16_t)angle->vy;
    scene->v3WorldAngle.vz = (int16_t)angle->vz;
    scene->v3WorldPosition.vx =
        (int16_t)(int32_t)position->vx;
    scene->v3WorldPosition.vy =
        (int16_t)(int32_t)position->vy;
    scene->v3WorldPosition.vz =
        (int16_t)(int32_t)position->vz;
}

/* 0xF5C30, 134 bytes, global, 3 named locals
 * scene_gSetSceneModelMatrixLV
 * PDB type: void (int, VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gSetSceneModelMatrixLV(
    int id,
    VECTOR *angle,
    VECTOR *position)
{
    sceneObject *scene = &maSceneData[id];

    UpdatePublicVars(&maPhysicsData[id]);
    scene->v3WorldAngle.vx = (int16_t)angle->vx;
    scene->v3WorldAngle.vy = (int16_t)angle->vy;
    scene->v3WorldAngle.vz = (int16_t)angle->vz;
    scene->v3WorldPosition.vx = (int16_t)position->vx;
    scene->v3WorldPosition.vy = (int16_t)position->vy;
    scene->v3WorldPosition.vz = (int16_t)position->vz;
}

/* 0xF5CC0, 3 bytes, global, 1 named locals
 * scene_gSetStrobe
 * PDB type: void (CVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gSetStrobe(CVECTOR *color)
{
    /* The matched retail body is a three-byte return stub. */
    (void)color;
}

/* 0xF5CD0, 62 bytes, global, 2 named locals
 * scene_gSetWorldPosition
 * PDB type: void (int, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_gSetWorldPosition(int id, VECTOR *position)
{
    sceneObject *scene = &maSceneData[id];
    int16_t x = (int16_t)position->vx;
    int16_t y = (int16_t)position->vy;
    int16_t z = (int16_t)position->vz;

    scene->v3WorldPosition.vx = x;
    scene->v3SnapShotPosition.vx = x;
    scene->v3WorldPosition.vy = y;
    scene->v3SnapShotPosition.vy = y;
    scene->v3WorldPosition.vz = z;
    scene->v3SnapShotPosition.vz = z;
}

/* 0xF5D10, 1408 bytes, global, 9 named locals
 * scene_middleRender
 * PDB type: void (MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */

void jpb_SceneSetMiddleRenderHooks(
    const JPBSceneMiddleRenderHooks *hooks,
    void *user_data)
{
    if (hooks == NULL) {
        memset(
            &scene_middle_render_hooks,
            0,
            sizeof(scene_middle_render_hooks));
        scene_middle_render_user_data = NULL;
        return;
    }
    scene_middle_render_hooks = *hooks;
    scene_middle_render_user_data = user_data;
}

static int scene_middle_render_simulation_enabled(void)
{
    return gSCENE_READY == 0 ||
        initialLevelPauseDelay < 2 ||
        (GameStruct.GameState & UINT32_C(0x02000000)) == 0;
}

static float scene_collision_frustum_percent(void)
{
    switch ((uint8_t)LevelSelect) {
    case 2:
        return 300.0f;
    case 5:
        return isTatoMaul == 0 ? 85.0f : 120.0f;
    case 15:
        return 120.0f;
    default:
        return 100.0f;
    }
}

static void scene_run_level_owner(void)
{
    int level = (uint8_t)LevelSelect;
    int argument0 = 0;
    int argument1 = 0;
    int argument2 = 0;

    switch (level) {
    case 1:
        level_Fed();
        return;
    case 3:
        level_Theed();
        return;
    case 4:
        level_Palace();
        return;
    case 6:
        level_Corus();
        return;
    case 9:
        level_Hangar();
        return;
    case 11:
        argument0 = 150;
        argument1 = 21;
        break;
    case 14:
        level_CountDown(0, 100, 0);
        return;
    case 16:
        level_CountDown(0, 0, 10500);
        return;
    case 17:
        level_CountDown(35, 0, 12500);
        return;
    case 18:
        level_CountDown(50, 0, 0);
        return;
    case 19:
        level_CountDown(0, 15, 0);
        return;
    case 20:
        level_CountDown(55, 20, 0);
        return;
    case 21:
        level_CountDown(65, 20, 0);
        return;
    case 22:
        level_CountDown(185, 45, 0);
        return;
    case 25:
        level_Arena();
        return;
    default:
        break;
    }
    if (scene_middle_render_hooks.levelOwner != NULL) {
        scene_middle_render_hooks.levelOwner(
            scene_middle_render_user_data,
            level,
            argument0,
            argument1,
            argument2);
    }
}

void scene_middleRender(MATRIX *matrix)
{
    MATRIX *view;
    _svector world_position;
    int simulation_enabled;

    (void)matrix;
    ++totalframes;
    globaltimer += gGlobalFrameRate >= 0
        ? gGlobalFrameRate / 0x800
        : -((-gGlobalFrameRate) / 0x800);
    if (gSCENE_READY == 0) {
        gCamera.viewType &= ~UINT32_C(0x1000);
    }

    camera_Camera2ViewVector(&gCamera, &gSceneGeometryEnv);
    view = scene_UpdateWorld2ScreenMatrix(&gSceneGeometryEnv);
    CameraMatrix = *view;
    camera_GetLocation(&cameraposition);
    twatcameramatrix(view, &twattedcameramatrix);
    buildfrustrum(
        &twattedcameramatrix,
        clippingfrustrum,
        &cameraposition,
        115.0f,
        320.0f,
        180.0f);
    buildfrustrum(
        &twattedcameramatrix,
        collisionfrustrum,
        &cameraposition,
        scene_collision_frustum_percent(),
        320.0f,
        180.0f);
    isTatoMaul = 0;

    simulation_enabled =
        scene_middle_render_simulation_enabled();
    if (simulation_enabled) {
        anim_ProcessAnimations();
    }
    if (scene_middle_render_hooks.afterAnimations != NULL) {
        scene_middle_render_hooks.afterAnimations(
            scene_middle_render_user_data, view);
    }

    game_DisplayOverlay();
    if (gSTROBE_MODE < 2) {
        PushMatrix();
        cube_NewWorldRender(view);
        if (scene_middle_render_hooks.afterWorld != NULL) {
            scene_middle_render_hooks.afterWorld(
                scene_middle_render_user_data, view);
        }
        PopMatrix();
    } else {
        prim_gSetBkColor(mStrobe.b, mStrobe.g, mStrobe.r);
    }

    PushMatrix();
    if (scene_middle_render_hooks.renderModels != NULL) {
        scene_middle_render_hooks.renderModels(
            scene_middle_render_user_data, view);
    } else {
        render_RenderScene();
    }
    PopMatrix();

    player_HandleSabre();
    if (simulation_enabled) {
        if (scene_middle_render_hooks.beforePlayerProcess != NULL) {
            scene_middle_render_hooks.beforePlayerProcess(
                scene_middle_render_user_data, view);
        }
        player_gProcessPlayers();
    }
    pwrup_CheckPowerUps();
    sprite_SpriteWork(view);
    if (simulation_enabled) {
        enemy_HandleEnemies();
    }
    _HandleBackDrop();

    if (gpWorld != NULL) {
        world_position.vx = (int16_t)gpWorld->location.vx;
        world_position.vy = (int16_t)gpWorld->location.vy;
        world_position.vz = (int16_t)gpWorld->location.vz;
        world_position.pad = 0;
        (void)TransformPoints(
            &world_position, &screenworldpos, 1);
    }

    if (gSCENE_READY != 0 && initialLevelPauseDelay < 2) {
        ++initialLevelPauseDelay;
    }
    if (simulation_enabled) {
        ProcessPhysicsObjects();
    }
    scene_run_level_owner();
}

/* 0xF6290, 128 bytes, global, 1 named locals
 * scene_postRender
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_postRender(void)
{
    if ((GameStruct.GameState & UINT32_C(0x02000000)) == 0) {
        gGlobalTimer += (uint32_t)flexmul(gGlobalFrameRate, 0x200);
    }
    mDrawingSurfaceId ^= 1;
    if (totalframes > 1) {
        gSCENE_READY = 1;
    }
    --gSTROBE_MODE;
    if (gSTROBE_MODE < 0) {
        if (gpWorld != NULL) {
            prim_gSetBkColor(
                gpWorld->bkColor.b >> 2,
                gpWorld->bkColor.g >> 2,
                gpWorld->bkColor.r >> 2);
        }
        gSTROBE_MODE = 0;
    }
}

/* 0xF6310, 3 bytes, global, 1 named locals
 * scene_preRender
 * PDB type: void (MATRIX**)
 * Source: W:\SWJediPowerBattles\Work\scene.c
 */
void scene_preRender(MATRIX **matrix)
{
    /* The matched retail body is a three-byte return stub. */
    (void)matrix;
}
