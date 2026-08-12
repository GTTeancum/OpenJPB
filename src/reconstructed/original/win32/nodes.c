/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\win32\nodes.c.
 *
 * Provenance:
 *   direct      - procedure/type/local names and geometry records from PDB.
 *   decompiled  - stream traversal, point-cache ownership, face submission,
 *                 and packet stride checked at RVAs 0x129030..0x1294CA.
 *   inferred    - descriptive names for primRendPacket's observed fields;
 *                 its two unobserved byte ranges remain explicitly reserved.
 *
 * PDB module: 0098
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\nodes.obj
 * Primary source: W:\SWJediPowerBattles\work\win32\nodes.c
 * Compiler language: c
 * Emitted procedures: 6
 *
 */

#include "jpb/globalarrays.h"
#include "jpb/camera.h"
#include "jpb/effects.h"
#include "jpb/flex.h"
#include "jpb/intersec.h"
#include "jpb/material.h"
#include "jpb/physics.h"
#include "jpb/render_nodes.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/whook.h"

#include <stdint.h>
#include <string.h>

int32_t mCurRendPacket;
primRendPacket gRendPacket[JPB_RENDER_PACKET_CAPACITY];
int32_t modelsclipped;
int32_t drawme;
int32_t screenborders[2][5];
SramModelStack *sr;

/* Exact PDB-named nodes.c module local at matched-PC RVA 0x92D938. */
static VECTOR mCameraLocation;

enum {
    RENDER_NODE_HIDDEN = 0x00000004,
    RENDER_NODE_EXPLOSION_STARTED = 0x00000020,
    RENDER_NODE_HIDE_AUTHORED = 0x00001000,
    RENDER_NODE_SCALE = 0x00400000,
    RENDER_NODE_ROTATION_ABS_DIRTY = 0x00800000,
    RENDER_NODE_EXPLODING = 0x04000000,
    RENDER_NODE_USE_ABSOLUTE_ROTATION = 0x20000000,
    RENDER_NODE_EVENT_FORCE_VISIBLE = 0x40000000
};

static void render_CopyRotationToFloat(
    const MATRIX *source, FMATRIX *destination)
{
    int row;
    int column;

    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            destination->m[row][column] = source->m[row][column];
        }
    }
}

static void render_CopyFloatRotationToMatrix(
    const FMATRIX *source, MATRIX *destination)
{
    int row;
    int column;

    memset(destination, 0, sizeof(*destination));
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            destination->m[row][column] = source->m[row][column];
        }
    }
}

static void render_MakeRotation(
    const _svector *rotation, FMATRIX *matrix, int zyx)
{
    MATRIX temporary;

    if (zyx != 0) {
        fRotMatrixZYX((_svector *)rotation, &temporary);
    } else {
        fRotMatrix((_svector *)rotation, &temporary);
    }
    render_CopyRotationToFloat(&temporary, matrix);
    matrix->t[0] = 0.0f;
    matrix->t[1] = 0.0f;
    matrix->t[2] = 0.0f;
}

static void render_ScaleRotation(FMATRIX *matrix, const VECTOR *scale)
{
    float x = (float)scale->vx * (1.0f / 4096.0f);
    float y = (float)scale->vy * (1.0f / 4096.0f);
    float z = (float)scale->vz * (1.0f / 4096.0f);
    int row;

    for (row = 0; row < 3; ++row) {
        matrix->m[row][0] *= x;
        matrix->m[row][1] *= y;
        matrix->m[row][2] *= z;
    }
}

static void render_MultiplyRotation(
    FMATRIX *left, const FMATRIX *right)
{
    MATRIX leftMatrix;
    MATRIX rightMatrix;

    render_CopyFloatRotationToMatrix(left, &leftMatrix);
    render_CopyFloatRotationToMatrix(right, &rightMatrix);
    fMulMatrix(&leftMatrix, &rightMatrix);
    render_CopyRotationToFloat(&leftMatrix, left);
}

