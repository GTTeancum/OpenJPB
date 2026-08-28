/*
 * COMPLETE REVIEWED RECONSTRUCTION.
 * PDB module: 0092
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\theoraplay.obj
 * Primary source: W:\SWJediPowerBattles\work\video\theoraplay.c
 * Compiler language: c
 * Emitted procedures: 23
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/theoraplay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xmmintrin.h>

#if defined(JPB_THEORAPLAY_CODEC_AVAILABLE)
#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <vorbis/codec.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if !defined(JPB_THEORAPLAY_CODEC_AVAILABLE)
typedef struct th_info {
    unsigned char version_major;
    unsigned char version_minor;
    unsigned char version_subminor;
    unsigned int frame_width;
    unsigned int frame_height;
    unsigned int pic_width;
    unsigned int pic_height;
    unsigned int pic_x;
    unsigned int pic_y;
    unsigned int fps_numerator;
    unsigned int fps_denominator;
    unsigned int aspect_numerator;
    unsigned int aspect_denominator;
    int colorspace;
    int pixel_fmt;
    int target_bitrate;
    int quality;
    int keyframe_granule_shift;
} th_info;

typedef struct th_img_plane {
    int width;
    int height;
    int stride;
    unsigned char *data;
} th_img_plane;
#endif

typedef unsigned char *(*THEORAPLAY_VideoConvert)(
    const th_info *info, const th_img_plane *planes);

_Static_assert(sizeof(th_info) == 64, "th_info must match the PDB layout");
_Static_assert(
    offsetof(th_info, pic_width) == 12,
    "th_info.pic_width offset changed");
_Static_assert(
    sizeof(th_img_plane) == 24,
    "th_img_plane must match the PDB layout");
_Static_assert(
    offsetof(th_img_plane, data) == 16,
    "th_img_plane.data offset changed");

struct TheoraDecoder {
    int thread_created;
    HANDLE lock;
    volatile int halt;
    int thread_done;
    HANDLE worker;
    THEORAPLAY_Io *io;
    unsigned int maxframes;
    volatile unsigned int prepped;
    volatile unsigned int videocount;
    volatile unsigned int audioms;
    volatile int hasvideo;
    volatile int hasaudio;
    volatile int decode_error;
    THEORAPLAY_VideoFormat vidfmt;
    THEORAPLAY_VideoConvert vidcvt;
    THEORAPLAY_VideoFrame *videolist;
    THEORAPLAY_VideoFrame *videolisttail;
    THEORAPLAY_AudioPacket *audiolist;
    THEORAPLAY_AudioPacket *audiolisttail;
};

_Static_assert(
    sizeof(struct TheoraDecoder) == 112,
    "TheoraDecoder must match the PDB layout");
_Static_assert(
    offsetof(struct TheoraDecoder, lock) == 8,
    "TheoraDecoder.lock offset changed");
_Static_assert(
    offsetof(struct TheoraDecoder, prepped) == 44,
    "TheoraDecoder.prepped offset changed");
_Static_assert(
    offsetof(struct TheoraDecoder, videolist) == 80,
    "TheoraDecoder.videolist offset changed");
_Static_assert(
    offsetof(struct TheoraDecoder, audiolist) == 96,
    "TheoraDecoder.audiolist offset changed");

static void theoraplay_lock(THEORAPLAY_Decoder *decoder)
{
    (void)WaitForSingleObject(decoder->lock, INFINITE);
}

static void theoraplay_unlock(THEORAPLAY_Decoder *decoder)
{
    (void)ReleaseMutex(decoder->lock);
}

/* 0x1062A0, 31 bytes. */
static unsigned char *ConvertVideoFrame420ToIYUV(
    const th_info *info, const th_img_plane *planes);

static unsigned char theoraplay_convert_component(float component)
{
    int rounded;

    if (component < 0.0f) {
        component = 0.0f;
    }
    else if (component > 255.0f) {
        component = 255.0f;
    }
    rounded = _mm_cvtss_si32(_mm_set_ss(component));
    return (unsigned char)rounded;
}

