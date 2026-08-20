#include "jpb/brain.h"
#include "jpb/anim.h"
#include "jpb/alloc.h"
#include "jpb/braindmg.h"
#include "jpb/combo.h"
#include "jpb/enemy.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/world.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;
static int feedback_calls;
static int feedback_last_padnum;
static int feedback_last_effect;
static int callback_calls;
static int callback_result;
static int chord_pressed;

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf( \
                stderr, \
                "FAIL %s:%d: %s\n", \
                __FILE__, \
                __LINE__, \
                #condition); \
            ++failures; \
        } \
    } while (0)

static void record_rumble(
    int32_t controller_index,
    uint16_t low_frequency,
    uint16_t high_frequency,
    uint32_t duration_ms,
    void *user_data)
{
    (void)user_data;
    ++feedback_calls;
    feedback_last_padnum = controller_index;
    feedback_last_effect =
        low_frequency == 0 &&
                high_frequency == 20560 &&
                duration_ms == 300
            ? 13
            : -1;
}

static int trace_callback(
    int32_t *cpad, playerObject *player)
{
    (void)cpad;
    (void)player;
    ++callback_calls;
    return callback_result;
}

static int report_running_motion(
    int32_t *cpad, playerObject *player)
{
    (void)cpad;
    player->currentMotion = 2;
    return 0;
}

static int read_chord(void *user_data)
{
    (void)user_data;
    return chord_pressed;
}

typedef struct ControlFixture {
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    modelObject model;
    animObject *animation;
    _animTemplate templates[160];
    Motion motions[160];
    Combo combos[1];
    Motion *current_motion;
} ControlFixture;

static void reset_traces(void)
{
    static WorldData test_world;
    int i;

    feedback_calls = 0;
    feedback_last_padnum = -1;
    feedback_last_effect = -1;
    callback_calls = 0;
    callback_result = 0;
    chord_pressed = 0;
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&test_world, 0, sizeof(test_world));
    gpWorld = &test_world;
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    OptionStruct.ShockFlag[0] = 1;
    OptionStruct.ShockFlag[1] = 1;
    jpb_InputSetRumbleProvider(
        record_rumble, NULL);
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    meminit();
    sprite_gInitSprites();
    LevelSelect = 0;
    gGlobalTimer = 0;
    totalframes = 100;
    player1InputType = 0;
    player2InputType = 0;
    g_p1X = 0.0f;
    g_p1Y = 0.0f;
    g_p2X = 0.0f;
    g_p2Y = 0.0f;
    OMNIDIRECTIONAL_MOVEMENT = 0;
    list_InitList(&enemyList[0]);
    list_InitList(&enemyList[1]);
    mCurEnemyList = 0;
    for (i = 0; i < JPB_PHYSICS_CAPACITY; ++i) {
        maPhysicsData[i].physicsRoot.objectID = -1;
    }
}

static void init_fixture(ControlFixture *fixture)
{
    int i;

    memset(fixture, 0, sizeof(*fixture));
    anim_InitAnimations(0);
    fixture->animation = &maAnimationData[0];
    fixture->player.playerRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->player.playerRoot.objectID = 18;
    fixture->player.playernum = 2;
    fixture->player.playerID = 0;
    fixture->player.paMotions =
        fixture->motions;
    fixture->player.maxMotions = 160;
    fixture->player.oldmaxCMotions = 160;
    fixture->player.paCombos = fixture->combos;
    fixture->player.maxCombos = 0;
    fixture->current_motion =
        &fixture->motions[0];
    fixture->current_motion->Damage = 1;
    fixture->player.pMotion =
        &fixture->current_motion;
    fixture->scene.pPhysics =
        &fixture->physics.physicsRoot;
    fixture->scene.pModel =
        &fixture->model.modelRoot;
    fixture->scene.pScene =
        &fixture->scene.sceneRoot;
    fixture->scene.pAnim =
        &fixture->animation->animRoot;
    fixture->scene.pPlayer =
        &fixture->player.playerRoot;
    fixture->physics.physicsRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->model.modelRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->animation->animRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->animation->depack_context.seqdata =
        fixture->templates;
    for (i = 0; i < 160; ++i) {
        fixture->motions[i].Seq = (uint16_t)i;
        fixture->motions[i].Lock = 1;
        fixture->motions[i].Speed = -1;
        fixture->templates[i].Lframe = 10;
    }
    GameStruct.aCharacterData[
        fixture->player.playernum].Energy = 100;
}

