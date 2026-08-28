#ifndef JPB_SDL_ABI_H
#define JPB_SDL_ABI_H

#include <stddef.h>
#include <stdint.h>

typedef struct SDL_Rect {
    int x;
    int y;
    int w;
    int h;
} SDL_Rect;

typedef struct SDL_Surface {
    uint32_t flags;
    void *format;
    int w;
    int h;
    int pitch;
    void *pixels;
    void *userdata;
    int locked;
    void *list_blitmap;
    SDL_Rect clip_rect;
    void *map;
    int refcount;
} SDL_Surface;

typedef void (*SDL_AudioCallback)(
    void *userdata, uint8_t *stream, int length);

typedef struct SDL_AudioSpec {
    int freq;
    uint16_t format;
    uint8_t channels;
    uint8_t silence;
    uint16_t samples;
    uint16_t padding;
    uint32_t size;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

typedef struct SDL_Keysym {
    int32_t scancode;
    int32_t sym;
    uint16_t mod;
    uint16_t padding;
    uint32_t unused;
} SDL_Keysym;

typedef struct SDL_KeyboardEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t window_id;
    uint8_t state;
    uint8_t repeat;
    uint8_t padding2;
    uint8_t padding3;
    SDL_Keysym keysym;
} SDL_KeyboardEvent;

typedef union SDL_Event {
    uint32_t type;
    SDL_KeyboardEvent key;
    uint8_t padding[56];
} SDL_Event;

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

#if defined(__cplusplus)
static_assert(sizeof(SDL_Rect) == 16, "SDL_Rect PDB layout changed");
static_assert(sizeof(SDL_Surface) == 96, "SDL_Surface PDB layout changed");
static_assert(sizeof(SDL_AudioSpec) == 32,
              "SDL_AudioSpec PDB layout changed");
static_assert(sizeof(SDL_KeyboardEvent) == 32,
              "SDL_KeyboardEvent PDB layout changed");
static_assert(sizeof(SDL_Event) == 56, "SDL_Event PDB layout changed");
static_assert(
    offsetof(SDL_Surface, pixels) == 32,
    "SDL_Surface.pixels offset changed");
#else
_Static_assert(sizeof(SDL_Rect) == 16, "SDL_Rect PDB layout changed");
_Static_assert(sizeof(SDL_Surface) == 96, "SDL_Surface PDB layout changed");
_Static_assert(sizeof(SDL_AudioSpec) == 32,
               "SDL_AudioSpec PDB layout changed");
_Static_assert(sizeof(SDL_KeyboardEvent) == 32,
               "SDL_KeyboardEvent PDB layout changed");
_Static_assert(sizeof(SDL_Event) == 56, "SDL_Event PDB layout changed");
_Static_assert(
    offsetof(SDL_Surface, pixels) == 32,
    "SDL_Surface.pixels offset changed");
#endif

#endif
