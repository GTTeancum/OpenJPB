#include "jpb/anim.h"
#include "jpb/ai.h"
#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/camera.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/world.h"

#include <stdio.h>
#include <string.h>

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

static void connect_player(
    playerObject *player, sceneObject *scene, physicsObject *physics)
{
    memset(player, 0, sizeof(*player));
    memset(scene, 0, sizeof(*scene));
    memset(physics, 0, sizeof(*physics));
    player->playerRoot.pParent = &scene->sceneRoot;
    scene->pPhysics = &physics->physicsRoot;
}

static int anim_sound_play_calls;
static int anim_sound_last_bank;
static char anim_sound_last_name[9];

static uint16_t anim_sound_play_hook(
    VECTOR *position,
    int bank,
    char *sound,
    uint32_t flags,
    void *user_data)
{
    (void)position;
    (void)flags;
    (void)user_data;
    ++anim_sound_play_calls;
    anim_sound_last_bank = bank;
    memcpy(anim_sound_last_name, sound, 8);
    anim_sound_last_name[8] = '\0';
    return (uint16_t)(70 + anim_sound_play_calls);
}

static int node_count(const List *list)
{
    const Node *node = list->head;
    int count = 0;

    while (node != NULL) {
        ++count;
        node = node->next;
    }
    return count;
}

static int test_motion_layout(void)
{
    CHECK(sizeof(Motion) == 100);
    CHECK(offsetof(Motion, vel) == 16);
    CHECK(offsetof(Motion, Charge) == 54);
    CHECK(offsetof(Motion, ChargeAcc) == 56);
    CHECK(sizeof(_animFrame) == 560);
    CHECK(offsetof(_animFrame, v3RootTranslationDelta) == 264);
#if UINTPTR_MAX == UINT64_MAX
    CHECK(sizeof(_dpcontext) == 64);
    CHECK(sizeof(animListNode) == 40);
    CHECK(sizeof(animObject) == 2496);
    CHECK(offsetof(animObject, pMotion) == 2464);
#endif
    return 0;
}

static int test_animation_pool(void)
{
    animObject before;
    animObject neighbor;
    int index;

    memset(maAnimationData, 0xa5, sizeof(maAnimationData));
    anim_InitAnimations(0);
    for (index = 0; index < JPB_ANIMATION_CAPACITY; ++index) {
        animObject *animation = &maAnimationData[index];
        Node *node = animation->animFreeList.head;
        int nodes = 0;

        CHECK(animation->animRoot.objectID == -1);
        CHECK(strncmp(animation->animRoot.objectName, "ANIM", 4) == 0);
        CHECK(animation->pCurrentAnimFrame ==
              animation->AnimFrameBuffer);
        CHECK(animation->pPreviousAnimFrame ==
              animation->AnimFrameBuffer);
        CHECK(animation->animFrameRate == JPB_FIXED_ONE);
        CHECK(animation->animList.head == NULL);
        CHECK(animation->animList.tail == NULL);
        while (node != NULL) {
            ++nodes;
            node = node->next;
        }
        CHECK(nodes == JPB_ANIM_QUEUE_NODE_CAPACITY);
        CHECK(animation->animFreeList.tail != NULL);
        CHECK(animation->animFreeList.tail->next == NULL);
    }

    memset(&before, 0x6b, sizeof(before));
    maAnimationData[9] = before;
    maAnimationData[10] = before;
    anim_InitAnimations(10);
    CHECK(memcmp(&maAnimationData[9], &before, sizeof(before)) == 0);
    CHECK(maAnimationData[10].animRoot.objectID == -1);

    before = maAnimationData[0];
    anim_InitAnimations(-1);
    CHECK(memcmp(&maAnimationData[0], &before, sizeof(before)) == 0);

    anim_InitAnimations(0);
    maAnimationData[10].animRoot.objectID = 10;
    maAnimationData[10].animFrameRate = 123;
    maAnimationData[10].animList =
        maAnimationData[10].animFreeList;
    neighbor = maAnimationData[11];
    jpb_AnimResetObjectSlot(10);
    CHECK(maAnimationData[10].animRoot.objectID == -1);
    CHECK(strncmp(
              maAnimationData[10].animRoot.objectName,
              "ANIM",
              4) == 0);
    CHECK(maAnimationData[10].pCurrentAnimFrame ==
          maAnimationData[10].AnimFrameBuffer);
    CHECK(maAnimationData[10].pPreviousAnimFrame ==
          maAnimationData[10].AnimFrameBuffer);
    CHECK(maAnimationData[10].animFrameRate == JPB_FIXED_ONE);
    CHECK(maAnimationData[10].animList.head == NULL);
    CHECK(maAnimationData[10].animList.tail == NULL);
    CHECK(node_count(&maAnimationData[10].animFreeList) ==
          JPB_ANIM_QUEUE_NODE_CAPACITY);
    CHECK(memcmp(
              &maAnimationData[11],
              &neighbor,
              sizeof(neighbor)) == 0);
    before = maAnimationData[0];
    jpb_AnimResetObjectSlot(-1);
    jpb_AnimResetObjectSlot(JPB_ANIMATION_CAPACITY);
    CHECK(memcmp(&maAnimationData[0], &before, sizeof(before)) == 0);
    return 0;
}

