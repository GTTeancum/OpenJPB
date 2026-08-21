#include "jpb/game.h"
#include "jpb/ai.h"
#include "jpb/alloc.h"
#include "jpb/boss.h"
#include "jpb/brain.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/combo.h"
#include "jpb/debugtext.h"
#include "jpb/force.h"
#include "jpb/extracharacters.h"
#include "jpb/jedi.h"
#include "jpb/level.h"
#include "jpb/menu.h"
#include "jpb/player.h"
#include "jpb/savegame.h"
#include "jpb/sprite.h"
#include "jpb/text.h"
#include "jpb/vehicle.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

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

#define CHECK_FLOAT_CLOSE(actual, expected, tolerance)                        \
    CHECK((actual) >= (expected) - (tolerance) &&                             \
          (actual) <= (expected) + (tolerance))

static void reset_game_state(void)
{
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(gaPlayerData, 0, sizeof(gaPlayerData));
    gGlobalTimer = 0;
    LevelSelect = 0;
}

static int test_process_status_death_routes(void)
{
    WorldData world;
    WorldData *saved_world = gpWorld;
    playerObject *saved_afterlife = afterLife;

    reset_game_state();
    memset(&world, 0, sizeof(world));
    memset(&menuVars, 0, sizeof(menuVars));
    world.player0 = &gaPlayerData[0];
    world.player1 = &gaPlayerData[1];
    gpWorld = &world;
    gaPlayerData[0].playerRoot.objectID = 0;
    gaPlayerData[0].playernum = 0;
    gaPlayerData[1].playerRoot.objectID = -1;
    gaPlayerData[1].playernum = 1;
    GameStruct.CurrentLevel = 1;
    GameStruct.NumPlayers = 1;
    GameStruct.mNumContinues = 1000;
    GameStruct.GameState = UINT32_C(0xa0);
    newcameraflag = 0;

    game_ProcessStatus();
    CHECK((GameStruct.GameState & UINT32_C(0xa0)) == 0);
    CHECK(GameStruct.StageExit == 1);
    CHECK(GameStruct.Continuing == 1);
    CHECK(GameStruct.LevelExit == 0);
    CHECK(GameStruct.ContinuesUsed == 1);
    CHECK(newcameraflag == 1);

    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&menuVars, 0, sizeof(menuVars));
    gaPlayerData[0].playerRoot.objectID = -1;
    gaPlayerData[1].playerRoot.objectID = -1;
    GameStruct.CurrentLevel = 1;
    GameStruct.NumPlayers = 1;
    GameStruct.mNumContinues = 0;
    GameStruct.GameState = UINT32_C(0x20);
    OptionStruct.Music = 0;
    menuVars.menuMode[0] = UINT16_C(0x66);

    game_ProcessStatus();
    CHECK((GameStruct.GameState & UINT32_C(0x20)) == 0);
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) != 0);
    CHECK(GameStruct.StageExit == 1);
    CHECK(GameStruct.LevelExit == 1);
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == UINT16_C(0x2d));
    CHECK(afterLife == NULL);

    memset(&GameStruct, 0, sizeof(GameStruct));
    GameStruct.CurrentLevel = 1;
    GameStruct.GameState = UINT32_C(0x00010080);
    game_ProcessStatus();
    CHECK((GameStruct.GameState & UINT32_C(0x80)) == 0);
    CHECK(GameStruct.StageExit == 1);

    gpWorld = saved_world;
    afterLife = saved_afterlife;
    return 0;
}

typedef struct OverlayTrace {
    int textCalls;
    wchar_t text[16][32];
    int textTint[16];
    int textAlpha[16];
    int textMode[16];
    int textX[16];
    int textY[16];
    float textScale[16];
    float textScaleAdjustment[16];
    int textFontStyle[16];
    int textureCalls;
    unsigned textures[2];
} OverlayTrace;

typedef struct TextureRectTrace {
    int calls;
    _Material *materials[16];
    SCREENRECT destinations[16];
    SCREENRECT sources[16];
    int hasSource[16];
    CVECTOR colors[16];
} TextureRectTrace;

typedef struct BarTrace {
    int calls;
    int x[8];
    int y[8];
    int width[8];
    int height[8];
    uint32_t color[8];
} BarTrace;

typedef struct BigNumberTrace {
    int calls;
    unsigned texture[16];
    float x[16];
    float y[16];
    float width[16];
    float height[16];
    unsigned transparency[16];
    int red[16];
    int green[16];
    int blue[16];
} BigNumberTrace;

static int capture_big_number_texture(
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
    BigNumberTrace *trace = (BigNumberTrace *)user_data;
    int index = trace->calls++;

    if (index < 16) {
        trace->texture[index] = texture;
        trace->x[index] = x;
        trace->y[index] = y;
        trace->width[index] = width;
        trace->height[index] = height;
        trace->transparency[index] = transparency;
        trace->red[index] = red;
        trace->green[index] = green;
        trace->blue[index] = blue;
    }
    return 4;
}

static int capture_overlay_text(
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
    OverlayTrace *trace = (OverlayTrace *)user_data;
    int index = trace->textCalls++;

    (void)tint;
    if (index < 16) {
        trace->textTint[index] = tint;
        trace->textAlpha[index] = alpha;
        trace->textMode[index] = mode;
        trace->textX[index] = x;
        trace->textY[index] = y;
        trace->textScale[index] = scale;
        trace->textScaleAdjustment[index] = scale_adjustment;
        trace->textFontStyle[index] = font_style;
        (void)wcsncpy(trace->text[index], text, 31);
        trace->text[index][31] = L'\0';
    }
    return (int)wcslen(text);
}

static int capture_psx_texture(
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
    OverlayTrace *trace = (OverlayTrace *)user_data;
    int index = trace->textureCalls++;

    (void)x;
    (void)y;
    (void)height;
    (void)transparency;
    (void)red;
    (void)green;
    (void)blue;
    if (index < 2) {
        trace->textures[index] = texture;
    }
    return (int)width;
}

static void capture_texture_rect(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth)
{
    TextureRectTrace *trace = (TextureRectTrace *)user_data;
    int index = trace->calls++;

    (void)source;
    (void)layer_depth;
    if (index < 16) {
        trace->materials[index] = texture;
        trace->destinations[index] = *destination;
        if (source != NULL) {
            trace->sources[index] = *source;
            trace->hasSource[index] = 1;
        } else {
            memset(&trace->sources[index], 0, sizeof(trace->sources[index]));
            trace->hasSource[index] = 0;
        }
        trace->colors[index] = color;
    }
}

static void capture_bar(
    void *user_data,
    int x,
    int y,
    int width,
    int height,
    uint32_t color)
{
    BarTrace *trace = (BarTrace *)user_data;
    int index = trace->calls++;

    if (index < 8) {
        trace->x[index] = x;
        trace->y[index] = y;
        trace->width[index] = width;
        trace->height[index] = height;
        trace->color[index] = color;
    }
}

static int find_scb_rect_with_texture(
    const _Material *texture,
    float x0,
    float y0,
    float x3,
    float y3)
{
    Node *node;

    for (node = mSCBDraw[0].head;
         node != NULL;
         node = node->next) {
        SCB *scb = (SCB *)node;

        if ((texture == NULL || scb->scb_Texture == texture) &&
            scb->scb_vertex0.vx == x0 &&
            scb->scb_vertex0.vy == y0 &&
            scb->scb_vertex3.vx == x3 &&
            scb->scb_vertex3.vy == y3) {
            return 1;
        }
    }
    return 0;
}

static SCB *find_scb_with_texture_rect(
    const _Material *texture,
    float x0,
    float y0,
    float x3,
    float y3)
{
    Node *node;

    for (node = mSCBDraw[0].head;
         node != NULL;
         node = node->next) {
        SCB *scb = (SCB *)node;

        if (scb->scb_Texture == texture &&
            scb->scb_vertex0.vx == x0 &&
            scb->scb_vertex0.vy == y0 &&
            scb->scb_vertex3.vx == x3 &&
            scb->scb_vertex3.vy == y3) {
            return scb;
        }
    }
    return NULL;
}

static int find_scb_rect(float x0, float y0, float x3, float y3)
{
    return find_scb_rect_with_texture(NULL, x0, y0, x3, y3);
}

static SCB *find_first_scb_with_texture(const _Material *texture)
{
    Node *node;

    for (node = mSCBDraw[0].head;
         node != NULL;
         node = node->next) {
        SCB *scb = (SCB *)node;

        if (scb->scb_Texture == texture) {
            return scb;
        }
    }
    return NULL;
}

static int find_texture_rect(
    const TextureRectTrace *trace,
    const _Material *material,
    int left,
    int top,
    int right,
    int bottom,
    unsigned char r,
    unsigned char g,
    unsigned char b,
    unsigned char cd)
{
    int i;

    for (i = 0; i < trace->calls && i < 16; ++i) {
        const SCREENRECT *destination = &trace->destinations[i];
        const CVECTOR *color = &trace->colors[i];

        if (trace->materials[i] == material &&
            destination->left == left &&
            destination->top == top &&
            destination->right == right &&
            destination->bottom == bottom &&
            color->r == r &&
            color->g == g &&
            color->b == b &&
            color->cd == cd) {
            return 1;
        }
    }
    return 0;
}

static int find_overlay_text(
    const OverlayTrace *trace,
    const wchar_t *text,
    int mode,
    int x,
    int y,
    float scale)
{
    int i;

    for (i = 0; i < trace->textCalls && i < 16; ++i) {
        if (wcscmp(trace->text[i], text) == 0 &&
            trace->textMode[i] == mode &&
            trace->textX[i] == x &&
            trace->textY[i] == y &&
            trace->textScale[i] >= scale - 0.0001f &&
            trace->textScale[i] <= scale + 0.0001f) {
            return 1;
        }
    }
    return 0;
}

static int test_energy_access_and_clamps(void)
{
    reset_game_state();

    GameStruct.aCharacterData[0].MaxEnergy = 100;
    CHECK(game_gSetEnergy(0, -1) == 0);
    CHECK(game_gGetEnergy(0) == 0);
    CHECK(game_gSetEnergy(0, 2000) == 1600);
    CHECK(game_gGetEnergy(0) == 1600);
    CHECK(GameStruct.aCharacterData[0].MaxEnergy == 1600);

    GameStruct.aCharacterData[1].MaxEnergy = 10;
    CHECK(game_gSetEnergy(1, 300) == 255);
    CHECK(game_gGetEnergy(1) == 255);
    CHECK(GameStruct.aCharacterData[1].MaxEnergy == 255);
    return 0;
}

static int test_energy_line_scaling(void)
{
    CharacterData *character;

    reset_game_state();
    character = &GameStruct.aCharacterData[0];
    GameStruct.ModelSelect[0] = 2;
    GameStruct.maxEnergyLineLength[2] = 37;
    character->MaxEnergy = 10;

    CHECK(game_gSetEnergy(0, 100) == 100);
    CHECK(character->MaxEnergy == 100);
    CHECK(character->MaxEnergyPerc == ((37u << 16) / 100u));

    character->MaxEnergyPerc = 0x12345678u;
    CHECK(game_gSetEnergy(0, 50) == 50);
    CHECK(character->MaxEnergy == 100);
    CHECK(character->MaxEnergyPerc == 0x12345678u);

    character = &GameStruct.aCharacterData[2];
    character->MaxEnergy = 0;
    CHECK(game_gSetEnergy(2, 49) == 49);
    CHECK(character->MaxEnergyPerc == ((12u << 16) / 49u));
    CHECK(game_gSetEnergy(2, 50) == 50);
    CHECK(character->MaxEnergyPerc == ((25u << 16) / 50u));
    return 0;
}

static int test_exact_setters(void)
{
    CharacterData *character;

    reset_game_state();
    character = &GameStruct.aCharacterData[0];
    GameStruct.ModelSelect[0] = 3;
    GameStruct.maxEnergyLineLength[3] = 37;
    GameStruct.maxForceLineLength[3] = 29;

    game_gSetMaxEnergy(0, 180);
    CHECK(character->MaxEnergy == 180);
    CHECK(
        character->MaxEnergyPerc ==
        ((37u << 16) / 180u));
    game_gSetMaxForce(0, 160);
    CHECK(character->MaxForce == 160);
    CHECK(game_gGetMaxForce(0) == 160);
    CHECK(
        character->MaxForcePerc ==
        (int16_t)((29u << 16) / 160u));
    CHECK(game_gSetForce(0, -1) == 0);
    CHECK(game_gSetForce(0, 300) == 255);
    CHECK(character->Force == 255);
    CHECK(game_gSetEnergy(0, 90) == 90);
    CHECK(game_gGetScaleEnergy(0) == 18);
    CHECK(
        game_gGetScaleMaxEnergy(0) ==
        (int)((180u * character->MaxEnergyPerc) >> 16));
    CHECK(
        game_gGetScaleForce(0) ==
        ((255 * (int32_t)character->MaxForcePerc) >> 16));
    CHECK(
        game_gGetScaleMaxForce(0) ==
        ((160 * (int32_t)character->MaxForcePerc) >> 16));

    game_gSetMaxEnergy(2, 49);
    CHECK(
        GameStruct.aCharacterData[2].MaxEnergyPerc ==
        ((12u << 16) / 49u));
    game_gSetMaxEnergy(2, 50);
    CHECK(
        GameStruct.aCharacterData[2].MaxEnergyPerc ==
        ((25u << 16) / 50u));
    game_gSetMaxForce(2, 0);
    CHECK(GameStruct.aCharacterData[2].MaxForcePerc == 0);

    CHECK(game_gSetScore(0, -10) == 0);
    CHECK(game_gSetScore(0, 42) == 42);
    CHECK(game_gSetScore(0, 1000000) == 999999);
    CHECK(character->Score == 999999);
    return 0;
}

