#include "jpb/shaolin.h"
#include "jpb/ai.h"
#include "jpb/anim.h"
#include "jpb/game.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"

#include <stdio.h>
#include <string.h>

static int failures;
static uint32_t test_pose_words[3];

static void init_test_animations(void)
{
    int index;

    (anim_InitAnimations)(0);
    for (index = 0; index < JPB_ANIMATION_CAPACITY; ++index) {
        maAnimationData[index].depack_context.huffdataorigin =
            test_pose_words;
        maAnimationData[index].depack_context3.huffdataorigin =
            test_pose_words;
    }
}

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

typedef struct ShaolinFixture {
    WorldData world;
    sceneObject attacker_scene;
    sceneObject target_scene;
    sceneObject other_scene;
    physicsObject attacker_physics;
    physicsObject target_physics;
    physicsObject other_physics;
    modelObject attacker_model;
    playerObject attacker;
    playerObject target;
    playerObject other;
    wsl_ENEMY enemy;
    uint8_t ai_storage[64];
    Motion motions[30];
    _animTemplate templates[30];
    animObject *animation;
    kfNode kungfu;
} ShaolinFixture;

static void connect_actor(
    sceneObject *scene,
    physicsObject *physics,
    playerObject *player)
{
    scene->pScene = &scene->sceneRoot;
    scene->pPhysics = &physics->physicsRoot;
    scene->pPlayer = &player->playerRoot;
    physics->physicsRoot.pParent =
        &scene->sceneRoot;
    player->playerRoot.pParent =
        &scene->sceneRoot;
}

static void init_fixture(ShaolinFixture *fixture)
{
    int index;
    int other_index;

    memset(fixture, 0, sizeof(*fixture));
    for (index = 0;
         index < JPB_PHYSICS_CAPACITY;
         ++index) {
        for (other_index = 0;
             other_index < JPB_PHYSICS_CAPACITY;
             ++other_index) {
            maRange[index][other_index] = -1.0f;
        }
    }
    memset(&GameStruct, 0, sizeof(GameStruct));
    init_test_animations();
    fixture->animation = &maAnimationData[2];
    connect_actor(
        &fixture->attacker_scene,
        &fixture->attacker_physics,
        &fixture->attacker);
    connect_actor(
        &fixture->target_scene,
        &fixture->target_physics,
        &fixture->target);
    connect_actor(
        &fixture->other_scene,
        &fixture->other_physics,
        &fixture->other);
    fixture->attacker_scene.pModel =
        &fixture->attacker_model.modelRoot;
    fixture->attacker_scene.pAnim =
        &fixture->animation->animRoot;
    fixture->animation->animRoot.pParent =
        &fixture->attacker_scene.sceneRoot;
    fixture->animation->depack_context.seqdata =
        fixture->templates;
    fixture->attacker.playerRoot.objectID = 2;
    fixture->target.playerRoot.objectID = 0;
    fixture->other.playerRoot.objectID = -1;
    fixture->attacker_physics.physicsRoot.objectID = 2;
    fixture->target_physics.physicsRoot.objectID = 0;
    fixture->other_physics.physicsRoot.objectID = 1;
    fixture->attacker.target = &fixture->target;
    fixture->attacker.pEnemy = &fixture->enemy;
    fixture->attacker.paiMemory =
        (aiData *)(void *)fixture->ai_storage;
    fixture->attacker.paMotions = fixture->motions;
    fixture->attacker.maxMotions = 29;
    fixture->attacker.pMotion =
        &fixture->animation->pMotion;
    fixture->attacker.playernum = 2;
    fixture->enemy.pPlayer = &fixture->attacker;
    fixture->enemy.currHTHDelay = -1;
    fixture->kungfu.id =
        &fixture->attacker.playerRoot;
    fixture->world.player0 = &fixture->target;
    fixture->world.player1 = &fixture->other;
    gpWorld = &fixture->world;
    tankID = -1;
    GameStruct.HTHRate = 8;
    GameStruct.aCharacterData[2].Energy = 100;
    GameStruct.aCharacterData[2].MaxEnergy = 100;
    for (index = 0; index < 30; ++index) {
        fixture->motions[index].Seq =
            (uint16_t)index;
        fixture->motions[index].Lock = 1;
        fixture->templates[index].Lframe = 10;
    }
}

