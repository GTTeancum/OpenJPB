/*
 * Executable/PDB-owned level placement data.
 *
 * These globals have exact PDB names and types. Their global-stream records
 * do not identify an owning object module, so they are kept together here
 * instead of inventing source ownership. The descriptive jpb_ helpers are a
 * portable extraction boundary, not claimed original symbols.
 */

#include "jpb/level_world.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

char *sLevelNames[JPB_LEVEL_NAME_COUNT] = {
    "council", "fed",    "marsh",  "theed",  "palace", "tato",
    "corus1",  "ruins",  "streets","hangar", "core",   "mini1",
    "mini2",   "mini3",  "mini4",  "corus2", "train1", "train2",
    "train3",  "train5", "train6", "train7", "train4", "train1",
    "train1",  "arena",  "derek",  "june"
};

float level_offset[JPB_LEVEL_COUNT][3] = {
    {   0.0f,   0.0f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   0.0f,  -0.5f,   1.0f },
    {   1.0f,  -0.5f,   0.0f },
    {  97.6f,  17.7f, -13.15f },
    {   1.0f,  -0.5f,   0.0f },
    { 128.5f,  61.7f,   2.5f },
    {-224.3f,   9.2f,   5.6f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    { 105.5f,  15.15f, -8.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f },
    {   1.0f,  -0.5f,   0.0f }
};

float level_scale[JPB_LEVEL_COUNT][3] = {
    {   0.0f,   0.0f,   0.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 283.4f, 283.4f, 283.4f },
    { 256.0f, 256.0f, 256.0f },
    {-256.0f,-256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f,-256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f },
    { 256.0f, 256.0f, 256.0f }
};

_svector startPos[JPB_LEVEL_COUNT][2] = {
    {{ 52,  50, 11, 0}, { 51,  49, 11, 0}},
    {{ 40,  76, 13, 0}, { 42,  76, 13, 0}},
    {{ 18,  99, 17, 0}, { 20,  99, 17, 0}},
    {{ 15,  28, 11, 0}, { 15,  29, 11, 0}},
    {{ 36,  69, 13, 0}, { 36,  68, 13, 0}},
    {{ 16,  28, 16, 0}, { 17,  29, 16, 0}},
    {{ 28,  50, 30, 0}, { 28,  49, 30, 0}},
    {{ 49,  11, 16, 0}, { 48,  11, 16, 0}},
    {{  7,  31, 13, 0}, {  7,  33, 13, 0}},
    {{ 77, 113, 30, 0}, { 78, 113, 30, 0}},
    {{ 27, 102,  8, 0}, { 28, 102,  8, 0}},
    {{ 38, 146, 15, 0}, { 39, 146, 15, 0}},
    {{ 44, 222, 18, 0}, { 44, 222, 19, 0}},
    {{ 77,  69, 52, 0}, { 79,  69, 52, 0}},
    {{ 69,  50, 11, 0}, { 68,  50, 11, 0}},
    {{235, 143, 43, 0}, {235, 144, 43, 0}},
    {{112,  46, 11, 0}, {112,  46, 11, 0}},
    {{109,  46, 11, 0}, {109,  46, 11, 0}},
    {{124,  52, 19, 0}, {124,  52, 19, 0}},
    {{104,  50, 11, 0}, {104,  50, 11, 0}},
    {{ 98,  50, 11, 0}, { 98,  50, 11, 0}},
    {{148,  73, 14, 0}, {148,  73, 14, 0}},
    {{134,  50, 11, 0}, {134,  50, 11, 0}},
    {{ 98,  54, 11, 0}, { 98,  54, 11, 0}},
    {{ 98,  54, 11, 0}, { 98,  54, 11, 0}},
    {{106,  21, 16, 0}, {106,  17, 16, 0}}
};

static int level_world_ascii_lower(int value)
{
    if (value >= 'A' && value <= 'Z') {
        return value + ('a' - 'A');
    }
    return value;
}

static int level_world_component_equal(
    const char *component, size_t length, const char *level_name)
{
    size_t index;

    for (index = 0; index < length && level_name[index] != '\0'; ++index) {
        if (level_world_ascii_lower((unsigned char)component[index]) !=
            level_world_ascii_lower((unsigned char)level_name[index])) {
            return 0;
        }
    }
    return index == length && level_name[index] == '\0';
}

int jpb_LevelIndexFromPath(const char *path)
{
    const char *component;
    const char *cursor;
    int match = JPB_LEVEL_INDEX_NONE;

    if (path == NULL) {
        return JPB_LEVEL_INDEX_NONE;
    }
    component = path;
    cursor = path;
    for (;;) {
        int at_separator =
            *cursor == '/' || *cursor == '\\' || *cursor == '\0';

        if (at_separator && cursor != component) {
            size_t length = (size_t)(cursor - component);
            size_t stem_length = length;
            size_t index;

            while (stem_length != 0 &&
                   component[stem_length - 1] != '.') {
                --stem_length;
            }
            if (stem_length != 0) {
                --stem_length;
            } else {
                stem_length = length;
            }
            for (index = 0; index < JPB_LEVEL_NAME_COUNT; ++index) {
                if (level_world_component_equal(
                        component, stem_length, sLevelNames[index])) {
                    match = (int)index;
                    break;
                }
            }
        }
        if (*cursor == '\0') {
            break;
        }
        if (at_separator) {
            component = cursor + 1;
        }
        ++cursor;
    }
    if (match >= JPB_LEVEL_COUNT) {
        return JPB_LEVEL_INDEX_NONE;
    }
    return match;
}

int jpb_LevelTransformFbxVertex(
    int level_index,
    float fbx_x,
    float fbx_y,
    float fbx_z,
    FVECTOR *game_vertex)
{
    if (level_index < 0 || level_index >= JPB_LEVEL_COUNT ||
        game_vertex == NULL) {
        return 0;
    }

    /*
     * cube_NewWorldRender converts the game's column-vector MATRIX into the
     * row-vector D3D matrix by selecting columns [-x, z, y]. Its translation
     * combines level_offset with exact constants 256, 512, and 256.
     * LevelVertexShader.hlsl applies level_scale before that matrix.
     */
    game_vertex->vx =
        -fbx_x * level_scale[level_index][0] +
        level_offset[level_index][0] * 256.0f;
    game_vertex->vy =
        fbx_z * level_scale[level_index][2] -
        level_offset[level_index][2] * 256.0f;
    game_vertex->vz =
        fbx_y * level_scale[level_index][1] -
        level_offset[level_index][1] * 512.0f;
    return 1;
}

void jpb_LevelFbxUvScroll(
    int level_index,
    const char *texture_name,
    float *scroll_u,
    float *scroll_v)
{
    float u = 0.0f;
    float v = 0.0f;

    if (texture_name != NULL) {
        if (level_index == 1 && strcmp(texture_name, "belt.bmp") == 0) {
            u = 0.082f;
        } else if (level_index == 9 &&
                   strcmp(texture_name, "t_water.tga") == 0) {
            u = 0.001f;
            v = 0.001f;
        } else if (level_index == 9 &&
                   strcmp(texture_name, "t_waterStatic.tga") == 0) {
            v = -0.1f;
        }
    }
    if (scroll_u != NULL) *scroll_u = u;
    if (scroll_v != NULL) *scroll_v = v;
}

int jpb_StreetsCullMeshIndexFromName(const char *mesh_name)
{
    const char *cursor;
    size_t length;
    int value = 0;

    if (mesh_name == NULL || strlen(mesh_name) < 4) {
        return JPB_LEVEL_INDEX_NONE;
    }

    /* The reference calls the CRT atoi public at RVA 0x20F65C. */
    cursor = mesh_name + 4;
    while (*cursor == ' ' || (*cursor >= '\t' && *cursor <= '\r')) {
        ++cursor;
    }
    if (*cursor == '+') {
        ++cursor;
    } else if (*cursor == '-') {
        /* No shipped Streets mesh uses a negative identifier. */
        return JPB_LEVEL_INDEX_NONE;
    }
    while (*cursor >= '0' && *cursor <= '9') {
        int digit = *cursor - '0';

        if (value > (INT_MAX - digit) / 10) {
            return JPB_LEVEL_INDEX_NONE;
        }
        value = value * 10 + digit;
        ++cursor;
    }

    length = strlen(mesh_name);
    if (length == 12) {
        if (value > INT_MAX / 2) {
            return JPB_LEVEL_INDEX_NONE;
        }
        value *= 2;
    } else if (length == 13) {
        if (value > INT_MAX / 2) {
            return JPB_LEVEL_INDEX_NONE;
        }
        value = value * 2 - 1;
    }

    if (value < 0 || value >= JPB_CULL_MESH_COUNT) {
        return JPB_LEVEL_INDEX_NONE;
    }
    return value;
}

int jpb_ShouldDrawFbxMesh(
    int level_index,
    JPBLevelFbxMeshPass pass,
    size_t mesh_count,
    size_t mesh_index,
    const char *mesh_name)
{
    int cull_index;

    if (pass < JPB_LEVEL_FBX_PASS_OPAQUE ||
        pass > JPB_LEVEL_FBX_PASS_GLASS ||
        mesh_index >= mesh_count) {
        return 0;
    }
    if (level_index == 8) {
        cull_index = jpb_StreetsCullMeshIndexFromName(mesh_name);
        return cull_index != JPB_LEVEL_INDEX_NONE &&
               cullmesh[cull_index] == 1;
    }
    if (pass != JPB_LEVEL_FBX_PASS_OPAQUE ||
        mesh_count > JPB_CULL_MESH_COUNT - 1) {
        return 1;
    }
    return cullmesh[mesh_index] == 1;
}
