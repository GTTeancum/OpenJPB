#ifndef JPB_STEAM_ACHIEVEMENTS_H
#define JPB_STEAM_ACHIEVEMENTS_H

#include "jpb/steam_callback.h"
#include "jpb/steam_interfaces.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

enum EResult : std::int32_t {
    k_EResultNone = 0,
    k_EResultOK = 1
};

struct SteamID_t {
    std::uint32_t low;
    std::uint32_t high;
};

struct UserStatsReceived_t {
    enum { k_iCallback = 1101 };

    std::uint64_t m_nGameID;
    EResult m_eResult;
    SteamID_t m_steamIDUser;
    std::uint32_t reserved;
};

struct UserStatsStored_t {
    enum { k_iCallback = 1102 };

    std::uint64_t m_nGameID;
    EResult m_eResult;
    std::uint32_t reserved;
};

struct UserAchievementStored_t {
    enum { k_iCallback = 1103 };

    std::uint64_t m_nGameID;
    bool m_bGroupAchievement;
    char m_rgchAchievementName[128];
    std::uint8_t reserved0[3];
    std::uint32_t m_nCurProgress;
    std::uint32_t m_nMaxProgress;
    std::uint32_t reserved1;
};

struct Achievement_t {
    int m_eAchievementID;
    const char *m_pchAchievementID;
    char m_rgchName[128];
    char m_rgchDescription[256];
    bool m_bAchieved;
    int m_iIconImage;
};

class CSteamAchievements {
public:
    CSteamAchievements(
        Achievement_t *Achievements, int NumAchievements);

    int GetAchievmentStatus(int achievement);
    bool RequestStats();
    bool SetAchievement(const char *achievementID);

private:
    bool IsReadyToUnlockPlatinum();
    void OnUserStatsReceived(UserStatsReceived_t *pCallback);
    void OnUserStatsStored(UserStatsStored_t *pCallback);
    void OnAchievementStored(UserAchievementStored_t *pCallback);

    std::uint64_t m_iAppID;
    Achievement_t *m_pAchievements;
    int m_iNumAchievements;
    bool m_bInitialized;
    std::map<std::string, bool> m_achievementStatusMap;
    CCallback<CSteamAchievements, UserStatsReceived_t, false>
        m_CallbackUserStatsReceived;
    CCallback<CSteamAchievements, UserStatsStored_t, false>
        m_CallbackUserStatsStored;
    CCallback<CSteamAchievements, UserAchievementStored_t, false>
        m_CallbackAchievementStored;
};

extern CSteamAchievements *g_SteamAchievements;
extern Achievement_t g_Achievements[43];

static_assert(sizeof(SteamID_t) == 8,
    "SteamID_t must match the callback's packed PDB field");
static_assert(offsetof(UserStatsReceived_t, m_steamIDUser) == 12,
    "UserStatsReceived_t Steam ID offset must match the PDB");
static_assert(sizeof(UserStatsReceived_t) == 24,
    "UserStatsReceived_t must match the PDB");
static_assert(sizeof(UserStatsStored_t) == 16,
    "UserStatsStored_t must match the PDB");
static_assert(offsetof(UserAchievementStored_t, m_nCurProgress) == 140,
    "UserAchievementStored_t progress offset must match the PDB");
static_assert(sizeof(UserAchievementStored_t) == 152,
    "UserAchievementStored_t must match the PDB");
static_assert(offsetof(Achievement_t, m_pchAchievementID) == 8,
    "Achievement_t ID pointer offset must match the PDB");
static_assert(offsetof(Achievement_t, m_bAchieved) == 400,
    "Achievement_t achieved offset must match the PDB");
static_assert(sizeof(Achievement_t) == 408,
    "Achievement_t must match PDB type 0x7BDE");

#if defined(_MSC_VER) && _ITERATOR_DEBUG_LEVEL == 0
static_assert(sizeof(CSteamAchievements) == 136,
    "CSteamAchievements must match PDB type 0x7DAA");
#endif

#endif
