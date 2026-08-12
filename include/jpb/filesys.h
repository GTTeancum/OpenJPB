#ifndef JPB_FILESYS_H
#define JPB_FILESYS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Direct PDB type 0x6DC4. */
typedef struct wapChunk {
    uint8_t id[8];
    uint32_t size;
    uint32_t realsize;
} wapChunk;

typedef const char *(*JPBChunkPathResolver)(
    const char *resource_name,
    int resource_type,
    const char *extension);
typedef void (*JPBChunksPostLoadHook)(
    const char *resource_name,
    int loadtextures);

enum JPBChunkRelocateResult {
    JPB_CHUNKS_OK = 0,
    JPB_CHUNKS_TRUNCATED = -1,
    JPB_CHUNKS_UNKNOWN = -2,
    JPB_CHUNKS_UNSUPPORTED = -3,
    JPB_CHUNKS_STALLED = -4
};

extern int gFileNotFound;

void file_SetChunkLoadHooks(
    JPBChunkPathResolver resolve_path,
    JPBChunksPostLoadHook post_load);
void file_LoadEffects(void);
void file_LoadResidentSprites(void);
char *file_LoadChunks2Pool(
    char *dir,
    char *file,
    char *ext,
    int32_t *size,
    int loadtextures);
int file_LoadActorChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadAiChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadAnimDefChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadAnimMapChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadColorChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadDollyChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadEmiterChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadEnemyChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadEntryChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadFatChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int32_t file_LoadFile(char *name, void *buffer);
char *file_LoadFile2PoolFunc(
    char *name, int32_t *size, int memtype, int line, char *file);
int file_LoadJonnyChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadLibChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadMapChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadMaterialChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadTagChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadThinChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_LoadVersionChunk(uint8_t **fd, int32_t flag, wapChunk *chunk);
int file_RelocateChunks(
    uint8_t *buffer,
    size_t buffer_size,
    uint8_t **end_cursor);
uint64_t file_getFileSize(char *name);
int file_loadEntryTags(uint8_t **fd, int32_t flag, wapChunk *chunk);
int readchunk(uint8_t **fd, wapChunk **chunk);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_FILESYS_STATIC_ASSERT(condition, message) \
    static_assert(condition, message)
#else
#define JPB_FILESYS_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)
#endif

JPB_FILESYS_STATIC_ASSERT(
    offsetof(wapChunk, id) == 0, "wapChunk.id layout changed");
JPB_FILESYS_STATIC_ASSERT(
    offsetof(wapChunk, size) == 8, "wapChunk.size layout changed");
JPB_FILESYS_STATIC_ASSERT(
    offsetof(wapChunk, realsize) == 12, "wapChunk.realsize layout changed");
JPB_FILESYS_STATIC_ASSERT(sizeof(wapChunk) == 16, "wapChunk size changed");

#undef JPB_FILESYS_STATIC_ASSERT

#endif
