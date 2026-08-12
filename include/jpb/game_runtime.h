#ifndef JPB_GAME_RUNTIME_H
#define JPB_GAME_RUNTIME_H

#include "jpb/anim.h"
#include "jpb/bmd.h"
#include "jpb/camera.h"
#include "jpb/cad.h"
#include "jpb/combo.h"
#include "jpb/huffman.h"
#include "jpb/jpx.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/software_renderer.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

enum JPBGameRuntimeResult {
    JPB_GAME_RUNTIME_OK = 0,
    JPB_GAME_RUNTIME_INVALID_ARGUMENT = -1,
    JPB_GAME_RUNTIME_OUT_OF_MEMORY = -2,
    JPB_GAME_RUNTIME_LOAD_FAILED = -3,
    JPB_GAME_RUNTIME_RENDER_FAILED = -4
};

enum {
    JPB_GAME_RUNTIME_ENEMY_CAPACITY =
        JPB_PLAYER_CAPACITY - 2,
    JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY = 64,
    JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY = 64,
    JPB_GAME_RUNTIME_TEXT_DRAW_CAPACITY = 64,
    JPB_GAME_RUNTIME_TEXT_CAPACITY = 64,
    JPB_GAME_RUNTIME_DRAW3D_TEXT_CAPACITY = 64,
    JPB_GAME_RUNTIME_DRAW3D_TEXT_BYTES = 256,
    JPB_GAME_RUNTIME_SPRITE_DISPLAY_CAPACITY = 64,
    JPB_GAME_RUNTIME_PSX_TEXTURE_DRAW_CAPACITY = 64,
    JPB_GAME_RUNTIME_GLOW_DRAW_CAPACITY = 128
};

typedef struct JPBGameRuntimeTextureCache
    JPBGameRuntimeTextureCache;
typedef struct JPBGameRuntimeEnemyState
    JPBGameRuntimeEnemyState;

typedef struct JPBGameRuntimeScreenDraw {
    uint32_t order;
    _Material *texture;
    SCREENRECT destination;
    SCREENRECT source;
    int textureWidth;
    int textureHeight;
    CVECTOR color;
    float layerDepth;
    int hasSource;
    int isPlayerHudTile;
} JPBGameRuntimeScreenDraw;

typedef struct JPBGameRuntimeScreenPolyDraw {
    _Material *texture;
    int vertexCount;
    int noScale;
    int deferred;
    JPBScreenPolyVertex vertices[
        JPB_SCREEN_POLY_VERTEX_CAPACITY];
} JPBGameRuntimeScreenPolyDraw;

typedef struct JPBGameRuntimeGlowDraw {
    _svector start;
    _svector end;
    int32_t radius;
    uint32_t color;
} JPBGameRuntimeGlowDraw;

typedef struct JPBGameRuntimeTextDraw {
    uint32_t order;
    int tint;
    int alpha;
    int mode;
    int x;
    int y;
    float scale;
    float scaleAdjustment;
    int fontStyle;
    int clipEnabled;
    int clipLeft;
    int clipTop;
    int clipRight;
    int clipBottom;
    size_t compositePixels;
    wchar_t text[JPB_GAME_RUNTIME_TEXT_CAPACITY];
} JPBGameRuntimeTextDraw;

typedef struct JPBGameRuntimeDraw3dText {
    uint32_t order;
    float x;
    float y;
    float z;
    float scale;
    uint32_t color;
    char text[JPB_GAME_RUNTIME_DRAW3D_TEXT_BYTES];
} JPBGameRuntimeDraw3dText;

typedef struct JPBGameRuntimeSpriteDisplay {
    uint32_t order;
    int type;
    int x;
    int y;
    int width;
    int height;
    int clut;
    const _Material *material;
    int textureWidth;
    int textureHeight;
} JPBGameRuntimeSpriteDisplay;

typedef struct JPBGameRuntimePsxTextureDraw {
    uint32_t order;
    unsigned texture;
    float x;
    float y;
    float width;
    float height;
    unsigned transparency;
    int red;
    int green;
    int blue;
} JPBGameRuntimePsxTextureDraw;