static void test_early_exit(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {0, 0};

    reset_traces();
    init_fixture(&fixture);
    fixture.player.pFlags = UINT32_C(0x00040000);
    fixture.player.currentMotion = 27;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 27);
}

static void test_ai_prelude_and_damage(void)
{
    ControlFixture fixture;
    Sprite shadow;
    SCB scb;
    int32_t cpad[2] = {0, 0};

    reset_traces();
    init_fixture(&fixture);
    memset(&shadow, 0, sizeof(shadow));
    memset(&scb, 0, sizeof(scb));
    shadow.sp_SCB = &scb;
    scb.scb_flags = 0x40;
    fixture.player.shadow =
        (int32_t *)(void *)&shadow;
    fixture.player.pFlags =
        UINT32_C(0x44000020);
    fixture.player.hitDelay = 1;
    brain_ControlPlayer(cpad, &fixture.player, 1);
    CHECK(fixture.player.currentMotion == 0);
    CHECK(fixture.player.ACTION_LOCK == 1);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x44000000)) == 0);
    CHECK((scb.scb_flags & 0x40) == 0);
    CHECK(fixture.player.ctime == 0);

    reset_traces();
    init_fixture(&fixture);
    fixture.current_motion = &fixture.motions[1];
    fixture.animation->pMotion = fixture.current_motion;
    fixture.player.currentMotion = 7;
    brain_ControlPlayer(cpad, &fixture.player, 1);
    CHECK(fixture.player.currentMotion == 7);
    CHECK(fixture.animation->pMotion == &fixture.motions[1]);

    reset_traces();
    init_fixture(&fixture);
    fixture.player.pFlags = UINT32_C(0x04000000);
    fixture.player.currentMotion = 7;
    fixture.player.ACTION_LOCK = 9;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 62);
    CHECK(fixture.player.ACTION_LOCK == 1);
    CHECK(fixture.animation->pMotion == &fixture.motions[62]);
}

static void test_callback_order(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {0, 0};

    reset_traces();
    init_fixture(&fixture);
    callback_result = 1;
    fixture.player.pMotionCallBack =
        trace_callback;
    fixture.player.pForceCallBack =
        trace_callback;
    brain_ControlPlayer(cpad, &fixture.player, 1);
    CHECK(callback_calls == 2);
    CHECK(fixture.player.pMotionCallBack == NULL);
    CHECK(fixture.player.pForceCallBack == NULL);

    reset_traces();
    init_fixture(&fixture);
    callback_result = 1;
    fixture.player.pMainCallBack =
        trace_callback;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(callback_calls == 1);
    CHECK(fixture.player.pMainCallBack == NULL);
    CHECK(fixture.player.ctime == 0);

    /* Executable offset 0x1bc is ACTION_LOCK, not currentMotion at 0x1b4.
     * Airborne callbacks depend on the active motion surviving this prelude. */
    reset_traces();
    init_fixture(&fixture);
    callback_result = -1;
    fixture.player.currentMotion = 22;
    fixture.player.ACTION_LOCK = 9;
    fixture.player.pMotionCallBack = trace_callback;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(callback_calls == 1);
    CHECK(fixture.player.currentMotion == 22);
    CHECK(fixture.player.ACTION_LOCK == 0);
    CHECK(fixture.player.pMotionCallBack == trace_callback);
}