static unsigned char *ConvertVideoFrame420ToRGBCommon(
    const th_info *info, const th_img_plane *planes)
{
    static const float chroma_scale = 0.004464286f;
    static const float luma_scale = 0.00456621f;
    static const float green_cb_scale = 87.754745f;
    static const float green_cr_scale = 182.10474f;
    static const float red_scale = 357.50998f;
    static const float blue_scale = 451.86f;
    unsigned char *pixels = (unsigned char *)malloc(
        (size_t)info->pic_width * info->pic_height * 4U);
    unsigned int row;

    if (pixels == NULL) {
        return NULL;
    }
    for (row = 0; row < info->pic_height; ++row) {
        const unsigned char *y_source =
            planes[0].data + (size_t)row * planes[0].stride;
        const unsigned char *cb_source =
            planes[1].data + (size_t)(row >> 1) * planes[1].stride;
        const unsigned char *cr_source =
            planes[2].data + (size_t)(row >> 1) * planes[2].stride;
        unsigned char *destination = pixels +
            (size_t)(info->pic_height - row - 1U) * info->pic_width * 4U;
        unsigned int column;

        for (column = 0; column < info->pic_width; ++column) {
            const float luma =
                ((float)y_source[column] - 16.0f) * luma_scale * 255.0f;
            const float cb =
                ((float)cb_source[column >> 1] - 128.0f) * chroma_scale;
            const float cr =
                ((float)cr_source[column >> 1] - 128.0f) * chroma_scale;

            destination[column * 4U] = theoraplay_convert_component(
                cr * red_scale + luma);
            destination[column * 4U + 1U] = theoraplay_convert_component(
                (luma - cb * green_cb_scale) - cr * green_cr_scale);
            destination[column * 4U + 2U] = theoraplay_convert_component(
                cb * blue_scale + luma);
            destination[column * 4U + 3U] = 255;
        }
    }
    return pixels;
}

/* 0x1062C0, 1867 bytes. */
static unsigned char *ConvertVideoFrame420ToRGB(
    const th_info *info, const th_img_plane *planes)
{
    return ConvertVideoFrame420ToRGBCommon(info, planes);
}

/* 0x106A10, 1867 bytes; machine-identical to the RGB converter. */
static unsigned char *ConvertVideoFrame420ToRGBA(
    const th_info *info, const th_img_plane *planes)
{
    return ConvertVideoFrame420ToRGBCommon(info, planes);
}

/* 0x107160, 453 bytes. */
static unsigned char *ConvertVideoFrame420ToYUVPlanar(
    const th_info *info,
    const th_img_plane *planes,
    int plane_0,
    int plane_1,
    int plane_2)
{
    const unsigned int luma_x = info->pic_x & ~1U;
    const unsigned int luma_y = info->pic_y & ~1U;
    const int chroma_offset =
        (int)(info->pic_y >> 1) * planes[1].stride +
        (int)(info->pic_x >> 1);
    unsigned char *pixels = (unsigned char *)malloc(
        (size_t)info->pic_width * info->pic_height * 2U);
    unsigned char *destination = pixels;
    unsigned int row;

    if (pixels == NULL) {
        return NULL;
    }
    for (row = 0; row < info->pic_height; ++row) {
        memcpy(
            destination,
            planes[plane_0].data +
                (size_t)(luma_y + row) * planes[plane_0].stride + luma_x,
            info->pic_width);
        destination += info->pic_width;
    }
    for (row = 0; row < info->pic_height / 2U; ++row) {
        memcpy(
            destination,
            planes[plane_1].data + chroma_offset +
                (size_t)row * planes[plane_1].stride,
            info->pic_width / 2U);
        destination += info->pic_width / 2U;
    }
    for (row = 0; row < info->pic_height / 2U; ++row) {
        memcpy(
            destination,
            planes[plane_2].data + chroma_offset +
                (size_t)row * planes[plane_2].stride,
            info->pic_width / 2U);
        destination += info->pic_width / 2U;
    }
    return pixels;
}

/* 0x107330, 31 bytes. */
static unsigned char *ConvertVideoFrame420ToYV12(
    const th_info *info, const th_img_plane *planes)
{
    return ConvertVideoFrame420ToYUVPlanar(info, planes, 0, 2, 1);
}

