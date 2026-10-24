#ifndef OPENJPB_XBOX_AUDIO_H
#define OPENJPB_XBOX_AUDIO_H

int jpb_XboxAudioInit(void);
void jpb_XboxAudioPump(void);

/* gain scales decoded samples before conversion (PC pc_movie_float_to_s16). */
int jpb_XboxAudioMovieBegin(unsigned rate, unsigned channels, float gain);
int jpb_XboxAudioMovieQueue(
    const float *samples, unsigned frames, unsigned channels);
unsigned jpb_XboxAudioMovieBufferedFrames(void);
void jpb_XboxAudioMovieStart(void);
void jpb_XboxAudioMovieEnd(void);

unsigned jpb_XboxAudioMoviePlayedFrames(void);

#endif
