/* SDL is confined to this translation unit, separate from the recovered
   game's SDL ABI. Publish the same neutral controller state as PC XInput. */
#include <SDL.h>
#include "jpb/input.h"

static SDL_GameController *controller;

int jpb_XboxControllerInit(void)
{
    return SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
}

void jpb_XboxControllerPoll(JPBControllerState *state)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERDEVICEADDED && !controller)
            controller = SDL_GameControllerOpen(event.cdevice.which);
        if (event.type == SDL_CONTROLLERDEVICEREMOVED && controller &&
            SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)) == event.cdevice.which) {
            SDL_GameControllerClose(controller);
            controller = NULL;
        }
    }
    SDL_GameControllerUpdate();
    SDL_memset(state,0,sizeof(*state));
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
