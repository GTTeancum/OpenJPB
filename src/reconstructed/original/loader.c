/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\loader.c.
 *
 * All 18 emitted procedures have reviewed bodies. Platform code may observe a
 * completed enemy for renderer bookkeeping, but it neither creates the actor
 * nor changes loader_CreateEnemy's result.
 *
 * PDB module: 0051
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\loader.obj
 * Primary source: W:\SWJediPowerBattles\Work\loader.c
 * Compiler language: c
 * Emitted procedures: 18
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/loader.h"
#include "jpb/ai.h"
#include "jpb/anim.h"
#include "jpb/animctrl.h"
#include "jpb/bmd.h"
#include "jpb/bucket.h"
#include "jpb/camera.h"
#include "jpb/combo.h"
#include "jpb/debugtext.h"
#include "jpb/extracharacters.h"
#include "jpb/enemy.h"
#include "jpb/filesys.h"
#include "jpb/fx.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/jedi.h"
#include "jpb/intersec.h"
#include "jpb/menu.h"
#include "jpb/level_world.h"
#include "jpb/memory.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/pwrup.h"
#include "jpb/resources.h"
#include "jpb/scene.h"
#include "jpb/settings.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/stubs.h"
#include "jpb/texture.h"
#include "jpb/vectors.h"
#include "jpb/whook.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(JPB_UFBX_AVAILABLE)
#include "ufbx.h"
typedef ufbx_load_opts JPBUfbxLoadOpts;
typedef ufbx_error JPBUfbxError;
#else
/* Exact matched ufbx 0.6.1 ABI fields used by loader_LevelLoad. Keeping the
 * renderer dependency opaque lets the original loader remain in the static
 * game library while each platform supplies the matched ufbx implementation. */
typedef struct JPBUfbxCoordinateAxes {
    int right;
    int up;
    int front;
} JPBUfbxCoordinateAxes;

typedef struct JPBUfbxLoadOpts {
    unsigned char prefix[280];
    JPBUfbxCoordinateAxes target_axes;
    float target_unit_meters;
    unsigned char suffix[160];
} JPBUfbxLoadOpts;

typedef struct JPBUfbxString {
    const char *data;
    size_t length;
} JPBUfbxString;

typedef struct JPBUfbxError {
    int type;
    int padding;
    JPBUfbxString description;
    unsigned char suffix[592];
} JPBUfbxError;

extern ufbx_scene *ufbx_load_file(
    const char *filename,
    const JPBUfbxLoadOpts *opts,
    JPBUfbxError *error);
extern void ufbx_free_scene(ufbx_scene *scene);

_Static_assert(sizeof(JPBUfbxLoadOpts) == 456,
    "ufbx_load_opts PDB layout changed");
_Static_assert(offsetof(JPBUfbxLoadOpts, target_axes) == 280,
    "ufbx_load_opts.target_axes offset changed");
_Static_assert(offsetof(JPBUfbxLoadOpts, target_unit_meters) == 292,
    "ufbx_load_opts.target_unit_meters offset changed");
_Static_assert(sizeof(JPBUfbxError) == 616,
    "ufbx_error PDB layout changed");
_Static_assert(offsetof(JPBUfbxError, description) == 8,
    "ufbx_error.description offset changed");
#endif

/* Exact PDB globals at matched-PC RVAs 0x4BCDA0, 0x4BD2B0, and
 * 0x4BD730. modelAnimConnect's four field names and signedness come from
 * PDB type 0x6EA9; the 115 records and strings were checked byte-for-byte
 * against game.exe. */
char *sAnimNames[JPB_ANIMATION_NAME_COUNT] = {
    "jedi", "battle_d", "destroye", "gungan", "queen", "droid",
    "pilot", "goliath", "loader", "mtt", "tank", "tusken",
    "desert_b", "turret", "maul", "swamp", "droid_f", "thug",
    "peck", "beacon", "taxi", "stap", "handmaid", "worm",
    "tatooine", "fedship", "palace", "core", "corus", "ruins",
    "hangar", "obi_coun", "qui_coun", "mace_cou", "adi_coun",
    "plo_coun", "thug_3", "roach", "jarjar", "ship", "probe",
    "maul_d", "thug_4", "maul_p", "bd_kadu", "pilot_d"
};

char *sObiNames[JPB_ACTOR_NAME_COUNT] = {
    "obi", "quigon", "bith", "padme", "bugeye", "greedo", "queen",
    "panaka", "weasel", "sithjedi", "jarjar", "queenprs", "r3po",
    "r2d2", "maintdrd", "21b", "handmaid", "baron", "21bblk",
    "21bgry", "21bprp", "21bred", "21bgrn", "stap", "corhum2",
    "horns", "destroyr", "pitdroid", "domedrd", "fatso", "hovdroid",
    "mtt", "swpcr2", "kaadu", "tarpals", "tank", "tusken", "flunky",
    "gonk", "gonkblk", "jabba", "r2org1", "r2red1", "corguard",
    "eopie", "2twigrn", "ronto", "drdfitr", "corbum1", "corbum2",
    "corbum3", "corbum4", "watto", "gunggrd", "gungrd", "gngsold1",
    "gamguard", "jawa", "anakin", "nguard", "nrguard", "hunter",
    "baronsec", "pekopeko", "yakface", "tatkid", "bossnass",
    "cortaxi", "spedblu", "spedcya", "spedgrn", "sub", "pwrdrink",
    "squidhed", "whale", "powrcyl", "powrpyr", "teemtpod", "pwrfuel1",
    "jarjar", "bighead", "bartend", "bigguy", "corhum1", "c3po",
    "r2yel1", "pwrserv1", "reeyees", "pwrunit1", "shmi", "tatcrit",
    "tc14", "pwrspan1", "teemto", "twilek1", "twilek2", "corhum4",
    "pwrpist1", "senldr", "ratboy", "r3poprp", "pwrshld1", "pwrsabr1",
    "r3poblk", "r3poblu", "r3pogrn", "tripod", "pwrmass1", "pwrrckt1",
    "pwrlizrd", "pwrgkey1", "pwrepuls", "corhum3", "duros", "ishitib"
};

