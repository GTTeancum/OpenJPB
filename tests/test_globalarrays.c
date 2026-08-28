#include "jpb/globalarrays.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>

int main(void)
{
    int values[JPB_POINTER_ARRAY_COUNT][3] = {{0}};
    int type;

    initArrays();
    for (type = 0; type < JPB_POINTER_ARRAY_COUNT; ++type)
    {
        assert(addPtr(&values[type][0], type) == 0);
        assert(addPtr(&values[type][1], type) == 1);
        assert(addPtr(&values[type][0], type) == 0);
        assert(addPtr(NULL, type) == 2);
        assert(addPtr(NULL, type) == 2);
        assert(getPtr(0, type) == &values[type][0]);
        assert(getPtr(1, type) == &values[type][1]);
        assert(getPtr(2, type) == NULL);
        assert(getPtr(3, type) == NULL);
        assert(getPtr(-1, type) == NULL);
    }

    assert(addPtr(&values[0][0], -1) == INT_MAX);
    assert(addPtr(&values[0][0], JPB_POINTER_ARRAY_COUNT) == INT_MAX);
    assert(getPtr(0, -1) == NULL);
    assert(getPtr(0, JPB_POINTER_ARRAY_COUNT) == NULL);

    vertexPointerCount = 1;
    normalPointerCount = 2;
    uvPointerCount = 3;
    colorPointerCount = 4;
    indexPointerCount = 5;
    aiPArrayCount = 6;
    wslEnemyPointerCount = 7;
    initArrays();

    assert(vertexPointerCount == 0);
    assert(normalPointerCount == 0);
    assert(uvPointerCount == 0);
    assert(colorPointerCount == 0);
    assert(indexPointerCount == 0);
    assert(aiPArrayCount == 0);
    assert(wslEnemyPointerCount == 0);
    for (type = 0; type < JPB_POINTER_ARRAY_COUNT; ++type)
    {
        assert(getPtr(0, type) == NULL);
        assert(addPtr(&values[type][2], type) == 0);
    }

    pointerRegistry_Reset();
    return 0;
}
