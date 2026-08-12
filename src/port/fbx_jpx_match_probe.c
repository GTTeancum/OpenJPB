/*
 * Evidence-only correspondence probe for the live FBX and legacy JPX level
 * mirrors. This is deliberately not linked into the game runtime. It uses the
 * matched ufbx 0.6.1 source to associate spatially split JPX triangles with
 * their original named FBX mesh surfaces.
 */

#include "jpb/jpx.h"
#include "jpb/level_world.h"

#include "ufbx.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PROBE_STREETS_DYNAMIC_SLOT_COUNT = 25
};

typedef struct ProbePoint {
    double x;
    double z;
    double y;
} ProbePoint;

typedef struct ProbeTriangle {
    ProbePoint point[3];
} ProbeTriangle;

typedef struct ProbeSurface {
    ProbeTriangle *triangles;
    size_t count;
    size_t capacity;
    ProbePoint minimum;
    ProbePoint maximum;
    int hasBounds;
} ProbeSurface;

typedef struct ProbeMatchContext {
    ProbeSurface slots[PROBE_STREETS_DYNAMIC_SLOT_COUNT];
    ProbeSurface baseMaterialOne;
    size_t siteIndex;
    size_t matchedTriangles;
    size_t baseOverlapTriangles;
    size_t candidateUnmatchedTriangles;
    size_t boundsFallbackTriangles;
    size_t materialMatched[JPB_JPX_MAX_MATERIALS];
    size_t slotMatched[PROBE_STREETS_DYNAMIC_SLOT_COUNT];
    int dumpMapping;
    int dumpUnmatched;
} ProbeMatchContext;

_Static_assert(
    UFBX_HEADER_VERSION == 6001u,
    "jpb_fbx_jpx_match_probe requires the matched ufbx 0.6.1 header");

static ProbePoint probe_point_from_fbx(ufbx_vec3 point)
{
    ProbePoint result;

    result.x = point.x / 2.54;
    result.z = -point.z / 2.54;
    result.y = point.y / 2.54;
    return result;
}

static ProbePoint probe_point_from_jpx(const JPBJpxVertex *vertex)
{
    ProbePoint result;

    result.x = vertex->x;
    result.z = vertex->z;
    result.y = vertex->y;
    return result;
}

static ProbePoint probe_subtract(ProbePoint left, ProbePoint right)
{
    ProbePoint result;

    result.x = left.x - right.x;
    result.z = left.z - right.z;
    result.y = left.y - right.y;
    return result;
}

static double probe_dot(ProbePoint left, ProbePoint right)
{
    return left.x * right.x + left.z * right.z + left.y * right.y;
}

static ProbePoint probe_cross(ProbePoint left, ProbePoint right)
{
    ProbePoint result;

    result.x = left.z * right.y - left.y * right.z;
    result.z = left.y * right.x - left.x * right.y;
    result.y = left.x * right.z - left.z * right.x;
    return result;
}

static void probe_surface_add_bounds(
    ProbeSurface *surface, ProbePoint point)
{
    if (!surface->hasBounds) {
        surface->minimum = surface->maximum = point;
        surface->hasBounds = 1;
        return;
    }
    if (point.x < surface->minimum.x) surface->minimum.x = point.x;
    if (point.z < surface->minimum.z) surface->minimum.z = point.z;
    if (point.y < surface->minimum.y) surface->minimum.y = point.y;
    if (point.x > surface->maximum.x) surface->maximum.x = point.x;
    if (point.z > surface->maximum.z) surface->maximum.z = point.z;
    if (point.y > surface->maximum.y) surface->maximum.y = point.y;
}

static int probe_surface_add_triangle(
    ProbeSurface *surface, const ProbeTriangle *triangle)
{
    size_t index;

    if (surface->count == surface->capacity) {
        size_t capacity = surface->capacity != 0
                              ? surface->capacity * 2 : 32;
        ProbeTriangle *triangles = (ProbeTriangle *)realloc(
            surface->triangles, capacity * sizeof(*triangles));

        if (triangles == NULL) {
            return 0;
        }
        surface->triangles = triangles;
        surface->capacity = capacity;
    }
    surface->triangles[surface->count++] = *triangle;
    for (index = 0; index < 3; ++index) {
        probe_surface_add_bounds(surface, triangle->point[index]);
    }
    return 1;
}

static int probe_point_in_bounds(
    const ProbeSurface *surface, ProbePoint point, double tolerance)
{
    return surface->hasBounds &&
           point.x >= surface->minimum.x - tolerance &&
           point.x <= surface->maximum.x + tolerance &&
           point.z >= surface->minimum.z - tolerance &&
           point.z <= surface->maximum.z + tolerance &&
           point.y >= surface->minimum.y - tolerance &&
           point.y <= surface->maximum.y + tolerance;
}

