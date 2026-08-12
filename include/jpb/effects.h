#ifndef JPB_EFFECTS_H
#define JPB_EFFECTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_PROJECT_TYPE_BYTES = 1088,
    JPB_EMITTER_BYTES = 1584,
    JPB_EFFECT_COUNT = 84,
    JPB_RESIDENT_SPRITE_COUNT = 50
};

typedef struct EffectHeader EffectHeader;
typedef struct _Material _Material;

extern uint8_t maProjTypes[JPB_PROJECT_TYPE_BYTES];
extern uint8_t aEmiter[JPB_EMITTER_BYTES];
extern EffectHeader *paEffects[JPB_EFFECT_COUNT];
extern int gMaxEffect;
extern _Material *effects1Handle[JPB_RESIDENT_SPRITE_COUNT];
extern void *addHandle;
extern void *transHandle;

typedef void *(*JPBTextureLoadHook)(
    const char *path,
    int texture_type,
    uint32_t option);

void file_SetTextureLoadHook(JPBTextureLoadHook load_texture);

#ifdef __cplusplus
}
#endif

#endif
