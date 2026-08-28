/*
 * PC importer for the exact ufbx 0.6.1 level path used by game.exe.
 * ufbx stops at this adapter: the runtime and software renderer consume the
 * dependency-free JPBSoftwareLevelMesh triangle/material representation.
 */

#include "pc_level_fbx.h"

#include "jpb/level_world.h"
#include "jpb/transparent_texture_database.h"
#include "jpb/whook.h"

#include "ufbx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(
    UFBX_HEADER_VERSION == 6001u,
    "the PC level importer requires the matched ufbx 0.6.1 source");

static void pc_fbx_error(
    char *text, size_t capacity, const char *message)
{
    if (text != NULL && capacity != 0) {
        snprintf(text, capacity, "%s", message != NULL ? message : "");
    }
}

static ufbx_string pc_fbx_texture_filename(
    const ufbx_material *material)
{
    static const ufbx_string empty = {NULL, 0};
    const ufbx_texture *texture;

    if (material == NULL || material->textures.count == 0) {
        return empty;
    }
    texture = material->textures.data[0].texture;
    if (texture == NULL) {
        return empty;
    }
    return texture->relative_filename.length != 0
        ? texture->relative_filename
        : texture->filename;
}

static void pc_fbx_copy_basename(
    char *destination,
    size_t capacity,
    const char *path,
    size_t length)
{
    size_t base = 0;
    size_t index;

    if (capacity == 0) {
        return;
    }
    if (path == NULL) {
        destination[0] = '\0';
        return;
    }
    for (index = 0; index < length; ++index) {
        if (path[index] == '/' || path[index] == '\\') {
            base = index + 1;
        }
    }
    length -= base;
    if (length >= capacity) {
        length = capacity - 1;
    }
    memcpy(destination, path + base, length);
    destination[length] = '\0';
}

static void pc_fbx_copy_string(
    char *destination,
    size_t capacity,
    ufbx_string source)
{
    size_t length = source.length;

    if (capacity == 0) {
        return;
    }
    if (source.data == NULL) {
        destination[0] = '\0';
        return;
    }
    if (length >= capacity) {
        length = capacity - 1;
    }
    memcpy(destination, source.data, length);
    destination[length] = '\0';
}

static JPBLevelFbxMeshPass pc_fbx_material_pass(
    const char *texture_name, int level_index)
{
    if (!jpb_IsTextureTransparent(texture_name, level_index)) {
        return JPB_LEVEL_FBX_PASS_OPAQUE;
    }
    return jpb_IsGlassTexture(texture_name)
        ? JPB_LEVEL_FBX_PASS_GLASS
        : JPB_LEVEL_FBX_PASS_TRANSPARENT;
}

static int pc_fbx_material_vertex_count(
    const ufbx_mesh *mesh,
    const ufbx_mesh_material *material,
    size_t *vertex_count)
{
    size_t count = 0;
    size_t face_list_index;

    for (face_list_index = 0;
         face_list_index < material->face_indices.count;
         ++face_list_index) {
        uint32_t face_index =
            material->face_indices.data[face_list_index];
        ufbx_face face = mesh->faces.data[face_index];

        if (face.num_indices > SIZE_MAX - count) return 0;
        count += face.num_indices;
    }
    /* DrawIndexedInstanced consumes the authored stream as a triangle list. */
    *vertex_count = count - count % 3;
    return 1;
}