static int test_freeze_window(void)
{
    objectRoot actor;
    sceneObject scene;
    animObject animation;
    animListNode sequence;
    _animTemplate template_data;
    Motion motion;

    memset(&actor, 0, sizeof(actor));
    memset(&scene, 0, sizeof(scene));
    memset(&animation, 0, sizeof(animation));
    memset(&sequence, 0, sizeof(sequence));
    memset(&template_data, 0, sizeof(template_data));
    memset(&motion, 0, sizeof(motion));
    actor.pParent = &scene.sceneRoot;
    scene.pAnim = &animation.animRoot;

    animation.animFrameIndex = 0;
    CHECK(anim_CheckFreeze(&actor) == 0);
    animation.tweenFramesLeft = 1;
    CHECK(anim_CheckFreeze(&actor) == 1);

    animation.tweenFramesLeft = 0;
    animation.pMotion = &motion;
    animation.pCurrentAnimSeq = &sequence;
    sequence.pAnimTemplate = &template_data;
    template_data.Fframe = 0;
    template_data.Lframe = 20;
    motion.frzin = 2;
    motion.frzout = 2;

    animation.animFrameIndex = 11 << JPB_FIXED_SHIFT;
    CHECK(anim_CheckFreeze(&actor) == 0);
    animation.animFrameIndex = 2 << JPB_FIXED_SHIFT;
    CHECK(anim_CheckFreeze(&actor) == 1);
    animation.animFrameIndex = 20 << JPB_FIXED_SHIFT;
    CHECK(anim_CheckFreeze(&actor) == 1);

    motion.motionFlags = 0x80000000u;
    animation.animFrameIndex = 11 << JPB_FIXED_SHIFT;
    CHECK(anim_CheckFreeze(&actor) == 0);
    return 0;
}

static int test_animation_queue(void)
{
    playerObject player;
    playerObject target;
    sceneObject scene;
    sceneObject target_scene;
    physicsObject physics;
    physicsObject target_physics;
    animObject *animation;
    animObject *target_animation;
    _animTemplate templates[4];
    _animTemplate target_templates[8];
    Motion motion;
    animListNode *node;
    int index;

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    target_animation = &maAnimationData[1];
    connect_player(&player, &scene, &physics);
    connect_player(&target, &target_scene, &target_physics);
    player.target = &target;
    animation->animRoot.pParent = &scene.sceneRoot;
    target_animation->animRoot.pParent = &target_scene.sceneRoot;
    scene.pAnim = &animation->animRoot;
    scene.pPlayer = &player.playerRoot;
    target_scene.pAnim = &target_animation->animRoot;
    target_scene.pPlayer = &target.playerRoot;
    animation->depack_context.seqdata = templates;
    player.maxMotions = 2;
    target_animation->depack_context3.seqdata = target_templates;
    player.maxMotions = 4;
    memset(templates, 0, sizeof(templates));
    memset(target_templates, 0, sizeof(target_templates));
    memset(&motion, 0, sizeof(motion));

    motion.Seq = 2;
    motion.twin = 7;
    motion.Lock = 9;
    motion.Speed = -1;
    CHECK(anim_AddNextAnimSeq(animation, &motion, 0) == 0);
    CHECK(node_count(&animation->animList) == 1);
    CHECK(node_count(&animation->animFreeList) == 7);
    node = (animListNode *)animation->animList.head;
    CHECK(node->pAnimTemplate == &templates[2]);
    CHECK(node->tweenLevel == 7);
    CHECK(node->Lock == 9);
    CHECK(node->Speed == JPB_FIXED_ONE);
    CHECK(node->pMotion == &motion);

    motion.Seq = 3;
    motion.Speed = 200;
    motion.motionFlags = 0x02000000u;
    CHECK(anim_AddNextAnimSeq(animation, &motion, 1) == 0);
    CHECK((motion.motionFlags & 0x02000000u) == 0);
    CHECK(node_count(&animation->animList) == 1);
    CHECK(node_count(&animation->animFreeList) == 7);
    node = (animListNode *)animation->animList.head;
    CHECK(node->pAnimTemplate == &templates[3]);
    CHECK(node->Lock == 0x1e);
    CHECK(node->Speed == 200);

    list_MoveList(&animation->animFreeList, &animation->animList);
    motion.Seq = 6;
    motion.motionFlags = 0;
    CHECK(anim_AddNextAnimSeq(animation, &motion, 0) == -1);
    CHECK(node_count(&animation->animFreeList) == 8);
    motion.motionFlags = 0x00000020u;
    CHECK(anim_AddNextAnimSeq(animation, &motion, 0) == 0);
    node = (animListNode *)animation->animList.head;
    CHECK(node->pAnimTemplate == &target_templates[6]);

    animation->pCurrentAnimSeq = node;
    motion.motionFlags = 0x80000020u;
    CHECK(anim_AddNextAnimSeq(animation, &motion, 0) == -1);
    CHECK(node_count(&animation->animList) == 1);

    list_MoveList(&animation->animFreeList, &animation->animList);
    animation->pCurrentAnimSeq = NULL;
    motion.Seq = 1;
    motion.motionFlags = 0;
    for (index = 0;
         index < JPB_ANIM_QUEUE_NODE_CAPACITY;
         ++index) {
        CHECK(anim_AddNextAnimSeq(animation, &motion, 0) == 0);
    }
    CHECK(anim_AddNextAnimSeq(animation, &motion, 0) == -1);
    CHECK(node_count(&animation->animList) ==
          JPB_ANIM_QUEUE_NODE_CAPACITY);
    return 0;
}

