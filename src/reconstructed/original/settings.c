/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\settings.c.
 * PDB module: 0077
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\settings.obj
 * Primary source: W:\SWJediPowerBattles\work\settings.c
 * Compiler language: c
 * Emitted procedures: 2
 *
 * Both emitted procedures have reviewed bodies. ai_InitPlayer preserves the
 * complete executable switch, its PDB-named collision tables, callback-table
 * selections, model/physics mutations, and shared movement-setting tail.
 */

#include "jpb/collision.h"
#include "jpb/combo.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/settings.h"
#include "jpb/world.h"

/* Exact linked globals at matched-PC RVAs 0x4CC100 and 0x4CC150. */
CollisionData maGunganNodeSizes[19] = {
    {0x20, 0x00, -1}, {0x00, 0x01, -1},
    {0x10, 0x02, -1}, {0x20, 0x03, -1},
    {0x00, 0x04, -1}, {0x10, 0x05, -1},
    {0x20, 0x06, -1}, {0x00, 0x07, -1},
    {0x20, 0x08, -1}, {0x00, 0x09, -1},
    {0x18, 0x0a, -1}, {0x18, 0x0b, -1},
    {0x10, 0x0c, -1}, {0x00, 0x0d, -1},
    {0x18, 0x0e, -1}, {0x20, 0x0f, -1},
    {0x60, 0x18, 0x0c}, {0x60, 0x19, 0x0c},
    {0x60, 0x1a, 0x0c}
};

CollisionData maGunchiefNodeSizes[19] = {
    {0x20, 0x00, -1}, {0x00, 0x01, -1},
    {0x10, 0x02, -1}, {0x20, 0x03, -1},
    {0x00, 0x04, -1}, {0x10, 0x05, -1},
    {0x20, 0x06, -1}, {0x00, 0x07, -1},
    {0x20, 0x08, -1}, {0x00, 0x09, -1},
    {0x18, 0x0a, -1}, {0x18, 0x0b, -1},
    {0x10, 0x0c, -1}, {0x00, 0x0d, -1},
    {0x18, 0x0e, -1}, {0x20, 0x0f, -1},
    {0x20, 0x17, 0x0c}, {0x20, 0x18, 0x0c},
    {0x20, 0x19, 0x0c}
};

/* Exact module-local collision tables in PDB address order. */
static CollisionData maSmallNodeSizes[1] = {
    {0x20, 0x00, -1}
};

/* Exact 22-record fallback combo block at matched-PC RVA 0x4CC1A0. */
static Combo combos[22] = {
    {.Index = 0x40, .comboFlags = UINT32_C(0x00290000),
     .String = "n"},
    {.Index = 0x41, .comboFlags = UINT32_C(0x00290000),
     .String = "s"},
    {.Index = 0x40, .comboFlags = UINT32_C(0x00290000),
     .String = "w"},
    {.Len = -1}
};

static CollisionData maLargeNodeSizes[1] = {
    {0x40, 0x00, -1}
};

static CollisionData maBoulderNodeSizes[1] = {
    {0x100, 0x00, -1}
};

/*
 * Direct local symbol maNodeSizes at matched-PC RVA 0x4CC6D0.
 * ai_InitPlayer selects this 19-entry profile by default, including for
 * Obi-Wan (model 0), protocol (model 12), pilot (model 15), battle_d
 * (model 17), and security (model 62).
 */
static CollisionData maNodeSizes[19] = {
    {0x20, 0x00, -1},
    {0x00, 0x01, -1},
    {0x10, 0x02, -1},
    {0x20, 0x03, -1},
    {0x00, 0x04, -1},
    {0x10, 0x05, -1},
    {0x20, 0x06, -1},
    {0x00, 0x07, -1},
    {0x20, 0x08, -1},
    {0x00, 0x09, -1},
    {0x18, 0x0a, -1},
    {0x18, 0x0b, -1},
    {0x10, 0x0c, -1},
    {0x00, 0x0d, -1},
    {0x18, 0x0e, -1},
    {0x20, 0x0f, -1},
    {0x10, 0x11, 0x0c},
    {0x20, 0x12, 0x0c},
    {0x20, 0x13, 0x0c}
};

