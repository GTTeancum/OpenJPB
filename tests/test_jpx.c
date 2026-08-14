#include "jpb/jpx.h"
#include "jpb/software_renderer.h"

#include <float.h>
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

static void write_u16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static int resolver_calls;
static int progress_calls;
static int progress_total;
static int patch_site_calls;
static size_t last_patch_offset;
static int spatial_node_calls;
static int texture_resolver_calls;
static uint32_t transparent_test_color =
    UINT32_C(0x80ff0000);

static int resolve_test_texture(
    void *user_data,
    const char *texture_name,
    JPBSoftwareTexture *texture)
{
    static const uint32_t pixels[4] = {
        UINT32_C(0xffff0000),
        UINT32_C(0xff00ff00),
        UINT32_C(0xff0000ff),
        UINT32_C(0xffffffff)
    };

    (void)user_data;
    ++texture_resolver_calls;
    if (texture_name == NULL ||
        strcmp(texture_name, "B.TGA") != 0) {
        return 0;
    }
    texture->pixels = pixels;
    texture->width = 2;
    texture->height = 2;
    texture->stridePixels = 2;
    return 1;
}

static int resolve_transparent_test_texture(
    void *user_data,
    const char *texture_name,
    JPBSoftwareTexture *texture)
{
    static uint32_t pixels[4];
    size_t index;

    (void)user_data;
    ++texture_resolver_calls;
    if (texture_name == NULL ||
        (strcmp(texture_name, "B.TGA") != 0 &&
         strcmp(texture_name, "T_ORANGE.TGA") != 0)) {
        return 0;
    }
    for (index = 0;
         index < sizeof(pixels) / sizeof(pixels[0]);
         ++index) {
        pixels[index] = transparent_test_color;
    }
    texture->pixels = pixels;
    texture->width = 2;
    texture->height = 2;
    texture->stridePixels = 2;
    return 1;
}

static int resolve_test_material(
    const char *level_name,
    const char *material_name,
    int8_t list_type,
    JPBJpxDescriptor *descriptor,
    void *user_data)
{
    int *user_calls = (int *)user_data;
    uint8_t fill;

    if (strcmp(level_name, "unit") != 0) {
        return 1;
    }
    if (strcmp(material_name, "FIRST.TGA") == 0 && list_type == 3) {
        fill = 0x11;
    } else if (strcmp(material_name, "B.TGA") == 0 && list_type == -2) {
        fill = 0x22;
    } else {
        return 1;
    }
    memset(descriptor->bytes, fill, sizeof(descriptor->bytes));
    ++resolver_calls;
    ++*user_calls;
    return 0;
}

static void test_progress(int amount, void *user_data)
{
    (void)user_data;
    ++progress_calls;
    progress_total += amount;
}

static int visit_test_patch_site(
    const JPBJpxPatchSite *site, void *user_data)
{
    JPBJpxVertex vertex;

    (void)user_data;
    if (!site->hasStripHeadMarker ||
        site->materialIndex != (uint32_t)(1 - patch_site_calls)) {
        return 1;
    }
    if (patch_site_calls == 0) {
        if (site->vertexCount != 8 ||
            site->followingMetadataSize != 4 ||
            jpx_DecodeVertex(site, 0, &vertex) != JPB_JPX_OK ||
            vertex.x != 1.0f ||
            vertex.z != -2.0f ||
            vertex.y != 3.0f ||
            vertex.rawU != 4096 ||
            vertex.rawV != -4096 ||
            vertex.u != 1.0f ||
            vertex.v != -1.0f ||
            vertex.attributes != UINT32_C(0xE0112233) ||
            jpx_DecodeVertex(site, 8, &vertex) !=
                JPB_JPX_INVALID_STRIP) {
            return 1;
        }
    } else if (site->vertexCount != 4 ||
               site->followingMetadataSize != 4) {
        return 1;
    }
    last_patch_offset = site->offset;
    ++patch_site_calls;
    return 0;
}

static int visit_test_spatial_node(
    const JPBJpxSpatialNode *node, void *user_data)
{
    (void)user_data;
    if (node->index != 7 ||
        node->x != 1.0f ||
        node->z != 2.0f ||
        node->y != 3.0f ||
        node->radius != 4.0f ||
        node->vertexCount != 8 ||
        node->offset != 32) {
        return 1;
    }
    ++spatial_node_calls;
    return 0;
}