static int test_motion_physics_handoff(void)
{
    playerObject player;
    playerObject target;
    sceneObject scene;
    sceneObject target_scene;
    physicsObject physics;
    physicsObject target_physics;
    Motion motion;

    connect_player(&player, &scene, &physics);
    connect_player(&target, &target_scene, &target_physics);
    player.target = &target;
    physics.vpos.vx = 10;
    physics.vpos.vz = 20;
    target_physics.vpos.vx = 50;
    target_physics.vpos.vz = 80;

    memset(&motion, 0, sizeof(motion));
    motion.vel = -20;
    motion.Charge = 120;
    motion.ChargeAcc = 15;
    physics.constmov.vy = 7.0f;
    jpb_AnimApplyMotionPhysics(&player, &motion);
    CHECK(physics.constmov.vx == 0.0f);
    CHECK(physics.constmov.vy == 7.0f);
    CHECK(physics.constmov.vz == 100.0f);
    CHECK(physics.accel.vz == 15.0f);

    motion.motionFlags = 0x00000008u;
    motion.ChargeAcc = -1;
    jpb_AnimApplyMotionPhysics(&player, &motion);
    CHECK(physics.constmov.vx == 100.0f);
    CHECK(physics.constmov.vz == 0.0f);
    CHECK(physics.accel.vz == 0.0f);
    return 0;
}

static int test_queued_motion_state_activation(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[4];
    Motion first;
    Motion second;
    animListNode *current;

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    connect_player(&player, &scene, &physics);
    animation->animRoot.objectID = 0;
    animation->animRoot.pParent = &scene.sceneRoot;
    scene.pAnim = &animation->animRoot;
    scene.pPlayer = &player.playerRoot;
    animation->depack_context.seqdata = templates;
    player.maxMotions = 4;
    player.currentMotion = 9;
    memset(templates, 0, sizeof(templates));
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    templates[2].Fframe = 4;
    templates[3].Fframe = 7;
    first.Seq = 2;
    first.twin = 3;
    first.twout = 6;
    first.Lock = 11;
    first.Speed = 100;
    first.vel = 10;
    first.Charge = 20;
    first.ChargeAcc = 5;
    first.FunctPtr = 9;
    gGlobalFrameRate = 2048;
    game_setFuncArray();

    CHECK(anim_AddNextAnimSeq(animation, &first, 0) == 0);
    CHECK(jpb_AnimActivateQueuedMotionState(animation) ==
          JPB_ANIM_PARTIAL_OK);
    CHECK(animation->animFrameIndex == 4 * JPB_FIXED_ONE);
    CHECK(animation->tweenLevel == 3);
    CHECK(animation->tweenFramesLeft == 0);
    CHECK(animation->dispIn == 0);
    CHECK(animation->animFrameRate == 110);
    CHECK(animation->animFrameAcc == 55);
    CHECK(animation->animGlobalFrameRateModifier == 0);
    CHECK(animation->Lock == 11);
    CHECK(animation->pMotion == &first);
    CHECK(player.previousMotion == 9);
    CHECK(player.currentMotion == 2);
    CHECK(player.pMotionCallBack ==
          force_AttackSpinCallBack);
    CHECK(physics.constmov.vz == 30.0f);
    CHECK(physics.accel.vz == 5.0f);

    second.Seq = 3;
    second.motionFlags = 1;
    second.twin = 2;
    second.Speed = 200;
    second.Lock = 12;
    second.FunctPtr =
        JPB_PLAYER_CALLBACK_CAPACITY;
    CHECK(anim_AddNextAnimSeq(animation, &second, 0) == 0);
    current = animation->pCurrentAnimSeq;
    CHECK(jpb_AnimActivateQueuedMotionState(animation) ==
          JPB_ANIM_PARTIAL_OK);
    CHECK(animation->pCurrentAnimSeq != current);
    CHECK(physics.angle.vy == 100);
    CHECK(animation->animFrameIndex == 7 * JPB_FIXED_ONE);
    CHECK(animation->tweenLevel == 6);
    CHECK(animation->animFrameRate == 220);
    CHECK(animation->animFrameAcc == 110);
    CHECK(animation->animGlobalFrameRateModifier == 0);
    CHECK(animation->pMotion == &second);
    CHECK(player.previousMotion == 2);
    CHECK(player.currentMotion == 3);
    CHECK(player.pMotionCallBack == NULL);
    CHECK(jpb_AnimActivateQueuedMotionState(animation) ==
          JPB_ANIM_PARTIAL_EMPTY);
    return 0;
}

