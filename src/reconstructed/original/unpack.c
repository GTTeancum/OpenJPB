/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\unpack.c.
 *
 * The reviewed subset establishes the Huffman tables, bit reservoir, cached
 * value path, compressed-vector decode, and pointer-based CAD context.
 *
 * Provenance:
 *   direct     - names/signatures and _dpcontext layout from the exact PDB.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - bit shifts/refills, lookup/tree selection, vector stores,
 *                pointer increments, header widths, and resets checked at
 *                RVAs 0x103260..0x10396C.
 *
 * PDB module: 0089
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\unpack.obj
 * Primary source: W:\SWJediPowerBattles\Work\unpack.c
 * Compiler language: c
 * Emitted procedures: 7
 *
 * Procedure/type names are retained exactly. Expanded identifiers such as
 * context/count/vectors replace terse PDB parameter names (ct/n/s) where
 * that materially improves readability; the exact originals remain in
 * inventory/functions.json.
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/unpack.h"

static _optab *optab;
static uint16_t *vals;
static uint32_t *smalltree;

static uint32_t unpack_low_mask(int bits)
{
    if (bits >= 32) {
        return UINT32_MAX;
    }
    return ((uint32_t)1 << bits) - 1u;
}

static int16_t unpack_sign_extend11(uint16_t value);

/* 0x103260, 169 bytes, local, 5 named locals
 * flushbits
 * PDB type: void (_dpcontext*, int)
 * Source: W:\SWJediPowerBattles\Work\unpack.c
 */
static void flushbits(_dpcontext *context, int bits)
{
    uint32_t next_bits;
    uint32_t shifted =
        context->n_bitmask >> bits;
    int available = context->huffbits;

    if (available < bits) {
        uint32_t next_word = *context->huffdata++;
        int needed = bits - available;

        if (available != 0) {
            next_bits =
                context->huffdword |
                ((next_word & unpack_low_mask(needed))
                 << available);
        } else {
            next_bits =
                next_word & unpack_low_mask(bits);
        }
        context->huffdword = next_word >> needed;
        context->huffbits = 32 - needed;
    } else {
        next_bits =
            context->huffdword & unpack_low_mask(bits);
        context->huffdword >>= bits;
        context->huffbits = available - bits;
    }
    context->n_bitmask =
        shifted | (next_bits << (32 - bits));
}

/* 0x103310, 332 bytes, local, 7 named locals
 * huffgetword
 * PDB type: unsigned short (_dpcontext*)
 * Source: W:\SWJediPowerBattles\Work\unpack.c
 */
static uint16_t huffgetword(_dpcontext *context)
{
    _optab *entry;

    if (context->wordsinbuffer != 0) {
        --context->wordsinbuffer;
        return *context->wordbuffer++;
    }

    entry = &optab[(uint8_t)context->n_bitmask];
    if (entry->vals >= 0) {
        int bits =
            7 - ((uint32_t)entry->gotto >> 28 & 7u);

        if (bits == 0) {
            bits = 8;
        }
        flushbits(context, bits);
        context->wordsinbuffer =
            entry->vals >> 28;
        context->wordbuffer =
            &vals[(uint32_t)entry->vals & 0x7fffu];
        return *context->wordbuffer++;
    }

    {
        uint32_t *node =
            &smalltree[(uint32_t)entry->gotto & 0x7fffu];

        flushbits(context, 8);
        for (;;) {
            uint32_t branches = *node;
            uint32_t bit =
                context->n_bitmask & 1u;
            uint16_t selected =
                bit != 0
                    ? (uint16_t)(branches >> 16)
                    : (uint16_t)branches;

            flushbits(context, 1);
            if ((int16_t)selected < 0) {
                return selected & 0x7fffu;
            }
            node = &smalltree[selected];
        }
    }
}

/* 0x103460, 869 bytes, global, 21 named locals
 * unpack_grabsvectors_raw
 * PDB type: void (_dpcontext*, int, short*)
 * Source: W:\SWJediPowerBattles\Work\unpack.c
 */
static uint32_t unpack_read_bits(
    _dpcontext *context, int bits)
{
    uint32_t value =
        context->n_bitmask & unpack_low_mask(bits);

    flushbits(context, bits);
    return value;
}

