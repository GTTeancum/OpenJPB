#include "jpb/alloc.h"

#include <stdint.h>
#include <stdio.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int test_sizes_and_reuse(void)
{
    void *a;
    void *b;
    void *reused;

    meminit();
    CHECK(mem_heap != NULL);
    CHECK(mem_heapend - mem_heap == 0x4000);
    a = memalloc(4);
    b = memalloc(5);
    CHECK(a != NULL);
    CHECK(b != NULL);
    CHECK(memsize(a) == 8);
    CHECK(memsize(b) == 12);

    memfree(a);
    reused = memalloc(4);
    CHECK(reused == a);
    return 0;
}

static int test_zero_and_capacity_boundaries(void)
{
    void *zero;
    void *whole_heap;

    meminit();
    zero = memalloc(0);
    CHECK(zero != NULL);
    CHECK(memsize(zero) == 4);
    memfree(zero);

    whole_heap = memalloc(65532);
    CHECK(whole_heap != NULL);
    CHECK(memsize(whole_heap) == 65536);
    CHECK(memalloc(1) == NULL);

    meminit();
    CHECK(memalloc(65533) == NULL);
    return 0;
}

static int test_bidirectional_coalescing(void)
{
    void *a;
    void *b;
    void *c;
    void *merged;

    meminit();
    a = memalloc(100);
    b = memalloc(200);
    c = memalloc(300);
    CHECK(a != NULL && b != NULL && c != NULL);

    memfree(b);
    memfree(a);
    merged = memalloc(300);
    CHECK(merged == a);

    meminit();
    a = memalloc(100);
    b = memalloc(200);
    c = memalloc(300);
    memfree(a);
    memfree(c);
    memfree(b);
    merged = memalloc(65532);
    CHECK(merged == a);
    return 0;
}

int main(void)
{
    int result = 0;

    result |= test_sizes_and_reuse();
    result |= test_zero_and_capacity_boundaries();
    result |= test_bidirectional_coalescing();
    return result;
}