static int pc_fbx_count_scene(
    const ufbx_scene *scene,
    int level_index,
    size_t *batch_count,
    size_t *vertex_count,
    size_t pass_batch_count[3],
    size_t *mesh_count)
{
    size_t node_index;

    *batch_count = 0;
    *vertex_count = 0;
    *mesh_count = 0;
    memset(pass_batch_count, 0, 3 * sizeof(*pass_batch_count));
    for (node_index = 0;
         node_index < scene->nodes.count;
         ++node_index) {
        const ufbx_mesh *mesh = scene->nodes.data[node_index]->mesh;
        size_t material_index;

        if (mesh == NULL) {
            continue;
        }
        ++*mesh_count;
        for (material_index = 0;
             material_index < mesh->materials.count;
             ++material_index) {
            const ufbx_mesh_material *material =
                &mesh->materials.data[material_index];
            size_t material_vertex_count = 0;
            ufbx_string texture;
            char texture_name[256];
            JPBLevelFbxMeshPass pass;

            if (!pc_fbx_material_vertex_count(
                    mesh, material, &material_vertex_count)) {
                return 0;
            }
            if (material_vertex_count == 0) {
                continue;
            }
            if (material_vertex_count > SIZE_MAX - *vertex_count) {
                return 0;
            }
            texture = pc_fbx_texture_filename(material->material);
            pc_fbx_copy_basename(
                texture_name, sizeof(texture_name),
                texture.data, texture.length);
            pass = pc_fbx_material_pass(texture_name, level_index);
            ++pass_batch_count[pass];
            ++*batch_count;
            *vertex_count += material_vertex_count;
        }
    }
    return *batch_count != 0 && *vertex_count != 0;
}

static int pc_fbx_write_vertex(
    const ufbx_mesh *mesh,
    uint32_t index,
    int level_index,
    float uv_scroll_u,
    float uv_scroll_v,
    JPBSoftwareLevelVertex *destination)
{
    ufbx_vec3 local =
        ufbx_get_vertex_vec3(&mesh->vertex_position, index);
    ufbx_vec2 uv = {0.0, 0.0};
    ufbx_vec4 color = {0.0, 0.0, 0.0, 0.0};

    /*
     * CD3DApplication::InitFBXLevelData copies vertex_position.values to the
     * GPU buffer. It stores geometry_to_world, but the three DrawLevel passes
     * use one global WVP and never multiply that per-node matrix.
     */
    if (!jpb_LevelTransformFbxVertex(
            level_index,
            (float)local.x,
            (float)local.y,
            (float)local.z,
            &destination->position)) {
        return 0;
    }
    if (mesh->vertex_uv.exists) {
        uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, index);
    }
    if (mesh->vertex_color.exists) {
        color = ufbx_get_vertex_vec4(&mesh->vertex_color, index);
    }
    destination->u = (float)uv.x;
    /* LevelVertexShader.hlsl converts FBX's bottom-left V to texture V. */
    destination->v = 1.0f - (float)uv.y;
    destination->red = (float)(color.x * 255.0);
    destination->green = (float)(color.y * 255.0);
    destination->blue = (float)(color.z * 255.0);
    destination->alpha = (float)(color.w * 255.0);
    destination->uvScrollU = uv_scroll_u;
    destination->uvScrollV = uv_scroll_v;
    return 1;
}

/*
 * Split portable body for PDB procedure el_chavo::InitFBXLevelData. The
 * renderer-owned allocations differ, while scene traversal, vertex payload,
 * pass partitioning, and draw-order metadata follow the shipped method.
 */