static unsigned char *ConvertVideoFrame420ToIYUV(
    const th_info *info, const th_img_plane *planes)
{
    return ConvertVideoFrame420ToYUVPlanar(info, planes, 0, 1, 2);
}

/* 0x107350, 31 bytes. */
static void IoFopenClose(THEORAPLAY_Io *io)
{
    (void)fclose((FILE *)io->userdata);
    free(io);
}

/* 0x107370, 75 bytes. */
static long IoFopenRead(
    THEORAPLAY_Io *io, void *buffer, long buffer_length)
{
    long result = (long)fread(
        buffer, 1, (size_t)buffer_length, (FILE *)io->userdata);

    if (result == 0 && ferror((FILE *)io->userdata)) {
        result = -1;
    }
    return result;
}

/* 0x1073C0, 68 bytes. */
unsigned int THEORAPLAY_availableAudio(THEORAPLAY_Decoder *decoder)
{
    unsigned int available = 0;

    if (decoder != NULL) {
        theoraplay_lock(decoder);
        available = decoder->audioms;
        theoraplay_unlock(decoder);
    }
    return available;
}

/* 0x107410, 68 bytes. */
unsigned int THEORAPLAY_availableVideo(THEORAPLAY_Decoder *decoder)
{
    unsigned int available = 0;

    if (decoder != NULL) {
        theoraplay_lock(decoder);
        available = decoder->videocount;
        theoraplay_unlock(decoder);
    }
    return available;
}

/* 0x107460, 68 bytes. */
int THEORAPLAY_decodingError(THEORAPLAY_Decoder *decoder)
{
    int failed = 0;

    if (decoder != NULL) {
        theoraplay_lock(decoder);
        failed = decoder->decode_error;
        theoraplay_unlock(decoder);
    }
    return failed;
}

/* 0x1074B0, 36 bytes. */
void THEORAPLAY_freeAudio(const THEORAPLAY_AudioPacket *packet)
{
    if (packet != NULL) {
        free(packet->samples);
        free((void *)packet);
    }
}

/* 0x1074E0, 36 bytes. */
void THEORAPLAY_freeVideo(const THEORAPLAY_VideoFrame *frame)
{
    if (frame != NULL) {
        free(frame->pixels);
        free((void *)frame);
    }
}

/* 0x107510, 93 bytes. */
const THEORAPLAY_AudioPacket *THEORAPLAY_getAudio(
    THEORAPLAY_Decoder *decoder)
{
    THEORAPLAY_AudioPacket *packet;

    theoraplay_lock(decoder);
    packet = decoder->audiolist;
    if (packet != NULL) {
        decoder->audioms -= packet->playms;
        decoder->audiolist = packet->next;
        packet->next = NULL;
        if (decoder->audiolist == NULL) {
            decoder->audiolisttail = NULL;
        }
    }
    theoraplay_unlock(decoder);
    return packet;
}

/* 0x107570, 92 bytes. */
const THEORAPLAY_VideoFrame *THEORAPLAY_getVideo(
    THEORAPLAY_Decoder *decoder)
{
    THEORAPLAY_VideoFrame *frame;

    theoraplay_lock(decoder);
    frame = decoder->videolist;
    if (frame != NULL) {
        decoder->videolist = frame->next;
        if (decoder->videolist == NULL) {
            decoder->videolisttail = NULL;
        }
        --decoder->videocount;
        frame->next = NULL;
    }
    theoraplay_unlock(decoder);
    return frame;
}

/* 0x1075D0, 68 bytes. */
int THEORAPLAY_hasAudioStream(THEORAPLAY_Decoder *decoder)
{
    int has_audio = 0;

    if (decoder != NULL) {
        theoraplay_lock(decoder);
        has_audio = decoder->hasaudio;
        theoraplay_unlock(decoder);
    }
    return has_audio;
}

/* 0x107620, 68 bytes. */
int THEORAPLAY_hasVideoStream(THEORAPLAY_Decoder *decoder)
{
    int has_video = 0;

    if (decoder != NULL) {
        theoraplay_lock(decoder);
        has_video = decoder->hasvideo;
        theoraplay_unlock(decoder);
    }
    return has_video;
}

