#include "jpb/theoraplay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static THEORAPLAY_AudioPacket *make_audio(unsigned int playms)
{
    THEORAPLAY_AudioPacket *packet =
        (THEORAPLAY_AudioPacket *)calloc(1, sizeof(*packet));

    if (packet != NULL) {
        packet->playms = playms;
    }
    return packet;
}

static THEORAPLAY_VideoFrame *make_video(unsigned int playms)
{
    THEORAPLAY_VideoFrame *frame =
        (THEORAPLAY_VideoFrame *)calloc(1, sizeof(*frame));

    if (frame != NULL) {
        frame->playms = playms;
    }
    return frame;
}

static int io_close_calls;

static long eof_read(
    THEORAPLAY_Io *io, void *buffer, long buffer_length)
{
    (void)io;
    (void)buffer;
    (void)buffer_length;
    return 0;
}

static void counting_close(THEORAPLAY_Io *io)
{
    ++io_close_calls;
    free(io);
}

static THEORAPLAY_Io *make_eof_io(void)
{
    THEORAPLAY_Io *io = (THEORAPLAY_Io *)malloc(sizeof(*io));

    if (io != NULL) {
        io->read = eof_read;
        io->close = counting_close;
        io->userdata = NULL;
    }
    return io;
}

static int wait_for_decoder_stop(
    THEORAPLAY_Decoder *decoder, unsigned int timeout_ms)
{
    unsigned int elapsed = 0;

    while (THEORAPLAY_isDecoding(decoder) && elapsed < timeout_ms) {
#if defined(_WIN32)
        Sleep(10);
#endif
        elapsed += 10;
    }
    return !THEORAPLAY_isDecoding(decoder);
}

#if defined(JPB_THEORAPLAY_REAL_MOVIE)
static int test_real_movie(void)
{
    THEORAPLAY_Decoder *decoder = THEORAPLAY_startDecodeFile(
        JPB_THEORAPLAY_REAL_MOVIE, 2, THEORAPLAY_VIDFMT_RGBA);
    const THEORAPLAY_VideoFrame *video = NULL;
    const THEORAPLAY_AudioPacket *audio = NULL;
    unsigned int elapsed = 0;

    CHECK(decoder != NULL);
    while (elapsed < 10000 &&
           (!THEORAPLAY_isInitialized(decoder) ||
            video == NULL ||
            THEORAPLAY_availableAudio(decoder) == 0)) {
        if (video == NULL && THEORAPLAY_availableVideo(decoder) != 0) {
            video = THEORAPLAY_getVideo(decoder);
        }
        else if (THEORAPLAY_availableVideo(decoder) >= 2) {
            const THEORAPLAY_VideoFrame *extra =
                THEORAPLAY_getVideo(decoder);

            THEORAPLAY_freeVideo(extra);
        }
        CHECK(!THEORAPLAY_decodingError(decoder));
        Sleep(10);
        elapsed += 10;
    }
    CHECK(THEORAPLAY_isInitialized(decoder) == 1);
    CHECK(THEORAPLAY_hasVideoStream(decoder) == 1);
    CHECK(THEORAPLAY_hasAudioStream(decoder) == 1);
    audio = THEORAPLAY_getAudio(decoder);
    CHECK(video != NULL);
    CHECK(video->width == 1920 && video->height == 1080);
    CHECK(video->format == THEORAPLAY_VIDFMT_RGBA);
    CHECK(video->pixels != NULL && video->pixels[3] == 255);
    CHECK(audio != NULL);
    CHECK(audio->channels > 0 && audio->freq > 0 && audio->frames > 0);
    THEORAPLAY_freeVideo(video);
    THEORAPLAY_freeAudio(audio);
    THEORAPLAY_stopDecode(decoder);
    return 0;
}
#endif

#if defined(_WIN32)
static unsigned char canonical_component(float value)
{
    if (value < 0.0f) value = 0.0f;
    if (value > 255.0f) value = 255.0f;
    return (unsigned char)_mm_cvtss_si32(_mm_set_ss(value));
}

