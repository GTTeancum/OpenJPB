#ifndef JPB_COMBO_H
#define JPB_COMBO_H

#include "jpb/player.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_COMBO_REFERENCE_CAPACITY = 0x1000,
    JPB_COMBO_AWARD_THRESHOLD_CAPACITY = 4
};

enum JPBComboLoadResult {
    JPB_COMBO_OK = 0,
    JPB_COMBO_INVALID_ARGUMENT = -1,
    JPB_COMBO_IO_ERROR = -2,
    JPB_COMBO_STORAGE_TOO_SMALL = -3,
    JPB_COMBO_INVALID_SIZE = -4
};

/* Exact matched-PC PDB type 0x118A. */
struct Combo {
    int16_t Len;
    uint8_t Crap;
    uint8_t Index;
    uint32_t comboFlags;
    int32_t FunctPtr;
    char String[JPB_PLAYER_MOTION_NAME_BYTES];
    int16_t kdmin;
    int16_t kdmax;
    int16_t slack;
    int16_t numHits;
    int16_t prev;
    int32_t userData;
};

/* Exact PDB globals at matched-PC RVAs 0x4BB040 and 0x4BBB80. */
extern Combo combos1[48];
extern Combo combos2[48];

extern int16_t comboTally[2][32];
/*
 * Inferred name for the anonymous matched-PC words at RVA 0x4F2F60.
 * combo_InitComboData and combo_ValidComboAward prove four int32 entries
 * ending immediately before comboTally.
 */
extern int32_t jpb_comboAwardHitThreshold[
    JPB_COMBO_AWARD_THRESHOLD_CAPACITY];
/*
 * Exact unsized PDB array at matched-PC RVA 0x4BD900. Its initialized
 * storage runs to the next linked global, proving 88 short entries.
 */
extern int16_t gaPoints[88];

int combo_CheckCombo(
    int32_t *cpad, playerObject *player);
void combo_CheckHeldPad(
    int32_t *cpad,
    playerObject *player,
    int32_t mask,
    int32_t button);
int combo_CheckPreCombo(
    int32_t *cpad, playerObject *player);
int combo_GetHeldTime(
    playerObject *player, int32_t flag);
void combo_InitComboData(playerObject *player);
void combo_ReadCombo(
    int32_t *cpad, playerObject *player);
void combo_ResetComboEngine(
    uint32_t time, playerObject *player);
int combo_ValidComboAward(int jedi, int combo);
enum JPBComboLoadResult jpb_ComboLoadFile(
    const char *path,
    void *storage,
    size_t storage_capacity,
    Combo **combos,
    int16_t *combo_count);

#if defined(__cplusplus)
#define JPB_COMBO_STATIC_ASSERT static_assert
#else
#define JPB_COMBO_STATIC_ASSERT _Static_assert
#endif

JPB_COMBO_STATIC_ASSERT(
    sizeof(Combo) == 60,
    "Combo must match PDB type 0x118A");
JPB_COMBO_STATIC_ASSERT(
    offsetof(Combo, String) == 12,
    "Combo.String layout changed");
JPB_COMBO_STATIC_ASSERT(
    offsetof(Combo, userData) == 56,
    "Combo.userData layout changed");

#undef JPB_COMBO_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