/* 0x107670, 84 bytes. */
int THEORAPLAY_isDecoding(THEORAPLAY_Decoder *decoder)
{
    int decoding = 0;

    if (decoder != NULL) {
        theoraplay_lock(decoder);
        decoding = decoder->audiolist != NULL ||
                   decoder->videolist != NULL ||
                   (decoder->thread_created && !decoder->thread_done);
        theoraplay_unlock(decoder);
    }
    return decoding;
}

/* 0x1076D0, 68 bytes. */
int THEORAPLAY_isInitialized(THEORAPLAY_Decoder *decoder)
{
    int initialized = 0;

    if (decoder != NULL) {
        theoraplay_lock(decoder);
        initialized = (int)decoder->prepped;
        theoraplay_unlock(decoder);
    }
    return initialized;
}

#if defined(JPB_THEORAPLAY_CODEC_AVAILABLE)
static void WorkerThread(THEORAPLAY_Decoder *decoder);

static void *WorkerThreadEntry(void *userdata)
{
    WorkerThread((THEORAPLAY_Decoder *)userdata);
    return NULL;
}

/* 0x107720, 294 bytes, global, 5 named locals
 * THEORAPLAY_startDecode
 * PDB type: THEORAPLAY_Decoder* (THEORAPLAY_...
 * Source: W:\SWJediPowerBattles\work\video\theoraplay.c
 */
THEORAPLAY_Decoder *THEORAPLAY_startDecode(
    THEORAPLAY_Io *io,
    unsigned int maxframes,
    THEORAPLAY_VideoFormat format)
{
    THEORAPLAY_Decoder *decoder = NULL;
    THEORAPLAY_VideoConvert converter = NULL;

    switch (format) {
    case THEORAPLAY_VIDFMT_YV12:
        converter = ConvertVideoFrame420ToYV12;
        break;
    case THEORAPLAY_VIDFMT_IYUV:
        converter = ConvertVideoFrame420ToIYUV;
        break;
    case THEORAPLAY_VIDFMT_RGB:
        converter = ConvertVideoFrame420ToRGB;
        break;
    case THEORAPLAY_VIDFMT_RGBA:
        converter = ConvertVideoFrame420ToRGBA;
        break;
    default:
        goto startdecode_failed;
    }
    decoder = (THEORAPLAY_Decoder *)malloc(sizeof(*decoder));
    if (decoder == NULL) {
        goto startdecode_failed;
    }
    memset(decoder, 0, sizeof(*decoder));
    decoder->maxframes = maxframes;
    decoder->vidfmt = format;
    decoder->vidcvt = converter;
    decoder->io = io;
    decoder->lock = CreateMutexA(NULL, FALSE, NULL);
    if (decoder->lock != NULL) {
        decoder->worker = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)WorkerThreadEntry,
            decoder,
            0,
            NULL);
        decoder->thread_created = decoder->worker != NULL;
        if (decoder->thread_created) {
            return decoder;
        }
    }
    (void)CloseHandle(decoder->lock);

startdecode_failed:
    io->close(io);
    free(decoder);
    return NULL;
}

/* 0x107850, 405 bytes, global, 7 named locals
 * THEORAPLAY_startDecodeFile
 * PDB type: THEORAPLAY_Decoder* (const char*...
 * Source: W:\SWJediPowerBattles\work\video\theoraplay.c
 */

/* 0x1079F0, 197 bytes, global, 6 named locals
 * THEORAPLAY_stopDecode
 * PDB type: void (THEORAPLAY_Decoder*)
 * Source: W:\SWJediPowerBattles\work\video\theoraplay.c
 */