static void test_main_callback_defers_new_actions(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {INT32_C(0x10), 0};
    Motion *initial_animation_motion;

    reset_traces();
    init_fixture(&fixture);
    fixture.player.playernum = 0;
    fixture.player.playerID = 0;
    OptionStruct.ControllerConfig[0] = 0;
    cpad[1] = gaButtonMap[0][4];
    GameStruct.aCharacterData[0].Force = 10;
    GameStruct.aCharacterData[0].Energy = 100;
    fixture.player.pMainCallBack = trace_callback;
    initial_animation_motion = fixture.animation->pMotion;

    /* Exact brain_ControlPlayer ordering lets the active trajectory/landing
     * owner consume the frame before Force, combo, lock, block, or movement.
     * A -1 result keeps that owner; a later 1 result releases it but still
     * does not reinterpret the same frame's held input as a new action. */
    callback_result = -1;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(callback_calls == 1);
    CHECK(fixture.player.pMainCallBack == trace_callback);
    CHECK(fixture.animation->pMotion == initial_animation_motion);
    CHECK(feedback_calls == 0);

    callback_result = 1;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(callback_calls == 2);
    CHECK(fixture.player.pMainCallBack == NULL);
    CHECK(fixture.animation->pMotion == initial_animation_motion);
    CHECK(feedback_calls == 0);

    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.animation->pMotion == &fixture.motions[99]);
    CHECK(fixture.motions[99].FunctPtr == 8);
    CHECK(feedback_calls == 1);
}

static void test_portable_power_battle_chord(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {0, 0};

    reset_traces();
    init_fixture(&fixture);
    GameStruct.NumPlayers = 2;
    gGlobalTimer = UINT32_C(0x100);
    chord_pressed = 1;
    jpb_InputSetPowerBattleChordProvider(
        read_chord, NULL);
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(GameStruct.versusModeFlag == 1);
    jpb_InputSetPowerBattleChordProvider(
        NULL, NULL);
}

static void test_jump_cheat_and_force_exit(void)
{
    ControlFixture fixture;
    wsl_ENEMY enemy;
    playerObject enemy_player;
    sceneObject enemy_scene;
    physicsObject enemy_physics;
    int32_t cpad[2] = {0, 0x0c};

    reset_traces();
    init_fixture(&fixture);
    memset(&enemy, 0, sizeof(enemy));
    memset(&enemy_player, 0, sizeof(enemy_player));
    memset(&enemy_scene, 0, sizeof(enemy_scene));
    memset(&enemy_physics, 0, sizeof(enemy_physics));
    enemy.pPlayer = &enemy_player;
    enemy.ownerType = 1;
    enemy_player.playerRoot.pParent =
        &enemy_scene.sceneRoot;
    enemy_scene.pPhysics =
        &enemy_physics.physicsRoot;
    enemy_physics.physicsRoot.objectID = 2;
    enemyList[0].head = &enemy.node;
    enemyList[0].tail = &enemy.node;
    OptionStruct.JumpCheat = 1;
    brain_ControlPlayer(cpad, &fixture.player, 1);
    CHECK(enemy.exit_flag == 1);

    reset_traces();
    init_fixture(&fixture);
    fixture.player.playerID = 0;
    fixture.player.playernum = 0;
    OptionStruct.ControllerConfig[0] = 0;
    cpad[0] = 0x10;
    cpad[1] = gaButtonMap[0][4];
    GameStruct.aCharacterData[
        fixture.player.playernum].Force = 10;
    GameStruct.aCharacterData[
        fixture.player.playernum].Energy = 100;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.bheld[2] == 100);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[99]);
    CHECK(fixture.motions[99].FunctPtr == 8);
    CHECK(feedback_calls == 1);
    CHECK(feedback_last_padnum ==
          fixture.player.playernum);
    CHECK(feedback_last_effect == 13);
    CHECK(fixture.player.ctime == 0);
}

