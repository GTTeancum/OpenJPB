#include "jpb/anim.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/physics.h"
#include "jpb/scene.h"

#include <stdio.h>
#include <string.h>

static int failures;
static int feedback_calls;

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
    CHECK(controller_index == 0);
    CHECK(low_frequency == 0);
    CHECK(high_frequency == 20560);
    CHECK(duration_ms == 300);
    ++feedback_calls;
}

static int callback(
    int32_t *cpad, playerObject *player)
{
    (void)cpad;
    (void)player;
    return 0;
}

enum { TEST_MOTION_COUNT = 160 };

typedef struct ActivateFixture {
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    Motion motions[TEST_MOTION_COUNT];
    _animTemplate templates[TEST_MOTION_COUNT];
} ActivateFixture;

static void reset_state(ActivateFixture *fixture)
{
    int index;

    memset(fixture, 0, sizeof(*fixture));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    anim_InitAnimations(0);
    fixture->animation = &maAnimationData[0];
    fixture->player.playerRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->player.playerID = 0;
    fixture->player.playernum = 0;
    fixture->player.paMotions = fixture->motions;
    fixture->player.maxMotions = TEST_MOTION_COUNT;
    fixture->player.oldmaxCMotions = TEST_MOTION_COUNT;
    fixture->player.pMotion =
        &fixture->animation->pMotion;
    fixture->scene.pPhysics =
        &fixture->physics.physicsRoot;
    fixture->scene.pAnim =
        &fixture->animation->animRoot;
    fixture->scene.pPlayer =
        &fixture->player.playerRoot;
    fixture->animation->animRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->animation->depack_context.seqdata =
        fixture->templates;
    for (index = 0; index < TEST_MOTION_COUNT; ++index) {
        fixture->motions[index].Seq =
            (uint16_t)index;
        fixture->motions[index].Speed = -1;
        fixture->templates[index].Lframe = 10;
    }
    GameStruct.aCharacterData[0].Force = 10;
    GameStruct.aCharacterData[0].MaxForce = 100;
    feedback_calls = 0;
    OptionStruct.ShockFlag[0] = 1;
    jpb_InputSetRumbleProvider(
        record_rumble, NULL);
}

int main(void)
{
    ActivateFixture fixture;
    int32_t cpad[2] = {0, 0};

    reset_state(&fixture);
    fixture.player.pForceCallBack = callback;
    CHECK(force_gActivate(cpad, &fixture.player) == 0);
    CHECK(fixture.animation->pMotion == NULL);

    reset_state(&fixture);
    fixture.player.pMotionCallBack = callback;
    CHECK(force_gActivate(cpad, &fixture.player) == 1);
    CHECK(fixture.animation->pMotion == NULL);

    reset_state(&fixture);
    GameStruct.aCharacterData[0].Force = 4;
    cpad[0] = 0x10;
    CHECK(force_gActivate(cpad, &fixture.player) == 0);
    CHECK(fixture.animation->pMotion == NULL);

    /* The matched routine evaluates the item slot before its shared
     * Force > 4 gate.  At the boundary the authored item animation has
     * therefore started even though force_gActivate reports zero. */
    reset_state(&fixture);
    GameStruct.aCharacterData[0].Force = 4;
    GameStruct.aCharacterData[0].Items = 1;
    cpad[0] = 0x80;
    CHECK(force_gActivate(cpad, &fixture.player) == 0);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[65]);
    CHECK(fixture.motions[65].FunctPtr == 23);
    CHECK(feedback_calls == 0);

    reset_state(&fixture);
    cpad[0] = 0x80;
    CHECK(force_gActivate(cpad, &fixture.player) == 0);
    CHECK(fixture.animation->pMotion == NULL);
    GameStruct.aCharacterData[0].Items = 1;
    CHECK(force_gActivate(cpad, &fixture.player) == 1);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[65]);
    CHECK(fixture.motions[65].FunctPtr == 23);

    /* Item is the first tested action bit.  With no inventory its early
     * return suppresses later simultaneously-held Force actions.  With an
     * item available, north is evaluated afterward but the item's lock 30
     * rejects that second sequence.  The later zero result becomes the
     * function result even though the item animation remains active. */
    reset_state(&fixture);
    cpad[0] = 0x90;
    CHECK(force_gActivate(cpad, &fixture.player) == 0);
    CHECK(fixture.animation->pMotion == NULL);
    CHECK(feedback_calls == 0);
    GameStruct.aCharacterData[0].Items = 1;
    CHECK(force_gActivate(cpad, &fixture.player) == 0);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[65]);
    CHECK(fixture.motions[65].FunctPtr == 23);
    CHECK(fixture.motions[99].FunctPtr == 8);
    CHECK(feedback_calls == 1);

    reset_state(&fixture);
    cpad[0] = 0x10;
    CHECK(force_gActivate(cpad, &fixture.player) == 1);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[99]);
    CHECK(fixture.motions[99].FunctPtr == 8);
    CHECK(feedback_calls == 1);

    /* Feedback is issued by the activation branch even when animation lock
     * 30 rejects the sequence.  Preserve that PDB-observed distinction
     * between action acceptance and controller feedback. */
    reset_state(&fixture);
    fixture.animation->Lock = 30;
    cpad[0] = 0x10;
    CHECK(force_gActivate(cpad, &fixture.player) == 0);
    CHECK(fixture.animation->pMotion == NULL);
    CHECK(feedback_calls == 1);

    reset_state(&fixture);
    cpad[0] = 0x20;
    CHECK(force_gActivate(cpad, &fixture.player) == 0);
    jediUpgrades[0].forcePowers = 0x2000;
    CHECK(force_gActivate(cpad, &fixture.player) == 1);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[134]);
    CHECK(fixture.motions[134].FunctPtr == 16);

    reset_state(&fixture);
    fixture.player.playerID = 12;
    cpad[0] = 0x10;
    CHECK(force_gActivate(cpad, &fixture.player) == 1);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[65]);

    reset_state(&fixture);
    GameStruct.ForceLevel = 9;
    cpad[0] = 0x40;
    CHECK(force_gActivate(cpad, &fixture.player) == 1);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[66]);
    CHECK(fixture.motions[66].FunctPtr == 14);

    /* South has its own 0x4000 authored-upgrade gate; the global level
     * override above is a separate executable branch. */
    reset_state(&fixture);
    cpad[0] = 0x40;
    CHECK(force_gActivate(cpad, &fixture.player) == 0);
    CHECK(fixture.animation->pMotion == NULL);
    CHECK(feedback_calls == 0);
    jediUpgrades[0].forcePowers = 0x4000;
    CHECK(force_gActivate(cpad, &fixture.player) == 1);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[66]);
    CHECK(fixture.motions[66].FunctPtr == 14);
    CHECK(feedback_calls == 1);

    if (failures != 0) {
        fprintf(
            stderr,
            "%d force activation test(s) failed\n",
            failures);
        return 1;
    }
    puts("force activation tests passed");
    return 0;
}
