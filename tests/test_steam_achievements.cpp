#include "jpb/steam_achievements.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "check failed: %s (%s:%d)\n", \
            #condition, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

class ISteamUser {
public:
    void **vtable;
};

class ISteamUserStats {
public:
    void **vtable;
};

class ISteamUtils {
public:
    void **vtable;
};

struct AchievementTrace {
    int register_calls;
    int unregister_calls;
    int context_calls;
    int find_calls;
    int get_hsteam_user_calls;
    int app_id_calls;
    int logged_on_calls;
    int request_calls;
    int get_count_calls;
    int get_name_calls;
    int get_achievement_calls;
    int display_attribute_calls;
    int set_achievement_calls;
    int store_calls;
    int find_users[3];
    const char *find_versions[3];
    CCallbackBase *callbacks[3];
    int callback_ids[3];
    bool logged_on;
    bool request_result;
    bool store_result;
    bool achievement_values[3];
    std::vector<std::string> get_ids;
    std::vector<std::string> set_ids;
    std::vector<std::string> messages;
};

static AchievementTrace trace;
static ISteamUser fake_user;
static ISteamUserStats fake_stats;
static ISteamUtils fake_utils;
static void *user_vtable[2];
static void *stats_vtable[16];
static void *utils_vtable[10];

static const char *const achievement_ids[3] = {
    "JPB_Trophy_001",
    "ACH_A",
    "ACH_B"
};

static bool fake_logged_on(ISteamUser *)
{
    ++trace.logged_on_calls;
    return trace.logged_on;
}

static bool fake_request_current_stats(ISteamUserStats *)
{
    ++trace.request_calls;
    return trace.request_result;
}

static int achievement_index(const char *id)
{
    for (int index = 0; index < 3; ++index) {
        if (std::strcmp(id, achievement_ids[index]) == 0) {
            return index;
        }
    }
    return -1;
}

static bool fake_get_achievement(
    ISteamUserStats *, const char *id, bool *achieved)
{
    const int index = achievement_index(id);

    ++trace.get_achievement_calls;
    trace.get_ids.emplace_back(id);
    if (index < 0) {
        if (std::strcmp(id, "STATUS_A") == 0) {
            *achieved = false;
            return true;
        }
        if (std::strcmp(id, "STATUS_B") == 0) {
            *achieved = true;
            return true;
        }
        return false;
    }
    *achieved = trace.achievement_values[index];
    return true;
}

static bool fake_set_achievement(ISteamUserStats *, const char *id)
{
    ++trace.set_achievement_calls;
    trace.set_ids.emplace_back(id);
    return false;
}

static bool fake_store_stats(ISteamUserStats *)
{
    ++trace.store_calls;
    return trace.store_result;
}

static const char *fake_display_attribute(
    ISteamUserStats *, const char *id, const char *key)
{
    ++trace.display_attribute_calls;
    if (std::strcmp(id, "ACH_A") == 0) {
        return std::strcmp(key, "name") == 0 ? "Name A" : "Desc A";
    }
    if (std::strcmp(id, "ACH_B") == 0) {
        return std::strcmp(key, "name") == 0 ? "Name B" : "Desc B";
    }
    return "Platinum";
}

static std::uint32_t fake_get_num_achievements(ISteamUserStats *)
{
    ++trace.get_count_calls;
    return 3;
}

static const char *fake_get_achievement_name(
    ISteamUserStats *, std::uint32_t index)
{
    ++trace.get_name_calls;
    return achievement_ids[index];
}

static std::uint32_t fake_get_app_id(ISteamUtils *)
{
    ++trace.app_id_calls;
    return 12345;
}

extern "C" void SteamAPI_RegisterCallback(
    CCallbackBase *callback, int callback_id)
{
    const int index = trace.register_calls++;

    trace.callbacks[index] = callback;
    trace.callback_ids[index] = callback_id;
    callback->m_iCallback = callback_id;
    callback->m_nCallbackFlags |=
        CCallbackBase::k_ECallbackFlagsRegistered;
}

extern "C" void SteamAPI_UnregisterCallback(CCallbackBase *callback)
{
    ++trace.unregister_calls;
    callback->m_nCallbackFlags &=
        ~CCallbackBase::k_ECallbackFlagsRegistered;
}