static int test_energy_modification(void)
{
    reset_game_state();
    GameStruct.aCharacterData[0].Energy = 60;
    GameStruct.aCharacterData[0].MaxEnergy = 100;

    CHECK(game_gModEnergy(0, 30) == 90);
    CHECK(game_gModEnergy(0, 50) == 100);
    CHECK(game_gModEnergy(0, -250) == 0);

    GameStruct.aCharacterData[0].Energy = 60;
    LevelSelect = 8;
    CHECK(game_gModEnergy(0, -20) == 60);
    CHECK(GameStruct.aCharacterData[0].Energy == 60);

    GameStruct.aCharacterData[2].Energy = 70;
    GameStruct.aCharacterData[2].MaxEnergy = 90;
    gaPlayerData[2].playerID = 0x17;
    CHECK(game_gModEnergy(2, -20) == 70);
    CHECK(GameStruct.aCharacterData[2].Energy == 70);
    gaPlayerData[2].playerID = 0x48;
    CHECK(game_gModEnergy(2, -20) == 70);
    gaPlayerData[2].playerID = 5;
    CHECK(game_gModEnergy(2, 20) == 0);
    CHECK(GameStruct.aCharacterData[2].Energy == 0);
    return 0;
}

static int test_game_flags_items_and_power(void)
{
    reset_game_state();

    GameStruct.Mode = -3;
    CHECK(game_GetGameMode() == -3);
    GameStruct.GameState = 0x10u;
    CHECK(game_gGetGameFlags() == 0x10u);
    CHECK(game_gSetGameFlags(0x24u) == 0x34u);
    CHECK(GameStruct.GameState == 0x34u);
    CHECK(game_gClrGameFlags(0x14u) == 0x20u);
    CHECK(game_gToggleGameFlags(0x60u) == 0x40u);
    CHECK(game_gGetGameFlags() == 0x40u);

    CHECK(game_gSetItemCount(3, -7) == 0);
    CHECK(GameStruct.aCharacterData[3].Items == 0);
    CHECK(game_gSetItemCount(3, 9) == 4);
    CHECK(GameStruct.aCharacterData[3].Items == 4);
    CHECK(game_gModItemCount(3, -2) == 2);
    CHECK(game_gModItemCount(3, -8) == 0);
    CHECK(game_gModItemCount(3, 12) == 4);

    CHECK(game_gSetPowerType(3, 7) == 7);
    CHECK(GameStruct.aCharacterData[3].PowerType == 7);
    CHECK(game_gSetPowerType(3, 0xffff) == -1);
    CHECK(GameStruct.aCharacterData[3].PowerType == -1);

    GameStruct.aCharacterData[3].Force = 40;
    GameStruct.aCharacterData[3].MaxForce = 100;
    CHECK(game_gGetForce(3) == 40);
    CHECK(game_gModForce(3, 25) == 65);
    CHECK(game_gModForce(3, 100) == 100);
    CHECK(game_gModForce(3, -250) == 0);

    gGlobalTimer = 1000;
    CHECK(game_gSetPowerLevel(3, -1) == 1000);
    CHECK(GameStruct.aCharacterData[3].PowerLevel == 1000);
    CHECK(game_gSetPowerLevel(3, 50) == 1050);
    CHECK(game_gSetPowerLevel(3, 0x80000) == 1000 + 0x70800);
    CHECK(GameStruct.aCharacterData[3].PowerLevel == 1000 + 0x70800);
    CHECK(game_gGetPowerType(3) == -1);
    CHECK(game_gGetPowerLevel(3) == 1000 + 0x70800);

    GameStruct.aCharacterData[3].Score = 40;
    CHECK(game_gGetScore(3) == 40);
    CHECK(game_gModScore(3, 12) == 52);
    CHECK(game_gModScore(3, -100) == 0);
    CHECK(game_gModScore(3, 2000000) == 999999);

    GameStruct.Counter = 7;
    CHECK(game_ModGameCounter(3) == 10);
    CHECK(GameStruct.Counter == 10);
    CHECK(game_ModGameCounter(-4) == 6);
    CHECK(GameStruct.Counter == 6);
    return 0;
}

static int test_authored_initial_game_tables(void)
{
    static const uint8_t expected_masks[9][6] = {
        {0x77, 0x0e, 0, 0, 0, 0},
        {0x5f, 0x09, 0, 0, 0, 0},
        {0x17, 0x10, 0, 0, 0, 0},
        {0x3f, 0x1e, 0, 0, 0, 0},
        {0x57, 0x00, 0, 0, 0, 0},
        {0xff, 0x1f, 0, 0, 0, 0},
        {0x7f, 0x00, 0, 0, 0, 0},
        {0x7f, 0x00, 0, 0, 0, 0},
        {0x3f, 0x01, 0, 0, 0, 0}
    };
    int model;

    memset(&GameStruct, 0xa5, sizeof(GameStruct));
    game_initEnergy();
    for (model = 0;
         model < JPB_GAME_JEDI_MODEL_CAPACITY;
         ++model) {
        CHECK(GameStruct.maxEnergyLevels[model] == 100);
        CHECK(GameStruct.maxEnergyLineLength[model] == 25);
        CHECK(GameStruct.maxForceLevels[model] == 100);
        CHECK(GameStruct.maxForceLineLength[model] == 25);
    }

    game_initCombos();
    CHECK(memcmp(
              GameStruct.jediComboMask,
              expected_masks,
              sizeof(expected_masks)) == 0);
    CHECK(GameStruct.jediComboMask[9].m[0] == UINT8_C(0xa5));
    CHECK(game_getCombo(0, 11) == UINT32_C(0x08));
    CHECK(game_getCombo(0, 12) == 0);
    game_enableCombo(0, 47);
    CHECK(game_getCombo(0, 47) == UINT32_C(0x80));
    game_disableCombo(0, 47);
    CHECK(game_getCombo(0, 47) == 0);
    return 0;
}

static int test_new_game_initialization(void)
{
    size_t index;

    memset(&GameStruct, 0xa5, sizeof(GameStruct));
    memset(abGlobalBits, 0xa5, sizeof(abGlobalBits));
    memset(jediUpgrades, 0xa5, sizeof(jediUpgrades));
    secretBits = UINT32_C(0xa5a5a5a5);

    newGameGameInit();

    for (index = 0;
         index < sizeof(GameStruct.jediLevelPlayed);
         ++index) {
        CHECK(((uint8_t *)GameStruct.jediLevelPlayed)[index] == 0);
    }
    for (index = 0;
         index < sizeof(GameStruct.jediScorePerLevel);
         ++index) {
        CHECK(((uint8_t *)GameStruct.jediScorePerLevel)[index] == 0);
    }
    for (index = 0;
         index < sizeof(GameStruct.aCharacterData);
         ++index) {
        CHECK(((uint8_t *)GameStruct.aCharacterData)[index] == 0);
    }
    for (index = 0; index < sizeof(abGlobalBits); ++index) {
        CHECK(abGlobalBits[index] == 0);
    }
    for (index = 0; index < sizeof(jediUpgrades); ++index) {
        CHECK(((uint8_t *)jediUpgrades)[index] == 0);
    }

    CHECK(GameStruct.Continuing == 0);
    CHECK(secretBits == 0);
    CHECK(GameStruct.gameCompleted == 0);
    CHECK(GameStruct.ModelSelect[0] == 0);
    CHECK(GameStruct.ModelSelect[1] == 1);
    CHECK(GameStruct.maxEnergyLevels[22] == 100);
    CHECK(GameStruct.maxEnergyLineLength[22] == 25);
    CHECK(GameStruct.maxForceLevels[22] == 100);
    CHECK(GameStruct.maxForceLineLength[22] == 25);
    CHECK(game_getCombo(0, 0) != 0);
    CHECK(game_getCombo(8, 8) != 0);

    /* The reference owner deliberately leaves unrelated state untouched. */
    CHECK((uint8_t)GameStruct.Mode == UINT8_C(0xa5));
    CHECK((uint32_t)GameStruct.StageExit == UINT32_C(0xa5a5a5a5));
    CHECK(GameStruct.jediComboMask[9].m[0] == UINT8_C(0xa5));
    CHECK(GameStruct.checkpoint[0] == UINT8_C(0xa5));
    return 0;
}

static int test_player_start_combos(void)
{
    Combo combos[4];

    reset_game_state();
    memset(combos, 0, sizeof(combos));
    gaPlayerData[0].playerID = 2;
    gaPlayerData[0].paCombos = combos;
    gaPlayerData[0].maxCombos = 4;
    strcpy(combos[0].String, "n");
    combos[0].comboFlags = UINT32_C(0x00200000);
    combos[1].comboFlags = UINT32_C(0x00200000);
    strcpy(combos[2].String, "s");
    combos[3].comboFlags = UINT32_C(0x00200000);
    strcpy(combos[3].String, "w");

    game_initPlayerStartCombos(0);
    CHECK(game_getCombo(2, 0) != 0);
    CHECK(game_getCombo(2, 1) == 0);
    CHECK(game_getCombo(2, 2) == 0);
    CHECK(game_getCombo(2, 3) != 0);
    CHECK(game_getCombo(0, 0) == 0);
    return 0;
}

static int test_level_difficulty_tables(void)
{
    reset_game_state();

    CHECK(jpb_game_ApplyLevelDifficulty(0, 1) == 1);
    CHECK(GameStruct.AIDamage == 8);
    CHECK(GameStruct.JediDamage == 8);
    CHECK(GameStruct.HTHRate == 8);
    CHECK(GameStruct.RangedRate == 8);
    CHECK(GameStruct.BlockRate == 8);
    CHECK(GameStruct.difficulty == 1);

    CHECK(jpb_game_ApplyLevelDifficulty(1, 0) == 1);
    CHECK(GameStruct.AIDamage == 4);
    CHECK(GameStruct.JediDamage == 11);
    CHECK(GameStruct.HTHRate == 13);
    CHECK(GameStruct.RangedRate == 10);
    CHECK(GameStruct.BlockRate == 10);
    CHECK(GameStruct.difficulty == 0);

    CHECK(jpb_game_ApplyLevelDifficulty(15, 1) == 1);
    CHECK(GameStruct.AIDamage == 7);
    CHECK(GameStruct.JediDamage == 9);
    CHECK(GameStruct.HTHRate == 9);
    CHECK(GameStruct.RangedRate == 11);
    CHECK(GameStruct.BlockRate == 7);

    CHECK(jpb_game_ApplyLevelDifficulty(16, 1) == 1);
    CHECK(GameStruct.AIDamage == 8);
    CHECK(GameStruct.JediDamage == 8);

    GameStruct.AIDamage = 42;
    CHECK(jpb_game_ApplyLevelDifficulty(0, 2) == 0);
    CHECK(GameStruct.AIDamage == 42);
    return 0;
}

