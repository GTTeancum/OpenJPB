#include "jpb/anim.h"
#include "jpb/camera.h"
#include "jpb/cad.h"
#include "jpb/huffman.h"
#include "jpb/game.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/unpack.h"

#include <stddef.h>
#include <stdint.h>
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

enum {
    TEST_SEQUENCE_COUNT = 2,
    TEST_SEQUENCE_OFFSET = 20,
    TEST_BITSTREAM_OFFSET =
        TEST_SEQUENCE_OFFSET +
        TEST_SEQUENCE_COUNT * sizeof(_animTemplate),
    TEST_MOTION_OFFSET = TEST_BITSTREAM_OFFSET + 8,
    TEST_PAYLOAD_SIZE =
        TEST_MOTION_OFFSET +
        TEST_SEQUENCE_COUNT * sizeof(Motion),
    TEST_FILE_SIZE = TEST_PAYLOAD_SIZE + 4
};

typedef union AlignedCadBuffer {
    uint64_t alignment;
    uint8_t bytes[TEST_FILE_SIZE];
} AlignedCadBuffer;

static void write_u16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static void append_lsb_bits(
    uint32_t *words,
    int *bit_cursor,
    uint32_t value,
    int bit_count)
{
    int bit;

    for (bit = 0; bit < bit_count; ++bit) {
        if ((value & ((uint32_t)1 << bit)) != 0) {
            words[*bit_cursor / 32] |=
                (uint32_t)1 << (*bit_cursor % 32);
        }
        ++*bit_cursor;
    }
}

static void make_cad(AlignedCadBuffer *storage)
{
    uint8_t *file = storage->bytes;
    uint8_t *payload = file + 4;
    Motion *motions;

    memset(storage, 0, sizeof(*storage));
    write_u32(file, TEST_PAYLOAD_SIZE);
    write_u32(payload, TEST_BITSTREAM_OFFSET);
    write_u32(payload + 4, TEST_SEQUENCE_OFFSET);
    write_u32(payload + 8, TEST_MOTION_OFFSET);
    write_u16(payload + 12, 24);
    write_u16(payload + 16, TEST_SEQUENCE_COUNT);
    write_u32(payload + TEST_BITSTREAM_OFFSET, 0x11223344u);
    write_u32(
        payload + TEST_BITSTREAM_OFFSET + 4,
        0x55667788u);
    motions = (Motion *)(payload + TEST_MOTION_OFFSET);
    motions[0].Seq = 0;
    memcpy(motions[0].name, "idle", sizeof("idle"));
    motions[1].Seq = 1;
    motions[1].vel = 27;
    memcpy(motions[1].name, "walk", sizeof("walk"));
}

static int test_cad_view_and_context(void)
{
    AlignedCadBuffer storage;
    JPBCadView view;
    _dpcontext context;

    make_cad(&storage);
    CHECK(jpb_CadInspect(
              storage.bytes, sizeof(storage.bytes), &view) ==
          JPB_CAD_OK);
    CHECK(view.payload_size == TEST_PAYLOAD_SIZE);
    CHECK(view.sequence_count == TEST_SEQUENCE_COUNT);
    CHECK(view.part_count == 24);
    CHECK(view.bitstream_size == 8);
    CHECK(
        view.depack_window_size ==
        TEST_PAYLOAD_SIZE - TEST_BITSTREAM_OFFSET);
    CHECK(view.motions[1].vel == 27);
    CHECK(strcmp(view.motions[1].name, "walk") == 0);

    memset(&context, 0xa5, sizeof(context));
    unpack_initcontext(&context, view.payload);
    CHECK(context.huffdataorigin ==
          (uint32_t *)view.bitstream);
    CHECK(context.huffdata ==
          (uint32_t *)view.bitstream + 2);
    CHECK(context.n_bitmask == 0x11223344u);
    CHECK(context.huffdword == 0x55667788u);
    CHECK(context.huffbits == 32);
    CHECK(context.wordsinbuffer == 0);
    CHECK(context.numseq == TEST_SEQUENCE_COUNT);
    CHECK(context.numparts == 24);
    CHECK(context.seqdata == view.sequences);

    context.n_bitmask = 0;
    context.huffdword = 0;
    unpack_seekcontext(&context, 0);
    CHECK(context.n_bitmask == 0x11223344u);
    CHECK(context.huffdword == 0x55667788u);
    return 0;
}