/*
 * Remaining exact module-local collision tables. Their PDB spellings are
 * preserved, including maDestroyeNodeSizes.
 */
static CollisionData maBD_StapNodeSizes[1] = {
    {0x400, 0x00, -1}
};

static CollisionData maMaul_DNodeSizes[22] = {
    {0x20, 0x00, -1}, {0x00, 0x01, -1},
    {0x10, 0x02, -1}, {0x20, 0x03, -1},
    {0x00, 0x04, -1}, {0x10, 0x05, -1},
    {0x20, 0x06, -1}, {0x00, 0x07, -1},
    {0x20, 0x08, -1}, {0x00, 0x09, -1},
    {0x18, 0x0a, -1}, {0x18, 0x0b, -1},
    {0x10, 0x0c, -1}, {0x00, 0x0d, -1},
    {0x18, 0x0e, -1}, {0x20, 0x0f, -1},
    {0x10, 0x11, 0x0c}, {0x20, 0x12, 0x0c},
    {0x20, 0x13, 0x0c}, {0x10, 0x13, 0x0c},
    {0x20, 0x16, 0x0c}, {0x20, 0x17, 0x0c}
};

static CollisionData maProbeNodeSizes[2] = {
    {0x20, 0x00, -1}, {0x20, 0x03, -1}
};

static CollisionData maAATNodeSizes[9] = {
    {0x110, 0x00, -1}, {0x1a8, 0x01, -1},
    {0x138, 0x02, -1}, {0x198, 0x03, -1},
    {0x140, 0x04, -1}, {0x100, 0x0c, -1},
    {0x100, 0x0f, -1}, {0x080, 0x13, -1},
    {0x080, 0x14, -1}
};

static CollisionData maBoxNodeSizes[1] = {
    {0x40, 0x01, -1}
};

static CollisionData maLoaderNodeSizes[9] = {
    {0x100, 0x00, -1}, {0x100, 0x01, -1},
    {0x080, 0x02, -1}, {0x100, 0x03, -1},
    {0x100, 0x04, -1}, {0x080, 0x05, -1},
    {0x100, 0x06, -1}, {0x100, 0x07, -1},
    {0x040, 0x0a, -1}
};

static CollisionData maDroid_FNodeSizes[6] = {
    {0x200, 0x00, -1}, {0x100, 0x12, -1},
    {0x100, 0x03, -1}, {0x100, 0x07, -1},
    {0x100, 0x10, -1}, {0x100, 0x0c, -1}
};

static CollisionData maTurret_DNodeSizes[6] = {
    {0x200, 0x00, -1}, {0x100, 0x02, -1},
    {0x100, 0x04, -1}, {0x100, 0x06, -1},
    {0x100, 0x08, -1}, {0x100, 0x09, -1}
};

static CollisionData maMttNodeSizes[8] = {
    {0x100, 0x00, -1}, {0x100, 0x05, -1},
    {0x100, 0x06, -1}, {0x100, 0x0a, -1},
    {0x100, 0x0b, -1}, {0x100, 0x0e, -1},
    {0x0c0, 0x0f, -1}, {0x0c0, 0x10, -1}
};

static CollisionData maFulumpNodeSizes[2] = {
    {0x0aa, 0x00, -1}, {0x100, 0x0e, -1}
};

static CollisionData maPlant_2NodeSizes[4] = {
    {0x080, 0x00, -1}, {0x080, 0x03, -1},
    {0x080, 0x05, -1}, {0x100, 0x19, -1}
};

static CollisionData maRoachNodeSizes[2] = {
    {0x040, 0x00, -1}, {0x080, 0x03, -1}
};

static CollisionData maDestroyeNodeSizes[8] = {
    {0x20, 0x00, -1},
    {0x40, 0x03, -1},
    {0x40, 0x06, -1},
    {0x40, 0x09, -1},
    {0x40, 0x0b, -1},
    {0x40, 0x0c, -1},
    {0x40, 0x0e, -1},
    {0x40, 0x12, -1}
};

