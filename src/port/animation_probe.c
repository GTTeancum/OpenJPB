#include "jpb/cad.h"
#include "jpb/huffman.h"
#include "jpb/unpack.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t checksum_vectors(
    uint64_t checksum, const int16_t *vectors, size_t value_count)
{
    size_t index;

    for (index = 0; index < value_count; ++index) {
        uint16_t value = (uint16_t)vectors[index];

        checksum ^= (uint8_t)value;
        checksum *= UINT64_C(1099511628211);
        checksum ^= (uint8_t)(value >> 8);
        checksum *= UINT64_C(1099511628211);
    }
    return checksum;
}

static int context_consumed_bits(
    const _dpcontext *context,
    const JPBCadView *view,
    uint32_t sequence_address,
    uint64_t *consumed_bits)
{
    uintptr_t cursor = (uintptr_t)context->huffdata;
    uint32_t seek_offset = sequence_address & ~3u;
    uintptr_t sequence_begin =
        (uintptr_t)view->bitstream + seek_offset;
    uintptr_t payload_end =
        (uintptr_t)view->payload + view->payload_size;
    uint64_t loaded_bits;
    uint64_t buffered_bits;

    if (cursor < sequence_begin ||
        cursor > payload_end ||
        consumed_bits == NULL ||
        context->huffbits < 0 ||
        context->huffbits > 32) {
        return 0;
    }
    loaded_bits =
        (uint64_t)(cursor - sequence_begin) * 8u;
    buffered_bits =
        32u + (uint32_t)context->huffbits;
    if (loaded_bits < buffered_bits) {
        return 0;
    }
    *consumed_bits = loaded_bits - buffered_bits;
    return 1;
}

static int context_consumption_is_in_bounds(
    const _dpcontext *context,
    const JPBCadView *view,
    uint32_t sequence_address)
{
    uint64_t consumed_bits;

    return context_consumed_bits(
               context,
               view,
               sequence_address,
               &consumed_bits) &&
           consumed_bits <=
        ((uint64_t)view->depack_window_size -
         (sequence_address & ~3u)) *
            8u;
}