static int test_sequence_end_transition(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[2];
    Motion looping;
    Motion strike;

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    connect_player(&player, &scene, &physics);
    animation->animRoot.objectID = 0;
    animation->animRoot.pParent = &scene.sceneRoot;
    scene.pAnim = &animation->animRoot;
    scene.pPlayer = &player.playerRoot;
    animation->depack_context.seqdata = templates;
    player.maxMotions = 2;
    memset(templates, 0, sizeof(templates));
    memset(&looping, 0, sizeof(looping));
    memset(&strike, 0, sizeof(strike));
    templates[0].Fframe = 2;
    templates[0].Lframe = 10;
    templates[1].Fframe = 4;
    templates[1].Lframe = 12;
    looping.Seq = 0;
    looping.motionFlags = UINT32_C(0x80000000);
    looping.Speed = JPB_FIXED_ONE;
    strike.Seq = 1;
    strike.Speed = JPB_FIXED_ONE;

    CHECK(anim_AddNextAnimSeq(
              animation, &looping, 0) == 0);
    CHECK(jpb_AnimActivateQueuedMotionState(
              animation) == JPB_ANIM_PARTIAL_OK);
    animation->animFrameIndex =
        templates[0].Lframe * JPB_FIXED_ONE;
    CHECK(jpb_AnimAdvanceQueuedMotionAtEnd(
              animation) == JPB_ANIM_PARTIAL_OK);
    CHECK(animation->animFrameIndex ==
          templates[0].Fframe * JPB_FIXED_ONE);
    CHECK(animation->pMotion == &looping);

    CHECK(anim_AddNextAnimSeq(
              animation, &strike, 0) == 0);
    looping.motionFlags = 0;
    animation->animFrameIndex =
        templates[0].Lframe * JPB_FIXED_ONE;
    CHECK(jpb_AnimAdvanceQueuedMotionAtEnd(
              animation) == JPB_ANIM_PARTIAL_OK);
    CHECK(animation->pMotion == &strike);
    CHECK(animation->animFrameIndex ==
          templates[1].Fframe * JPB_FIXED_ONE);
    CHECK(player.currentMotion == 1);
    return 0;
}

static int test_sequence_end_motion_recovery(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[21];
    Motion motions[21];

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    connect_player(&player, &scene, &physics);
    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));
    memset(&GameStruct, 0, sizeof(GameStruct));
    animation->animRoot.objectID = 0;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    scene.pAnim = &animation->animRoot;
    scene.pPlayer = &player.playerRoot;
    player.paMotions = motions;
    player.maxMotions = 21;
    player.oldmaxCMotions = 21;
    player.playernum = 0;
    player.currentMotion = 2;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[0].MaxEnergy = 100;
    templates[0].Fframe = 0;
    templates[0].Lframe = 8;
    templates[2].Fframe = 0;
    templates[2].Lframe = 10;
    motions[0].Seq = 0;
    motions[0].Speed = JPB_FIXED_ONE;
    motions[2].Seq = 2;
    motions[2].Speed = JPB_FIXED_ONE;
    motions[2].Charge = 20;

    CHECK(anim_AddNextAnimSeq(
              animation, &motions[2], 0) == 0);
    CHECK(jpb_AnimActivateQueuedMotionState(
              animation) == JPB_ANIM_PARTIAL_OK);
    CHECK(physics.constmov.vz == 20.0f);
    animation->animFrameIndex =
        templates[2].Lframe * JPB_FIXED_ONE;

    CHECK(jpb_AnimAdvanceQueuedMotionAtEnd(
              animation) == JPB_ANIM_PARTIAL_OK);
    CHECK(animation->pMotion == &motions[0]);
    CHECK(player.currentMotion == 0);
    CHECK(animation->animFrameIndex ==
          templates[0].Fframe * JPB_FIXED_ONE);
    CHECK(physics.constmov.vx == 0.0f);
    CHECK(physics.constmov.vy == 0.0f);
    CHECK(physics.constmov.vz == 0.0f);
    return 0;
}

