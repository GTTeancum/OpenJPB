#include "jpb/globalarrays.h"
#include "jpb/material.h"
#include "jpb/physics.h"
#include "jpb/render_nodes.h"
#include "jpb/scene.h"
#include "jpb/whook.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                               \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct RenderTrace {
    int calls;
    _Material *material;
    int vertexCount;
    int noScale;
    JPBScreenPolyVertex vertices[4];
} RenderTrace;

static uint32_t pack_vertex(int x, int y, int z)
{
    return ((uint32_t)x & UINT32_C(0x3ff)) |
           (((uint32_t)y & UINT32_C(0x3ff)) << 10) |
           (((uint32_t)z & UINT32_C(0x3ff)) << 20);
}

static void capture_polygon(
    void *user_data,
    _Material *material,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale)
{
    RenderTrace *trace = (RenderTrace *)user_data;

    ++trace->calls;
    trace->material = material;
    trace->vertexCount = vertex_count;
    trace->noScale = no_scale;
    memcpy(
        trace->vertices,
        vertices,
        (size_t)vertex_count * sizeof(vertices[0]));
}

static void set_identity(FMATRIX *matrix, float x, float y, float z)
{
    memset(matrix, 0, sizeof(*matrix));
    matrix->m[0][0] = 1.0f;
    matrix->m[1][1] = 1.0f;
    matrix->m[2][2] = 1.0f;
    matrix->t[0] = x;
    matrix->t[1] = y;
    matrix->t[2] = z;
}

