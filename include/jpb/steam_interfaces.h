#ifndef JPB_STEAM_INTERFACES_H
#define JPB_STEAM_INTERFACES_H

#include <cstddef>
#include <cstdint>

class ISteamFriends;
class ISteamUser;
class ISteamUserStats;
class ISteamUtils;

extern "C" {
bool SteamAPI_Init(void);
bool SteamAPI_RestartAppIfNecessary(std::uint32_t app_id);
void SteamAPI_Shutdown(void);
void *SteamInternal_ContextInit(void *context_init_data);
void *SteamInternal_FindOrCreateUserInterface(
    int steam_user, const char *version);
int SteamAPI_GetHSteamUser(void);
void SteamAPI_RunCallbacks(void);
}

void SteamInternal_Init_SteamFriends(ISteamFriends **friends);
void SteamInternal_Init_SteamUser(ISteamUser **user);
void SteamInternal_Init_SteamUserStats(ISteamUserStats **stats);
void SteamInternal_Init_SteamUtils(ISteamUtils **utils);

namespace jpb_steam_detail {

struct CallbackCounterAndContext {
    void *initializer;
    void *callback_counter;
    void *context;
};

template <class Interface>
Interface *ResolveInterface(CallbackCounterAndContext *state)
{
    return *static_cast<Interface **>(SteamInternal_ContextInit(state));
}

} // namespace jpb_steam_detail

inline ISteamFriends *SteamFriends()
{
    static jpb_steam_detail::CallbackCounterAndContext state = {
        reinterpret_cast<void *>(&SteamInternal_Init_SteamFriends),
        nullptr,
        nullptr
    };
    return jpb_steam_detail::ResolveInterface<ISteamFriends>(&state);
}

inline ISteamUser *SteamUser()
{
    static jpb_steam_detail::CallbackCounterAndContext state = {
        reinterpret_cast<void *>(&SteamInternal_Init_SteamUser),
        nullptr,
        nullptr
    };
    return jpb_steam_detail::ResolveInterface<ISteamUser>(&state);
}

inline ISteamUserStats *SteamUserStats()
{
    static jpb_steam_detail::CallbackCounterAndContext state = {
        reinterpret_cast<void *>(&SteamInternal_Init_SteamUserStats),
        nullptr,
        nullptr
    };
    return jpb_steam_detail::ResolveInterface<ISteamUserStats>(&state);
}

inline ISteamUtils *SteamUtils()
{
    static jpb_steam_detail::CallbackCounterAndContext state = {
        reinterpret_cast<void *>(&SteamInternal_Init_SteamUtils),
        nullptr,
        nullptr
    };
    return jpb_steam_detail::ResolveInterface<ISteamUtils>(&state);
}

static_assert(
    sizeof(jpb_steam_detail::CallbackCounterAndContext) == 24,
    "Steam accessor context must match the PDB's 24-byte record");

#endif
