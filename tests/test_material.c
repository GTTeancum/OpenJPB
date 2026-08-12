#include "jpb/generic_hook.h"
#include "jpb/world.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            fprintf(stderr, "CHECK failed: %s:%d: %s\n",                \
                    __FILE__, __LINE__, #condition);                       \
            return 1;                                                      \
        }                                                                  \
    } while (0)

static void reset_material(
    _Material *material, const char *filename)
{
    memset(material, 0, sizeof(*material));
    material->colorOverride = -1;
    memcpy(
        material->filename,
        filename,
        strlen(filename) + 1);
}

static int test_cached_texture_indices(void)
{
    WorldData world;
    WorldData *old_world = gpWorld;

    ClearCachedTextureIndices();
    CHECK(cachedTextureIndexForBus == -1);
    CHECK(cachedTextureIndexForCoffin == -1);
    CHECK(foundAllLevelColorOverrides == 0);
    CHECK(!IsBusTextureForCorus2(5, "models/bus.tga", 7));
    CHECK(IsBusTextureForCorus2(6, "models/bus.tga", 7));
    CHECK(IsBusTextureForCorus2(15, "models/other.tga", 7));
    CHECK(!IsBusTextureForCorus2(6, "models/bus.tga", 8));

    memset(&world, 0, sizeof(world));
    world.currentDolly = 3;
    world.aDolly[3].flags = UINT32_C(0x400);
    gpWorld = &world;
    CHECK(IsCoffinTextureForPalace(
        4, "levels/coffin256.tga", 11));
    CHECK(IsCoffinTextureForPalace(
        4, "levels/other.tga", 11));
    CHECK(!IsCoffinTextureForPalace(
        4, "levels/coffin256.tga", 12));
    world.aDolly[3].flags = 0;
    CHECK(!IsCoffinTextureForPalace(
        4, "levels/coffin256.tga", 11));
    gpWorld = old_world;
    return 0;
}

static int test_color_overrides(void)
{
    _Material material;

    ClearCachedTextureIndices();
    reset_material(&material, "models/Loadbody.tga");
    SetTextureColorOverride(1, &material);
    CHECK(material.colorOverride == 0x80);

    ClearCachedTextureIndices();
    reset_material(&material, "models/boulder.tga");
    SetTextureColorOverride(5, &material);
    CHECK(material.colorOverride == -1000);
    CHECK(foundAllLevelColorOverrides == 1);

    ClearCachedTextureIndices();
    reset_material(&material, "models/ful_body.tga");
    SetTextureColorOverride(2, &material);
    CHECK(material.colorOverride == 0xc0);

    ClearCachedTextureIndices();
    reset_material(&material, "models/bus.tga");
    SetTextureColorOverride(6, &material);
    CHECK(material.flags == JPB_MATERIAL_MODE_TWO_SIDED);
    CHECK(material.colorOverride == 100);

    ClearCachedTextureIndices();
    reset_material(&material, "models/qui_hair.tga");
    SetTextureColorOverride(9, &material);
    CHECK(material.colorOverride == 0xc0);

    reset_material(&material, "bus.tga");
    SetTextureColorOverride(6, &material);
    CHECK(material.flags == JPB_MATERIAL_MODE_BACKFACE_REJECT);
    CHECK(material.colorOverride == -1);
    return 0;
}

int main(void)
{
    CHECK(test_cached_texture_indices() == 0);
    CHECK(test_color_overrides() == 0);
    puts("Material tests passed");
    return 0;
}