static int test_motion_chain_wrapper(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[2];
    Motion motion;

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    connect_player(&player, &scene, &physics);
    animation->animRoot.pParent = &scene.sceneRoot;
    scene.pAnim = &animation->animRoot;
    scene.pPlayer = &player.playerRoot;
    animation->depack_context.seqdata = templates;
    player.maxMotions = 2;
    memset(templates, 0, sizeof(templates));
    memset(&motion, 0, sizeof(motion));
    motion.Seq = 1;
    motion.Speed = -1;

    CHECK(animctrl_MotionChain(&player.playerRoot, &motion) == 1);
    CHECK(animation->animList.head != NULL);
    motion.Seq = 2;
    CHECK(animctrl_MotionChain(&player.playerRoot, &motion) == 0);
    return 0;
}

static int test_motion_lock_wrappers(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[2];
    Motion motion;

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    connect_player(&player, &scene, &physics);
    animation->animRoot.pParent = &scene.sceneRoot;
    scene.pAnim = &animation->animRoot;
    scene.pPlayer = &player.playerRoot;
    animation->depack_context.seqdata = templates;
    player.maxMotions = 2;
    memset(templates, 0, sizeof(templates));
    memset(&motion, 0, sizeof(motion));
    templates[1].Lframe = 10;
    motion.Seq = 1;
    motion.Speed = 0;

    animation->Lock = 5;
    motion.Lock = 5;
    CHECK(animctrl_MotionLock(
              &player.playerRoot, &motion) == 0);
    CHECK(animation->pCurrentAnimSeq == NULL);
    CHECK(animctrl_MotionEqualLock(
              &player.playerRoot, &motion) == 1);
    CHECK(animation->pMotion == &motion);
    CHECK(animation->Lock == 5);
    CHECK((animation->animFlags & UINT32_C(0x20)) != 0);

    motion.Lock = 4;
    CHECK(animctrl_MotionEqualLock(
              &player.playerRoot, &motion) == 0);
    motion.Lock = 6;
    motion.motionFlags |= UINT32_C(0x20000000);
    CHECK(animctrl_MotionLock(
              &player.playerRoot, &motion) == 1);
    CHECK(animation->Lock == 6);
    CHECK((animation->animFlags & UINT32_C(0x20000000)) != 0);

    motion.Lock = 7;
    CHECK(animctrl_MotionLockLevel(
              &player.playerRoot, &motion, 6) == 0);
    CHECK(animctrl_MotionLockLevel(
              &player.playerRoot, &motion, 7) == 1);
    CHECK(animation->Lock == 7);

    motion.Lock = 1;
    CHECK(animctrl_MotionNoLock(
              &player.playerRoot, &motion) == 1);
    CHECK(animation->Lock == 1);
    return 0;
}

static int test_motion_combo_chain_wrapper(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[2];
    Motion motion;

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    connect_player(&player, &scene, &physics);
    animation->animRoot.pParent = &scene.sceneRoot;
    scene.pAnim = &animation->animRoot;
    scene.pPlayer = &player.playerRoot;
    animation->depack_context.seqdata = templates;
    player.maxMotions = 2;
    memset(templates, 0, sizeof(templates));
    memset(&motion, 0, sizeof(motion));
    templates[1].Lframe = 10;
    motion.Seq = 1;
    motion.Speed = -1;

    animation->Lock = 30;
    CHECK(animctrl_MotionComboChain(
              &player.playerRoot,
              &motion,
              0,
              0,
              1) == 0);
    CHECK((motion.motionFlags &
           UINT32_C(0x02000002)) ==
          UINT32_C(0x02000002));

    CHECK(animctrl_MotionComboChain(
              &player.playerRoot,
              &motion,
              0,
              1,
              0) == 1);
    CHECK(animation->pMotion == &motion);

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    animation->animRoot.pParent = &scene.sceneRoot;
    scene.pAnim = &animation->animRoot;
    animation->depack_context.seqdata = templates;
    CHECK(animctrl_MotionComboChain(
              &player.playerRoot,
              &motion,
              1,
              0,
              0) == -1);
    CHECK(animation->animList.head != NULL);
    CHECK(animation->pCurrentAnimSeq == NULL);
    return 0;
}