static int test_animation_object_binding(void)
{
    AlignedCadBuffer storage;
    JPBCadView view;
    sceneObject scene;
    sceneObject reused_scene;
    objectRoot reused_actor;
    animObject *animation;

    make_cad(&storage);
    CHECK(jpb_CadInspect(
              storage.bytes, sizeof(storage.bytes), &view) ==
          JPB_CAD_OK);
    anim_InitAnimations(0);
    memset(&scene, 0, sizeof(scene));
    scene.sceneRoot.objectID = 0;
    animation =
        anim_CreateObject(&scene, view.payload, NULL, 99);
    CHECK(animation == &maAnimationData[0]);
    CHECK(animation->animRoot.objectID == 0);
    CHECK(animation->animRoot.pParent == &scene.sceneRoot);
    CHECK(scene.pAnim == &animation->animRoot);
    CHECK(animation->paMotions == view.motions);
    CHECK(animation->depack_context.seqdata == view.sequences);
    CHECK(animation->depack_context3.seqdata == view.sequences);
    CHECK(animation->depack_context.numseq ==
          TEST_SEQUENCE_COUNT);

    memset(&reused_scene, 0, sizeof(reused_scene));
    memset(&reused_actor, 0, sizeof(reused_actor));
    reused_actor.pParent = &scene.sceneRoot;
    CHECK(anim_CreateObject(
              &reused_scene, NULL, &reused_actor, 0) ==
          animation);
    CHECK(reused_scene.pAnim == &animation->animRoot);
    CHECK(animation->animRoot.pParent ==
          &scene.sceneRoot);
    return 0;
}

static int test_huffman_vector_decode(void)
{
    _optab table[256];
    uint16_t values[6] = {
        0x07ff, 0x0400, 1, 2, 3, 4
    };
    uint32_t tree[1] = {0};
    uint32_t trailing_words[2] = {0};
    int16_t vectors[8];
    _dpcontext context;

    memset(table, 0, sizeof(table));
    memset(vectors, 0, sizeof(vectors));
    memset(&context, 0, sizeof(context));
    table[0x34].vals = (5 << 28);
    unpack_init(table, values, tree, 1);
    context.n_bitmask = 0x12345634u;
    context.huffdword = 0;
    context.huffbits = 32;
    context.huffdata = trailing_words;
    unpack_grabsvectors_s(&context, 2, vectors);
    CHECK(vectors[0] == -1);
    CHECK(vectors[1] == -2048);
    CHECK(vectors[2] == 1);
    CHECK(vectors[3] == 0);
    CHECK(vectors[4] == 4);
    CHECK(vectors[5] == 6);
    CHECK(vectors[6] == 8);
    CHECK(vectors[7] == 0);

    values[0] = 0x0fff;
    values[1] = 0x0123;
    values[2] = 0x07ff;
    values[3] = 0;
    values[4] = 1;
    table[0x34].vals = (4 << 28);
    memset(vectors, 0, sizeof(vectors));
    memset(&context, 0, sizeof(context));
    context.n_bitmask = 0x12345634u;
    context.huffbits = 32;
    context.huffdata = trailing_words;
    unpack_grabsvectors_s(&context, 1, vectors);
    CHECK(vectors[0] == -1);
    CHECK(vectors[1] == 0);
    CHECK(vectors[2] == 1);
    CHECK(vectors[3] == 0x0123);
    return 0;
}

static int test_raw_vector_decode(void)
{
    uint32_t words[3] = {0, 0, 0};
    int bit_cursor = 0;
    int16_t vector[4] = {0, 0, 0, 0};
    _dpcontext context;

    append_lsb_bits(words, &bit_cursor, 1, 1);
    append_lsb_bits(words, &bit_cursor, 0x07ff, 11);
    append_lsb_bits(words, &bit_cursor, 0x0400, 11);
    append_lsb_bits(words, &bit_cursor, 1, 11);
    append_lsb_bits(words, &bit_cursor, 0x1234, 16);
    CHECK(bit_cursor == 50);

    memset(&context, 0, sizeof(context));
    context.n_bitmask = words[0];
    context.huffdword = words[1];
    context.huffbits = 32;
    context.huffdata = &words[2];
    unpack_grabsvectors_raw(&context, 1, vector);
    CHECK(vector[0] == -1);
    CHECK(vector[1] == -2048);
    CHECK(vector[2] == 1);
    CHECK(vector[3] == 0x1234);
    return 0;
}