static void render_MultiplySceneRotation(
    const MATRIX *scene,
    const FMATRIX *local,
    FMATRIX *destination)
{
    MATRIX localMatrix;
    MATRIX result;

    render_CopyFloatRotationToMatrix(local, &localMatrix);
    fMulMatrix0((MATRIX *)scene, &localMatrix, &result);
    render_CopyRotationToFloat(&result, destination);
}

static void render_TransposeRotation(
    const FMATRIX *source, FMATRIX *destination)
{
    MATRIX sourceMatrix;
    MATRIX result;

    render_CopyFloatRotationToMatrix(source, &sourceMatrix);
    fTransposeMatrix(&sourceMatrix, &result);
    render_CopyRotationToFloat(&result, destination);
    destination->t[0] = 0.0f;
    destination->t[1] = 0.0f;
    destination->t[2] = 0.0f;
}

static int render_OtagPosition(int x, int y, int z)
{
    /*
     * jon_otagpos is an exact three-byte retail compatibility stub. Its
     * nominal int return is not defined by the body and packet submission is
     * insertion-ordered, so the portable source defines the unused value.
     */
    (void)x;
    (void)y;
    (void)z;
    return 0;
}

static void render_ApplyMatrix(
    const FMATRIX *matrix,
    const FVECTOR *source,
    FVECTOR *destination)
{
    destination->vx =
        source->vy * matrix->m[0][1] +
        source->vx * matrix->m[0][0] +
        source->vz * matrix->m[0][2];
    destination->vy =
        source->vy * matrix->m[1][1] +
        source->vx * matrix->m[1][0] +
        source->vz * matrix->m[1][2];
    destination->vz =
        source->vx * matrix->m[2][0] +
        source->vy * matrix->m[2][1] +
        source->vz * matrix->m[2][2];
}

static int render_AbsoluteVertexIndex(int16_t index)
{
    return index < 0 ? -(int)index : (int)index;
}

/* 0x129030, 1029 bytes, global, 19 named locals
 * _RenderNode
 * PDB type: int (geomData*, FMATRIX*, FMATRI...
 * Source: W:\SWJediPowerBattles\work\win32\nodes.c
 */
int _RenderNode(
    geomData *pGeomData,
    FMATRIX *matrix,
    FMATRIX *light,
    FVECTOR *aPointCache)
{
    int16_t *pIndex;
    int32_t *pVertex;
    faceUV *pUV;
    CVECTOR *pColor;
    _Material *material;
    int i;
    int face;

    (void)light;
    pIndex = (int16_t *)getPtr(
        pGeomData->pIndex, JPB_POINTER_ARRAY_INDEX);
    pVertex = (int32_t *)getPtr(
        pGeomData->pVertex, JPB_POINTER_ARRAY_VERTEX);
    (void)getPtr(
        pGeomData->pNormal, JPB_POINTER_ARRAY_NORMAL);
    pUV = (faceUV *)getPtr(
        pGeomData->pUV, JPB_POINTER_ARRAY_UV);
    pColor = (CVECTOR *)getPtr(
        pGeomData->pColor, JPB_POINTER_ARRAY_COLOR);
    material = (_Material *)(uintptr_t)pGeomData->t.TextureID;

    for (i = 0; i < pGeomData->numVerts * 3; ++i) {
        FVECTOR source;
        FVECTOR *destination =
            &aPointCache[pGeomData->numShareVerts + i];
        uint32_t packed = (uint32_t)pVertex[i];

        source.vx = (float)((int32_t)(packed << 22) >> 22);
        source.vy = (float)((int32_t)(packed << 12) >> 22);
        source.vz = (float)((int32_t)(packed << 2) >> 22);
        render_ApplyMatrix(matrix, &source, destination);
        destination->vx += matrix->t[0];
        destination->vy += matrix->t[1];
        destination->vz += matrix->t[2];
    }

    for (face = 0; face < pGeomData->numFaces; ++face) {
        FVECTOR points[4];
        int vertexCount = pIndex[3] == INT16_MAX ? 3 : 4;
        int j;

        for (j = 0; j < vertexCount; ++j) {
            points[j] = aPointCache[
                render_AbsoluteVertexIndex(pIndex[j])];
        }

        _StartPoly(vertexCount, material);
        for (j = 0; j < vertexCount; ++j) {
            uint8_t r = pColor[j].r;
            uint8_t g = pColor[j].g;
            uint8_t b = pColor[j].b;
            uint32_t argb;

            if (material->colorOverride < 0) {
                if (material->colorOverride == -1000 && r < 3) {
                    r = 0x12;
                    g = 0x12;
                    b = 0x12;
                }
            } else {
                r = (uint8_t)material->colorOverride;
                g = (uint8_t)material->colorOverride;
                b = (uint8_t)material->colorOverride;
            }
            argb = UINT32_C(0xff000000) |
                   ((uint32_t)r << 16) |
                   ((uint32_t)g << 8) |
                   (uint32_t)b;
            _SetVert(
                j,
                points[j].vx,
                points[j].vy,
                points[j].vz,
                argb,
                pUV->uv[j].u,
                pUV->uv[j].v);
        }
        _NoScaleEndPoly();
        ++pUV;
        pIndex += 4;
        pColor += vertexCount;
    }
    return 1;
}