static int test_overlay_owner_chain(void)
{
    OverlayTrace trace;
    TextureRectTrace texture_trace;
    _Material score_material = {0};
    _Material rescue_material = {0};
    _Material counter_material = {0};
    _Material credit_material = {0};
    _Material item_materials[4] = {{0}};
    Node *node;
    SCB *item_scb;
    SCB *score_scb;
    SCB *credit_scb;
    SCB *rescue_scb;
    MATRIX matrix;
    int scb_count = 0;
    int full_alpha_count = 0;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&matrix, 0, sizeof(matrix));

    currentTextAlpha = 0.0f;
    currentSpriteAlpha = 0.0f;
    textBright = 0;
    spriteBright = 0;
    UpdateBright(130.0f, 200.0f);
    CHECK(currentTextAlpha == 1.7333333f);
    CHECK(currentSpriteAlpha == 2.6666667f);
    CHECK(textBright == 1);
    CHECK(spriteBright == 2);
    currentTextAlpha = 130.0f;
    currentSpriteAlpha = 200.0f;
    UpdateBright(0.0f, 0.0f);
    CHECK(currentTextAlpha == 113.75f);
    CHECK(currentSpriteAlpha == 175.0f);
    CHECK(textBright == 113);
    CHECK(spriteBright == 175);

    CHECK(charStuff[0] == 45);
    CHECK(charStuff[1] == 46);
    CHECK(charStuff[2] == 45);
    CHECK(charStuff[3] == 48);
    CHECK(charStuff[4] == 47);
    CHECK(charStuff[5] == 45);
    CHECK(charStuff[6] == 45);
    CHECK(charStuff[7] == 45);
    CHECK(charStuff[8] == 48);
    CHECK(charStuff[9] == 46);
    jpb_TextSetPsxTextureHook(
        capture_psx_texture, &trace);
    game_DrawBigNum(42, 10, 20);
    CHECK(trace.textureCalls == 2);
    CHECK(trace.textures[0] == 0xb9);
    CHECK(trace.textures[1] == 0xb7);
    jpb_TextSetPsxTextureHook(NULL, NULL);

    meminit();
    sprite_gInitSprites();
    score_material.iw = 1024;
    score_material.ih = 512;
    credit_material.iw = 128;
    credit_material.ih = 128;
    item_materials[0].iw = 512;
    item_materials[0].ih = 512;
    item_materials[1].iw = 512;
    item_materials[1].ih = 512;
    item_materials[2].iw = 512;
    item_materials[2].ih = 512;
    item_materials[3].iw = 512;
    item_materials[3].ih = 512;
    counter_material.iw = 512;
    counter_material.ih = 512;
    rescue_material.iw = 512;
    rescue_material.ih = 512;
    effects1Handle[40] = &score_material;
    effects1Handle[43] = &counter_material;
    effects1Handle[44] = &rescue_material;
    effects1Handle[45] = &item_materials[0];
    effects1Handle[46] = &item_materials[1];
    effects1Handle[47] = &item_materials[2];
    effects1Handle[48] = &item_materials[3];
    effects1Handle[49] = &credit_material;
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    OptionStruct.overlayMode = 2;
    scaleAdjustment = 1.0f;
    LevelSelect = 1;
    GameStruct.CurrentLevel = 1;
    GameStruct.NumPlayers = 1;
    GameStruct.ModelSelect[0] = 0;
    GameStruct.maxEnergyLineLength[0] = 100;
    GameStruct.maxForceLineLength[0] = 25;
    GameStruct.mNumContinues = 2;
    GameStruct.aCharacterData[0].Score = 42;
    GameStruct.aCharacterData[0].Items = 3;
    game_gSetMaxEnergy(0, 100);
    game_gSetEnergy(0, 100);
    game_gSetMaxForce(0, 100);
    game_gSetForce(0, 100);
    GameStruct.aCharacterData[1].Energy = 100;
    gaPlayerData[0].playernum = 0;
    gaPlayerData[0].playerRoot.objectID = 0;
    currentTextAlpha = 130.0f;
    currentSpriteAlpha = 200.0f;
    textBright = 130;
    spriteBright = 200;
    trace.textCalls = 0;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);

    game_DisplayOverlay();

    CHECK(trace.textCalls == 2);
    CHECK(wcscmp(trace.text[0], L"0000042") == 0);
    CHECK(wcscmp(trace.text[1], L"3") == 0);
    CHECK(trace.textX[0] == 212);
    CHECK(trace.textY[0] == 60);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.24f, 0.0001f);
    CHECK(trace.textX[1] == 188);
    CHECK(trace.textY[1] == 368);
    CHECK_FLOAT_CLOSE(trace.textScale[1], 1.8f, 0.0001f);
    for (node = mSCBDraw[0].head;
         node != NULL;
         node = node->next) {
        SCB *scb = (SCB *)node;

        CHECK(scb->scb_cvertex.pad == 8);
        if (scb->scb_vertex0.pad == 200) {
            ++full_alpha_count;
        }
        ++scb_count;
    }
    CHECK(scb_count == 4);
    CHECK(full_alpha_count == 2);
    CHECK(find_scb_rect_with_texture(
              &score_material, 48.0f, 36.0f, 528.0f, 240.0f) == 1);
    CHECK(find_scb_rect_with_texture(
              &item_materials[0], 44.0f, 256.0f, 236.0f, 448.0f) == 1);
    CHECK(find_scb_rect_with_texture(
              &credit_material, 274.0f, 44.0f, 328.0f, 98.0f) == 1);
    CHECK(find_scb_rect_with_texture(
              &credit_material, 312.0f, 44.0f, 366.0f, 98.0f) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              211,
              151,
              527,
              164,
              0x10,
              0xfc,
              0x10,
              130) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              211,
              175,
              527,
              188,
              0x00,
              0x00,
              0xff,
              130) == 1);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    GameStruct.NumPlayers = 1;
    GameStruct.ModelSelect[0] = 0;
    GameStruct.aCharacterData[0].Items = 3;
    GameStruct.aCharacterData[0].Score = 42;
    game_gSetMaxEnergy(0, 100);
    game_gSetEnergy(0, 100);
    game_gSetMaxForce(0, 100);
    game_gSetForce(0, 100);
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    game_DrawScore(0);
    CHECK(trace.textCalls == 1);
    CHECK(wcscmp(trace.text[0], L"0000042") == 0);
    CHECK(trace.textX[0] == 106);
    CHECK(trace.textY[0] == 30);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.24f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &score_material, 24.0f, 18.0f, 264.0f, 120.0f) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              105,
              75,
              263,
              82,
              0x10,
              0xfc,
              0x10,
              130) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              105,
              87,
              263,
              94,
              0x00,
              0x00,
              0xff,
              130) == 1);
    score_scb = find_scb_with_texture_rect(
        &score_material, 24.0f, 18.0f, 264.0f, 120.0f);
    CHECK(score_scb != NULL);
    CHECK(score_scb->scb_cvertex.pad == 8);
    CHECK(score_scb->scb_vertex0.pad == 200);
    memset(&texture_trace, 0, sizeof(texture_trace));
    _RenderSprite(&matrix, score_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &score_material);
    CHECK(texture_trace.destinations[0].left == 24);
    CHECK(texture_trace.destinations[0].top == 18);
    CHECK(texture_trace.destinations[0].right == 264);
    CHECK(texture_trace.destinations[0].bottom == 120);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 1024);
    CHECK(texture_trace.sources[0].bottom == 512);
    CHECK(texture_trace.colors[0].cd == 200);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    game_DrawItems(0);
    CHECK(trace.textCalls == 1);
    CHECK(wcscmp(trace.text[0], L"3") == 0);
    CHECK(trace.textX[0] == 94);
    CHECK(trace.textY[0] == 484);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 1.8f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &item_materials[0], 22.0f, 428.0f, 118.0f, 524.0f) == 1);
    item_scb = find_scb_with_texture_rect(
        &item_materials[0], 22.0f, 428.0f, 118.0f, 524.0f);
    CHECK(item_scb != NULL);
    memset(&texture_trace, 0, sizeof(texture_trace));
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    _RenderSprite(&matrix, item_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &item_materials[0]);
    CHECK(texture_trace.destinations[0].left == 22);
    CHECK(texture_trace.destinations[0].top == 428);
    CHECK(texture_trace.destinations[0].right == 118);
    CHECK(texture_trace.destinations[0].bottom == 524);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 512);
    CHECK(texture_trace.sources[0].bottom == 512);
    CHECK(texture_trace.colors[0].cd == 200);

    {
        static const struct {
            int model;
            _Material *material;
            unsigned spriteType;
        } item_cases[] = {
            {0, NULL, 45},
            {1, NULL, 46},
            {4, NULL, 47},
            {3, NULL, 48},
            {8, NULL, 48},
            {9, NULL, 46},
            {12, NULL, 46}
        };
        const _Material *expected_materials[] = {
            &item_materials[0],
            &item_materials[1],
            &item_materials[2],
            &item_materials[3],
            &item_materials[3],
            &item_materials[1],
            &item_materials[1]
        };
        size_t item_case;

        for (item_case = 0;
             item_case < sizeof(item_cases) / sizeof(item_cases[0]);
             ++item_case) {
            int model = item_cases[item_case].model;
            unsigned charstuff_index = model >= 9 ? 9u : (unsigned)model;
            const _Material *expected_material =
                expected_materials[item_case];

            memset(&trace, 0, sizeof(trace));
            GameStruct.ModelSelect[0] = model;
            GameStruct.aCharacterData[0].Items = (int)item_case;
            game_DrawItems(0);
            CHECK(trace.textCalls == 1);
            CHECK(trace.textX[0] == 94);
            CHECK(trace.textY[0] == 484);
            CHECK(item_scb->scb_Texture == expected_material);
            CHECK(charStuff[charstuff_index] ==
                  item_cases[item_case].spriteType);
            CHECK(effects1Handle[charStuff[charstuff_index]] ==
                  expected_material);
            CHECK(item_scb->scb_cvertex.pad == 8);
            CHECK(item_scb->scb_vertex0.pad == 200);
            CHECK(item_scb->scb_vertex0.vx == 22.0f);
            CHECK(item_scb->scb_vertex0.vy == 428.0f);
            CHECK(item_scb->scb_vertex3.vx == 118.0f);
            CHECK(item_scb->scb_vertex3.vy == 524.0f);

            memset(&texture_trace, 0, sizeof(texture_trace));
            _RenderSprite(&matrix, item_scb);
            CHECK(texture_trace.calls == 1);
            CHECK(texture_trace.materials[0] == expected_material);
            CHECK(texture_trace.destinations[0].left == 22);
            CHECK(texture_trace.destinations[0].top == 428);
            CHECK(texture_trace.destinations[0].right == 118);
            CHECK(texture_trace.destinations[0].bottom == 524);
            CHECK(texture_trace.hasSource[0] == 1);
            CHECK(texture_trace.sources[0].left == 0);
            CHECK(texture_trace.sources[0].top == 0);
            CHECK(texture_trace.sources[0].right == 512);
            CHECK(texture_trace.sources[0].bottom == 512);
            CHECK(texture_trace.colors[0].cd == 200);
        }
    }

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    GameStruct.NumPlayers = 2;
    GameStruct.ModelSelect[1] = 1;
    GameStruct.aCharacterData[1].Items = 4;
    GameStruct.aCharacterData[1].Score = 9001;
    gaPlayerData[1].playernum = 1;
    gaPlayerData[1].playerRoot.objectID = 1;
    GameStruct.maxEnergyLineLength[1] = 100;
    GameStruct.maxForceLineLength[1] = 25;
    game_gSetMaxEnergy(1, 100);
    game_gSetEnergy(1, 100);
    game_gSetMaxForce(1, 100);
    game_gSetForce(1, 100);
    game_DrawScore(1);
    CHECK(trace.textCalls == 1);
    CHECK(wcscmp(trace.text[0], L"0009001") == 0);
    CHECK(trace.textX[0] == 708);
    CHECK(trace.textY[0] == 30);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.24f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &score_material, 936.0f, 18.0f, 696.0f, 120.0f) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              696,
              75,
              854,
              82,
              0x10,
              0xfc,
              0x10,
              130) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              696,
              87,
              854,
              94,
              0x00,
              0x00,
              0xff,
              130) == 1);
    score_scb = find_scb_with_texture_rect(
        &score_material, 936.0f, 18.0f, 696.0f, 120.0f);
    CHECK(score_scb != NULL);
    CHECK(score_scb->scb_cvertex.pad == 8);
    CHECK(score_scb->scb_vertex0.pad == 200);
    memset(&texture_trace, 0, sizeof(texture_trace));
    _RenderSprite(&matrix, score_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &score_material);
    CHECK(texture_trace.destinations[0].left == 696);
    CHECK(texture_trace.destinations[0].top == 18);
    CHECK(texture_trace.destinations[0].right == 936);
    CHECK(texture_trace.destinations[0].bottom == 120);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 1024);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 0);
    CHECK(texture_trace.sources[0].bottom == 512);
    CHECK(texture_trace.colors[0].cd == 200);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    game_DrawItems(1);
    CHECK(trace.textCalls == 1);
    CHECK(wcscmp(trace.text[0], L"4") == 0);
    CHECK(trace.textX[0] == 914);
    CHECK(trace.textY[0] == 484);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 1.8f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &item_materials[1],
              842.0f,
              428.0f,
              938.0f,
              524.0f) == 1);
    item_scb = find_scb_with_texture_rect(
        &item_materials[1],
        842.0f,
        428.0f,
        938.0f,
        524.0f);
    CHECK(item_scb != NULL);
    CHECK(item_scb->scb_cvertex.pad == 8);
    CHECK(item_scb->scb_vertex0.pad == 200);
    memset(&texture_trace, 0, sizeof(texture_trace));
    _RenderSprite(&matrix, item_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &item_materials[1]);
    CHECK(texture_trace.destinations[0].left == 842);
    CHECK(texture_trace.destinations[0].top == 428);
    CHECK(texture_trace.destinations[0].right == 938);
    CHECK(texture_trace.destinations[0].bottom == 524);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 512);
    CHECK(texture_trace.sources[0].bottom == 512);
    CHECK(texture_trace.colors[0].cd == 200);

    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    GameStruct.mNumContinues = 6;
    GameStruct.ContinuesUsed = 0;
    GameStruct.NumPlayers = 1;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[1].Energy = 100;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    game_DisplayOverlay();
    CHECK(find_scb_rect_with_texture(
              &credit_material, 217.0f, 44.0f, 271.0f, 98.0f) == 1);
    CHECK(find_scb_rect_with_texture(
              &credit_material, 293.0f, 82.0f, 347.0f, 136.0f) == 1);
    credit_scb = find_scb_with_texture_rect(
        &credit_material, 217.0f, 44.0f, 271.0f, 98.0f);
    CHECK(credit_scb != NULL);
    CHECK(credit_scb->scb_cvertex.pad == 8);
    CHECK(credit_scb->scb_vertex0.pad == 10);
    memset(&texture_trace, 0, sizeof(texture_trace));
    _RenderSprite(&matrix, credit_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &credit_material);
    CHECK(texture_trace.destinations[0].left == 217);
    CHECK(texture_trace.destinations[0].top == 44);
    CHECK(texture_trace.destinations[0].right == 271);
    CHECK(texture_trace.destinations[0].bottom == 98);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 128);
    CHECK(texture_trace.sources[0].bottom == 128);
    CHECK(texture_trace.colors[0].cd == 10);
    credit_scb = find_scb_with_texture_rect(
        &credit_material, 293.0f, 82.0f, 347.0f, 136.0f);
    CHECK(credit_scb != NULL);
    CHECK(credit_scb->scb_cvertex.pad == 8);
    CHECK(credit_scb->scb_vertex0.pad == 0);
    memset(&texture_trace, 0, sizeof(texture_trace));
    _RenderSprite(&matrix, credit_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &credit_material);
    CHECK(texture_trace.destinations[0].left == 293);
    CHECK(texture_trace.destinations[0].top == 82);
    CHECK(texture_trace.destinations[0].right == 347);
    CHECK(texture_trace.destinations[0].bottom == 136);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 128);
    CHECK(texture_trace.sources[0].bottom == 128);
    CHECK(texture_trace.colors[0].cd == 0);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    GameStruct.mNumContinues = 2;
    GameStruct.ContinuesUsed = 0;
    GameStruct.NumPlayers = 1;
    GameStruct.aCharacterData[0].Energy = 10;
    GameStruct.aCharacterData[1].Energy = 100;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    game_DisplayOverlay();
    credit_scb = find_scb_with_texture_rect(
        &credit_material, 274.0f, 44.0f, 328.0f, 98.0f);
    CHECK(credit_scb != NULL);
    CHECK(credit_scb->scb_cvertex.pad == 8);
    CHECK(credit_scb->scb_vertex0.pad == 20);
    credit_scb = find_scb_with_texture_rect(
        &credit_material, 312.0f, 44.0f, 366.0f, 98.0f);
    CHECK(credit_scb != NULL);
    CHECK(credit_scb->scb_cvertex.pad == 2);
    CHECK(credit_scb->scb_vertex0.pad == 3);
    memset(&texture_trace, 0, sizeof(texture_trace));
    _RenderSprite(&matrix, credit_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &credit_material);
    CHECK(texture_trace.destinations[0].left == 312);
    CHECK(texture_trace.destinations[0].top == 44);
    CHECK(texture_trace.destinations[0].right == 366);
    CHECK(texture_trace.destinations[0].bottom == 98);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 128);
    CHECK(texture_trace.sources[0].bottom == 128);
    CHECK(texture_trace.colors[0].cd == 3);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    GameStruct.mNumContinues = 2;
    GameStruct.ContinuesUsed = 0;
    GameStruct.NumPlayers = 2;
    GameStruct.ModelSelect[1] = 1;
    GameStruct.maxEnergyLineLength[1] = 100;
    GameStruct.maxForceLineLength[1] = 25;
    game_gSetMaxEnergy(1, 100);
    game_gSetEnergy(1, 100);
    game_gSetMaxForce(1, 100);
    game_gSetForce(1, 100);
    GameStruct.aCharacterData[1].Score = 9001;
    GameStruct.aCharacterData[1].Items = 4;
    gaPlayerData[1].playernum = 1;
    gaPlayerData[1].playerRoot.objectID = 1;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    game_DisplayOverlay();
    CHECK(trace.textCalls == 4);
    CHECK(wcscmp(trace.text[2], L"0009001") == 0);
    CHECK(trace.textX[2] == 136);
    CHECK(trace.textY[2] == 60);
    CHECK_FLOAT_CLOSE(trace.textScale[2], 3.24f, 0.0001f);
    CHECK(wcscmp(trace.text[3], L"4") == 0);
    CHECK(trace.textX[3] == 548);
    CHECK(trace.textY[3] == 368);
    CHECK_FLOAT_CLOSE(trace.textScale[3], 1.8f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &score_material, 592.0f, 36.0f, 112.0f, 240.0f) == 1);
    CHECK(find_scb_rect_with_texture(
              &item_materials[1], 404.0f, 256.0f, 596.0f, 448.0f) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              113,
              151,
              429,
              164,
              0x10,
              0xfc,
              0x10,
              130) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              113,
              175,
              429,
              188,
              0x00,
              0x00,
              0xff,
              130) == 1);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    GameStruct.NumPlayers = 1;
    GameStruct.ModelSelect[0] = 0;
    GameStruct.aCharacterData[0].Items = 3;
    OptionStruct.overlayMode = 1;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    game_DisplayOverlay();
    CHECK(trace.textCalls == 2);
    CHECK(wcscmp(trace.text[0], L"0000042") == 0);
    CHECK(trace.textX[0] == 48);
    CHECK(trace.textY[0] == 62);
    CHECK(trace.textMode[0] == 0);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.24f, 0.0001f);
    CHECK(wcscmp(trace.text[1], L"3") == 0);
    CHECK(trace.textX[1] == 188);
    CHECK(trace.textY[1] == 368);
    CHECK(trace.textMode[1] == 0);
    CHECK_FLOAT_CLOSE(trace.textScale[1], 1.8f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &item_materials[0], 44.0f, 256.0f, 236.0f, 448.0f) == 1);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    GameStruct.NumPlayers = 1;
    GameStruct.ModelSelect[0] = 0;
    GameStruct.aCharacterData[0].Items = 3;
    OptionStruct.overlayMode = 1;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    game_DisplayOverlay();
    CHECK(trace.textCalls == 2);
    CHECK(wcscmp(trace.text[0], L"0000042") == 0);
    CHECK(trace.textX[0] == 24);
    CHECK(trace.textY[0] == 31);
    CHECK(trace.textMode[0] == 0);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.24f, 0.0001f);
    CHECK(wcscmp(trace.text[1], L"3") == 0);
    CHECK(trace.textX[1] == 94);
    CHECK(trace.textY[1] == 484);
    CHECK(trace.textMode[1] == 0);
    CHECK_FLOAT_CLOSE(trace.textScale[1], 1.8f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &item_materials[0], 22.0f, 428.0f, 118.0f, 524.0f) == 1);
    item_scb = find_scb_with_texture_rect(
        &item_materials[0], 22.0f, 428.0f, 118.0f, 524.0f);
    CHECK(item_scb != NULL);
    CHECK(item_scb->scb_cvertex.pad == 8);
    CHECK(item_scb->scb_vertex0.pad == 200);
    memset(&texture_trace, 0, sizeof(texture_trace));
    _RenderSprite(&matrix, item_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &item_materials[0]);
    CHECK(texture_trace.destinations[0].left == 22);
    CHECK(texture_trace.destinations[0].top == 428);
    CHECK(texture_trace.destinations[0].right == 118);
    CHECK(texture_trace.destinations[0].bottom == 524);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 512);
    CHECK(texture_trace.sources[0].bottom == 512);
    CHECK(texture_trace.colors[0].cd == 200);
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;

    sprite_gInitSprites();
    memset(&trace, 0, sizeof(trace));
    GameStruct.NumPlayers = 2;
    GameStruct.aCharacterData[1].Score = 9001;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    game_DisplayOverlay();
    CHECK(trace.textCalls == 4);
    CHECK(wcscmp(trace.text[2], L"0009001") == 0);
    CHECK(trace.textX[2] == 592);
    CHECK(trace.textY[2] == 62);
    CHECK(trace.textMode[2] == 1);
    CHECK_FLOAT_CLOSE(trace.textScale[2], 3.24f, 0.0001f);

    sprite_gInitSprites();
    memset(&trace, 0, sizeof(trace));
    GameStruct.NumPlayers = 1;
    OptionStruct.overlayMode = 2;
    GameStruct.GameState = UINT32_C(0x01000000);
    GameStruct.Counter = 7;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    game_DisplayOverlay();
    CHECK(trace.textCalls == 3);
    CHECK(wcscmp(trace.text[2], L"7") == 0);
    CHECK(trace.textX[2] == 368);
    CHECK(trace.textY[2] == 368);
    CHECK_FLOAT_CLOSE(trace.textScale[2], 1.8f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &counter_material, 224.0f, 256.0f, 416.0f, 448.0f) == 1);
    rescue_scb = find_scb_with_texture_rect(
        &counter_material, 224.0f, 256.0f, 416.0f, 448.0f);
    CHECK(rescue_scb != NULL);
    CHECK(rescue_scb->scb_cvertex.pad == 8);
    CHECK(rescue_scb->scb_vertex0.pad == 200);
    memset(&texture_trace, 0, sizeof(texture_trace));
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    _RenderSprite(&matrix, rescue_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &counter_material);
    CHECK(texture_trace.destinations[0].left == 224);
    CHECK(texture_trace.destinations[0].top == 256);
    CHECK(texture_trace.destinations[0].right == 416);
    CHECK(texture_trace.destinations[0].bottom == 448);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 512);
    CHECK(texture_trace.sources[0].bottom == 512);
    CHECK(texture_trace.colors[0].cd == 200);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    GameStruct.CurrentLevel = 3;
    GameStruct.GameState = UINT32_C(0x01000000);
    GameStruct.Counter = 5;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    game_DisplayOverlay();
    CHECK(trace.textCalls == 3);
    CHECK(wcscmp(trace.text[2], L"5") == 0);
    CHECK(trace.textX[2] == 368);
    CHECK(trace.textY[2] == 368);
    CHECK_FLOAT_CLOSE(trace.textScale[2], 1.8f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &rescue_material, 224.0f, 256.0f, 416.0f, 448.0f) == 1);
    rescue_scb = find_scb_with_texture_rect(
        &rescue_material, 224.0f, 256.0f, 416.0f, 448.0f);
    CHECK(rescue_scb != NULL);
    CHECK(rescue_scb->scb_cvertex.pad == 8);
    CHECK(rescue_scb->scb_vertex0.pad == 200);
    memset(&texture_trace, 0, sizeof(texture_trace));
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    _RenderSprite(&matrix, rescue_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &rescue_material);
    CHECK(texture_trace.destinations[0].left == 224);
    CHECK(texture_trace.destinations[0].top == 256);
    CHECK(texture_trace.destinations[0].right == 416);
    CHECK(texture_trace.destinations[0].bottom == 448);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 512);
    CHECK(texture_trace.sources[0].bottom == 512);
    CHECK(texture_trace.colors[0].cd == 200);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    GameStruct.GameState = 0;
    GameStruct.CurrentLevel = 1;
    LevelSelect = 11;
    gPilotDeathCount = 7;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    game_DisplayOverlay();
    CHECK(trace.textCalls == 2);
    CHECK(wcscmp(trace.text[1], L"7") == 0);
    CHECK(trace.textX[1] == 368);
    CHECK(trace.textY[1] == 368);
    CHECK(trace.textMode[1] == 0);
    CHECK_FLOAT_CLOSE(trace.textScale[1], 1.8f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &counter_material, 224.0f, 256.0f, 416.0f, 448.0f) == 1);
    rescue_scb = find_scb_with_texture_rect(
        &counter_material, 224.0f, 256.0f, 416.0f, 448.0f);
    CHECK(rescue_scb != NULL);
    CHECK(rescue_scb->scb_cvertex.pad == 8);
    CHECK(rescue_scb->scb_vertex0.pad == 200);

    memset(&trace, 0, sizeof(trace));
    gPilotDeathCount = 11;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    game_DisplayOverlay();
    CHECK(trace.textCalls == 2);
    CHECK(wcscmp(trace.text[1], L"11") == 0);
    CHECK(trace.textX[1] == 377);
    CHECK(trace.textY[1] == 371);
    CHECK_FLOAT_CLOSE(trace.textScale[1], 1.5f, 0.0001f);
    CHECK(find_scb_rect_with_texture(
              &counter_material, 224.0f, 256.0f, 416.0f, 448.0f) == 1);
    rescue_scb = find_scb_with_texture_rect(
        &counter_material, 224.0f, 256.0f, 416.0f, 448.0f);
    CHECK(rescue_scb != NULL);
    CHECK(rescue_scb->scb_cvertex.pad == 8);
    CHECK(rescue_scb->scb_vertex0.pad == 200);
    jpb_TextSetDrawHook(NULL, NULL);
    return 0;
}