static void test_pool_and_list_lifecycle(void)
{
    kfNode *first;
    kfNode *second;

    memset(akfNodes, 0xff, sizeof(akfNodes));
    memset(attackChoice, 0xff, sizeof(attackChoice));
    kfList.head = (Node *)(uintptr_t)1;
    kfList.tail = (Node *)(uintptr_t)2;
    shaolin_InitKungfu();
    CHECK(kfList.head == NULL);
    CHECK(kfList.tail == NULL);
    CHECK(attackChoice[0] == 0);
    CHECK(attackChoice[JPB_KUNGFU_CAPACITY - 1] == 0);
    CHECK(akfNodes[0].id == NULL);
    CHECK(akfNodes[JPB_KUNGFU_CAPACITY - 1].flags == 0);

    akfNodes[3].timer = 99;
    akfNodes[3].id = (objectRoot *)(uintptr_t)1;
    akfNodes[3].chi = 7;
    akfNodes[3].loc = 8;
    akfNodes[3].flags = 9;
    first = shaolin_GetKungfu(3);
    CHECK(first == &akfNodes[3]);
    CHECK(first->node.next == NULL);
    CHECK(first->timer == 0);
    CHECK(first->id == NULL);
    CHECK(first->chi == 0);
    CHECK(first->loc == 0);
    CHECK(first->flags == 0);

    second = shaolin_GetKungfu(4);
    shaolin_AddKungfu(first);
    shaolin_AddKungfu(second);
    CHECK(kfList.head == &first->node);
    CHECK(kfList.tail == &second->node);
    CHECK(first->node.next == &second->node);
    CHECK(second->node.next == NULL);

    a[0] = 4;
    a[1] = 5;
    shaolin_StartKungfu();
    CHECK(a[0] == 0);
    CHECK(a[1] == 0);
    CHECK(kfList.head == NULL);
    CHECK(kfList.tail == NULL);
}

static void test_attack_choice_deduplication(void)
{
    shaolin_InitKungfu();
    CHECK(shaolin_CheckMove(0, 3) == 1);
    CHECK(shaolin_CheckMove(7, 3) == 0);
    CHECK(attackChoice[3] == 7);
    CHECK(shaolin_CheckMove(7, 9) == 1);
    CHECK(attackChoice[9] == 0);
    CHECK(shaolin_CheckMove(0x107, 5) == 0);
    CHECK(attackChoice[5] == 7);
}

static void test_ai_movement_owners(void)
{
    ShaolinFixture fixture;
    VECTOR waypoint = {500, 0, 0, 0};

    init_fixture(&fixture);
    fixture.target_physics.pos.vx = 100.75f;
    fixture.target_physics.pos.vy = -2.5f;
    fixture.target_physics.pos.vz = 300.25f;
    fixture.target_physics.vpos.vx = 100;
    fixture.target_physics.vpos.vy = -2;
    fixture.target_physics.vpos.vz = 300;
    fixture.attacker.pFlags =
        UINT32_C(0x80400020);
    ai_WalktoPlayer(
        &fixture.attacker, 1, 999);
    CHECK(fixture.enemy.destination.vx == 100);
    CHECK(fixture.enemy.destination.vy == -2);
    CHECK(fixture.enemy.destination.vz == 300);
    CHECK(fixture.enemy.radius == 0x100);
    CHECK((fixture.attacker.pFlags &
           UINT32_C(0x08000000)) != 0);
    CHECK((fixture.attacker.pFlags &
           UINT32_C(0x80000000)) == 0);
    CHECK((fixture.attacker.pFlags &
           UINT32_C(0x20)) == 0);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[26]);

    init_fixture(&fixture);
    fixture.attacker.currentMotion = 1;
    fixture.attacker.pFlags =
        UINT32_C(0x08000020);
    CHECK(ai_WalkToPoint(
              &fixture.attacker,
              1,
              &waypoint,
              0x20) == 0);
    CHECK(fixture.enemy.destination.vx == 500);
    CHECK(fixture.enemy.destination.vy == 0);
    CHECK(fixture.enemy.destination.vz == 0);
    CHECK(fixture.enemy.radius == 0x20);
    CHECK((fixture.attacker.pFlags &
           UINT32_C(0x80000000)) != 0);
    CHECK((fixture.attacker.pFlags &
           UINT32_C(0x08000000)) == 0);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[1]);

    init_fixture(&fixture);
    waypoint.vx = 8;
    fixture.attacker.currentMotion = 2;
    fixture.attacker.pFlags =
        UINT32_C(0x00400020);
    fixture.attacker.hitMask = 99;
    CHECK(ai_WalkToPoint(
              &fixture.attacker,
              1,
              &waypoint,
              0x20) == 1);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[25]);
    CHECK((fixture.attacker.pFlags &
           UINT32_C(0x00400000)) == 0);
    CHECK(fixture.attacker.hitMask == 0);
}

