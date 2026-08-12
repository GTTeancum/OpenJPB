/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\filesys.c.
 *
 * The integrated block covers the generic file-loading boundary, all 25 PDB
 * procedures, exact chunk-header decoding, and dependency-light WorldData
 * relocation. Resource-path, texture, event-list, and UV initialization
 * effects enter through narrow hooks so the same game-owned loader remains
 * usable by the PC build and the later nxdk adapter.
 *
 * Provenance:
 *   direct     - names/signatures/locals and wapChunk layout from exact PDB.
 *   decompiled - file calls and 46-entry chunk dispatch checked in Ghidra.
 *   assembly   - cursor advancement, returns, and comparison extent checked
 *                at exact function RVAs.
 *
 * PDB module: 0034
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\filesys.obj
 * Primary source: W:\SWJediPowerBattles\Work\filesys.c
 * Compiler language: c
 * Emitted procedures: 25
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/filesys.h"

#include "jpb/effects.h"
#include "jpb/globalarrays.h"
#include "jpb/io.h"
#include "jpb/jonny.h"
#include "jpb/memory.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t *jpb_level_data_begin;
static const uint8_t *jpb_level_data_end;

int jpb_LevelDataContains(const void *data, size_t size)
{
    uintptr_t address;
    uintptr_t begin;
    uintptr_t end;

    if (data == NULL) {
        return 0;
    }
    /* Unit-level exact-owner tests may install leveldata directly. */
    if (jpb_level_data_begin == NULL || jpb_level_data_end == NULL) {
        return 1;
    }
    address = (uintptr_t)data;
    begin = (uintptr_t)jpb_level_data_begin;
    end = (uintptr_t)jpb_level_data_end;
    return address >= begin && address <= end && size <= end - address;
}

void jpb_LevelDataClearBounds(void)
{
    jpb_level_data_begin = NULL;
    jpb_level_data_end = NULL;
}

int gFileNotFound;
static int numThins;
static JPBJonnyPostLoadHook filesys_clear_events_hook;
static JPBJonnyPostLoadHook filesys_initialize_uvs_hook;
static JPBChunkPathResolver filesys_resolve_path_hook;
static JPBChunksPostLoadHook filesys_post_load_hook;
static JPBTextureLoadHook filesys_load_texture_hook;

static const char *const filesys_resident_sprite_names[
    JPB_RESIDENT_SPRITE_COUNT] = {
    "a_pal",       "a_smokegry",   "a_sabrflar2", "a_starbrst1",
    "a_spark",     "a_projorg",    "a_goldglw",   "a_forcpwr6",
    "a_sparkles",  "a_ligtng2",    "a_forcpwr5",  "a_ring3b",
    "a_forcpwr4",  "a_forcpwr3",   "a_ligtng1",   "a_sabrflar",
    "a_sabrlash",  "a_blnt_hit",   "a_enrgyrg",   "a_streaks",
    "a_jy1",       "a_jy2",        "a_jy3",        "a_jy4",
    "a_jy5",       "a_jy6",        "a_jy8",        "a_jy9",
    "a_jy10",      "a_td_laser",   "a_bd_laser",   "a_thermal",
    "a_shadow",    "a_slug",       "a_adi_shld",   "a_ring_txt",
    "a_lockon",    "a_lockon2",    "a_gball",      "a_meter_lights",
    "a_meter_main", "a_meter_bars", "checker",      "a_pilot",
    "a_maiden",    "a_detonator",  "a_bolt",       "a_battery",
    "a_shield",    "a_credit"
};