THEORAPLAY_Decoder *THEORAPLAY_startDecodeFile(
    const char *filename,
    unsigned int maxframes,
    THEORAPLAY_VideoFormat format)
{
    THEORAPLAY_Io *io = (THEORAPLAY_Io *)malloc(sizeof(*io));
    FILE *file;

    if (io == NULL) {
        return NULL;
    }
    file = fopen(filename, "rb");
    if (file == NULL) {
        free(io);
        return NULL;
    }
    io->read = IoFopenRead;
    io->close = IoFopenClose;
    io->userdata = file;
    return THEORAPLAY_startDecode(io, maxframes, format);
}
#endif
void THEORAPLAY_stopDecode(THEORAPLAY_Decoder *decoder)
{
    THEORAPLAY_VideoFrame *video;
    THEORAPLAY_AudioPacket *audio;

    if (decoder == NULL) {
        return;
    }
    if (decoder->thread_created) {
        HANDLE worker = decoder->worker;

        decoder->halt = 1;
        (void)WaitForSingleObject(worker, INFINITE);
        (void)CloseHandle(worker);
        (void)CloseHandle(decoder->lock);
    }
    video = decoder->videolist;
    while (video != NULL) {
        THEORAPLAY_VideoFrame *next = video->next;

        free(video->pixels);
        free(video);
        video = next;
    }
    audio = decoder->audiolist;
    while (audio != NULL) {
        THEORAPLAY_AudioPacket *next = audio->next;

        free(audio->samples);
        free(audio);
        audio = next;
    }
    free(decoder);
}

/* 0x107AC0, 3070 bytes, local, 46 named locals
 * WorkerThread
 * PDB type: void (TheoraDecoder*)
 * Source: W:\SWJediPowerBattles\work\video\theoraplay.c
 */
#if defined(JPB_THEORAPLAY_CODEC_AVAILABLE)
static int FeedMoreOggData(THEORAPLAY_Io *io, ogg_sync_state *sync)
{
    long buffer_length = 4096;
    char *buffer = ogg_sync_buffer(sync, buffer_length);

    if (buffer == NULL) {
        return -1;
    }
    buffer_length = io->read(io, buffer, buffer_length);
    if (buffer_length <= 0) {
        return 0;
    }
    return ogg_sync_wrote(sync, buffer_length) == 0 ? 1 : -1;
}

