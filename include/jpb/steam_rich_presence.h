#ifndef JPB_STEAM_RICH_PRESENCE_H
#define JPB_STEAM_RICH_PRESENCE_H

#include "jpb/steam_interfaces.h"

class CSteamRichPresence {
public:
    CSteamRichPresence();
    ~CSteamRichPresence();

    void SetRichPresence(int inMenu, int currentLevel);
    void ClearRichPresence();

private:
    int m_initialized;
    int m_inMenu;
    int m_currentLevel;
};

extern CSteamRichPresence *g_SteamRicherPresence;

static_assert(sizeof(CSteamRichPresence) == 12,
    "CSteamRichPresence must match PDB type 0x7F21");

#endif