/* Portable host-owned storage for the retail second world-player slot. */
typedef struct JPBGameRuntimeSecondPlayerState
    JPBGameRuntimeSecondPlayerState;

typedef struct JPBGameRuntime {
    uint8_t *meshStorage;
    uint8_t *collisionStorage;
    size_t collisionStorageSize;
    uint8_t *cadStorage;
    uint8_t *bmdStorage;
    uint8_t *comboStorage;
    uint8_t *enemyCadStorage;
    uint8_t *enemyBmdStorage;
    uint8_t *enemyAiStorage;
    size_t enemyAiStorageSize;
    WorldData *world;
    JPBHuffmanTableSet *huffmanTables;
    JPBGameRuntimeTextureCache *worldTextureCache;
    JPBGameRuntimeTextureCache *textureCache;
    JPBGameRuntimeTextureCache *enemyTextureCache;
    JPBGameRuntimeTextureCache *uiTextureCache;
    JPBGameRuntimeTextureCache *defaultTextureCache;
    JPBGameRuntimeEnemyState *enemyState;
    size_t enemyActorCount;
    size_t enemyActorPeakCount;
    size_t enemySpawnCount;
    size_t enemyLoadedClassCount;
    size_t enemyPlacedClassCount;
    size_t enemyActiveClassCount;
    size_t enemyActiveClassPeakCount;
    size_t enemyActivatedClassCount;
    size_t enemyRenderedClassCount;
    size_t powerupCount;
    size_t checkpointCount;
    size_t powerupDrawCount;
    size_t powerupCollectedCount;
    float *renderDepthBuffer;
    size_t renderDepthCapacity;
    float *glowDepthBuffer;
    size_t glowDepthCapacity;
    JPBJpxView meshView;
    JPBCadView cadView;
    JPBBmdView bmdView;
    JPBCadView enemyCadView;
    JPBBmdView enemyBmdView;
    JPBSoftwareJpxScene scene;
    const JPBSoftwareLevelMesh *levelRenderMesh;
    objectRoot actorRoot;
    sceneObject *actorScene;
    modelObject actorModel;
    physicsObject *physics;
    animObject *animation;
    playerObject *player;
    objectRoot inactivePlayerActorRoot;
    sceneObject *inactivePlayerScene;
    physicsObject *inactivePlayerPhysics;
    playerObject *inactivePlayer;
    JPBGameRuntimeSecondPlayerState *secondPlayerState;
    objectRoot enemyActorRoot;
    sceneObject *enemyScene;
    modelObject enemyModel;
    physicsObject *enemyPhysics;
    animObject *enemyAnimation;
    playerObject *enemyPlayer;
    wsl_ENEMY *enemy;
    Camera camera;
    sceneGeometryEnv environment;
    float targetX;
    float targetY;
    float targetZ;
    float orbitYaw;
    float orbitPitch;
    float orbitDistance;
    float minimumOrbitDistance;
    float maximumOrbitDistance;
    float cameraCollisionFraction;
    uint32_t cameraCollisionFrameCount;
    uint32_t authoredCameraFrameCount;
    int16_t authoredCameraDolly;
    int topView;
    int authoredMotionReady;
    int authoredFrameReady;
    int authoredPoseReady;
    int enemyAuthoredMotionReady;
    int enemyAuthoredFrameReady;
    int enemyAuthoredPoseReady;
    int collisionReady;
    uint32_t decodedFrameCount;
    uint32_t authoredTweenFrameCount;
    uint32_t enemyDecodedFrameCount;
    uint32_t enemyMainCallbackFrameCount;
    uint32_t enemyKungfuSchedulerFrameCount;
    uint32_t enemyAuthoredOpcodeFrameCount;
    uint32_t enemyOpcodeBoundaryFrameCount;
    uint16_t enemyOpcodeBoundary;
    uint32_t authoredLocomotionMotionFrameCount;
    int16_t lastAuthoredLocomotionMotion;
    uint32_t authoredDamageMotionFrameCount;
    int16_t lastAuthoredDamageMotion;
    uint32_t authoredRunningAttackFrameCount;
    int16_t lastAuthoredRunningAttackMotion;
    uint32_t passiveMotionReportFrameCount;
    uint32_t authoredHotFrameCount;
    uint32_t authoredHotNodePeak;
    uint32_t closestHotTargetNodeDistance;
    int32_t closestHotTargetCollisionRadius;
    int16_t closestHotNodeId;
    int16_t closestTargetNodeId;
    uint32_t closestHotTargetDistanceByNode[
        JPB_COLLISION_NODE_CAPACITY];
    int32_t hotTargetCollisionRadiusByNode[
        JPB_COLLISION_NODE_CAPACITY];
    int16_t closestTargetNodeByHotNode[
        JPB_COLLISION_NODE_CAPACITY];
    uint32_t combatHitCount;
    uint32_t enemyDamageProcessedCount;
    uint32_t enemyReactionMotionFrameCount;
    uint32_t enemyRecoilReactionCount;
    int16_t lastEnemyReactionMotion;
    int16_t lastEnemyDamageMotion;
    int16_t enemyAnimationMotion;
    int16_t enemyAiLevel;
    int16_t enemyInitialEnergy;
    int16_t enemyMinimumEnergy;
    float lastEnemyRecoil;
    size_t worldLoadedTextures;
    size_t worldRenderedPixels;
    size_t playerRenderedTriangles;
    size_t playerRenderedPixels;
    uint32_t playerVisibleFrameCount;
    uint32_t playerAuthoredAiAttachCount;
    uint32_t playerAuthoredAiReleaseCount;
    int16_t lastPlayerAuthoredAiEnemyId;
    int16_t lastPlayerAuthoredAiOwnerType;
    int playerAuthoredAiObserved;
    uint32_t controlPressedFrameCount[2];
    uint32_t controlHeldFrameCount[2];
    uint32_t controlReleaseEventCount[2];
    uint32_t controlObservedPressedBits[2];
    uint32_t controlObservedHeldBits[2];
    uint32_t controlObservedReleasedBits[2];
    uint32_t controlPreviousHeldBits[2];
    uint32_t controlLockToggleCount[2];
    uint8_t controlLockInitialized[2];
    uint8_t controlLockActive[2];
    int16_t lastControlMotion[2];
    uint32_t controlLocomotionFrameCount[2];
    int16_t lastControlLocomotionMotion[2];
    uint32_t controlDirectionalFrameCount[2];
    float lastControlAxisX[2];
    float lastControlAxisY[2];
    int32_t lastControlCameraAngle[2];
    int32_t lastControlDesiredFacing[2];
    int32_t lastControlFacing[2];
    size_t playerProjectileLaunchCount[2];
    int16_t lastPlayerProjectileType[2];
    uint32_t lastPlayerProjectileFlags[2];
    VECTOR lastPlayerProjectileStart[2];
    VECTOR lastPlayerProjectileTarget[2];
    size_t enemyRenderedTriangles;
    size_t enemyRenderedPixels;
    JPBGameRuntimeScreenDraw screenDraws[
        JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY];
    uint32_t drawOrder;
    size_t screenDrawCount;
    size_t screenDrawDroppedCount;
    size_t screenDrawCompositePixelCount;
    size_t screenDrawTextureAlphaModulatedPixelCount;
    size_t itemHudTextureAlphaModulatedPixelCount;
    size_t creditHudTextureAlphaModulatedPixelCount;
    size_t rescueHudTextureAlphaModulatedPixelCount;
    size_t playerHudTileDrawCount;
    size_t playerHudTileDroppedCount;
    size_t playerHudTileCompositePixelCount;
    JPBGameRuntimeTextDraw textDraws[
        JPB_GAME_RUNTIME_TEXT_DRAW_CAPACITY];
    size_t textDrawCount;
    size_t textDrawDroppedCount;
    size_t textDrawCompositePixelCount;
    size_t textTrueTypeDrawCount;
    size_t textFallbackDrawCount;
    int maximumTextPointSize;
    int maximumTextMeasuredWidth;
    int maximumTextMeasuredHeight;
    JPBGameRuntimeDraw3dText draw3dTextDraws[
        JPB_GAME_RUNTIME_DRAW3D_TEXT_CAPACITY];
    size_t draw3dTextDrawCount;
    size_t draw3dTextDroppedCount;
    JPBGameRuntimeSpriteDisplay spriteDisplayDraws[
        JPB_GAME_RUNTIME_SPRITE_DISPLAY_CAPACITY];
    size_t spriteDisplayDrawCount;
    size_t spriteDisplayDroppedCount;
    JPBGameRuntimePsxTextureDraw psxTextureDraws[
        JPB_GAME_RUNTIME_PSX_TEXTURE_DRAW_CAPACITY];
    size_t psxTextureDrawCount;
    size_t psxTextureDrawDroppedCount;
    JPBGameRuntimeGlowDraw glowDraws[
        JPB_GAME_RUNTIME_GLOW_DRAW_CAPACITY];
    size_t glowDrawCount;
    size_t glowDrawDroppedCount;
    size_t glowDrawCompositePixelCount;
    size_t cylinderDrawCount;
    JPBGameRuntimeScreenPolyDraw screenPolyDraws[
        JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY];
    size_t screenPolyDrawCount;
    size_t screenPolyDroppedCount;
    size_t screenPolyCompositePixelCount;
    size_t waterPolyDrawCount;
    size_t waterPolyCompositePixelCount;
    int drawTextureHookReady;
    int screenPolyHookReady;
    int barHookReady;
    int playerTileHookReady;
    int textHookReady;
    int draw3dTextHookReady;
    int spriteDisplayHookReady;
    int glowHookReady;
    int cylinderHookReady;
    int powerupDrawHookReady;
    int playerProcessObserverReady;
    int bulletLaunchObserverReady;
    int sceneMiddleRenderHooksReady;
} JPBGameRuntime;

