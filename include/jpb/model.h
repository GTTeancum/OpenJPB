#ifndef JPB_MODEL_H
#define JPB_MODEL_H

#include "jpb/anim.h"
#include "jpb/collision.h"
#include "jpb/objroot.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Exact matched-PC PDB type 0x1200. Native node pointers preserve the
 * 104-byte x64 evidence layout and compact naturally for the Xbox runtime.
 */
typedef struct modelObject {
    objectRoot modelRoot;
    char sModelName[32];
    Mnode *pRootNode;
    CVECTOR ambientLight;
    VECTOR v3Scale;
    uint32_t idMask;
    uint32_t eventMask;
    uint32_t effectMask;
    uint32_t flags;
    int16_t clipradius;
    int16_t clipBits;
} modelObject;

enum {
    JPB_MODEL_REGISTRY_CAPACITY = 20,
    JPB_MODEL_NODE_CAPACITY = 32,
    JPB_MODEL_TEXTURE_TRACKER_CAPACITY = 128,
    JPB_MODEL_REGISTERED_NAME_CAPACITY = 32,
    JPB_MODEL_PACK_TRACKER_CAPACITY = 256
};

/* Exact matched-PC PDB type 0xA521. */
typedef struct modelSpace {
    modelObject Model;
    Mnode aNodes[JPB_MODEL_NODE_CAPACITY];
    int32_t curNode;
} modelSpace;

struct _Material;

/* Exact matched-PC PDB type 0xA527. */
typedef struct TextureTracker {
    uint32_t name[2];
    struct _Material *th;
} TextureTracker;

extern int32_t mModelID;
extern geomData *mpGeomArray;
extern uint8_t packFlag;
extern char maRegisteredModels
    [JPB_MODEL_REGISTRY_CAPACITY]
    [JPB_MODEL_REGISTERED_NAME_CAPACITY];
extern int32_t mNumRegisteredModels;
extern int32_t mReuseModel;
extern uint16_t packTracker[JPB_MODEL_PACK_TRACKER_CAPACITY];
extern modelSpace maTempModelSpace;
extern modelSpace *maModelSpace;
extern modelSpace xmaModelSpace[JPB_MODEL_REGISTRY_CAPACITY];
extern TextureTracker texTrack[JPB_MODEL_TEXTURE_TRACKER_CAPACITY];
extern modelObject *mObject;

int EndsWith(const char *str, const char *suffix);
/* addTexTrack writes a sentinel into tt[1], matching the retail list owner. */
void addTexTrack(
    TextureTracker *tt,
    uint32_t *name,
    struct _Material *pTextureHandle);
unsigned levTexParseFarce(unsigned char *name);
modelObject *model_GetModel(int id);
Mnode *model_GetNodes(int id, int num);
void model_InitModels(void);
void model_MakeNode(Mnode *pNew, geomData *pNode, char *modelName);
int model_RegisterModel(char *name);
modelObject *model_gInitModelRoot(
    geomData *pRoot, char *name, int id);
int jpb_ModelPrepareRegisteredGeometry(
    geomData *pRoot, const char *name);
int pack_checkPage(
    uint16_t *page, unsigned width, unsigned height);
int pack_getVRAM(
    unsigned width, unsigned height, unsigned *x, unsigned *y);
void pack_page(uint16_t *page, unsigned width, unsigned height);
void resetTexTrack(void);
void resetTexturePacker(void);

/*
 * Portable bounds for the pointer-free BMD payload consumed by the exact
 * model_gInitModelRoot/model_MakeNode hierarchy owners. The retail code
 * trusted loader relocation; the reviewed PC path validates the same record
 * indices before publishing pointers.
 */
void jpb_ModelSetGeometryBounds(void *base, size_t size);

typedef enum JPBModelPoseResult {
    JPB_MODEL_POSE_OK = 0,
    JPB_MODEL_POSE_INVALID_ARGUMENT = -1,
    JPB_MODEL_POSE_UNSUPPORTED_NODE_ID = -2
} JPBModelPoseResult;

JPBModelPoseResult jpb_ModelApplyAnimFrame(
    Mnode *root, const _animFrame *frame);
JPBModelPoseResult jpb_ModelApplyAnimFrameForScene(
    Mnode *root,
    const _animFrame *frame,
    objectRoot *scene_root);
/*
 * Portable publication of the complete render_RenderModel animation state:
 * joint poses, scene collision-hot state, and per-model event/effect masks.
 */
JPBModelPoseResult jpb_ModelPublishAnimFrame(
    modelObject *model,
    const _animFrame *frame,
    objectRoot *scene_root);

#if defined(__cplusplus)
#define JPB_MODEL_STATIC_ASSERT static_assert
#else
#define JPB_MODEL_STATIC_ASSERT _Static_assert
#endif

#if UINTPTR_MAX == UINT64_MAX
JPB_MODEL_STATIC_ASSERT(
    sizeof(modelObject) == 104,
    "modelObject must match PDB type 0x1200");
JPB_MODEL_STATIC_ASSERT(
    offsetof(modelObject, v3Scale) == 68,
    "modelObject.v3Scale x64 offset changed");
JPB_MODEL_STATIC_ASSERT(
    offsetof(modelObject, eventMask) == 88,
    "modelObject.eventMask x64 offset changed");
JPB_MODEL_STATIC_ASSERT(
    offsetof(modelObject, flags) == 96,
    "modelObject.flags x64 offset changed");
JPB_MODEL_STATIC_ASSERT(
    sizeof(modelSpace) == 4976,
    "modelSpace must match PDB type 0xA521");
JPB_MODEL_STATIC_ASSERT(
    offsetof(modelSpace, aNodes) == 104,
    "modelSpace.aNodes x64 offset changed");
JPB_MODEL_STATIC_ASSERT(
    offsetof(modelSpace, curNode) == 4968,
    "modelSpace.curNode x64 offset changed");
JPB_MODEL_STATIC_ASSERT(
    sizeof(TextureTracker) == 16,
    "TextureTracker must match PDB type 0xA527");
#elif UINTPTR_MAX != UINT32_MAX
#error Unsupported pointer width
#endif

#undef JPB_MODEL_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