static void test_attack_scheduler(void)
{
    ShaolinFixture fixture;
    aiData *data;

    init_fixture(&fixture);
    data = fixture.attacker.paiMemory;
    data->violence[0] = 2;
    data->violence[1] = 1;
    data->hth[0] = 5;
    data->hth[1] = 1;
    fixture.attacker_physics.vpos.vx = 100;
    fixture.target_physics.vpos.vx = 0;
    fixture.attacker_physics.angle.vy = 123;
    shaolin_InitKungfu();
    shaolin_StartKungfu();
    shaolin_Attack(&fixture.kungfu);
    CHECK(a[0] == 1);
    CHECK(a[1] == 0);
    CHECK(fixture.enemy.currHTHDelay == 4);
    CHECK(attackChoice[2] == 5);
    CHECK(fixture.kungfu.chi == 1);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[5]);
    CHECK(fixture.attacker_physics.angle.vy ==
          (ratan2(-100, 0) & 0x0fff));

    /*
     * A far target selects the exact walk owner and consumes no attack
     * motion, while the optimized third parameter remains observationally
     * unused inside ai_WalktoPlayer.
     */
    init_fixture(&fixture);
    fixture.attacker.pFlags = UINT32_C(1);
    fixture.attacker_physics.vpos.vx = 1000;
    fixture.target_physics.vpos.vx = 0;
    shaolin_StartKungfu();
    shaolin_Attack(&fixture.kungfu);
    CHECK(a[0] == 1);
    CHECK(fixture.enemy.radius == 0x100);
    CHECK(fixture.animation->pMotion == NULL);
}

static void test_ai_attack_opcode_owners(void)
{
    ShaolinFixture fixture;
    UDATA vars[4];
    aiData *data;
    animListNode *queued;

    init_fixture(&fixture);
    memset(vars, 0, sizeof(vars));
    fixture.world.player1 = &fixture.target;
    fixture.attacker_physics.vpos.vx = 160;
    gGlobalTimer = 100;
    shaolin_InitKungfu();
    shaolin_StartKungfu();
    CHECK(ai_HthAttack(
              &fixture.enemy, vars) == 1);
    CHECK(fixture.enemy.kungfu ==
          &akfNodes[2]);
    CHECK(fixture.enemy.kungfu->id ==
          &fixture.attacker.playerRoot);
    CHECK(fixture.enemy.kungfu->timer ==
          gGlobalTimer + UINT32_C(0x7800));
    CHECK(fixture.enemy.kungfu->chi == 10);
    CHECK(kfList.head ==
          &fixture.enemy.kungfu->node);

    init_fixture(&fixture);
    memset(vars, 0, sizeof(vars));
    fixture.world.player1 = &fixture.target;
    vars[1].si = 5;
    fixture.attacker.pFlags = UINT32_C(0x20);
    fixture.attacker_physics.vpos.vx = 100;
    fixture.target_physics.vpos.vx = 0;
    fixture.attacker_physics.angle.vy = 123;
    fixture.enemy.kungfu =
        &fixture.kungfu;
    CHECK(ai_HthAttack(
              &fixture.enemy, vars) == 1);
    CHECK(fixture.enemy.kungfu == NULL);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[5]);
    CHECK((fixture.attacker.pFlags &
           UINT32_C(0x20)) == 0);
    CHECK(fixture.attacker_physics.angle.vy ==
          (ratan2(-100, 0) & 0x0fff));

    init_fixture(&fixture);
    fixture.attacker_physics.vpos.vx = 100;
    fixture.target_physics.vpos.vx = 0;
    fixture.attacker_physics.angle.vy = 123;
    shaolin_InitKungfu();
    shaolin_StartKungfu();
    a[0] = 3;
    shaolin_Attack(&fixture.kungfu);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[0x15]);
    CHECK(fixture.attacker_physics.angle.vy ==
          (ratan2(-100, 0) & 0x0fff));

    init_fixture(&fixture);
    memset(vars, 0, sizeof(vars));
    fixture.world.player1 = &fixture.target;
    data = fixture.attacker.paiMemory;
    data->reload[0] = 2;
    data->reload[1] = 1;
    vars[3].si = 6;
    GameStruct.RangedRate = 8;
    fixture.enemy.currRangedDelay = -1;
    ai_RangedAttack(&fixture.enemy, vars);
    CHECK(fixture.enemy.currRangedDelay == 4);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[6]);

    init_fixture(&fixture);
    memset(vars, 0, sizeof(vars));
    fixture.world.player1 = &fixture.target;
    data = fixture.attacker.paiMemory;
    data->combos[0] = 20;
    data->combos[1] = 1;
    fixture.ai_storage[20] = 24;
    fixture.ai_storage[21] = 3;
    fixture.ai_storage[24] = 5;
    fixture.ai_storage[25] = 6;
    fixture.ai_storage[26] = 0;
    CHECK(ai_SeqAttack(
              &fixture.enemy, vars) == 1);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[5]);
    queued = (animListNode *)
        fixture.animation->animList.tail;
    CHECK(queued != NULL);
    CHECK(queued->pMotion ==
          &fixture.motions[6]);
    CHECK(fixture.enemy.currSeq == 0);
    CHECK(fixture.enemy.currSeqIndex == 0);
    CHECK(fixture.enemy.seqMode == 0);
}