static int test_rgb_all_components(void)
{
    /* All 256^3 Y/Cb/Cr triples, row inversion, and a six-pixel SIMD tail. */
    enum { width = 518, height = 512, chroma_width = width / 2 };
    unsigned char *y = (unsigned char *)malloc(width * height);
    unsigned char *u = (unsigned char *)malloc(chroma_width * height / 2);
    unsigned char *v = (unsigned char *)malloc(chroma_width * height / 2);
    unsigned cb;
    unsigned row;
    unsigned column;
    CHECK(y != NULL && u != NULL && v != NULL);
    for (row = 0; row < height; ++row) {
        for (column = 0; column < width; ++column) {
            y[row * width + column] = (unsigned char)(column >> 1);
        }
    }
    for (row = 0; row < height / 2; ++row) {
        memset(v + row * chroma_width, row, chroma_width);
    }
    for (cb = 0; cb < 256; ++cb) {
        unsigned char *pixels;
        const float cb_scaled = ((float)cb - 128.0f) * 0.004464286f;
        memset(u, cb, chroma_width * height / 2);
        pixels = jpb_THEORAPLAYConvertRGBForTest(width, height,
            y, width, u, chroma_width, v, chroma_width, 1);
        CHECK(pixels != NULL);
        for (row = 0; row < height; ++row) {
            const float cr = ((float)(row >> 1) - 128.0f) * 0.004464286f;
            for (column = 0; column < width; ++column) {
                const float luma = ((float)y[row * width + column] - 16.0f) *
                    0.00456621f * 255.0f;
                const unsigned char *pixel = pixels +
                    ((height - row - 1) * width + column) * 4;
                CHECK(pixel[0] == canonical_component(cr * 357.50998f + luma));
                CHECK(pixel[1] == canonical_component(
                    (luma - cb_scaled * 87.754745f) - cr * 182.10474f));
                CHECK(pixel[2] == canonical_component(cb_scaled * 451.86f + luma));
                CHECK(pixel[3] == 255);
            }
        }
        free(pixels);
    }
    free(y);
    free(u);
    free(v);
    puts("rgb_exhaustive=(yuv_triples=16777216,match=1,tail=6)");
    return 0;
}

static int audit_media(const char *path, const char *prefix)
{
    char output[1024];
    FILE *frames;
    FILE *samples;
    THEORAPLAY_Decoder *decoder;
    unsigned int video_count = 0;
    unsigned long long audio_frames = 0;
    DWORD started = GetTickCount();

    snprintf(output, sizeof(output), "%s.frames.csv", prefix);
    frames = fopen(output, "w");
    CHECK(frames != NULL);
    snprintf(output, sizeof(output), "%s.f32", prefix);
    samples = fopen(output, "wb");
    CHECK(samples != NULL);
    decoder = THEORAPLAY_startDecodeFile(path, 8, THEORAPLAY_VIDFMT_IYUV);
    CHECK(decoder != NULL);
    fputs("index,playms,adler32\n", frames);
    while (THEORAPLAY_isDecoding(decoder)) {
        const THEORAPLAY_VideoFrame *video;
        const THEORAPLAY_AudioPacket *audio;
        CHECK(GetTickCount() - started < 180000);
        while ((audio = THEORAPLAY_getAudio(decoder)) != NULL) {
            const size_t count = (size_t)audio->frames * audio->channels;
            CHECK(fwrite(audio->samples, sizeof(float), count, samples) == count);
            CHECK(audio->playms == (uint32_t)(
                ((double)audio_frames / audio->freq) * 1000.0));
            audio_frames += audio->frames;
            THEORAPLAY_freeAudio(audio);
        }
        while ((video = THEORAPLAY_getVideo(decoder)) != NULL) {
            const size_t count = (size_t)video->width * video->height * 3 / 2;
            uint32_t a = 1;
            uint32_t b = 0;
            size_t index;
            for (index = 0; index < count; ++index) {
                a = (a + video->pixels[index]) % 65521;
                b = (b + a) % 65521;
            }
            fprintf(frames, "%u,%u,%08x\n", video_count++, video->playms,
                    (b << 16) | a);
            THEORAPLAY_freeVideo(video);
        }
        Sleep(1);
    }
    CHECK(!THEORAPLAY_decodingError(decoder));
    THEORAPLAY_stopDecode(decoder);
    CHECK(fclose(samples) == 0);
    CHECK(fclose(frames) == 0);
    printf("media_audit=(video_frames=%u,audio_frames=%llu)\n",
           video_count, audio_frames);
    return 0;
}

