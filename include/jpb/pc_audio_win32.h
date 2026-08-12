#ifndef JPB_PC_AUDIO_WIN32_H
#define JPB_PC_AUDIO_WIN32_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JPBPCAudio JPBPCAudio;

typedef struct JPBPCAudioWavInfo {
    uint16_t formatTag;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t averageBytesPerSecond;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t dataOffset;
    uint32_t dataSize;
} JPBPCAudioWavInfo;

typedef struct JPBPCAudioStats {
    uint32_t sfxStarted;
    uint32_t musicStarted;
} JPBPCAudioStats;

/*
 * Win32-only host adapter for the platform-neutral sound callbacks. This is
 * a substituted PC implementation, not an original PDB symbol. The caller
 * may disable output and still use exact bank/path and WAV inspection.
 */
JPBPCAudio *jpb_PCAudioCreate(
    const char *world_path,
    const char *player_one_cad_path,
    const char *player_two_cad_path,
    int level_index,
    int enable_output);
void jpb_PCAudioDestroy(JPBPCAudio *audio);
void jpb_PCAudioUpdate(JPBPCAudio *audio);
void jpb_PCAudioGetStats(
    const JPBPCAudio *audio,
    JPBPCAudioStats *stats);

int jpb_PCAudioResolveSound(
    const JPBPCAudio *audio,
    int bank_id,
    const char *sound,
    char *path,
    size_t path_capacity);
int jpb_PCAudioResolveStream(
    const JPBPCAudio *audio,
    const char *stream_name,
    char *path,
    size_t path_capacity);
int jpb_PCAudioInspectWavMemory(
    const void *bytes,
    size_t size,
    JPBPCAudioWavInfo *info);
int jpb_PCAudioInspectWavFile(
    const char *path,
    JPBPCAudioWavInfo *info);

#ifdef __cplusplus
}
#endif

#endif