static void write_f32(uint8_t *bytes, float value)
{
    memcpy(bytes, &value, sizeof(value));
}

int main(void)
{
    enum {
        WORLD_OFFSET = 32,
        FIRST_STRIP_OFFSET = 28,
        FIRST_STRIP = WORLD_OFFSET + FIRST_STRIP_OFFSET,
        FIRST_VERTEX = FIRST_STRIP + JPB_JPX_DESCRIPTOR_SIZE,
        SECOND_STRIP =
            FIRST_VERTEX + 8 * JPB_JPX_VERTEX_SIZE + 4,
        SECOND_VERTEX = SECOND_STRIP + JPB_JPX_DESCRIPTOR_SIZE,
        FILE_SIZE =
            SECOND_VERTEX + 4 * JPB_JPX_VERTEX_SIZE + 4 + 4
    };
    uint8_t bytes[FILE_SIZE] = {0};
    uint8_t invalid[FILE_SIZE];
    uint8_t runtime_bytes[FILE_SIZE];
    uint8_t runtime_storage[FILE_SIZE];
    uint8_t vertex_swap[JPB_JPX_VERTEX_SIZE];
    uint8_t second_strip_vertices[4 * JPB_JPX_VERTEX_SIZE];
    uint8_t near_clip_vertices[8 * JPB_JPX_VERTEX_SIZE];
    uint8_t near_clip_second_vertices[4 * JPB_JPX_VERTEX_SIZE];
    JPBJpxDescriptor descriptors[2];
    JPBJpxDescriptor runtime_descriptors[2];
    JPBJpxLoadConfig config;
    JPBJpxView view;
    JPBSoftwareJpxScene software_scene;
    JPBSoftwareOwnedLevelMesh owned_level_mesh;
    JPBSoftwareFramebuffer framebuffer;
    JPBSoftwareDepthBuffer depth_buffer;
    JPBSoftwareRenderStats render_stats;
    MATRIX perspective_view;
    FVECTOR camera_focus;
    FVECTOR desired_eye;
    FVECTOR clipped_eye;
    float camera_hit_fraction;
    uint32_t pixels[64 * 64];
    float depth_values[64 * 64];
    const char *runtime_path = "jpb_jpx_runtime_test.bin";
    FILE *runtime_file;
    int user_resolver_calls = 0;

    write_u16(bytes, 2);
    write_u16(bytes + 2, WORLD_OFFSET);
    write_u16(bytes + 4, FIRST_STRIP_OFFSET);
    write_u16(bytes + 6, 14);
    bytes[8] = 3;
    write_u16(bytes + 10, 24);
    bytes[12] = (uint8_t)-2;
    memcpy(bytes + 14, "FIRST.TGA", 10);
    memcpy(bytes + 24, "B.TGA", 6);
    write_u16(bytes + FIRST_STRIP - 28, UINT16_C(0xFFFF));
    write_u16(bytes + FIRST_STRIP - 26, 7);
    write_f32(bytes + FIRST_STRIP - 24, 1.0f);
    write_f32(bytes + FIRST_STRIP - 20, 2.0f);
    write_f32(bytes + FIRST_STRIP - 16, 3.0f);
    write_f32(bytes + FIRST_STRIP - 12, 4.0f);
    write_u32(bytes + FIRST_STRIP - 8, 1);
    write_u32(bytes + FIRST_STRIP - 4, 8U << 16);
    write_u32(bytes + FIRST_STRIP, SECOND_STRIP - FIRST_STRIP);
    write_u32(bytes + FIRST_STRIP + 4, 1);
    memcpy(bytes + FIRST_STRIP + 8, "STRPHEAD", 8);
    write_u16(bytes + FIRST_VERTEX, 128);
    write_u16(bytes + FIRST_VERTEX + 2, (uint16_t)-256);
    write_f32(bytes + FIRST_VERTEX + 4, 3.0f);
    write_u16(bytes + FIRST_VERTEX + 8, 4096);
    write_u16(bytes + FIRST_VERTEX + 10, (uint16_t)-4096);
    write_u32(bytes + FIRST_VERTEX + 12, UINT32_C(0xE0112233));
    write_u32(bytes + SECOND_STRIP - 4, 4U << 16);
    write_u32(bytes + SECOND_STRIP, 84);
    write_u32(bytes + SECOND_STRIP + 4, 0);
    memcpy(bytes + SECOND_STRIP + 8, "STRPHEAD", 8);
    memcpy(runtime_bytes, bytes, sizeof(runtime_bytes));

    CHECK(jpx_Inspect(bytes, sizeof(bytes), &view) == JPB_JPX_OK);
    CHECK(view.numMaterials == 2);
    CHECK(view.worldOffset == WORLD_OFFSET);
    CHECK(view.firstStripOffset == FIRST_STRIP_OFFSET);
    CHECK(view.stripCount == 2);
    CHECK(strcmp(jpx_GetMaterialName(&view, 0), "FIRST.TGA") == 0);
    CHECK(strcmp(jpx_GetMaterialName(&view, 1), "B.TGA") == 0);
    CHECK(jpx_GetMaterialName(&view, 2) == NULL);
    CHECK(jpx_GetMaterialListType(&view, 0) == 3);
    CHECK(jpx_GetMaterialListType(&view, 1) == -2);
    CHECK(jpb_SoftwarePrepareJpxScene(
              &view, &software_scene) == JPB_SOFTWARE_RENDER_OK);
    CHECK(software_scene.strips == 2);
    CHECK(software_scene.vertices == 12);
    CHECK(software_scene.minX == 0.0f);
    CHECK(software_scene.maxX == 1.0f);
    CHECK(software_scene.minZ == -2.0f);
    CHECK(software_scene.maxZ == 0.0f);
    framebuffer.pixels = pixels;
    framebuffer.width = 64;
    framebuffer.height = 64;
    framebuffer.stridePixels = 64;
    CHECK(jpb_SoftwareRenderJpxWireframe(
              &software_scene,
              NULL,
              &framebuffer,
              UINT32_C(0x00010203),
              &render_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.lines > 0);
    CHECK(render_stats.pixels > 0);

    /*
     * Give the first strip one visible triangle with repeating UVs and the
     * packed JPX CVECTOR bytes used by the materialized world path.
     */
    write_u16(bytes + FIRST_VERTEX, 0);
    write_u16(bytes + FIRST_VERTEX + 2, 0);
    write_f32(bytes + FIRST_VERTEX + 4, 0.0f);
    write_u16(bytes + FIRST_VERTEX + 8, 0);
    write_u16(bytes + FIRST_VERTEX + 10, 0);
    write_u32(
        bytes + FIRST_VERTEX + 12,
        UINT32_C(0xffffffff));
    write_u16(
        bytes + FIRST_VERTEX + JPB_JPX_VERTEX_SIZE,
        128);
    write_u16(
        bytes + FIRST_VERTEX +
            JPB_JPX_VERTEX_SIZE + 2,
        0);
    write_f32(
        bytes + FIRST_VERTEX +
            JPB_JPX_VERTEX_SIZE + 4,
        0.0f);
    write_u16(
        bytes + FIRST_VERTEX +
            JPB_JPX_VERTEX_SIZE + 8,
        8192);
    write_u16(
        bytes + FIRST_VERTEX +
            JPB_JPX_VERTEX_SIZE + 10,
        0);
    write_u32(
        bytes + FIRST_VERTEX +
            JPB_JPX_VERTEX_SIZE + 12,
        UINT32_C(0xffffffff));
    write_u16(
        bytes + FIRST_VERTEX +
            2 * JPB_JPX_VERTEX_SIZE,
        0);
    write_u16(
        bytes + FIRST_VERTEX +
            2 * JPB_JPX_VERTEX_SIZE + 2,
        128);
    write_f32(
        bytes + FIRST_VERTEX +
            2 * JPB_JPX_VERTEX_SIZE + 4,
        0.0f);
    write_u16(
        bytes + FIRST_VERTEX +
            2 * JPB_JPX_VERTEX_SIZE + 8,
        0);
    write_u16(
        bytes + FIRST_VERTEX +
            2 * JPB_JPX_VERTEX_SIZE + 10,
        8192);
    write_u32(
        bytes + FIRST_VERTEX +
            2 * JPB_JPX_VERTEX_SIZE + 12,
        UINT32_C(0xffffffff));
    {
        size_t vertex_index;

        for (vertex_index = 3; vertex_index < 8; ++vertex_index) {
            memcpy(
                bytes + FIRST_VERTEX +
                    vertex_index * JPB_JPX_VERTEX_SIZE,
                bytes + FIRST_VERTEX + 2 * JPB_JPX_VERTEX_SIZE,
                JPB_JPX_VERTEX_SIZE);
        }
    }
    CHECK(jpb_SoftwarePrepareJpxScene(
              &view, &software_scene) ==
          JPB_SOFTWARE_RENDER_OK);
    memset(&owned_level_mesh, 0, sizeof(owned_level_mesh));
    CHECK(jpb_SoftwareBuildJpxLevelMesh(
              &software_scene, &owned_level_mesh) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(owned_level_mesh.mesh.batchCount > 0);
    CHECK(owned_level_mesh.mesh.batchCount <= software_scene.strips);
    CHECK(owned_level_mesh.mesh.triangles > 0);
    CHECK(owned_level_mesh.mesh.vertices ==
          owned_level_mesh.mesh.triangles * 3);
    CHECK(owned_level_mesh.mesh.batches == owned_level_mesh.batches);
    CHECK(owned_level_mesh.mesh.batches[0].vertices != NULL);
    CHECK(owned_level_mesh.mesh.batches[0].vertexCount >= 3);
    CHECK(owned_level_mesh.mesh.batches[0].meshCount ==
          software_scene.strips);
    jpb_SoftwareFreeOwnedLevelMesh(&owned_level_mesh);
    CHECK(owned_level_mesh.mesh.batches == NULL);
    CHECK(owned_level_mesh.vertices == NULL);
    camera_focus.vx = 0.25f;
    camera_focus.vy = 50.0f;
    camera_focus.vz = 0.25f;
    desired_eye.vx = 0.25f;
    desired_eye.vy = -50.0f;
    desired_eye.vz = 0.25f;
    CHECK(jpb_SoftwareClipCameraToJpx(
              &software_scene,
              &camera_focus,
              &desired_eye,
              10.0f,
              &clipped_eye,
              &camera_hit_fraction) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(camera_hit_fraction > 0.49f);
    CHECK(camera_hit_fraction < 0.51f);
    CHECK(clipped_eye.vx == 0.25f);
    CHECK(clipped_eye.vy > 9.9f);
    CHECK(clipped_eye.vy < 10.1f);
    CHECK(clipped_eye.vz == 0.25f);
    desired_eye.vy = 100.0f;
    CHECK(jpb_SoftwareClipCameraToJpx(
              &software_scene,
              &camera_focus,
              &desired_eye,
              10.0f,
              &clipped_eye,
              &camera_hit_fraction) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(camera_hit_fraction == 1.0f);
    CHECK(clipped_eye.vy == desired_eye.vy);
    depth_buffer.values = depth_values;
    depth_buffer.width = 64;
    depth_buffer.height = 64;
    depth_buffer.strideValues = 64;
    texture_resolver_calls = 0;
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              NULL,
              &framebuffer,
              UINT32_C(0x00010203),
              resolve_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(texture_resolver_calls == 1);
    CHECK(render_stats.triangles > 0);
    CHECK(render_stats.lines == 0);
    CHECK(render_stats.pixels > 0);
    CHECK(render_stats.modelPixels == 0);
    CHECK(depth_values[32 * 64 + 32] < FLT_MAX);
    CHECK(jpb_SoftwareClearDepthBuffer(
              &depth_buffer) == 1);
    CHECK(depth_values[32 * 64 + 32] == FLT_MAX);

    /*
     * Live FBX geometry remains floating point while the recovered gameplay
     * world, collision positions, and authored cameras wrap at signed
     * 16-bit coordinates. Coruscant 1 crosses that seam. Prove that the
     * renderer selects the FBX world image nearest the live camera, while an
     * unidentified inspection scene retains the unwrapped source geometry.
     */
    {
        const uint32_t clear_color = UINT32_C(0x00010203);
        JPBSoftwareLevelVertex seam_vertices[3] = {
            {{65534.0f, -2.0f, 10.0f},
             0.0f, 0.0f, 255.0f, 255.0f, 255.0f, 255.0f},
            {{65538.0f, -2.0f, 10.0f},
             1.0f, 0.0f, 255.0f, 255.0f, 255.0f, 255.0f},
            {{65536.0f, 2.0f, 10.0f},
             0.5f, 1.0f, 255.0f, 255.0f, 255.0f, 255.0f}
        };
        JPBSoftwareLevelBatch seam_batch = {
            seam_vertices,
            3,
            NULL,
            "signed_world_seam",
            JPB_LEVEL_FBX_PASS_OPAQUE,
            0,
            32
        };
        JPBSoftwareLevelMesh seam_mesh = {
            &seam_batch,
            1,
            6,
            3,
            1
        };

        memset(&perspective_view, 0, sizeof(perspective_view));
        perspective_view.m[0][0] = 1.0f;
        perspective_view.m[1][1] = 1.0f;
        perspective_view.m[2][2] = 1.0f;
        software_scene.levelIndex = JPB_LEVEL_INDEX_NONE;
        CHECK(jpb_SoftwareRenderLevelMesh(
                  &seam_mesh,
                  &software_scene,
                  &perspective_view,
                  &framebuffer,
                  clear_color,
                  NULL,
                  NULL,
                  &depth_buffer,
                  &render_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(render_stats.triangles == 1);
        CHECK(render_stats.pixels == 0);

        software_scene.levelIndex = 6;
        CHECK(jpb_SoftwareRenderLevelMesh(
                  &seam_mesh,
                  &software_scene,
                  &perspective_view,
                  &framebuffer,
                  clear_color,
                  NULL,
                  NULL,
                  &depth_buffer,
                  &render_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(render_stats.triangles == 1);
        CHECK(render_stats.pixels > 0);
        CHECK(pixels[32 * 64 + 32] != clear_color);
    }

    /* The live D3D level PSO clips strip triangles at camera Z == 1. */
    memcpy(
        near_clip_vertices,
        bytes + FIRST_VERTEX,
        sizeof(near_clip_vertices));
    memcpy(
        near_clip_second_vertices,
        bytes + SECOND_VERTEX,
        sizeof(near_clip_second_vertices));
    write_u16(bytes + FIRST_VERTEX, (uint16_t)-64);
    write_u16(bytes + FIRST_VERTEX + 2, 0);
    write_f32(bytes + FIRST_VERTEX + 4, -0.5f);
    write_u16(
        bytes + FIRST_VERTEX + JPB_JPX_VERTEX_SIZE,
        64);
    write_u16(
        bytes + FIRST_VERTEX + JPB_JPX_VERTEX_SIZE + 2,
        0);
    write_f32(
        bytes + FIRST_VERTEX + JPB_JPX_VERTEX_SIZE + 4,
        -0.5f);
    write_u16(
        bytes + FIRST_VERTEX + 2 * JPB_JPX_VERTEX_SIZE,
        0);
    write_u16(
        bytes + FIRST_VERTEX + 2 * JPB_JPX_VERTEX_SIZE + 2,
        512);
    write_f32(
        bytes + FIRST_VERTEX + 2 * JPB_JPX_VERTEX_SIZE + 4,
        0.5f);
    {
        size_t vertex_index;

        for (vertex_index = 3; vertex_index < 8; ++vertex_index) {
            memcpy(
                bytes + FIRST_VERTEX +
                    vertex_index * JPB_JPX_VERTEX_SIZE,
                bytes + FIRST_VERTEX +
                    2 * JPB_JPX_VERTEX_SIZE,
                JPB_JPX_VERTEX_SIZE);
        }
        for (vertex_index = 0; vertex_index < 4; ++vertex_index) {
            memcpy(
                bytes + SECOND_VERTEX +
                    vertex_index * JPB_JPX_VERTEX_SIZE,
                bytes + FIRST_VERTEX,
                JPB_JPX_VERTEX_SIZE);
        }
    }
    CHECK(jpb_SoftwarePrepareJpxScene(
              &view, &software_scene) ==
          JPB_SOFTWARE_RENDER_OK);
    memset(&perspective_view, 0, sizeof(perspective_view));
    perspective_view.m[0][0] = 1.0f;
    perspective_view.m[1][1] = 1.0f;
    perspective_view.m[2][2] = 1.0f;
    CHECK(jpb_SoftwareClearDepthBuffer(&depth_buffer) == 1);
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              &perspective_view,
              &framebuffer,
              UINT32_C(0x00010203),
              resolve_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.pixels > 0);
    write_u16(
        bytes + FIRST_VERTEX + 2 * JPB_JPX_VERTEX_SIZE + 2,
        0);
    {
        size_t vertex_index;

        for (vertex_index = 3; vertex_index < 8; ++vertex_index) {
            memcpy(
                bytes + FIRST_VERTEX +
                    vertex_index * JPB_JPX_VERTEX_SIZE,
                bytes + FIRST_VERTEX +
                    2 * JPB_JPX_VERTEX_SIZE,
                JPB_JPX_VERTEX_SIZE);
        }
    }
    CHECK(jpb_SoftwarePrepareJpxScene(
              &view, &software_scene) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(jpb_SoftwareClearDepthBuffer(&depth_buffer) == 1);
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              &perspective_view,
              &framebuffer,
              UINT32_C(0x00010203),
              resolve_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.pixels == 0);
    memcpy(
        bytes + FIRST_VERTEX,
        near_clip_vertices,
        sizeof(near_clip_vertices));
    memcpy(
        bytes + SECOND_VERTEX,
        near_clip_second_vertices,
        sizeof(near_clip_second_vertices));
    CHECK(jpb_SoftwarePrepareJpxScene(
              &view, &software_scene) ==
          JPB_SOFTWARE_RENDER_OK);

    /* The exact level PSO uses D3D12_CULL_MODE_NONE. */
    memcpy(
        second_strip_vertices,
        bytes + SECOND_VERTEX,
        sizeof(second_strip_vertices));
    {
        size_t vertex_index;

        for (vertex_index = 0; vertex_index < 4; ++vertex_index) {
            memcpy(
                bytes + SECOND_VERTEX +
                    vertex_index * JPB_JPX_VERTEX_SIZE,
                bytes + FIRST_VERTEX,
                JPB_JPX_VERTEX_SIZE);
        }
    }
    memcpy(
        vertex_swap,
        bytes + FIRST_VERTEX + JPB_JPX_VERTEX_SIZE,
        sizeof(vertex_swap));
    memcpy(
        bytes + FIRST_VERTEX + JPB_JPX_VERTEX_SIZE,
        bytes + FIRST_VERTEX + 2 * JPB_JPX_VERTEX_SIZE,
        sizeof(vertex_swap));
    memcpy(
        bytes + FIRST_VERTEX + 2 * JPB_JPX_VERTEX_SIZE,
        vertex_swap,
        sizeof(vertex_swap));
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              NULL,
              &framebuffer,
              UINT32_C(0x00010203),
              resolve_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.triangles == 1);
    CHECK(render_stats.pixels > 0);
    memcpy(
        vertex_swap,
        bytes + FIRST_VERTEX + JPB_JPX_VERTEX_SIZE,
        sizeof(vertex_swap));
    memcpy(
        bytes + FIRST_VERTEX + JPB_JPX_VERTEX_SIZE,
        bytes + FIRST_VERTEX + 2 * JPB_JPX_VERTEX_SIZE,
        sizeof(vertex_swap));
    memcpy(
        bytes + FIRST_VERTEX + 2 * JPB_JPX_VERTEX_SIZE,
        vertex_swap,
        sizeof(vertex_swap));
    memcpy(
        bytes + SECOND_VERTEX,
        second_strip_vertices,
        sizeof(second_strip_vertices));
    CHECK(jpb_SoftwareClearDepthBuffer(&depth_buffer) == 1);

    /* Type 16 is the shipped P_* punch-through/cutout, not blend, queue. */
    bytes[12] = 16;
    texture_resolver_calls = 0;
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              NULL,
              &framebuffer,
              UINT32_C(0x00010203),
              resolve_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(texture_resolver_calls == 1);
    CHECK(render_stats.levelTransparentTriangles == 0);
    CHECK(render_stats.levelTransparentPixels == 0);
    CHECK(depth_values[32 * 64 + 32] < FLT_MAX);

    /*
     * MATHEAD list type 8 selects the reference transparency shader/pass.
     * Ordinary transparent surfaces retain depth writes; names from the
     * recovered glassTextures initializer use the later no-depth-write pass.
     */
    bytes[12] = 8;
    transparent_test_color = UINT32_C(0x80ff0000);
    texture_resolver_calls = 0;
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              NULL,
              &framebuffer,
              UINT32_C(0x000000ff),
              resolve_transparent_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(texture_resolver_calls == 1);
    CHECK(render_stats.levelTransparentTriangles > 0);
    CHECK(render_stats.levelTransparentPixels > 0);
    CHECK(render_stats.levelGlassTriangles == 0);
    CHECK(render_stats.levelGlassPixels == 0);
    CHECK(pixels[32 * 64 + 32] == UINT32_C(0x0080007f));
    CHECK(depth_values[32 * 64 + 32] < FLT_MAX);

    write_u16(bytes + 10, 14);
    memcpy(bytes + 14, "T_ORANGE.TGA", 13);
    texture_resolver_calls = 0;
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              NULL,
              &framebuffer,
              UINT32_C(0x000000ff),
              resolve_transparent_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(texture_resolver_calls == 1);
    CHECK(render_stats.levelTransparentTriangles > 0);
    CHECK(render_stats.levelTransparentPixels > 0);
    CHECK(render_stats.levelGlassTriangles > 0);
    CHECK(render_stats.levelGlassPixels > 0);
    CHECK(pixels[32 * 64 + 32] == UINT32_C(0x0080007f));
    CHECK(depth_values[32 * 64 + 32] == FLT_MAX);

    /* LevelTransparencyPixelShader.hlsl discards sampled alpha below 0.1. */
    transparent_test_color = UINT32_C(0x19ff0000);
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              NULL,
              &framebuffer,
              UINT32_C(0x000000ff),
              resolve_transparent_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.levelTransparentTriangles > 0);
    CHECK(render_stats.levelTransparentPixels == 0);
    CHECK(render_stats.levelGlassTriangles > 0);
    CHECK(render_stats.levelGlassPixels == 0);
    CHECK(pixels[32 * 64 + 32] == UINT32_C(0x000000ff));
    CHECK(depth_values[32 * 64 + 32] == FLT_MAX);

    /*
     * The same JPX 8.3 ID resolves to t_orangefloor on Mini 1 and to
     * t_orangeglass on Mini 3. The exact per-level FBX database therefore
     * puts both in the transparent pass but only the latter in glass.
     */
    transparent_test_color = UINT32_C(0x80ff0000);
    CHECK(jpb_SoftwarePrepareJpxLevelScene(
              &view, 11, &software_scene) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              NULL,
              &framebuffer,
              UINT32_C(0x000000ff),
              resolve_transparent_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.levelTransparentTriangles > 0);
    CHECK(render_stats.levelGlassTriangles == 0);

    CHECK(jpb_SoftwarePrepareJpxLevelScene(
              &view, 13, &software_scene) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(jpb_SoftwareRenderJpxMaterialized(
              &software_scene,
              NULL,
              &framebuffer,
              UINT32_C(0x000000ff),
              resolve_transparent_test_texture,
              NULL,
              &depth_buffer,
              &render_stats) ==
          JPB_SOFTWARE_RENDER_OK);
    CHECK(render_stats.levelGlassTriangles > 0);

    CHECK(jpb_SoftwarePrepareJpxScene(NULL, &software_scene) ==
          JPB_SOFTWARE_RENDER_INVALID_ARGUMENT);
    CHECK(jpb_SoftwareClipCameraToJpx(
              NULL,
              &camera_focus,
              &desired_eye,
              0.0f,
              &clipped_eye,
              NULL) ==
          JPB_SOFTWARE_RENDER_INVALID_ARGUMENT);
    CHECK(jpb_SoftwareRenderJpxWireframe(
              &software_scene,
              NULL,
              NULL,
              0,
              NULL) == JPB_SOFTWARE_RENDER_INVALID_ARGUMENT);

    memset(descriptors[0].bytes, 0x11, sizeof(descriptors[0].bytes));
    memset(descriptors[1].bytes, 0x22, sizeof(descriptors[1].bytes));
    CHECK(jpx_PatchMaterials(&view, descriptors, 1) ==
          JPB_JPX_INVALID_MATERIAL);
    CHECK(jpx_PatchMaterials(&view, descriptors, 2) == JPB_JPX_OK);
    CHECK(memcmp(
              bytes + FIRST_STRIP,
              descriptors[1].bytes,
              JPB_JPX_DESCRIPTOR_SIZE) == 0);
    CHECK(memcmp(
              bytes + SECOND_STRIP,
              descriptors[0].bytes,
              JPB_JPX_DESCRIPTOR_SIZE) == 0);

    memcpy(invalid, bytes, sizeof(invalid));
    write_u16(invalid, JPB_JPX_MAX_MATERIALS + 1);
    CHECK(jpx_Inspect(invalid, sizeof(invalid), &view) ==
          JPB_JPX_INVALID_HEADER);

    memset(invalid, 0, sizeof(invalid));
    write_u16(invalid, 1);
    write_u16(invalid + 2, WORLD_OFFSET);
    write_u16(invalid + 4, FIRST_STRIP_OFFSET);
    write_u16(invalid + 6, WORLD_OFFSET);
    CHECK(jpx_Inspect(invalid, sizeof(invalid), &view) ==
          JPB_JPX_INVALID_MATERIAL);

    memset(invalid, 0, sizeof(invalid));
    write_u16(invalid, 1);
    write_u16(invalid + 2, WORLD_OFFSET);
    write_u16(invalid + 4, FIRST_STRIP_OFFSET);
    write_u16(invalid + 6, 10);
    memcpy(invalid + 10, "A.TGA", 6);
    write_u32(invalid + FIRST_STRIP, 16);
    write_u32(invalid + FIRST_STRIP + 4, 1);
    CHECK(jpx_Inspect(invalid, sizeof(invalid), &view) ==
          JPB_JPX_INVALID_STRIP);

    runtime_file = fopen(runtime_path, "wb");
    CHECK(runtime_file != NULL);
    CHECK(fwrite(
              runtime_bytes, 1, sizeof(runtime_bytes), runtime_file) ==
          sizeof(runtime_bytes));
    CHECK(fclose(runtime_file) == 0);

    memset(&config, 0, sizeof(config));
    config.storage = runtime_storage;
    config.storageCapacity = sizeof(runtime_storage) - 1;
    config.descriptors = runtime_descriptors;
    config.descriptorCapacity = 2;
    config.levelName = "unit";
    config.resolveMaterial = resolve_test_material;
    config.visitPatchSite = visit_test_patch_site;
    config.visitSpatialNode = visit_test_spatial_node;
    config.progress = test_progress;
    config.userData = &user_resolver_calls;
    CHECK(jpx_LoadFile(runtime_path, &config, &view) ==
          JPB_JPX_STORAGE_TOO_SMALL);
    config.storageCapacity = sizeof(runtime_storage);
    resolver_calls = 0;
    progress_calls = 0;
    progress_total = 0;
    patch_site_calls = 0;
    last_patch_offset = 0;
    spatial_node_calls = 0;
    jpx_SetRuntimeConfig(&config);
    CHECK(InitJPX((char *)runtime_path) == JPB_JPX_OK);
    CHECK(resolver_calls == 2);
    CHECK(user_resolver_calls == 2);
    CHECK(progress_calls == 4);
    CHECK(progress_total == 400);
    CHECK(patch_site_calls == 2);
    CHECK(last_patch_offset == SECOND_STRIP);
    CHECK(spatial_node_calls == 1);
    CHECK(WorldmeshData == (int32_t *)(runtime_storage + WORLD_OFFSET));
    CHECK(gJpxWorldmeshSize == FILE_SIZE - WORLD_OFFSET);
    CHECK(jpx_GetRuntimeView()->stripCount == 2);
    CHECK(memcmp(
              runtime_storage + FIRST_STRIP,
              runtime_descriptors[1].bytes,
              JPB_JPX_DESCRIPTOR_SIZE) == 0);
    CHECK(memcmp(
              runtime_storage + SECOND_STRIP,
              runtime_descriptors[0].bytes,
              JPB_JPX_DESCRIPTOR_SIZE) == 0);
    jpx_SetRuntimeConfig(NULL);
    CHECK(WorldmeshData == NULL);
    CHECK(gJpxWorldmeshSize == 0);
    CHECK(remove(runtime_path) == 0);

    CHECK(jpx_Inspect(NULL, 0, &view) == JPB_JPX_TRUNCATED);
    puts("jpx tests passed");
    return 0;
}