static void test_kungfu_coordinator(void)
{
    ShaolinFixture fixture;

    init_fixture(&fixture);
    memset(maPhysicsData, 0, sizeof(maPhysicsData));
    fixture.target.pFlags = UINT32_C(0x80);
    fixture.attacker.pFlags = UINT32_C(0x400000);
    fixture.attacker.locked = &fixture.other;
    fixture.attacker_physics.vpos.vx = 100;
    fixture.attacker_physics.vpos.vz = -85;
    shaolin_InitKungfu();
    shaolin_StartKungfu();
    shaolin_AddKungfu(&fixture.kungfu);
    shaolin_DoKungfu();
    CHECK(fixture.kungfu.flags == 1);
    CHECK(fixture.kungfu.loc >= 0);
    CHECK(fixture.kungfu.loc < 4);
    CHECK((fixture.attacker.pFlags &
           UINT32_C(0x400000)) == 0);
    CHECK(fixture.attacker.locked == NULL);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[0]);
}

static void test_malformed_kungfu_owner_chains(void)
{
    ShaolinFixture fixture;

    shaolin_InitKungfu();
    shaolin_StartKungfu();
    shaolin_Attack(NULL);
    CHECK(a[0] == 0);
    CHECK(a[1] == 0);

    init_fixture(&fixture);
    shaolin_InitKungfu();
    shaolin_StartKungfu();
    fixture.kungfu.id = NULL;
    shaolin_Attack(&fixture.kungfu);
    CHECK(a[0] == 0);
    CHECK(a[1] == 0);

    init_fixture(&fixture);
    shaolin_InitKungfu();
    shaolin_StartKungfu();
    fixture.attacker.playerRoot.pParent = NULL;
    shaolin_Attack(&fixture.kungfu);
    CHECK(a[0] == 0);
    CHECK(a[1] == 0);

    init_fixture(&fixture);
    shaolin_InitKungfu();
    shaolin_StartKungfu();
    fixture.attacker_scene.pPlayer = NULL;
    shaolin_Attack(&fixture.kungfu);
    CHECK(a[0] == 0);
    CHECK(a[1] == 0);

    init_fixture(&fixture);
    shaolin_InitKungfu();
    shaolin_StartKungfu();
    fixture.kungfu.id = NULL;
    shaolin_AddKungfu(&fixture.kungfu);
    shaolin_DoKungfu();
    CHECK(fixture.kungfu.flags == 1);
}

int main(void)
{
    test_pool_and_list_lifecycle();
    test_attack_choice_deduplication();
    test_ai_movement_owners();
    test_attack_scheduler();
    test_ai_attack_opcode_owners();
    test_kungfu_coordinator();
    test_malformed_kungfu_owner_chains();

    if (failures != 0) {
        fprintf(
            stderr,
            "%d shaolin test(s) failed\n",
            failures);
        return 1;
    }
    puts("shaolin tests passed");
    return 0;
}
