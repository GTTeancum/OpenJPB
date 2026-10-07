#ifndef OPENJPB_XBOX_MOVIE_H
#define OPENJPB_XBOX_MOVIE_H

#include "jpb/software_renderer.h"

int jpb_XboxMoviePlay(
    unsigned movie,
    JPBSoftwareFramebuffer *framebuffer,
    int gpu_initialized,
    int ignore_input);

#endif