static void WorkerThread(THEORAPLAY_Decoder *decoder)
{
    unsigned long audioframes = 0;
    unsigned long videoframes = 0;
    double fps = 0.0;
    int was_error = 1;
    int eos = 0;
    ogg_packet packet;
    ogg_sync_state sync;
    ogg_page page;
    int vpackets = 0;
    vorbis_info vinfo;
    vorbis_comment vcomment;
    ogg_stream_state vstream;
    int vdsp_init = 0;
    vorbis_dsp_state vdsp;
    int tpackets = 0;
    th_info tinfo;
    th_comment tcomment;
    ogg_stream_state tstream;
    int vblock_init = 0;
    vorbis_block vblock;
    th_dec_ctx *tdec = NULL;
    th_setup_info *tsetup = NULL;
    int bos = 1;

    ogg_sync_init(&sync);
    vorbis_info_init(&vinfo);
    vorbis_comment_init(&vcomment);
    th_comment_init(&tcomment);
    th_info_init(&tinfo);

#define QUEUE_OGG_PAGE()                                                     \
    do {                                                                     \
        if (tpackets) {                                                      \
            (void)ogg_stream_pagein(&tstream, &page);                        \
        }                                                                    \
        if (vpackets) {                                                      \
            (void)ogg_stream_pagein(&vstream, &page);                        \
        }                                                                    \
    } while (0)

    while (!decoder->halt && bos) {
        if (FeedMoreOggData(decoder->io, &sync) <= 0) {
            goto cleanup;
        }
        while (!decoder->halt && ogg_sync_pageout(&sync, &page) > 0) {
            ogg_stream_state test;

            if (!ogg_page_bos(&page)) {
                QUEUE_OGG_PAGE();
                bos = 0;
                break;
            }
            (void)ogg_stream_init(&test, ogg_page_serialno(&page));
            (void)ogg_stream_pagein(&test, &page);
            (void)ogg_stream_packetout(&test, &packet);
            if (!tpackets &&
                th_decode_headerin(
                    &tinfo, &tcomment, &tsetup, &packet) >= 0) {
                memcpy(&tstream, &test, sizeof(test));
                tpackets = 1;
            }
            else if (!vpackets &&
                     vorbis_synthesis_headerin(
                         &vinfo, &vcomment, &packet) >= 0) {
                memcpy(&vstream, &test, sizeof(test));
                vpackets = 1;
            }
            else {
                (void)ogg_stream_clear(&test);
            }
        }
    }
    if (decoder->halt || (!vpackets && !tpackets)) {
        goto cleanup;
    }

    while (!decoder->halt &&
           ((tpackets && tpackets < 3) ||
            (vpackets && vpackets < 3))) {
        while (!decoder->halt && tpackets && tpackets < 3) {
            if (ogg_stream_packetout(&tstream, &packet) != 1) {
                break;
            }
            if (!th_decode_headerin(
                    &tinfo, &tcomment, &tsetup, &packet)) {
                goto cleanup;
            }
            ++tpackets;
        }
        while (!decoder->halt && vpackets && vpackets < 3) {
            if (ogg_stream_packetout(&vstream, &packet) != 1) {
                break;
            }
            if (vorbis_synthesis_headerin(
                    &vinfo, &vcomment, &packet)) {
                goto cleanup;
            }
            ++vpackets;
        }
        if (ogg_sync_pageout(&sync, &page) > 0) {
            QUEUE_OGG_PAGE();
        }
        else if (FeedMoreOggData(decoder->io, &sync) <= 0) {
            goto cleanup;
        }
    }

    if (!decoder->halt && tpackets) {
        int pp_level_max = 0;

        if (tinfo.frame_width > 99999 || tinfo.frame_height > 99999 ||
            (tinfo.colorspace != TH_CS_UNSPECIFIED &&
             tinfo.colorspace != TH_CS_ITU_REC_470M &&
             tinfo.colorspace != TH_CS_ITU_REC_470BG) ||
            tinfo.pixel_fmt != TH_PF_420) {
            goto cleanup;
        }
        if (tinfo.fps_denominator != 0) {
            fps = (double)tinfo.fps_numerator /
                (double)tinfo.fps_denominator;
        }
        tdec = th_decode_alloc(&tinfo, tsetup);
        if (tdec == NULL) {
            goto cleanup;
        }
        (void)th_decode_ctl(
            tdec,
            TH_DECCTL_SET_PPLEVEL,
            &pp_level_max,
            sizeof(pp_level_max));
    }
    if (tsetup != NULL) {
        th_setup_free(tsetup);
        tsetup = NULL;
    }
    if (!decoder->halt && vpackets) {
        vdsp_init = vorbis_synthesis_init(&vdsp, &vinfo) == 0;
        if (!vdsp_init) {
            goto cleanup;
        }
        vblock_init = vorbis_block_init(&vdsp, &vblock) == 0;
        if (!vblock_init) {
            goto cleanup;
        }
    }

    theoraplay_lock(decoder);
    decoder->prepped = 1;
    decoder->hasvideo = tpackets != 0;
    decoder->hasaudio = vpackets != 0;
    theoraplay_unlock(decoder);

    while (!decoder->halt && !eos) {
        int need_pages = 0;
        int saw_video_frame = 0;

        while (!decoder->halt && vpackets) {
            float **pcm = NULL;
            const int frames = vorbis_synthesis_pcmout(&vdsp, &pcm);

            if (frames > 0) {
                const int channels = vinfo.channels;
                THEORAPLAY_AudioPacket *item =
                    (THEORAPLAY_AudioPacket *)malloc(sizeof(*item));
                float *samples;
                int frame_index;

                if (item == NULL) {
                    goto cleanup;
                }
                item->playms = (unsigned long)(
                    ((double)audioframes / (double)vinfo.rate) * 1000.0);
                item->channels = channels;
                item->freq = vinfo.rate;
                item->frames = frames;
                item->samples = (float *)malloc(
                    sizeof(float) * (size_t)frames * (size_t)channels);
                item->next = NULL;
                if (item->samples == NULL) {
                    free(item);
                    goto cleanup;
                }
                samples = item->samples;
                for (frame_index = 0; frame_index < frames; ++frame_index) {
                    int channel_index;

                    for (channel_index = 0;
                         channel_index < channels;
                         ++channel_index) {
                        *samples++ = pcm[channel_index][frame_index];
                    }
                }
                (void)vorbis_synthesis_read(&vdsp, frames);
                audioframes += (unsigned long)frames;
                theoraplay_lock(decoder);
                decoder->audioms += item->playms;
                if (decoder->audiolisttail != NULL) {
                    decoder->audiolisttail->next = item;
                }
                else {
                    decoder->audiolist = item;
                }
                decoder->audiolisttail = item;
                theoraplay_unlock(decoder);
            }
            else {
                if (ogg_stream_packetout(&vstream, &packet) <= 0) {
                    if (!tpackets) {
                        need_pages = 1;
                    }
                    break;
                }
                if (vorbis_synthesis(&vblock, &packet) == 0) {
                    (void)vorbis_synthesis_blockin(&vdsp, &vblock);
                }
            }
        }

        if (!decoder->halt && tpackets) {
            if (ogg_stream_packetout(&tstream, &packet) <= 0) {
                need_pages = 1;
            }
            else {
                ogg_int64_t granule_position = 0;
                const int result = th_decode_packetin(
                    tdec, &packet, &granule_position);

                if (result == TH_DUPFRAME) {
                    ++videoframes;
                }
                else if (result == 0) {
                    th_ycbcr_buffer ycbcr;

                    if (th_decode_ycbcr_out(tdec, ycbcr) == 0) {
                        THEORAPLAY_VideoFrame *item =
                            (THEORAPLAY_VideoFrame *)malloc(sizeof(*item));

                        if (item == NULL) {
                            goto cleanup;
                        }
                        item->playms = fps == 0.0
                            ? 0
                            : (unsigned int)(
                                  ((double)videoframes / fps) * 1000.0);
                        item->fps = fps;
                        item->width = tinfo.pic_width;
                        item->height = tinfo.pic_height;
                        item->format = decoder->vidfmt;
                        item->pixels = decoder->vidcvt(&tinfo, ycbcr);
                        item->next = NULL;
                        if (item->pixels == NULL) {
                            free(item);
                            goto cleanup;
                        }
                        theoraplay_lock(decoder);
                        if (decoder->videolisttail != NULL) {
                            decoder->videolisttail->next = item;
                        }
                        else {
                            decoder->videolist = item;
                        }
                        decoder->videolisttail = item;
                        ++decoder->videocount;
                        theoraplay_unlock(decoder);
                        saw_video_frame = 1;
                    }
                    ++videoframes;
                }
            }
        }

        if (!decoder->halt && need_pages) {
            const int result = FeedMoreOggData(decoder->io, &sync);

            if (result == 0) {
                eos = 1;
            }
            else if (result < 0) {
                goto cleanup;
            }
            else {
                while (!decoder->halt &&
                       ogg_sync_pageout(&sync, &page) > 0) {
                    QUEUE_OGG_PAGE();
                }
            }
        }
        if (saw_video_frame) {
            int go_on = !decoder->halt;

            while (go_on) {
                theoraplay_lock(decoder);
                go_on = !decoder->halt &&
                    decoder->videocount >= decoder->maxframes;
                theoraplay_unlock(decoder);
                if (go_on) {
                    Sleep(10);
                }
            }
        }
    }
    was_error = 0;

cleanup:
    decoder->decode_error = !decoder->halt && was_error;
    if (tdec != NULL) {
        th_decode_free(tdec);
    }
    if (tsetup != NULL) {
        th_setup_free(tsetup);
    }
    if (vblock_init) {
        vorbis_block_clear(&vblock);
    }
    if (vdsp_init) {
        vorbis_dsp_clear(&vdsp);
    }
    if (tpackets) {
        ogg_stream_clear(&tstream);
    }
    if (vpackets) {
        ogg_stream_clear(&vstream);
    }
    th_info_clear(&tinfo);
    th_comment_clear(&tcomment);
    vorbis_comment_clear(&vcomment);
    vorbis_info_clear(&vinfo);
    ogg_sync_clear(&sync);
    decoder->io->close(decoder->io);
    decoder->thread_done = 1;
#undef QUEUE_OGG_PAGE
}
#endif

