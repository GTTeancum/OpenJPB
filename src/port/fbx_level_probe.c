/*
 * Evidence probe for the live FBX level path in the matched PC build.
 *
 * game.exe publishes ufbx_source_version == 6001, so this probe deliberately
 * accepts only ufbx 0.6.1. It mirrors InitFBXLevelData's scene-node ordering
 * and material partition without making ufbx a game/runtime dependency.
 */

#include "jpb/transparent_texture_database.h"
#include "jpb/level_world.h"

#include "ufbx.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ProbeBounds {
    double minX;
    double minY;
    double minZ;
    double maxX;
    double maxY;
    double maxZ;
    int valid;
} ProbeBounds;

typedef struct ProbeAlphaRange {
    double minimum;
    double maximum;
    double total;
    size_t samples;
} ProbeAlphaRange;

_Static_assert(
    UFBX_HEADER_VERSION == 6001u,
    "jpb_fbx_level_probe requires the matched ufbx 0.6.1 header");

static void probe_bounds_add(ProbeBounds *bounds, ufbx_vec3 position)
{
    if (!bounds->valid) {
        bounds->minX = bounds->maxX = position.x;
        bounds->minY = bounds->maxY = position.y;
        bounds->minZ = bounds->maxZ = position.z;
        bounds->valid = 1;
        return;
    }
    if (position.x < bounds->minX) bounds->minX = position.x;
    if (position.y < bounds->minY) bounds->minY = position.y;
    if (position.z < bounds->minZ) bounds->minZ = position.z;
    if (position.x > bounds->maxX) bounds->maxX = position.x;
    if (position.y > bounds->maxY) bounds->maxY = position.y;
    if (position.z > bounds->maxZ) bounds->maxZ = position.z;
}

static void probe_face_bounds(
    const ufbx_node *node,
    const ufbx_mesh *mesh,
    const ufbx_mesh_material *mesh_material,
    ProbeBounds *bounds,
    ProbeBounds *material_game_bounds,
    ProbeBounds *game_bounds,
    int level_index,
    int reference_space)
{
    size_t face_list_index;

    for (face_list_index = 0;
         face_list_index < mesh_material->face_indices.count;
         ++face_list_index) {
        uint32_t face_index =
            mesh_material->face_indices.data[face_list_index];
        ufbx_face face = mesh->faces.data[face_index];
        uint32_t vertex_offset;

        for (vertex_offset = 0;
             vertex_offset < face.num_indices;
             ++vertex_offset) {
            uint32_t index = face.index_begin + vertex_offset;
            ufbx_vec3 local =
                ufbx_get_vertex_vec3(&mesh->vertex_position, index);
            ufbx_vec3 world = ufbx_transform_position(
                &node->geometry_to_world, local);
            FVECTOR game;
            ufbx_vec3 game_position;

            probe_bounds_add(bounds, world);
            if (jpb_LevelTransformFbxVertex(
                    level_index,
                    reference_space ? (float)local.x : (float)(world.x / 2.54),
                    reference_space ? (float)local.y : (float)(-world.z / 2.54),
                    reference_space ? (float)local.z : (float)(world.y / 2.54),
                    &game)) {
                game_position.x = game.vx;
                game_position.y = game.vy;
                game_position.z = game.vz;
                probe_bounds_add(material_game_bounds, game_position);
                probe_bounds_add(game_bounds, game_position);
            }
        }
    }
}

