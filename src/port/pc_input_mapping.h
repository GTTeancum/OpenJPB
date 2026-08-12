#ifndef JPB_PC_INPUT_MAPPING_H
#define JPB_PC_INPUT_MAPPING_H

#include <stdint.h>

/*
 * Physical keyboard state consumed by the pure PC translation seam. The
 * field names preserve the matched ReadKeyboardInput key assignments while
 * keeping Win32 virtual-key polling out of the reconstructed game core.
 */
typedef struct JPBPCGameplayKeyboardState {
    uint8_t moveUp;
    uint8_t moveLeft;
    uint8_t moveDown;
    uint8_t moveRight;
    uint8_t walkModifier;
    uint8_t zoomIn;
    uint8_t comboSouth;
    uint8_t comboWest;
    uint8_t comboNorth;
    uint8_t lockOn;
    uint8_t jumpBlockChord;
    uint8_t northBlockChord;
    uint8_t southBlockChord;
    uint8_t westBlockChord;
    uint8_t block;
    uint8_t jump;
    uint8_t start;
} JPBPCGameplayKeyboardState;

/*
 * Pure translation of the gameplay branch of PDB procedure
 * ReadKeyboardInput (wInput.c, RVA 0x12BE30). axis_x/axis_y receive the
 * original SDL convention: right/down are positive and left/up are negative.
 */
uint32_t jpb_PCMapGameplayKeyboard(
    const JPBPCGameplayKeyboardState *keyboard,
    float *axis_x,
    float *axis_y);

/* Complete ReadKeyboardInput state split. Menus use Shift as block/back,
 * Escape as jump/back, and Enter/Space as Start on the title or A/confirm
 * elsewhere; gameplay retains the authored action/chord layout. */
uint32_t jpb_PCMapKeyboard(
    const JPBPCGameplayKeyboardState *keyboard,
    int in_menu_state,
    int in_title_state,
    float *axis_x,
    float *axis_y);

/* Dependency-free controller ownership recovered from ReadJoystickInput,
 * checkStartDevice, and checkP2Device. P2 keeps one explicitly assigned
 * physical XInput user; P1 owns the first connected user that is not P2's. */
int jpb_PCControllerUserForPlayer(
    unsigned player,
    int player_count,
    int player_two_user,
    uint32_t connected_mask,
    unsigned *user_index);

/* Chooses P2's initial controller at the join boundary. A keyboard-owned P1
 * leaves the first controller available; a controller-owned P1 reserves it. */
int jpb_PCChoosePlayerTwoUser(
    int player_one_uses_keyboard,
    uint32_t connected_mask,
    unsigned *user_index);

/* Recovered AddControllerDevice rule: a disconnected P2 is restored only by
 * a newly attached, non-P1 physical controller, not an already attached pad. */
int jpb_PCChoosePlayerTwoAddedUser(
    int player_one_uses_keyboard,
    uint32_t connected_mask,
    uint32_t added_mask,
    unsigned *user_index);

/* ReadJoystickInput(0, ...) returns keyboard input immediately and only
 * scans P1's unreserved controllers when the keyboard produced no bits. */
uint32_t jpb_PCSelectPlayerOneInput(
    uint32_t keyboard_bits,
    uint32_t controller_bits);

#endif