/* 0x1086C0, 16 bytes, local, 1 named locals
 * WorkerThreadEntry
 * PDB type: void* (void*)
 * Source: W:\SWJediPowerBattles\work\video\theoraplay.c
 */

#if defined(JPB_THEORAPLAY_TESTING)
THEORAPLAY_Decoder *jpb_THEORAPLAYCreateDecoderForTest(void)
{
    THEORAPLAY_Decoder *decoder =
        (THEORAPLAY_Decoder *)calloc(1, sizeof(*decoder));

    if (decoder != NULL) {
        decoder->lock = CreateMutexA(NULL, FALSE, NULL);
        if (decoder->lock == NULL) {
            free(decoder);
            decoder = NULL;
        }
    }
    return decoder;
}

void jpb_THEORAPLAYDestroyDecoderForTest(THEORAPLAY_Decoder *decoder)
{
    if (decoder != NULL) {
        (void)CloseHandle(decoder->lock);
        free(decoder);
    }
}

void jpb_THEORAPLAYSetStatusForTest(
    THEORAPLAY_Decoder *decoder,
    int thread_created,
    int thread_done,
    int initialized,
    int has_video,
    int has_audio,
    int decode_error)
{
    decoder->thread_created = thread_created;
    decoder->thread_done = thread_done;
    decoder->prepped = (unsigned int)initialized;
    decoder->hasvideo = has_video;
    decoder->hasaudio = has_audio;
    decoder->decode_error = decode_error;
}