static void test_second_player_uses_shared_gameplay_scheme(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {INT32_C(0x10), 0};

    reset_traces();
    init_fixture(&fixture);
    fixture.player.playernum = 1;
    fixture.player.playerID = 0;
    OptionStruct.ControllerConfig[0] = 1;
    OptionStruct.ControllerConfig[1] = 0;
    cpad[1] = gaButtonMap[1][4];
    GameStruct.aCharacterData[1].Force = 10;
    GameStruct.aCharacterData[1].Energy = 100;

    brain_ControlPlayer(cpad, &fixture.player, 0);

    CHECK(fixture.animation->pMotion == &fixture.motions[99]);
    CHECK(fixture.motions[99].FunctPtr == 8);
    CHECK(feedback_calls == 1);
    CHECK(feedback_last_padnum == 1);
    CHECK(feedback_last_effect == 13);
}

static void test_combo_busy_exit(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {0x20, 0};

    reset_traces();
    init_fixture(&fixture);
    fixture.player.maxCombos = 1;
    fixture.combos[0].Len = 1;
    fixture.combos[0].Index = UINT8_C(0xff);
    strcpy(fixture.combos[0].String, "e");
    GameStruct.jediComboMask[0].m[0] = 1;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.ctime == 100);
    CHECK(strcmp(fixture.player.PreMotion, "e.") == 0);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x00200010)) ==
          UINT32_C(0x00200010));
}

static void test_idle_attack_and_direction(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {0, 0};

    reset_traces();
    init_fixture(&fixture);
    fixture.player.runCounter = 9;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 0);
    CHECK(fixture.player.runCounter == 0);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[0]);

    reset_traces();
    init_fixture(&fixture);
    OptionStruct.ControllerConfig[0] = 0;
    cpad[1] = gaButtonMap[0][3];
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 15);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[15]);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x20)) != 0);
    CHECK(fixture.animation->animList.head != NULL);

    reset_traces();
    init_fixture(&fixture);
    fixture.player.playernum = 2;
    GameStruct.aCharacterData[2].Energy = 100;
    player1InputType = 1;
    player2InputType = 1;
    g_p2X = 0.0f;
    g_p2Y = 1.0f;
    cpad[0] = 0;
    cpad[1] = INT32_C(0x1400);
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(OMNIDIRECTIONAL_MOVEMENT == 1);
    CHECK(fixture.physics.movemode == MOVE_NORMAL);
    CHECK(fixture.player.currentMotion == 2);
    CHECK(fixture.player.runCounter == 1);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x100)) != 0);
}

static void test_idle_selection_and_block_release(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {0, 0};

    /* Exact 0x1D65C idle priority: lock-on Motion 20 wins over the
     * sub-26-energy Motion 19, while energy 26 remains Motion 0. */
    reset_traces();
    init_fixture(&fixture);
    GameStruct.aCharacterData[2].Energy = 25;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 19);
    CHECK(fixture.animation->pMotion == &fixture.motions[19]);

    reset_traces();
    init_fixture(&fixture);
    GameStruct.aCharacterData[2].Energy = 26;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 0);
    CHECK(fixture.animation->pMotion == &fixture.motions[0]);

    reset_traces();
    init_fixture(&fixture);
    GameStruct.aCharacterData[2].Energy = 1;
    fixture.player.pFlags = UINT32_C(0x00400000);
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 20);
    CHECK(fixture.animation->pMotion == &fixture.motions[20]);

    /* Releasing block while its authored Motion 21 is current must lower
     * the lock before idle selection. Otherwise the player can remain
     * trapped in the chained block pose after the button edge is gone. */
    reset_traces();
    init_fixture(&fixture);
    fixture.player.currentMotion = 21;
    fixture.current_motion = &fixture.motions[21];
    fixture.animation->pMotion = fixture.current_motion;
    fixture.animation->Lock = 25;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 0);
    CHECK(fixture.animation->pMotion == &fixture.motions[0]);
    CHECK(fixture.animation->Lock == fixture.motions[0].Lock);
    CHECK((fixture.player.pFlags & UINT32_C(0x20)) == 0);

    /* A held block in Motion 21 is deliberately stable and must not queue
     * another 15 -> 21 pair every frame. */
    reset_traces();
    init_fixture(&fixture);
    fixture.player.currentMotion = 21;
    fixture.current_motion = &fixture.motions[21];
    fixture.animation->pMotion = fixture.current_motion;
    fixture.animation->Lock = 25;
    cpad[1] = gaButtonMap[0][3];
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 21);
    CHECK(fixture.animation->pMotion == &fixture.motions[21]);
    CHECK(fixture.animation->animList.head == NULL);
    CHECK((fixture.player.pFlags & UINT32_C(0x20)) != 0);
}