static const uint8_t filesys_chunk_ids[][8] = {
    {'B', '3', 'D', '_', 'N', 'U', 'L', 'L'},
    {'B', '3', 'D', ' ', 'F', 'I', 'L', 'E'},
    {'B', '3', 'D', '_', 'P', 'A', 'L', 'S'},
    {'B', '3', 'D', '_', 'G', 'M', 'A', 'P'},
    {'B', '3', 'D', '_', 'G', 'V', 'R', 'T'},
    {'B', '3', 'D', '_', 'L', 'V', 'R', 'T'},
    {'B', '3', 'D', '_', 'T', 'E', 'X', ' '},
    {'B', '3', 'D', '_', 'M', 'A', 'T', ' '},
    {'B', '3', 'D', '_', 'A', 'P', 'R', 'T'},
    {'B', '3', 'D', '_', 'L', 'I', 'T', '1'},
    {'B', '3', 'D', '_', 'L', 'I', 'T', '2'},
    {'B', '3', 'D', '_', 'E', 'N', 'D', '1'},
    {'B', '3', 'D', '_', 'A', 'D', 'E', 'F'},
    {'B', '3', 'D', '_', 'A', 'M', 'A', 'P'},
    {'B', '3', 'D', '_', 'E', 'N', 'M', 'Y'},
    {'B', '3', 'D', '_', 'A', 'I', ' ', ' '},
    {'B', '3', 'D', '_', 'A', 'N', 'A', 'M'},
    {'B', '3', 'D', '_', 'S', 'N', 'A', 'M'},
    {'B', '3', 'D', '_', 'S', 'C', 'A', 'L'},
    {'B', '3', 'D', '_', 'S', 'P', 'L', 'A'},
    {'B', '3', 'D', '_', 'E', 'P', 'L', 'A'},
    {'B', '3', 'D', '_', 'S', 'R', 'E', 'F'},
    {'B', '3', 'D', '_', 'D', 'L', 'G', ' '},
    {'B', '3', 'D', '_', 'V', 'E', 'R', ' '},
    {'B', '3', 'D', ' ', 'T', 'M', 'A', 'P'},
    {'B', '3', 'D', ' ', 'T', 'L', 'I', 'B'},
    {'B', '3', 'D', '_', 'T', 'P', 'L', 'Y'},
    {'B', '3', 'D', '_', 'T', 'L', 'I', 'T'},
    {'B', '3', 'D', '_', 'T', 'A', 'N', 'M'},
    {'B', '3', 'D', '_', 'E', 'N', 'D', '2'},
    {'B', '3', 'D', '_', 'T', 'A', 'I', ' '},
    {'B', '3', 'D', '_', 'T', 'E', 'N', 'M'},
    {'B', '3', 'D', '_', 'T', 'A', 'F', 'N'},
    {'B', '3', 'D', '_', 'T', 'A', 'D', 'F'},
    {'B', '3', 'D', '_', 'T', 'A', 'M', 'P'},
    {'B', '3', 'D', '_', 'G', 'R', 'A', 'D'},
    {'B', '3', 'D', '_', 'T', 'P', 'L', 'C'},
    {'B', '3', 'D', '_', 'T', 'C', 'L', 'L'},
    {'B', '3', 'D', '_', 'M', 'E', 'N', 'T'},
    {'B', '3', 'D', '_', 'F', 'A', 'T', 'P'},
    {'B', '3', 'D', '_', 'T', 'E', 'P', 'L'},
    {'B', '3', 'D', '_', 'D', 'L', 'L', 'Y'},
    {'B', '3', 'D', '_', 'L', 'T', 'A', 'G'},
    {'B', '3', 'D', '_', 'T', 'H', 'I', 'N'},
    {'B', '3', 'D', '_', 'E', 'T', 'A', 'G'},
    {'J', 'O', 'N', 'C', 'H', 'U', 'N', 'K'}
};

static uint32_t filesys_read_u32(const uint8_t *source)
{
    uint32_t value;

    memcpy(&value, source, sizeof(value));
    return value;
}