/* 0x129440, 138 bytes, global, 3 named locals
 * _RenderPackets
 * PDB type: void (primRendPacket*, int)
 * Source: W:\SWJediPowerBattles\work\win32\nodes.c
 */
void _RenderPackets(primRendPacket *pCurrentPacket, int num)
{
    FVECTOR aPointCache[3072];
    int i;

    for (i = 0; i < num; ++i) {
        _RenderNode(
            pCurrentPacket[i].geometry,
            &pCurrentPacket[i].modelMatrix,
            &pCurrentPacket[i].lightMatrix,
            aPointCache);
    }
}

/* 0x1294D0, 257 bytes, global, 5 named locals
 * render_CreateRenderPacket
 * PDB type: void (FMATRIX*)
 * Source: W:\SWJediPowerBattles\work\win32\nodes.c
 */
void render_CreateRenderPacket(FMATRIX *m3LocalMatrix)
{
    primRendPacket *pCurrentPacket;
    FVECTOR v3temp;

    if (mCurRendPacket >= JPB_RENDER_PACKET_CAPACITY) {
        return;
    }
    pCurrentPacket = &gRendPacket[mCurRendPacket++];
    render_TransposeRotation(
        m3LocalMatrix, &pCurrentPacket->lightMatrix);
    render_MultiplySceneRotation(
        &gSceneGeometryEnv.matrix,
        m3LocalMatrix,
        &pCurrentPacket->modelMatrix);
    v3temp.vx =
        (float)gSceneGeometryEnv.pos.vx + m3LocalMatrix->t[0];
    v3temp.vy =
        (float)gSceneGeometryEnv.pos.vy + m3LocalMatrix->t[1];
    v3temp.vz =
        (float)gSceneGeometryEnv.pos.vz + m3LocalMatrix->t[2];
    fApplyMatrixFV(
        &gSceneGeometryEnv.matrix,
        &v3temp,
        (FVECTOR *)(void *)pCurrentPacket->modelMatrix.t);
    pCurrentPacket->geometry = sr->node->pGeomData;
    pCurrentPacket->sceneObjectIndex = sr->sceneObjectIndex;
}

/* 0x1295E0, 882 bytes, global, 2 named locals
 * render_RenderModel
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\nodes.c
 */