static int test_ai_throw_callback(void)
{
    playerObject player;
    playerObject target;
    sceneObject scene;
    sceneObject target_scene;
    physicsObject physics;
    physicsObject target_physics;
    animObject *animation;
    animObject *target_animation;
    _animTemplate templates[2];
    Motion motions[2];

    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    target_animation = &maAnimationData[2];
    connect_player(&player, &scene, &physics);
    connect_player(
        &target, &target_scene, &target_physics);

    player.playerRoot.objectID = 0;
    player.playernum = 0;
    player.target = &target;
    player.locked = &target;
    player.paMotions = motions;
    player.maxMotions = 2;
    player.oldmaxCMotions = 2;
    player.pMotion = &animation->pMotion;
    target.playerRoot.objectID = 2;
    target.playernum = 2;
    target.target = &player;
    target.locked = &player;
    target.pFlags = UINT32_C(0x00400000);
    target.paMotions = motions;
    target.maxMotions = 2;
    target.oldmaxCMotions = 2;
    target.pMotion = &target_animation->pMotion;

    scene.pAnim = &animation->animRoot;
    scene.pPlayer = &player.playerRoot;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    animation->depack_context3.seqdata = templates;
    animation->pMotion = &motions[0];
    target_scene.pAnim = &target_animation->animRoot;
    target_scene.pPlayer = &target.playerRoot;
    target_animation->animRoot.pParent =
        &target_scene.sceneRoot;
    target_animation->depack_context.seqdata = templates;
    target_animation->depack_context3.seqdata = templates;

    motions[0].Seq = 0;
    motions[0].Lock = 1;
    motions[0].Speed = -1;
    motions[0].fx2 = 0;
    motions[1].Seq = 1;
    motions[1].Speed = -1;
    templates[0].Lframe = 8;
    templates[1].Lframe = 8;
    physics.vpos.vz = 0;
    target_physics.vpos.vz = 64;
    physics.angle.vy = 0;
    maRange[0][2] = -1.0f;

    CHECK(ai_Throw(NULL, &player) == 1);
    CHECK(player.locked == NULL);
    CHECK(target.locked == NULL);
    CHECK(target.target == &player);
    CHECK((target.pFlags &
           UINT32_C(0x00400000)) == 0);
    CHECK((target.pFlags &
           UINT32_C(0x801)) == UINT32_C(0x801));
    CHECK((motions[1].motionFlags &
           UINT32_C(0x40000020)) ==
          UINT32_C(0x40000020));
    CHECK(motions[1].FunctPtr == 5);
    CHECK(motions[1].Lock == 0x1e);
    CHECK(animation->pMotion == &motions[0]);
    CHECK(target_animation->pMotion == &motions[1]);

    return 0;
}

static int test_animation_control_utilities(void)
{
    playerObject player;
    playerObject target;
    sceneObject scene;
    sceneObject target_scene;
    physicsObject physics;
    physicsObject target_physics;
    animObject animation;
    animObject target_animation;
    animListNode current;
    animListNode queued;
    _animTemplate templates[2];
    _animTemplate target_templates[2];
    Motion motion;
    int first;
    int last;

    connect_player(&player, &scene, &physics);
    connect_player(&target, &target_scene, &target_physics);
    memset(&animation, 0, sizeof(animation));
    memset(&target_animation, 0, sizeof(target_animation));
    memset(&current, 0, sizeof(current));
    memset(&queued, 0, sizeof(queued));
    memset(templates, 0, sizeof(templates));
    memset(target_templates, 0, sizeof(target_templates));
    memset(&motion, 0, sizeof(motion));
    player.target = &target;
    animation.animRoot.pParent = &scene.sceneRoot;
    target_animation.animRoot.pParent = &target_scene.sceneRoot;
    scene.pAnim = &animation.animRoot;
    scene.pPlayer = &player.playerRoot;
    target_scene.pAnim = &target_animation.animRoot;
    target_scene.pPlayer = &target.playerRoot;
    animation.depack_context.seqdata = templates;
    target_animation.depack_context.seqdata =
        target_templates;
    target_animation.depack_context.numparts = 17;
    motion.Seq = 1;
    templates[1].Fframe = 10;
    templates[1].Lframe = 30;
    target_templates[1].Fframe = 40;
    current.pAnimTemplate = &templates[1];
    current.pMotion = &motion;
    current.Lock = 12;
    animation.pCurrentAnimSeq = &current;
    animation.pMotion = &motion;
    animation.Lock = 22;
    animation.animFrameIndex = 18 * JPB_FIXED_ONE;

    anim_GetSeqFrameRange(
        &player.playerRoot, &motion, &first, &last);
    CHECK(first == 10);
    CHECK(last == 30);
    CHECK(anim_GetTargetContext(&animation) ==
          &target_animation.depack_context);
    CHECK(anim_GetTargetPartNum(&animation) == 17);
    CHECK(anim_GetTargetSeqPtr(&animation, &motion) ==
          &target_templates[1]);
    anim_gDumpSeq(1, &templates[1]);

    CHECK(animutl_GetCurrentLock(&player.playerRoot) == 22);
    CHECK(animutl_GetLockLevel(&player.playerRoot) == 12);
    CHECK(animutl_GetPercentPlayed(&player.playerRoot) == 8);
    animation.animFrameIndex = 8 * JPB_FIXED_ONE;
    CHECK(animutl_GetPercentPlayed(&player.playerRoot) == 0);
    animation.tweenFramesLeft = 7;
    CHECK(animutl_GetTweeningFramesLeft(
              &player.playerRoot) == 7);
    CHECK(animutl_gGetCurrentAnimLength(
              &player.playerRoot) == 20);
    CHECK(animutl_gGetCurrentFrameIndex(
              &player.playerRoot) == -2);

    motion.Damage = 0;
    CHECK(animutl_GetWindow(&player.playerRoot) == 6);
    animation.Lock = 23;
    CHECK(animutl_GetWindow(&player.playerRoot) == 20);
    motion.Damage = 1;
    motion.cutin = 5;
    motion.disp = 3;
    animation.animFrameIndex = 12 * JPB_FIXED_ONE;
    CHECK(animutl_GetWindow(&player.playerRoot) == 3);
    animation.animFrameIndex = 20 * JPB_FIXED_ONE;
    CHECK(animutl_GetWindow(&player.playerRoot) == -7);
    animation.animFrameIndex = 29 * JPB_FIXED_ONE;
    CHECK(animutl_GetWindow(&player.playerRoot) == 7);
    motion.cutin = UINT8_MAX;
    CHECK(animutl_GetWindow(&player.playerRoot) == 20);

    animation.Lock = 20;
    animutl_SetCurrentLock(&player.playerRoot, 25);
    CHECK(animation.Lock == 20);
    animutl_SetCurrentLock(&player.playerRoot, 10);
    CHECK(animation.Lock == 10);
    animutl_SetCurrentLock(&player.playerRoot, -1);
    CHECK(animation.Lock == UINT16_MAX);

    animation.animFlags = 0x10;
    animutl_gPauseAnim(&player.playerRoot);
    CHECK(animation.animFlags == 0x14);
    animutl_gUnPauseAnim(&player.playerRoot);
    CHECK(animation.animFlags == 0x10);
    animutl_gSetAnimFrameRate(&player.playerRoot, 1000);
    animutl_gScaleAnimFrameRate(
        &player.playerRoot, JPB_FIXED_ONE / 2);
    CHECK(animation.animFrameRate == 500);
    animutl_gSetAnimFrameRate(&player.playerRoot, -1000);
    animutl_gScaleAnimFrameRate(
        &player.playerRoot, JPB_FIXED_ONE / 2);
    CHECK(animation.animFrameRate == -500);
    animutl_gSetCurrentFrameIndex(&player.playerRoot, 3);
    CHECK(animation.animFrameIndex == 13 * JPB_FIXED_ONE);

    list_InitList(&animation.animFreeList);
    list_InitList(&animation.animList);
    list_AddTail(&animation.animList, &queued.anm_Node);
    animutl_FlushSeqQueue(&player.playerRoot);
    CHECK(animation.animList.head == NULL);
    CHECK(animation.animFreeList.head == &queued.anm_Node);
    return 0;
}