modelAnimConnect model_anim_table[JPB_ACTOR_NAME_COUNT] = {
    {0, 0, -1, 0}, {1, 0, -1, 0}, {2, 0, -1, 0}, {3, 0, -1, 0},
    {4, 0, -1, 0}, {5, 0, -1, 0}, {6, 0, -1, 0}, {7, 0, -1, 0},
    {8, 0, -1, 0}, {9, 14, -1, 0}, {10, 38, -1, 0}, {11, 4, 0, 0},
    {12, 5, 0, 0}, {13, 5, 0, 0}, {14, 5, 0, 0}, {15, 45, -1, 0},
    {16, 4, 0, 0}, {17, 1, -1, 0}, {18, 1, -1, 0}, {19, 1, -1, 0},
    {20, 1, -1, 0}, {21, 1, -1, 0}, {22, 1, -1, 0}, {23, 21, 0, 0},
    {24, 21, 0, 0}, {25, 44, 0, 0}, {26, 2, -1, 0}, {27, 2, -1, 0},
    {28, 2, -1, 0}, {29, 7, 0, 0}, {30, 8, -1, 0}, {31, 9, 0, 0},
    {32, 15, 32, 0}, {33, 3, -1, 0}, {34, 3, -1, 0}, {35, 10, 0, 0},
    {36, 11, -1, 0}, {37, 11, -1, 0}, {38, 40, 0, 0}, {39, 40, 0, 0},
    {40, 12, -1, 0}, {41, 13, 0, 0}, {42, 13, 0, 0}, {43, 41, -1, 0},
    {44, 15, 0, 0}, {45, 15, 8, 0}, {46, 15, 24, 0}, {47, 16, 0, 0},
    {48, 17, -1, 0}, {49, 17, -1, 0}, {50, 36, -1, 0}, {51, 42, -1, 0},
    {52, 18, 0, 0}, {53, 3, -1, 0}, {54, 3, -1, 0}, {55, 3, -1, 0},
    {56, 3, -1, 0}, {57, 24, 0, 0}, {58, 24, 56, 0}, {59, 6, -1, 0},
    {60, 6, -1, 0}, {61, 1, -1, 0}, {62, 1, -1, 0}, {63, 15, 16, 0},
    {64, 4, 0, 0}, {65, 4, 0, 0}, {66, 3, -1, 0}, {67, 20, 0, 0},
    {68, 20, 0, 0}, {69, 20, 0, 0}, {70, 20, 0, 0}, {71, 25, 32, 0},
    {72, 19, 0, 0}, {73, 24, 24, 0}, {74, 13, 0, 0}, {75, 24, 8, 0},
    {76, 27, 32, 0}, {77, 23, 0, 0}, {78, 37, 0, 0}, {79, 3, -1, 0},
    {80, 31, 0, 0}, {81, 32, 0, 0}, {82, 33, 0, 0}, {83, 34, 0, 0},
    {84, 35, 0, 0}, {85, 13, -1, 0}, {86, 25, 0, 0}, {87, 25, 8, 0},
    {88, 25, 16, 0}, {89, 15, 40, 0}, {90, 15, 48, 0}, {91, 15, 56, 0},
    {92, 24, 32, 0}, {93, 25, 24, 0}, {94, 25, 32, 0}, {95, 25, 40, 0},
    {96, 28, 0, 0}, {97, 26, 0, 0}, {98, 26, 8, 0}, {99, 24, 40, 0},
    {100, 26, 16, 0}, {101, 27, 0, 0}, {102, 27, 8, 0},
    {103, 27, 16, 0}, {104, 28, 8, 0}, {105, 30, 0, 0},
    {106, 25, 48, 0}, {107, 24, 48, 0}, {108, 27, 24, 0},
    {109, 26, 16, 0}, {110, 19, 0, 0}, {111, 29, 0, 0},
    {112, 39, 0, 0}, {113, 39, 8, 0}, {114, 26, 16, 0}
};

/*
 * Exact PDB global at matched-PC RVA 0x4BCF10. The 115 pointers and their
 * strings were checked directly against the executable through the next
 * named table at RVA 0x4BD2B0.
 */
char *sModelNames[JPB_MODEL_NAME_COUNT] = {
    "obi_wan", "qui_gon", "mace", "adi", "plo", "maul_p",
    "amidala", "panaka", "ki_adi", "maul", "jar_jar", "queen",
    "protocol", "r2", "worker", "pilot", "handmaid", "battle_d",
    "rifle", "grapple", "grap_up", "flame", "plasma", "stap",
    "bd_stap", "bd_kadu", "destroye", "destroym", "destroyu",
    "goliath", "loader", "mtt", "plant_2", "kadu", "gunchief",
    "aat", "tusken_s", "tusken_r", "probe", "probe_b", "desert_b",
    "turret_d", "turret_u", "maul_d", "fulump", "nuna", "pikobis",
    "droid_f", "thug_1", "thug_2", "thug_3", "thug_4", "peck",
    "gungan_1", "gungan_2", "gungan_s", "gungan_b", "jawa", "anakin",
    "n_pilot", "n_guard", "commandr", "security", "peko", "hombre_1",
    "chica_1", "bossnass", "taxi_1", "taxi_2", "taxi_3", "bus",
    "hype", "beacon", "sarlacc", "turret_s", "boulder", "blades",
    "worm", "roach", "jar_jar_playable", "obi_wan1", "qui_gon1",
    "mace1", "adi1", "plo1", "cannon", "fed_door", "piston", "fan",
    "spore", "mushroom", "tree", "spire", "box", "lift_1", "lift_2",
    "lift_3", "coffin", "s_door", "twister", "pal_door", "laser_h",
    "laser_v", "cor_lift", "cor_fan", "han_door", "turret", "rubble_d",
    "laser_s", "conc", "th_gate", "rubble_r", "qn_ship", "sithbike",
    "hngdr"
};

/* Exact PDB pointer arrays at matched-PC RVAs 0x538F70 and 0x5390E0. */
char *maAnimData[JPB_ANIMATION_NAME_COUNT];
char *maModelData[JPB_MODEL_NAME_COUNT];

/* Exact PDB globals at matched-PC RVAs 0x508580 and 0x10D7E28. */
int gotJPX;
ufbx_scene *scene;

_Static_assert(
    sizeof(sAnimNames) / sizeof(sAnimNames[0]) ==
        JPB_ANIMATION_NAME_COUNT,
    "sAnimNames count changed");
_Static_assert(
    sizeof(sModelNames) / sizeof(sModelNames[0]) ==
        JPB_MODEL_NAME_COUNT,
    "sModelNames count changed");
_Static_assert(
    sizeof(sObiNames) / sizeof(sObiNames[0]) ==
        JPB_ACTOR_NAME_COUNT,
    "sObiNames count changed");
_Static_assert(
    sizeof(model_anim_table) /
            sizeof(model_anim_table[0]) ==
        JPB_ACTOR_NAME_COUNT,
    "model_anim_table count changed");
_Static_assert(sizeof(modelAnimConnect) == 4,
    "modelAnimConnect layout changed");

static JPBLoaderEnemyCreatedObserver
    jpb_loader_enemy_created_observer;