void render_RenderModel(void)
{
    FVECTOR v3RootTranslationTemp;

    sr->matrix = &sr->scene->m3LocalModelMatrix;
    sr->model = (modelObject *)(void *)sr->scene->pModel;
    sr->keyFrame = sr->scene->pKeyFrameModel;
    sr->model->eventMask = 0;
    sr->model->effectMask = 0;
    sr->model->flags &= ~UINT32_C(0x20);

    render_MakeRotation(
        &sr->scene->v3WorldAngle, sr->matrix, 0);
    render_ScaleRotation(sr->matrix, &sr->model->v3Scale);
    sr->matrix->t[0] = (float)sr->scene->v3WorldPosition.vx;
    sr->matrix->t[1] = (float)sr->scene->v3WorldPosition.vy;
    sr->matrix->t[2] = (float)sr->scene->v3WorldPosition.vz;

    v3RootTranslationTemp.vx =
        (float)sr->keyFrame->v3RootTranslation.vx;
    v3RootTranslationTemp.vy =
        (float)sr->keyFrame->v3RootTranslation.vy;
    v3RootTranslationTemp.vz =
        (float)sr->keyFrame->v3RootTranslation.vz;
    render_ApplyMatrix(
        sr->matrix,
        &v3RootTranslationTemp,
        &sr->transformedTranslation);
    sr->matrix->t[0] += sr->transformedTranslation.vx;
    sr->matrix->t[1] += sr->transformedTranslation.vy;
    sr->matrix->t[2] += sr->transformedTranslation.vz;
    sr->scene->v3SnapShotPosition.vx = (int32_t)sr->matrix->t[0];
    sr->scene->v3SnapShotPosition.vy = (int32_t)sr->matrix->t[1];
    sr->scene->v3SnapShotPosition.vz = (int32_t)sr->matrix->t[2];

    sr->matrixStack[sr->matrixStackLevel] = *sr->matrix;
    sr->matrixStack[sr->matrixStackLevel + 1] =
        sr->matrixStack[sr->matrixStackLevel];
    ++sr->matrixStackLevel;
    drawme = 1;
    sr->model->flags &= ~UINT32_C(0x4);
    render_RenderNode(sr->model->pRootNode);
}

/* 0x129960, 2854 bytes, global, 12 named locals
 * render_RenderNode
 * PDB type: void (Mnode*)
 * Source: W:\SWJediPowerBattles\work\win32\nodes.c
 */