static void test_jump_launch(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {0x20, 0};
    JPBPlayerCallback old_callback =
        jpb_TrajectoryCallbackSlot;

    reset_traces();
    init_fixture(&fixture);
    fixture.player.pSettings.JumpVel = -100;
    fixture.player.pSettings.JumpAngle = 0x333;
    fixture.player.pSettings.RunningJumpVel = 222;
    fixture.player.pSettings.RunningJumpAngle = 0x444;
    fixture.player.pFlags = UINT32_C(0x2000);
    fixture.physics.reversoi = 7;
    jpb_TrajectoryCallbackSlot = trace_callback;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 4);
    CHECK(fixture.player.pMotionCallBack ==
          trace_callback);
    CHECK(fixture.physics.reversoi == 0);
    CHECK(fixture.physics.airspeed == -100);
    CHECK(fixture.physics.trajectory == 0x400);
    jpb_TrajectoryCallbackSlot = old_callback;
}

static void test_running_jump_launch(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {
        INT32_C(0x20),
        INT32_C(0x1020),
    };
    JPBPlayerCallback old_callback =
        jpb_TrajectoryCallbackSlot;

    reset_traces();
    init_fixture(&fixture);
    fixture.player.pSettings.JumpVel = -100;
    fixture.player.pSettings.JumpAngle = 0x333;
    fixture.player.pSettings.RunningJumpVel = 222;
    fixture.player.pSettings.RunningJumpAngle = 0x444;
    fixture.physics.reversoi = 7;
    jpb_TrajectoryCallbackSlot = trace_callback;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 4);
    CHECK(fixture.player.pMotionCallBack == trace_callback);
    CHECK(fixture.physics.reversoi == 0);
    CHECK(fixture.physics.airspeed == 222);
    CHECK(fixture.physics.trajectory == 0x444);
    jpb_TrajectoryCallbackSlot = old_callback;
}

static void test_run_stop_and_run_block(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {0, 0};

    reset_traces();
    init_fixture(&fixture);
    fixture.player.currentMotion = 2;
    fixture.current_motion = &fixture.motions[2];
    fixture.animation->pMotion = fixture.current_motion;
    fixture.player.pMotionCallBack = report_running_motion;
    fixture.player.runCounter = 17;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 25);
    CHECK(fixture.animation->pMotion == &fixture.motions[25]);
    CHECK(fixture.motions[25].FunctPtr == 4);
    CHECK(fixture.player.hitDelay == 0);

    reset_traces();
    init_fixture(&fixture);
    fixture.player.currentMotion = 2;
    fixture.current_motion = &fixture.motions[2];
    fixture.animation->pMotion = fixture.current_motion;
    fixture.player.pMotionCallBack = report_running_motion;
    fixture.player.runCounter = 17;
    OptionStruct.ControllerConfig[0] = 1;
    cpad[1] = gaButtonMap[1][3];
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 25);
    CHECK(fixture.animation->pMotion == &fixture.motions[25]);
    CHECK(fixture.motions[25].FunctPtr == 4);

    reset_traces();
    init_fixture(&fixture);
    fixture.player.currentMotion = 2;
    fixture.current_motion = &fixture.motions[2];
    fixture.animation->pMotion = fixture.current_motion;
    fixture.player.pMotionCallBack = report_running_motion;
    fixture.player.runCounter = 17;
    LevelSelect = 13;
    OptionStruct.ControllerConfig[0] = 1;
    cpad[1] = gaButtonMap[1][3];
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 0);
    CHECK(fixture.animation->pMotion == &fixture.motions[0]);
}

