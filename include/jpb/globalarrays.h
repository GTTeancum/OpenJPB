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

extern int vertexPointerCount;
extern int normalPointerCount;
extern int uvPointerCount;
extern int colorPointerCount;
extern int indexPointerCount;
extern int aiPArrayCount;
extern int wslEnemyPointerCount;

int addPtr(void *pointer, int type);
void *getPtr(int index, int type);
void initArrays(void);
void pointerRegistry_Reset(void);

#ifdef __cplusplus
}
#endif

#endif
