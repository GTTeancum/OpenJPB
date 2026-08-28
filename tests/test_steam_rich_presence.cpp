#include "jpb/steam_rich_presence.h"

#include <cstdio>
#include <cstring>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "check failed: %s (%s:%d)\n", \
            #condition, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

class ISteamUser {};

class ISteamFriends {
public:
    void **vtable;
};

struct RichPresenceTrace {
    int context_calls;
    int user_init_calls;
    int friends_init_calls;
    int get_user_calls;
    int find_calls;
    int set_calls;
    int clear_calls;
    int steam_user;
    const char *version;
    const char *key;
    const char *value;
};

static RichPresenceTrace trace;
static ISteamUser fake_user;
static ISteamFriends fake_friends;
static void *fake_friends_vtable[45];

static bool capture_set_rich_presence(
    ISteamFriends *, const char *key, const char *value)
{
    ++trace.set_calls;
    trace.key = key;
    trace.value = value;
    return false;
}

static void capture_clear_rich_presence(ISteamFriends *)
{
    ++trace.clear_calls;
}

void SteamInternal_Init_SteamUser(ISteamUser **user)
{
    ++trace.user_init_calls;
    *user = &fake_user;
}

extern "C" void *SteamInternal_ContextInit(void *context_init_data)
{
    void **state = static_cast<void **>(context_init_data);

    ++trace.context_calls;
    if (state[2] == nullptr &&
        state[0] == reinterpret_cast<void *>(
                        &SteamInternal_Init_SteamUser)) {
        SteamInternal_Init_SteamUser(
            reinterpret_cast<ISteamUser **>(&state[2]));
    } else if (state[2] == nullptr &&
               state[0] == reinterpret_cast<void *>(
                               &SteamInternal_Init_SteamFriends)) {
        ++trace.friends_init_calls;
        SteamInternal_Init_SteamFriends(
            reinterpret_cast<ISteamFriends **>(&state[2]));
    }
    return &state[2];
}

extern "C" void *SteamInternal_FindOrCreateUserInterface(
    int steam_user, const char *version)
{
    ++trace.find_calls;
    trace.steam_user = steam_user;
    trace.version = version;
    return &fake_friends;
}

extern "C" int SteamAPI_GetHSteamUser(void)
{
    ++trace.get_user_calls;
    return 73;
}

int main()
{
    static const char *const expected_level_strings[26] = {
        "#Playing_Level_FED",
        "#Playing_Level_MARSH",
        "#Playing_Level_THEED",
        "#Playing_Level_PALACE",
        "#Playing_Level_TATOOINE",
        "#Playing_Level_CORUS",
        "#Playing_Level_RUINS",
        "#Playing_Level_STREETS",
        "#Playing_Level_HANGAR",
        "#Playing_Level_CORE",
        "#Playing_Level_MINI1",
        "#Playing_Level_MINI2",
        "#Playing_Level_MINI3",
        "#Playing_Level_MINI4",
        "#Playing_Level_CORUS",
        "#Playing_Level_TRAINING1",
        "#Playing_Level_TRAINING2",
        "#Playing_Level_TRAINING3",
        "#Playing_Level_TRAINING4",
        "#Playing_Level_TRAINING5",
        "#Playing_Level_TRAINING6",
        "#Playing_Level_TRAINING7",
        nullptr,
        nullptr,
        "#Playing_Level_ARENA",
        nullptr
    };

    std::memset(&trace, 0, sizeof(trace));
    std::memset(fake_friends_vtable, 0, sizeof(fake_friends_vtable));
    fake_friends_vtable[43] = reinterpret_cast<void *>(
        &capture_set_rich_presence);
    fake_friends_vtable[44] = reinterpret_cast<void *>(
        &capture_clear_rich_presence);
    fake_friends.vtable = fake_friends_vtable;

    CHECK(g_SteamRicherPresence == nullptr);
    {
        CSteamRichPresence presence;

        CHECK(sizeof(presence) == 12);
        CHECK(trace.user_init_calls == 1);
        CHECK(trace.friends_init_calls == 1);
        CHECK(trace.get_user_calls == 1);
        CHECK(trace.find_calls == 1);
        CHECK(trace.steam_user == 73);
        CHECK(std::strcmp(trace.version, "SteamFriends017") == 0);

        presence.SetRichPresence(1, 7);
        CHECK(trace.set_calls == 1);
        CHECK(std::strcmp(trace.key, "steam_display") == 0);
        CHECK(std::strcmp(trace.value, "#Menu") == 0);

        presence.SetRichPresence(1, 7);
        CHECK(trace.set_calls == 1);

        for (int level = 1; level <= 26; ++level) {
            presence.SetRichPresence(0, level);
            CHECK(trace.set_calls == level + 1);
            CHECK(std::strcmp(trace.key, "steam_display") == 0);
            if (expected_level_strings[level - 1] == nullptr) {
                CHECK(trace.value == nullptr);
            } else {
                CHECK(std::strcmp(
                    trace.value,
                    expected_level_strings[level - 1]) == 0);
            }
        }

        presence.SetRichPresence(0, 26);
        CHECK(trace.set_calls == 27);

        presence.ClearRichPresence();
        CHECK(trace.clear_calls == 1);
    }
    CHECK(trace.clear_calls == 2);

    std::puts("steam rich presence tests passed");
    return 0;
}
