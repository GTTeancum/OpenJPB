#include "jpb/cad.h"
#include "jpb/io.h"

#include <limits.h>
#include <string.h>

static uint16_t cad_read_u16(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] |
           ((uint16_t)bytes[1] << 8);
}

static uint32_t cad_read_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static int cad_range_fits(
    uint32_t offset, size_t bytes, uint32_t payload_size)
{
    return offset <= payload_size &&
           bytes <= (size_t)payload_size - offset;
}

JPBCadResult jpb_CadInspect(
    void *buffer, size_t buffer_size, JPBCadView *view)
{
    uint8_t *file_data = (uint8_t *)buffer;
    uint8_t *payload;
    uint32_t payload_size;
    uint32_t bitstream_offset;
    uint32_t sequence_offset;
    uint32_t motion_offset;
    uint16_t part_count;
    uint16_t sequence_count;
    size_t sequence_bytes;
    size_t motion_bytes;

    if (buffer == NULL || view == NULL) {
        return JPB_CAD_INVALID_ARGUMENT;
    }
    memset(view, 0, sizeof(*view));
    if (buffer_size < 24) {
        return JPB_CAD_TRUNCATED;
    }

    payload_size = cad_read_u32(file_data);
    if (payload_size != buffer_size - 4) {
        return JPB_CAD_INVALID_SIZE;
    }
    payload = file_data + 4;
    if (((uintptr_t)payload & 3u) != 0) {
        return JPB_CAD_INVALID_LAYOUT;
    }
    bitstream_offset = cad_read_u32(payload);
    sequence_offset = cad_read_u32(payload + 4);
    motion_offset = cad_read_u32(payload + 8);
    part_count = cad_read_u16(payload + 12);
    sequence_count = cad_read_u16(payload + 16);
    sequence_bytes =
        (size_t)sequence_count * sizeof(_animTemplate);
    motion_bytes =
        (size_t)sequence_count * sizeof(Motion);

    if (!cad_range_fits(
            sequence_offset, sequence_bytes, payload_size) ||
        !cad_range_fits(
            bitstream_offset, 8, payload_size) ||
        !cad_range_fits(
            motion_offset, motion_bytes, payload_size)) {
        return JPB_CAD_INVALID_OFFSET;
    }
    if (sequence_offset < 20 ||
        sequence_offset + sequence_bytes !=
            bitstream_offset ||
        bitstream_offset > motion_offset ||
        motion_offset + motion_bytes != payload_size ||
        (sequence_offset & 3u) != 0 ||
        (bitstream_offset & 3u) != 0 ||
        (motion_offset & 3u) != 0) {
        return JPB_CAD_INVALID_LAYOUT;
    }

    view->file_data = file_data;
    view->file_size = buffer_size;
    view->payload = (char *)payload;
    view->payload_size = payload_size;
    view->bitstream_offset = bitstream_offset;
    view->sequence_offset = sequence_offset;
    view->motion_offset = motion_offset;
    view->part_count = part_count;
    view->sequence_count = sequence_count;
    view->sequences =
        (_animTemplate *)(payload + sequence_offset);
    view->motions =
        (Motion *)(payload + motion_offset);
    view->bitstream = payload + bitstream_offset;
    view->bitstream_size =
        (size_t)motion_offset - bitstream_offset;
    view->depack_window_size =
        (size_t)payload_size - bitstream_offset;
    return JPB_CAD_OK;
}

JPBCadResult jpb_CadLoadFile(
    const char *path,
    void *storage,
    size_t storage_capacity,
    JPBCadView *view)
{
    JPBFileHandle file = 0;
    uint64_t file_size;

    if (view != NULL) {
        memset(view, 0, sizeof(*view));
    }
    if (path == NULL || storage == NULL || view == NULL) {
        return JPB_CAD_INVALID_ARGUMENT;
    }
    if (!file_OPEN((char *)path, &file)) {
        return JPB_CAD_IO_ERROR;
    }
    file_size = file_GETSIZE(&file);
    if (file_size > storage_capacity ||
        file_size > INT32_MAX) {
        (void)file_CLOSE(&file);
        return JPB_CAD_STORAGE_TOO_SMALL;
    }
    if (file_READ(
            &file,
            (char *)storage,
            (int32_t)file_size,
            JPB_FILE_READ_STREAM) != file_size) {
        (void)file_CLOSE(&file);
        return JPB_CAD_IO_ERROR;
    }
    (void)file_CLOSE(&file);
    return jpb_CadInspect(storage, (size_t)file_size, view);
}
