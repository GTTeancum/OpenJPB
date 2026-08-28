#ifndef JPB_STEAM_GAME_MANAGER_H
#define JPB_STEAM_GAME_MANAGER_H

#include "jpb/steam_callback.h"

#include <cstdint>

struct GameOverlayActivated_t {
    enum { k_iCallback = 331 };

    std::uint8_t m_bActive;
    bool m_bUserInitiated;
    std::uint16_t reserved2;
    std::uint32_t m_nAppID;
};

class CSteamGameManager {
public:
    CSteamGameManager();

    CCallbackBase *OverlayCallback()
    {
        return m_CallbackOverlayReceived.AsCallbackBase();
    }

private:
    void OnGameOverlayActivated(GameOverlayActivated_t *pCallback);

    CCallback<CSteamGameManager, GameOverlayActivated_t, false>
        m_CallbackOverlayReceived;
};

extern CSteamGameManager *g_SteamGameManager;

static_assert(sizeof(GameOverlayActivated_t) == 8,
    "GameOverlayActivated_t must match PDB type 0x7124");
static_assert(sizeof(CSteamGameManager) == 32,
    "CSteamGameManager must match PDB type 0x710B");

#endif
