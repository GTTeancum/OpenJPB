/*
 * Reviewed reconstruction of W:\SWJediPowerBattles\Work\alloc.c.
 *
 * Provenance:
 *   direct     - API signatures, local names, code extents, heap size, and
 *                header constants from the exact PDB / executable pair.
 *   decompiled - boundary-tag behavior checked against Ghidra and x64
 *                instructions at RVAs 0x17470 through 0x1763B.
 *
 * Blocks are measured in 32-bit words and include their header word. Header
 * bits 0..14 hold the block length, bit 15 marks allocation, and bits 16..30
 * hold the preceding block length. The original allocator is intentionally
 * linear, four-byte aligned, unsynchronized, and unchecked on memfree.
 */

#include "jpb/alloc.h"

#include <stdint.h>

#define MEM_HEAP_WORDS 0x4000u
#define MEM_LENGTH_MASK 0x7fffu
#define MEM_ALLOCATED 0x8000u

/*
 * Two boundary words reproduce the writable location immediately following
 * the 0x4000-word heap in the reference BSS without exposing it as payload.
 */
static uint32_t mem_blob[MEM_HEAP_WORDS + 2];
static uint32_t mem_heap_words;
uint32_t *mem_heap;
uint32_t *mem_heapend;

/* Reference RVA 0x17470, 201 bytes. */
void *memalloc(unsigned size)
{
    uint32_t requested_words = (size + 7u) >> 2;
    uint32_t current = 0;

    for (;;) {
        uint32_t header = mem_heap[current];
        uint32_t length = header & MEM_LENGTH_MASK;

        if ((header & MEM_ALLOCATED) == 0 && requested_words <= length) {
            uint32_t next = current + requested_words;
            uint32_t spare = length - requested_words;
            uint32_t following;
            uint32_t preceding_length;

            mem_heap[current] =
                (header & 0xffff8000u) | requested_words | MEM_ALLOCATED;

            preceding_length = requested_words | MEM_ALLOCATED;
            if ((int32_t)spare > 0) {
                if (next < mem_heap_words) {
                    mem_heap[next] = (requested_words << 16) | spare;
                }
                following = next + spare;
                preceding_length = spare;
            } else {
                following = next;
            }

            if (following < mem_heap_words) {
                mem_heap[following] =
                    (preceding_length << 16)
                    | (mem_heap[following] & 0xffffu);
            }
            return &mem_heap[current + 1];
        }

        current += length;
        if (current >= mem_heap_words) {
            return 0;
        }
    }
}

/* Reference RVA 0x17540, 165 bytes. */
void memfree(void *ptr)
{
    uint32_t *header_ptr = (uint32_t *)ptr - 1;
    uint32_t current = (uint32_t)(header_ptr - mem_heap);
    uint32_t header = *header_ptr;
    uint32_t length = header & MEM_LENGTH_MASK;
    uint32_t previous_length = (header >> 16) & MEM_LENGTH_MASK;
    uint32_t previous = current - previous_length;
    uint32_t next = current + length;
    uint32_t previous_header = mem_heap[previous];
    uint32_t start;

    if ((previous_header & MEM_ALLOCATED) == 0) {
        length += previous_header & MEM_LENGTH_MASK;
        start = previous;
    } else {
        start = current;
    }

    if (next < mem_heap_words) {
        uint32_t next_header = mem_heap[next];
        uint32_t following = next;

        if ((next_header & MEM_ALLOCATED) == 0) {
            uint32_t next_length = next_header & MEM_LENGTH_MASK;

            length += next_length;
            following = next + next_length;
        }
        mem_heap[following] =
            (length << 16) | (mem_heap[following] & 0xffffu);
    }

    mem_heap[start] = (mem_heap[start] & 0xffff0000u) | length;
}

/* Reference RVA 0x175F0, 49 bytes. */
void meminit(void)
{
    mem_heap_words = MEM_HEAP_WORDS;
    mem_heap = mem_blob;
    mem_heapend = mem_blob + MEM_HEAP_WORDS;
    mem_heap[0] = MEM_HEAP_WORDS;
}

/* Reference RVA 0x17630, 12 bytes. */
unsigned memsize(void *ptr)
{
    uint32_t header = *((uint32_t *)ptr - 1);

    return (header & MEM_LENGTH_MASK) << 2;
}