void render_RenderNode(Mnode *pNode)
{
    FVECTOR pos;
    _svector currentRotation;
    uint32_t nodeIndex;
    uint32_t flag;
    int i;

    sr->node = pNode;
    pos.vx = (float)pNode->v3Translation.vx;
    pos.vy = (float)pNode->v3Translation.vy;
    pos.vz = (float)pNode->v3Translation.vz;
    render_ApplyMatrix(
        &sr->matrixStack[sr->matrixStackLevel],
        &pos,
        &sr->transformedTranslation);

    nodeIndex = (uint32_t)pNode->id & NODE_INDEX_MASK;
    if ((((uint32_t)pNode->id & NODE_STATIC) == 0) &&
        (((uint32_t)pNode->id & NODE_VIRTUAL) == 0)) {
        currentRotation = sr->keyFrame->av3JointAngle[nodeIndex];
    } else {
        memset(&currentRotation, 0, sizeof(currentRotation));
    }

    if ((pNode->flags &
         (RENDER_NODE_USE_ABSOLUTE_ROTATION |
          RENDER_NODE_ROTATION_ABS_DIRTY)) != 0) {
        currentRotation = pNode->v3RotationAbs;
        pNode->flags &= ~RENDER_NODE_ROTATION_ABS_DIRTY;
    }

    if ((pNode->flags &
         (RENDER_NODE_EXPLODING | RENDER_NODE_HIDDEN)) ==
        RENDER_NODE_EXPLODING) {
        physicsObject *physics =
            (physicsObject *)(void *)sr->scene->pPhysics;

        sr->matrixStack[sr->matrixStackLevel].t[0] =
            (float)pNode->v3Translation2.vx;
        sr->matrixStack[sr->matrixStackLevel].t[1] =
            (float)pNode->v3Translation2.vy;
        sr->matrixStack[sr->matrixStackLevel].t[2] =
            (float)pNode->v3Translation2.vz;
        pNode->v3Translation2.vx = (int16_t)(
            pNode->v3Translation2.vx + pNode->v3Velocity2.vx);
        pNode->v3Translation2.vy = (int16_t)(
            pNode->v3Translation2.vy + pNode->v3Velocity2.vy -
            pNode->time / 16);
        pNode->v3Translation2.vz = (int16_t)(
            pNode->v3Translation2.vz + pNode->v3Velocity2.vz);
        pNode->time += flexmul12(0x40, gGlobalFrameRate);

        if ((pNode->flags & RENDER_NODE_EXPLOSION_STARTED) == 0 &&
            (pNode->time > 0x800 ||
             (physics != NULL &&
              (float)pNode->v3Translation2.vy <= physics->airGround))) {
            if (pNode->time < 0x1000) {
                int effectIndex = nodeIndex == 8 ? 71 : 83;
                EffectHeader *effect = paEffects[effectIndex];
                VECTOR soundPosition;

                if (effect != NULL) {
                    (void)sprite_AddSpriteEffectAtNode(
                        effect->aEffects,
                        (int)effect->num,
                        sr->scene->sceneRoot.objectID,
                        (int)nodeIndex);
                }
                memcpy(
                    &soundPosition,
                    sr->matrixStack[sr->matrixStackLevel].t,
                    3U * sizeof(int32_t));
                soundPosition.pad = 0;
                (void)sound_Play(
                    &soundPosition, 0, "explosm", 0);
                pNode->flags |= RENDER_NODE_HIDDEN;
                pNode->time = 0x1000;
            } else {
                pNode->flags |= RENDER_NODE_HIDDEN;
            }
        }
    } else if (nodeIndex != 0) {
        sr->matrixStack[sr->matrixStackLevel].t[0] +=
            sr->transformedTranslation.vx;
        sr->matrixStack[sr->matrixStackLevel].t[1] +=
            sr->transformedTranslation.vy;
        sr->matrixStack[sr->matrixStackLevel].t[2] +=
            sr->transformedTranslation.vz;
    }

    render_MakeRotation(&currentRotation, &sr->nodeMatrix, 1);
    if ((pNode->flags & RENDER_NODE_SCALE) != 0) {
        render_ScaleRotation(&sr->nodeMatrix, &pNode->v3Scale);
    }
    render_MultiplyRotation(
        &sr->matrixStack[sr->matrixStackLevel],
        &sr->nodeMatrix);

    pNode->v3Velocity.vx = (int16_t)(
        (int32_t)sr->matrixStack[sr->matrixStackLevel].t[0] -
        pNode->v3RotCenter.vx);
    pNode->v3Velocity.vy = (int16_t)(
        (int32_t)sr->matrixStack[sr->matrixStackLevel].t[1] -
        pNode->v3RotCenter.vy);
    pNode->v3Velocity.vz = (int16_t)(
        (int32_t)sr->matrixStack[sr->matrixStackLevel].t[2] -
        pNode->v3RotCenter.vz);
    pNode->v3RotCenter.vx =
        (int32_t)sr->matrixStack[sr->matrixStackLevel].t[0];
    pNode->v3RotCenter.vy =
        (int32_t)sr->matrixStack[sr->matrixStackLevel].t[1];
    pNode->v3RotCenter.vz =
        (int32_t)sr->matrixStack[sr->matrixStackLevel].t[2];

    if (nodeIndex == 0) {
        FVECTOR rootPosition = {
            sr->matrixStack[sr->matrixStackLevel].t[0],
            sr->matrixStack[sr->matrixStackLevel].t[1],
            sr->matrixStack[sr->matrixStackLevel].t[2]};

        if (sr->sceneObjectIndex < 2) {
            sr->model->clipBits = (int16_t)cliptofrustrum(
                clippingfrustrum,
                &rootPosition,
                0x80,
                screenborders[sr->sceneObjectIndex]);
        } else {
            if ((sr->model->flags & UINT32_C(0x8)) == 0) {
                sr->model->clipBits = (int16_t)cliptofrustrum(
                    clippingfrustrum,
                    &rootPosition,
                    sr->model->clipradius,
                    NULL);
            }
            if (sr->model->clipBits != 0) {
                sr->model->flags |= UINT32_C(0x4);
                ++modelsclipped;
            }
        }
    }

    if ((sr->model->flags & UINT32_C(0x10)) != 0) {
        drawme = 0;
    }
    flag = pNode->flags;
    if ((flag & RENDER_NODE_HIDE_AUTHORED) != 0 &&
        (flag & RENDER_NODE_HIDDEN) == 0) {
        pNode->flags = flag | RENDER_NODE_HIDDEN;
    }
    if (drawme != 0) {
        pNode->ZBufferOffset = (int16_t)(render_OtagPosition(
            (int)((float)sr->otagCameraLocation.vx -
                  sr->matrixStack[sr->matrixStackLevel].t[0]),
            (int)((float)sr->otagCameraLocation.vy -
                  sr->matrixStack[sr->matrixStackLevel].t[1]),
            (int)((float)sr->otagCameraLocation.vz -
                  sr->matrixStack[sr->matrixStackLevel].t[2])) << 2);
        sr->model->flags |= UINT32_C(0x20);
    }

    pNode->flags =
        ((uint32_t)(int32_t)(int8_t)
             sr->keyFrame->event[nodeIndex] & UINT32_C(0x0b)) |
        (pNode->flags & UINT32_C(0xffffff04));
    flag = pNode->flags;
    if ((flag & RENDER_NODE_EVENT_FORCE_VISIBLE) != 0 &&
        (flag & UINT32_C(0x1)) == 0) {
        pNode->flags = flag | UINT32_C(0x1);
    }
    flag = pNode->flags;
    if (flag != 0) {
        uint32_t nodeMask = UINT32_C(1) << (nodeIndex & 31U);

        if ((flag & UINT32_C(0x1)) != 0) {
            sr->scene->sceneRoot.flags |= UINT32_C(0x10);
        }
        if ((flag & UINT32_C(0x2)) != 0) {
            sr->model->eventMask |= nodeMask;
        }
        if ((flag & UINT32_C(0x8)) != 0) {
            sr->model->effectMask |= nodeMask;
        }
    }

    if (drawme == 0) {
        if (sr->sceneObjectIndex > 1) {
            return;
        }
    } else if ((pNode->flags & RENDER_NODE_HIDDEN) == 0 &&
               pNode->pGeomData->numFaces != 0) {
        render_CreateRenderPacket(
            &sr->matrixStack[sr->matrixStackLevel]);
    }

    pNode->flags &= ~RENDER_NODE_HIDDEN;
    for (i = 0; i < pNode->numChildNodes; ++i) {
        Mnode *child = &pNode->aChildNode[i];

        sr->matrixStack[sr->matrixStackLevel + 1] =
            sr->matrixStack[sr->matrixStackLevel];
        ++sr->matrixStackLevel;
        render_RenderNode(child);
        --sr->matrixStackLevel;
    }
}