static void test_secondary_input_type_direction_gate(void)
{
    ControlFixture fixture;
    int32_t cpad[2] = {0, INT32_C(0x1400)};

    reset_traces();
    init_fixture(&fixture);
    fixture.player.playernum = 2;
    GameStruct.aCharacterData[2].Energy = 100;
    player1InputType = 1;
    player2InputType = 0;
    g_p2X = 0.0f;
    g_p2Y = -1.0f;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 1);

    reset_traces();
    init_fixture(&fixture);
    fixture.player.playernum = 2;
    GameStruct.aCharacterData[2].Energy = 100;
    player1InputType = 0;
    player2InputType = 1;
    g_p2X = 0.0f;
    g_p2Y = -1.0f;
    brain_ControlPlayer(cpad, &fixture.player, 0);
    CHECK(fixture.player.currentMotion == 2);
}

static void test_running_directional_attacks(void)
{
    static const struct {
        int32_t pressed;
        int motion;
        int displacement;
    } cases[] = {
        {INT32_C(0x40), 92, 0x0f},
        {INT32_C(0x80), 93, 0x0f},
        {INT32_C(0x10), 94, 0x0e},
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        ControlFixture fixture;
        int32_t cpad[2] = {
            cases[i].pressed,
            INT32_C(0x1400),
        };

        reset_traces();
        init_fixture(&fixture);
        fixture.player.runCounter = 21;
        player2InputType = 1;
        g_p2Y = -1.0f;
        brain_ControlPlayer(cpad, &fixture.player, 0);
        CHECK(fixture.player.currentMotion ==
              cases[i].motion);
        CHECK(fixture.animation->pMotion ==
              &fixture.motions[cases[i].motion]);
        CHECK(fixture.motions[cases[i].motion].disp ==
              cases[i].displacement);
        CHECK(fixture.player.runCounter == 21);
        CHECK((fixture.player.pFlags &
               UINT32_C(0x100)) == 0);
    }

    {
        ControlFixture fixture;
        int32_t cpad[2] = {
            INT32_C(0x40),
            INT32_C(0x1400),
        };

        reset_traces();
        init_fixture(&fixture);
        fixture.player.runCounter = 20;
        player2InputType = 1;
        g_p2Y = -1.0f;
        brain_ControlPlayer(cpad, &fixture.player, 0);
        CHECK(fixture.player.currentMotion == 2);
        CHECK(fixture.player.runCounter == 1);
        CHECK(fixture.motions[92].disp == 0);
    }
}

int main(void)
{
    test_early_exit();
    test_ai_prelude_and_damage();
    test_callback_order();
    test_main_callback_defers_new_actions();
    test_portable_power_battle_chord();
    test_jump_cheat_and_force_exit();
    test_second_player_uses_shared_gameplay_scheme();
    test_combo_busy_exit();
    test_idle_attack_and_direction();
    test_idle_selection_and_block_release();
    test_jump_launch();
    test_running_jump_launch();
    test_run_stop_and_run_block();
    test_secondary_input_type_direction_gate();
    test_running_directional_attacks();

    if (failures != 0) {
        fprintf(
            stderr,
            "%d brain control test(s) failed\n",
            failures);
        return 1;
    }
    puts("brain control tests passed");
    return 0;
}