static int test_huffman_table_validation(void)
{
    JPBHuffmanTableSet tables;

    memset(&tables, 0, sizeof(tables));
    CHECK(
        jpb_HuffmanValidateTables(&tables) ==
        JPB_HUFFMAN_OK);

    tables.options[3].vals =
        (1 << 28) | (JPB_HUFFMAN_VALUE_COUNT - 1);
    CHECK(
        jpb_HuffmanValidateTables(&tables) ==
        JPB_HUFFMAN_INVALID_TABLE);

    memset(&tables, 0, sizeof(tables));
    tables.tree[7] = JPB_HUFFMAN_TREE_COUNT;
    CHECK(
        jpb_HuffmanValidateTables(&tables) ==
        JPB_HUFFMAN_INVALID_TABLE);

    tables.tree[7] = 0x9000u;
    CHECK(
        jpb_HuffmanValidateTables(&tables) ==
        JPB_HUFFMAN_INVALID_TABLE);
    return 0;
}

static int test_animation_frame_accumulation(void)
{
    _optab table[256];
    uint16_t values[12] = {
        1, 2, 3, 4, 5, 6,
        1, 1, 1, 0x07ff, 1, 1
    };
    uint32_t tree[1] = {0};
    uint32_t words[8] = {0};
    int bit_cursor = 0;
    animObject animation;
    animListNode current_sequence;
    _animTemplate sequence;
    Motion motion;
    playerObject player;
    playerObject target_player;
    sceneObject scene;
    sceneObject target_scene;
    animObject target_animation;
    physicsObject physics;
    _animFrame *frame;

    append_lsb_bits(words, &bit_cursor, 0, 1);
    append_lsb_bits(words, &bit_cursor, 10, 11);
    append_lsb_bits(words, &bit_cursor, 20, 11);
    append_lsb_bits(words, &bit_cursor, 30, 11);
    append_lsb_bits(words, &bit_cursor, 0, 1);
    append_lsb_bits(words, &bit_cursor, 1, 11);
    append_lsb_bits(words, &bit_cursor, 2, 11);
    append_lsb_bits(words, &bit_cursor, 3, 11);
    CHECK(bit_cursor == 68);
    append_lsb_bits(words, &bit_cursor, 0x34, 7);
    append_lsb_bits(words, &bit_cursor, 0x56, 8);

    memset(table, 0, sizeof(table));
    table[0x34].vals = (5 << 28);
    table[0x56].vals = (5 << 28) | 6;
    unpack_init(table, values, tree, 1);

    memset(&animation, 0, sizeof(animation));
    memset(&current_sequence, 0, sizeof(current_sequence));
    memset(&sequence, 0, sizeof(sequence));
    memset(&motion, 0, sizeof(motion));
    sequence.Lframe = 3;
    sequence.parts = 1;
    current_sequence.pAnimTemplate = &sequence;
    current_sequence.pMotion = &motion;
    animation.pCurrentAnimSeq = &current_sequence;
    animation.pMotion = &motion;
    animation.pCurrentAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.pPreviousAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.depack_context.huffdataorigin = words;

    CHECK(
        jpb_AnimDecodeFrameState(&animation, &frame) ==
        JPB_ANIM_PARTIAL_OK);
    CHECK(frame == &animation.AnimFrameBuffer[1]);
    CHECK(animation.pPreviousAnimFrame ==
          &animation.AnimFrameBuffer[0]);
    CHECK(animation.animFrameIndex == JPB_FIXED_ONE);
    CHECK(frame->v3RootTranslation.vx == 10);
    CHECK(frame->v3RootTranslation.vy == 40);
    CHECK(frame->v3RootTranslation.vz == 30);
    CHECK(frame->av3JointAngle[0].vx == 2);
    CHECK(frame->av3JointAngle[0].vy == 4);
    CHECK(frame->av3JointAngle[0].vz == 6);

    animation.animFrameAcc = JPB_FIXED_ONE;
    CHECK(
        jpb_AnimDecodeFrameState(&animation, &frame) ==
        JPB_ANIM_PARTIAL_OK);
    CHECK(animation.animFrameIndex == 2 * JPB_FIXED_ONE);
    CHECK(frame->v3RootTranslation.vx == 11);
    CHECK(frame->v3RootTranslation.vy == 44);
    CHECK(frame->v3RootTranslation.vz == 33);
    CHECK(frame->av3JointAngle[0].vx == 10);
    CHECK(frame->av3JointAngle[0].vy == 14);
    CHECK(frame->av3JointAngle[0].vz == 18);

    animation.animFrameAcc = JPB_FIXED_ONE;
    CHECK(
        jpb_AnimDecodeFrameState(&animation, &frame) ==
        JPB_ANIM_PARTIAL_OK);
    CHECK(animation.animFrameIndex == 3 * JPB_FIXED_ONE);
    CHECK(frame->v3RootTranslation.vx == 13);
    CHECK(frame->v3RootTranslation.vy == 50);
    CHECK(frame->v3RootTranslation.vz == 37);
    CHECK(frame->av3JointAngle[0].vx == 16);
    CHECK(frame->av3JointAngle[0].vy == 26);
    CHECK(frame->av3JointAngle[0].vz == 32);

    animation.animFrameAcc = JPB_FIXED_ONE;
    frame->event[0] = 0x7f;
    CHECK(
        jpb_AnimDecodeFrameState(&animation, &frame) ==
        JPB_ANIM_PARTIAL_OK);
    CHECK(animation.animFrameIndex == 3 * JPB_FIXED_ONE);
    CHECK(frame->event[0] == 0);

    /*
     * The first publication count is dispIn + Motion.cutin. Motion.disp is
     * independently consumed by anim_CheckSlack and must not skip decode
     * frames. These unequal values protect the exact +0x0A/+0x0C offsets.
     */
    memset(animation.AnimFrameBuffer, 0,
           sizeof(animation.AnimFrameBuffer));
    animation.pCurrentAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.pPreviousAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.animFrameIndex = 0;
    animation.animFrameAcc = 0;
    animation.curBufferId = 0;
    animation.depack_context.huffdataorigin = words;
    motion.cutin = 2;
    motion.disp = 0;
    CHECK(
        jpb_AnimDecodeFrameState(&animation, &frame) ==
        JPB_ANIM_PARTIAL_OK);
    CHECK(animation.animFrameIndex == 2 * JPB_FIXED_ONE);

    memset(animation.AnimFrameBuffer, 0,
           sizeof(animation.AnimFrameBuffer));
    animation.pCurrentAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.pPreviousAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.animFrameIndex = 0;
    animation.animFrameAcc = 0;
    animation.curBufferId = 0;
    animation.depack_context.huffdataorigin = words;
    motion.cutin = 0;
    motion.disp = 2;
    CHECK(
        jpb_AnimDecodeFrameState(&animation, &frame) ==
        JPB_ANIM_PARTIAL_OK);
    CHECK(animation.animFrameIndex == JPB_FIXED_ONE);

    memset(animation.AnimFrameBuffer, 0,
           sizeof(animation.AnimFrameBuffer));
    animation.pCurrentAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.pPreviousAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.animFrameIndex = 0;
    animation.animFrameAcc = 0;
    animation.curBufferId = 0;
    animation.depack_context.huffdataorigin = words;
    sequence.pad1 = 1;
    motion.disp = 0;
    CHECK(
        jpb_AnimDecodeFrameState(&animation, &frame) ==
        JPB_ANIM_PARTIAL_OK);
    CHECK(frame == &animation.AnimFrameBuffer[1]);
    CHECK(animation.animFrameIndex == 2 * JPB_FIXED_ONE);
    CHECK(frame->v3RootTranslation.vx == 11);
    CHECK(frame->v3RootTranslation.vy == 44);
    CHECK(frame->v3RootTranslation.vz == 33);
    CHECK(frame->av3JointAngle[0].vx == 10);
    CHECK(frame->av3JointAngle[0].vy == 14);
    CHECK(frame->av3JointAngle[0].vz == 18);

    memset(animation.AnimFrameBuffer, 0,
           sizeof(animation.AnimFrameBuffer));
    memset(&player, 0, sizeof(player));
    memset(&target_player, 0, sizeof(target_player));
    memset(&scene, 0, sizeof(scene));
    memset(&target_scene, 0, sizeof(target_scene));
    memset(&target_animation, 0, sizeof(target_animation));
    animation.pCurrentAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.pPreviousAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.animFrameIndex = 0;
    animation.animFrameAcc = 0;
    animation.curBufferId = 0;
    animation.animRoot.pParent = &scene.sceneRoot;
    scene.pPlayer = &player.playerRoot;
    player.target = &target_player;
    target_player.playerRoot.pParent = &target_scene.sceneRoot;
    target_scene.pAnim = &target_animation.animRoot;
    target_animation.depack_context3.huffdataorigin = words;
    sequence.pad1 = 0;
    motion.motionFlags = UINT32_C(0x20);
    CHECK(
        jpb_AnimDecodeFrameState(&animation, &frame) ==
        JPB_ANIM_PARTIAL_OK);
    CHECK(frame->v3RootTranslation.vx == 10);
    CHECK(frame->v3RootTranslation.vy == 40);
    CHECK(frame->v3RootTranslation.vz == 30);

    memset(&physics, 0, sizeof(physics));
    memset(&current_sequence, 0, sizeof(current_sequence));
    memset(&animation, 0, sizeof(animation));
    memset(&scene, 0, sizeof(scene));
    memset(&player, 0, sizeof(player));
    memset(&motion, 0, sizeof(motion));
    memset(&sequence, 0, sizeof(sequence));
    list_InitList(&animation.animList);
    list_InitList(&animation.animFreeList);
    list_AddTail(
        &animation.animFreeList,
        &current_sequence.anm_Node);
    animation.animRoot.objectID = 0;
    animation.animRoot.pParent = &scene.sceneRoot;
    animation.pCurrentAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.pPreviousAnimFrame =
        &animation.AnimFrameBuffer[0];
    animation.animFrameRate = JPB_FIXED_ONE;
    animation.depack_context.seqdata = &sequence;
    animation.depack_context.huffdataorigin = words;
    scene.pAnim = &animation.animRoot;
    scene.pPlayer = &player.playerRoot;
    scene.pPhysics = &physics.physicsRoot;
    player.playerRoot.pParent = &scene.sceneRoot;
    player.paMotions = &motion;
    player.maxMotions = 1;
    player.oldmaxCMotions = 1;
    motion.Seq = 0;
    motion.Speed = JPB_FIXED_ONE;
    motion.Lock = 4;
    motion.FunctPtr = -1;
    sequence.Lframe = 3;
    sequence.parts = 1;
    gGlobalFrameRate = 0;
    CHECK(anim_AddNextAnimSeq(
              &animation, &motion, 1) == 0);
    CHECK(anim_ForceNextAnimSeq(&animation, 0) == 0);
    CHECK((animation.animFlags & UINT32_C(0x20)) != 0);
    CHECK(animation.pMotion == &motion);
    CHECK(animation.pCurrentAnimFrame ==
          &animation.AnimFrameBuffer[1]);
    CHECK(animation.animFrameIndex == JPB_FIXED_ONE);
    CHECK(animation.pCurrentAnimFrame->v3RootTranslation.vx == 10);
    CHECK(animation.pCurrentAnimFrame->v3RootTranslation.vy == 40);
    CHECK(animation.pCurrentAnimFrame->v3RootTranslation.vz == 30);
    return 0;
}

