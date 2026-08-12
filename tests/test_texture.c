#include "jpb/globalarrays.h"
#include "jpb/texture.h"
#include "jpb/world.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            fprintf(stderr, "CHECK failed: %s:%d: %s\n",               \
                    __FILE__, __LINE__, #condition);                       \
            return 1;                                                      \
        }                                                                  \
    } while (0)

static int texture_token;
static int load_count;
static int unload_count;
static unsigned observed_option;
static int observed_material_type;
static char observed_filename[256];

static void *test_load_texture(
    void *user_data,
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height)
{
    (void)user_data;
    ++load_count;
    observed_option = option;
    observed_material_type = material_type;
    strncpy(
        observed_filename,
        filename,
        sizeof(observed_filename) - 1);
    observed_filename[sizeof(observed_filename) - 1] = '\0';
    *width = 64;
    *height = 32;
    return &texture_token;
}

static void test_unload_texture(void *user_data, void *texture)
{
    (void)user_data;
    if (texture == &texture_token) {
        ++unload_count;
    }
}

static int test_material_pool_and_load_policy(void)
{
    char alpha_path[] = "C:/game/res/model/tga/a_glow.tga";
    char point_path[] = "C:/game/res/front/WINIF2.PNG";
    _Material *alpha;
    _Material *cached;
    _Material *point;

    memset(g_material, 0, sizeof(g_material));
    load_count = 0;
    unload_count = 0;
    LevelSelect = 0;
    jpb_TextureSetPlatformHooks(
        test_load_texture, test_unload_texture, NULL);

    alpha = _LoadTexture(
        alpha_path,
        TT_PLAYERCHAR,
        UINT32_C(0x02000002));
    CHECK(alpha == &g_material[0]);
    CHECK(alpha->texture == &texture_token);
    CHECK(alpha->type == TT_PLAYERCHAR);
    CHECK(alpha->iw == 64);
    CHECK(alpha->ih == 32);
    CHECK(alpha->samplerType == TEXTURESAMPLER_LINEARCLAMP);
    CHECK(alpha->colorOverride == -1);
    CHECK(strcmp(alpha->filename, alpha_path) == 0);
    CHECK(load_count == 1);
    CHECK(observed_option == UINT32_C(0x02000000));
    CHECK(observed_material_type == 2);
    CHECK(strcmp(observed_filename, alpha_path) == 0);

    cached = _LoadTexture(alpha_path, TT_NONPLAYERCHAR, 0);
    CHECK(cached == alpha);
    CHECK(load_count == 1);

    point = _LoadTexture(point_path, TT_FRONT, 0);
    CHECK(point == &g_material[1]);
    CHECK(point->samplerType == TEXTURSAMPLER_POINTCLAMP);
    CHECK(observed_material_type == 0);
    CHECK(load_count == 2);

    texture_Flush((unsigned)TT_ANY);
    CHECK(unload_count == 2);
    CHECK(g_material[0].type == TT_FREE);
    CHECK(g_material[1].type == TT_FREE);
    CHECK(g_material[0].texture == NULL);
    CHECK(g_material[1].texture == NULL);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    return 0;
}

static int test_exact_names_and_atlas_database(void)
{
    char source[] = "model/obi.bmp";

    CHECK(TT_PLAYERCHAR == 2);
    CHECK(TT_NONPLAYERCHAR == 8);
    CHECK(TT_INGAME == 4096);
    CHECK(TT_ANY == -1);
    CHECK(strcmp(texture_Name(source), "model/obi.tga") == 0);
    CHECK(jpb_TextureIsPartOfAtlas("front/_winif2.bmp"));
    CHECK(jpb_TextureIsPartOfAtlas("FRONT\\winif2_cleanUp.PNG"));
    CHECK(jpb_TextureIsPartOfAtlas("loadbar_gradient.png"));
    CHECK(!jpb_TextureIsPartOfAtlas("models/obi_body.tga"));
    return 0;
}

int main(void)
{
    CHECK(test_material_pool_and_load_policy() == 0);
    CHECK(test_exact_names_and_atlas_database() == 0);
    puts("Texture tests passed");
    return 0;
}