static int probe_point_on_triangle(
    ProbePoint point, const ProbeTriangle *triangle)
{
    const double planeTolerance = 0.04;
    const double edgeTolerance = 0.025;
    ProbePoint edge0 = probe_subtract(
        triangle->point[1], triangle->point[0]);
    ProbePoint edge1 = probe_subtract(
        triangle->point[2], triangle->point[0]);
    ProbePoint relative = probe_subtract(point, triangle->point[0]);
    ProbePoint normal = probe_cross(edge0, edge1);
    double normalLengthSquared = probe_dot(normal, normal);
    double dot00;
    double dot01;
    double dot11;
    double dot20;
    double dot21;
    double denominator;
    double first;
    double second;

    if (normalLengthSquared < 1.0e-12) {
        return 0;
    }
    if (fabs(probe_dot(relative, normal)) >
        planeTolerance * sqrt(normalLengthSquared)) {
        return 0;
    }

    dot00 = probe_dot(edge0, edge0);
    dot01 = probe_dot(edge0, edge1);
    dot11 = probe_dot(edge1, edge1);
    dot20 = probe_dot(relative, edge0);
    dot21 = probe_dot(relative, edge1);
    denominator = dot00 * dot11 - dot01 * dot01;
    if (fabs(denominator) < 1.0e-12) {
        return 0;
    }
    first = (dot11 * dot20 - dot01 * dot21) / denominator;
    second = (dot00 * dot21 - dot01 * dot20) / denominator;
    return first >= -edgeTolerance && second >= -edgeTolerance &&
           first + second <= 1.0 + edgeTolerance;
}

static int probe_surface_contains_point(
    const ProbeSurface *surface, ProbePoint point)
{
    size_t index;

    if (!probe_point_in_bounds(surface, point, 0.04)) {
        return 0;
    }
    for (index = 0; index < surface->count; ++index) {
        if (probe_point_on_triangle(point, &surface->triangles[index])) {
            return 1;
        }
    }
    return 0;
}

static int probe_surface_contains_triangle(
    const ProbeSurface *surface, const ProbeTriangle *triangle)
{
    ProbePoint centroid;

    /*
     * JPX spatial splitting can retriangulate across adjacent coplanar FBX
     * faces. The centroid still lies on the owning source surface even when
     * a generated triangle's three corners fall in different source faces.
     */
    centroid.x = (triangle->point[0].x + triangle->point[1].x +
                  triangle->point[2].x) / 3.0;
    centroid.z = (triangle->point[0].z + triangle->point[1].z +
                  triangle->point[2].z) / 3.0;
    centroid.y = (triangle->point[0].y + triangle->point[1].y +
                  triangle->point[2].y) / 3.0;
    return probe_surface_contains_point(surface, centroid);
}

static int probe_triangle_degenerate(const ProbeTriangle *triangle)
{
    ProbePoint first = probe_subtract(
        triangle->point[1], triangle->point[0]);
    ProbePoint second = probe_subtract(
        triangle->point[2], triangle->point[0]);
    ProbePoint cross = probe_cross(first, second);

    return probe_dot(cross, cross) < 1.0e-12;
}

static int probe_collect_mesh_material(
    const ufbx_node *node,
    const ufbx_mesh *mesh,
    const ufbx_mesh_material *material,
    ProbeSurface *surface)
{
    size_t face_list_index;

    for (face_list_index = 0;
         face_list_index < material->face_indices.count;
         ++face_list_index) {
        ufbx_face face = mesh->faces.data[
            material->face_indices.data[face_list_index]];
        ProbeTriangle triangle;
        uint32_t vertex_offset;

        if (face.num_indices != 3) {
            fprintf(stderr, "non-triangle FBX face encountered\n");
            return 0;
        }
        for (vertex_offset = 0; vertex_offset < 3; ++vertex_offset) {
            uint32_t index = face.index_begin + vertex_offset;
            ufbx_vec3 local = ufbx_get_vertex_vec3(
                &mesh->vertex_position, index);
            ufbx_vec3 world = ufbx_transform_position(
                &node->geometry_to_world, local);

            triangle.point[vertex_offset] = probe_point_from_fbx(world);
        }
        if (!probe_surface_add_triangle(surface, &triangle)) {
            return 0;
        }
    }
    return 1;
}

