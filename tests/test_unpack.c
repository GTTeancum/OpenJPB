#include "jpb/unpack.h"

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

static void init_stream_context(
    _dpcontext *context,
    uint32_t *words)
{
    memset(context, 0, sizeof(*context));
    context->n_bitmask = words[0];
    context->huffdword = words[1];
    context->huffbits = 32;
    context->huffdata = &words[2];
}

static int test_context_initialization_and_seek(void)
{
    uint32_t payload[20] = {0};
    _dpcontext context;
    uint16_t *wordbuffer = (uint16_t *)(uintptr_t)0x1234;
    uint16_t *handyvalues = (uint16_t *)(uintptr_t)0x5678;

    payload[0] = 20;
    payload[1] = 48;
    payload[3] = 24;
    payload[4] = 3;
    payload[5] = 0x11223344u;
    payload[6] = 0x55667788u;
    payload[7] = 0x99aabbccu;
    memset(&context, 0xa5, sizeof(context));
    context.wordbuffer = wordbuffer;
    context.handyvalues = handyvalues;

    unpack_initcontext(&context, (char *)payload);
    CHECK(context.wordbuffer == wordbuffer);
    CHECK(context.handyvalues == handyvalues);
    CHECK(context.wordsinbuffer == 0);
    CHECK(context.huffdataorigin == &payload[5]);
    CHECK(context.huffdata == &payload[7]);
    CHECK(context.n_bitmask == 0x11223344u);
    CHECK(context.huffdword == 0x55667788u);
    CHECK(context.huffbits == 32);
    CHECK(context.numseq == 3);
    CHECK(context.numparts == 24);
    CHECK(context.seqdata == (_animTemplate *)((char *)payload + 48));

    context.wordsinbuffer = 7;
    unpack_seekcontext(&context, 4);
    CHECK(context.wordbuffer == wordbuffer);
    CHECK(context.handyvalues == handyvalues);
    CHECK(context.wordsinbuffer == 0);
    CHECK(context.huffdataorigin == &payload[5]);
    CHECK(context.huffdata == &payload[8]);
    CHECK(context.n_bitmask == 0x55667788u);
    CHECK(context.huffdword == 0x99aabbccu);
    CHECK(context.huffbits == 32);
    CHECK(context.numseq == 3);
    CHECK(context.numparts == 24);
    return 0;
}

static int test_fast_table_and_value_cache(void)
{
    _optab table[256];
    uint16_t values[3] = {0x07ff, 0x0400, 1};
    uint32_t tree[1] = {0};
    uint32_t words[3] = {0x1234565au, 0x89abcdefu, 0};
    int16_t vector[4] = {0};
    _dpcontext context;

    memset(table, 0, sizeof(table));
    table[0x5a].vals = 2 << 28;
    table[0x5a].gotto = 7 << 28;
    unpack_init(table, values, tree, -1);
    init_stream_context(&context, words);

    unpack_grabsvectors_s(&context, 1, vector);
    CHECK(vector[0] == -1);
    CHECK(vector[1] == -2048);
    CHECK(vector[2] == 1);
    CHECK(vector[3] == 0);
    CHECK(context.wordbuffer == &values[3]);
    CHECK(context.wordsinbuffer == 0);
    CHECK(context.n_bitmask == 0xef123456u);
    CHECK(context.huffdword == 0x0089abcdu);
    CHECK(context.huffbits == 24);
    CHECK(context.huffdata == &words[2]);
    return 0;
}

static int test_slow_tree_across_refill(void)
{
    _optab table[256];
    uint16_t values[1] = {0};
    uint32_t tree[1] = {0x80028001u};
    uint32_t words[4] = {0};
    static const unsigned branches[9] = {
        0, 1, 0,
        1, 0, 1,
        0, 1, 0
    };
    int16_t vectors[12] = {0};
    _dpcontext context;
    int bit_cursor = 0;
    int index;

    memset(table, 0, sizeof(table));
    table[0xa5].vals = -1;
    table[0xa5].gotto = 0;
    for (index = 0; index < 9; ++index) {
        append_lsb_bits(words, &bit_cursor, 0xa5, 8);
        append_lsb_bits(words, &bit_cursor, branches[index], 1);
    }
    CHECK(bit_cursor == 81);

    unpack_init(table, values, tree, 1);
    init_stream_context(&context, words);
    unpack_grabsvectors_s(&context, 3, vectors);
    CHECK(vectors[0] == 1);
    CHECK(vectors[1] == 4);
    CHECK(vectors[2] == 1);
    CHECK(vectors[3] == 0);
    CHECK(vectors[4] == 4);
    CHECK(vectors[5] == 2);
    CHECK(vectors[6] == 4);
    CHECK(vectors[7] == 0);
    CHECK(vectors[8] == 2);
    CHECK(vectors[9] == 4);
    CHECK(vectors[10] == 2);
    CHECK(vectors[11] == 0);
    CHECK(context.huffdata == &words[4]);
    CHECK(context.huffbits == 15);
    return 0;
}

static int test_raw_vectors_and_nonpositive_count(void)
{
    uint32_t words[4] = {0};
    int16_t vectors[8] = {0};
    int16_t untouched[4] = {11, 22, 33, 44};
    _dpcontext context;
    _dpcontext snapshot;
    int bit_cursor = 0;

    append_lsb_bits(words, &bit_cursor, 1, 1);
    append_lsb_bits(words, &bit_cursor, 0x07ff, 11);
    append_lsb_bits(words, &bit_cursor, 0x0400, 11);
    append_lsb_bits(words, &bit_cursor, 1, 11);
    append_lsb_bits(words, &bit_cursor, 0x1234, 16);
    append_lsb_bits(words, &bit_cursor, 0, 1);
    append_lsb_bits(words, &bit_cursor, 3, 11);
    append_lsb_bits(words, &bit_cursor, 4, 11);
    append_lsb_bits(words, &bit_cursor, 0x07ff, 11);
    CHECK(bit_cursor == 84);

    init_stream_context(&context, words);
    unpack_grabsvectors_raw(&context, 2, vectors);
    CHECK(vectors[0] == -1);
    CHECK(vectors[1] == -2048);
    CHECK(vectors[2] == 1);
    CHECK(vectors[3] == 0x1234);
    CHECK(vectors[4] == 6);
    CHECK(vectors[5] == 8);
    CHECK(vectors[6] == 4094);
    CHECK(vectors[7] == 0);
    CHECK(context.huffdata == &words[4]);
    CHECK(context.huffbits == 12);

    snapshot = context;
    unpack_grabsvectors_raw(&context, 0, untouched);
    CHECK(memcmp(&context, &snapshot, sizeof(context)) == 0);
    CHECK(untouched[0] == 11 && untouched[3] == 44);
    unpack_grabsvectors_s(&context, -1, untouched);
    CHECK(memcmp(&context, &snapshot, sizeof(context)) == 0);
    CHECK(untouched[0] == 11 && untouched[3] == 44);
    return 0;
}

int main(void)
{
    if (test_context_initialization_and_seek() != 0) {
        return 1;
    }
    if (test_fast_table_and_value_cache() != 0) {
        return 1;
    }
    if (test_slow_tree_across_refill() != 0) {
        return 1;
    }
    if (test_raw_vectors_and_nonpositive_count() != 0) {
        return 1;
    }

    puts("unpack tests passed");
    return 0;
}