static int32_t filesys_i32_from_bits(uint32_t bits)
{
    int32_t value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int32_t filesys_read_i32(const uint8_t *source)
{
    return filesys_i32_from_bits(filesys_read_u32(source));
}

static void filesys_write_u32(uint8_t *destination, uint32_t value)
{
    memcpy(destination, &value, sizeof(value));
}

static int32_t filesys_shift_fixed12(int32_t value)
{
    return filesys_i32_from_bits((uint32_t)value << 12);
}

void file_SetJonnyPostLoadHooks(
    JPBJonnyPostLoadHook clear_events,
    JPBJonnyPostLoadHook initialize_uvs)
{
    filesys_clear_events_hook = clear_events;
    filesys_initialize_uvs_hook = initialize_uvs;
}

void file_SetChunkLoadHooks(
    JPBChunkPathResolver resolve_path,
    JPBChunksPostLoadHook post_load)
{
    filesys_resolve_path_hook = resolve_path;
    filesys_post_load_hook = post_load;
}

void file_SetTextureLoadHook(JPBTextureLoadHook load_texture)
{
    filesys_load_texture_hook = load_texture;
}

static const char *filesys_resolve_resource(
    const char *name,
    int resource_type)
{
    if (filesys_resolve_path_hook != NULL) {
        return filesys_resolve_path_hook(name, resource_type, NULL);
    }
    return name;
}

static int filesys_load_fixed_resource(
    const char *name,
    int resource_type,
    void *destination,
    size_t capacity)
{
    const char *path = filesys_resolve_resource(name, resource_type);
    JPBFileHandle fd;
    uint64_t length;

    if (path == NULL || !file_OPEN((char *)path, &fd)) {
        gFileNotFound = 1;
        return 0;
    }
    length = file_GETSIZE(&fd);
    if (length > capacity) {
        gFileNotFound = 1;
        (void)file_CLOSE(&fd);
        return 0;
    }
    (void)file_READ(&fd, (char *)destination, (int32_t)length, 0);
    (void)file_CLOSE(&fd);
    return 1;
}

/* Reference RVA 0x986C0, 313 bytes. */
int file_LoadActorChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    int32_t sizes[512];
    int32_t count = (int32_t)chunk->size;
    uint8_t *size_table;
    uint8_t *record;
    int32_t span;
    int32_t index;
    char **actors;

    (void)flag;
    span = filesys_read_i32(*fd);
    *fd += sizeof(int32_t);
    gpWorld->nActor = count;
    if (count > 0) {
        actors = (char **)malloc((size_t)count * sizeof(*actors));
        size_table = *fd;
        *fd += (ptrdiff_t)span;
        memcpy(sizes, size_table, (size_t)count * sizeof(sizes[0]));
        record = size_table + (size_t)count * sizeof(int32_t);
        for (index = 0; index < count; ++index) {
            actors[index] = (char *)record;
            record += (ptrdiff_t)sizes[index];
        }
        gpWorld->apActorNames = actors;
    }
    return 1;
}

/* Reference RVA 0x98800, 380 bytes. */
int file_LoadAiChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    int32_t sizes[512];
    int32_t count = (int32_t)chunk->size;
    uint8_t *size_table;
    uint8_t *record;
    int32_t index;
    BAP_AI **ai_list;

    (void)flag;
    gpWorld->nAI = count;
    if (count > 0) {
        ai_list = (BAP_AI **)malloc((size_t)count * sizeof(*ai_list));
        size_table = *fd;
        *fd += chunk->realsize;
        memcpy(sizes, size_table, (size_t)count * sizeof(sizes[0]));
        record = size_table + (size_t)count * sizeof(int32_t);
        for (index = 0; index < count; ++index) {
            BAP_AI *ai = (BAP_AI *)record;
            int32_t unavailable = ai->numNodes - ai->numAvailable;
            uint8_t *variables = record + sizeof(BAP_AI);

            if (unavailable > 1) {
                variables +=
                    (size_t)(unavailable - 1) * sizeof(BAP_AINODE);
            }
            ai_list[index] = ai;
            ai->pVars = (uint32_t)addPtr(variables, JPB_POINTER_ARRAY_AI);
            record += (ptrdiff_t)sizes[index];
        }
        gpWorld->apAI = ai_list;
    }
    return 1;
}

/* Reference RVA 0x98980, 224 bytes. */
int file_LoadAnimDefChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    const uint8_t *size_table = *fd;
    uint32_t definition_index;

    (void)flag;
    gpWorld->nADef = (int32_t)chunk->size;
    if (chunk->size != 0) {
        *fd += (size_t)chunk->size * sizeof(int32_t);
        for (definition_index = 0;
             definition_index < chunk->size;
             ++definition_index) {
            wsl_BT_ANIMDEF *definition = (wsl_BT_ANIMDEF *)*fd;
            int32_t node_index;

            *fd += (ptrdiff_t)filesys_read_i32(
                size_table + (size_t)definition_index * sizeof(int32_t));
            gpWorld->animDef[definition_index] = definition;
            definition->numFrames =
                filesys_shift_fixed12(definition->numFrames);
            for (node_index = 0;
                 node_index < definition->totalNodes;
                 ++node_index) {
                wsl_BT_ANIMNODE *node = &definition->aNodes[node_index];
                int32_t entry_index;

                if (node->nodeSpeed == 0) {
                    node->nodeSpeed = 0x1000;
                }
                for (entry_index = 0;
                     entry_index < node->numEntries;
                     ++entry_index) {
                    node->aEntry[entry_index].frame =
                        filesys_shift_fixed12(
                            node->aEntry[entry_index].frame);
                }
            }
        }
    }
    return 1;
}

