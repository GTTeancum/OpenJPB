#ifndef OPENJPB_XBOX_AUDIO_H
#define OPENJPB_XBOX_AUDIO_H

int jpb_XboxAudioInit(void);
void jpb_XboxAudioPump(void);

int jpb_XboxAudioMovieBegin(unsigned rate, unsigned channels);
int jpb_XboxAudioMovieQueue(
    const float *samples, unsigned frames, unsigned channels);
unsigned jpb_XboxAudioMovieBufferedFrames(void);
void jpb_XboxAudioMovieStart(void);
void jpb_XboxAudioMovieEnd(void);

unsigned jpb_XboxAudioMoviePlayedFrames(void);

#endif