static int settings_apply_player_profile(
    playerObject *player,
    CollisionData *node_sizes,
    int node_count,
    int scale_x,
    int scale_y,
    int scale_z,
    int clip_radius,
    int minimum_closing_distance,
    int mass,
    int height,
    JPBPlayerCallback callback)
{
    sceneObject *scene;
    modelObject *model;
    physicsObject *physics;

    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return 0;
    }
    scene = (sceneObject *)player->playerRoot.pParent;
    if (scene->pModel == NULL ||
        scene->pPhysics == NULL) {
        return 0;
    }
    model = (modelObject *)scene->pModel;
    physics = (physicsObject *)scene->pPhysics;

    model->v3Scale.vx = scale_x;
    model->v3Scale.vy = scale_y;
    model->v3Scale.vz = scale_z;
    model->clipradius = (int16_t)-clip_radius;
    player->fScale = model->v3Scale.vx;
    player->paNodesSizes = node_sizes;
    player->numCollisionNodes = node_count;
    player->pMainCallBack = callback;
    player->pSettings.minClosingDist =
        (int16_t)minimum_closing_distance;
    physics->radius =
        player->pSettings.minClosingDist;
    physics->mass = (int16_t)mass;
    physics->height = (int16_t)height;
    player->pSettings.JumpVel = 0x7a;
    player->pSettings.RunningJumpVel = 0x73;
    player->pSettings.dblJumpVel = 0x73;
    player->pSettings.JumpAngle = 0x2f9;
    player->pSettings.RunningJumpAngle = 0x341;
    player->pSettings.dblJumpAngle = 0x35a;
    player->pSettings.bkJumpAngle = 0x555;
    player->pSettings.gravity = UINT16_C(0xe314);
    player->paCombos = combos;
    return 1;
}

int jpb_ai_ApplyDefaultPlayerSettings(
    playerObject *player)
{
    /*
     * Exact ai_InitPlayer default branch at RVAs
     * 0xF69B2..0xF69D0, followed by the common stores at
     * 0xF6FA1..0xF702C. Slot 33 is exact ai_Main through the portable
     * callback-signature adapter documented in ai.h.
     */
    return settings_apply_player_profile(
        player,
        maNodeSizes,
        19,
        0x78a,
        0x78a,
        0x78a,
        0x78,
        0x33,
        0x800,
        200,
        funcArray[33]);
}

/* 0xF6320, 3 bytes, global, 1 named locals
 * ai_InitModelData
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\work\settings.c
 */
void ai_InitModelData(playerObject *pPlayer)
{
    (void)pPlayer;
}

/* 0xF6330, 3822 bytes, global, 10 named locals
 * ai_InitPlayer
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\work\settings.c
 */