static int test_overlay_hud_gate_boundaries(void)
{
    OverlayTrace trace;
    TextureRectTrace texture_trace;
    _Material score_material = {0};
    _Material item_material = {0};
    _Material credit_material = {0};
    SCB *score_scb;
    SCB *item_scb;
    SCB *credit_scb;
    MATRIX matrix;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&matrix, 0, sizeof(matrix));
    meminit();
    sprite_gInitSprites();
    jpb_GameResetOverlayScbs();
    score_material.iw = 1024;
    score_material.ih = 512;
    item_material.iw = 512;
    item_material.ih = 512;
    credit_material.iw = 128;
    credit_material.ih = 128;
    effects1Handle[40] = &score_material;
    effects1Handle[45] = &item_material;
    effects1Handle[49] = &credit_material;
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    OptionStruct.overlayMode = 2;
    scaleAdjustment = 1.0f;
    LevelSelect = 1;
    GameStruct.CurrentLevel = 1;
    GameStruct.NumPlayers = 1;
    GameStruct.mNumContinues = 1;
    GameStruct.ModelSelect[0] = 0;
    GameStruct.aCharacterData[0].Score = 42;
    GameStruct.aCharacterData[0].Items = 3;
    game_gSetMaxEnergy(0, 100);
    game_gSetEnergy(0, 100);
    game_gSetMaxForce(0, 100);
    game_gSetForce(0, 100);
    gaPlayerData[0].playernum = 0;
    gaPlayerData[0].playerRoot.objectID = 0;
    currentTextAlpha = 130.0f;
    currentSpriteAlpha = 200.0f;
    textBright = 130;
    spriteBright = 200;

    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);
    game_DisplayOverlay();
    CHECK(trace.textCalls == 2);
    CHECK(find_overlay_text(&trace, L"0000042", 0, 212, 60, 3.24f) == 1);
    CHECK(find_overlay_text(&trace, L"3", 0, 188, 368, 1.8f) == 1);
    score_scb = find_scb_with_texture_rect(
        &score_material, 48.0f, 36.0f, 528.0f, 240.0f);
    item_scb = find_scb_with_texture_rect(
        &item_material, 44.0f, 256.0f, 236.0f, 448.0f);
    credit_scb = find_first_scb_with_texture(&credit_material);
    CHECK(score_scb != NULL);
    CHECK(item_scb != NULL);
    CHECK(credit_scb != NULL);
    CHECK((score_scb->scb_flags & 0x40) == 0);
    CHECK((item_scb->scb_flags & 0x40) == 0);
    CHECK((credit_scb->scb_flags & 0x40) == 0);
    memset(&texture_trace, 0, sizeof(texture_trace));
    _RenderSprite(&matrix, item_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &item_material);
    CHECK(texture_trace.destinations[0].left == 44);
    CHECK(texture_trace.destinations[0].top == 256);
    CHECK(texture_trace.destinations[0].right == 236);
    CHECK(texture_trace.destinations[0].bottom == 448);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 512);
    CHECK(texture_trace.sources[0].bottom == 512);
    CHECK(texture_trace.colors[0].cd == 200);
    memset(&texture_trace, 0, sizeof(texture_trace));
    _RenderSprite(&matrix, credit_scb);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &credit_material);
    CHECK(texture_trace.hasSource[0] == 1);
    CHECK(texture_trace.sources[0].left == 0);
    CHECK(texture_trace.sources[0].top == 0);
    CHECK(texture_trace.sources[0].right == 128);
    CHECK(texture_trace.sources[0].bottom == 128);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    OptionStruct.overlayMode = 0;
    game_DisplayOverlay();
    CHECK(trace.textCalls == 0);
    CHECK((score_scb->scb_flags & 0x40) == 0);
    CHECK((item_scb->scb_flags & 0x40) == 0);
    CHECK((credit_scb->scb_flags & 0x40) == 0);

    memset(&trace, 0, sizeof(trace));
    meminit();
    sprite_gInitSprites();
    jpb_GameResetOverlayScbs();
    effects1Handle[40] = &score_material;
    effects1Handle[45] = &item_material;
    effects1Handle[49] = &credit_material;
    LevelSelect = 13;
    OptionStruct.overlayMode = 2;
    game_DrawScore(0);
    game_DrawItems(0);
    CHECK(trace.textCalls == 0);
    CHECK(find_first_scb_with_texture(&score_material) == NULL);
    CHECK(find_first_scb_with_texture(&item_material) == NULL);

    memset(&trace, 0, sizeof(trace));
    LevelSelect = 11;
    game_DrawItems(0);
    CHECK(trace.textCalls == 0);
    CHECK(find_first_scb_with_texture(&item_material) == NULL);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    meminit();
    sprite_gInitSprites();
    jpb_GameResetOverlayScbs();
    effects1Handle[40] = &score_material;
    effects1Handle[45] = &item_material;
    effects1Handle[49] = &credit_material;
    LevelSelect = 1;
    GameStruct.screenShotFlag = 0;
    game_DisplayOverlay();
    score_scb = find_scb_with_texture_rect(
        &score_material, 48.0f, 36.0f, 528.0f, 240.0f);
    item_scb = find_scb_with_texture_rect(
        &item_material, 44.0f, 256.0f, 236.0f, 448.0f);
    credit_scb = find_first_scb_with_texture(&credit_material);
    CHECK(score_scb != NULL);
    CHECK(item_scb != NULL);
    CHECK(credit_scb != NULL);
    memset(&trace, 0, sizeof(trace));
    LevelSelect = 1;
    GameStruct.screenShotFlag = 2;
    game_DisplayOverlay();
    CHECK(trace.textCalls == 2);
    CHECK(find_overlay_text(&trace, L"0000042", 0, 212, 60, 3.24f) == 1);
    CHECK(find_overlay_text(&trace, L"3", 0, 188, 368, 1.8f) == 1);
    CHECK(trace.textAlpha[0] == 113);
    CHECK(trace.textAlpha[1] == 113);
    CHECK((score_scb->scb_flags & 0x40) == 0);
    CHECK((item_scb->scb_flags & 0x40) == 0);
    CHECK((credit_scb->scb_flags & 0x40) == 0);
    CHECK(score_scb->scb_vertex0.pad == 175);
    CHECK(item_scb->scb_vertex0.pad == 175);
    CHECK(textBright == 113);
    CHECK(spriteBright == 175);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    return 0;
}

