/*
 * Dependency-light PC integration probe for reconstructed J3D relocation.
 * This is intentionally a command-line tool rather than a renderer: it makes
 * archive and runtime-layout failures observable before platform work begins.
 */

#include "jpb/filesys.h"
#include "jpb/globalarrays.h"
#include "jpb/memory.h"
#include "jpb/world.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_chunk_id(const uint8_t *cursor, const uint8_t *end)
{
    size_t index;

    fputs("chunk=\"", stderr);
    for (index = 0; index < 8 && cursor + index < end; ++index) {
        unsigned char value = cursor[index];

        fputc(isprint(value) ? value : '.', stderr);
    }
    fputs("\"", stderr);
}

int main(int argc, char **argv)
{
    WorldData world;
    char *buffer;
    uint8_t *cursor;
    int32_t file_size = 0;
    int print_placements = 0;
    int print_ai = -1;
    int find_opcode = -1;
    int print_opcode_summary = 0;
    int argument;
    int result;

    if (argc < 2) {
        fprintf(
            stderr,
            "usage: %s <level.j3d> [--placements] [--ai <index>] "
            "[--opcode <value>] [--opcode-summary]\n",
            argv[0]);
        return 2;
    }
    for (argument = 2; argument < argc; ++argument) {
        if (strcmp(argv[argument], "--placements") == 0) {
            print_placements = 1;
        } else if (strcmp(argv[argument], "--ai") == 0 &&
                   argument + 1 < argc) {
            char *end = NULL;
            long index = strtol(
                argv[++argument], &end, 10);

            if (end == argv[argument] ||
                *end != '\0' ||
                index < 0 ||
                index > INT32_MAX) {
                fprintf(
                    stderr,
                    "invalid AI index: %s\n",
                    argv[argument]);
                return 2;
            }
            print_ai = (int)index;
        } else if (strcmp(argv[argument], "--opcode") == 0 &&
                   argument + 1 < argc) {
            char *end = NULL;
            long opcode = strtol(
                argv[++argument], &end, 0);

            if (end == argv[argument] ||
                *end != '\0' ||
                opcode < 0 ||
                opcode > UINT16_MAX) {
                fprintf(
                    stderr,
                    "invalid opcode: %s\n",
                    argv[argument]);
                return 2;
            }
            find_opcode = (int)opcode;
        } else if (strcmp(
                       argv[argument],
                       "--opcode-summary") == 0) {
            print_opcode_summary = 1;
        } else {
            fprintf(
                stderr,
                "usage: %s <level.j3d> [--placements] "
                "[--ai <index>] [--opcode <value>] "
                "[--opcode-summary]\n",
                argv[0]);
            return 2;
        }
    }
    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    pointerRegistry_Reset();
    (void)memory_InitMemorySystem();
    buffer = file_LoadFile2PoolFunc(
        argv[1],
        &file_size,
        MEMORY_POOL_ANY,
        __LINE__,
        (char *)__FILE__);
    if (buffer == NULL) {
        fprintf(stderr, "could not load %s\n", argv[1]);
        return 4;
    }

    result = file_RelocateChunks(
        (uint8_t *)buffer, (size_t)(uint32_t)file_size, &cursor);
    if (result != JPB_CHUNKS_OK) {
        ptrdiff_t offset = cursor - (uint8_t *)buffer;

        fprintf(
            stderr,
            "relocation failed: status=%d offset=%td ",
            result,
            offset);
        print_chunk_id(
            cursor, (uint8_t *)buffer + (size_t)(uint32_t)file_size);
        fputc('\n', stderr);
        pointerRegistry_Reset();
        return 5;
    }

    printf(
        "loaded=%d consumed=%td textures=%d libraries=%d tags=%d "
        "enemies=%d ai=%d actors=%d animdefs=%d animmap=%d "
        "powerups=%d map=%dx%d\n",
        file_size,
        cursor - (uint8_t *)buffer,
        world.numTexture,
        world.numLibs,
        world.numTags,
        world.nEnemy,
        world.nAI,
        world.nActor,
        world.nADef,
        world.nAnimMap,
        world.nPowerups,
        world.sizeX,
        world.sizeZ);
    printf(
        "start=(%d,%d,%d) bounds=(%d,%d)-(%d,%d) background=(%u,%u,%u,%u)\n",
        world.start.vx,
        world.start.vy,
        world.start.vz,
        world.minX,
        world.minZ,
        world.maxX,
        world.maxZ,
        (unsigned)world.bkColor.r,
        (unsigned)world.bkColor.g,
        (unsigned)world.bkColor.b,
        (unsigned)world.bkColor.cd);
    if (print_placements) {
        int index;

        for (index = 0; index < world.nActor; ++index) {
            printf(
                "actor[%d]=%s\n",
                index,
                world.apActorNames[index]);
        }
        for (index = 0; index < world.nEnemy; ++index) {
            const wsl_BAP_PLACEMENT *placement =
                world.apEnemy[index];
            int extension;

            printf(
                "enemy[%d] name=%s actor=%d ai=%d owner=%d "
                "flags=0x%08x mode=%d move=%d hp=%d speed=%d "
                "range=%d/%d status=%d loc=(%d,%d,%d) waypoints=%d "
                "enemyExt=(",
                index,
                placement->aName,
                placement->actorNum,
                placement->aiNum,
                placement->aiDf.ownerType,
                placement->aiDf.activeFlags,
                placement->aiDf.startMode,
                placement->aiDf.movementMode,
                placement->aiDf.hitPoints,
                placement->aiDf.movementSpeed,
                placement->aiDf.aRange,
                placement->aiDf.daRange,
                placement->status,
                placement->loc.vx,
                placement->loc.vy,
                placement->loc.vz,
                placement->nWaypnt);
            for (extension = 0; extension < 12; ++extension) {
                printf(
                    "%s%u",
                    extension == 0 ? "" : ",",
                    (unsigned)placement->aiDf
                        .enemyExt[extension]);
            }
            fputs(") path=(", stdout);
            for (extension = 0;
                 extension < placement->nWaypnt;
                 ++extension) {
                printf(
                    "%s%d/%d/%d/0x%08x",
                    extension == 0 ? "" : ",",
                    placement->wayPoints[extension].loc.vx,
                    placement->wayPoints[extension].loc.vy,
                    placement->wayPoints[extension].loc.vz,
                    (unsigned)placement
                        ->wayPoints[extension].flags);
            }
            puts(")");
        }
    }
    if (print_ai >= 0) {
        BAP_AI *ai;
        UDATA *variables;
        int variable_count;
        int node_count;
        int node;

        if (print_ai >= world.nAI) {
            fprintf(
                stderr,
                "AI index %d is outside 0..%d\n",
                print_ai,
                world.nAI - 1);
            pointerRegistry_Reset();
            return 6;
        }
        ai = world.apAI[print_ai];
        node_count =
            ai->numNodes - ai->numAvailable;
        if (node_count < 0 ||
            node_count > ai->numNodes) {
            fprintf(
                stderr,
                "AI index %d has invalid node accounting "
                "(total=%d available=%d)\n",
                print_ai,
                ai->numNodes,
                ai->numAvailable);
            pointerRegistry_Reset();
            return 6;
        }
        variable_count =
            (ai->bSize -
             (int)offsetof(BAP_AI, aiNodes) -
             node_count * (int)sizeof(BAP_AINODE)) /
            (int)sizeof(UDATA);
        variables = (UDATA *)getPtr(
            (int)ai->pVars,
            JPB_POINTER_ARRAY_AI);
        printf(
            "ai[%d] total=%d available=%d nodes=%d "
            "bytes=%d vars=%u/%d\n",
            print_ai,
            ai->numNodes,
            ai->numAvailable,
            node_count,
            ai->bSize,
            ai->pVars,
            variable_count);
        if (variables != NULL) {
            int variable;

            for (variable = 0;
                 variable < variable_count;
                 ++variable) {
                printf(
                    "ai[%d].var[%d]=0x%08x/%d/%g\n",
                    print_ai,
                    variable,
                    variables[variable].ui,
                    variables[variable].si,
                    (double)variables[variable].f);
            }
        }
        for (node = 0; node < node_count; ++node) {
            const BAP_AINODE *entry =
                &ai->aiNodes[node];

            printf(
                "ai[%d].node[%d] parent=%d child=%d "
                "sibling=%d opcode=%d value="
                "0x%08x/%d/%g\n",
                print_ai,
                node,
                entry->iParent,
                entry->iChild,
                entry->iSibling,
                entry->opcode,
                entry->vx.ui,
                entry->vx.si,
                (double)entry->vx.f);
            if (((uint16_t)entry->opcode &
                 UINT16_C(0x4000)) == 0 &&
                variables != NULL &&
                entry->vx.ui <=
                    (uint32_t)variable_count &&
                variable_count -
                        (int)entry->vx.ui >=
                    4 &&
                (entry->opcode == 0x301 ||
                 entry->opcode == 0x302 ||
                 entry->opcode == 0x307)) {
                const UDATA *args =
                    &variables[entry->vx.ui];

                printf(
                    "  args=%d,%d,%d,%d\n",
                    args[0].si,
                    args[1].si,
                    args[2].si,
                    args[3].si);
            }
        }
    }
    if (find_opcode >= 0) {
        int ai_index;

        for (ai_index = 0;
             ai_index < world.nAI;
             ++ai_index) {
            BAP_AI *ai = world.apAI[ai_index];
            int node_count =
                ai->numNodes - ai->numAvailable;
            int variable_count;
            UDATA *variables;
            int node;

            if (node_count < 0 ||
                node_count > ai->numNodes) {
                continue;
            }
            variable_count =
                (ai->bSize -
                 (int)offsetof(BAP_AI, aiNodes) -
                 node_count *
                     (int)sizeof(BAP_AINODE)) /
                (int)sizeof(UDATA);
            variables = (UDATA *)getPtr(
                (int)ai->pVars,
                JPB_POINTER_ARRAY_AI);
            for (node = 0; node < node_count; ++node) {
                uint16_t encoded =
                    (uint16_t)ai->aiNodes[node].opcode;
                uint16_t opcode =
                    (encoded & UINT16_C(0x4000)) != 0
                        ? encoded & UINT16_C(0x0fff)
                        : encoded;

                if ((int)opcode == find_opcode) {
                    printf(
                        "opcode=0x%03x ai=%d node=%d "
                        "encoded=0x%04x value=0x%08x\n",
                        find_opcode,
                        ai_index,
                        node,
                        encoded,
                        ai->aiNodes[node].vx.ui);
                    if ((encoded &
                         UINT16_C(0x4000)) != 0) {
                        printf(
                            "  args=%d\n",
                            ai->aiNodes[node].vx.si);
                    } else if (variables != NULL &&
                               ai->aiNodes[node].vx.ui <=
                                   (uint32_t)variable_count) {
                        const UDATA *args =
                            &variables[
                                ai->aiNodes[node].vx.ui];
                        int available =
                            variable_count -
                            (int)ai->aiNodes[node].vx.ui;
                        int count =
                            available < 4 ? available : 4;
                        int arg;

                        fputs("  args=", stdout);
                        for (arg = 0;
                             arg < count;
                             ++arg) {
                            printf(
                                "%s%d",
                                arg == 0 ? "" : ",",
                                args[arg].si);
                        }
                        fputc('\n', stdout);
                    }
                }
            }
        }
    }
    if (print_opcode_summary) {
        enum { opcode_capacity = UINT16_MAX + 1 };
        uint32_t *counts =
            (uint32_t *)calloc(
                opcode_capacity, sizeof(*counts));
        int ai_index;
        int opcode;

        if (counts == NULL) {
            fputs(
                "could not allocate opcode summary\n",
                stderr);
            pointerRegistry_Reset();
            return 7;
        }
        for (ai_index = 0;
             ai_index < world.nAI;
             ++ai_index) {
            BAP_AI *ai = world.apAI[ai_index];
            int node_count =
                ai->numNodes - ai->numAvailable;
            int node;

            if (node_count < 0 ||
                node_count > ai->numNodes) {
                continue;
            }
            for (node = 0;
                 node < node_count;
                 ++node) {
                uint16_t encoded =
                    (uint16_t)ai->aiNodes[node]
                        .opcode;
                uint16_t decoded =
                    (encoded &
                     UINT16_C(0x4000)) != 0
                        ? encoded &
                              UINT16_C(0x0fff)
                        : encoded;

                ++counts[decoded];
            }
        }
        for (opcode = 0;
             opcode < opcode_capacity;
             ++opcode) {
            if (counts[opcode] != 0) {
                printf(
                    "opcode[0x%03x]=%u\n",
                    opcode,
                    (unsigned)counts[opcode]);
            }
        }
        free(counts);
    }
    pointerRegistry_Reset();
    return 0;
}