static int InitFBXLevelData(
    ufbx_scene *scene,
    int level_index,
    JPBPcFbxLevel *level,
    char *error_text,
    size_t error_text_capacity)
{
    size_t batch_count;
    size_t vertex_count;
    size_t pass_batch_count[3];
    size_t pass_batch_index[3] = {0, 0, 0};
    size_t mesh_count;
    size_t node_index;
    size_t mesh_index = 0;
    size_t batch_index = 0;
    size_t vertex_index = 0;

    if (scene == NULL || level == NULL ||
        level_index < 0 || level_index >= JPB_LEVEL_COUNT) {
        pc_fbx_error(error_text, error_text_capacity, "invalid FBX level arguments");
        return 0;
    }
    memset(level, 0, sizeof(*level));
    if (!pc_fbx_count_scene(
            scene,
            level_index,
            &batch_count,
            &vertex_count,
            pass_batch_count,
            &mesh_count)) {
        pc_fbx_error(error_text, error_text_capacity, "FBX level contains no render triangles");
        return 0;
    }
    level->batches = (JPBSoftwareLevelBatch *)calloc(
        batch_count, sizeof(*level->batches));
    level->vertices = (JPBSoftwareLevelVertex *)calloc(
        vertex_count, sizeof(*level->vertices));
    level->textureNames = (char (*)[256])calloc(
        batch_count, sizeof(*level->textureNames));
    level->meshNames = (char (*)[128])calloc(
        batch_count, sizeof(*level->meshNames));
    if (level->batches == NULL || level->vertices == NULL ||
        level->textureNames == NULL || level->meshNames == NULL) {
        pc_fbx_error(error_text, error_text_capacity, "out of memory importing FBX level");
        jpb_PCFreeFbxLevel(level);
        return 0;
    }

    for (node_index = 0;
         node_index < scene->nodes.count;
         ++node_index) {
        const ufbx_node *node = scene->nodes.data[node_index];
        const ufbx_mesh *mesh = node->mesh;
        size_t material_index;

        if (mesh == NULL) {
            continue;
        }
        for (material_index = 0;
             material_index < mesh->materials.count;
             ++material_index) {
            const ufbx_mesh_material *material =
                &mesh->materials.data[material_index];
            JPBSoftwareLevelBatch *batch;
            ufbx_string texture;
            JPBLevelFbxMeshPass pass;
            size_t face_list_index;
            size_t first_vertex = vertex_index;
            size_t material_vertex_count;
            float uv_scroll_u;
            float uv_scroll_v;

            if (!pc_fbx_material_vertex_count(
                    mesh, material, &material_vertex_count)) {
                pc_fbx_error(error_text, error_text_capacity, "invalid FBX material index stream");
                jpb_PCFreeFbxLevel(level);
                return 0;
            }
            if (material_vertex_count == 0) {
                continue;
            }
            texture = pc_fbx_texture_filename(material->material);
            pc_fbx_copy_basename(
                level->textureNames[batch_index],
                sizeof(level->textureNames[batch_index]),
                texture.data,
                texture.length);
            pc_fbx_copy_string(
                level->meshNames[batch_index],
                sizeof(level->meshNames[batch_index]),
                node->name);
            pass = pc_fbx_material_pass(
                level->textureNames[batch_index], level_index);
            jpb_LevelFbxUvScroll(
                level_index,
                level->textureNames[batch_index],
                &uv_scroll_u,
                &uv_scroll_v);

            for (face_list_index = 0;
                 face_list_index < material->face_indices.count;
                 ++face_list_index) {
                uint32_t face_index =
                    material->face_indices.data[face_list_index];
                ufbx_face face = mesh->faces.data[face_index];
                uint32_t corner;

                for (corner = 0; corner < face.num_indices; ++corner) {
                    if (vertex_index - first_vertex >=
                        material_vertex_count) {
                        break;
                    }
                    if (vertex_index >= vertex_count ||
                        !pc_fbx_write_vertex(
                            mesh,
                            face.index_begin + corner,
                            level_index,
                            uv_scroll_u,
                            uv_scroll_v,
                            &level->vertices[vertex_index++])) {
                        pc_fbx_error(error_text, error_text_capacity, "invalid FBX vertex data");
                        jpb_PCFreeFbxLevel(level);
                        return 0;
                    }
                }
            }
            batch = &level->batches[batch_index];
            batch->vertices = &level->vertices[first_vertex];
            batch->vertexCount = vertex_index - first_vertex;
            batch->textureName = level->textureNames[batch_index];
            batch->meshName = level->meshNames[batch_index];
            batch->pass = pass;
            if (pass == JPB_LEVEL_FBX_PASS_OPAQUE) {
                batch->meshIndex = mesh_index;
                batch->meshCount = mesh_count;
            } else {
                batch->meshIndex = pass_batch_index[pass]++;
                batch->meshCount = pass_batch_count[pass];
            }
            ++batch_index;
        }
        ++mesh_index;
    }
    if (batch_index != batch_count || vertex_index != vertex_count) {
        pc_fbx_error(error_text, error_text_capacity, "FBX triangle count changed during import");
        jpb_PCFreeFbxLevel(level);
        return 0;
    }
    level->mesh.batches = level->batches;
    level->mesh.batchCount = batch_count;
    level->mesh.levelIndex = level_index;
    level->mesh.vertices = vertex_count;
    level->mesh.triangles = vertex_count / 3;
    pc_fbx_error(error_text, error_text_capacity, "");
    return 1;
}