static int verify_timeline(const char *path, unsigned expected_frames,
                           uint32_t expected_hash, unsigned long long expected_audio)
{
    THEORAPLAY_Decoder *decoder = THEORAPLAY_startDecodeFile(
        path, 8, THEORAPLAY_VIDFMT_IYUV);
    DWORD started = GetTickCount();
    uint32_t hash = 2166136261u;
    unsigned frames = 0;
    unsigned long long audio_frames = 0;

    CHECK(decoder != NULL);
    while (THEORAPLAY_isDecoding(decoder)) {
        const THEORAPLAY_VideoFrame *video;
        const THEORAPLAY_AudioPacket *audio;
        CHECK(GetTickCount() - started < 120000);
        while ((video = THEORAPLAY_getVideo(decoder)) != NULL) {
            unsigned shift;
            for (shift = 0; shift < 32; shift += 8) {
                hash = (hash ^ ((video->playms >> shift) & 255u)) * 16777619u;
            }
            ++frames;
            THEORAPLAY_freeVideo(video);
        }
        while ((audio = THEORAPLAY_getAudio(decoder)) != NULL) {
            CHECK(audio->playms == (uint32_t)(
                ((double)audio_frames / audio->freq) * 1000.0));
            audio_frames += audio->frames;
            THEORAPLAY_freeAudio(audio);
        }
        Sleep(1);
    }
    CHECK(!THEORAPLAY_decodingError(decoder));
    THEORAPLAY_stopDecode(decoder);
    printf("timeline=(frames=%u,fnv32=%08x,audio_frames=%llu)\n",
           frames, hash, audio_frames);
    CHECK(frames == expected_frames);
    CHECK(hash == expected_hash);
    CHECK(audio_frames == expected_audio);
    return 0;
}
#endif