static int probe_collect_fbx_surfaces(
    const ufbx_scene *scene, ProbeMatchContext *context)
{
    size_t node_index;

    for (node_index = 0; node_index < scene->nodes.count; ++node_index) {
        const ufbx_node *node = scene->nodes.data[node_index];
        const ufbx_mesh *mesh = node->mesh;

        if (mesh == NULL) {
            continue;
        }
        if (node_index == 1) {
            if (mesh->materials.count <= 1 ||
                !probe_collect_mesh_material(
                    node,
                    mesh,
                    &mesh->materials.data[1],
                    &context->baseMaterialOne)) {
                return 0;
            }
        } else if (node_index > 1) {
            int slot = jpb_StreetsCullMeshIndexFromName(node->name.data);
            size_t material_index;

            if (slot <= 0 ||
                slot >= PROBE_STREETS_DYNAMIC_SLOT_COUNT) {
                fprintf(
                    stderr,
                    "unexpected Streets node name: %.*s\n",
                    (int)node->name.length,
                    node->name.data);
                return 0;
            }
            for (material_index = 0;
                 material_index < mesh->materials.count;
                 ++material_index) {
                const ufbx_mesh_material *material =
                    &mesh->materials.data[material_index];

                if (material->num_faces != 0 &&
                    !probe_collect_mesh_material(
                        node, mesh, material, &context->slots[slot])) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

static uint32_t probe_match_triangle(
    ProbeMatchContext *context,
    uint32_t material,
    const ProbeTriangle *triangle,
    uint32_t *boundsMask)
{
    int firstSlot;
    int lastSlot;
    int slot;
    uint32_t mask = 0;
    ProbePoint centroid;

    if (material == 6) {
        firstSlot = 1;
        lastSlot = 12;
    } else if (material == 1) {
        firstSlot = 13;
        lastSlot = 24;
    } else {
        return 0;
    }
    centroid.x = (triangle->point[0].x + triangle->point[1].x +
                  triangle->point[2].x) / 3.0;
    centroid.z = (triangle->point[0].z + triangle->point[1].z +
                  triangle->point[2].z) / 3.0;
    centroid.y = (triangle->point[0].y + triangle->point[1].y +
                  triangle->point[2].y) / 3.0;
    for (slot = firstSlot; slot <= lastSlot; ++slot) {
        if (probe_point_in_bounds(&context->slots[slot], centroid, 0.04)) {
            *boundsMask |= UINT32_C(1) << slot;
            if (probe_surface_contains_triangle(
                    &context->slots[slot], triangle)) {
                mask |= UINT32_C(1) << slot;
            }
        }
    }
    return mask;
}

static int probe_visit_jpx_site(
    const JPBJpxPatchSite *site, void *userData)
{
    ProbeMatchContext *context = (ProbeMatchContext *)userData;
    JPBJpxVertex first;
    JPBJpxVertex second;
    uint16_t index;

    if (site->vertexCount < 3) {
        ++context->siteIndex;
        return 0;
    }
    if (jpx_DecodeVertex(site, 0, &first) != JPB_JPX_OK ||
        jpx_DecodeVertex(site, 1, &second) != JPB_JPX_OK) {
        return 1;
    }
    for (index = 2; index < site->vertexCount; ++index) {
        JPBJpxVertex third;
        ProbeTriangle triangle;
        uint32_t mask;
        uint32_t boundsMask = 0;
        int slot;

        if (jpx_DecodeVertex(site, index, &third) != JPB_JPX_OK) {
            return 1;
        }
        triangle.point[0] = probe_point_from_jpx(&first);
        triangle.point[1] = probe_point_from_jpx(&second);
        triangle.point[2] = probe_point_from_jpx(&third);
        if (!probe_triangle_degenerate(&triangle)) {
            mask = probe_match_triangle(
                context, site->materialIndex, &triangle, &boundsMask);
            if (mask == 0 && boundsMask != 0) {
                /*
                 * A small set of shipped Streets triangles cross source-face
                 * diagonals introduced by JPX retriangulation. Their
                 * centroids remain inside only the owning wall bounds. The
                 * final strict correspondence pass counts this separately.
                 */
                mask = boundsMask;
                ++context->boundsFallbackTriangles;
            }
            if (mask != 0) {
                ++context->matchedTriangles;
                ++context->materialMatched[site->materialIndex];
                for (slot = 1;
                     slot < PROBE_STREETS_DYNAMIC_SLOT_COUNT;
                     ++slot) {
                    if ((mask & (UINT32_C(1) << slot)) != 0) {
                        ++context->slotMatched[slot];
                    }
                }
                if (site->materialIndex == 1 &&
                    probe_surface_contains_triangle(
                        &context->baseMaterialOne, &triangle)) {
                    ++context->baseOverlapTriangles;
                }
                if (context->dumpMapping) {
                    printf(
                        "match site=%zu offset=%zu triangle=%u "
                        "material=%u mask=0x%08x\n",
                        context->siteIndex,
                        site->offset,
                        (unsigned)(index - 2),
                        (unsigned)site->materialIndex,
                        (unsigned)mask);
                }
            } else if (boundsMask != 0) {
                ++context->candidateUnmatchedTriangles;
                if (context->dumpUnmatched) {
                    printf(
                        "unmatched site=%zu offset=%zu triangle=%u "
                        "material=%u p=%.6f,%.6f,%.6f;"
                        "%.6f,%.6f,%.6f;%.6f,%.6f,%.6f\n",
                        context->siteIndex,
                        site->offset,
                        (unsigned)(index - 2),
                        (unsigned)site->materialIndex,
                        triangle.point[0].x,
                        triangle.point[0].z,
                        triangle.point[0].y,
                        triangle.point[1].x,
                        triangle.point[1].z,
                        triangle.point[1].y,
                        triangle.point[2].x,
                        triangle.point[2].z,
                        triangle.point[2].y);
                }
            }
        }
        first = second;
        second = third;
    }
    ++context->siteIndex;
    return 0;
}

static void probe_free_context(ProbeMatchContext *context)
{
    size_t index;

    free(context->baseMaterialOne.triangles);
    for (index = 0;
         index < PROBE_STREETS_DYNAMIC_SLOT_COUNT;
         ++index) {
        free(context->slots[index].triangles);
    }
}

static uint64_t probe_fnv1a64(const uint8_t *data, size_t size)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    size_t index;

    for (index = 0; index < size; ++index) {
        hash ^= data[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

int main(int argc, char **argv)
{
    ufbx_error error;
    ufbx_scene *scene = NULL;
    ProbeMatchContext context;
    JPBJpxLoadConfig config;
    JPBJpxView view;
    uint8_t *storage = NULL;
    int result = 1;
    int slot;

    if (argc != 4 && argc != 5) {
        fprintf(
            stderr,
            "usage: %s 8 <streets.fbx> <STREETS.jpx> "
            "[--mapping|--unmatched]\n",
            argv[0]);
        return 2;
    }
    if (atoi(argv[1]) != 8 ||
        (argc == 5 && strcmp(argv[4], "--mapping") != 0 &&
         strcmp(argv[4], "--unmatched") != 0)) {
        fputs("this evidence probe currently supports only level 8\n", stderr);
        return 2;
    }
    memset(&context, 0, sizeof(context));
    context.dumpMapping = argc == 5 && strcmp(argv[4], "--mapping") == 0;
    context.dumpUnmatched =
        argc == 5 && strcmp(argv[4], "--unmatched") == 0;
    memset(&error, 0, sizeof(error));
    scene = ufbx_load_file(argv[2], NULL, &error);
    if (scene == NULL) {
        fprintf(
            stderr,
            "FBX load failed: %.*s\n",
            (int)error.description.length,
            error.description.data != NULL ? error.description.data : "");
        result = 3;
        goto cleanup;
    }
    if (!probe_collect_fbx_surfaces(scene, &context)) {
        fputs("could not collect FBX mesh surfaces\n", stderr);
        result = 4;
        goto cleanup;
    }

    storage = (uint8_t *)malloc(JPB_JPX_REFERENCE_WORLD_CAPACITY);
    if (storage == NULL) {
        fputs("could not allocate JPX storage\n", stderr);
        result = 5;
        goto cleanup;
    }
    memset(&config, 0, sizeof(config));
    config.storage = storage;
    config.storageCapacity = JPB_JPX_REFERENCE_WORLD_CAPACITY;
    if (jpx_LoadFile(argv[3], &config, &view) != JPB_JPX_OK ||
        jpx_ForEachPatchSite(
            &view, probe_visit_jpx_site, &context) != JPB_JPX_OK) {
        fputs("could not inspect JPX mesh\n", stderr);
        result = 6;
        goto cleanup;
    }

    printf(
        "correspondence ufbx=%u jpx_fnv1a64=0x%016llx "
        "sites=%zu matched=%zu "
        "material1=%zu material6=%zu base_overlap=%zu "
        "bounds_fallback=%zu candidate_unmatched=%zu\n",
        (unsigned)ufbx_source_version,
        (unsigned long long)probe_fnv1a64(view.data, view.size),
        context.siteIndex,
        context.matchedTriangles,
        context.materialMatched[1],
        context.materialMatched[6],
        context.baseOverlapTriangles,
        context.boundsFallbackTriangles,
        context.candidateUnmatchedTriangles);
    for (slot = 1; slot < PROBE_STREETS_DYNAMIC_SLOT_COUNT; ++slot) {
        printf(
            "slot[%d] source_triangles=%zu matched_triangles=%zu\n",
            slot,
            context.slots[slot].count,
            context.slotMatched[slot]);
    }
    result = 0;

cleanup:
    free(storage);
    if (scene != NULL) {
        ufbx_free_scene(scene);
    }
    probe_free_context(&context);
    return result;
}