static int test_animation_sound_scheduler(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate sequence;
    Motion motion;
    char saved_level = LevelSelect;
    uint32_t saved_timer = gGlobalTimer;

    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    connect_player(&player, &scene, &physics);
    scene.pPlayer = &player.playerRoot;
    scene.pAnim = &animation->animRoot;
    animation->animRoot.objectID = 0;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = &sequence;
    player.maxMotions = 1;
    player.oldmaxCMotions = 1;
    player.playernum = 0;
    memset(&sequence, 0, sizeof(sequence));
    memset(&motion, 0, sizeof(motion));
    sequence.Lframe = 10;
    motion.Seq = 0;
    motion.Speed = JPB_FIXED_ONE;
    motion.FunctPtr = -1;
    memcpy(motion.snd[0], "swing", 5);
    memcpy(motion.snd[1], "step", 4);
    motion.sndDelay[1] = 2;
    LevelSelect = 1;
    gGlobalTimer = 1000;
    anim_sound_play_calls = 0;
    anim_sound_last_bank = -1;
    memset(anim_sound_last_name, 0, sizeof(anim_sound_last_name));
    jpb_SoundSetPlaySfxHook(anim_sound_play_hook, NULL);

    CHECK(anim_AddNextAnimSeq(animation, &motion, 0) == 0);
    CHECK(jpb_AnimActivateQueuedMotionState(animation) ==
          JPB_ANIM_PARTIAL_OK);
    CHECK(anim_sound_play_calls == 1);
    CHECK(anim_sound_last_bank == 1);
    CHECK(strcmp(anim_sound_last_name, "swing") == 0);
    CHECK(animation->loopHandle[0] == 71);
    CHECK(animation->soundTimer[1] == 2024);

    gGlobalTimer = 2023;
    jpb_AnimHandleSoundState(animation);
    CHECK(anim_sound_play_calls == 1);
    gGlobalTimer = 2024;
    jpb_AnimHandleSoundState(animation);
    CHECK(anim_sound_play_calls == 2);
    CHECK(strcmp(anim_sound_last_name, "step") == 0);
    CHECK(animation->soundTimer[1] == 0);

    memcpy(motion.snd[0], "taxiaway", 8);
    CHECK(shouldReplayAnimSound(0, animation) == 0);
    memcpy(motion.snd[0], "ordinary", 8);
    CHECK(shouldReplayAnimSound(0, animation) == 1);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    LevelSelect = saved_level;
    gGlobalTimer = saved_timer;
    return 0;
}