int main(int argc, char **argv)
{
    static const unsigned char y_plane[36] = {
        0, 1, 2, 3, 4, 5,
        10, 11, 12, 13, 14, 15,
        20, 21, 22, 23, 24, 25,
        30, 31, 32, 33, 34, 35,
        40, 41, 42, 43, 44, 45,
        50, 51, 52, 53, 54, 55
    };
    static const unsigned char u_plane[9] = {
        60, 61, 62, 63, 64, 65, 66, 67, 68
    };
    static const unsigned char v_plane[9] = {
        70, 71, 72, 73, 74, 75, 76, 77, 78
    };
    static const unsigned char expected_iyuv[24] = {
        0, 1, 2, 3, 10, 11, 12, 13,
        20, 21, 22, 23, 30, 31, 32, 33,
        60, 61, 63, 64, 70, 71, 73, 74
    };
    static const unsigned char expected_yv12[24] = {
        0, 1, 2, 3, 10, 11, 12, 13,
        20, 21, 22, 23, 30, 31, 32, 33,
        70, 71, 73, 74, 60, 61, 63, 64
    };
    static const unsigned char rgb_y[8] = {
        16, 81, 145, 210,
        235, 81, 145, 16
    };
    static const unsigned char rgb_u[2] = {128, 90};
    static const unsigned char rgb_v[2] = {128, 240};
    static const unsigned char expected_rgb[32] = {
        255, 255, 255, 255,
        76, 76, 76, 255,
        255, 74, 74, 255,
        179, 0, 0, 255,
        0, 0, 0, 255,
        76, 76, 76, 255,
        255, 74, 74, 255,
        255, 150, 149, 255
    };
#if defined(_WIN32)
    if (argc == 2 && strcmp(argv[1], "--verify-rgb") == 0) {
        return test_rgb_all_components();
    }
    if (argc == 4 && strcmp(argv[1], "--audit-media") == 0) {
        return audit_media(argv[2], argv[3]);
    }
    if (argc == 6 && strcmp(argv[1], "--verify-timeline") == 0) {
        return verify_timeline(argv[2], (unsigned)strtoul(argv[3], NULL, 10),
                               (uint32_t)strtoul(argv[4], NULL, 16),
                               strtoull(argv[5], NULL, 10));
    }
#else
    (void)argc;
    (void)argv;
#endif
    THEORAPLAY_Decoder *decoder = jpb_THEORAPLAYCreateDecoderForTest();
    THEORAPLAY_AudioPacket *audio_a = make_audio(11);
    THEORAPLAY_AudioPacket *audio_b = make_audio(29);
    THEORAPLAY_VideoFrame *video_a = make_video(7);
    THEORAPLAY_VideoFrame *video_b = make_video(13);
    const THEORAPLAY_AudioPacket *audio;
    const THEORAPLAY_VideoFrame *video;
    unsigned char *converted;
    THEORAPLAY_Io *io;
    THEORAPLAY_Decoder *started;

    CHECK(decoder != NULL);
    CHECK(audio_a != NULL && audio_b != NULL);
    CHECK(video_a != NULL && video_b != NULL);
    CHECK(THEORAPLAY_availableAudio(NULL) == 0);
    CHECK(THEORAPLAY_availableVideo(NULL) == 0);
    CHECK(THEORAPLAY_decodingError(NULL) == 0);
    CHECK(THEORAPLAY_hasAudioStream(NULL) == 0);
    CHECK(THEORAPLAY_hasVideoStream(NULL) == 0);
    CHECK(THEORAPLAY_isDecoding(NULL) == 0);
    CHECK(THEORAPLAY_isInitialized(NULL) == 0);

    jpb_THEORAPLAYSetStatusForTest(decoder, 1, 0, 1, 3, 5, 7);
    CHECK(THEORAPLAY_isInitialized(decoder) == 1);
    CHECK(THEORAPLAY_hasVideoStream(decoder) == 3);
    CHECK(THEORAPLAY_hasAudioStream(decoder) == 5);
    CHECK(THEORAPLAY_decodingError(decoder) == 7);
    CHECK(THEORAPLAY_isDecoding(decoder) == 1);

    jpb_THEORAPLAYSetStatusForTest(decoder, 1, 1, 1, 3, 5, 7);
    CHECK(THEORAPLAY_isDecoding(decoder) == 0);

    jpb_THEORAPLAYQueueAudioForTest(decoder, audio_a);
    jpb_THEORAPLAYQueueAudioForTest(decoder, audio_b);
    CHECK(THEORAPLAY_availableAudio(decoder) == 40);
    CHECK(THEORAPLAY_isDecoding(decoder) == 1);
    audio = THEORAPLAY_getAudio(decoder);
    CHECK(audio == audio_a && audio->next == NULL);
    CHECK(THEORAPLAY_availableAudio(decoder) == 29);
    THEORAPLAY_freeAudio(audio);
    audio = THEORAPLAY_getAudio(decoder);
    CHECK(audio == audio_b && audio->next == NULL);
    CHECK(THEORAPLAY_availableAudio(decoder) == 0);
    THEORAPLAY_freeAudio(audio);
    CHECK(THEORAPLAY_getAudio(decoder) == NULL);

    jpb_THEORAPLAYQueueVideoForTest(decoder, video_a);
    jpb_THEORAPLAYQueueVideoForTest(decoder, video_b);
    CHECK(THEORAPLAY_availableVideo(decoder) == 2);
    video = THEORAPLAY_getVideo(decoder);
    CHECK(video == video_a && video->next == NULL);
    CHECK(THEORAPLAY_availableVideo(decoder) == 1);
    THEORAPLAY_freeVideo(video);
    video = THEORAPLAY_getVideo(decoder);
    CHECK(video == video_b && video->next == NULL);
    CHECK(THEORAPLAY_availableVideo(decoder) == 0);
    THEORAPLAY_freeVideo(video);
    CHECK(THEORAPLAY_getVideo(decoder) == NULL);
    CHECK(THEORAPLAY_isDecoding(decoder) == 0);

    converted = jpb_THEORAPLAYConvert420ForTest(
        4, 4, 1, 1,
        y_plane, 6, u_plane, 3, v_plane, 3, 0);
    CHECK(converted != NULL);
    CHECK(memcmp(converted, expected_iyuv, sizeof(expected_iyuv)) == 0);
    free(converted);
    converted = jpb_THEORAPLAYConvert420ForTest(
        4, 4, 1, 1,
        y_plane, 6, u_plane, 3, v_plane, 3, 1);
    CHECK(converted != NULL);
    CHECK(memcmp(converted, expected_yv12, sizeof(expected_yv12)) == 0);
    free(converted);

    converted = jpb_THEORAPLAYConvertRGBForTest(
        4, 2, rgb_y, 4, rgb_u, 2, rgb_v, 2, 0);
    CHECK(converted != NULL);
    CHECK(memcmp(converted, expected_rgb, sizeof(expected_rgb)) == 0);
    free(converted);
    converted = jpb_THEORAPLAYConvertRGBForTest(
        4, 2, rgb_y, 4, rgb_u, 2, rgb_v, 2, 1);
    CHECK(converted != NULL);
    CHECK(memcmp(converted, expected_rgb, sizeof(expected_rgb)) == 0);
    free(converted);

    io_close_calls = 0;
    io = make_eof_io();
    CHECK(io != NULL);
    CHECK(THEORAPLAY_startDecode(
              io, 2, (THEORAPLAY_VideoFormat)99) == NULL);
    CHECK(io_close_calls == 1);

    io = make_eof_io();
    CHECK(io != NULL);
    started = THEORAPLAY_startDecode(
        io, 2, THEORAPLAY_VIDFMT_RGBA);
    CHECK(started != NULL);
    CHECK(wait_for_decoder_stop(started, 5000));
    CHECK(THEORAPLAY_decodingError(started) == 1);
    CHECK(io_close_calls == 2);
    THEORAPLAY_stopDecode(started);

#if defined(JPB_THEORAPLAY_REAL_MOVIE)
    CHECK(test_real_movie() == 0);
    CHECK(test_real_movie() == 0);
    CHECK(test_real_movie() == 0);
#endif

    jpb_THEORAPLAYDestroyDecoderForTest(decoder);
    puts("theoraplay queue tests passed");
    return 0;
}