/* Reference RVA 0x98A60, 51 bytes. */
int file_LoadAnimMapChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)flag;
    gpWorld->nAnimMap = (int32_t)chunk->size;
    gpWorld->animMapEnemies = (int32_t *)*fd;
    *fd += (size_t)chunk->size * sizeof(int32_t);
    return 1;
}

static int filesys_relocate_chunk(
    int chunk_id,
    uint8_t **fd,
    wapChunk *chunk)
{
    switch (chunk_id) {
    case 7:
        (void)file_LoadMaterialChunk(fd, 0, chunk);
        break;
    case 0x0E:
        (void)file_LoadEnemyChunk(fd, 0, chunk);
        break;
    case 0x0F:
        (void)file_LoadAiChunk(fd, 0, chunk);
        break;
    case 0x10:
        (void)file_LoadActorChunk(fd, 0, chunk);
        break;
    case 0x17:
        (void)file_LoadVersionChunk(fd, 0, chunk);
        break;
    case 0x18:
        (void)file_LoadMapChunk(fd, 0, chunk);
        break;
    case 0x19:
        (void)file_LoadLibChunk(fd, 0, chunk);
        break;
    case 0x21:
        (void)file_LoadAnimDefChunk(fd, 0, chunk);
        break;
    case 0x22:
        (void)file_LoadAnimMapChunk(fd, 0, chunk);
        break;
    case 0x25:
        (void)file_LoadColorChunk(fd, 0, chunk);
        break;
    case 0x26:
        (void)file_LoadEntryChunk(fd, 0, chunk);
        break;
    case 0x27:
        (void)file_LoadFatChunk(fd, 0, chunk);
        break;
    case 0x28:
        (void)file_LoadEmiterChunk(fd, 0, chunk);
        break;
    case 0x29:
        (void)file_LoadDollyChunk(fd, 0, chunk);
        break;
    case 0x2A:
        (void)file_LoadTagChunk(fd, 0, chunk);
        break;
    case 0x2B:
        (void)file_LoadThinChunk(fd, 0, chunk);
        break;
    case 0x2C:
        (void)file_loadEntryTags(fd, 0, chunk);
        break;
    case 0x2D:
    {
        uint8_t *payload = *fd;

        (void)file_LoadJonnyChunk(fd, 0, chunk);
        /*
         * The reference handler advances past its 16-byte prefix and then
         * adds realsize. In shipped archives realsize covers the complete
         * payload and Jonny is terminal, so the inlined loop simply exits
         * 16 bytes beyond EOF. Keep the handler exact, but normalize the
         * bounded portable loop to the header-defined payload boundary.
         */
        *fd = payload + chunk->realsize;
        break;
    }
    default:
        return JPB_CHUNKS_UNSUPPORTED;
    }
    return JPB_CHUNKS_OK;
}

/*
 * Dependency-light form of the inlined dispatch loop at reference RVA
 * 0x98AA0. The added bounds/status surface makes corrupt archives observable
 * instead of reproducing the reference's cursor stall.
 */
