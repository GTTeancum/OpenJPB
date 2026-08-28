#include "jpb/theoraplay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(void)
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
#endif

    jpb_THEORAPLAYDestroyDecoderForTest(decoder);
    puts("theoraplay queue tests passed");
    return 0;
}