static int test_global_animation_scheduler(void)
{
    animObject *active;
    animObject *hidden;
    animListNode active_sequence;
    animListNode hidden_sequence;
    _animTemplate active_template;
    _animTemplate hidden_template;
    Motion active_motion;
    Motion hidden_motion;
    _animFrame hidden_frame;

    anim_InitAnimations(0);
    jpb_SceneInitPool(0);
    memset(&active_sequence, 0, sizeof(active_sequence));
    memset(&hidden_sequence, 0, sizeof(hidden_sequence));
    memset(&active_template, 0, sizeof(active_template));
    memset(&hidden_template, 0, sizeof(hidden_template));
    memset(&active_motion, 0, sizeof(active_motion));
    memset(&hidden_motion, 0, sizeof(hidden_motion));
    memset(&hidden_frame, 0, sizeof(hidden_frame));

    active = &maAnimationData[3];
    active->animRoot.objectID = 3;
    active->pCurrentAnimSeq = &active_sequence;
    active->pMotion = &active_motion;
    active->pCurrentAnimFrame = &active->tweenAnimFrame;
    active->tweenFramesLeft = 2;
    active->tweenAnimFrame.v3RootTranslation.vx = 10;
    active->tweenDeltaTranslation.vx = 4;
    active_sequence.pAnimTemplate = &active_template;
    active_template.Lframe = 10;

    hidden = &maAnimationData[4];
    hidden->animRoot.objectID = 4;
    hidden->animRoot.flags = UINT32_C(0x20);
    hidden->pCurrentAnimSeq = &hidden_sequence;
    hidden->pMotion = &hidden_motion;
    hidden->pCurrentAnimFrame = &hidden_frame;
    hidden->tweenFramesLeft = 2;
    hidden_sequence.pAnimTemplate = &hidden_template;
    hidden_template.Lframe = 10;

    anim_ProcessAnimations();
    CHECK(active->tweenFramesLeft == 1);
    CHECK(active->tweenAnimFrame.v3RootTranslation.vx == 14);
    CHECK(maSceneData[3].pKeyFrameModel ==
          &active->tweenAnimFrame);
    CHECK(hidden->tweenFramesLeft == 2);
    CHECK(maSceneData[4].pKeyFrameModel == NULL);
    return 0;
}

static int test_animation_console_command(void)
{
    WorldData world;
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[4];
    Motion motions[4];
    char *play_arguments[] = {"PLAY", "1", "2"};
    int play_values[] = {0, 1, 2};
    char *fx_arguments[] = {"fx", "0", "0"};
    int fx_values[] = {0, -1, 200};
    WorldData *saved_world = gpWorld;

    memset(&world, 0, sizeof(world));
    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    connect_player(&player, &scene, &physics);
    scene.pPlayer = &player.playerRoot;
    scene.pAnim = &animation->animRoot;
    animation->animRoot.objectID = 0;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    player.paMotions = motions;
    player.maxMotions = 3;
    player.oldmaxCMotions = 3;
    motions[1].Seq = 1;
    motions[1].Lock = 2;
    motions[1].Speed = JPB_FIXED_ONE;
    motions[1].FunctPtr = -1;
    motions[2].Seq = 2;
    motions[2].Lock = 2;
    motions[2].Speed = JPB_FIXED_ONE;
    motions[2].FunctPtr = -1;
    templates[1].Lframe = 8;
    templates[2].Lframe = 8;
    world.player0 = &player;
    gpWorld = &world;

    CHECK(console_AnimCommand(
              3, play_arguments, play_values, NULL) == 0);
    CHECK(animation->pMotion == &motions[1]);
    CHECK(node_count(&animation->animList) == 1);
    CHECK(((animListNode *)animation->animList.head)->pMotion ==
          &motions[2]);

    CHECK(console_AnimCommand(
              3, fx_arguments, fx_values, NULL) == 0);
    CHECK(motions[0].fx1 == 0x54);
    fx_values[1] = 99;
    fx_values[2] = -1;
    CHECK(console_AnimCommand(
              3, fx_arguments, fx_values, NULL) == 0);
    CHECK(motions[3].fx1 == 0);

    gpWorld = saved_world;
    return 0;
}

int main(void)
{
    CHECK(test_motion_layout() == 0);
    CHECK(test_animation_pool() == 0);
    CHECK(test_freeze_window() == 0);
    CHECK(test_animation_queue() == 0);
    CHECK(test_motion_physics_handoff() == 0);
    CHECK(test_queued_motion_state_activation() == 0);
    CHECK(test_sequence_end_transition() == 0);
    CHECK(test_sequence_end_motion_recovery() == 0);
    CHECK(test_motion_chain_wrapper() == 0);
    CHECK(test_motion_lock_wrappers() == 0);
    CHECK(test_motion_combo_chain_wrapper() == 0);
    CHECK(test_ai_throw_callback() == 0);
    CHECK(test_animation_control_utilities() == 0);
    CHECK(test_animation_sound_scheduler() == 0);
    CHECK(test_global_animation_scheduler() == 0);
    CHECK(test_animation_console_command() == 0);
    puts("animation tests passed");
    return 0;
}
