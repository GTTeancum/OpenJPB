#include "jpb/linkstubs.h"
#include "jpb/overlay.h"
#include "jpb/stubs.h"
#include "jpb/win_audio.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    unsigned char source[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    unsigned char destination[8] = {0};

    if (kmAudio_IsAudioStarted() != 1 || kmAudio_StartUp() != 1) {
        return 1;
    }
    kmAudio_ShutDown();

    ovrlay_DoAlphaScreen();
    ovrlay_DoWork();
    ovrlay_InitOverLay(1, 2);
    ovrlay_SetForceBar(1, 2, 3);
    ovrlay_SetForceGem(1, 2, 3);
    ovrlay_SetLifeBar(1, 2, 3);
    ovrlay_SetStunBar(1, 2);
    ovrlay_gSetBarColor(1, 2);
    ovrlay_gToggleOverlay(1);

    LoadBackdrop(NULL, 0);
    PlotBackdrop(0);
    SetBackdropBrightness(0);
    _DisplayIcon(0, 0, 0);
    cd_gInitMusic();
    cd_gPause();
    cd_gPlay();
    cd_gPlayTrack();
    cd_gStop();
    psx_DrawBlur();
    sound_Debug();

    CopyMemLong(destination, source, sizeof(source));
    if (memcmp(destination, source, sizeof(source)) != 0 ||
        OpenTIM(NULL) != 0 ||
        platform_completeLevel(4) != 0 ||
        platform_enterLevel(4) != 0 ||
        platform_isSuspended() != 0 ||
        getScratchAddr(17) - getScratchAddr(0) != 17) {
        return 1;
    }

    puts("win audio compatibility tests passed");
    return 0;
}
