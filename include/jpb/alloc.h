#ifndef JPB_ALLOC_H
#define JPB_ALLOC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact PDB globals; localheap includes the writable terminal boundary tag. */
extern unsigned mem_heaplen;
extern unsigned localheap[0x4001];
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
