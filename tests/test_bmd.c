#include "jpb/bmd.h"
#include "jpb/material.h"
#include "jpb/software_renderer.h"

#include <float.h>
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

static void write_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static void make_archive(uint8_t *file_data, size_t file_size)
{
    geomData *records;

    memset(file_data, 0, file_size);
    write_u32(file_data, (uint32_t)(file_size - 4));
    records = (geomData *)(file_data + 4);
    memcpy(records[1].name, "pelvis", sizeof("pelvis"));
    records[1].id = 1;
    records[1].trans.vx = 10;
    records[1].trans.vy = 20;
    records[1].trans.vz = 30;
    records[1].numChildren = 2;
    records[1].aChildren[0] = 2;
    records[1].aChildren[1] = 4;
    memcpy(records[2].name, "torso", sizeof("torso"));
    records[2].id = 2;
    records[2].numChildren = 1;
    records[2].aChildren[0] = 3;
    memcpy(records[3].name, "head", sizeof("head"));
    records[3].id = 3;
    memcpy(records[4].name, "leg", sizeof("leg"));
    records[4].id = NODE_STATIC | 4;
}

static uint32_t test_material_flags;
static int32_t test_color_override = -1;

static int resolve_white_texture(
    void *user_data,
    const char *texture_name,
    JPBSoftwareTexture *texture)
{
    static const uint32_t white = UINT32_C(0xffffffff);

    (void)user_data;
    if (texture_name == NULL || texture == NULL) {
        return 0;
    }
    texture->pixels = &white;
    texture->width = 1;
    texture->height = 1;
    texture->stridePixels = 1;
    texture->materialFlags = test_material_flags;
    texture->samplerType = TEXTURESAMPLER_LINEARCLAMP;
    texture->colorOverride = test_color_override;
    return 1;
}

static int test_inspect_and_build(void)
{
    uint8_t archive[4 + 5 * sizeof(geomData)];
    JPBBmdView view;
    modelObject model;
    Mnode nodes[4];

    make_archive(archive, sizeof(archive));
    CHECK(jpb_BmdInspect(
              archive, sizeof(archive), &view) == JPB_BMD_OK);
    CHECK(view.node_count == 4);
    CHECK(strcmp(view.root->name, "pelvis") == 0);
    memset(&model, 0, sizeof(model));
    CHECK(jpb_BmdBuildModel(
              &view, &model, nodes, 4, -1) == JPB_BMD_OK);
    CHECK(model.pRootNode == &nodes[0]);
    CHECK(model.pRootNode->v3Translation.vx == 10);
    CHECK(model.pRootNode->v3Translation.vy == 20);
    CHECK(model.pRootNode->v3Translation.vz == 30);
    CHECK(model.pRootNode->aChildNode == &nodes[1]);
    CHECK(model.pRootNode->aChildNode[0].pParent ==
          model.pRootNode);
    CHECK(model.pRootNode->aChildNode[1].id ==
          (modelNodeId)(NODE_STATIC | 4));
    CHECK(model.pRootNode->aChildNode[0].aChildNode ==
          &nodes[3]);
    CHECK(model.pRootNode->aChildNode[0].aChildNode[0].id == 3);
    CHECK(nodes[0].pGeomData == view.root);
    CHECK((model.idMask & (1u << 1)) != 0);
    CHECK((model.idMask & (1u << 4)) != 0);
    return 0;
}

static int test_invalid_archives(void)
{
    uint8_t archive[4 + 5 * sizeof(geomData)];
    JPBBmdView view;
    geomData *records;
    modelObject model;
    Mnode nodes[4];

    make_archive(archive, sizeof(archive));
    write_u32(archive, 7);
    CHECK(jpb_BmdInspect(
              archive, sizeof(archive), &view) ==
          JPB_BMD_INVALID_SIZE);
    make_archive(archive, sizeof(archive));
    records = (geomData *)(archive + 4);
    records[2].aChildren[0] = 1;
    CHECK(jpb_BmdInspect(
              archive, sizeof(archive), &view) ==
          JPB_BMD_INVALID_LAYOUT);
    make_archive(archive, sizeof(archive));
    CHECK(jpb_BmdInspect(
              archive, sizeof(archive), &view) == JPB_BMD_OK);
    memset(&model, 0, sizeof(model));
    CHECK(jpb_BmdBuildModel(
              &view, &model, nodes, 3, -1) ==
          JPB_BMD_NODE_STORAGE_TOO_SMALL);
    return 0;
}