static ProbeAlphaRange probe_face_alpha(
    const ufbx_mesh *mesh,
    const ufbx_mesh_material *mesh_material)
{
    ProbeAlphaRange range = {DBL_MAX, -DBL_MAX, 0.0, 0};
    size_t face_list_index;

    if (!mesh->vertex_color.exists) {
        return range;
    }
    for (face_list_index = 0;
         face_list_index < mesh_material->face_indices.count;
         ++face_list_index) {
        uint32_t face_index =
            mesh_material->face_indices.data[face_list_index];
        ufbx_face face = mesh->faces.data[face_index];
        uint32_t vertex_offset;

        for (vertex_offset = 0;
             vertex_offset < face.num_indices;
             ++vertex_offset) {
            uint32_t index = face.index_begin + vertex_offset;
            double alpha =
                ufbx_get_vertex_vec4(&mesh->vertex_color, index).w;

            if (alpha < range.minimum) range.minimum = alpha;
            if (alpha > range.maximum) range.maximum = alpha;
            range.total += alpha;
            ++range.samples;
        }
    }
    return range;
}

static ufbx_string probe_texture_filename(const ufbx_material *material)
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
    if (texture->relative_filename.length != 0) {
        return texture->relative_filename;
    }
    return texture->filename;
}

static const char *probe_basename(const char *path, size_t length)
{
    size_t index;
    const char *base = path;

    for (index = 0; index < length; ++index) {
        if (path[index] == '/' || path[index] == '\\') {
            base = path + index + 1;
        }
    }
    return base;
}

static size_t probe_basename_length(
    const char *path, size_t length, const char *base)
{
    return length - (size_t)(base - path);
}

static const char *probe_pass_name(int transparent, int glass)
{
    if (glass) return "glass";
    if (transparent) return "transparent";
    return "opaque";
}

static void probe_dump_dynamic_triangles(
    size_t scene_node_index,
    const ufbx_node *node,
    const ufbx_mesh *mesh,
    const ufbx_mesh_material *mesh_material,
    const char *pass_name,
    int include_base_mesh)
{
    size_t face_list_index;

    if (!include_base_mesh && scene_node_index <= 1) {
        return;
    }
    for (face_list_index = 0;
         face_list_index < mesh_material->face_indices.count;
         ++face_list_index) {
        uint32_t face_index =
            mesh_material->face_indices.data[face_list_index];
        ufbx_face face = mesh->faces.data[face_index];
        ufbx_vec3 world[3];
        uint32_t vertex_offset;

        if (face.num_indices != 3) {
            continue;
        }
        for (vertex_offset = 0; vertex_offset < 3; ++vertex_offset) {
            uint32_t index = face.index_begin + vertex_offset;
            ufbx_vec3 local =
                ufbx_get_vertex_vec3(&mesh->vertex_position, index);
            world[vertex_offset] = ufbx_transform_position(
                &node->geometry_to_world, local);
        }
        /*
         * The legacy JPX mirror stores the same authored geometry in inches
         * and flips authoring Z into its horizontal Z field. The FBX scene
         * publishes centimeters, hence the exact 2.54 conversion.
         */
        printf(
            "    triangle node=%zu pass=%s "
            "p=%.9f,%.9f,%.9f;%.9f,%.9f,%.9f;%.9f,%.9f,%.9f\n",
            scene_node_index,
            pass_name,
            world[0].x / 2.54,
            -world[0].z / 2.54,
            world[0].y / 2.54,
            world[1].x / 2.54,
            -world[1].z / 2.54,
            world[1].y / 2.54,
            world[2].x / 2.54,
            -world[2].z / 2.54,
            world[2].y / 2.54);
    }
}