extern "C" void *SteamInternal_ContextInit(void *context_init_data)
{
    void **state = static_cast<void **>(context_init_data);

    ++trace.context_calls;
    if (state[2] == nullptr) {
        if (state[0] == reinterpret_cast<void *>(
                            &SteamInternal_Init_SteamUser)) {
            SteamInternal_Init_SteamUser(
                reinterpret_cast<ISteamUser **>(&state[2]));
        } else if (state[0] == reinterpret_cast<void *>(
                                   &SteamInternal_Init_SteamUserStats)) {
            SteamInternal_Init_SteamUserStats(
                reinterpret_cast<ISteamUserStats **>(&state[2]));
        } else if (state[0] == reinterpret_cast<void *>(
                                   &SteamInternal_Init_SteamUtils)) {
            SteamInternal_Init_SteamUtils(
                reinterpret_cast<ISteamUtils **>(&state[2]));
        }
    }
    return &state[2];
}

extern "C" void *SteamInternal_FindOrCreateUserInterface(
    int steam_user, const char *version)
{
    const int index = trace.find_calls++;

    trace.find_users[index] = steam_user;
    trace.find_versions[index] = version;
    if (std::strcmp(version, "SteamUser023") == 0) {
        return &fake_user;
    }
    if (std::strcmp(
            version,
            "STEAMUSERSTATS_INTERFACE_VERSION012") == 0) {
        return &fake_stats;
    }
    if (std::strcmp(version, "SteamUtils010") == 0) {
        return &fake_utils;
    }
    return nullptr;
}

extern "C" int SteamAPI_GetHSteamUser(void)
{
    ++trace.get_hsteam_user_calls;
    return 73;
}

void WriteToOutputFile(const char *text)
{
    trace.messages.emplace_back(text);
}

static CCallbackBase *find_callback(int callback_id)
{
    for (int index = 0; index < trace.register_calls; ++index) {
        if (trace.callback_ids[index] == callback_id) {
            return trace.callbacks[index];
        }
    }
    return nullptr;
}