static int test_life_tile_2d_owner(void)
{
    TextureRectTrace texture_trace;
    BarTrace bar_trace;

    reset_game_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&bar_trace, 0, sizeof(bar_trace));
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    LevelSelect = 1;
    GameStruct.CurrentLevel = 1;
    gaPlayerData[0].playerRoot.objectID = 0;
    gaPlayerData[0].playernum = 0;
    gaPlayerData[1].playerRoot.objectID = 1;
    gaPlayerData[1].playernum = 1;
    game_gSetMaxEnergy(0, 100);
    game_gSetEnergy(0, 50);
    GameStruct.maxForceLineLength[0] = 25;
    GameStruct.ModelSelect[0] = 0;
    game_gSetMaxForce(0, 100);
    game_gSetForce(0, 25);
    game_gSetMaxEnergy(1, 100);
    game_gSetEnergy(1, 50);
    GameStruct.maxForceLineLength[1] = 25;
    GameStruct.ModelSelect[1] = 1;
    game_gSetMaxForce(1, 100);
    game_gSetForce(1, 25);
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &texture_trace);

    _AddLifeTile2D(&gaPlayerData[0], 100, 200, 77);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              100,
              200,
              258,
              213,
              0x10,
              0xfc,
              0x10,
              77) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              100,
              224,
              175,
              237,
              0x00,
              0x00,
              0xff,
              77) == 1);

    memset(&texture_trace, 0, sizeof(texture_trace));
    _AddLifeTile2D(&gaPlayerData[1], 500, 200, 88);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              342,
              200,
              500,
              213,
              0x10,
              0xfc,
              0x10,
              88) == 1);
    CHECK(find_texture_rect(
              &texture_trace,
              NULL,
              424,
              224,
              500,
              237,
              0x00,
              0x00,
              0xff,
              88) == 1);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    jpb_GameSetBarHook(capture_bar, &bar_trace);
    _AddBar(11, 22, 33, 44, INT32_C(0x00123456));
    CHECK(bar_trace.calls == 1);
    CHECK(bar_trace.x[0] == 11);
    CHECK(bar_trace.y[0] == 22);
    CHECK(bar_trace.width[0] == 33);
    CHECK(bar_trace.height[0] == 44);
    CHECK(bar_trace.color[0] == UINT32_C(0x7f123456));
    jpb_GameSetBarHook(NULL, NULL);
    return 0;
}

static int test_menu_big_numbers_and_mini4(void)
{
    BigNumberTrace trace;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    jpb_TextSetPsxTextureHook(
        capture_big_number_texture, &trace);

    CHECK(menu_drawBigNum(7, 10, 20, 1, 2, 3) == 5);
    CHECK(trace.calls == 1);
    CHECK(trace.texture[0] == 0xbc);
    CHECK(trace.x[0] == 10.0f);
    CHECK(trace.y[0] == 20.0f);
    CHECK(trace.width[0] == 0.0f);
    CHECK(trace.height[0] == 0.0f);
    CHECK(trace.transparency[0] == 0xff);
    CHECK(trace.red[0] == 1);
    CHECK(trace.green[0] == 2);
    CHECK(trace.blue[0] == 3);

    memset(&trace, 0, sizeof(trace));
    game_DrawBigNum(42, 10, 20);
    CHECK(trace.calls == 2);
    CHECK(trace.texture[0] == 0xb9);
    CHECK(trace.texture[1] == 0xb7);
    CHECK(trace.x[0] == 10.0f);
    CHECK(trace.y[0] == 20.0f);
    CHECK(trace.x[1] == 28.0f);
    CHECK(trace.y[1] == 20.0f);
    CHECK(trace.width[0] == 16.0f);
    CHECK(trace.height[0] == 14.0f);
    CHECK(trace.width[1] == 16.0f);
    CHECK(trace.height[1] == 14.0f);
    CHECK(trace.transparency[0] == 200);
    CHECK(trace.transparency[1] == 200);
    CHECK(trace.red[0] == 0x80);
    CHECK(trace.green[0] == 0x80);
    CHECK(trace.blue[0] == 0x80);

    memset(&trace, 0, sizeof(trace));
    game_DrawBigNum(7, 100, 50);
    CHECK(trace.calls == 2);
    CHECK(trace.texture[0] == 0xb5);
    CHECK(trace.texture[1] == 0xbc);
    CHECK(trace.x[0] == 100.0f);
    CHECK(trace.x[1] == 118.0f);

    memset(&trace, 0, sizeof(trace));
    menu_drawBigNums(42, 4, 100, 50, 9, 8, 7);
    CHECK(trace.calls == 4);
    CHECK(trace.texture[0] == 0xb5);
    CHECK(trace.texture[1] == 0xb5);
    CHECK(trace.texture[2] == 0xb9);
    CHECK(trace.texture[3] == 0xb7);
    CHECK(trace.x[0] == 100.0f);
    CHECK(trace.y[0] == 50.0f);
    CHECK(trace.x[1] == 105.0f);
    CHECK(trace.y[1] == 50.0f);
    CHECK(trace.x[2] == 110.0f);
    CHECK(trace.y[2] == 50.0f);
    CHECK(trace.x[3] == 115.0f);
    CHECK(trace.y[3] == 50.0f);
    CHECK(trace.width[0] == 0.0f);
    CHECK(trace.height[0] == 0.0f);
    CHECK(trace.transparency[0] == 0xff);
    CHECK(trace.transparency[1] == 0xff);
    CHECK(trace.transparency[2] == 0xff);
    CHECK(trace.transparency[3] == 0xff);
    CHECK(trace.red[0] == 9);
    CHECK(trace.green[0] == 8);
    CHECK(trace.blue[0] == 7);

    memset(&trace, 0, sizeof(trace));
    zerobss_levelReset = 73;
    gDeathCount = 25;
    gPilotDeathCount = 11;
    level_Mini4();
    CHECK(zerobss_levelReset == 0);
    CHECK(gDeathCount == 0);
    CHECK(gPilotDeathCount == 0);
    CHECK(trace.calls == 3);
    CHECK(trace.texture[0] == 0xb6);
    CHECK(trace.texture[1] == 0xb5);
    CHECK(trace.texture[2] == 0xb5);
    CHECK(trace.x[0] == 240.0f);
    CHECK(trace.y[0] == 184.0f);
    CHECK(trace.x[1] == 245.0f);
    CHECK(trace.y[1] == 184.0f);
    CHECK(trace.x[2] == 250.0f);
    CHECK(trace.y[2] == 184.0f);
    CHECK(trace.width[0] == 0.0f);
    CHECK(trace.height[0] == 0.0f);
    CHECK(trace.transparency[0] == 0xff);
    CHECK(trace.transparency[1] == 0xff);
    CHECK(trace.transparency[2] == 0xff);
    CHECK(trace.red[0] == 0x80);
    CHECK(trace.green[0] == 0x80);
    CHECK(trace.blue[0] == 0x80);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    zerobss_levelReset = 74;
    gDeathCount = 25;
    gPilotDeathCount = 11;
    level_Mini4();
    CHECK(trace.calls == 3);
    CHECK(trace.texture[0] == 0xb6);
    CHECK(trace.texture[1] == 0xb5);
    CHECK(trace.texture[2] == 0xb5);
    CHECK(trace.x[0] == 240.0f);
    CHECK(trace.y[0] == 184.0f);
    CHECK(trace.x[1] == 245.0f);
    CHECK(trace.y[1] == 184.0f);
    CHECK(trace.x[2] == 250.0f);
    CHECK(trace.y[2] == 184.0f);
    CHECK(trace.width[0] == 0.0f);
    CHECK(trace.height[0] == 0.0f);
    CHECK(trace.transparency[0] == 0xff);
    CHECK(trace.red[0] == 0x80);
    CHECK(trace.green[0] == 0x80);
    CHECK(trace.blue[0] == 0x80);
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;

    /* The shared reset pulse settles on the following frame. */
    level_Mini4();
    memset(&trace, 0, sizeof(trace));
    gDeathCount = 100;
    secretBits = 0;
    level_Mini4();
    CHECK(trace.calls == 0);
    CHECK((secretBits & UINT32_C(0x100)) != 0);

    GameStruct.GameState = UINT32_C(0x02000000);
    zerobss_levelReset = 91;
    gDeathCount = 40;
    level_Mini4();
    CHECK(zerobss_levelReset == 91);
    CHECK(gDeathCount == 40);

    jpb_TextSetPsxTextureHook(NULL, NULL);
    return 0;
}