int file_RelocateChunks(
    uint8_t *buffer,
    size_t buffer_size,
    uint8_t **end_cursor)
{
    uint8_t *cursor = buffer;
    uint8_t *end;

    if (end_cursor != NULL) {
        *end_cursor = buffer;
    }
    if (buffer == NULL) {
        return buffer_size == 0 ? JPB_CHUNKS_OK : JPB_CHUNKS_TRUNCATED;
    }
    end = buffer + buffer_size;
    while (cursor < end) {
        uint8_t *chunk_start = cursor;
        uint8_t *payload;
        wapChunk *chunk;
        int chunk_id;
        int result;

        if ((size_t)(end - cursor) < sizeof(wapChunk)) {
            if (end_cursor != NULL) {
                *end_cursor = cursor;
            }
            return JPB_CHUNKS_TRUNCATED;
        }
        chunk_id = readchunk(&cursor, &chunk);
        if (chunk_id < 0) {
            if (end_cursor != NULL) {
                *end_cursor = chunk_start;
            }
            return JPB_CHUNKS_UNKNOWN;
        }
        payload = cursor;
        result = filesys_relocate_chunk(chunk_id, &cursor, chunk);
        if (result != JPB_CHUNKS_OK) {
            if (end_cursor != NULL) {
                *end_cursor = chunk_start;
            }
            return result;
        }
        /*
         * Zero-sized chunks are valid header-only markers. The reference
         * loop has already advanced past the header in that case, even when
         * the selected handler has no records to consume.
         */
        if (cursor < payload ||
            (cursor == payload && chunk->realsize != 0)) {
            if (end_cursor != NULL) {
                *end_cursor = chunk_start;
            }
            return JPB_CHUNKS_STALLED;
        }
        if (cursor > end) {
            if (end_cursor != NULL) {
                *end_cursor = chunk_start;
            }
            return JPB_CHUNKS_TRUNCATED;
        }
    }
    if (end_cursor != NULL) {
        *end_cursor = cursor;
    }
    return JPB_CHUNKS_OK;
}

/* Reference RVA 0x98AA0, 2,843 bytes; portable ownership/path boundary. */
char *file_LoadChunks2Pool(
    char *dir,
    char *file,
    char *ext,
    int32_t *size,
    int loadtextures)
{
    char fallback_path[1024];
    const char *path;
    char *buffer;
    uint8_t *end_cursor;
    int32_t loaded_size = 0;
    int result;

    if (file == NULL) {
        return NULL;
    }
    if (filesys_resolve_path_hook != NULL) {
        path = filesys_resolve_path_hook(file, 5, ext);
    } else {
        const char *directory = dir != NULL ? dir : "";
        const char *extension = ext != NULL ? ext : "";
        int written = snprintf(
            fallback_path,
            sizeof(fallback_path),
            "%s%s%s",
            directory,
            file,
            extension);

        if (written < 0 || (size_t)written >= sizeof(fallback_path)) {
            return NULL;
        }
        path = fallback_path;
    }
    if (path == NULL) {
        return NULL;
    }
    buffer = file_LoadFile2PoolFunc(
        (char *)path,
        &loaded_size,
        MEMORY_POOL_ANY,
        __LINE__,
        (char *)__FILE__);
    if (buffer == NULL) {
        return NULL;
    }
    result = file_RelocateChunks(
        (uint8_t *)buffer, (size_t)(uint32_t)loaded_size, &end_cursor);
    if (result != JPB_CHUNKS_OK) {
        return NULL;
    }
    if (size != NULL) {
        *size = loaded_size;
    }
    if (filesys_post_load_hook != NULL) {
        filesys_post_load_hook(file, loadtextures);
    }
    return (char *)end_cursor;
}

/* Reference RVA 0x995C0, 54 bytes. */
int file_LoadColorChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)flag;
    gpWorld->pPalette = (int16_t *)*fd;
    *fd += 0x400;
    gpWorld->pColor = (char *)*fd;
    *fd += chunk->size;
    return 1;
}

/* Reference RVA 0x99600, 186 bytes. */
int file_LoadDollyChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    BAP_CAMERADOLLY temporary[32];
    BAP_CAMERADOLLY *source = (BAP_CAMERADOLLY *)*fd;

    (void)flag;
    (void)chunk;
    *fd += sizeof(temporary);
    memcpy(temporary, source, sizeof(temporary));
    gpWorld->pObiDolly = source;
    gpWorld->aDolly[0] = temporary[0];
    return 1;
}

