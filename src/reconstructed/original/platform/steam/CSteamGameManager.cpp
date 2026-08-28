/*
 * REVIEWED RECONSTRUCTION.
 *
 * Provenance:
 *   - matched-PC PDB types 0x590F, 0x7088, 0x70D7, 0x710B, and 0x7124
 *   - direct retail disassembly at RVAs 0xE4A90-0xE4BD7
 *
 * Steam callback registration remains the original external API boundary.
 * No alternate registration behavior is supplied here.
 */

#include "jpb/steam_game_manager.h"

extern "C" void menu_enterPauseMode(void);

/* Exact matched-PC PDB global at RVA 0x582628. */
CSteamGameManager *g_SteamGameManager;

/* 0xE4A90, 66 bytes, global, 1 named locals
 * CSteamGameManager::CSteamGameManager
 * PDB type: void CSteamGameManager::()
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamGameManager.cpp
 */
CSteamGameManager::CSteamGameManager()
    : m_CallbackOverlayReceived(
          this, &CSteamGameManager::OnGameOverlayActivated)
{
}

/* 0xE4AE0, 24 bytes, global, 1 named locals
 * CCallbackImpl<8>::~CCallbackImpl<8>
 * PDB type: void CCallbackImpl<8>::()
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
CCallbackImpl<8>::~CCallbackImpl()
{
    if ((m_nCallbackFlags & k_ECallbackFlagsRegistered) != 0) {
        SteamAPI_UnregisterCallback(this);
    }
}

/* 0xE4B00, 70 bytes, global, 1 named locals
 * CCallback<CSteamGameManager,GameOverlayActivated_t,0>::`scalar deleting destructor'
 * PDB type: compiler-generated scalar deleting destructor
 * Source: no line mapping
 */

/* 0xE4B50, 70 bytes, global, 1 named locals
 * CCallbackImpl<8>::`scalar deleting destructor'
 * PDB type: compiler-generated scalar deleting destructor
 * Source: no line mapping
 */

/* 0xE4BA0, 6 bytes, global, 1 named locals
 * CCallbackImpl<8>::GetCallbackSizeBytes
 * PDB type: int CCallbackImpl<8>::()
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
int CCallbackImpl<8>::GetCallbackSizeBytes()
{
    return 8;
}

/* 0xE4BB0, 10 bytes, global, 2 named locals
 * CSteamGameManager::OnGameOverlayActivated
 * PDB type: void CSteamGameManager::(GameOverlayActivated_t*)
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamGameManager.cpp
 */
void CSteamGameManager::OnGameOverlayActivated(
    GameOverlayActivated_t *pCallback)
{
    if (pCallback->m_bActive != 0) {
        menu_enterPauseMode();
    }
}

/* 0xE4BC0, 11 bytes, global, 2 named locals
 * CCallback<CSteamGameManager,GameOverlayActivated_t,0>::Run
 * PDB type: void CCallback<CSteamGameManager,GameOverlayActivated_t,0>::(void*)
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_internal.h
 */
template <>
void CCallback<CSteamGameManager,GameOverlayActivated_t,0>::Run(
    void *pvParam)
{
    (m_pObj->*m_Func)(static_cast<GameOverlayActivated_t *>(pvParam));
}

/* 0xE4BD0, 7 bytes, global, 4 named locals
 * CCallbackImpl<8>::Run
 * PDB type: void CCallbackImpl<8>::(void*, bool, unsigned __int64)
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
void CCallbackImpl<8>::Run(
    void *pvParam, bool, std::uint64_t)
{
    Run(pvParam);
}