static int test_psx_texture_hud_leaf_contract(void)
{
    TextureRectTrace trace;
    _Material digit_material = {0};
    int width;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    memset(menuTextures, 0, sizeof(menuTextures));
    memset(fontSpec, 0, sizeof(fontSpec));
    digit_material.iw = 256;
    digit_material.ih = 128;
    menuTextures[3] = &digit_material;
    fontSpec[0xb5].clut = 3;
    fontSpec[0xb5].x = 7;
    fontSpec[0xb5].y = 11;
    fontSpec[0xb5].w = 4;
    fontSpec[0xb5].h = 5;
    gPSXDrawScaleX = 2.0f;
    gPSXDrawScaleY = 3.0f;
    gPSXDrawScaleW = 4.0f;
    gPSXDrawScaleH = 5.0f;
    scaleAdjustmentMM = 0.5f;
    frontRGBoff = 10;
    jpb_WHookSetDrawTextureHook(capture_texture_rect, &trace);

    width = jpb_PsxDrawTextureLayer(
        0xb5,
        6.0f,
        8.0f,
        0.0f,
        0.0f,
        0x8400,
        240,
        7,
        8,
        0.33f);
    CHECK(width == 8);
    CHECK(trace.calls == 1);
    CHECK(trace.materials[0] == &digit_material);
    CHECK(trace.destinations[0].left == 12);
    CHECK(trace.destinations[0].top == 24);
    CHECK(trace.destinations[0].right == 20);
    CHECK(trace.destinations[0].bottom == 36);
    CHECK(trace.hasSource[0] == 1);
    CHECK(trace.sources[0].left == 28);
    CHECK(trace.sources[0].top == 44);
    CHECK(trace.sources[0].right == 44);
    CHECK(trace.sources[0].bottom == 64);
    CHECK(trace.colors[0].r == 250);
    CHECK(trace.colors[0].g == 17);
    CHECK(trace.colors[0].b == 18);
    CHECK(trace.colors[0].cd == 0x3f);

    memset(&trace, 0, sizeof(trace));
    frontRGBoff = 32;
    width = jpb_PsxDrawTextureLayer(
        0xb5,
        1.0f,
        2.0f,
        3.0f,
        4.0f,
        0x8100,
        250,
        250,
        250,
        0.2f);
    CHECK(width == 12);
    CHECK(trace.calls == 1);
    CHECK(trace.destinations[0].left == 2);
    CHECK(trace.destinations[0].top == 6);
    CHECK(trace.destinations[0].right == 14);
    CHECK(trace.destinations[0].bottom == 26);
    CHECK(trace.colors[0].r == 255);
    CHECK(trace.colors[0].g == 255);
    CHECK(trace.colors[0].b == 255);
    CHECK(trace.colors[0].cd == 0x7f);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    frontRGBoff = 0;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    gPSXDrawScaleW = 3.75f;
    gPSXDrawScaleH = 4.5f;
    scaleAdjustmentMM = 1.0f;
    return 0;
}

static int test_sdl_text_hud_leaf_contract(void)
{
    OverlayTrace trace;
    int result;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    scaleAdjustment = 1.125f;
    scaleAdjustmentMM = 0.5f;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);

    result = SDLTextWriteScale(
        11,
        130,
        2,
        238,
        67,
        3.24f,
        2,
        L"%07d",
        42);
    CHECK(result == 7);
    CHECK(trace.textCalls == 1);
    CHECK(wcscmp(trace.text[0], L"0000042") == 0);
    CHECK(trace.textTint[0] == 11);
    CHECK(trace.textAlpha[0] == 130);
    CHECK(trace.textMode[0] == 2);
    CHECK(trace.textX[0] == 238);
    CHECK(trace.textY[0] == 67);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.24f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.textScaleAdjustment[0], 1.125f, 0.0001f);
    CHECK(trace.textFontStyle[0] == 2);

    memset(&trace, 0, sizeof(trace));
    result = SDLTextWriteScaleMM(
        11,
        190,
        0,
        604,
        390,
        2.5f,
        2,
        L"%ls",
        L"Select Level");
    CHECK(result == 12);
    CHECK(trace.textCalls == 1);
    CHECK(wcscmp(trace.text[0], L"Select Level") == 0);
    CHECK(trace.textAlpha[0] == 190);
    CHECK(trace.textX[0] == 604);
    CHECK(trace.textY[0] == 390);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 2.5f, 0.0001f);
    CHECK_FLOAT_CLOSE(
        trace.textScaleAdjustment[0],
        0.5f,
        0.0001f);
    CHECK(trace.textFontStyle[0] == 2);

    jpb_TextSetDrawHook(NULL, NULL);
    scaleAdjustment = 1.0f;
    scaleAdjustmentMM = 1.0f;
    return 0;
}

static int test_draw_text_hud_bridge_contract(void)
{
    OverlayTrace trace;
    int result;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    scaleAdjustment = 1.125f;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);

    result = _DrawText(
        238.75f,
        67.25f,
        0.0001f,
        1.08f,
        UINT32_C(0x82010203),
        "score %07d",
        42);
    CHECK(result == 0);
    CHECK(trace.textCalls == 1);
    CHECK(wcscmp(trace.text[0], L"score 0000042") == 0);
    CHECK(trace.textTint[0] == 11);
    CHECK(trace.textAlpha[0] == 0x82);
    CHECK(trace.textMode[0] == 0);
    CHECK(trace.textX[0] == 238);
    CHECK(trace.textY[0] == 67);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.24f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.textScaleAdjustment[0], 1.125f, 0.0001f);
    CHECK(trace.textFontStyle[0] == 2);

    memset(&trace, 0, sizeof(trace));
    CHECK(_DrawText(1.0f, 2.0f, 0.0f, 1.0f, UINT32_C(0x7F000000), NULL) == 0);
    CHECK(trace.textCalls == 0);

    jpb_TextSetDrawHook(NULL, NULL);
    scaleAdjustment = 1.0f;
    return 0;
}

static int test_level_countdown_outcomes(void)
{
    OverlayTrace trace;
    int frame;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    allText[435] = L"SUCCESS";
    allText[436] = L"FAILED";
    allText[427] = L"targets";

    zerobss_levelReset = 101;
    level_CountDown(2, 0, 0);
    CHECK(trace.textCalls == 2);
    CHECK(wcscmp(trace.text[0], L"002") == 0);
    CHECK(trace.textX[0] == 231);
    CHECK(trace.textY[0] == 328);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.0f, 0.0001f);
    CHECK(wcscmp(trace.text[1], L"sec") == 0);
    CHECK(trace.textX[1] == 353);
    CHECK(trace.textY[1] == 359);
    CHECK_FLOAT_CLOSE(trace.textScale[1], 1.5f, 0.0001f);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    zerobss_levelReset = 104;
    level_CountDown(2, 0, 0);
    CHECK(trace.textCalls == 2);
    CHECK(wcscmp(trace.text[0], L"002") == 0);
    CHECK(trace.textX[0] == 435);
    CHECK(trace.textY[0] == 464);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.0f, 0.0001f);
    CHECK(wcscmp(trace.text[1], L"sec") == 0);
    CHECK(trace.textX[1] == 496);
    CHECK(trace.textY[1] == 479);
    CHECK_FLOAT_CLOSE(trace.textScale[1], 1.5f, 0.0001f);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    zerobss_levelReset = 102;
    level_CountDown(0, 3, 0);
    CHECK(trace.textCalls == 2);
    CHECK(wcscmp(trace.text[0], L"003") == 0);
    CHECK(trace.textX[0] == 48);
    CHECK(trace.textY[0] == 200);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.0f, 0.0001f);
    CHECK(wcscmp(trace.text[1], L"t") == 0);
    CHECK(trace.textX[1] == 48);
    CHECK(trace.textY[1] == 260);
    CHECK_FLOAT_CLOSE(trace.textScale[1], 1.5f, 0.0001f);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    zerobss_levelReset = 105;
    level_CountDown(0, 3, 0);
    CHECK(trace.textCalls == 2);
    CHECK(wcscmp(trace.text[0], L"003") == 0);
    CHECK(trace.textX[0] == 24);
    CHECK(trace.textY[0] == 100);
    CHECK_FLOAT_CLOSE(trace.textScale[0], 3.0f, 0.0001f);
    CHECK(wcscmp(trace.text[1], L"t") == 0);
    CHECK(trace.textX[1] == 24);
    CHECK(trace.textY[1] == 130);
    CHECK_FLOAT_CLOSE(trace.textScale[1], 1.5f, 0.0001f);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    zerobss_levelReset = 103;
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    LevelSelect = 14;
    level_CountDown(0, 0, 0);
    abGlobalBits[3] = UINT8_C(2);
    level_CountDown(0, 0, 0);
    memset(&trace, 0, sizeof(trace));
    level_CountDown(0, 0, 0);
    CHECK(find_overlay_text(
              &trace, L"SUCCESS", 2, 320, 200, 1.0f) == 1);

    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(&trace, 0, sizeof(trace));
    GameStruct.GameState = 0;
    secretBits = 0;
    LevelSelect = 14;
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    zerobss_levelReset = 106;
    level_CountDown(0, 0, 0);
    abGlobalBits[3] = UINT8_C(2);
    level_CountDown(0, 0, 0);
    memset(&trace, 0, sizeof(trace));
    level_CountDown(0, 0, 0);
    CHECK(find_overlay_text(
              &trace, L"SUCCESS", 2, 480, 100, 1.0f) == 1);

    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    for (frame = 0; frame < 254; ++frame) {
        level_CountDown(0, 0, 0);
    }
    CHECK((secretBits & UINT32_C(0x100)) != 0);
    CHECK((abGlobalBits[0] & UINT8_C(2)) != 0);

    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(&trace, 0, sizeof(trace));
    GameStruct.GameState = 0;
    secretBits = 0;
    gGlobalTimer = 100;
    zerobss_levelReset = 202;
    level_CountDown(1, 0, 0);
    level_CountDown(1, 0, 0);
    gCheckPoint = 7;
    reStartScore[0] = 999;
    gGlobalTimer = 100 + UINT32_C(0x3c00);
    level_CountDown(1, 0, 0);
    memset(&trace, 0, sizeof(trace));
    level_CountDown(1, 0, 0);
    CHECK(find_overlay_text(
              &trace, L"FAILED", 2, 320, 200, 1.0f) == 1);

    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(&trace, 0, sizeof(trace));
    GameStruct.GameState = 0;
    secretBits = 0;
    gGlobalTimer = 100;
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    zerobss_levelReset = 203;
    level_CountDown(1, 0, 0);
    level_CountDown(1, 0, 0);
    gGlobalTimer = 100 + UINT32_C(0x3c00);
    level_CountDown(1, 0, 0);
    memset(&trace, 0, sizeof(trace));
    level_CountDown(1, 0, 0);
    CHECK(find_overlay_text(
              &trace, L"FAILED", 2, 480, 100, 1.0f) == 1);

    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    for (frame = 0; frame < 254; ++frame) {
        level_CountDown(1, 0, 0);
    }
    CHECK((abGlobalBits[3] & UINT8_C(1)) != 0);
    CHECK((GameStruct.GameState & UINT32_C(0x60)) ==
          UINT32_C(0x60));
    CHECK(gCheckPoint == 0);
    CHECK(reStartScore[0] == 0);

    jpb_TextSetDrawHook(NULL, NULL);
    allText[427] = NULL;
    allText[435] = NULL;
    allText[436] = NULL;
    return 0;
}