int main(int argc, char **argv)
{
    JPBHuffmanTableSet tables;
    JPBHuffmanResult huffman_result;
    uint8_t *cad_storage;
    JPBCadView cad;
    JPBCadResult cad_result;
    uint64_t checksum = UINT64_C(14695981039346656037);
    size_t raw_frame_count = 0;
    size_t compressed_frame_count = 0;
    size_t sequence_index;

    if (argc != 5) {
        fprintf(
            stderr,
            "usage: %s animation.cad huffman.tab "
            "huffman.val huffman.opt\n",
            argv[0]);
        return 2;
    }
    huffman_result = jpb_HuffmanLoadFiles(
        argv[2], argv[3], argv[4], &tables);
    if (huffman_result != JPB_HUFFMAN_OK) {
        fprintf(
            stderr,
            "invalid Huffman files: result=%d\n",
            (int)huffman_result);
        return 3;
    }
    jpb_HuffmanUseTables(&tables);

    cad_storage =
        (uint8_t *)calloc(1, JPB_CAD_REFERENCE_CAPACITY);
    if (cad_storage == NULL) {
        fputs("failed to allocate CAD storage\n", stderr);
        return 4;
    }
    cad_result = jpb_CadLoadFile(
        argv[1],
        cad_storage,
        JPB_CAD_REFERENCE_CAPACITY,
        &cad);
    if (cad_result != JPB_CAD_OK) {
        fprintf(
            stderr,
            "invalid CAD %s: result=%d\n",
            argv[1],
            (int)cad_result);
        free(cad_storage);
        return 5;
    }

    for (sequence_index = 0;
         sequence_index < cad.sequence_count;
         ++sequence_index) {
        const _animTemplate *sequence =
            &cad.sequences[sequence_index];
        animObject animation;
        animListNode current_sequence;
        Motion motion;
        _animFrame *decoded_frame;
        int vector_count = (int)sequence->parts + 1;
        int frame_index;

        if (vector_count < 1 ||
            vector_count > JPB_ANIM_JOINT_CAPACITY + 1 ||
            (sequence->FframeAddr & ~3u) >
                cad.depack_window_size ||
            cad.depack_window_size -
                    (sequence->FframeAddr & ~3u) <
                8) {
            fprintf(
                stderr,
                "invalid sequence metadata: index=%zu "
                "address=%" PRIu32 " parts=%d\n",
                sequence_index,
                sequence->FframeAddr,
                (int)sequence->parts);
            free(cad_storage);
            return 6;
        }

        if (sequence->Lframe <= sequence->Fframe) {
            continue;
        }
        memset(&animation, 0, sizeof(animation));
        memset(&current_sequence, 0, sizeof(current_sequence));
        memset(&motion, 0, sizeof(motion));
        current_sequence.pAnimTemplate =
            (_animTemplate *)sequence;
        current_sequence.pMotion = &motion;
        animation.pCurrentAnimSeq = &current_sequence;
        animation.pMotion = &motion;
        animation.pCurrentAnimFrame =
            &animation.AnimFrameBuffer[0];
        animation.pPreviousAnimFrame =
            &animation.AnimFrameBuffer[0];
        animation.animFrameIndex =
            (int32_t)sequence->Fframe * JPB_FIXED_ONE;
        unpack_initcontext(
            &animation.depack_context, cad.payload);

        if (jpb_AnimDecodeFrameState(
                &animation, &decoded_frame) !=
                JPB_ANIM_PARTIAL_OK ||
            decoded_frame == NULL ||
            !context_consumption_is_in_bounds(
                &animation.depack_context,
                &cad,
                sequence->FframeAddr)) {
            uint64_t consumed_bits = 0;

            (void)context_consumed_bits(
                &animation.depack_context,
                &cad,
                sequence->FframeAddr,
                &consumed_bits);
            fprintf(
                stderr,
                "raw frame crossed bitstream: index=%zu "
                "address=%" PRIu32 " cursor=%zu size=%zu "
                "consumed_bits=%" PRIu64 "\n",
                sequence_index,
                sequence->FframeAddr,
                (size_t)(
                    (uintptr_t)animation
                        .depack_context.huffdata -
                    (uintptr_t)cad.bitstream),
                cad.bitstream_size,
                consumed_bits);
            free(cad_storage);
            return 7;
        }
        checksum = checksum_vectors(
            checksum,
            (const int16_t *)&decoded_frame
                ->v3RootTranslation,
            (size_t)vector_count * 4);
        ++raw_frame_count;

        for (frame_index = sequence->Fframe + 1;
             frame_index < sequence->Lframe;
             ++frame_index) {
            animation.animFrameAcc = JPB_FIXED_ONE;
            if (jpb_AnimDecodeFrameState(
                    &animation, &decoded_frame) !=
                    JPB_ANIM_PARTIAL_OK ||
                decoded_frame == NULL) {
                fprintf(
                    stderr,
                    "frame accumulation failed: index=%zu\n",
                    sequence_index);
                free(cad_storage);
                return 8;
            }
            if (!context_consumption_is_in_bounds(
                    &animation.depack_context,
                    &cad,
                    sequence->FframeAddr)) {
                fprintf(
                    stderr,
                    "compressed frame crossed bitstream: "
                    "index=%zu\n",
                    sequence_index);
                free(cad_storage);
                return 8;
            }
            checksum = checksum_vectors(
                checksum,
                (const int16_t *)&decoded_frame
                    ->v3RootTranslation,
                (size_t)vector_count * 4);
            ++compressed_frame_count;
        }
        if (animation.animFrameIndex !=
            (int32_t)sequence->Lframe * JPB_FIXED_ONE) {
            fprintf(
                stderr,
                "frame endpoint mismatch: index=%zu "
                "actual=%" PRId32 " expected=%" PRId32 "\n",
                sequence_index,
                animation.animFrameIndex,
                (int32_t)sequence->Lframe *
                    JPB_FIXED_ONE);
            free(cad_storage);
            return 9;
        }
    }

    printf(
        "cad=%s sequences=%zu raw=%zu compressed=%zu "
        "checksum=%016" PRIx64 "\n",
        argv[1],
        sequence_index,
        raw_frame_count,
        compressed_frame_count,
        checksum);
    free(cad_storage);
    return 0;
}