static int test_animation_tween_publication(void)
{
    _optab table[256];
    uint16_t values[12] = {
        1, 2, 3, 4, 5, 6,
        1, 1, 1, 0x07ff, 1, 1
    };
    uint32_t tree[1] = {0};
    uint32_t words[8] = {0};
    int bit_cursor = 0;
    animObject animation;
    animListNode current_sequence;
    _animTemplate sequence;
    Motion motion;
    _animFrame *frame;

    append_lsb_bits(words, &bit_cursor, 0, 1);
    append_lsb_bits(words, &bit_cursor, 10, 11);
    append_lsb_bits(words, &bit_cursor, 20, 11);
    append_lsb_bits(words, &bit_cursor, 30, 11);
    append_lsb_bits(words, &bit_cursor, 0, 1);
    append_lsb_bits(words, &bit_cursor, 1, 11);
    append_lsb_bits(words, &bit_cursor, 2, 11);
    append_lsb_bits(words, &bit_cursor, 3, 11);
    memset(table, 0, sizeof(table));
    table[0x34].vals = (5 << 28);
    table[0x56].vals = (5 << 28) | 6;
    unpack_init(table, values, tree, 1);

    memset(&animation, 0, sizeof(animation));
    memset(&current_sequence, 0, sizeof(current_sequence));
    memset(&sequence, 0, sizeof(sequence));
    memset(&motion, 0, sizeof(motion));
    sequence.Lframe = 3;
    sequence.parts = 1;
    current_sequence.pAnimTemplate = &sequence;
    current_sequence.pMotion = &motion;
    animation.pCurrentAnimSeq = &current_sequence;
    animation.pMotion = &motion;
    animation.pCurrentAnimFrame = &animation.AnimFrameBuffer[0];
    animation.pPreviousAnimFrame = &animation.AnimFrameBuffer[0];
    animation.depack_context.huffdataorigin = words;
    animation.tweenLevel = 4;
    animation.AnimFrameBuffer[0].v3RootTranslation.vx = 30;
    animation.AnimFrameBuffer[0].v3RootTranslation.vy = 0;
    animation.AnimFrameBuffer[0].v3RootTranslation.vz = 10;
    animation.AnimFrameBuffer[0].av3JointAngle[0].vx = -2;
    animation.AnimFrameBuffer[0].av3JointAngle[0].vy = 0;
    animation.AnimFrameBuffer[0].av3JointAngle[0].vz = 2;
    animation.AnimFrameBuffer[0].av3JointAngle[0].pad = 7;

    CHECK(jpb_AnimCreateTweenFrameState(
              &animation, &frame) == JPB_ANIM_PARTIAL_OK);
    CHECK(frame == &animation.tweenAnimFrame);
    CHECK(animation.animFrameIndex == JPB_FIXED_ONE);
    CHECK(animation.tweenFramesLeft == 3);
    CHECK((animation.animFlags & 0x40u) != 0);
    CHECK(frame->v3RootTranslation.vx == 25);
    CHECK(frame->v3RootTranslation.vy == 10);
    CHECK(frame->v3RootTranslation.vz == 15);
    CHECK(frame->av3JointAngle[0].vx == -1);
    CHECK(frame->av3JointAngle[0].vy == 1);
    CHECK(frame->av3JointAngle[0].vz == 3);
    CHECK(frame->av3JointAngle[0].pad == 7);

    CHECK(jpb_AnimStepFrameState(
              &animation, &frame) == JPB_ANIM_PARTIAL_OK);
    CHECK(frame == &animation.tweenAnimFrame);
    CHECK(animation.tweenFramesLeft == 2);
    CHECK(frame->v3RootTranslation.vx == 20);
    CHECK(frame->v3RootTranslation.vy == 20);
    CHECK(frame->v3RootTranslation.vz == 20);
    CHECK(frame->av3JointAngle[0].vx == 0);
    CHECK(frame->av3JointAngle[0].vy == 2);
    CHECK(frame->av3JointAngle[0].vz == 4);
    return 0;
}

