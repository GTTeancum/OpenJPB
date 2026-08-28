/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\win32\win_moviePlayer.c.
 * PDB module: 0102
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\win_moviePlayer.obj
 * Primary source: W:\SWJediPowerBattles\work\win32\win_moviePlayer.c
 * Compiler language: c
 * Emitted procedures: 2
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/resources.h"
#include "jpb/win_movie_player.h"

extern void PlayVideo(char *filename);

/* Exact PDB global at matched-PC RVA 0x4D4910. */
const char *ptrMovies[10] = {
    "1080\\flipped\\IntroFlippedVertical_converted.ogg",
    "1080\\flipped\\English1920Vertical_converted.ogg",
    "1080\\flipped\\HorizontalFlippedObi_converted.ogg",
    "1080\\flipped\\HorizontalFlippedQui_converted.ogg",
    "1080\\flipped\\HorizontalFlippedMace_converted.ogg",
    "1080\\flipped\\HorizontalFlippedAdi_converted.ogg",
    "1080\\flipped\\HorizontalFlippedPlo_converted.ogg",
    "1080\\flipped\\End1080Flipped_converted.ogg",
    "1080\\flipped\\Aspyr_Logo_1080_Flipped.ogg",
    "1080\\flipped\\photo_warning_English_1080_Flipped.ogg"
};

/* 0x12BDF0, 3 bytes, global, 0 named locals
 * winMovie_Init
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\win_moviePlayer.c
 */
void winMovie_Init(void)
{
}

/* 0x12BE00, 40 bytes, global, 3 named locals
 * winMovie_Play
 * PDB type: void (int, void*)
 * Source: W:\SWJediPowerBattles\work\win32\win_moviePlayer.c
 */
void winMovie_Play(int movieIndex, void *unused)
{
    const char *path;

    (void)unused;
    path = resource_getPath(ptrMovies[movieIndex], JPB_RESOURCE_MOVIE);
    PlayVideo((char *)path);
}
