#include "jpb/transparent_texture_database.h"

#include <stdio.h>

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            fprintf(stderr, "CHECK failed: %s:%d: %s\n",                \
                    __FILE__, __LINE__, #condition);                       \
            return 1;                                                      \
        }                                                                  \
    } while (0)

static int test_exact_classifiers(void)
{
    CHECK(jpb_IsGlassTexture("t_railing.tga"));
    CHECK(jpb_IsGlassTexture("T_ORANGEGLASS.TGA"));
    CHECK(!jpb_IsGlassTexture("t_blueglass.tga"));
    CHECK(!jpb_IsGlassTexture(NULL));

    CHECK(jpb_IsTextureTransparent("T_O_FLOORBASE.TGA", 1));
    CHECK(jpb_IsTextureTransparent("p_fern05a.tga", 2));
    CHECK(jpb_IsTextureTransparent("p_theed2_b.tga", 3));
    CHECK(jpb_IsTextureTransparent("t_pal_16.tga", 4));
    CHECK(jpb_IsTextureTransparent("t_water.tga", 9));
    /* Retail lowercases the query but not this initializer entry. */
    CHECK(!jpb_IsTextureTransparent("t_waterStatic.tga", 9));
    CHECK(!jpb_IsTextureTransparent("t_waterstatic.tga", 9));
    CHECK(jpb_IsTextureTransparent("p_train4.tga", 16));
    CHECK(jpb_IsTextureTransparent("p_train4.tga", 17));
    CHECK(jpb_IsTextureTransparent("p_train2.tga", 22));
    CHECK(jpb_IsTextureTransparent("fed2a.tga", 25));

    CHECK(!jpb_IsTextureTransparent("p_treebottom.tga", 2));
    CHECK(!jpb_IsTextureTransparent("t_pal_15.tga", 4));
    CHECK(!jpb_IsTextureTransparent("p_train4.tga", 18));
    CHECK(!jpb_IsTextureTransparent("fed2a.tga", 1));
    CHECK(!jpb_IsTextureTransparent("anything.tga", -1));
    CHECK(!jpb_IsTextureTransparent("anything.tga", 26));
    CHECK(!jpb_IsTextureTransparent(NULL, 8));
    return 0;
}

static int test_jpx_mirror_names(void)
{
    /* Collision-resolved 8.3 spellings from the shipped JPX files. */
    CHECK(jpb_IsGlassTextureForJpxMirror("T_RAILIN.TGA"));
    CHECK(jpb_IsGlassTextureForJpxMirror("T_WINDOW.TGA"));
    CHECK(!jpb_IsGlassTextureForJpxMirror("T_BLUEGL.TGA"));

    CHECK(jpb_IsTextureTransparentForJpxMirror(
        "P_FERN0A.TGA", 2));
    CHECK(jpb_IsTextureTransparentForJpxMirror(
        "P_STREEU.TGA", 8));
    CHECK(jpb_IsTextureTransparentForJpxMirror(
        "p_streets0_b.tga", 8));
    CHECK(jpb_IsTextureTransparentForJpxMirror(
        "P_CORUSB.TGA", 15));
    CHECK(jpb_IsTextureTransparentForJpxMirror(
        "T_PAL_16.TGA", 4));
    CHECK(jpb_IsTextureGlassForJpxMirror("T_WINDOW.TGA", 4));
    CHECK(jpb_IsTextureGlassForJpxMirror("T_ORANGE.TGA", 13));
    CHECK(!jpb_IsTextureGlassForJpxMirror("T_ORANGE.TGA", 11));

    CHECK(!jpb_IsTextureTransparentForJpxMirror(
        "P_TREEBO.TGA", 2));
    CHECK(!jpb_IsTextureTransparentForJpxMirror(
        "T_PAL_15.TGA", 4));
    CHECK(!jpb_IsTextureTransparentForJpxMirror(
        "P_TRAIN4.TGA", 18));
    return 0;
}

int main(void)
{
    CHECK(test_exact_classifiers() == 0);
    CHECK(test_jpx_mirror_names() == 0);
    puts("Transparent texture database tests passed");
    return 0;
}