static void *jpb_loader_enemy_created_user_data;
#ifdef JPB_LOADER_TESTING
static JPBLoaderEnemyCreateTestHook
    jpb_loader_enemy_create_test_hook;
static void *jpb_loader_enemy_create_test_user_data;
#endif
static char *loader_LoadJediCAD(int pnum);
static void loader_LoadJediCMB(
    int pnum, playerObject *pPlayer);
static void loader_PostCreatePlayer(playerObject *pPlayer);
static void loader_loadEnemies(unsigned level);
static geomData *loader_loadJediBMD(int pnum);

static void loader_ApplyLevelLoadSpecials(int level)
{
    switch (level) {
    case 0:
        LoadBackdrop(
            (char *)resource_getPath("jedi2a.bck", JPB_RESOURCE_BACKDROP),
            0x80);
        break;
    case 1:
    case 6:
    case 7:
    case 8:
        (void)game_gClrGameFlags(UINT32_C(0x01000000));
        break;
    case 2:
        LoadBackdrop(
            (char *)resource_getPath("trees.bck", JPB_RESOURCE_BACKDROP),
            0x80);
        break;
    case 3:
        LoadBackdrop(
            (char *)resource_getPath("theed.bck", JPB_RESOURCE_BACKDROP),
            0x80);
        (void)game_gSetGameFlags(UINT32_C(0x01000000));
        break;
    case 4:
        gpWorld->aDolly[19].flags |= UINT32_C(0x2000);
        gpWorld->aBkDolly[19].flags |= UINT32_C(0x2000);
        LoadBackdrop(
            (char *)resource_getPath("dj_pic.bck", JPB_RESOURCE_BACKDROP),
            0x80);
        break;
    case 5:
        gpWorld->aDolly[39].flags |= UINT32_C(0x2000);
        gpWorld->aBkDolly[39].flags |= UINT32_C(0x2000);
        break;
    case 9:
        (void)game_gSetGameFlags(UINT32_C(0x01000000));
        break;
    case 10:
        LoadBackdrop(
            (char *)resource_getPath("core.bck", JPB_RESOURCE_BACKDROP),
            0x80);
        break;
    case 11:
        gpWorld->aDolly[8].flags |= UINT32_C(0x2000);
        gpWorld->aBkDolly[8].flags |= UINT32_C(0x2000);
        break;
    case 12:
        gpWorld->aDolly[0].flags = 0;
        gpWorld->aDolly[0].offset.vx -= 0x52d0;
        gpWorld->aDolly[0].offset.vy -= 0x02bc;
        gpWorld->aDolly[0].offset.vz -= 0x0064;
        gpWorld->aBkDolly[0].flags = 0;
        gpWorld->aBkDolly[0].offset.vx -= 0x52d0;
        gpWorld->aBkDolly[0].offset.vy -= 0x02bc;
        gpWorld->aBkDolly[0].offset.vz -= 0x0064;
        break;
    default:
        (void)game_gClrGameFlags(UINT32_C(0x01000000));
        break;
    }
}

#ifdef JPB_LOADER_TESTING
void jpb_LoaderSetEnemyCreateTestHook(
    JPBLoaderEnemyCreateTestHook hook,
    void *user_data)
{
    jpb_loader_enemy_create_test_hook = hook;
    jpb_loader_enemy_create_test_user_data = user_data;
}

void jpb_LoaderApplyLevelLoadSpecialsForTest(int level)
{
    loader_ApplyLevelLoadSpecials(level);
}
#endif

void jpb_LoaderSetEnemyCreatedObserver(
    JPBLoaderEnemyCreatedObserver observer,
    void *user_data)
{
    jpb_loader_enemy_created_observer = observer;
    jpb_loader_enemy_created_user_data = user_data;
}

/* 0xBBD00, 14 bytes, global, 1 named locals
 * IsPlayerCharacter
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\include\brain.h
 */
int IsPlayerCharacter(playerObject *player)
{
    return player->playernum < 2;
}

/* 0xBBD10, 135 bytes, global, 5 named locals
 * getNodeByName
 * PDB type: geomData* (geomData*, long, cons...
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
geomData *getNodeByName(
    geomData *root, long bufferSize, const char *name)
{
    int i;

    for (i = 0;
         (size_t)i < (size_t)bufferSize / sizeof(*root);
         ++i) {
        geomData *node = &root[i];

        if (strstr(node->name, name) != NULL) {
            return node;
        }
    }
    return NULL;
}

/* 0xBBDA0, 48 bytes, global, 0 named locals
 * initDataArrays
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
void initDataArrays(void)
{
    memset(maAnimData, 0, sizeof(maAnimData));
    memset(maModelData, 0, sizeof(maModelData));
}

/* 0xBBDD0, 548 bytes, local, 6 named locals
 * loadPowerupModels
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */

/* 0xBC000, 169 bytes, local, 8 named locals
 * loader_CreateCharacter
 * PDB type: playerObject* (int, int, geomDat...
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
static playerObject *loader_CreateCharacter(
    int ID,
    int type,
    geomData *model,
    char *anim,
    JPBPlayerInitCallback fpPlayerInit)
{
    char *name = sModelNames[type];
    sceneObject *pScene;
    playerObject *pPlayer = NULL;

    memory_gSetDefaultMemoryType(-1);
    pScene = scene_gCreateObject(name, model, ID);
    if (pScene != NULL) {
        (void)anim_CreateObject(pScene, anim, NULL, ID);
        (void)physics_gCreateObject(pScene);
        pPlayer = player_gCreateObject(pScene, type, fpPlayerInit);
        player_gConnectMotionData(pPlayer, anim);
    }
    memory_gSetDefaultMemoryType(0);
    return pPlayer;
}

/* 0xBC0B0, 1114 bytes, global, 9 named locals
 * loader_CreateEnemy
 * PDB type: int (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
static void loadPowerupModels(void)
{
    unsigned type = 0;

    while (powerUpFiles[type] != NULL) {
        char texture_name[256];
        const char *path;
        char *file_data;
        void *payload;
        geomData *geometry;
        char *texture_path;
        char *extension;
        int32_t size = 0;
        size_t cursor;

        if (powerUpFiles[type][0] == '\0') {
            powerUpData[type] = NULL;
            ++type;
            continue;
        }
        path = resource_getPathWithExtension(
            powerUpFiles[type], JPB_RESOURCE_MODEL, "BMD");
        file_data = path != NULL
            ? file_LoadFile2PoolFunc(
                  (char *)(void *)path,
                  &size,
                  MEMORY_POOL_ANY,
                  0x420,
                  "W:\\SWJediPowerBattles\\Work\\loader.c")
            : NULL;
        if (file_data == NULL) {
            powerUpData[type] = NULL;
            ++type;
            continue;
        }
        payload = file_data + sizeof(uint32_t);
        powerUpData[type] = payload;
        geometry = (geomData *)(
            (uint8_t *)payload + sizeof(geomData));
        geometry->pVertex = addPtr(
            (uint8_t *)payload + geometry->pVertex,
            JPB_POINTER_ARRAY_VERTEX);
        geometry->pNormal = addPtr(
            (uint8_t *)payload + geometry->pNormal,
            JPB_POINTER_ARRAY_NORMAL);
        geometry->pUV = addPtr(
            (uint8_t *)payload + geometry->pUV,
            JPB_POINTER_ARRAY_UV);
        geometry->pIndex = addPtr(
            (uint8_t *)payload + geometry->pIndex,
            JPB_POINTER_ARRAY_INDEX);
        geometry->pColor = addPtr(
            (uint8_t *)payload + geometry->pColor,
            JPB_POINTER_ARRAY_COLOR);

        (void)snprintf(
            texture_name,
            sizeof(texture_name),
            "tga/%s",
            geometry->t.Texture);
        texture_path = (char *)(void *)resource_getPathWithExtension(
            texture_name, JPB_RESOURCE_MODEL, "BMP");
        extension = texture_path != NULL
            ? strstr(texture_path, ".bmp")
            : NULL;
        if (extension != NULL) {
            memcpy(extension, ".png", sizeof(".png"));
            for (cursor = 0; texture_path[cursor] != '\0'; ++cursor) {
                if (texture_path[cursor] == '-') {
                    texture_path[cursor] = '_';
                }
            }
            geometry->t.TextureID = (uint64_t)(uintptr_t)_LoadTexture(
                texture_path, (TT_TEXTYPE)8, 0);
            FixDrawPowerUp(type);
        }
        ++type;
    }
}

size_t jpb_LoaderLoadPowerupModels(void)
{
    size_t loaded = 0;
    size_t type;

    loadPowerupModels();
    for (type = 0; powerUpFiles[type] != NULL; ++type) {
        if (powerUpFiles[type][0] != '\0' &&
            powerUpData[type] != NULL) {
            geomData *geometry = (geomData *)(
                (uint8_t *)powerUpData[type] + sizeof(geomData));

            if (geometry->t.TextureID != 0) {
                ++loaded;
            }
        }
    }
    return loaded;
}

static void loader_ApplyEnemyModelSpecials(
    playerObject *player, Motion *motions, int modelID)
{
    static const uint64_t player_flag_model_mask =
        UINT64_C(0x10040000200);

    /* Retail writes these model bits at player +0x144 (forceFlags), not the
     * adjacent pFlags field at +0x140. */
    switch (modelID) {
    case 9:
        motions[86].FunctPtr = 0x18;
        player->forceFlags |= UINT32_C(0x200);
        break;
    case 0x15:
        motions[95].FunctPtr = 0x0b;
        motions[96].FunctPtr = 0x0b;
        motions[97].FunctPtr = 0x0b;
        motions[114].FunctPtr = 0x0b;
        break;
    case 0x22:
        motions[76].FunctPtr = 0x19;
        player->forceFlags |= UINT32_C(0x200);
        break;
    case 0x2b:
    case 0x2f:
    case 0x33:
    case 0x4d:
    case 0x6a:
        player->forceFlags |= UINT32_C(0x200);
        break;
    case 0x38:
        motions[76].FunctPtr = 0x19;
        break;
    default:
        if (modelID < 0x29 &&
            ((player_flag_model_mask >> modelID) & UINT64_C(1)) != 0) {
            player->forceFlags |= UINT32_C(0x200);
        }
        break;
    }
}