/* 0x12A490, 572 bytes, global, 5 named locals
 * render_RenderScene
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\nodes.c
 */
void render_RenderScene(void)
{
    SramModelStack modelStack;

    memset(&modelStack, 0, sizeof(modelStack));
    sr = &modelStack;
    mCurRendPacket = 0;
    camera_gGetLocation(&mCameraLocation);
    sr->sceneMatrix = gSceneGeometryEnv.matrix;
    sr->scenePosition = gSceneGeometryEnv.pos;
    sr->sceneObjectIndex = 0;
    sr->scene = gSceneRoot.paSceneModels;

    while (sr->sceneObjectIndex < JPB_SCENE_CAPACITY) {
        physicsObject *physics =
            &maPhysicsData[sr->sceneObjectIndex];

        sr->matrixStackLevel = 0;
        if ((physics->flags & UINT32_C(0x20)) != 0) {
            physicsObject *source =
                &maPhysicsData[physics->flags & UINT32_C(0x1f)];

            physics->pos = source->pos;
            physics->angle = source->angle;
            physics->mov = source->mov;
            physics->mapinfo = source->mapinfo;
        }
        if (sr->scene->sceneRoot.objectID != -1 &&
            (sr->scene->sceneRoot.flags & UINT32_C(0x20)) == 0) {
            render_RenderModel();
        }
        ++sr->sceneObjectIndex;
        ++sr->scene;
    }
    if (mCurRendPacket != 0) {
        _RenderPackets(gRendPacket, mCurRendPacket);
    }
}
