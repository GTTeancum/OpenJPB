#ifndef JPB_THEORAPLAY_H
#define JPB_THEORAPLAY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct THEORAPLAY_AudioPacket {
    uint32_t playms;
    int channels;
    int freq;
    int frames;
    float *samples;
    struct THEORAPLAY_AudioPacket *next;
} THEORAPLAY_AudioPacket;

typedef struct AudioQueue {
    const THEORAPLAY_AudioPacket *audio;
    int offset;
    struct AudioQueue *next;
} AudioQueue;

typedef enum THEORAPLAY_VideoFormat {
    THEORAPLAY_VIDFMT_YV12 = 0,
    THEORAPLAY_VIDFMT_IYUV = 1,
    THEORAPLAY_VIDFMT_RGB = 2,
    THEORAPLAY_VIDFMT_RGBA = 3
} THEORAPLAY_VideoFormat;

typedef struct THEORAPLAY_Io THEORAPLAY_Io;

typedef long (*THEORAPLAY_IoRead)(
    THEORAPLAY_Io *io, void *buffer, long buffer_length);
typedef void (*THEORAPLAY_IoClose)(THEORAPLAY_Io *io);

struct THEORAPLAY_Io {
    THEORAPLAY_IoRead read;
    THEORAPLAY_IoClose close;
    void *userdata;
};

typedef struct TheoraDecoder THEORAPLAY_Decoder;

typedef struct THEORAPLAY_VideoFrame {
    uint32_t playms;
    double fps;
    uint32_t width;
    uint32_t height;
    THEORAPLAY_VideoFormat format;
    unsigned char *pixels;
    struct THEORAPLAY_VideoFrame *next;
} THEORAPLAY_VideoFrame;

THEORAPLAY_Decoder *THEORAPLAY_startDecode(
    THEORAPLAY_Io *io,
    unsigned int maxframes,
    THEORAPLAY_VideoFormat format);
THEORAPLAY_Decoder *THEORAPLAY_startDecodeFile(
    const char *filename,
    unsigned int maxframes,
    THEORAPLAY_VideoFormat format);

void THEORAPLAY_freeAudio(const THEORAPLAY_AudioPacket *packet);
void THEORAPLAY_freeVideo(const THEORAPLAY_VideoFrame *frame);
unsigned int THEORAPLAY_availableAudio(THEORAPLAY_Decoder *decoder);
unsigned int THEORAPLAY_availableVideo(THEORAPLAY_Decoder *decoder);
int THEORAPLAY_decodingError(THEORAPLAY_Decoder *decoder);
const THEORAPLAY_AudioPacket *THEORAPLAY_getAudio(
    THEORAPLAY_Decoder *decoder);
const THEORAPLAY_VideoFrame *THEORAPLAY_getVideo(
    THEORAPLAY_Decoder *decoder);
int THEORAPLAY_hasAudioStream(THEORAPLAY_Decoder *decoder);
int THEORAPLAY_hasVideoStream(THEORAPLAY_Decoder *decoder);
int THEORAPLAY_isDecoding(THEORAPLAY_Decoder *decoder);
int THEORAPLAY_isInitialized(THEORAPLAY_Decoder *decoder);
void THEORAPLAY_stopDecode(THEORAPLAY_Decoder *decoder);

#if defined(JPB_THEORAPLAY_TESTING)
THEORAPLAY_Decoder *jpb_THEORAPLAYCreateDecoderForTest(void);
void jpb_THEORAPLAYDestroyDecoderForTest(THEORAPLAY_Decoder *decoder);
void jpb_THEORAPLAYSetStatusForTest(
    THEORAPLAY_Decoder *decoder,
    int thread_created,
    int thread_done,
    int initialized,
    int has_video,
    int has_audio,
    int decode_error);
void jpb_THEORAPLAYQueueAudioForTest(
    THEORAPLAY_Decoder *decoder,
    THEORAPLAY_AudioPacket *packet);
void jpb_THEORAPLAYQueueVideoForTest(
    THEORAPLAY_Decoder *decoder,
    THEORAPLAY_VideoFrame *frame);
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
    int yv12);
unsigned char *jpb_THEORAPLAYConvertRGBForTest(
    unsigned int picture_width,
    unsigned int picture_height,
    const unsigned char *y,
    int y_stride,
    const unsigned char *u,
    int u_stride,
    const unsigned char *v,
    int v_stride,
    int rgba);
#endif

#if defined(__cplusplus)
static_assert(
    sizeof(THEORAPLAY_Io) == 24,
    "THEORAPLAY_Io must match the PDB layout");
static_assert(
    offsetof(THEORAPLAY_Io, userdata) == 16,
    "THEORAPLAY_Io.userdata offset changed");
static_assert(
    sizeof(THEORAPLAY_AudioPacket) == 32,
    "THEORAPLAY_AudioPacket must match the PDB layout");
static_assert(
    offsetof(THEORAPLAY_AudioPacket, samples) == 16,
    "THEORAPLAY_AudioPacket.samples offset changed");
static_assert(
    offsetof(THEORAPLAY_AudioPacket, next) == 24,
    "THEORAPLAY_AudioPacket.next offset changed");
static_assert(
    sizeof(AudioQueue) == 24,
    "AudioQueue must match the PDB layout");
static_assert(
    offsetof(AudioQueue, next) == 16,
    "AudioQueue.next offset changed");
static_assert(
    sizeof(THEORAPLAY_VideoFrame) == 48,
    "THEORAPLAY_VideoFrame must match the PDB layout");
static_assert(
    offsetof(THEORAPLAY_VideoFrame, pixels) == 32,
    "THEORAPLAY_VideoFrame.pixels offset changed");
static_assert(
    offsetof(THEORAPLAY_VideoFrame, next) == 40,
    "THEORAPLAY_VideoFrame.next offset changed");
#else
_Static_assert(
    sizeof(THEORAPLAY_Io) == 24,
    "THEORAPLAY_Io must match the PDB layout");
_Static_assert(
    offsetof(THEORAPLAY_Io, userdata) == 16,
    "THEORAPLAY_Io.userdata offset changed");
_Static_assert(
    sizeof(THEORAPLAY_AudioPacket) == 32,
    "THEORAPLAY_AudioPacket must match the PDB layout");
_Static_assert(
    offsetof(THEORAPLAY_AudioPacket, samples) == 16,
    "THEORAPLAY_AudioPacket.samples offset changed");
_Static_assert(
    offsetof(THEORAPLAY_AudioPacket, next) == 24,
    "THEORAPLAY_AudioPacket.next offset changed");
_Static_assert(sizeof(AudioQueue) == 24, "AudioQueue must match the PDB layout");
_Static_assert(
    offsetof(AudioQueue, next) == 16,
    "AudioQueue.next offset changed");
_Static_assert(
    sizeof(THEORAPLAY_VideoFrame) == 48,
    "THEORAPLAY_VideoFrame must match the PDB layout");
_Static_assert(
    offsetof(THEORAPLAY_VideoFrame, pixels) == 32,
    "THEORAPLAY_VideoFrame.pixels offset changed");
_Static_assert(
    offsetof(THEORAPLAY_VideoFrame, next) == 40,
    "THEORAPLAY_VideoFrame.next offset changed");
#endif

#ifdef __cplusplus
}
#endif

#endif
