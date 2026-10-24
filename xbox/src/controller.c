/* SDL is confined to this translation unit, separate from the recovered
   game's SDL ABI. Publish the same neutral controller state as PC XInput. */
#include <SDL.h>
#include "jpb/input.h"

enum { JPB_XBOX_PADS = 4 };
/* Controllers in the order they were connected; slots 0 and 1 are the
   game's player one and player two. */
static SDL_GameController *controllers[JPB_XBOX_PADS];

/* Test-only remote pads for XEMU, one 4-word record per player. Only a
   debugger attached to the emulator writes them (tools/xemu_pad.py); retail
   input ignores a record until its magic word is present. [1] button mask
   (bit i = button[i]), [2] left stick X | Y<<16 as int16, [3] left | right
   trigger<<16. */
#define JPB_XBOX_REMOTE_PAD_MAGIC 0x5242504Au
volatile uint32_t jpb_XboxRemotePad[2][4];

int jpb_XboxControllerInit(void)
{
    return SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
}

static void update_controllers(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERDEVICEADDED) {
            for (int i = 0; i < JPB_XBOX_PADS; ++i) {
                if (controllers[i]) continue;
                controllers[i] = SDL_GameControllerOpen(event.cdevice.which);
                break;
            }
        }
        if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            for (int i = 0; i < JPB_XBOX_PADS; ++i) {
                if (controllers[i] && SDL_JoystickInstanceID(
                        SDL_GameControllerGetJoystick(controllers[i])) ==
                        event.cdevice.which) {
                    SDL_GameControllerClose(controllers[i]);
                    controllers[i] = NULL;
                }
            }
        }
    }
    SDL_GameControllerUpdate();
}

static int remote_pad(int pad, JPBControllerState *state)
{
    volatile uint32_t *record = jpb_XboxRemotePad[pad];
    if (record[0] != JPB_XBOX_REMOTE_PAD_MAGIC) return 0;
    uint32_t buttons = record[1];
    state->name="Xbox Series X Controller";
    state->axis[0]=(int16_t)(record[2] & 0xffffu);
    state->axis[1]=(int16_t)(record[2] >> 16);
    state->axis[4]=(int16_t)(record[3] & 0xffffu);
    state->axis[5]=(int16_t)(record[3] >> 16);
    for (int i = 0; i < 21; ++i) state->button[i]=(buttons >> i) & 1u;
    return 1;
}

/* Poll every attached pad once per frame, then read players with
   jpb_XboxControllerRead. */
void jpb_XboxControllerUpdate(void)
{
    update_controllers();
}

int jpb_XboxControllerConnected(int pad)
{
    if (pad < 0 || pad > 1) return 0;
    return controllers[pad] != NULL ||
        jpb_XboxRemotePad[pad][0] == JPB_XBOX_REMOTE_PAD_MAGIC;
}

unsigned jpb_XboxControllerCount(void)
{
    unsigned count = 0;
    for (int pad = 0; pad < 2; ++pad) count += jpb_XboxControllerConnected(pad);
    return count;
}

void jpb_XboxControllerRead(int pad, JPBControllerState *state)
{
    SDL_GameController *controller;
    SDL_memset(state,0,sizeof(*state));
    if (pad < 0 || pad > 1) return;
    if (remote_pad(pad, state)) return;
    controller = controllers[pad];
    if (!controller) return;
    state->name="Xbox Series X Controller";
    state->axis[0]=SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTX);
    state->axis[1]=SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTY);
    state->axis[4]=SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    state->axis[5]=SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    state->button[0]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_A);
    state->button[1]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_B);
    state->button[2]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_X);
    state->button[3]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_Y);
    state->button[4]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_BACK);
    state->button[6]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_START);
    state->button[9]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    state->button[10]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    state->button[11]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_DPAD_UP);
    state->button[12]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    state->button[13]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    state->button[14]=SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
}

/* Player one, for callers outside the per-frame input owner (movies). */
void jpb_XboxControllerPoll(JPBControllerState *state)
{
    update_controllers();
    jpb_XboxControllerRead(0, state);
}