/* Reference RVA 0x996C0, 542 bytes. */
void file_LoadEffects(void)
{
    int index;

    (void)filesys_load_fixed_resource(
        "project.eff",
        4,
        maProjTypes,
        sizeof(maProjTypes));
    for (index = 0; index < JPB_EFFECT_COUNT; ++index) {
        char name[256];
        const char *path;
        int32_t length = 0;

        (void)snprintf(name, sizeof(name), "f%d.eff", index);
        path = filesys_resolve_resource(name, 4);
        if (path == NULL) {
            paEffects[index] = NULL;
        } else {
            paEffects[index] = (EffectHeader *)(void *)
                file_LoadFile2PoolFunc(
                    (char *)path,
                    &length,
                    0,
                    __LINE__,
                    (char *)__FILE__);
        }
    }
    gMaxEffect = JPB_EFFECT_COUNT;
    (void)filesys_load_fixed_resource(
        "particle.prt",
        4,
        aEmiter,
        sizeof(aEmiter));
}

/* Reference RVA 0x998E0, 54 bytes. */
int file_LoadEmiterChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)flag;
    gpWorld->nPowerups = (int32_t)(chunk->size >> 4);
    if (gpWorld->nPowerups != 0) {
        gpWorld->pPowerups = (wsl_Powerup *)*fd;
        *fd += chunk->size;
    }
    return 1;
}

/* Reference RVA 0x99920, 401 bytes. */
int file_LoadEnemyChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    int32_t sizes[512];
    int32_t count;
    uint8_t *size_table;
    uint8_t *record;
    int32_t index;
    wsl_BAP_PLACEMENT **enemies;

    (void)flag;
    count = filesys_read_i32(*fd);
    *fd += sizeof(int32_t);
    gpWorld->nEnemy = count;
    if (count > 0) {
        enemies = (wsl_BAP_PLACEMENT **)malloc(
            (size_t)count * sizeof(*enemies));
        size_table = *fd;
        *fd += chunk->size;
        memcpy(sizes, size_table, (size_t)count * sizeof(sizes[0]));
        record = size_table + (size_t)count * sizeof(int32_t);
        for (index = 0; index < count; ++index) {
            enemies[index] = (wsl_BAP_PLACEMENT *)record;
            filesys_write_u32(record + 204, 0);
            filesys_write_u32(record + 208, UINT32_MAX);
            memcpy(record + 172, record + 236, sizeof(rdVECTOR));
            record += (ptrdiff_t)sizes[index];
        }
        gpWorld->apEnemy = enemies;
    }
    return 1;
}

/* Reference RVA 0x99AC0, 27 bytes. */
int file_LoadEntryChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)flag;
    gpWorld->pEntry = (wsl_mapEntry *)*fd;
    *fd += chunk->size;
    return 1;
}

/* Reference RVA 0x99AE0, 31 bytes. */
int file_LoadFatChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)flag;
    gpWorld->pFat = (wsl_fatPoly *)*fd;
    *fd += (size_t)chunk->size * 44;
    return 1;
}

/* Reference RVA 0x99B00, 118 bytes. */
int32_t file_LoadFile(char *name, void *buffer)
{
    JPBFileHandle fd;
    uint64_t length;

    if (!file_OPEN(name, &fd)) {
        gFileNotFound = 1;
        return 0;
    }
    length = file_GETSIZE(&fd);
    (void)file_READ(&fd, (char *)buffer, (int32_t)length, 0);
    (void)file_CLOSE(&fd);
    return (int32_t)(uint32_t)length;
}

/* Reference RVA 0x99B80, 180 bytes. */
char *file_LoadFile2PoolFunc(
    char *name, int32_t *size, int memtype, int line, char *file)
{
    JPBFileHandle fd;
    uint64_t length;
    char *buffer;

    if (!file_OPEN(name, &fd)) {
        return NULL;
    }
    length = file_GETSIZE(&fd);
    buffer = (char *)memory_gCallocMemoryFunc(
        (unsigned)(uint32_t)length,
        1,
        memtype,
        line,
        file);
    (void)file_READ(&fd, buffer, (int32_t)length, 0);
    *size = (int32_t)(uint32_t)length;
    (void)file_CLOSE(&fd);
    return buffer;
}

