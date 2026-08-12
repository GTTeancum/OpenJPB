#ifndef JPB_CAD_H
#define JPB_CAD_H

#include "jpb/anim.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum JPBCadResult {
    JPB_CAD_OK = 0,
    JPB_CAD_INVALID_ARGUMENT = -1,
    JPB_CAD_TRUNCATED = -2,
    JPB_CAD_INVALID_SIZE = -3,
    JPB_CAD_INVALID_OFFSET = -4,
    JPB_CAD_INVALID_LAYOUT = -5,
    JPB_CAD_IO_ERROR = -6,
    JPB_CAD_STORAGE_TOO_SMALL = -7
} JPBCadResult;

enum { JPB_CAD_REFERENCE_CAPACITY = 0x40000 };

typedef struct JPBCadView {
    uint8_t *file_data;
    size_t file_size;
    char *payload;
    uint32_t payload_size;
    uint32_t bitstream_offset;
    uint32_t sequence_offset;
    uint32_t motion_offset;
    uint16_t part_count;
    uint16_t sequence_count;
    _animTemplate *sequences;
    Motion *motions;
    uint8_t *bitstream;
    /* Bytes before Motion records; useful for structural inspection. */
    size_t bitstream_size;
    /*
     * Full source window visible to the original unchecked decoder. Some
     * shipped terminal sequences intentionally consume bytes overlapping
     * the following Motion table.
     */
    size_t depack_window_size;
} JPBCadView;

JPBCadResult jpb_CadInspect(
    void *buffer, size_t buffer_size, JPBCadView *view);
JPBCadResult jpb_CadLoadFile(
    const char *path,
    void *storage,
    size_t storage_capacity,
    JPBCadView *view);

#ifdef __cplusplus
}
#endif

#endif
