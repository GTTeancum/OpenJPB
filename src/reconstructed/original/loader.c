/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\loader.c.
 *
 * The model/animation construction performed by loader_CreateEnemy still
 * depends on unrecovered renderer-era object creation. A narrow provider
 * keeps that boundary explicit while allowing the exact enemy.c spawn
 * orchestration to be reconstructed and tested without importing a host
 * graphics or input dependency into original gameplay code.
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
#include "jpb/level_world.h"

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

static JPBLoaderEnemyCreateProvider
    jpb_loader_enemy_create_provider;
static void *jpb_loader_enemy_create_user_data;

void jpb_LoaderSetEnemyCreateProvider(
    JPBLoaderEnemyCreateProvider provider,
    void *user_data)
{
    jpb_loader_enemy_create_provider = provider;
    jpb_loader_enemy_create_user_data = user_data;
}

/* 0xBBD00, 14 bytes, global, 1 named locals
 * IsPlayerCharacter
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\include\brain.h
 */

/* 0xBBD10, 135 bytes, global, 5 named locals
 * getNodeByName
 * PDB type: geomData* (geomData*, long, cons...
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */

/* 0xBBDA0, 48 bytes, global, 0 named locals
 * initDataArrays
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */

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

/* 0xBC0B0, 1114 bytes, global, 9 named locals
 * loader_CreateEnemy
 * PDB type: int (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
int loader_CreateEnemy(wsl_ENEMY *pEnemy)
{
    if (jpb_loader_enemy_create_provider == NULL) {
        return 0;
    }
    return jpb_loader_enemy_create_provider(
        pEnemy, jpb_loader_enemy_create_user_data);
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

/* 0xBCDB0, 244 bytes, local, 3 named locals
 * loader_LoadJediCAD
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */

/* 0xBCEB0, 347 bytes, local, 5 named locals
 * loader_LoadJediCMB
 * PDB type: void (int, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */

/* 0xBD010, 2577 bytes, local, 17 named locals
 * loader_PostCreatePlayer
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */

/* 0xBDA30, 654 bytes, local, 10 named locals
 * loader_loadEnemies
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */

/* 0xBDCC0, 832 bytes, global, 24 named locals
 * loader_loadJarJarOverrideModel
 * PDB type: void (geomData**, long*)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */

/* 0xBE000, 193 bytes, local, 4 named locals
 * loader_loadJediBMD
 * PDB type: geomData* (int)
 * Source: W:\SWJediPowerBattles\Work\loader.c
 */
