#ifndef JPB_SCENE_H
#define JPB_SCENE_H

#include "jpb/fmath.h"
#include "jpb/objroot.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Direct PDB type 0x125B from the matched x64 build. All members are
 * fixed-width math records, preserving the 176-byte layout on PC and Xbox.
 */
typedef struct sceneGeometryEnv {
    _svector angle;
    _svector pos;
    MATRIX matrix;
    _svector angleDest;
    _svector posDest;
    MATRIX matrixDest;
    MATRIX matrixRaw;
} sceneGeometryEnv;

enum { JPB_SCENE_CAPACITY = 20 };

typedef struct _animFrame _animFrame;
typedef struct playerObject playerObject;
typedef struct Camera Camera;
typedef struct geomData geomData;
typedef struct physicsObject physicsObject;

typedef void (*JPBSceneMiddleRenderStageHook)(
    void *user_data, MATRIX *matrix);
typedef void (*JPBSceneLevelOwnerHook)(
    void *user_data,
    int level,
    int argument0,
    int argument1,
    int argument2);

/*
 * Renderer-neutral stage seams inside the exact scene_middleRender owner.
 * They publish completed original-engine stages to the portable PC renderer
 * without moving gameplay scheduling back into the host.
 */
typedef struct JPBSceneMiddleRenderHooks {
    JPBSceneMiddleRenderStageHook afterAnimations;
    JPBSceneMiddleRenderStageHook afterWorld;
    JPBSceneMiddleRenderStageHook renderModels;
    JPBSceneMiddleRenderStageHook beforePlayerProcess;
    JPBSceneLevelOwnerHook levelOwner;
} JPBSceneMiddleRenderHooks;

/*
 * Exact matched-PC PDB type 0x11A8. Its component pointers refer to the
 * objectRoot prefixes of the corresponding subsystem records.
 */
struct sceneObject {
    objectRoot sceneRoot;
    objectRoot *pScene;
    objectRoot *pModel;
    objectRoot *pPhysics;
    objectRoot *pAnim;
    objectRoot *pPlayer;
    _svector v3WorldAngle;
    _svector v3WorldPosition;
    VECTOR v3SnapShotPosition;
    _animFrame *pKeyFrameModel;
    FMATRIX m3LocalModelMatrix;
};

/* Direct PDB type 0x122D, including its four-byte trailing alignment. */
typedef struct sceneRoot {
    sceneObject *paSceneModels;
    sceneGeometryEnv GeometryEnv;
    Camera *pCamera;
    int32_t camType;
} sceneRoot;

/*
 * The reference object at RVA 0x547B58 has no linked symbol. This descriptive
 * reconstruction name is kept separate from the two exact PDB global names.
 */
extern sceneRoot gSceneRoot;
#define gSceneGeometryEnv (gSceneRoot.GeometryEnv)
extern MATRIX CameraMatrix;
extern int32_t gSCENE_READY;
extern int32_t gSTROBE_MODE;
extern int32_t gCurrentSceneObject;
extern sceneObject maSceneData[JPB_SCENE_CAPACITY];
extern VECTOR v3Translate;
extern MATRIX gGTEMATRIX;
extern int32_t timesincetank[2];
extern int32_t jumpheld[2];
extern int32_t playertankindex;
extern playerObject *tankdrivers[2];
extern int32_t playeronscreen[2];
extern int32_t globaltimer;
extern int32_t screenworldpos;
extern CVECTOR mStrobe;

void hurtplayer(playerObject *player, int mod);
void playerOffScreenArrow(
    physicsObject *p0, unsigned color);
void scene_AspectCorrectMatrix(MATRIX *matrix, VECTOR *position);
void scene_DimScreen(void);
MATRIX *scene_GetRawSceneMatrix(void);
MATRIX *scene_GetSceneMatrix(void);
_svector *scene_GetViewPos(void);
MATRIX *scene_UpdateWorld2ScreenMatrix(sceneGeometryEnv *environment);
sceneObject *scene_gGetNewSceneObject(int ID);
sceneObject *scene_gCreateObject(
    char *name,
    geomData *geometry,
    int id);
void scene_gGetSceneModelMatrix(
    int id,
    _svector *angle,
    VECTOR *position,
    VECTOR *snapshot_position);
void scene_gGetSceneModelMatrixFV(
    int id,
    _svector *pv3Angle,
    FVECTOR *pv3Position,
    FVECTOR *pv3SnapShotPosition);
void scene_gGetSnapShotPosition(int id, VECTOR *position);
void scene_gInitRoot(void);
void scene_gInitScenes(int start);
void scene_gProject2Screen(_svector *position, int *screen);
void scene_gSetSceneModelMatrix(
    int id,
    _svector *angle,
    VECTOR *position);
void scene_gSetSceneModelMatrixFV(
    int index, VECTOR *angle, FVECTOR *position);
void scene_gSetSceneModelMatrixLV(
    int id,
    VECTOR *angle,
    VECTOR *position);
void scene_gSetSceneModelKeyFrame(
    int index, _animFrame *key_frame);
void scene_gSetStrobe(CVECTOR *color);
void scene_gSetWorldPosition(int id, VECTOR *position);
void jpb_SceneSetMiddleRenderHooks(
    const JPBSceneMiddleRenderHooks *hooks,
    void *user_data);
void scene_middleRender(MATRIX *matrix);
void scene_postRender(void);
void scene_preRender(MATRIX **matrix);
void jpb_SceneInitPool(int start);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_SCENE_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_SCENE_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_SCENE_STATIC_ASSERT(
    sizeof(sceneGeometryEnv) == 176, "sceneGeometryEnv layout changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneGeometryEnv, angle) == 0,
    "sceneGeometryEnv.angle layout changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneGeometryEnv, pos) == 8,
    "sceneGeometryEnv.pos layout changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneGeometryEnv, matrix) == 16,
    "sceneGeometryEnv.matrix layout changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneGeometryEnv, angleDest) == 64,
    "sceneGeometryEnv.angleDest layout changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneGeometryEnv, posDest) == 72,
    "sceneGeometryEnv.posDest layout changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneGeometryEnv, matrixDest) == 80,
    "sceneGeometryEnv.matrixDest layout changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneGeometryEnv, matrixRaw) == 128,
    "sceneGeometryEnv.matrixRaw layout changed");
#if UINTPTR_MAX == UINT64_MAX
JPB_SCENE_STATIC_ASSERT(
    sizeof(sceneRoot) == 200,
    "sceneRoot must match PDB type 0x122D");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneRoot, GeometryEnv) == 8,
    "sceneRoot.GeometryEnv x64 offset changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneRoot, pCamera) == 184,
    "sceneRoot.pCamera x64 offset changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneRoot, camType) == 192,
    "sceneRoot.camType x64 offset changed");
#endif
#if UINTPTR_MAX == UINT64_MAX
JPB_SCENE_STATIC_ASSERT(
    sizeof(sceneObject) == 152,
    "sceneObject must match PDB type 0x11A8");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneObject, pPhysics) == 40,
    "sceneObject.pPhysics x64 offset changed");
JPB_SCENE_STATIC_ASSERT(
    offsetof(sceneObject, v3WorldPosition) == 72,
    "sceneObject.v3WorldPosition x64 offset changed");
#endif

#undef JPB_SCENE_STATIC_ASSERT

#endif