int jpb_GameRuntimeInit(
    JPBGameRuntime *runtime, const char *jpx_path);
int jpb_GameRuntimeInitWithCad(
    JPBGameRuntime *runtime,
    const char *jpx_path,
    const char *cad_path);
int jpb_GameRuntimeInitWithAssets(
    JPBGameRuntime *runtime,
    const char *jpx_path,
    const char *cad_path,
    const char *bmd_path);
int jpb_GameRuntimeInitWithPlayerAssets(
    JPBGameRuntime *runtime,
    const char *jpx_path,
    const char *cad_path,
    const char *bmd_path,
    int player_model_id);
const char *jpb_GameRuntimeLastFailureStage(void);
int jpb_GameRuntimeAddEnemyAssets(
    JPBGameRuntime *runtime,
    const char *cad_path,
    const char *bmd_path);
int jpb_GameRuntimeEnemyClassModelId(
    const JPBGameRuntime *runtime,
    int actor_num);
int jpb_GameRuntimeEnemyClassWasActive(
    const JPBGameRuntime *runtime,
    int actor_num);
int jpb_GameRuntimeAddPlayerComboData(
    JPBGameRuntime *runtime,
    const char *cmb_path);
int jpb_GameRuntimeActivateSecondPlayer(
    JPBGameRuntime *runtime,
    const char *cad_path,
    const char *bmd_path,
    int player_model_id);
int jpb_GameRuntimeAddSecondPlayerComboData(
    JPBGameRuntime *runtime,
    const char *cmb_path);
int jpb_GameRuntimeSecondPlayerReady(
    const JPBGameRuntime *runtime);
size_t jpb_GameRuntimeSecondPlayerRenderedTriangles(
    const JPBGameRuntime *runtime);
size_t jpb_GameRuntimeSecondPlayerRenderedPixels(
    const JPBGameRuntime *runtime);
void jpb_GameRuntimeSetLevelRenderMesh(
    JPBGameRuntime *runtime,
    const JPBSoftwareLevelMesh *mesh);
void jpb_GameRuntimeShutdown(JPBGameRuntime *runtime);
int jpb_GameRuntimeFrame(
    JPBGameRuntime *runtime,
    float elapsed_seconds,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareRenderStats *stats);
int jpb_GameRuntimeTitleFrame(
    JPBGameRuntime *runtime,
    JPBSoftwareFramebuffer *framebuffer);

#ifdef __cplusplus
}
#endif

#endif
