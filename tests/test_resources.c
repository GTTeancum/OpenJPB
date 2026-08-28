#include "jpb/resources.h"
#include "jpb/win_movie_player.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                              \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static unsigned update_milliseconds;
static int update_calls;
static char played_video[JPB_RESOURCE_PATH_CAPACITY];

void PlayVideo(char *filename)
{
    snprintf(played_video, sizeof(played_video), "%s", filename);
}

static void update_hook(unsigned milliseconds, void *user_data)
{
    int *marker = (int *)user_data;

    update_milliseconds = milliseconds;
    ++update_calls;
    ++*marker;
}

int main(void)
{
    char mixed_case[] = "MODEL/TGA/ObI_WaN.TGA";
    char long_path[JPB_RESOURCE_PATH_CAPACITY + 1];
    const char *resolved;
    int marker = 0;

    CHECK(jpb_ResourceSetBasePath(NULL) == 0);
    CHECK(resource_getPath(
              "huffman.tab", JPB_RESOURCE_ANIMATION) == NULL);
    CHECK(jpb_ResourceSetBasePath("C:/game") == 1);
    CHECK(strcmp(
              resource_getPath(
                  "huffman.tab", JPB_RESOURCE_ANIMATION),
              "C:/game/res/animation/huffman.tab") == 0);
    CHECK(strcmp(
              resource_getPath(
                  "battle_d.bmd", JPB_RESOURCE_MODEL),
              "C:/game/res/model/battle_d.bmd") == 0);
    CHECK(strcmp(
              resource_getPath(
                  "pad.png", JPB_RESOURCE_CONTROLLER_SECONDARY),
              "C:/game/res/platform/PC/CONTROLLER/pad.png") == 0);
    CHECK(strcmp(
              resource_getPath(
                  "pad.png", JPB_RESOURCE_SWITCH_SECONDARY),
              "C:/game/res/platform/PC/SWITCH/pad.png") == 0);
    CHECK(resource_getPath("x", (ResourceType)-1) == NULL);
    CHECK(resource_getPath(
              "x", (ResourceType)JPB_RESOURCE_TYPE_COUNT) == NULL);

    jpb_ResourceSetLoadingUpdateHook(update_hook, &marker);
    resolved = resource_getPathWithExtension(
        "obi_wan", JPB_RESOURCE_MODEL, "bmd");
    CHECK(resolved != NULL);
    CHECK(strcmp(
              resolved,
              "C:/game/res/model/obi_wan.bmd") == 0);
    CHECK(update_calls == 1);
    CHECK(update_milliseconds == 50);
    CHECK(marker == 1);
    jpb_ResourceSetLoadingUpdateHook(NULL, NULL);

    resource_pathToLower(mixed_case);
    CHECK(strcmp(mixed_case, "model/tga/obi_wan.tga") == 0);

    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    CHECK(jpb_ResourceSetBasePath(long_path) == 0);
    CHECK(resource_getPath("x", JPB_RESOURCE_TEMP) == NULL);

    CHECK(jpb_ResourceSetBasePath("C:/game") == 1);
    winMovie_Init();
    winMovie_Play(9, (void *)1);
    CHECK(strcmp(
              played_video,
              "C:/game/res/movies/1080\\flipped\\"
              "photo_warning_English_1080_Flipped.ogg") == 0);
    CHECK(strcmp(
              ptrMovies[0],
              "1080\\flipped\\IntroFlippedVertical_converted.ogg") == 0);
    CHECK(strcmp(
              ptrMovies[7],
              "1080\\flipped\\End1080Flipped_converted.ogg") == 0);

    puts("resource tests passed");
    return 0;
}
