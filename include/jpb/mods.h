#ifndef JPB_MODS_H
#define JPB_MODS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { JPB_MOD_PATH = 1024, JPB_MOD_FIRST_ID = 115, JPB_MOD_LAST_ID = 254, JPB_MOD_SABER_ICON_BASE = 2000 };
typedef struct JPBModCharacter {
    int modelId, animationDonor, forceDonor, isJedi, hidden;
    int legacyHighestLevel, legacySkillPercent;
    char id[96], name[128], model[64], soundBank[64];
    char bmd[JPB_MOD_PATH], cad[JPB_MOD_PATH], cmb[JPB_MOD_PATH], portrait[JPB_MOD_PATH];
    uint32_t colors[3], icons[3];
    char saberIcons[2][JPB_MOD_PATH];
} JPBModCharacter;
/* Loading is transactional: a malformed enabled package rejects the set. */
int jpb_ModsLoad(const char *game_root, char *error, size_t error_size);
void jpb_ModsReset(void);
void jpb_ModResetProgress(void);
/* Native save sidecar; leaves the raw legacy payload and proxy sidecar formats intact. */
int jpb_ModSaveCommit(const char *path, const void *payload, size_t size, const int models[2]);
/* 1 restored, 0 absent/stale, -1 invalid/I/O, -2 required package unavailable. */
int jpb_ModSaveRestore(const char *path, const void *payload, size_t size, int models[2]);
const char *jpb_ModSaveError(void);
/* Stable-package completion records; kept outside the fixed legacy arrays. */
int jpb_ModRecordLevel(int model_id, int level, uint32_t score);
int jpb_ModLevelPlayed(int model_id, int level);
uint32_t jpb_ModLevelScore(int model_id, int level);
/* Six-byte combo mask, seeded once from the caller's canonical defaults. */
uint8_t *jpb_ModComboMask(int model_id, const uint8_t initial[6]);
struct Upgrades;
/* Independent upgrades, initialized from canonical new-character defaults. */
struct Upgrades *jpb_ModUpgrades(int model_id);
int jpb_ModsSetAssetRoot(const char *game_root);
/* Resolve paths made relative to either a package or the installed resource tree. */
int jpb_ModsResolvePath(const char *path, char *output, size_t capacity);
int jpb_ModsBasePath(const char *path, char *output, size_t capacity);
size_t jpb_ModsCount(void);
const JPBModCharacter *jpb_ModCharacterById(int model_id);
const JPBModCharacter *jpb_ModCharacterAt(size_t index);
int jpb_ModDonor(int model_id);
int jpb_ModLastSelectable(int stock_last);
int jpb_ModToggleColor(int model_id);
int jpb_ModSelectColor(int model_id, int alternate);
const char *jpb_ModSaberIconPath(int model_id);
/* Runtime slots retain package identity while stock combat uses its donor ID. */
void jpb_ModSetPlayer(int player, int model_id);
const JPBModCharacter *jpb_ModPlayer(int player);
int jpb_ModPlayerModel(int player, int fallback);
int jpb_ModSaberNodes(int player, unsigned *base, unsigned *tip,
                     unsigned *second_base, unsigned *second_tip);
/* Relative paths retain res/... under each package; stock fallback is caller-owned. */
int jpb_ModsResolve(const char *relative_path, char *output, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