static int test_geometry_view(void)
{
    enum {
        RECORD_BYTES = 5 * sizeof(geomData),
        VERTEX_OFFSET = RECORD_BYTES,
        FACE_OFFSET = VERTEX_OFFSET + 3 * sizeof(uint32_t),
        UV_OFFSET = FACE_OFFSET + sizeof(JPBBmdFaceRecord),
        NORMAL_OFFSET = UV_OFFSET + sizeof(faceUV),
        COLOR_OFFSET = NORMAL_OFFSET + 3 * sizeof(uint32_t),
        PAYLOAD_BYTES = COLOR_OFFSET + 3 * sizeof(CVECTOR)
    };
    uint8_t archive[4 + PAYLOAD_BYTES];
    geomData *records;
    uint32_t *vertices;
    JPBBmdFaceRecord *face;
    JPBBmdPackedFaceRecord *packed_face;
    faceUV *uv;
    CVECTOR *colors;
    JPBBmdView view;
    JPBBmdGeometryView geometry;
    modelObject model;
    Mnode node;
    JPBJpxView world_view;
    JPBSoftwareJpxScene world_scene;
    JPBSoftwareFramebuffer framebuffer;
    JPBSoftwareDepthBuffer depth_buffer;
    JPBSoftwareRenderStats render_stats;
    MATRIX view_matrix;
    _animFrame key_frame;
    FVECTOR world_position = {1.0f, 2.0f, 3.0f};
    FVECTOR decoded;
    uint32_t pixels[64 * 64];
    float depth_values[64 * 64];

    make_archive(archive, sizeof(archive));
    records = (geomData *)(archive + 4);
    records[1].id = 0;
    memset(&records[1].trans, 0, sizeof(records[1].trans));
    records[1].numChildren = 0;
    records[1].numVerts = 1;
    records[1].numFaces = 1;
    records[1].pVertex = VERTEX_OFFSET;
    records[1].pIndex = FACE_OFFSET;
    records[1].pUV = UV_OFFSET;
    records[1].pNormal = NORMAL_OFFSET;
    records[1].pColor = COLOR_OFFSET;
    vertices =
        (uint32_t *)(archive + 4 + VERTEX_OFFSET);
    vertices[0] =
        (uint32_t)(1 & 0x3ff) |
        (uint32_t)((-2 & 0x3ff) << 10) |
        (uint32_t)((3 & 0x3ff) << 20);
    vertices[1] =
        (uint32_t)(4 & 0x3ff) |
        (uint32_t)((-2 & 0x3ff) << 10) |
        (uint32_t)((-4 & 0x3ff) << 20);
    vertices[2] =
        (uint32_t)((-4 & 0x3ff)) |
        (uint32_t)((-2 & 0x3ff) << 10) |
        (uint32_t)((-4 & 0x3ff) << 20);
    face =
        (JPBBmdFaceRecord *)(archive + 4 + FACE_OFFSET);
    face->vertex[0] = 0;
    face->vertex[1] = 1;
    face->vertex[2] = -2;
    face->vertex[3] = INT16_MAX;
    uv = (faceUV *)(archive + 4 + UV_OFFSET);
    uv->uv[0].u = 0.25f;
    uv->uv[0].v = 0.5f;
    uv->uv[1].u = 0.75f;
    uv->uv[1].v = 0.5f;
    uv->uv[2].u = 0.5f;
    uv->uv[2].v = 1.0f;
    colors = (CVECTOR *)(archive + 4 + COLOR_OFFSET);
    colors[0].r = 10;
    colors[0].g = 20;
    colors[0].b = 30;
    colors[1].r = 40;
    colors[1].g = 50;
    colors[1].b = 60;
    colors[2].r = 70;
    colors[2].g = 80;
    colors[2].b = 90;
    CHECK(jpb_BmdInspect(
              archive, sizeof(archive), &view) == JPB_BMD_OK);
    CHECK(jpb_BmdGetGeometry(
              &view, view.root, &geometry) == JPB_BMD_OK);
    CHECK(geometry.local_vertex_count == 3);
    CHECK(geometry.face_encoding == JPB_BMD_FACE_SIGNED_16);
    CHECK(geometry.face_count == 1);
    CHECK(geometry.corner_count == 3);
    CHECK(jpb_BmdFaceCornerCount(&geometry, 0) == 3);
    CHECK(jpb_BmdFaceUv(&geometry, 0, 0)->u == 0.25f);
    CHECK(jpb_BmdFaceUv(&geometry, 0, 2)->v == 1.0f);
    CHECK(jpb_BmdFaceUv(&geometry, 0, 3) == NULL);
    CHECK(jpb_BmdFaceColor(&geometry, 0, 0)->r == 10);
    CHECK(jpb_BmdFaceColor(&geometry, 0, 2)->b == 90);
    CHECK(jpb_BmdFaceColor(&geometry, 0, 3) == NULL);
    jpb_BmdDecodePackedVertex(vertices[0], &decoded);
    CHECK(decoded.vx == 1.0f);
    CHECK(decoded.vy == -2.0f);
    CHECK(decoded.vz == 3.0f);
    memset(&model, 0, sizeof(model));
    CHECK(jpb_BmdBuildModel(
              &view, &model, &node, 1, -1) == JPB_BMD_OK);
    CHECK(model.v3Scale.vx == 2 * JPB_FIXED_ONE);
    CHECK(model.v3Scale.vy == 2 * JPB_FIXED_ONE);
    CHECK(model.v3Scale.vz == 2 * JPB_FIXED_ONE);
    memset(&world_view, 0, sizeof(world_view));
    memset(&world_scene, 0, sizeof(world_scene));
    world_scene.view = &world_view;
    world_scene.minX = -10.0f;
    world_scene.maxX = 10.0f;
    world_scene.minY = -10.0f;
    world_scene.maxY = 10.0f;
    world_scene.minZ = -10.0f;
    world_scene.maxZ = 10.0f;
    memset(pixels, 0, sizeof(pixels));
    framebuffer.pixels = pixels;
    framebuffer.width = 64;
    framebuffer.height = 64;
    framebuffer.stridePixels = 64;
    memset(&render_stats, 0, sizeof(render_stats));
    CHECK(jpb_SoftwareRenderBmdWireframe(
              &view,
              &model,
              NULL,
              &world_position,
              0,
              NULL,
              &world_scene,
              &framebuffer,
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.modelTriangles == 1);
    CHECK(render_stats.modelLines == 3);
    CHECK(render_stats.modelPixels > 0);
    CHECK(node.v3RotCenter.vx == 1);
    CHECK(node.v3RotCenter.vy == 2);
    CHECK(node.v3RotCenter.vz == 3);
    CHECK(node.v3Velocity.vx == 1);
    CHECK(node.v3Velocity.vy == 2);
    CHECK(node.v3Velocity.vz == 3);
    memset(&key_frame, 0, sizeof(key_frame));
    key_frame.v3RootTranslation.vx = 1;
    key_frame.v3RootTranslation.vy = 2;
    key_frame.v3RootTranslation.vz = 3;
    node.v3Translation.vx = 100;
    node.v3Translation.vy = 100;
    node.v3Translation.vz = 100;
    memset(&render_stats, 0, sizeof(render_stats));
    CHECK(jpb_SoftwareRenderBmdWireframe(
              &view,
              &model,
              &key_frame,
              &world_position,
              0,
              NULL,
              &world_scene,
              &framebuffer,
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    /* model scale 2 transforms root (1,2,3), then world (1,2,3) is added. */
    CHECK(node.v3RotCenter.vx == 3);
    CHECK(node.v3RotCenter.vy == 6);
    CHECK(node.v3RotCenter.vz == 9);
    node.v3Translation.vx = 0;
    node.v3Translation.vy = 0;
    node.v3Translation.vz = 0;
    node.v3RotCenter.vx = (int32_t)world_position.vx;
    node.v3RotCenter.vy = (int32_t)world_position.vy;
    node.v3RotCenter.vz = (int32_t)world_position.vz;
    memset(pixels, 0, sizeof(pixels));
    memset(&render_stats, 0, sizeof(render_stats));
    CHECK(jpb_SoftwareRenderBmdMaterialized(
              &view,
              &model,
              NULL,
              &world_position,
              0,
              NULL,
              &world_scene,
              &framebuffer,
              resolve_white_texture,
              NULL,
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.modelTriangles == 1);
    CHECK(render_stats.modelLines == 0);
    CHECK(render_stats.modelPixels > 0);
    CHECK(node.v3Velocity.vx == 0);
    CHECK(node.v3Velocity.vy == 0);
    CHECK(node.v3Velocity.vz == 0);
    depth_buffer.values = depth_values;
    depth_buffer.width = 64;
    depth_buffer.height = 64;
    depth_buffer.strideValues = 64;
    CHECK(jpb_SoftwareClearDepthBuffer(
              &depth_buffer) == 1);
    memset(pixels, 0, sizeof(pixels));
    memset(&render_stats, 0, sizeof(render_stats));
    CHECK(jpb_SoftwareRenderBmdMaterializedWithDepth(
              &view,
              &model,
              NULL,
              &world_position,
              0,
              NULL,
              &world_scene,
              &framebuffer,
              resolve_white_texture,
              NULL,
              &depth_buffer,
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.modelPixels > 0);
    {
        size_t depth_index;
        int depth_written = 0;

        for (depth_index = 0;
             depth_index <
                 sizeof(depth_values) /
                     sizeof(depth_values[0]);
             ++depth_index) {
            if (depth_values[depth_index] < FLT_MAX) {
                depth_written = 1;
                break;
            }
        }
        CHECK(depth_written);
    }

    /*
     * NoScaleEndPoly rejects negative projected winding only for the exact
     * flags-zero material mode. Flag one makes the same face two-sided.
     */
    face->vertex[0] = 0;
    face->vertex[1] = -2;
    face->vertex[2] = 1;
    face->vertex[3] = INT16_MAX;
    test_material_flags = JPB_MATERIAL_MODE_BACKFACE_REJECT;
    test_color_override = -1;
    CHECK(jpb_SoftwareClearDepthBuffer(&depth_buffer) == 1);
    memset(pixels, 0, sizeof(pixels));
    memset(&render_stats, 0, sizeof(render_stats));
    CHECK(jpb_SoftwareRenderBmdMaterializedWithDepth(
              &view,
              &model,
              NULL,
              &world_position,
              0,
              NULL,
              &world_scene,
              &framebuffer,
              resolve_white_texture,
              NULL,
              &depth_buffer,
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.modelPixels == 0);

    test_material_flags = JPB_MATERIAL_MODE_TWO_SIDED;
    test_color_override = 100;
    CHECK(jpb_SoftwareClearDepthBuffer(&depth_buffer) == 1);
    memset(pixels, 0, sizeof(pixels));
    memset(&render_stats, 0, sizeof(render_stats));
    CHECK(jpb_SoftwareRenderBmdMaterializedWithDepth(
              &view,
              &model,
              NULL,
              &world_position,
              0,
              NULL,
              &world_scene,
              &framebuffer,
              resolve_white_texture,
              NULL,
              &depth_buffer,
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.modelPixels > 0);
    {
        size_t pixel_index;
        int saw_override = 0;

        for (pixel_index = 0;
             pixel_index < sizeof(pixels) / sizeof(pixels[0]);
             ++pixel_index) {
            if (pixels[pixel_index] == UINT32_C(0x00646464)) {
                saw_override = 1;
                break;
            }
        }
        CHECK(saw_override);
    }
    test_material_flags = JPB_MATERIAL_MODE_BACKFACE_REJECT;
    test_color_override = -1;

    /*
     * The D3D perspective stage clips a primitive that straddles camera
     * Z == 1 instead of dropping the whole face or dividing behind-camera
     * vertices by a clamped depth. This fixture has one vertex in front and
     * two behind the near plane.
     */
    memset(&view_matrix, 0, sizeof(view_matrix));
    view_matrix.m[0][0] = 1.0f;
    view_matrix.m[1][1] = 1.0f;
    view_matrix.m[2][2] = 1.0f;
    test_material_flags = JPB_MATERIAL_MODE_TWO_SIDED;
    world_position.vx = 0.0f;
    world_position.vy = 3.0f;
    world_position.vz = 0.0f;
    CHECK(jpb_SoftwareClearDepthBuffer(&depth_buffer) == 1);
    memset(pixels, 0, sizeof(pixels));
    memset(&render_stats, 0, sizeof(render_stats));
    CHECK(jpb_SoftwareRenderBmdMaterializedWithDepth(
              &view,
              &model,
              NULL,
              &world_position,
              0,
              &view_matrix,
              &world_scene,
              &framebuffer,
              resolve_white_texture,
              NULL,
              &depth_buffer,
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.modelPixels > 0);
    {
        size_t depth_index;
        float minimum_depth = FLT_MAX;

        for (depth_index = 0;
             depth_index <
                 sizeof(depth_values) /
                     sizeof(depth_values[0]);
             ++depth_index) {
            if (depth_values[depth_index] < minimum_depth) {
                minimum_depth = depth_values[depth_index];
            }
        }
        CHECK(minimum_depth >= 1.0f / 10240.0f);
    }
    world_position.vz = -20.0f;
    CHECK(jpb_SoftwareClearDepthBuffer(&depth_buffer) == 1);
    memset(pixels, 0, sizeof(pixels));
    memset(&render_stats, 0, sizeof(render_stats));
    CHECK(jpb_SoftwareRenderBmdMaterializedWithDepth(
              &view,
              &model,
              NULL,
              &world_position,
              0,
              &view_matrix,
              &world_scene,
              &framebuffer,
              resolve_white_texture,
              NULL,
              &depth_buffer,
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.modelPixels == 0);
    test_material_flags = JPB_MATERIAL_MODE_BACKFACE_REJECT;
    world_position.vx = 1.0f;
    world_position.vy = 2.0f;
    world_position.vz = 3.0f;

    face->vertex[0] = 0x0303;
    CHECK(jpb_BmdGetGeometry(
              &view, view.root, &geometry) ==
          JPB_BMD_INVALID_LAYOUT);
    packed_face = (JPBBmdPackedFaceRecord *)face;
    packed_face->vertex[0] = 0;
    packed_face->vertex[1] = 1;
    packed_face->vertex[2] = 2;
    packed_face->vertex[3] = UINT8_MAX;
    CHECK(jpb_BmdGetGeometry(
              &view, view.root, &geometry) == JPB_BMD_OK);
    CHECK(geometry.face_encoding == JPB_BMD_FACE_PACKED_8);
    CHECK(geometry.corner_count == 3);
    memset(&render_stats, 0, sizeof(render_stats));
    CHECK(jpb_SoftwareRenderBmdWireframe(
              &view,
              &model,
              NULL,
              &world_position,
              0,
              NULL,
              &world_scene,
              &framebuffer,
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.modelTriangles == 1);
    packed_face->vertex[2] = 3;
    CHECK(jpb_BmdGetGeometry(
              &view, view.root, &geometry) ==
          JPB_BMD_INVALID_LAYOUT);
    return 0;
}

int main(void)
{
    CHECK(test_inspect_and_build() == 0);
    CHECK(test_invalid_archives() == 0);
    CHECK(test_geometry_view() == 0);
    puts("BMD tests passed");
    return 0;
}
