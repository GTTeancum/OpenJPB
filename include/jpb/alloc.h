#ifndef JPB_ALLOC_H
#define JPB_ALLOC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * mem_heap is a direct optimized-local name from the PDB. mem_heapend is an
 * inferred name for the companion bound used by inlined ownership checks.
 */
extern uint32_t *mem_heap;
extern uint32_t *mem_heapend;

void *memalloc(unsigned size);
void memfree(void *ptr);
void meminit(void);
unsigned memsize(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
