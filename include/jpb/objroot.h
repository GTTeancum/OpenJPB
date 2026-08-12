#ifndef JPB_OBJROOT_H
#define JPB_OBJROOT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct objectRoot {
    struct objectRoot *pParent;
    uint32_t flags;
    int32_t objectID;
    char objectName[8];
} objectRoot;

typedef struct sceneObject sceneObject;

int obj_gCheckObjectFlag(objectRoot *object, int type, uint32_t flag);
objectRoot *obj_gClearObject(objectRoot *object);
void obj_gClrObjectFlag(objectRoot *child, int type, uint32_t mask);
objectRoot *obj_gSetChildObject(
    sceneObject *parent, objectRoot *child, int type);
void obj_gSetObjectFlag(objectRoot *child, int type, uint32_t mask);

#if defined(__cplusplus)
#define JPB_OBJROOT_STATIC_ASSERT static_assert
#else
#define JPB_OBJROOT_STATIC_ASSERT _Static_assert
#endif

#if UINTPTR_MAX == UINT64_MAX
JPB_OBJROOT_STATIC_ASSERT(
    sizeof(objectRoot) == 24,
    "objectRoot size must match PDB type 0x1161");
#elif UINTPTR_MAX == UINT32_MAX
JPB_OBJROOT_STATIC_ASSERT(
    sizeof(objectRoot) == 20,
    "32-bit objectRoot runtime size changed");
#else
#error Unsupported pointer width
#endif

#undef JPB_OBJROOT_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