/* Reference RVA 0x99C40, 272 bytes. */
int file_LoadJonnyChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    uint8_t *header = *fd;
    uint32_t offset;

    (void)flag;
    memcpy(&gpWorld->bkColor, header, sizeof(gpWorld->bkColor));
    memcpy(&gpWorld->start, header + sizeof(CVECTOR), sizeof(gpWorld->start));
    *fd += sizeof(CVECTOR) + sizeof(rdVECTOR);

    jonnylevel = (char *)*fd;
    jpb_level_data_begin = *fd;
    jpb_level_data_end = *fd + chunk->realsize;
    *fd += chunk->realsize;
    leveldata = (int32_t *)(jonnylevel + 16);
    mapyend = filesys_read_i32((uint8_t *)jonnylevel + 8) >> 10;
    vertbase = (int32_t *)(
        (uint8_t *)leveldata +
        filesys_read_i32((uint8_t *)jonnylevel + 12));
    colorbase = (int32_t *)(
        (uint8_t *)leveldata +
        filesys_read_i32((uint8_t *)jonnylevel + 8));
    texturebase = (int32_t *)(
        (uint8_t *)leveldata +
        filesys_read_i32((uint8_t *)jonnylevel + 4));

    for (offset = 0; offset < 0x400; offset += sizeof(uint32_t)) {
        uint8_t *color_address = (uint8_t *)colorbase - offset;
        uint32_t color = filesys_read_u32(color_address);
        uint32_t red = ((color >> 8) & UINT32_C(0xFF)) * 2;
        uint32_t green = (color & UINT32_C(0xFF)) * 2;
        uint32_t blue = ((color >> 16) & UINT32_C(0xFF)) * 2;

        if (red > UINT32_C(0xFF)) {
            red = UINT32_C(0xFF);
        }
        if (green > UINT32_C(0xFF)) {
            green = UINT32_C(0xFF);
        }
        if (blue > UINT32_C(0xFF)) {
            blue = UINT32_C(0xFF);
        }
        filesys_write_u32(
            color_address, (green << 16) | (red << 8) | blue);
    }
    if (filesys_clear_events_hook != NULL) {
        filesys_clear_events_hook();
    }
    if (filesys_initialize_uvs_hook != NULL) {
        filesys_initialize_uvs_hook();
    }
    return 1;
}

/* Reference RVA 0x99D50, 223 bytes. */
int file_LoadLibChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    uint32_t index;

    (void)flag;
    /*
     * The reference allocates four bytes per entry, retaining a legacy
     * 32-bit pointer assumption in its x64 build. Allocate native pointers
     * here so the PC reconstruction is safe and the nxdk layout stays 4-byte.
     */
    gpWorld->pLib = (wsl_libPart **)memory_gCalloc(
        (unsigned)sizeof(*gpWorld->pLib), chunk->size);
    gpWorld->numLibs = (int32_t)chunk->size;
    for (index = 0; index < chunk->size; ++index) {
        wsl_libPart *part = (wsl_libPart *)*fd;
        uint32_t shared_count =
            (uint32_t)(part->numverts & UINT8_C(0x1F)) * 3;
        uint32_t high_vertices = (uint32_t)(part->numverts >> 5);

        *fd += sizeof(void *) * 2 + 8;
        if (high_vertices != 0) {
            shared_count = shared_count - 3 + high_vertices;
        }
        gpWorld->pLib[index] = part;
        *fd += (size_t)part->numpolys * sizeof(wsl_libPoly);
        part->index = (int32_t *)*fd;
        *fd += (size_t)part->numpolys * sizeof(int32_t);
        part->shared = (int16_t *)*fd;
        *fd += (size_t)shared_count * sizeof(int16_t);
        if ((shared_count & 1) != 0) {
            *fd += sizeof(int16_t);
        }
    }
    return 1;
}