void unpack_grabsvectors_raw(
    _dpcontext *context, int count, int16_t *vectors)
{
    int index;

    for (index = 0; index < count; ++index) {
        uint16_t has_pad =
            (uint16_t)unpack_read_bits(context, 1);
        uint16_t x =
            (uint16_t)unpack_read_bits(context, 11);
        uint16_t y =
            (uint16_t)unpack_read_bits(context, 11);
        uint16_t z =
            (uint16_t)unpack_read_bits(context, 11);
        uint16_t pad =
            has_pad != 0
                ? (uint16_t)unpack_read_bits(context, 16)
                : 0;

        if (index == 0) {
            vectors[0] =
                unpack_sign_extend11(x);
            vectors[1] =
                (int16_t)((uint16_t)
                    unpack_sign_extend11(y) * 2u);
            vectors[2] =
                unpack_sign_extend11(z);
        } else {
            vectors[0] =
                (int16_t)(x * 2u);
            vectors[1] =
                (int16_t)(y * 2u);
            vectors[2] =
                (int16_t)(z * 2u);
        }
        vectors[3] = (int16_t)pad;
        vectors += 4;
    }
}

/* 0x1037D0, 211 bytes, global, 5 named locals
 * unpack_grabsvectors_s
 * PDB type: void (_dpcontext*, int, short*)
 * Source: W:\SWJediPowerBattles\Work\unpack.c
 */
static int16_t unpack_sign_extend11(uint16_t value)
{
    uint16_t extended =
        (value & 0x0400u) != 0
            ? value | 0xf800u
            : value & 0x07ffu;

    return (int16_t)extended;
}

void unpack_grabsvectors_s(
    _dpcontext *context, int count, int16_t *vectors)
{
    int index;

    for (index = 0; index < count; ++index) {
        uint16_t x = huffgetword(context);
        uint16_t y;
        uint16_t z;
        uint16_t pad = 0;

        if (x == 0x0fffu) {
            pad = huffgetword(context);
            x = huffgetword(context);
        }
        y = huffgetword(context);
        z = huffgetword(context);

        if (index == 0) {
            vectors[0] =
                unpack_sign_extend11(x);
            vectors[1] =
                (int16_t)((uint16_t)
                    unpack_sign_extend11(y) * 2u);
            vectors[2] =
                unpack_sign_extend11(z);
        } else {
            vectors[0] =
                (int16_t)((uint16_t)x * 2u);
            vectors[1] =
                (int16_t)((uint16_t)y * 2u);
            vectors[2] =
                (int16_t)((uint16_t)z * 2u);
        }
        vectors[3] = (int16_t)pad;
        vectors += 4;
    }
}

/* 0x1038B0, 22 bytes, global, 4 named locals
 * unpack_init
 * PDB type: void (_optab*, unsigned short*, ...
 * Source: W:\SWJediPowerBattles\Work\unpack.c
 */

void unpack_init(
    _optab *optable,
    uint16_t *values,
    uint32_t *tree,
    int tree_size)
{
    (void)tree_size;
    optab = optable;
    vals = values;
    smalltree = tree;
}

/* 0x1038D0, 84 bytes, global, 2 named locals
 * unpack_initcontext
 * PDB type: void (_dpcontext*, char*)
 * Source: W:\SWJediPowerBattles\Work\unpack.c
 */
void unpack_initcontext(_dpcontext *context, char *data)
{
    int32_t *header = (int32_t *)data;
    uint32_t *words =
        (uint32_t *)(data + header[0]);

    context->huffdata = words;
    context->huffdataorigin = words;
    context->n_bitmask = *words++;
    context->huffdata = words;
    context->huffdword = *words++;
    context->huffdata = words;
    context->huffbits = 0x20;
    context->wordsinbuffer = 0;
    context->seqdata =
        (_animTemplate *)(data + header[1]);
    context->numseq = (uint16_t)header[4];
    context->numparts = (uint16_t)header[3];
}

/* 0x103930, 60 bytes, global, 2 named locals
 * unpack_seekcontext
 * PDB type: void (_dpcontext*, int)
 * Source: W:\SWJediPowerBattles\Work\unpack.c
 */
void unpack_seekcontext(_dpcontext *context, int seek)
{
    uint32_t *words =
        context->huffdataorigin + (seek >> 2);

    context->huffdata = words;
    context->n_bitmask = *words++;
    context->huffdata = words;
    context->huffdword = *words++;
    context->huffdata = words;
    context->huffbits = 0x20;
    context->wordsinbuffer = 0;
}