int main(int argc, char **argv)
{
    ufbx_error error;
    ufbx_scene *scene;
    size_t scene_node_index;
    size_t opaque_mesh_index = 0;
    size_t transparent_mesh_index = 0;
    size_t glass_mesh_index = 0;
    int level_index;
    int dump_triangle_mode = 0;
    int reference_space = 0;
    ProbeBounds game_bounds = {0};
    ufbx_load_opts load_opts;

    if (argc != 3 && argc != 4) {
        fprintf(
            stderr,
            "usage: %s <level-index> <level.fbx> "
            "[--dynamic-triangles|--all-triangles|--reference-space]\n",
            argv[0]);
        return 2;
    }
    if (argc == 4) {
        if (strcmp(argv[3], "--reference-space") == 0) {
            reference_space = 1;
        } else if (strcmp(argv[3], "--dynamic-triangles") != 0) {
            if (strcmp(argv[3], "--all-triangles") != 0) {
                fputs("unknown option\n", stderr);
                return 2;
            }
            dump_triangle_mode = 2;
        } else {
            dump_triangle_mode = 1;
        }
    }
    level_index = atoi(argv[1]);
    if (level_index < 0 || level_index >= 26) {
        fputs("level index must be in the range 0..25\n", stderr);
        return 2;
    }

    memset(&error, 0, sizeof(error));
    memset(&load_opts, 0, sizeof(load_opts));
    if (reference_space) {
        /* Exact loader_LevelLoad options recovered at matched-PC RVA
         * 0xBC560: right +X, up +Y, front -Z, one output unit per meter. */
        load_opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
        load_opts.target_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
        load_opts.target_axes.front = UFBX_COORDINATE_AXIS_NEGATIVE_Z;
        load_opts.target_unit_meters = 1.0f;
    }
    scene = ufbx_load_file(
        argv[2], reference_space ? &load_opts : NULL, &error);
    if (scene == NULL) {
        fprintf(
            stderr,
            "FBX load failed: %.*s\n",
            (int)error.description.length,
            error.description.data != NULL ? error.description.data : "");
        return 3;
    }

    printf(
        "ufbx=%u nodes=%zu meshes=%zu materials=%zu level=%d "
        "unit_meters=%.9g axes=%d,%d,%d\n",
        (unsigned)ufbx_source_version,
        scene->nodes.count,
        scene->meshes.count,
        scene->materials.count,
        level_index,
        (double)scene->settings.unit_meters,
        (int)scene->settings.axes.right,
        (int)scene->settings.axes.up,
        (int)scene->settings.axes.front);
    for (scene_node_index = 0;
         scene_node_index < scene->nodes.count;
         ++scene_node_index) {
        const ufbx_node *node = scene->nodes.data[scene_node_index];
        const ufbx_mesh *mesh = node->mesh;
        size_t material_index;
        size_t opaque_count = 0;
        size_t transparent_count = 0;
        size_t glass_count = 0;

        if (mesh == NULL) {
            continue;
        }
        for (material_index = 0;
             material_index < mesh->materials.count;
             ++material_index) {
            const ufbx_mesh_material *mesh_material =
                &mesh->materials.data[material_index];
            ufbx_string texture =
                probe_texture_filename(mesh_material->material);
            const char *base = probe_basename(
                texture.data != NULL ? texture.data : "",
                texture.length);
            size_t base_length = probe_basename_length(
                texture.data != NULL ? texture.data : "",
                texture.length,
                base);
            int transparent = jpb_IsTextureTransparent(
                base, level_index);
            int glass = transparent && jpb_IsGlassTexture(base);

            if (mesh_material->num_faces == 0) {
                continue;
            }
            if (glass) {
                ++glass_count;
            } else if (transparent) {
                ++transparent_count;
            } else {
                ++opaque_count;
            }
        }

        printf(
            "node[%zu] opaque_mesh=%zu transparent_start=%lld "
            "glass_start=%lld node_name=%.*s mesh_name=%.*s "
            "vertices=%zu faces=%zu triangles=%zu materials=%zu "
            "passes=%zu/%zu/%zu "
            "geometry_to_world="
            "%.9g,%.9g,%.9g,%.9g/"
            "%.9g,%.9g,%.9g,%.9g/"
            "%.9g,%.9g,%.9g,%.9g\n",
            scene_node_index,
            opaque_mesh_index,
            transparent_count != 0
                ? (long long)transparent_mesh_index : -1ll,
            glass_count != 0 ? (long long)glass_mesh_index : -1ll,
            (int)node->name.length,
            node->name.data,
            (int)mesh->name.length,
            mesh->name.data,
            mesh->num_vertices,
            mesh->num_faces,
            mesh->num_triangles,
            mesh->materials.count,
            opaque_count,
            transparent_count,
            glass_count,
            node->geometry_to_world.m00,
            node->geometry_to_world.m01,
            node->geometry_to_world.m02,
            node->geometry_to_world.m03,
            node->geometry_to_world.m10,
            node->geometry_to_world.m11,
            node->geometry_to_world.m12,
            node->geometry_to_world.m13,
            node->geometry_to_world.m20,
            node->geometry_to_world.m21,
            node->geometry_to_world.m22,
            node->geometry_to_world.m23);

        for (material_index = 0;
             material_index < mesh->materials.count;
             ++material_index) {
            const ufbx_mesh_material *mesh_material =
                &mesh->materials.data[material_index];
            ufbx_string texture =
                probe_texture_filename(mesh_material->material);
            const char *texture_data =
                texture.data != NULL ? texture.data : "";
            const char *base = probe_basename(
                texture_data, texture.length);
            size_t base_length = probe_basename_length(
                texture_data, texture.length, base);
            int transparent = jpb_IsTextureTransparent(
                base, level_index);
            int glass = transparent && jpb_IsGlassTexture(base);
            ProbeBounds bounds = {0};
            ProbeBounds material_game_bounds = {0};
            ProbeAlphaRange alpha_range;

            if (mesh_material->num_faces == 0) {
                continue;
            }
            probe_face_bounds(
                node, mesh, mesh_material, &bounds,
                &material_game_bounds,
                &game_bounds, level_index, reference_space);
            alpha_range = probe_face_alpha(mesh, mesh_material);
            printf(
                "  material[%zu] name=%.*s texture=%.*s "
                "texture_path=%.*s pass=%s "
                "faces=%zu triangles=%zu "
                "vertex_alpha=%.6f..%.6f/%.6f/%zu "
                "bounds=%.6f,%.6f,%.6f..%.6f,%.6f,%.6f "
                "game_bounds=%.3f,%.3f,%.3f..%.3f,%.3f,%.3f\n",
                material_index,
                mesh_material->material != NULL
                    ? (int)mesh_material->material->name.length : 0,
                mesh_material->material != NULL
                    ? mesh_material->material->name.data : "",
                (int)base_length,
                base,
                (int)texture.length,
                texture_data,
                probe_pass_name(transparent, glass),
                mesh_material->num_faces,
                mesh_material->num_triangles,
                alpha_range.samples != 0 ? alpha_range.minimum : 0.0,
                alpha_range.samples != 0 ? alpha_range.maximum : 0.0,
                alpha_range.samples != 0
                    ? alpha_range.total / (double)alpha_range.samples : 0.0,
                alpha_range.samples,
                bounds.minX,
                bounds.minY,
                bounds.minZ,
                bounds.maxX,
                bounds.maxY,
                bounds.maxZ,
                material_game_bounds.minX,
                material_game_bounds.minY,
                material_game_bounds.minZ,
                material_game_bounds.maxX,
                material_game_bounds.maxY,
                material_game_bounds.maxZ);
            if (dump_triangle_mode != 0) {
                probe_dump_dynamic_triangles(
                    scene_node_index,
                    node,
                    mesh,
                    mesh_material,
                    probe_pass_name(transparent, glass),
                    dump_triangle_mode == 2);
            }
        }
        ++opaque_mesh_index;
        transparent_mesh_index += transparent_count;
        glass_mesh_index += glass_count;
    }
    printf(
        "mesh_vectors: opaque=%zu transparent=%zu glass=%zu\n",
        opaque_mesh_index,
        transparent_mesh_index,
        glass_mesh_index);
    if (game_bounds.valid) {
        printf(
            "game_bounds=%.3f,%.3f,%.3f..%.3f,%.3f,%.3f\n",
            game_bounds.minX, game_bounds.minY, game_bounds.minZ,
            game_bounds.maxX, game_bounds.maxY, game_bounds.maxZ);
    }
    ufbx_free_scene(scene);
    return 0;
}
