#include "jpb/bmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct BmdGeometryStats {
    size_t signed_faces;
    size_t packed_faces;
    size_t vertices;
} BmdGeometryStats;

static JPBBmdResult validate_node_geometry(
    const JPBBmdView *view,
    const Mnode *node,
    BmdGeometryStats *stats)
{
    JPBBmdGeometryView geometry;
    JPBBmdResult result;
    int child;

    result = jpb_BmdGetGeometry(
        view, node->pGeomData, &geometry);
    if (result != JPB_BMD_OK) {
        fprintf(
            stderr,
            "geometry validation failed at node %.32s "
            "id=0x%x result=%d\n",
            node->pGeomData != NULL
                ? node->pGeomData->name
                : "<null>",
            (unsigned)node->id,
            (int)result);
        return result;
    }
    if (geometry.face_encoding == JPB_BMD_FACE_SIGNED_16) {
        stats->signed_faces += geometry.face_count;
    } else {
        stats->packed_faces += geometry.face_count;
    }
    stats->vertices += geometry.local_vertex_count;
    for (child = 0; child < node->numChildNodes; ++child) {
        result = validate_node_geometry(
            view, &node->aChildNode[child], stats);
        if (result != JPB_BMD_OK) {
            return result;
        }
    }
    return JPB_BMD_OK;
}

static void print_node_hierarchy(
    const Mnode *node, int parent_id, int depth)
{
    int child;

    printf(
        "node depth=%d name=%.32s id=0x%x index=%u "
        "parent=%d translation=(%d,%d,%d) children=%d\n",
        depth,
        node->pGeomData != NULL
            ? node->pGeomData->name
            : "<null>",
        (unsigned)node->id,
        (unsigned)node->id & NODE_INDEX_MASK,
        parent_id,
        (int)node->v3Translation.vx,
        (int)node->v3Translation.vy,
        (int)node->v3Translation.vz,
        (int)node->numChildNodes);
    for (child = 0; child < node->numChildNodes; ++child) {
        print_node_hierarchy(
            &node->aChildNode[child],
            (int)((unsigned)node->id & NODE_INDEX_MASK),
            depth + 1);
    }
}

int main(int argc, char **argv)
{
    uint8_t *storage;
    JPBBmdView view;
    JPBBmdResult result;
    JPBBmdResult geometry_result = JPB_BMD_OK;
    modelObject model;
    modelObject *registered_model;
    BmdGeometryStats geometry_stats = {0};
    int expected_result = JPB_BMD_OK;
    int expected_geometry_result = JPB_BMD_OK;
    int validate_geometry = 0;
    int print_nodes = 0;

    if (argc != 2 && argc != 3 && argc != 4) {
        fprintf(
            stderr,
            "usage: %s model.bmd [--validate-geometry | "
            "--nodes | "
            "--expect-result status | "
            "--expect-geometry-result status]\n",
            argv[0]);
        return 2;
    }
    if (argc == 3) {
        if (strcmp(argv[2], "--validate-geometry") == 0) {
            validate_geometry = 1;
        } else if (strcmp(argv[2], "--nodes") == 0) {
            print_nodes = 1;
        } else {
            return 2;
        }
    } else if (argc == 4) {
        if (strcmp(argv[2], "--expect-result") == 0) {
            expected_result = atoi(argv[3]);
        } else if (
            strcmp(
                argv[2],
                "--expect-geometry-result") == 0) {
            expected_geometry_result = atoi(argv[3]);
            validate_geometry = 1;
        } else {
            return 2;
        }
    }
    storage = (uint8_t *)malloc(JPB_BMD_REFERENCE_CAPACITY);
    if (storage == NULL) {
        fputs("failed to allocate BMD storage\n", stderr);
        return 1;
    }
    result = jpb_BmdLoadFile(
        argv[1],
        storage,
        JPB_BMD_REFERENCE_CAPACITY,
        &view);
    if ((int)result != expected_result) {
        fprintf(
            stderr,
            "unexpected BMD result %s: result=%d expected=%d\n",
            argv[1],
            (int)result,
            expected_result);
        free(storage);
        return 1;
    }
    if (result != JPB_BMD_OK) {
        printf(
            "bmd=%s expected-result=%d\n",
            argv[1],
            (int)result);
        free(storage);
        return 0;
    }
    if (validate_geometry || print_nodes) {
        memset(&model, 0, sizeof(model));
        model_InitModels();
        jpb_ModelSetGeometryBounds(
            view.payload, view.payload_size);
        registered_model = model_gInitModelRoot(
            (geomData *)(void *)view.payload,
            "BMD_PROBE",
            -1);
        if (registered_model == NULL) {
            result = JPB_BMD_INVALID_LAYOUT;
        } else {
            if (!mReuseModel) {
                view.geometry_streams_relocated = 1;
            }
            model = *registered_model;
            result = JPB_BMD_OK;
        }
        if (result == JPB_BMD_OK && validate_geometry) {
            geometry_result = validate_node_geometry(
                &view, model.pRootNode, &geometry_stats);
        } else {
            geometry_result = result;
        }
        if ((int)geometry_result !=
            expected_geometry_result) {
            fprintf(
                stderr,
                "unexpected BMD geometry result %s: "
                "result=%d expected=%d\n",
                argv[1],
                (int)geometry_result,
                expected_geometry_result);
            free(storage);
            return 1;
        }
        if (print_nodes && result == JPB_BMD_OK) {
            print_node_hierarchy(model.pRootNode, -1, 0);
        }
    }
    printf(
        "bmd=%s bytes=%zu nodes=%zu root=%.32s id=0x%x "
        "geometry=%d signed_faces=%zu packed_faces=%zu "
        "vertices=%zu\n",
        argv[1],
        view.file_size,
        view.node_count,
        view.root->name,
        (unsigned)view.root->id,
        (int)geometry_result,
        geometry_stats.signed_faces,
        geometry_stats.packed_faces,
        geometry_stats.vertices);
    free(storage);
    return 0;
}