static int loader_FinalizeEnemyPlayer(
    playerObject *player, wsl_ENEMY *pEnemy, int modelID)
{
    wsl_BAP_PLACEMENT *pPlace = pEnemy->pPlace;
    sceneObject *scene = (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics = (physicsObject *)scene->pPhysics;
    animObject *pAnim = (animObject *)scene->pAnim;
    Motion *motions;
    int offset;
    int t;

    if (player->playerID != 0x48) {
        player->shadow = (int32_t *)(void *)sprite_GetBaseNodeMarker(
            player->playerRoot.objectID,
            player->pSettings.minClosingDist);
    } else {
        player->shadow = NULL;
    }
    physics_gSetPosition(
        &player->playerRoot,
        pPlace->loc.vx,
        pPlace->loc.vy,
        pPlace->loc.vz);
    game_gSetMaxEnergy(
        player->playerRoot.objectID, pPlace->aiDf.hitPoints);
    game_gSetEnergy(
        player->playerRoot.objectID, pPlace->aiDf.hitPoints);
    player_RefreshPlayer(player);
    physics_gSetFacing(&player->playerRoot, pPlace->aiDf.angle);

    offset = model_anim_table[modelID].poolOffset;
    if (offset < 0) {
        player->subOffset = 0;
    } else {
        pAnim->paMotions += offset;
        player->subOffset = (int16_t)-offset;
        player->paMotions = pAnim->paMotions;
        player->pFlags |= UINT32_C(0x8000);
        pAnim->animFlags |= UINT32_C(1);
        for (t = 0; t < 8; ++t) {
            player->paMotions[t].Seq = (uint16_t)t;
        }
        (void)animctrl_MotionEqualLock(
            &player->playerRoot, player->paMotions);
    }

    switch (pEnemy->movementMode) {
    case 2:
        physics->movemode = MOVE_HOVER;
        physics->flags |= UINT32_C(0x2000);
        break;
    case 3:
        physics->movemode = MOVE_HOVER3D;
        physics->flags |= UINT32_C(0x2000);
        break;
    case 4:
        physics->movemode = MOVE_FLY;
        physics->flags |= UINT32_C(0x2000);
        break;
    default:
        physics->movemode = MOVE_NORMAL;
        physics->flags &= ~UINT32_C(0x2000);
        break;
    }

    motions = player->paMotions;
    motions[2].Lock = motions[1].Lock;
    loader_ApplyEnemyModelSpecials(player, motions, modelID);
    (void)ai_ValidateData(player);
    return player->playerRoot.objectID;
}

#ifdef JPB_LOADER_TESTING
int jpb_LoaderFinalizeEnemyForTest(wsl_ENEMY *pEnemy)
{
    return loader_FinalizeEnemyPlayer(
        pEnemy->pPlayer, pEnemy, pEnemy->pPlayer->playerID);
}

void jpb_LoaderApplyEnemyModelSpecialsForTest(
    playerObject *player, Motion *motions, int model_id)
{
    loader_ApplyEnemyModelSpecials(player, motions, model_id);
}
#endif

int loader_CreateEnemy(wsl_ENEMY *pEnemy)
{
    wsl_BAP_PLACEMENT *pPlace = pEnemy->pPlace;
    playerObject *player;
    char *pModelBuffer;
    char *pAnimBuffer;
    int modelID;
    int object_id;

#ifdef JPB_LOADER_TESTING
    if (jpb_loader_enemy_create_test_hook != NULL) {
        return jpb_loader_enemy_create_test_hook(
            pEnemy, jpb_loader_enemy_create_test_user_data);
    }
#endif
    if ((unsigned)pPlace->actorNum > 20) {
        return 0;
    }
    modelID = maModelID[pPlace->actorNum][0];
    pModelBuffer = maModelData[modelID];
    if (pModelBuffer == NULL) {
        (void)debug_printf("loader_CreateEnemy: model not loaded\n");
        return 0;
    }
    pAnimBuffer = maAnimData[model_anim_table[modelID].poolID];
    if (pAnimBuffer == NULL) {
        (void)debug_printf("loader_CreateEnemy: anim not loaded\n");
        return 0;
    }
    player = loader_CreateCharacter(
        -1,
        modelID,
        (geomData *)(void *)pModelBuffer,
        pAnimBuffer,
        ai_InitPlayer);
    if (player == NULL) {
        return 0;
    }

    player->target = gpWorld->player0;
    player->pEnemy = pEnemy;
    pEnemy->pPlayer = player;
    player->paiMemory = ai_GetAIHandle(
        maModelID[pPlace->actorNum][1], pEnemy->aiLevel);
    object_id = loader_FinalizeEnemyPlayer(player, pEnemy, modelID);
    if (jpb_loader_enemy_created_observer != NULL) {
        jpb_loader_enemy_created_observer(
            pEnemy,
            object_id,
            jpb_loader_enemy_created_user_data);
    }
    return object_id;
}

/* 0xBC510, 15 bytes, global, 1 named locals
 * loader_GetALevelName
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
char *loader_GetALevelName(int index)
{
    return sLevelNames[index];
}

/* 0xBC520, 15 bytes, global, 1 named locals
 * loader_GetEnemyName
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
char *loader_GetEnemyName(int index)
{
    return sModelNames[index];
}

/* 0xBC530, 20 bytes, global, 0 named locals
 * loader_GetLevelName
 * PDB type: char* ()
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
char *loader_GetLevelName(void)
{
    return sLevelNames[(uint8_t)LevelSelect];
}

/* 0xBC550, 15 bytes, global, 1 named locals
 * loader_GetModelName
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
char *loader_GetModelName(int index)
{
    return sModelNames[index];
}

/* 0xBC560, 1316 bytes, global, 11 named locals
 * loader_LevelLoad
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */

/* 0xBCA90, 791 bytes, global, 5 named locals
 * loader_LoadJedi
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
void loader_LevelLoad(void)
{
    long size = 0;
    JPBUfbxLoadOpts opts;
    JPBUfbxError error;
    char fileNameBuffer[1024];
    char *levelName;
    char *fullFilePath;
    uint8_t level = (uint8_t)LevelSelect;

    menu_addTotal(100);
    texture_Flush(0x0f80);
    console_loadfont();
    fx_Init();
    menu_addTotal(100);
    menu_addTotal(100);
    updateBucketWrapper((uint8_t *)sLevelNames[(int8_t)LevelSelect]);
    clearzerobss();
    menu_addTotal(100);
    level_InitSpecials((int8_t)LevelSelect);

    if (level >= 1 && level <= 25) {
        const char *bankName;

        if (level == 15) {
            bankName = "corus1";
        } else if (level >= 16 && level <= 22) {
            bankName = "training_level";
        } else {
            bankName = sLevelNames[level];
        }
        (void)sound_LoadBank((char *)bankName, 3);
    }

    menu_addTotal(100);
    /* Retail computes gotJPX == 0 as a sixth outgoing argument, but the
     * exact PDB callee type has five parameters and consumes 2 here. */
    (void)(gotJPX == 0);
    (void)file_LoadChunks2Pool(
        "../../../res/level\\w3d\\",
        sLevelNames[(int8_t)LevelSelect],
        "j3d",
        &size,
        2);

    menu_addTotal(100);
    fullFilePath = (char *)resource_getPathWithExtension(
        sLevelNames[(int8_t)LevelSelect],
        JPB_RESOURCE_LEVEL_CAMERA,
        "cam");
    (void)file_LoadFile(fullFilePath, gpWorld->aDolly);
    memcpy(gpWorld->aBkDolly, gpWorld->aDolly, 0x2000);
    menu_addTotal(100);
    gpWorld->gotbackdrop = 0;

    loader_ApplyLevelLoadSpecials((int8_t)LevelSelect);

    menu_addTotal(100);
    loadPowerupModels();
    loader_loadEnemies((int8_t)LevelSelect);
    menu_addTotal(100);

    memset(&opts, 0, sizeof(opts));
    memset(&error, 0, sizeof(error));
    opts.target_axes.right = 0;
    opts.target_axes.up = 2;
    opts.target_axes.front = 5;
    opts.target_unit_meters = 1.0f;
    levelName = sLevelNames[(int8_t)LevelSelect];
    if (LevelSelect != 0 && LevelSelect != 12) {
        (void)sprintf(
            fileNameBuffer, "%s/%s.fbx", levelName, levelName);
        fullFilePath = (char *)resource_getPath(
            fileNameBuffer, JPB_RESOURCE_LEVEL_JPX);
        if (scene != NULL) {
            ufbx_free_scene(scene);
        }
        scene = ufbx_load_file(fullFilePath, &opts, &error);
        if (scene == NULL) {
            fprintf(stderr, "Failed to load: %s\n", error.description.data);
            exit(1);
        }
        _InitFBXLevelData(scene);
    }

    menu_addTotal(100);
    menu_killLoadScreen();
    maPhysicsData[0].pos.vy = (float)intersec_FindWalkHeight(
        (VECTOR *)(void *)&maPhysicsData[0].pos,
        NULL,
        &maPhysicsData[0].physicsRoot,
        0);
    maPhysicsData[1].pos.vy = (float)intersec_FindWalkHeight(
        (VECTOR *)(void *)&maPhysicsData[1].pos,
        NULL,
        &maPhysicsData[1].physicsRoot,
        0);
    (void)vec_IdentMatrix(&CameraMatrix);
}
void loader_LoadJedi(void)
{
    char *pAnimBuffer;
    geomData *pModelBuffer;
    playerObject *pPlayer0;
    playerObject *pPlayer1;

    if (LevelSelect == 0x0d) {
        GameStruct.ModelSelect[0] = 0x36;
        GameStruct.ModelSelect[1] = 0x35;
    } else if (LevelSelect == 0x0b) {
        GameStruct.ModelSelect[0] = 0x1a;
        GameStruct.ModelSelect[1] = 0x1c;
    } else if (LevelSelect == 0x0c) {
        GameStruct.ModelSelect[0] = 0x3e;
        GameStruct.ModelSelect[1] = 0x0f;
    }

    sound_FreeBank(3);
    sound_FreeBank(2);
    sound_FreeBank(1);
    initDataArrays();
    texture_Flush(14);

    if (LevelSelect != 0) {
        (void)sound_LoadBank(
            sModelNames[GameStruct.ModelSelect[0]], 1);
        if (GameStruct.NumPlayers == 2) {
            pauseUnpauseBucket();
            (void)sound_LoadBank(
                sModelNames[GameStruct.ModelSelect[1]], 2);
            pauseUnpauseBucket();
        }
    }

    pAnimBuffer = loader_LoadJediCAD(0);
    pModelBuffer = loader_loadJediBMD(0);
    pPlayer0 = loader_CreateCharacter(
        0,
        GameStruct.ModelSelect[0],
        pModelBuffer,
        pAnimBuffer,
        jedi_InitPlayer);
    loader_LoadJediCMB(0, pPlayer0);
    pPlayer0->shadow = (int32_t *)(void *)sprite_GetBaseNodeMarker(
        pPlayer0->playerRoot.objectID, 0x30);
    game_gSetMaxEnergy(
        0, GameStruct.maxEnergyLevels[pPlayer0->playerID]);
    game_gSetEnergy(
        0, GameStruct.maxEnergyLevels[pPlayer0->playerID]);
    pauseUnpauseBucket();

    if (GameStruct.NumPlayers == 2 && LevelSelect != 0x0c) {
        pAnimBuffer = loader_LoadJediCAD(1);
        pModelBuffer = loader_loadJediBMD(1);
        pPlayer1 = loader_CreateCharacter(
            1,
            GameStruct.ModelSelect[1],
            pModelBuffer,
            pAnimBuffer,
            jedi_InitPlayer);
        loader_LoadJediCMB(1, pPlayer1);
        pPlayer1->shadow = (int32_t *)(void *)sprite_GetBaseNodeMarker(
            pPlayer1->playerRoot.objectID, 0x30);
        game_gSetMaxEnergy(
            1, GameStruct.maxEnergyLevels[pPlayer1->playerID]);
        game_gSetEnergy(
            1, GameStruct.maxEnergyLevels[pPlayer1->playerID]);
    } else {
        geomData *secondModelBuffer = pModelBuffer;

        if (LevelSelect == 0x0c) {
            secondModelBuffer = loader_loadJediBMD(1);
        }
        pPlayer1 = loader_CreateCharacter(
            1,
            LevelSelect == 0x0c
                ? GameStruct.ModelSelect[1]
                : GameStruct.ModelSelect[0],
            secondModelBuffer,
            pAnimBuffer,
            jedi_InitPlayer);
    }
    pauseUnpauseBucket();

    pPlayer1->target = pPlayer0;
    pPlayer0->target = pPlayer1;
    if (GameStruct.NumPlayers == 2) {
        camera_SetCurrentCameraType(0);
    } else if (LevelSelect == 0x0c) {
        obj_gSetObjectFlag(&pPlayer1->playerRoot, 4, UINT32_C(0x20));
    } else {
        obj_gSetObjectFlag(&pPlayer1->playerRoot, 4, UINT32_C(0x20));
        obj_gSetObjectFlag(&pPlayer1->playerRoot, 3, UINT32_C(0x20));
        obj_gSetObjectFlag(&pPlayer1->playerRoot, 0, UINT32_C(0x20));
        obj_gSetObjectFlag(&pPlayer1->playerRoot, 2, UINT32_C(0x20));
        obj_gSetObjectFlag(&pPlayer1->playerRoot, 1, UINT32_C(0x20));
        camera_SetCurrentCameraType(1);
    }
    loader_PostCreatePlayer(pPlayer0);
    loader_PostCreatePlayer(pPlayer1);
}