static int test_invalid_cad_rejected(void)
{
    AlignedCadBuffer storage;
    uint8_t misaligned[TEST_FILE_SIZE + 4];
    uint8_t *misaligned_file = misaligned;
    JPBCadView view;

    make_cad(&storage);
    CHECK(jpb_CadInspect(storage.bytes, 8, &view) ==
          JPB_CAD_TRUNCATED);
    write_u32(storage.bytes, TEST_PAYLOAD_SIZE - 1);
    CHECK(jpb_CadInspect(
              storage.bytes, sizeof(storage.bytes), &view) ==
          JPB_CAD_INVALID_SIZE);
    make_cad(&storage);
    write_u32(storage.bytes + 4 + 8, UINT32_MAX);
    CHECK(jpb_CadInspect(
              storage.bytes, sizeof(storage.bytes), &view) ==
          JPB_CAD_INVALID_OFFSET);
    make_cad(&storage);
    write_u32(
        storage.bytes + 4,
        TEST_BITSTREAM_OFFSET + 4);
    CHECK(jpb_CadInspect(
              storage.bytes, sizeof(storage.bytes), &view) ==
          JPB_CAD_INVALID_LAYOUT);
    make_cad(&storage);
    while (((uintptr_t)misaligned_file & 3u) == 0) {
        ++misaligned_file;
    }
    memcpy(misaligned_file, storage.bytes, TEST_FILE_SIZE);
    CHECK(jpb_CadInspect(
              misaligned_file, TEST_FILE_SIZE, &view) ==
          JPB_CAD_INVALID_LAYOUT);
    return 0;
}

int main(void)
{
    CHECK(test_cad_view_and_context() == 0);
    CHECK(test_animation_object_binding() == 0);
    CHECK(test_huffman_vector_decode() == 0);
    CHECK(test_raw_vector_decode() == 0);
    CHECK(test_huffman_table_validation() == 0);
    CHECK(test_animation_frame_accumulation() == 0);
    CHECK(test_animation_tween_publication() == 0);
    CHECK(test_invalid_cad_rejected() == 0);
    puts("CAD tests passed");
    return 0;
}