int main()
{
    trace = AchievementTrace{};
    std::memset(user_vtable, 0, sizeof(user_vtable));
    std::memset(stats_vtable, 0, sizeof(stats_vtable));
    std::memset(utils_vtable, 0, sizeof(utils_vtable));

    user_vtable[1] = reinterpret_cast<void *>(&fake_logged_on);
    stats_vtable[0] = reinterpret_cast<void *>(
        &fake_request_current_stats);
    stats_vtable[6] = reinterpret_cast<void *>(&fake_get_achievement);
    stats_vtable[7] = reinterpret_cast<void *>(&fake_set_achievement);
    stats_vtable[10] = reinterpret_cast<void *>(&fake_store_stats);
    stats_vtable[12] = reinterpret_cast<void *>(
        &fake_display_attribute);
    stats_vtable[14] = reinterpret_cast<void *>(
        &fake_get_num_achievements);
    stats_vtable[15] = reinterpret_cast<void *>(
        &fake_get_achievement_name);
    utils_vtable[9] = reinterpret_cast<void *>(&fake_get_app_id);
    fake_user.vtable = user_vtable;
    fake_stats.vtable = stats_vtable;
    fake_utils.vtable = utils_vtable;

    trace.logged_on = true;
    trace.request_result = true;
    trace.store_result = true;
    trace.achievement_values[0] = false;
    trace.achievement_values[1] = false;
    trace.achievement_values[2] = false;

    Achievement_t achievements[2] = {};
    achievements[0].m_pchAchievementID = "ACH_A";
    achievements[1].m_pchAchievementID = "ACH_B";
    std::strcpy(achievements[0].m_rgchName, "STATUS_A");
    std::strcpy(achievements[1].m_rgchName, "STATUS_B");

    CHECK(g_SteamAchievements == nullptr);
    {
        CSteamAchievements manager(achievements, 2);
        CCallbackBase *received = find_callback(1101);
        CCallbackBase *stored = find_callback(1102);
        CCallbackBase *achievement = find_callback(1103);

        CHECK(trace.register_calls == 3);
        CHECK(received != nullptr);
        CHECK(stored != nullptr);
        CHECK(achievement != nullptr);
        CHECK(received->GetCallbackSizeBytes() == 24);
        CHECK(stored->GetCallbackSizeBytes() == 16);
        CHECK(achievement->GetCallbackSizeBytes() == 152);
        CHECK(trace.find_calls == 3);
        CHECK(trace.find_users[0] == 0);
        CHECK(std::strcmp(trace.find_versions[0], "SteamUtils010") == 0);
        CHECK(std::strcmp(
            trace.find_versions[1],
            "STEAMUSERSTATS_INTERFACE_VERSION012") == 0);
        CHECK(std::strcmp(trace.find_versions[2], "SteamUser023") == 0);
        CHECK(trace.find_users[1] == 73);
        CHECK(trace.find_users[2] == 73);
        CHECK(trace.get_hsteam_user_calls == 2);
        CHECK(trace.app_id_calls == 1);
        CHECK(trace.request_calls == 1);
        CHECK(trace.get_count_calls == 1);
        CHECK(trace.get_name_calls == 3);
        CHECK(trace.get_achievement_calls == 3);
        CHECK(manager.GetAchievmentStatus(0) == 0);
        CHECK(!manager.SetAchievement("ACH_A"));
        CHECK(trace.set_achievement_calls == 0);

        UserStatsReceived_t stats_received = {};
        stats_received.m_nGameID = 9876;
        stats_received.m_eResult = k_EResultOK;
        received->Run(&stats_received, false, UINT64_C(0));
        CHECK(!manager.SetAchievement("ACH_A"));

        stats_received.m_nGameID = 12345;
        stats_received.m_eResult = static_cast<EResult>(2);
        received->Run(&stats_received);
        CHECK(!manager.SetAchievement("ACH_A"));

        trace.achievement_values[2] = true;
        stats_received.m_eResult = k_EResultOK;
        received->Run(&stats_received, false, UINT64_C(19));
        CHECK(trace.get_achievement_calls == 5);
        CHECK(trace.get_ids[3] == "STATUS_A");
        CHECK(trace.get_ids[4] == "STATUS_B");
        CHECK(trace.display_attribute_calls == 4);
        CHECK(std::strcmp(achievements[0].m_rgchName, "Name A") == 0);
        CHECK(std::strcmp(
            achievements[0].m_rgchDescription, "Desc A") == 0);
        CHECK(std::strcmp(achievements[1].m_rgchName, "Name B") == 0);
        CHECK(manager.GetAchievmentStatus(1) == 1);

        CHECK(manager.SetAchievement("ACH_A"));
        CHECK(trace.set_achievement_calls == 1);
        CHECK(trace.set_ids[0] == "ACH_A");
        CHECK(trace.store_calls == 1);

        CHECK(manager.SetAchievement("ACH_B"));
        CHECK(trace.set_achievement_calls == 3);
        CHECK(trace.set_ids[1] == "ACH_B");
        CHECK(trace.set_ids[2] == "JPB_Trophy_001");
        CHECK(trace.store_calls == 2);

        trace.store_result = false;
        CHECK(!manager.SetAchievement("ACH_A"));
        CHECK(trace.set_ids[3] == "ACH_A");
        CHECK(trace.set_ids[4] == "JPB_Trophy_001");
        CHECK(trace.store_calls == 3);

        trace.logged_on = false;
        CHECK(!manager.RequestStats());
        CHECK(trace.request_calls == 1);
        trace.logged_on = true;
        trace.request_result = false;
        CHECK(!manager.RequestStats());
        CHECK(trace.request_calls == 2);

        UserStatsStored_t stats_stored = {};
        stats_stored.m_nGameID = 12345;
        stats_stored.m_eResult = static_cast<EResult>(2);
        stored->Run(&stats_stored);

        UserAchievementStored_t achievement_stored = {};
        achievement->Run(&achievement_stored, false, UINT64_C(7));

        SteamAPI_UnregisterCallback(stored);
        CHECK(trace.unregister_calls == 1);
    }
    CHECK(trace.unregister_calls == 3);

    std::puts("steam achievements tests passed");
    return 0;
}