static int test_level_hangar(void)
{
    OverlayTrace trace;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    g_levelUVScroll.vx = 0.0f;
    g_levelUVScroll.vy = 0.0f;
    gGlobalTimer = 100;
    GameStruct.Counter = 6;
    zerobss_levelReset = 303;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);

    level_Hangar();
    CHECK(zerobss_levelReset == 0);
    CHECK(pilotsKilled == 0);
    CHECK(g_levelUVScroll.vx == 0.994f);
    CHECK(g_levelUVScroll.vy == 0.99f);
    CHECK((GameStruct.GameState & UINT32_C(0x01000000)) != 0);
    CHECK((abGlobalBits[6] & UINT8_C(4)) != 0);
    CHECK(trace.textCalls == 2);
    CHECK(find_overlay_text(&trace, L"400", 0, 231, 213, 3.0f) == 1);
    CHECK(find_overlay_text(&trace, L"sec", 0, 353, 244, 1.5f) == 1);

    memset(&trace, 0, sizeof(trace));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    GameStruct.GameState = 0;
    GameStruct.Counter = 6;
    gGlobalTimer = 100;
    zerobss_levelReset = 304;
    level_Hangar();
    CHECK(trace.textCalls == 2);
    CHECK(find_overlay_text(&trace, L"400", 0, 435, 406, 3.0f) == 1);
    CHECK(find_overlay_text(&trace, L"sec", 0, 496, 422, 1.5f) == 1);

    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    /* Settle the shared reset pulse, then complete the pilot objective. */
    level_Hangar();
    memset(&trace, 0, sizeof(trace));
    gGlobalTimer = 100 + UINT32_C(0x3c00);
    pilotsKilled = 2;
    level_Hangar();
    CHECK(find_overlay_text(&trace, L"399", 0, 231, 213, 3.0f) == 1);
    CHECK(find_overlay_text(&trace, L"sec", 0, 353, 244, 1.5f) == 1);
    CHECK((abGlobalBits[0] & UINT8_C(2)) != 0);

    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(&trace, 0, sizeof(trace));
    GameStruct.GameState = 0;
    GameStruct.Counter = 4;
    gCheckPoint = 9;
    reStartScore[0] = 1234;
    zerobss_levelReset = 404;
    level_Hangar();
    level_Hangar();
    pilotsKilled = 2;
    level_Hangar();
    CHECK((abGlobalBits[3] & UINT8_C(1)) != 0);
    CHECK((GameStruct.GameState & UINT32_C(0x60)) ==
          UINT32_C(0x60));
    CHECK(gCheckPoint == 0);
    CHECK(reStartScore[0] == 0);
    jpb_TextSetDrawHook(NULL, NULL);
    return 0;
}

static int test_level_arena(void)
{
    OverlayTrace trace;
    int frame;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    allText[437] = L"ONE WINS";
    allText[438] = L"TWO WINS";
    allText[439] = L"DRAW";
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    GameStruct.mNumContinues = 3;
    GameStruct.ContinuesUsed = 3;
    GameStruct.aCharacterData[0].Score = 100;
    GameStruct.aCharacterData[1].Score = 50;
    gaPlayerData[0].playerRoot.objectID = 0;
    gaPlayerData[1].playerRoot.objectID = 1;
    afterLife = &gaPlayerData[0];
    zerobss_levelReset = 505;

    level_Arena();
    level_Arena();
    level_Arena();
    level_Arena();
    CHECK(find_overlay_text(
              &trace, L"O", 0, 192, 112, 3.0f) == 1);
    CHECK(trace.textAlpha[0] == 254);
    CHECK(trace.textTint[0] == 11);
    for (frame = 0; frame < 254; ++frame) {
        level_Arena();
    }
    CHECK(GameStruct.LevelExit == 1);
    CHECK(afterLife == NULL);

    memset(&trace, 0, sizeof(trace));
    zerobss_levelReset = 506;
    GameStruct.LevelExit = 0;
    GameStruct.mNumContinues = 3;
    GameStruct.ContinuesUsed = 3;
    GameStruct.aCharacterData[0].Score = 10;
    GameStruct.aCharacterData[1].Score = 20;
    level_Arena();
    level_Arena();
    level_Arena();
    level_Arena();
    CHECK(find_overlay_text(
              &trace, L"T", 0, 192, 112, 3.0f) == 1);
    CHECK(trace.textAlpha[0] == 254);
    CHECK(trace.textTint[0] == 11);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    zerobss_levelReset = 508;
    GameStruct.LevelExit = 0;
    GameStruct.mNumContinues = 3;
    GameStruct.ContinuesUsed = 3;
    GameStruct.aCharacterData[0].Score = 100;
    GameStruct.aCharacterData[1].Score = 50;
    level_Arena();
    level_Arena();
    level_Arena();
    level_Arena();
    CHECK(find_overlay_text(
              &trace, L"O", 0, 352, 142, 3.0f) == 1);
    CHECK(trace.textAlpha[0] == 254);
    CHECK(trace.textTint[0] == 11);

    memset(&trace, 0, sizeof(trace));
    zerobss_levelReset = 510;
    GameStruct.LevelExit = 0;
    GameStruct.mNumContinues = 3;
    GameStruct.ContinuesUsed = 3;
    GameStruct.aCharacterData[0].Score = 10;
    GameStruct.aCharacterData[1].Score = 20;
    level_Arena();
    level_Arena();
    level_Arena();
    level_Arena();
    CHECK(find_overlay_text(
              &trace, L"T", 0, 352, 142, 3.0f) == 1);
    CHECK(trace.textAlpha[0] == 254);
    CHECK(trace.textTint[0] == 11);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    zerobss_levelReset = 507;
    GameStruct.LevelExit = 0;
    GameStruct.mNumContinues = 3;
    GameStruct.ContinuesUsed = 3;
    GameStruct.aCharacterData[0].Score = 33;
    GameStruct.aCharacterData[1].Score = 33;
    level_Arena();
    level_Arena();
    level_Arena();
    level_Arena();
    CHECK(find_overlay_text(
              &trace, L"D", 0, 192, 112, 3.0f) == 1);
    CHECK(trace.textAlpha[0] == 254);
    CHECK(trace.textTint[0] == 11);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    zerobss_levelReset = 509;
    GameStruct.LevelExit = 0;
    GameStruct.mNumContinues = 3;
    GameStruct.ContinuesUsed = 3;
    GameStruct.aCharacterData[0].Score = 33;
    GameStruct.aCharacterData[1].Score = 33;
    level_Arena();
    level_Arena();
    level_Arena();
    level_Arena();
    CHECK(find_overlay_text(
              &trace, L"D", 0, 352, 142, 3.0f) == 1);
    CHECK(trace.textAlpha[0] == 254);
    CHECK(trace.textTint[0] == 11);

    reset_game_state();
    jpb_TextSetDrawHook(NULL, NULL);
    GameStruct.mNumContinues = 3;
    GameStruct.ContinuesUsed = 0;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[0].Force = 100;
    GameStruct.aCharacterData[0].MaxEnergy = 1000;
    GameStruct.aCharacterData[0].MaxForce = 1000;
    GameStruct.aCharacterData[1].Energy = 100;
    GameStruct.aCharacterData[1].Force = 100;
    GameStruct.aCharacterData[1].MaxEnergy = 1000;
    GameStruct.aCharacterData[1].MaxForce = 1000;
    gaPlayerData[0].playerRoot.objectID = -1;
    gaPlayerData[1].playerRoot.objectID = 1;
    zerobss_levelReset = 606;
    level_Arena();
    CHECK(GameStruct.aCharacterData[1].Energy == 255);
    CHECK(GameStruct.aCharacterData[1].Force == 320);

    allText[437] = NULL;
    allText[438] = NULL;
    allText[439] = NULL;
    return 0;
}

static int test_level_theed(void)
{
    WorldData world = {0};
    OverlayTrace trace;
    _Material rescue_material = {0};
    SCB *rescue_scb;

    reset_game_state();
    memset(&trace, 0, sizeof(trace));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    meminit();
    sprite_gInitSprites();
    jpb_GameResetOverlayScbs();
    rescue_material.iw = 512;
    rescue_material.ih = 512;
    effects1Handle[44] = &rescue_material;
    gpWorld = &world;
    LevelSelect = 3;
    GameStruct.CurrentLevel = 3;
    GameStruct.NumPlayers = 1;
    GameStruct.Counter = 5;
    OptionStruct.overlayMode = 2;
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    currentTextAlpha = 130.0f;
    currentSpriteAlpha = 200.0f;
    textBright = 130;
    spriteBright = 200;
    gGlobalTimer = 1234;
    zerobss_levelReset = 707;
    level_Theed();
    CHECK(zerobss_levelReset == 0);
    CHECK((GameStruct.GameState & UINT32_C(0x01000000)) != 0);
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    game_DisplayOverlay();
    CHECK(find_overlay_text(&trace, L"5", 0, 368, 368, 1.8f) == 1);
    rescue_scb = find_scb_with_texture_rect(
        &rescue_material, 224.0f, 256.0f, 416.0f, 448.0f);
    CHECK(rescue_scb != NULL);
    CHECK(rescue_scb->scb_cvertex.pad == 8);
    CHECK(rescue_scb->scb_vertex0.pad == 200);

    memset(&trace, 0, sizeof(trace));
    sprite_gInitSprites();
    jpb_GameResetOverlayScbs();
    effects1Handle[44] = &rescue_material;
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    jpb_TextSetDrawHook(capture_overlay_text, &trace);
    game_DisplayOverlay();
    CHECK(find_overlay_text(&trace, L"5", 0, 504, 484, 1.8f) == 1);
    rescue_scb = find_scb_with_texture_rect(
        &rescue_material, 432.0f, 428.0f, 528.0f, 524.0f);
    CHECK(rescue_scb != NULL);
    CHECK(rescue_scb->scb_cvertex.pad == 8);
    CHECK(rescue_scb->scb_vertex0.pad == 200);

    abGlobalBits[8] |= UINT8_C(0x40);
    GameStruct.Counter = 6;
    gCheckPoint = 11;
    reStartScore[0] = 123;
    reStartScore[1] = 456;
    level_Theed();
    CHECK((abGlobalBits[3] & UINT8_C(1)) != 0);
    CHECK((abGlobalBits[8] & UINT8_C(0x40)) == 0);
    CHECK((GameStruct.GameState & UINT32_C(0x60)) ==
          UINT32_C(0x60));
    CHECK(GameStruct.Counter == 0);
    CHECK(gCheckPoint == 0);
    CHECK(reStartScore[0] == 0);
    CHECK(reStartScore[1] == 0);
    jpb_TextSetDrawHook(NULL, NULL);
    gpWorld = NULL;
    return 0;
}