void jpb_THEORAPLAYQueueAudioForTest(
    THEORAPLAY_Decoder *decoder,
    THEORAPLAY_AudioPacket *packet)
{
    packet->next = NULL;
    decoder->audioms += packet->playms;
    if (decoder->audiolisttail == NULL) {
        decoder->audiolist = packet;
    }
    else {
        decoder->audiolisttail->next = packet;
    }
    decoder->audiolisttail = packet;
}

void jpb_THEORAPLAYQueueVideoForTest(
    THEORAPLAY_Decoder *decoder,
    THEORAPLAY_VideoFrame *frame)
{
    frame->next = NULL;
    if (decoder->videolisttail == NULL) {
        decoder->videolist = frame;
    }
    else {
        decoder->videolisttail->next = frame;
    }
    decoder->videolisttail = frame;
    ++decoder->videocount;
}

unsigned char *jpb_THEORAPLAYConvert420ForTest(
    unsigned int picture_width,
    unsigned int picture_height,
    unsigned int picture_x,
    unsigned int picture_y,
    const unsigned char *y,
    int y_stride,
    const unsigned char *u,
    int u_stride,
    const unsigned char *v,
    int v_stride,
    int yv12)
{
    th_info info = {0};
    th_img_plane planes[3] = {0};

    info.pic_width = picture_width;
    info.pic_height = picture_height;
    info.pic_x = picture_x;
    info.pic_y = picture_y;
    planes[0].stride = y_stride;
    planes[0].data = (unsigned char *)y;
    planes[1].stride = u_stride;
    planes[1].data = (unsigned char *)u;
    planes[2].stride = v_stride;
    planes[2].data = (unsigned char *)v;
    return yv12
        ? ConvertVideoFrame420ToYV12(&info, planes)
        : ConvertVideoFrame420ToIYUV(&info, planes);
}

unsigned char *jpb_THEORAPLAYConvertRGBForTest(
    unsigned int picture_width,
    unsigned int picture_height,
    const unsigned char *y,
    int y_stride,
    const unsigned char *u,
    int u_stride,
    const unsigned char *v,
    int v_stride,
    int rgba)
{
    th_info info = {0};
    th_img_plane planes[3] = {0};

    info.pic_width = picture_width;
    info.pic_height = picture_height;
    planes[0].stride = y_stride;
    planes[0].data = (unsigned char *)y;
    planes[1].stride = u_stride;
    planes[1].data = (unsigned char *)u;
    planes[2].stride = v_stride;
    planes[2].data = (unsigned char *)v;
    return rgba
        ? ConvertVideoFrame420ToRGBA(&info, planes)
        : ConvertVideoFrame420ToRGB(&info, planes);
}
#endif
