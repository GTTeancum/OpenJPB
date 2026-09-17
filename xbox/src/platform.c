/* nxdk platform boundary. No host desktop or official XDK dependencies. */
#include "jpb/whook.h"
#include "jpb/platform.h"
#include <hal/video.h>
#include <hal/debug.h>
#include <stdlib.h>

/* Xbox has no PC keyboard. Controller gameplay is supplied by the input
   provider, not fabricated keyboard events. */
int KeyPressed(int key) { (void)key; return 0; }
int KeyHeld(int key) { (void)key; return 0; }
int LastKey(void) { return 0; }
int ShiftKeyDown(void) { return 0; }
double atof(const char *value) { return strtod(value, NULL); }

void initXAstuff(void) { /* Matched retail function is empty. */ }

/* The software capture hooks own rendering; there is no SDL renderer to
   begin/end or reset a clip rectangle on this backend. */
void __StartRender(void) {}
void __EndRender(void) {}
void SDL_ResetClipRect(void) {}
void PresentWindow(void) { XVideoFlushFB(); }
void UpdateResolution(int width, int height, int window_mode)
{
    (void)window_mode;
    if (width != 720 || height != 480)
        debugPrint("Xbox: requested unsupported video mode %dx%d\n", width, height);
}

/* No Steam service exists on Xbox. Do not report achievement success. */
int platform_completeAchievement(int id) { (void)id; return 0; }
int platform_getCompleteAchievement(int id) { (void)id; return 0; }