/* 0xBCDB0, 244 bytes, local, 3 named locals
 * loader_LoadJediCAD
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
static char *loader_LoadJediCAD(int jedi)
{
    int32_t size = 0;
    int model = GameStruct.ModelSelect[jedi];
    const char *name;
    char *path;

    if (LevelSelect == 8 || LevelSelect == 12) {
        name = sModelNames[0];
    } else if (model < 9) {
        name = sModelNames[model];
    } else {
        name = sAnimNames[model_anim_table[model].poolID];
    }
    path = (char *)resource_getPathWithExtension(
        name, JPB_RESOURCE_ANIMATION, "cad");

    if (model_anim_table[model].poolOffset >= 0) {
        path = (char *)resource_getPathWithExtension(
            sModelNames[model], JPB_RESOURCE_ANIMATION, "cad");
    }
    if (model == 0x29) {
        path = (char *)resource_getPathWithExtension(
            "turret_a", JPB_RESOURCE_ANIMATION, "cad");
    }
    return file_LoadFile2PoolFunc(
               path,
               &size,
               1,
               0x162,
               "W:\\SWJediPowerBattles\\Work\\loader.c") + 4;
}

/* 0xBCEB0, 347 bytes, local, 5 named locals
 * loader_LoadJediCMB
 * PDB type: void (int, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
static void loader_LoadJediCMB(int jedi, playerObject *player)
{
    int model = GameStruct.ModelSelect[jedi];
    char *path = (char *)resource_getPathWithExtension(
        sModelNames[model], JPB_RESOURCE_COMBO, "cmb");
    int32_t size;

    if (player->playerID == 0x4f) {
        path = (char *)resource_getPathWithExtension(
            sModelNames[53], JPB_RESOURCE_COMBO, "cmb");
    }
    size = file_LoadFile(path, player->paCombos);
    if (size == 0) {
        path = (char *)resource_getPathWithExtension(
            sModelNames[17], JPB_RESOURCE_COMBO, "cmb");
        size = file_LoadFile(path, player->paCombos);
    }
    if (size != 0) {
        player->maxCombos = (int16_t)(size / (int32_t)sizeof(Combo));
        if (player->maxCombos > 48) {
            _Exit(1);
        }
        combo_InitComboData(player);
    }

    if (player->playerID == 0) {
        player->paMotions[66].FunctPtr = 0x0e;
    }
    if (player->playerID == 4) {
        player->paMotions[99].FunctPtr = 0x18;
    }
    if (player->playerID == 1) {
        player->paMotions[66].FunctPtr = 0x0e;
        player->paMotions[127].FunctPtr = 0x11;
    }
    if (player->playerID == 2) {
        player->paMotions[127].FunctPtr = 0x14;
    }
    player->paMotions[25].disp = 10;
}

/* 0xBD010, 2577 bytes, local, 17 named locals
 * loader_PostCreatePlayer
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
static void loader_PostCreatePlayer(playerObject *pPlayer)
{
    Motion *motions = pPlayer->paMotions;
    sceneObject *scene =
        (sceneObject *)pPlayer->playerRoot.pParent;
    int model = pPlayer->playerID;

    if (IsExtraCharacter((model_id)model)) {
        Motion *comboMotions[7];
        animObject *pAnim = (animObject *)scene->pAnim;
        int index;

        game_gSetMaxEnergy(pPlayer->playernum, 200);
        game_gSetEnergy(pPlayer->playernum, 200);
        game_gSetMaxForce(pPlayer->playernum, 200);
        game_gSetForce(pPlayer->playernum, 200);
        for (index = 33; index < 51; ++index) {
            if ((motions[index].motionFlags & UINT32_C(0x01000000)) == 0) {
                motions[index].motionFlags |= UINT32_C(0x01000000);
            }
        }
        for (index = 0; index < 7; ++index) {
            comboMotions[index] =
                &motions[pPlayer->paCombos[index].Index];
        }

        switch (model) {
        case 0x35:
        case 0x4f:
            if (strcmp(motions[71].name, "gs_stab_base") == 0) {
                memcpy(motions[71].snd[0], "vggatk2", 8);
                memcpy(motions[71].snd[1], "pnchswng", 8);
                pAnim->soundTimer[0] = 0x4b00;
                pAnim->soundTimer[1] = 0x5300;
            }
            comboMotions[0]->Speed = 0x1800;
            comboMotions[1]->Speed = 0x1800;
            comboMotions[2]->Speed = 0x1800;
            break;
        case 0x31:
            for (index = 0; index < 7; ++index) {
                comboMotions[index]->Speed = 0x1b33;
            }
            break;
        case 0x24:
            for (index = 0; index < 4; ++index) {
                comboMotions[index]->Speed = 0x1b33;
            }
            break;
        case 0x0f:
            for (index = 0; index < 4; ++index) {
                comboMotions[index]->Speed = 0x1b33;
            }
            comboMotions[4]->Speed = 0x14cc;
            comboMotions[5]->Speed = 0x1b33;
            break;
        default:
            break;
        }
    }

    if (model == 0x1e) {
        animObject *pAnim = (animObject *)scene->pAnim;
        Motion *sourceMotion = &motions[71];
        _animTemplate *sourceTemplate =
            &pAnim->depack_context.seqdata[sourceMotion->globalID];
        Motion *RamMotion = &motions[15];
        Motion *BlockIdleMotion = &motions[21];
        _animTemplate *RamAnimTemplate =
            &pAnim->depack_context.seqdata[15];
        _animTemplate *BlockIdleAnimTemplate =
            &pAnim->depack_context.seqdata[21];

        *RamAnimTemplate = *sourceTemplate;
        *RamMotion = *sourceMotion;
        RamMotion->vel = 0;
        RamMotion->Damage = 0;
        RamMotion->Seq = 15;
        RamMotion->globalID = 15;
        RamMotion->Speed = 0x2000;

        *BlockIdleAnimTemplate = *sourceTemplate;
        BlockIdleAnimTemplate->Lframe = 13;
        *BlockIdleMotion = *RamMotion;
        BlockIdleMotion->motionFlags = UINT32_C(0xa0000003);
        BlockIdleMotion->Seq = 21;
        BlockIdleMotion->globalID = 21;
        BlockIdleMotion->Speed = -1;

        motions[pPlayer->paCombos[0].Index].Speed = 0x2000;
        motions[pPlayer->paCombos[1].Index].Speed = 0x3000;
        motions[pPlayer->paCombos[2].Index].Speed = 0x3000;
        motions[pPlayer->paCombos[3].Index].Speed = 0x2000;
        motions[pPlayer->paCombos[4].Index].Speed = 0x2000;
        motions[19] = motions[0];
        motions[61].globalID = (uint16_t)pPlayer->maxMotions;
    }

    if (model == 0x4f || model == 0x35 || model == 0x36) {
        animObject *pAnim = (animObject *)scene->pAnim;
        _animTemplate *sourceTemplate =
            &pAnim->depack_context.seqdata[24];
        _animTemplate *EdgeGrabAnimTemplate =
            &pAnim->depack_context.seqdata[62];

        motions[2].vel = (int16_t)(motions[2].vel * 1.2);
        pAnim->depack_context.seqdata[63] = *sourceTemplate;
        motions[63] = motions[24];
        motions[63].Seq = 63;
        motions[63].globalID = 63;
        *EdgeGrabAnimTemplate = *sourceTemplate;
        EdgeGrabAnimTemplate->Lframe = 2;
        motions[62] = motions[24];
        motions[62].Seq = 62;
        motions[62].globalID = 62;
    }

    if (model == 0x0f || model == 0x15 || model == 0x12) {
        motions[82] = motions[0];
    }
    if (model == 0x25) {
        motions[72].FunctPtr = 0x31;
    }

    switch (model) {
    case 0x0f:
        motions[65] = motions[75];
        motions[65].FunctPtr = 0x19;
        break;
    case 0x1a:
        motions[65] = motions[66];
        motions[65].FunctPtr = 0x19;
        break;
    case 0x32:
        motions[65] = motions[73];
        motions[65].FunctPtr = 0x19;
        break;
    case 0x35:
    case 0x4f:
        motions[65] = motions[76];
        motions[65].FunctPtr = 0x19;
        break;
    default:
        break;
    }

    if (LevelSelect == 0x0b) {
        physicsObject *physics = (physicsObject *)scene->pPhysics;
        physics->radius = 0x50;
    }
    if (LevelSelect == 0x14 &&
        (model == 0x11 || model == 6 || model == 7)) {
        int source = model == 0x11 ? 15 : 16;

        motions[16] = motions[source];
        motions[17] = motions[source];
        motions[18] = motions[source];
    }
}

/* 0xBDA30, 654 bytes, local, 10 named locals
 * loader_loadEnemies
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
static void loader_loadEnemies(unsigned level)
{
    int m;

    (void)level;
    memset(maModelID, 0, sizeof(maModelID));
    for (m = 0; m < gpWorld->nActor && m < 20; ++m) {
        unsigned char *modName =
            (unsigned char *)gpWorld->apActorNames[m];
        char stemp[64];
        int modelID;
        int j;

        menu_addTotal(50);
        for (j = 0; j < JPB_ACTOR_NAME_COUNT; ++j) {
            strcpy(stemp, sObiNames[j]);
            strcat(stemp, ".baf");
            if (strstr((char *)modName, stemp) != NULL) {
                break;
            }
        }
        modelID = j < JPB_ACTOR_NAME_COUNT ? j : -1;
        if (modelID == 0x19) {
            modelID = 0x21;
        }

        if (maModelData[modelID] == NULL) {
            long size = 0;
            const char *fullFilePath = resource_getPathWithExtension(
                sModelNames[modelID], JPB_RESOURCE_MODEL, "bmd");
            char *pModelBuffer = file_LoadFile2PoolFunc(
                (char *)fullFilePath,
                &size,
                -1,
                0x3e5,
                "W:\\SWJediPowerBattles\\Work\\loader.c") + 4;

            maModelData[modelID] = pModelBuffer;
            (void)model_gInitModelRoot(
                (geomData *)(void *)pModelBuffer,
                sModelNames[modelID],
                -1);
        }

        {
            int poolID = model_anim_table[modelID].poolID;

            if (maAnimData[poolID] == NULL) {
                long size = 0;
                const char *fullFilePath = resource_getPathWithExtension(
                    sAnimNames[poolID], JPB_RESOURCE_ANIMATION, "cad");
                char *pAnimBuffer = file_LoadFile2PoolFunc(
                    (char *)fullFilePath,
                    &size,
                    -1,
                    0x3f4,
                    "W:\\SWJediPowerBattles\\Work\\loader.c") + 4;

                maAnimData[poolID] = pAnimBuffer;
            }
        }

        maModelID[m][0] = modelID;
        maModelID[m][1] = m;
        (void)ai_LoadAI(m, sModelNames[modelID]);
        (void)memory_gMemUseage();
    }
}

/* 0xBDCC0, 832 bytes, global, 24 named locals
 * loader_loadJarJarOverrideModel
 * PDB type: void (geomData**, long*)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
void loader_loadJarJarOverrideModel(
    geomData **ppModelBuffer, long *pSize)
{
    geomData *pModelBuffer = *ppModelBuffer;
    long size = *pSize;
    long gunganSize = 0;
    const char *gunganModelPath = resource_getPathWithExtension(
        sModelNames[53], JPB_RESOURCE_MODEL, "bmd");
    geomData *gunganModel = (geomData *)(void *)(
        file_LoadFile2PoolFunc(
            (char *)gunganModelPath,
            &gunganSize,
            1,
            0xda,
            "W:\\SWJediPowerBattles\\Work\\loader.c") + 4);
    geomData *gunganWeapon = getNodeByName(
        gunganModel, gunganSize, "weapon1");
    const uint64_t vertSize =
        (uint64_t)gunganWeapon->numVerts * 4;
    const uint64_t UVSize =
        (uint64_t)gunganWeapon->numFaces * 128;
    const uint64_t colorSize =
        (uint64_t)gunganWeapon->numFaces * 16;
    const uint64_t indexSize =
        (uint64_t)gunganWeapon->numFaces * 8;
    const long newSize = (long)(
        size + vertSize * 2 + UVSize + colorSize + indexSize);
    geomData *newModelBuffer = (geomData *)malloc((size_t)newSize);
    void *DestVertex;
    void *DestNormal;
    void *DestUV;
    void *DestColor;
    void *DestIndex;
    geomData *weaponChildren;
    geomData *jarjarWeapon;
    int i;

    memcpy(newModelBuffer, pModelBuffer, (size_t)size);
    DestVertex = (char *)newModelBuffer + size;
    memset(
        DestVertex,
        0,
        (size_t)(newSize - size));
    DestNormal = (char *)DestVertex + vertSize;
    DestUV = (char *)DestNormal + vertSize;
    DestColor = (char *)DestUV + UVSize;
    DestIndex = (char *)DestColor + colorSize;

    memcpy(
        DestVertex,
        (char *)gunganModel + gunganWeapon->pVertex,
        (size_t)vertSize);
    memcpy(
        DestNormal,
        (char *)gunganModel + gunganWeapon->pNormal,
        (size_t)vertSize);
    memcpy(
        DestUV,
        (char *)gunganModel + gunganWeapon->pUV,
        (size_t)UVSize);
    memcpy(
        DestColor,
        (char *)gunganModel + gunganWeapon->pColor,
        (size_t)colorSize);
    memcpy(
        DestIndex,
        (char *)gunganModel + gunganWeapon->pIndex,
        (size_t)indexSize);

    weaponChildren = &newModelBuffer[39];
    for (i = 0; i < gunganWeapon->numChildren; ++i) {
        memcpy(
            &weaponChildren[i],
            &gunganModel[gunganWeapon->aChildren[i]],
            sizeof(*weaponChildren));
    }

    jarjarWeapon = getNodeByName(
        newModelBuffer, size, "weapon1");
    memcpy(jarjarWeapon, gunganWeapon, sizeof(*jarjarWeapon));
    jarjarWeapon->pVertex =
        (int32_t)((char *)DestVertex - (char *)newModelBuffer);
    jarjarWeapon->pNormal =
        (int32_t)((char *)DestNormal - (char *)newModelBuffer);
    jarjarWeapon->pUV =
        (int32_t)((char *)DestUV - (char *)newModelBuffer);
    jarjarWeapon->pColor =
        (int32_t)((char *)DestColor - (char *)newModelBuffer);
    jarjarWeapon->pIndex =
        (int32_t)((char *)DestIndex - (char *)newModelBuffer);
    for (i = 0; i < gunganWeapon->numChildren; ++i) {
        jarjarWeapon->aChildren[i] = i + 39;
    }

    *ppModelBuffer = newModelBuffer;
    *pSize = newSize;
}

/* 0xBE000, 193 bytes, local, 4 named locals
 * loader_loadJediBMD
 * PDB type: geomData* (int)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
static geomData *loader_loadJediBMD(int pnum)
{
    long size = 0;
    int model = GameStruct.ModelSelect[pnum];
    const char *fullFilePath = resource_getPathWithExtension(
        sModelNames[model], JPB_RESOURCE_MODEL, "bmd");
    geomData *pModelBuffer;

    if (model == 0x4f) {
        fullFilePath = resource_getPathWithExtension(
            sModelNames[10], JPB_RESOURCE_MODEL, "bmd");
    }
    pModelBuffer = (geomData *)(void *)(
        file_LoadFile2PoolFunc(
            (char *)fullFilePath,
            &size,
            1,
            0x130,
            "W:\\SWJediPowerBattles\\Work\\loader.c") + 4);
    if (model == 0x4f) {
        loader_loadJarJarOverrideModel(&pModelBuffer, &size);
    }
    maModelData[model] = (char *)pModelBuffer;
    return pModelBuffer;
}
