#include <SDL.h>
#include <SDL_image.h>
#include <stdint.h>
#include <stdio.h>

extern FILE *jpb_XboxFopen(const char *, const char *);

static SDL_Surface *load_image(const char *path)
{
    FILE *file = jpb_XboxFopen(path, "rb");
    if (!file) return NULL;
    SDL_RWops *stream = SDL_RWFromFP(file, SDL_TRUE);
    if (!stream) { fclose(file); return NULL; }
    return IMG_Load_RW(stream, 1);
}

int jpb_XboxInspectImage(const char *path, int *width, int *height)
{
    SDL_Surface *image = load_image(path);
    if (!image) return 0;
    *width = image->w;
    *height = image->h;
    SDL_FreeSurface(image);
    return 1;
}

int jpb_XboxLoadImage(const char *path, int width, int height,
    uint32_t *pixels, int stride_pixels)
{
    SDL_Surface *image = load_image(path);
    int result;
    if (!image) return 0;
    if (image->w != width || image->h != height) { SDL_FreeSurface(image); return 0; }
    result = SDL_ConvertPixels(width, height, image->format->format,
        image->pixels, image->pitch, SDL_PIXELFORMAT_ARGB8888,
        pixels, stride_pixels * 4);
    SDL_FreeSurface(image);
    return result == 0;
}
