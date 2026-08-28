#include "jpb/bmd.h"
#include "jpb/globalarrays.h"
#include "jpb/jonny.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/wrender.h"
#include "jpb/world.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static MATRIX identity_matrix(int z)
{
    MATRIX matrix = {{{1.0f, 0.0f, 0.0f},
                      {0.0f, 1.0f, 0.0f},
                      {0.0f, 0.0f, 1.0f}}, {0, 0, z}};
    return matrix;
}

static void expect_near(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.0001f);
}

static uint32_t pack_vertex(int x, int y, int z)
{
    return ((uint32_t)x & UINT32_C(0x3ff)) |
           (((uint32_t)y & UINT32_C(0x3ff)) << 10) |
           (((uint32_t)z & UINT32_C(0x3ff)) << 20);
}

static void set_scratch_i16(size_t offset, int16_t value)
{
    memcpy(&gaScratch[offset], &value, sizeof(value));
}

static void set_scratch_pointer(size_t offset, void *pointer)
{
    uintptr_t value = (uintptr_t)pointer;

    memcpy(&gaScratch[offset], &value, sizeof(value));
}

static void test_perspective_transform(void)
{
    MATRIX matrix = identity_matrix(0);
    _svector source = {100, -50, 1000, 0};
    fPoint4 destination = {0};

    _PerspectiveTransform(&matrix, &source, &destination);
    expect_near(destination.x, 396.8f);
    expect_near(destination.y, 201.6f);
    expect_near(destination.z, 0.09765625f);
}

static void test_cull_polarity_and_bounds(void)
{
    MATRIX matrix = identity_matrix(0);
    VECTOR point = {0, 0, 744, 0};

    assert(_Cull(&matrix, &point) == 0);
    point.vx = 1000;
    assert(_Cull(&matrix, &point) == 1);
    point.vx = 0;
    point.vz = 0;
    assert(_Cull(&matrix, &point) == 1);
}

static void test_render_node_expands_packed_vertices(void)
{
    geomData geometry;
    MATRIX matrix = identity_matrix(1000);
    MATRIX light = identity_matrix(0);
    fPoint4 cache[8];
    uint32_t vertices[3] = {
        pack_vertex(-1, -2, -3),
        pack_vertex(1, 2, 3),
        pack_vertex(0, 0, 0)};
    uint32_t normals[1] = {0};
    faceUV uv[1] = {{{{0.0f, 0.0f}}}};
    CVECTOR colors[4] = {{0}};
    uint32_t indices[1] = {UINT32_C(0xff020100)};

    memset(&geometry, 0, sizeof(geometry));
    memset(cache, 0, sizeof(cache));
    pointerRegistry_Reset();
    geometry.numVerts = 1;
    geometry.numShareVerts = 1;
    geometry.pVertex = addPtr(vertices, JPB_POINTER_ARRAY_VERTEX);
    geometry.pNormal = addPtr(normals, JPB_POINTER_ARRAY_NORMAL);
    geometry.pUV = addPtr(uv, JPB_POINTER_ARRAY_UV);
    geometry.pColor = addPtr(colors, JPB_POINTER_ARRAY_COLOR);
    geometry.pIndex = addPtr(indices, JPB_POINTER_ARRAY_INDEX);

    assert(gl_RenderNode(&geometry, &matrix, &light, cache) == 1);
    expect_near(cache[1].x, 320.0f - 768.0f / 997.0f);
    expect_near(cache[1].y, 240.0f - 1536.0f / 997.0f);
    expect_near(cache[1].z, 997.0f / 10240.0f);
    expect_near(cache[2].x, 320.0f + 768.0f / 1003.0f);
    expect_near(cache[2].y, 240.0f + 1536.0f / 1003.0f);
    expect_near(cache[2].z, 1003.0f / 10240.0f);
}

static void test_particle_fixed_point_activation(void)
{
    MATRIX matrix = identity_matrix(0);
    VECTOR position = {0, 0, 1000, 0};
    PCB pcb;

    memset(&pcb, 0, sizeof(pcb));
    memset(&gSceneGeometryEnv, 0, sizeof(gSceneGeometryEnv));
    pcb.pcb_Pos = &position;
    pcb.pcb_fRate = 0x1000;
    pcb.pcb_Bits = 1;
    pcb.pcb_Particle[0].pos.vz = 1000;
    pcb.pcb_Particle[0].vx = 2;
    pcb.pcb_Particle[0].vy = 3;
    pcb.pcb_Particle[0].vz = 4;

    _RenderParticle(&matrix, &pcb);
    assert(pcb.pcb_Interp == 0x20);
    assert(pcb.pcb_fLaunch == 0);
    assert(pcb.pcb_Bits == 0x100);
    assert(pcb.pcb_Particle[0].pos.vx == 2);
    assert(pcb.pcb_Particle[0].pos.vy == 3);
    assert(pcb.pcb_Particle[0].pos.vz == 1004);
}

static void test_map_renderers_follow_scratch_pointers(void)
{
    WorldData world;
    wsl_BAP_TEXTURE textures[1];
    wsl_libPart part;
    wsl_libPart *parts[1] = {&part};
    int32_t indices[1] = {0x00020100};
    int16_t shared[3] = {0, 0x20, 1};
    uint8_t face_colors[4] = {0, 0, 1, 2};
    CVECTOR palette[3] = {
        {0x10, 0x20, 0x30, 0},
        {0x40, 0x50, 0x60, 0},
        {0x70, 0x80, 0x90, 0}};
    wsl_mapEntry map_entry;
    wsl_thinPoly thin;
    MATRIX matrix = identity_matrix(0);
    char colors[3] = {0, 1, 2};

    memset(&world, 0, sizeof(world));
    memset(textures, 0, sizeof(textures));
    memset(&part, 0, sizeof(part));
    memset(&map_entry, 0, sizeof(map_entry));
    memset(&thin, 0, sizeof(thin));
    memset(gaScratch, 0, 2048);
    world.pTexture = textures;
    world.pLib = parts;
    gpWorld = &world;
    set_scratch_i16(8, 0);
    set_scratch_i16(12, 0);
    set_scratch_i16(16, 1000);
    set_scratch_pointer(0x50, face_colors);
    set_scratch_pointer(0x90, palette);

    part.index = indices;
    part.shared = shared;
    part.numverts = 1;
    part.numpolys = 1;
    part.polys[0].textureID = 0;
    map_entry.libPart = 0;
    assert(_CubeRender(&map_entry, &matrix) == 1);

    thin.textureID = 0;
    thin.verts[0] = 0;
    thin.verts[1] = 0x20;
    thin.verts[2] = 1;
    assert(_ThinRender(&thin, &matrix, colors) == 1);
    gpWorld = NULL;
}

int main(void)
{
    test_perspective_transform();
    test_cull_polarity_and_bounds();
    test_render_node_expands_packed_vertices();
    test_particle_fixed_point_activation();
    test_map_renderers_follow_scratch_pointers();
    puts("wRender tests passed");
    return 0;
}