int main(void)
{
    int32_t packedVertices[3] = {
        (int32_t)pack_vertex(1, 2, 3),
        (int32_t)pack_vertex(-4, 5, 6),
        (int32_t)pack_vertex(7, -8, 9)};
    uint32_t packedNormals[3] = {0};
    int16_t indices[4] = {0, -1, 2, INT16_MAX};
    faceUV uvs = {{{0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}}};
    CVECTOR colors[3] = {
        {1, 2, 3, 0}, {0x40, 0x50, 0x60, 0}, {7, 8, 9, 0}};
    _Material material = {0};
    geomData geometry = {0};
    geomData sharedGeometry;
    primRendPacket packets[2] = {0};
    FMATRIX matrix;
    FVECTOR pointCache[16] = {0};
    RenderTrace trace = {0};
    sceneObject scenes[JPB_SCENE_CAPACITY];
    modelObject model;
    Mnode hierarchy[2];
    _animFrame frame;
    int sceneIndex;

    pointerRegistry_Reset();
    material.colorOverride = -1;
    geometry.numFaces = 1;
    geometry.numVerts = 1;
    geometry.pVertex = addPtr(
        packedVertices, JPB_POINTER_ARRAY_VERTEX);
    geometry.pNormal = addPtr(
        packedNormals, JPB_POINTER_ARRAY_NORMAL);
    geometry.pUV = addPtr(&uvs, JPB_POINTER_ARRAY_UV);
    geometry.pColor = addPtr(colors, JPB_POINTER_ARRAY_COLOR);
    geometry.pIndex = addPtr(indices, JPB_POINTER_ARRAY_INDEX);
    geometry.t.TextureID = (uint64_t)(uintptr_t)&material;
    set_identity(&matrix, 10.0f, 20.0f, 30.0f);
    jpb_WHookSetScreenPolyHook(capture_polygon, &trace);

    CHECK(_RenderNode(
              &geometry, &matrix, &matrix, pointCache) == 1);
    CHECK(trace.calls == 1);
    CHECK(trace.material == &material);
    CHECK(trace.vertexCount == 3);
    CHECK(trace.noScale == 1);
    CHECK(trace.vertices[0].x == 11.0f);
    CHECK(trace.vertices[0].y == 22.0f);
    CHECK(trace.vertices[0].z == 33.0f);
    CHECK(trace.vertices[1].x == 6.0f);
    CHECK(trace.vertices[1].y == 25.0f);
    CHECK(trace.vertices[1].z == 36.0f);
    CHECK(trace.vertices[2].x == 17.0f);
    CHECK(trace.vertices[2].y == 12.0f);
    CHECK(trace.vertices[2].z == 39.0f);
    CHECK(trace.vertices[0].argb == UINT32_C(0xff010203));
    CHECK(trace.vertices[1].argb == UINT32_C(0xff405060));
    CHECK(trace.vertices[2].tu == 0.5f);
    CHECK(trace.vertices[2].tv == 1.0f);

    material.colorOverride = -1000;
    trace.calls = 0;
    CHECK(_RenderNode(
              &geometry, &matrix, &matrix, pointCache) == 1);
    CHECK(trace.calls == 1);
    CHECK(trace.vertices[0].argb == UINT32_C(0xff121212));
    CHECK(trace.vertices[1].argb == UINT32_C(0xff405060));

    material.colorOverride = 0x80;
    trace.calls = 0;
    CHECK(_RenderNode(
              &geometry, &matrix, &matrix, pointCache) == 1);
    CHECK(trace.vertices[0].argb == UINT32_C(0xff808080));
    CHECK(trace.vertices[2].argb == UINT32_C(0xff808080));

    material.colorOverride = -1;
    packets[0].geometry = &geometry;
    geometry.numFaces = 0;
    set_identity(&packets[0].modelMatrix, 100.0f, 200.0f, 300.0f);
    sharedGeometry = geometry;
    sharedGeometry.numFaces = 1;
    sharedGeometry.numVerts = 0;
    sharedGeometry.numShareVerts = 3;
    packets[1].geometry = &sharedGeometry;
    set_identity(&packets[1].modelMatrix, 0.0f, 0.0f, 0.0f);
    trace.calls = 0;
    _RenderPackets(packets, 2);
    CHECK(trace.calls == 1);
    CHECK(trace.vertices[0].x == 101.0f);
    CHECK(trace.vertices[0].y == 202.0f);
    CHECK(trace.vertices[0].z == 303.0f);
    CHECK(trace.vertices[1].x == 96.0f);
    CHECK(trace.vertices[2].z == 309.0f);

    memset(scenes, 0, sizeof(scenes));
    memset(&model, 0, sizeof(model));
    memset(hierarchy, 0, sizeof(hierarchy));
    memset(&frame, 0, sizeof(frame));
    memset(maPhysicsData, 0, sizeof(maPhysicsData));
    memset(&gSceneGeometryEnv, 0, sizeof(gSceneGeometryEnv));
    memset(clippingfrustrum, 0, sizeof(clippingfrustrum));
    for (sceneIndex = 0;
         sceneIndex < JPB_SCENE_CAPACITY;
         ++sceneIndex) {
        scenes[sceneIndex].sceneRoot.objectID = -1;
    }
    for (sceneIndex = 0; sceneIndex < 5; ++sceneIndex) {
        clippingfrustrum[sceneIndex].vw = -10000.0f;
    }
    gSceneGeometryEnv.matrix.m[0][0] = 1.0f;
    gSceneGeometryEnv.matrix.m[1][1] = 1.0f;
    gSceneGeometryEnv.matrix.m[2][2] = 1.0f;
    gSceneGeometryEnv.pos.vx = -10;
    gSceneGeometryEnv.pos.vy = -20;
    gSceneGeometryEnv.pos.vz = -30;
    gSceneRoot.paSceneModels = scenes;

    geometry.numFaces = 1;
    geometry.numVerts = 1;
    geometry.numShareVerts = 0;
    material.colorOverride = -1;
    hierarchy[0].id = 0;
    hierarchy[0].pGeomData = &geometry;
    hierarchy[0].numChildNodes = 1;
    hierarchy[0].aChildNode = &hierarchy[1];
    hierarchy[1].id = 1;
    hierarchy[1].pGeomData = &geometry;
    hierarchy[1].pParent = &hierarchy[0];
    hierarchy[1].v3Translation.vx = 10;
    hierarchy[1].flags = UINT32_C(0x00400000);
    hierarchy[1].v3Scale.vx = 8192;
    hierarchy[1].v3Scale.vy = 4096;
    hierarchy[1].v3Scale.vz = 4096;
    model.pRootNode = &hierarchy[0];
    model.v3Scale.vx = 4096;
    model.v3Scale.vy = 4096;
    model.v3Scale.vz = 4096;
    model.clipradius = 128;
    model.modelRoot.pParent = &scenes[0].sceneRoot;
    frame.v3RootTranslation.vx = 1;
    frame.v3RootTranslation.vy = 2;
    frame.v3RootTranslation.vz = 3;
    frame.event[0] = 2;
    frame.event[1] = 8;
    scenes[0].sceneRoot.objectID = 0;
    scenes[0].pModel = &model.modelRoot;
    scenes[0].pPhysics = &maPhysicsData[0].physicsRoot;
    scenes[0].pKeyFrameModel = &frame;
    scenes[0].v3WorldPosition.vx = 100;
    scenes[0].v3WorldPosition.vy = 200;
    scenes[0].v3WorldPosition.vz = 300;

    maPhysicsData[3].pos.vx = 31.0f;
    maPhysicsData[3].angle.vy = 32;
    maPhysicsData[3].mov.vz = 33.0f;
    maPhysicsData[3].mapinfo.cube =
        (int32_t *)(uintptr_t)UINT64_C(0x1234);
    maPhysicsData[3].mapinfo.entry =
        (int32_t *)(uintptr_t)UINT64_C(0x5678);
    maPhysicsData[3].mapinfo.poly =
        (int32_t *)(uintptr_t)UINT64_C(0x9abc);
    maPhysicsData[5].flags = UINT32_C(0x20) | 3U;

    trace.calls = 0;
    render_RenderScene();
    CHECK(mCurRendPacket == 2);
    CHECK(trace.calls == 2);
    CHECK(model.eventMask == UINT32_C(1));
    CHECK(model.effectMask == UINT32_C(2));
    CHECK(scenes[0].v3SnapShotPosition.vx == 101);
    CHECK(scenes[0].v3SnapShotPosition.vy == 202);
    CHECK(scenes[0].v3SnapShotPosition.vz == 303);
    CHECK(hierarchy[0].v3RotCenter.vx == 101);
    CHECK(hierarchy[1].v3RotCenter.vx == 111);
    CHECK(gRendPacket[0].sceneObjectIndex == 0);
    CHECK(gRendPacket[0].geometry == &geometry);
    CHECK(gRendPacket[0].modelMatrix.t[0] == 91.0f);
    CHECK(gRendPacket[1].modelMatrix.t[0] == 101.0f);
    CHECK(gRendPacket[1].modelMatrix.m[0][0] == 2.0f);
    CHECK(trace.vertices[0].x == 103.0f);
    CHECK(trace.vertices[0].y == 184.0f);
    CHECK(trace.vertices[0].z == 276.0f);
    CHECK(maPhysicsData[5].pos.vx == 31.0f);
    CHECK(maPhysicsData[5].angle.vy == 32);
    CHECK(maPhysicsData[5].mov.vz == 33.0f);
    CHECK(maPhysicsData[5].mapinfo.cube ==
          maPhysicsData[3].mapinfo.cube);
    CHECK(maPhysicsData[5].mapinfo.entry ==
          maPhysicsData[3].mapinfo.entry);
    CHECK(maPhysicsData[5].mapinfo.poly ==
          maPhysicsData[3].mapinfo.poly);

    mCurRendPacket = JPB_RENDER_PACKET_CAPACITY;
    render_CreateRenderPacket(&matrix);
    CHECK(mCurRendPacket == JPB_RENDER_PACKET_CAPACITY);

    jpb_WHookSetScreenPolyHook(NULL, NULL);
    pointerRegistry_Reset();
    puts("render node tests passed");
    return 0;
}