typedef struct PcFbxImportContext {
    int levelIndex;
    JPBPcFbxLevel *level;
    char *errorText;
    size_t errorTextCapacity;
    int result;
} PcFbxImportContext;

static PcFbxImportContext pc_fbx_import_context;
static int pc_fbx_import_active;

static void pc_fbx_import_level_hook(
    void *user_data, ufbx_scene *scene)
{
    PcFbxImportContext *context = (PcFbxImportContext *)user_data;

    context->result = InitFBXLevelData(
        scene,
        context->levelIndex,
        context->level,
        context->errorText,
        context->errorTextCapacity);
}

int jpb_PCBeginFbxLevelImport(
    int level_index,
    JPBPcFbxLevel *level,
    char *error_text,
    size_t error_text_capacity)
{
    if (pc_fbx_import_active || level == NULL ||
        level_index < 0 || level_index >= JPB_LEVEL_COUNT) {
        pc_fbx_error(
            error_text, error_text_capacity,
            "invalid FBX level import target");
        return 0;
    }
    memset(&pc_fbx_import_context, 0, sizeof(pc_fbx_import_context));
    pc_fbx_import_context.levelIndex = level_index;
    pc_fbx_import_context.level = level;
    pc_fbx_import_context.errorText = error_text;
    pc_fbx_import_context.errorTextCapacity = error_text_capacity;
    pc_fbx_import_active = 1;
    jpb_WHookSetInitFBXLevelDataHook(
        pc_fbx_import_level_hook, &pc_fbx_import_context);
    return 1;
}

int jpb_PCEndFbxLevelImport(void)
{
    int result;

    if (!pc_fbx_import_active) {
        return 0;
    }
    jpb_WHookSetInitFBXLevelDataHook(NULL, NULL);
    result = pc_fbx_import_context.result;
    memset(&pc_fbx_import_context, 0, sizeof(pc_fbx_import_context));
    pc_fbx_import_active = 0;
    return result;
}

int jpb_PCLoadFbxLevel(
    const char *path,
    int level_index,
    JPBPcFbxLevel *level,
    char *error_text,
    size_t error_text_capacity)
{
    ufbx_error error;
    ufbx_load_opts load_opts;
    ufbx_scene *scene;

    if (path == NULL || level == NULL ||
        level_index < 0 || level_index >= JPB_LEVEL_COUNT) {
        pc_fbx_error(error_text, error_text_capacity, "invalid FBX level arguments");
        return 0;
    }
    memset(&error, 0, sizeof(error));
    memset(&load_opts, 0, sizeof(load_opts));
    /* Exact loader_LevelLoad stores at matched-PC RVA 0xBC8F2..0xBC925. */
    load_opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
    load_opts.target_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
    load_opts.target_axes.front = UFBX_COORDINATE_AXIS_NEGATIVE_Z;
    load_opts.target_unit_meters = 1.0f;
    scene = ufbx_load_file(path, &load_opts, &error);
    if (scene == NULL) {
        if (error_text != NULL && error_text_capacity != 0) {
            snprintf(
                error_text,
                error_text_capacity,
                "ufbx load failed: %.*s",
                (int)error.description.length,
                error.description.data != NULL
                    ? error.description.data : "unknown error");
        }
        return 0;
    }

    if (!jpb_PCBeginFbxLevelImport(
            level_index, level, error_text, error_text_capacity)) {
        ufbx_free_scene(scene);
        return 0;
    }
    _InitFBXLevelData(scene);
    ufbx_free_scene(scene);
    return jpb_PCEndFbxLevelImport();
}

void jpb_PCFreeFbxLevel(JPBPcFbxLevel *level)
{
    if (level == NULL) {
        return;
    }
    free(level->meshNames);
    free(level->textureNames);
    free(level->vertices);
    free(level->batches);
    memset(level, 0, sizeof(*level));
}
