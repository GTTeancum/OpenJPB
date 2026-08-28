#include "jpb/globalarrays.h"
#include "jpb/resources.h"
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
static const char *failed_filename;
static int fail_all_loads;

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
    strncpy(observed_filename,
        filename != NULL ? filename : "<null>",
        sizeof(observed_filename) - 1);
    observed_filename[sizeof(observed_filename) - 1] = '\0';
    if (fail_all_loads ||
        (failed_filename != NULL && filename != NULL &&
         strcmp(filename, failed_filename) == 0)) {
        return NULL;
    }
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
    char pvr_path[] = "C:/game/res/legacy/model.pvr";
    char sgi_path[] = "C:/game/res/legacy/model.sgi";
    _Material *alpha;
    _Material *cached;
    _Material *point;

    memset(g_material, 0, sizeof(g_material));
    load_count = 0;
    unload_count = 0;
    failed_filename = NULL;
    fail_all_loads = 0;
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
    CHECK(observed_option == UINT32_C(0x02000002));
    CHECK(observed_material_type == 0);
    CHECK(strcmp(observed_filename, alpha_path) == 0);

    cached = _LoadTexture(alpha_path, TT_NONPLAYERCHAR, 0);
    CHECK(cached == alpha);
    CHECK(load_count == 1);

    point = _LoadTexture(point_path, TT_FRONT, 0);
    CHECK(point == &g_material[1]);
    CHECK(point->samplerType == TEXTURSAMPLER_POINTCLAMP);
    CHECK(observed_material_type == 0);
    CHECK(load_count == 2);

    CHECK(_LoadTexture(pvr_path, TT_LEVEL, 0) == &g_material[2]);
    CHECK(strcmp(observed_filename, "C:/game/res/legacy/model.tga") == 0);
    CHECK(strcmp(g_material[2].filename, pvr_path) == 0);
    CHECK(_LoadTexture(sgi_path, TT_LEVEL, 0) == &g_material[3]);
    CHECK(strcmp(observed_filename, "C:/game/res/legacy/model.tim") == 0);
    CHECK(strcmp(g_material[3].filename, sgi_path) == 0);

    texture_Flush((unsigned)TT_ANY);
    CHECK(unload_count == 4);
    CHECK(g_material[0].type == TT_FREE);
    CHECK(g_material[1].type == TT_FREE);
    CHECK(g_material[2].type == TT_FREE);
    CHECK(g_material[3].type == TT_FREE);
    CHECK(g_material[0].texture == NULL);
    CHECK(g_material[1].texture == NULL);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    return 0;
}

static int test_retail_cache_and_failure_policy(void)
{
    char windows_alpha_path[] = "C:\\game\\a_glow.tga";
    char embedded_sgi_path[] = "C:/game/sgi_bank/model.bmp";
    char embedded_sgi_no_dot[] = "C:/game/sgi_bank/model";
    char fallback_path[] = "C:/game/res/model/tga/missing.tga";
    char failed_path[] = "C:/game/res/model/tga/always_missing.tga";
    _Material *material;
    _Material *stale;

    memset(g_material, 0, sizeof(g_material));
    load_count = 0;
    unload_count = 0;
    failed_filename = NULL;
    fail_all_loads = 0;
    jpb_TextureSetPlatformHooks(
        test_load_texture, test_unload_texture, NULL);

    material = _LoadTexture(windows_alpha_path, TT_LEVEL, 2);
    CHECK(material == &g_material[0]);
    CHECK(observed_material_type == 2);

    CHECK(_LoadTexture(embedded_sgi_path, TT_LEVEL, 0) == &g_material[1]);
    CHECK(strcmp(observed_filename, "C:/game/sgi_bank/model.tim") == 0);
    CHECK(_LoadTexture(embedded_sgi_no_dot, TT_LEVEL, 0) == &g_material[2]);
    CHECK(strcmp(observed_filename, embedded_sgi_no_dot) == 0);

    failed_filename = fallback_path;
    material = _LoadTexture(fallback_path, TT_LEVEL, UINT32_C(0x02000002));
    CHECK(material == &g_material[3]);
    CHECK(material->texture == &texture_token);
    CHECK(strcmp(material->filename, fallback_path) == 0);
    CHECK(strcmp(observed_filename,
        "../../../res/default\\o_default.tga") == 0);
    CHECK(observed_option == 2);
    CHECK(observed_material_type == 0);

    fail_all_loads = 1;
    CHECK(_LoadTexture(failed_path, TT_LEVEL, 0) == NULL);
    CHECK(g_material[4].type == TT_FREE);
    stale = _LoadTexture(failed_path, TT_LEVEL, 0);
    CHECK(stale == &g_material[4]);
    CHECK(stale->texture == NULL);
    CHECK(load_count == 7);

    fail_all_loads = 0;
    failed_filename = NULL;
    texture_Flush((unsigned)TT_ANY);
    CHECK(unload_count == 4);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    return 0;
}

static int test_try_load_cache_lifetime(void)
{
    _Material *material;
    _Material *cached;
    _Material *reloaded;

    memset(g_material, 0, sizeof(g_material));
    load_count = 0;
    unload_count = 0;
    failed_filename = NULL;
    fail_all_loads = 0;
    CHECK(jpb_ResourceSetBasePath("C:/game"));
    jpb_TextureSetPlatformHooks(
        test_load_texture, test_unload_texture, NULL);
    _ClearTextureCache();
    LevelSelect = 25;

    material = _TryLoadTexture("arena_wall.tga", TT_LEVEL, 0);
    CHECK(material == &g_material[0]);
    CHECK(strcmp(observed_filename,
        "C:/game/res/level/jpx/fed/arena_wall.tga") == 0);
    CHECK(load_count == 1);
    _FreeTexture(material);
    CHECK(material->texture == NULL);

    cached = _TryLoadTexture("arena_wall.tga", TT_LEVEL, 0);
    CHECK(cached == material);
    CHECK(load_count == 1);

    _ClearTextureCache();
    reloaded = _TryLoadTexture("arena_wall.tga", TT_LEVEL, 0);
    CHECK(reloaded == &g_material[1]);
    CHECK(reloaded != material);
    CHECK(load_count == 2);

    texture_Flush((unsigned)TT_ANY);
    _ClearTextureCache();
    CHECK(unload_count == 2);
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
    CHECK(jpb_TextureIsPartOfAtlas("FRONT\\WINIF2.TGA"));
    CHECK(!jpb_TextureIsPartOfAtlas("FRONT\\winif2_cleanUp.PNG"));
    CHECK(jpb_TextureIsPartOfAtlas("loadbar_gradient.png"));
    CHECK(!jpb_TextureIsPartOfAtlas("models/obi_body.tga"));
    return 0;
}

int main(void)
{
    CHECK(test_material_pool_and_load_policy() == 0);
    CHECK(test_retail_cache_and_failure_policy() == 0);
    CHECK(test_try_load_cache_lifetime() == 0);
    CHECK(test_exact_names_and_atlas_database() == 0);
    puts("Texture tests passed");
    return 0;
}