static int test_callback_table(void)
{
    funcArray[0] = brain_HangCallback;
    funcArray[49] = brain_HangCallback;
    game_setFuncArray();

    CHECK(funcArray[0] == NULL);
    CHECK(funcArray[1] == ai_FireWeapon);
    CHECK(funcArray[2] == ai_Throw);
    CHECK(funcArray[3] == brain_HangCallback);
    CHECK(funcArray[4] == brain_SkidCallBack);
    CHECK(funcArray[5] == brain_ThrowEnder);
    CHECK(funcArray[6] ==
          brainutil_PlotTrajectory);
    CHECK(funcArray[7] ==
          force_AbsorbReflectCallBack);
    CHECK(funcArray[8] ==
          force_AttackCallBack);
    CHECK(funcArray[9] ==
          force_AttackSpinCallBack);
    CHECK(funcArray[10] ==
          force_CloakCallBack);
    CHECK(funcArray[11] ==
          force_FlameCallBack);
    CHECK(funcArray[12] ==
          force_HealingCallBack);
    CHECK(funcArray[13] ==
          force_MesmerizeCallBack);
    CHECK(funcArray[14] ==
          force_PushCallBack);
    CHECK(funcArray[15] ==
          force_Ranged3CallBack);
    CHECK(funcArray[16] ==
          force_ReflectCallBack);
    CHECK(funcArray[17] ==
          force_RingCallBack);
    CHECK(funcArray[18] ==
          force_SabreSpinCallBack);
    CHECK(funcArray[19] ==
          force_SabreTossCallBack);
    CHECK(funcArray[20] ==
          force_SabreYoYoBack);
    CHECK(funcArray[21] ==
          force_ShieldCallBack);
    CHECK(funcArray[22] ==
          force_StarCallBack);
    CHECK(funcArray[23] ==
          force_TossCallBack);
    CHECK(funcArray[24] ==
          force_ZapCallBack);
    CHECK(funcArray[25] ==
          force_TossGrenadeCallBack);
    CHECK(funcArray[26] == jedi_FireWeapon);
    CHECK(funcArray[27] == maul_PushCallBack);
    CHECK(funcArray[28] == maul_RingCallBack);
    CHECK(funcArray[29] == maul_ZapCallBack);
    CHECK(funcArray[30] == ai_Tank);
    CHECK(funcArray[31] == ai_Stap);
    CHECK(funcArray[32] == jedi_Main);
    CHECK(funcArray[33] ==
          jpb_ai_MainCallback);
    CHECK(funcArray[34] == ai_Kadu);
    CHECK(funcArray[35] == ai_Thug);
    CHECK(funcArray[36] == ai_Blades);
    CHECK(funcArray[37] == ai_Maul);
    CHECK(funcArray[38] == ai_JarJar);
    CHECK(funcArray[39] == ai_Worm);
    CHECK(funcArray[40] ==
          ai_StarFighter);
    CHECK(funcArray[41] == ai_Krakis);
    CHECK(funcArray[42] ==
          ai_LoaderDroid);
    CHECK(funcArray[43] == ai_Mtt);
    CHECK(funcArray[44] ==
          ai_Destroyer);
    CHECK(funcArray[45] == ai_AAT);
    CHECK(funcArray[46] == ai_TurretDroid);
    CHECK(funcArray[47] == ai_Deadly);
    CHECK(funcArray[48] ==
          brainutil_PlotMaulTrajectory);
    CHECK(funcArray[49] == tusken_stab);
    return 0;
}

static int test_save_game_persistence(void)
{
    const char *game_path = "jpb_game_test_save.bin";
    const char *options_path = "jpb_game_test_options.bin";
    saveGameStruct expected_save;
    saveGameStruct corrupt_save;
    saveGameStruct sentinel_save;
    gamestruct sentinel_game;
    optionstruct expected_options;
    FILE *file;
    void *serialized;
    long file_size;

    (void)remove(game_path);
    (void)remove(options_path);
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&SaveGameStruct, 0, sizeof(SaveGameStruct));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    GameStruct.CurrentLevel = 7;
    GameStruct.ModelSelect[0] = 3;
    GameStruct.ModelSelect[1] = 4;
    GameStruct.NumPlayers = 2;
    GameStruct.maxEnergyLevels[3] = 125;
    GameStruct.maxEnergyLineLength[3] = 31;
    GameStruct.maxForceLevels[4] = 140;
    GameStruct.maxForceLineLength[4] = 35;
    GameStruct.jediLevelPlayed[3][7] = 1;
    GameStruct.jediScorePerLevel[3][7] = 345678;
    GameStruct.checkpoint[7] = 5;
    GameStruct.mNumContinues = 8;
    GameStruct.ContinuesUsed = 2;
    GameStruct.aCharacterData[0].Score = 123456;
    GameStruct.aCharacterData[0].Energy = 89;
    GameStruct.jediComboMask[3].m[2] = UINT8_C(0x40);
    GameStruct.AIDamage = 11;
    GameStruct.JediDamage = 12;
    GameStruct.HTHRate = 13;
    GameStruct.RangedRate = 14;
    GameStruct.BlockRate = 15;
    GameStruct.ComboLevel = 6;
    GameStruct.ForceLevel = 7;
    GameStruct.continueAble = 1;
    GameStruct.difficulty = 2;
    GameStruct.gameCompleted = 1;
    secretBits = UINT32_C(0x12345678);
    abGlobalBits[6] = UINT8_C(0xa5);
    jediUpgrades[3].healthUpgrades = 4;
    ExtraCharacters[2].Unlocked = 1;
    ExtraCharacters[5].Unlocked = 1;

    CHECK(jpb_SaveGameWriteFile(game_path) == JPB_SAVE_OK);
    CHECK(SaveGameStruct.validFlag == 1);
    CHECK(SaveGameStruct.lastlevel == 7);
    CHECK(SaveGameStruct.secretBits == UINT32_C(0x12345678));
    CHECK(SaveGameStruct.players[0] == 3);
    CHECK(SaveGameStruct.players[1] == 4);
    expected_save = SaveGameStruct;
    file = fopen(game_path, "rb");
    CHECK(file != NULL);
    CHECK(fseek(file, 0, SEEK_END) == 0);
    file_size = ftell(file);
    CHECK(fclose(file) == 0);
    CHECK(file_size == (long)sizeof(saveGameStruct));

    serialized = serializeGameStruct();
    CHECK(serialized != NULL);
    memset(&SaveGameStruct, 0, sizeof(SaveGameStruct));
    deserializeGameStruct(serialized);
    free(serialized);
    CHECK(memcmp(&SaveGameStruct, &expected_save,
                 sizeof(SaveGameStruct)) == 0);

    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&SaveGameStruct, 0, sizeof(SaveGameStruct));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    secretBits = 0;
    ExtraCharacters[2].Unlocked = 0;
    ExtraCharacters[5].Unlocked = 0;
    CHECK(jpb_SaveGameReadFile(game_path) == JPB_SAVE_OK);
    CHECK(memcmp(&SaveGameStruct, &expected_save,
                 sizeof(SaveGameStruct)) == 0);
    CHECK(GameStruct.NumPlayers == 1);
    CHECK(GameStruct.ModelSelect[0] == 3);
    CHECK(GameStruct.ModelSelect[1] == 4);
    CHECK(GameStruct.maxEnergyLevels[3] == 125);
    CHECK(GameStruct.maxForceLevels[4] == 140);
    CHECK(GameStruct.jediScorePerLevel[3][7] == 345678);
    CHECK(GameStruct.aCharacterData[0].Score == 123456);
    CHECK(GameStruct.jediComboMask[3].m[2] == UINT8_C(0x40));
    CHECK(secretBits == UINT32_C(0x12345678));
    CHECK(abGlobalBits[6] == UINT8_C(0xa5));
    CHECK(jediUpgrades[3].healthUpgrades == 4);
    CHECK(ExtraCharacters[2].Unlocked == 1);
    CHECK(ExtraCharacters[5].Unlocked == 1);

    sentinel_save = SaveGameStruct;
    sentinel_game = GameStruct;
    file = fopen(game_path, "ab");
    CHECK(file != NULL);
    CHECK(fputc(0, file) != EOF);
    CHECK(fclose(file) == 0);
    CHECK(jpb_SaveGameReadFile(game_path) == JPB_SAVE_INVALID_DATA);
    CHECK(memcmp(&SaveGameStruct, &sentinel_save,
                 sizeof(SaveGameStruct)) == 0);
    CHECK(memcmp(&GameStruct, &sentinel_game,
                 sizeof(GameStruct)) == 0);
    corrupt_save = expected_save;
    corrupt_save.saveFileVer = 1;
    file = fopen(game_path, "wb");
    CHECK(file != NULL);
    CHECK(fwrite(&corrupt_save, 1, sizeof(corrupt_save), file) ==
          sizeof(corrupt_save));
    CHECK(fclose(file) == 0);
    CHECK(jpb_SaveGameReadFile(game_path) == JPB_SAVE_INVALID_DATA);
    CHECK(memcmp(&SaveGameStruct, &sentinel_save,
                 sizeof(SaveGameStruct)) == 0);
    corrupt_save = expected_save;
    corrupt_save.validFlag = 0;
    corrupt_save.continueAble = 0;
    file = fopen(game_path, "wb");
    CHECK(file != NULL);
    CHECK(fwrite(&corrupt_save, 1, sizeof(corrupt_save), file) ==
          sizeof(corrupt_save));
    CHECK(fclose(file) == 0);
    CHECK(jpb_SaveGameReadFile(game_path) == JPB_SAVE_INVALID_DATA);
    corrupt_save = expected_save;
    corrupt_save.validFlag = 0;
    corrupt_save.continueAble = 1;
    file = fopen(game_path, "wb");
    CHECK(file != NULL);
    CHECK(fwrite(&corrupt_save, 1, sizeof(corrupt_save), file) ==
          sizeof(corrupt_save));
    CHECK(fclose(file) == 0);
    CHECK(jpb_SaveGameReadFile(game_path) == JPB_SAVE_OK);
    CHECK(SaveGameStruct.validFlag == 0);
    CHECK(SaveGameStruct.continueAble == 1);
    sentinel_save = SaveGameStruct;
    sentinel_game = GameStruct;
    file = fopen(game_path, "wb");
    CHECK(file != NULL);
    CHECK(fputc(0, file) != EOF);
    CHECK(fclose(file) == 0);
    CHECK(jpb_SaveGameReadFile(game_path) == JPB_SAVE_INVALID_DATA);
    CHECK(memcmp(&SaveGameStruct, &sentinel_save,
                 sizeof(SaveGameStruct)) == 0);
    (void)remove(game_path);
    CHECK(jpb_SaveGameReadFile(game_path) == JPB_SAVE_NOT_FOUND);

    OptionStruct = defaultOptionStruct;
    OptionStruct.Music = 0;
    OptionStruct.musicVolume = 17;
    OptionStruct.WindowMode = 1;
    expected_options = OptionStruct;
    CHECK(jpb_SaveOptionsWriteFile(options_path) == JPB_SAVE_OK);
    memset(&OptionStruct, 0xff, sizeof(OptionStruct));
    CHECK(jpb_SaveOptionsReadFile(options_path) == JPB_SAVE_OK);
    CHECK(memcmp(&OptionStruct, &expected_options,
                 sizeof(OptionStruct)) == 0);
    expected_options.saveFileVer = 1;
    file = fopen(options_path, "wb");
    CHECK(file != NULL);
    CHECK(fwrite(&expected_options, 1, sizeof(expected_options), file) ==
          sizeof(expected_options));
    CHECK(fclose(file) == 0);
    CHECK(jpb_SaveOptionsReadFile(options_path) ==
          JPB_SAVE_INVALID_DATA);
    expected_options.saveFileVer = 0;
    file = fopen(options_path, "wb");
    CHECK(file != NULL);
    CHECK(fputc(0, file) != EOF);
    CHECK(fclose(file) == 0);
    CHECK(jpb_SaveOptionsReadFile(options_path) ==
          JPB_SAVE_INVALID_DATA);
    CHECK(memcmp(&OptionStruct, &expected_options,
                 sizeof(OptionStruct)) == 0);
    (void)remove(options_path);
    return 0;
}

int main(void)
{
    CHECK(test_process_status_death_routes() == 0);
    CHECK(test_energy_access_and_clamps() == 0);
    CHECK(test_energy_line_scaling() == 0);
    CHECK(test_exact_setters() == 0);
    CHECK(test_energy_modification() == 0);
    CHECK(test_game_flags_items_and_power() == 0);
    CHECK(test_authored_initial_game_tables() == 0);
    CHECK(test_new_game_initialization() == 0);
    CHECK(test_player_start_combos() == 0);
    CHECK(test_level_difficulty_tables() == 0);
    CHECK(test_overlay_owner_chain() == 0);
    CHECK(test_overlay_hud_gate_boundaries() == 0);
    CHECK(test_life_tile_2d_owner() == 0);
    CHECK(test_menu_big_numbers_and_mini4() == 0);
    CHECK(test_psx_texture_hud_leaf_contract() == 0);
    CHECK(test_sdl_text_hud_leaf_contract() == 0);
    CHECK(test_draw_text_hud_bridge_contract() == 0);
    CHECK(test_level_countdown_outcomes() == 0);
    CHECK(test_level_hangar() == 0);
    CHECK(test_level_arena() == 0);
    CHECK(test_level_theed() == 0);
    CHECK(test_callback_table() == 0);
    CHECK(test_save_game_persistence() == 0);
    puts("game tests passed");
    return 0;
}
