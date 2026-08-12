#ifndef JPB_GLOBALARRAYS_H
#define JPB_GLOBALARRAYS_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_POINTER_ARRAY_COUNT = 7,
    JPB_POINTER_ARRAY_VERTEX = 0,
    JPB_POINTER_ARRAY_NORMAL = 1,
    JPB_POINTER_ARRAY_UV = 2,
    JPB_POINTER_ARRAY_COLOR = 3,
    JPB_POINTER_ARRAY_INDEX = 4,
    JPB_POINTER_ARRAY_AI = 5,
    JPB_POINTER_ARRAY_ENEMY = 6
};

int addPtr(void *pointer, int type);
void *getPtr(int index, int type);

/*
 * Portable registry-only reset. The reference initArrays also resets unrelated
 * rendering counters that have not yet been reconstructed.
 */
void pointerRegistry_Reset(void);

#ifdef __cplusplus
}
#endif

#endif
