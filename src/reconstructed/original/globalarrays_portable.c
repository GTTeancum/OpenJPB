/*
 * PARTIAL REVIEWED RECONSTRUCTION of the pointer registry in
 * W:\SWJediPowerBattles\work\globalarrays.cpp.
 *
 * The reference combines std::set uniqueness checks with insertion-ordered
 * std::vector storage. A linear C vector preserves the observable indices and
 * lookup behavior while avoiding the desktop C++ standard-library dependency.
 *
 * Provenance:
 *   direct      - addPtr/getPtr names, signatures, and seven registries.
 *   decompiled  - category dispatch, insertion ordering, and bounds behavior.
 *   substituted - linear uniqueness lookup and C heap growth.
 */

#include "jpb/globalarrays.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct PointerRegistry {
    void **values;
    size_t size;
    size_t capacity;
} PointerRegistry;

static PointerRegistry pointer_registries[JPB_POINTER_ARRAY_COUNT];

/* Reference RVA 0xADC40, 1,660 bytes. */
int addPtr(void *pointer, int type)
{
    PointerRegistry *registry;
    size_t index;

    if (type < 0 || type >= JPB_POINTER_ARRAY_COUNT) {
        return INT_MAX;
    }
    registry = &pointer_registries[type];
    for (index = 0; index < registry->size; ++index) {
        if (registry->values[index] == pointer) {
            return (int)index;
        }
    }
    if (registry->size == registry->capacity) {
        size_t new_capacity =
            registry->capacity == 0 ? 16 : registry->capacity * 2;
        void **new_values;

        if (new_capacity > (size_t)INT_MAX) {
            return INT_MAX;
        }
        new_values = (void **)realloc(
            registry->values, new_capacity * sizeof(*new_values));
        if (new_values == NULL) {
            return INT_MAX;
        }
        registry->values = new_values;
        registry->capacity = new_capacity;
    }
    registry->values[registry->size] = pointer;
    return (int)registry->size++;
}

/* Reference RVA 0xAE2C0, 324 bytes. */
void *getPtr(int index, int type)
{
    const PointerRegistry *registry;

    if (index < 0 || type < 0 || type >= JPB_POINTER_ARRAY_COUNT) {
        return NULL;
    }
    registry = &pointer_registries[type];
    if ((size_t)index >= registry->size) {
        return NULL;
    }
    return registry->values[index];
}

void pointerRegistry_Reset(void)
{
    int type;

    for (type = 0; type < JPB_POINTER_ARRAY_COUNT; ++type) {
        free(pointer_registries[type].values);
        pointer_registries[type].values = NULL;
        pointer_registries[type].size = 0;
        pointer_registries[type].capacity = 0;
    }
}
