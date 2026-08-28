#include "jpb/steam_game_manager.h"

#define _Static_assert static_assert
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/menu.h"
#include "jpb/world.h"
#undef _Static_assert

#include <cstdint>
#include <cstdio>
#include <cstring>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "check failed: %s (%s:%d)\n", \
            #condition, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

struct SteamCallbackTrace {
    int register_calls;
    int unregister_calls;
    int callback_id;
    CCallbackBase *callback;
};

static SteamCallbackTrace callback_trace;

extern "C" void SteamAPI_RegisterCallback(
    CCallbackBase *callback, int callback_id)
{
    ++callback_trace.register_calls;
    callback_trace.callback_id = callback_id;
    callback_trace.callback = callback;
    callback->m_iCallback = callback_id;
    callback->m_nCallbackFlags |=
        CCallbackBase::k_ECallbackFlagsRegistered;
}

extern "C" void SteamAPI_UnregisterCallback(CCallbackBase *callback)
{
    ++callback_trace.unregister_calls;
    callback_trace.callback = callback;
    callback->m_nCallbackFlags &=
        ~CCallbackBase::k_ECallbackFlagsRegistered;
}

static void reset_pause_state()
{
    std::memset(&GameStruct, 0, sizeof(GameStruct));
    std::memset(&menuVars, 0, sizeof(menuVars));
    LevelSelect = 1;
    p1Disconnected = 0;
    p2Disconnected = 0;
}

int main()
{
    GameOverlayActivated_t event = {};

    std::memset(&callback_trace, 0, sizeof(callback_trace));
    reset_pause_state();
    {
        CSteamGameManager manager;
        CCallbackBase *callback = manager.OverlayCallback();

        CHECK(sizeof(manager) == 32);
        CHECK(callback_trace.register_calls == 1);
        CHECK(callback_trace.callback == callback);
        CHECK(callback_trace.callback_id == 331);
        CHECK(callback->GetICallback() == 331);
        CHECK(callback->GetCallbackSizeBytes() == 8);

        event.m_bActive = 0;
        callback->Run(&event);
        CHECK(GameStruct.inMenuFlag == 0);

        event.m_bActive = 1;
        callback->Run(&event, false, UINT64_C(0));
        CHECK(GameStruct.inMenuFlag == 1);
        CHECK((GameStruct.GameState & UINT32_C(0x02000000)) != 0);
        CHECK(menuVars.menuModeSP == 2);
        CHECK(menuVars.menuMode[1] == 0x40);
        CHECK(menuVars.menuMode[2] == 0x14);
    }
    CHECK(callback_trace.unregister_calls == 1);

    {
        CSteamGameManager manager;

        CHECK(callback_trace.register_calls == 2);
        SteamAPI_UnregisterCallback(manager.OverlayCallback());
        CHECK(callback_trace.unregister_calls == 2);
    }
    CHECK(callback_trace.unregister_calls == 2);

    std::puts("steam game manager tests passed");
    return 0;
}