/* Reference RVA 0x99E30, 239 bytes. */
int file_LoadMapChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    const uint8_t *map_data;
    uint32_t packed;
    int32_t span;

    (void)flag;
    gpWorld->sizeX = (int16_t)(chunk->size >> 8);
    gpWorld->sizeZ = (int16_t)(chunk->size & UINT32_C(0xFF));

    packed = filesys_read_u32(*fd);
    memcpy(&gpWorld->bkColor, &packed, sizeof(gpWorld->bkColor));
    *fd += sizeof(uint32_t);

    map_data = *fd;
    *fd += 5 * sizeof(int32_t);
    packed = filesys_read_u32(map_data + 12);
    gpWorld->minX = (int16_t)(uint16_t)packed;
    gpWorld->minZ = (int16_t)(uint16_t)(packed >> 16);
    packed = filesys_read_u32(map_data + 16);
    gpWorld->maxX = (int16_t)(uint16_t)packed;
    gpWorld->maxZ = (int16_t)(uint16_t)(packed >> 16);

    gpWorld->start.vx = filesys_i32_from_bits(
        filesys_read_u32(map_data) + (uint32_t)(int32_t)gpWorld->minX);
    gpWorld->start.vy =
        filesys_i32_from_bits(filesys_read_u32(map_data + 8));
    gpWorld->start.vz = filesys_i32_from_bits(
        filesys_read_u32(map_data + 4) +
        (uint32_t)(int32_t)gpWorld->minZ);

    span = filesys_i32_from_bits(filesys_read_u32(*fd));
    *fd += sizeof(int32_t);
    gpWorld->pNewMap = (wsl_mapSlot *)*fd;
    *fd += (ptrdiff_t)span;
    return 1;
}

/* Reference RVA 0x99F20, 47 bytes. */
int file_LoadMaterialChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)flag;
    gpWorld->pTexture = (wsl_BAP_TEXTURE *)*fd;
    *fd += chunk->size;
    gpWorld->numTexture = (int32_t)(chunk->size >> 4);
    return 0;
}

/* Reference RVA 0x99F50, 214 bytes. */
void file_LoadResidentSprites(void)
{
    int index;

    for (index = 0; index < JPB_RESIDENT_SPRITE_COUNT; ++index) {
        char filename[256];
        const char *path;
        uint32_t option = index < 39 ? 2 : 1;

        (void)snprintf(
            filename,
            sizeof(filename),
            "%s.tga",
            filesys_resident_sprite_names[index]);
        path = filesys_resolve_resource(filename, 2);
        effects1Handle[index] =
            filesys_load_texture_hook != NULL
                ? filesys_load_texture_hook(path, 1, option)
                : NULL;
    }
    transHandle =
        filesys_load_texture_hook != NULL
            ? filesys_load_texture_hook(NULL, 1, 1)
            : NULL;
    addHandle =
        filesys_load_texture_hook != NULL
            ? filesys_load_texture_hook(NULL, 1, 2)
            : NULL;
}

/* Reference RVA 0x9A030, 58 bytes. */
int file_LoadTagChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)flag;
    gpWorld->pLibTags = (wsl_libTags *)*fd;
    *fd += chunk->size;
    gpWorld->numTags = (int32_t)(chunk->size / 12);
    return 1;
}

/* Reference RVA 0x9A070, 45 bytes. */
int file_LoadThinChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)flag;
    gpWorld->pThin = (wsl_thinPoly *)*fd;
    *fd += (size_t)chunk->size * 20;
    numThins = (int32_t)chunk->size;
    return 1;
}

/* Reference RVA 0x9A0A0, 3 bytes. */
int file_LoadVersionChunk(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)fd;
    (void)flag;
    (void)chunk;
    return 0;
}

/* Reference RVA 0x9A0B0, 66 bytes. */
uint64_t file_getFileSize(char *name)
{
    JPBFileHandle fd;
    uint64_t length;

    if (!file_OPEN(name, &fd)) {
        return 0;
    }
    length = file_GETSIZE(&fd);
    (void)file_CLOSE(&fd);
    return length;
}

/* Reference RVA 0x9A100, 27 bytes. */
int file_loadEntryTags(uint8_t **fd, int32_t flag, wapChunk *chunk)
{
    (void)flag;
    gpWorld->pEntryTags = (wsl_entryTags *)*fd;
    *fd += chunk->size;
    return 1;
}

/* Reference RVA 0x9A120, 147 bytes. */
int readchunk(uint8_t **fd, wapChunk **chunk)
{
    size_t index;

    *chunk = (wapChunk *)*fd;
    *fd += sizeof(wapChunk);
    for (index = 0;
         index < sizeof(filesys_chunk_ids) / sizeof(filesys_chunk_ids[0]);
         ++index) {
        if (memcmp((*chunk)->id, filesys_chunk_ids[index], 8) == 0) {
            return (int)index;
        }
    }
    return -1;
}