int ai_InitPlayer(playerObject *pPlayer)
{
    sceneObject *scene =
        (sceneObject *)pPlayer->playerRoot.pParent;
    modelObject *model = (modelObject *)scene->pModel;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    CollisionData *node_sizes = maNodeSizes;
    int node_count = 19;
    int scale_x = 0x78a;
    int scale_y = 0x78a;
    int scale_z = 0x78a;
    int clip_radius = 0x78;
    int minimum_closing_distance = 0x33;
    int mass = 0x800;
    int height = 200;
    int callback_index = 33;

    switch (pPlayer->playerID) {
    case 10:
    case 79:
        node_sizes = maGunganNodeSizes;
        mass = 0x7fff;
        callback_index = 38;
        break;
    case 11:
    case 16:
    case 58:
    case 59:
        mass = 0x7fff;
        break;
    case 19:
        scale_x = scale_y = scale_z = 0x96c;
        mass = 0x2000;
        minimum_closing_distance = 0x40;
        break;
    case 22:
        scale_x = scale_z = 0xd31;
        scale_y = 0xb4f;
        mass = 0x7fff;
        minimum_closing_distance = 0x40;
        break;
    case 24:
        node_sizes = maBD_StapNodeSizes;
        node_count = 1;
        clip_radius = 0x100;
        minimum_closing_distance = 0x66;
        mass = 0x7fff;
        height = 0x200;
        break;
    case 25:
    case 33:
        node_sizes = maFulumpNodeSizes;
        node_count = 2;
        minimum_closing_distance = 0x55;
        mass = 0x7fff;
        height = 0x160;
        callback_index = 34;
        break;
    case 26:
        node_sizes = maDestroyeNodeSizes;
        node_count = 8;
        mass = 0xc00;
        height = 0xe0;
        callback_index = 44;
        break;
    case 29:
        node_sizes = maTurret_DNodeSizes;
        node_count = 6;
        scale_x = scale_y = scale_z = 0xb4f;
        minimum_closing_distance = 0x99;
        mass = 0x7fff;
        height = 400;
        model->flags |= UINT32_C(0x00000008);
        pPlayer->pFlags |= UINT32_C(0x00002000);
        break;
    case 30:
        node_sizes = maLoaderNodeSizes;
        node_count = 9;
        scale_x = scale_y = scale_z = 0xd31;
        minimum_closing_distance = 0x66;
        mass = 0x7fff;
        height = 0x100;
        model->flags |= UINT32_C(0x00000008);
        pPlayer->pFlags |= UINT32_C(0x00002000);
        callback_index = 42;
        break;
    case 31:
        node_sizes = maMttNodeSizes;
        node_count = 8;
        scale_x = scale_y = scale_z = 0x25b2;
        minimum_closing_distance = 0x133;
        mass = 0x7fff;
        height = 0x200;
        model->flags |= UINT32_C(0x00000008);
        pPlayer->pFlags |= UINT32_C(0x00002000);
        callback_index = 43;
        break;
    case 32:
        node_sizes = maPlant_2NodeSizes;
        node_count = 4;
        minimum_closing_distance = 0x80;
        mass = 0x7fff;
        height = 0x200;
        break;
    case 34:
        node_sizes = maGunchiefNodeSizes;
        scale_x = scale_y = scale_z = 0x96c;
        minimum_closing_distance = 0x4c;
        mass = 0x7fff;
        break;
    case 35:
        node_sizes = maAATNodeSizes;
        node_count = 9;
        scale_x = scale_y = scale_z = 0x608;
        clip_radius = 0x200;
        minimum_closing_distance = 0x120;
        mass = 0x7fff;
        height = 0x240;
        callback_index = 45;
        break;
    case 38:
    case 39:
        node_sizes = maProbeNodeSizes;
        node_count = 2;
        scale_x = scale_y = scale_z = 0x96c;
        mass = 0x400;
        break;
    case 40:
        node_sizes = maDesert_BNodeSizes;
        node_count = 4;
        scale_x = scale_y = scale_z = 0xf14;
        minimum_closing_distance = 0xcc;
        mass = 0x7fff;
        height = 0x200;
        model->flags |= UINT32_C(0x00000008);
        pPlayer->pFlags |= UINT32_C(0x00002000);
        physics->flags |= UINT32_C(0x00800000);
        callback_index = 41;
        break;
    case 41:
        node_sizes = maTurret_DNodeSizes;
        node_count = 6;
        scale_x = scale_y = scale_z = 0xb4f;
        minimum_closing_distance = 0x99;
        mass = 0x7fff;
        height = 400;
        model->flags |= UINT32_C(0x00000008);
        pPlayer->pFlags |= UINT32_C(0x00002000);
        callback_index = 46;
        break;
    case 9:
        mass = 0x7fff;
        callback_index = 37;
        break;
    case 43:
        node_sizes = maMaul_DNodeSizes;
        node_count = 22;
        mass = 0x7fff;
        callback_index = 37;
        break;
    case 44:
        node_sizes = maFulumpNodeSizes;
        node_count = 2;
        minimum_closing_distance = 0x55;
        mass = 0x7fff;
        height = 0x160;
        break;
    case 45:
        node_sizes = maSmallNodeSizes;
        node_count = 1;
        minimum_closing_distance = 0x0c;
        mass = 0x200;
        height = 0x32;
        break;
    case 46:
        if (LevelSelect == 2 &&
            (gpWorld->aDolly[gpWorld->currentDolly].flags &
             UINT32_C(0x00000400)) == 0 &&
            gHidePikobisModel != 0) {
            if ((pPlayer->forceFlags &
                 UINT32_C(0x80000000)) == 0) {
                pPlayer->forceFlags |= UINT32_C(0x00000080);
            }
            model->flags |= UINT32_C(0x00000010);
            gHidePikobisModel = 0;
        }
        node_sizes = maSmallNodeSizes;
        node_count = 1;
        minimum_closing_distance = 0x19;
        height = 100;
        break;
    case 47:
        node_sizes = maDroid_FNodeSizes;
        node_count = 6;
        scale_x = scale_y = scale_z = 0xf14;
        minimum_closing_distance = 0xcc;
        mass = 0x7fff;
        height = 0x200;
        model->flags |= UINT32_C(0x00000008);
        pPlayer->pFlags |= UINT32_C(0x00002000);
        callback_index = 40;
        break;
    case 49:
        mass = 0x2000;
        break;
    case 51:
        scale_x = scale_y = scale_z = 0xb4f;
        minimum_closing_distance = 0x4c;
        mass = 0x7fff;
        callback_index = 35;
        break;
    case 53:
    case 56:
        node_sizes = maGunganNodeSizes;
        mass = 0xc00;
        break;
    case 57:
        minimum_closing_distance = 0x26;
        mass = 0x400;
        height = 100;
        break;
    case 67:
    case 68:
    case 69:
        node_sizes = maLargeNodeSizes;
        node_count = 0;
        scale_x = scale_y = scale_z = 0x1a63;
        clip_radius = 0x200;
        minimum_closing_distance = 0x100;
        mass = 0x7fff;
        height = -0x80;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        break;
    case 70:
        node_sizes = maLargeNodeSizes;
        node_count = 1;
        scale_x = scale_y = scale_z = 0x25b2;
        clip_radius = 0x200;
        minimum_closing_distance = 0x200;
        mass = 0x7fff;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        break;
    case 72:
        node_sizes = NULL;
        node_count = 0;
        scale_x = scale_y = scale_z = 0x1e2;
        minimum_closing_distance = 0;
        model->flags |= UINT32_C(0x00000010);
        break;
    case 73:
        node_sizes = maBoulderNodeSizes;
        node_count = 0;
        minimum_closing_distance = 0;
        mass = 0x7fff;
        callback_index = 47;
        break;
    case 74:
        node_sizes = maTurret_DNodeSizes;
        node_count = 6;
        scale_x = scale_y = scale_z = 0x486;
        break;
    case 75:
        node_sizes = maBoulderNodeSizes;
        node_count = 1;
        scale_x = scale_y = scale_z = 0x96c;
        minimum_closing_distance = 0x10;
        mass = 0x7fff;
        height = 0x40;
        break;
    case 76:
        scale_x = scale_y = scale_z = 0x2d3c;
        minimum_closing_distance = 0x400;
        mass = 0x7fff;
        height = 0x10;
        model->flags |= UINT32_C(0x0000000a);
        callback_index = 36;
        break;
    case 77:
        node_sizes = maWormNodeSizes;
        node_count = 6;
        scale_x = scale_y = scale_z = 0xd31;
        minimum_closing_distance = 0x66;
        mass = 0x7fff;
        height = 0x200;
        model->flags |= UINT32_C(0x00000008);
        pPlayer->pFlags |= UINT32_C(0x00002000);
        physics->flags |= UINT32_C(0x00800000);
        callback_index = 39;
        break;
    case 78:
        node_sizes = maRoachNodeSizes;
        node_count = 2;
        scale_x = scale_y = scale_z = 0x169e;
        minimum_closing_distance = 0x10;
        mass = 0x400;
        height = 0x40;
        break;
    case 86:
        scale_x = scale_y = scale_z = 0x169e;
        minimum_closing_distance = 0;
        mass = 0x7fff;
        height = 400;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        coll_SetNodeZBufferOffset(
            pPlayer->playerRoot.objectID, 0, 8);
        break;
    case 87:
        node_sizes = maLargeNodeSizes;
        node_count = 1;
        scale_x = scale_y = scale_z = 0x10f6;
        minimum_closing_distance = 0xd8;
        mass = 0x7fff;
        height = 0x168;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        break;
    case 89:
        minimum_closing_distance = 0;
        mass = 0x7fff;
        height = 0x40;
        break;
    case 93:
        node_sizes = maBoxNodeSizes;
        node_count = 1;
        scale_x = scale_y = scale_z = 0x12d9;
        minimum_closing_distance = 0x38;
        mass = 0x7fff;
        height = 0x40;
        model->flags |= UINT32_C(0x00000002);
        break;
    case 94:
    case 96:
        scale_x = scale_y = scale_z = 0x12d9;
        mass = 0x7fff;
        height = 0x10;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        coll_SetNodeZBufferOffset(
            pPlayer->playerRoot.objectID, 0, 0x10);
        break;
    case 95:
        scale_x = scale_y = scale_z = 0x14bb;
        mass = 0x4000;
        height = 0x10;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        coll_SetNodeZBufferOffset(
            pPlayer->playerRoot.objectID, 0, 8);
        break;
    case 97:
        scale_x = scale_y = scale_z = 0x1c45;
        minimum_closing_distance = 0x80;
        mass = 0x7fff;
        height = 0x100;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        break;
    case 98:
        scale_x = scale_y = scale_z = 0x14bb;
        minimum_closing_distance = 0;
        mass = 0x7fff;
        height = 400;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        coll_SetNodeZBufferOffset(
            pPlayer->playerRoot.objectID, 0, 8);
        break;
    case 99:
        scale_x = scale_y = scale_z = 0x12d9;
        minimum_closing_distance = 0;
        mass = 0x7fff;
        height = 800;
        break;
    case 100:
    case 114:
        scale_x = scale_y = scale_z = 0x10f6;
        minimum_closing_distance = 0;
        mass = 0x7fff;
        height = 400;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        coll_SetNodeZBufferOffset(
            pPlayer->playerRoot.objectID, 0, 8);
        break;
    case 102:
        node_sizes = maBoxNodeSizes;
        node_count = 1;
        scale_x = scale_y = scale_z = 0xd92;
        mass = 0x7fff;
        height = 0x200;
        model->flags |= UINT32_C(0x00000008);
        break;
    case 103:
        scale_x = scale_y = scale_z = 0x5956;
        mass = 0x4000;
        height = 0x10;
        model->pRootNode->flags |= UINT32_C(0x00001000);
        model->flags |= UINT32_C(0x0000000a);
        break;
    case 105:
        scale_x = scale_y = scale_z = 0x1096;
        minimum_closing_distance = 0;
        mass = 0x7fff;
        height = 400;
        model->flags |= UINT32_C(0x0000000a);
        coll_SetNodeZBufferOffset(
            pPlayer->playerRoot.objectID, 0, 8);
        break;
    case 112:
        scale_x = scale_y = scale_z = 0x25b2;
        minimum_closing_distance = 0x66;
        mass = 0x7fff;
        height = 0x200;
        model->flags |= UINT32_C(0x00000008);
        break;
    }

    (void)settings_apply_player_profile(
        pPlayer,
        node_sizes,
        node_count,
        scale_x,
        scale_y,
        scale_z,
        clip_radius,
        minimum_closing_distance,
        mass,
        height,
        funcArray[callback_index]);

    switch (pPlayer->playerID) {
    case 9:
        pPlayer->pSettings.JumpVel = 0xb3;
        pPlayer->pSettings.RunningJumpVel = 0xc0;
        break;
    case 48:
    case 49:
    case 50:
    case 51:
        pPlayer->pSettings.JumpVel = 0x7a;
        break;
    case 75:
    case 78:
        pPlayer->pSettings.JumpVel = 0x3d;
        break;
    }
    return 1;
}

int jpb_ai_ApplyPlayerSettings(playerObject *player)
{
    sceneObject *scene;

    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return 0;
    }
    scene = (sceneObject *)player->playerRoot.pParent;
    if (scene->pModel == NULL ||
        scene->pPhysics == NULL) {
        return 0;
    }
    return ai_InitPlayer(player);
}
