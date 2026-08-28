/*
 * COMPLETE REVIEWED RECONSTRUCTION OF ALL PROJECT PROCEDURES.
 * PDB module: 0061
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\CSteamAchievements.obj
 * Primary source: W:\SWJediPowerBattles\work\platform\steam\CSteamAchievements.cpp
 * Compiler language: c++
 * Emitted procedures: 49
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/steam_achievements.h"

#include <cstdio>

extern void WriteToOutputFile(const char *text);

namespace {

constexpr const char *kPlatinumAchievement = "JPB_Trophy_001";

template <class Return, class Interface, class... Args>
Return CallSteamVirtual(
    Interface *object, std::size_t slot, Args... args)
{
    typedef Return (*Function)(Interface *, Args...);
    void **vtable = *reinterpret_cast<void ***>(object);

    return reinterpret_cast<Function>(vtable[slot])(object, args...);
}

bool SteamUserLoggedOn(ISteamUser *user)
{
    return CallSteamVirtual<bool>(user, 1);
}

bool SteamStatsRequestCurrent(ISteamUserStats *stats)
{
    return CallSteamVirtual<bool>(stats, 0);
}

bool SteamStatsGetAchievement(
    ISteamUserStats *stats, const char *id, bool *achieved)
{
    return CallSteamVirtual<bool>(stats, 6, id, achieved);
}

bool SteamStatsSetAchievement(ISteamUserStats *stats, const char *id)
{
    return CallSteamVirtual<bool>(stats, 7, id);
}

bool SteamStatsStore(ISteamUserStats *stats)
{
    return CallSteamVirtual<bool>(stats, 10);
}

const char *SteamStatsDisplayAttribute(
    ISteamUserStats *stats, const char *id, const char *key)
{
    return CallSteamVirtual<const char *>(stats, 12, id, key);
}

std::uint32_t SteamStatsAchievementCount(ISteamUserStats *stats)
{
    return CallSteamVirtual<std::uint32_t>(stats, 14);
}

const char *SteamStatsAchievementName(
    ISteamUserStats *stats, std::uint32_t index)
{
    return CallSteamVirtual<const char *>(stats, 15, index);
}

std::uint32_t SteamUtilsAppID(ISteamUtils *utils)
{
    return CallSteamVirtual<std::uint32_t>(utils, 9);
}

} // namespace

/* Exact matched-PC PDB global at RVA 0x582620. */
CSteamAchievements *g_SteamAchievements;

/* 0xE3510, 313 bytes, global, 11 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::basic_string<char,std::char_traits<char>,std::allocator<char> ><char *,0>
 * PDB type: void std::basic_string<char,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xE3650, 100 bytes, global, 5 named locals
 * std::operator<<char,std::char_traits<char>,std::allocator<char> >
 * PDB type: bool (const std::basic_string<ch...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xE36C0, 191 bytes, global, 8 named locals
 * std::_Tree_val<std::_Tree_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,bool> > >::_Erase_tree<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,bool>,void *> > >
 * PDB type: void std::_Tree_val<std::_Tree_s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xE3780, 197 bytes, global, 7 named locals
 * std::_Integral_to_string<char,int>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\string
 */

/* 0xE3850, 433 bytes, global, 9 named locals
 * std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,bool,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,bool> > >::_Try_emplace<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >
 * PDB type: std::pair<std::_Tree_node<std::p...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\map
 */

/* 0xE3A10, 64 bytes, global, 3 named locals
 * std::_UIntegral_to_buff<char,unsigned int>
 * PDB type: char* (char*, unsigned)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\string
 */

/* 0xE3A50, 711 bytes, global, 12 named locals
 * CSteamAchievements::CSteamAchievements
 * PDB type: void CSteamAchievements::(Achiev...
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamAchievements.cpp
 */
CSteamAchievements::CSteamAchievements(
    Achievement_t *Achievements, int NumAchievements)
    : m_iAppID(0),
      m_bInitialized(false),
      m_achievementStatusMap(),
      m_CallbackUserStatsReceived(
          this, &CSteamAchievements::OnUserStatsReceived),
      m_CallbackUserStatsStored(
          this, &CSteamAchievements::OnUserStatsStored),
      m_CallbackAchievementStored(
          this, &CSteamAchievements::OnAchievementStored)
{
    WriteToOutputFile("Getting App ID");
    m_iAppID = SteamUtilsAppID(SteamUtils());
    m_pAchievements = Achievements;
    m_iNumAchievements = NumAchievements;

    WriteToOutputFile("Requesting Stats");
    RequestStats();

    const int achievement_count = static_cast<int>(
        SteamStatsAchievementCount(SteamUserStats()));
    for (int index = 0; index < achievement_count; ++index) {
        const char *achievement_id = SteamStatsAchievementName(
            SteamUserStats(), static_cast<std::uint32_t>(index));
        bool achieved = false;

        if (SteamStatsGetAchievement(
                SteamUserStats(), achievement_id, &achieved)) {
            m_achievementStatusMap[achievement_id] = achieved;
        }
    }
}

/* 0xE3D20, 24 bytes, global, 1 named locals
 * CCallback<CSteamAchievements,UserAchievementStored_t,0>::~CCallback<CSteamAchievements,UserAchievementStored_t,0>
 * PDB type: void CCallback<CSteamAchievement...
 * Source: no line mapping
 */

/* 0xE3D40, 24 bytes, global, 1 named locals
 * CCallback<CSteamAchievements,UserStatsReceived_t,0>::~CCallback<CSteamAchievements,UserStatsReceived_t,0>
 * PDB type: void CCallback<CSteamAchievement...
 * Source: no line mapping
 */

/* 0xE3D60, 24 bytes, global, 1 named locals
 * CCallback<CSteamAchievements,UserStatsStored_t,0>::~CCallback<CSteamAchievements,UserStatsStored_t,0>
 * PDB type: void CCallback<CSteamAchievement...
 * Source: no line mapping
 */

/* 0xE3D80, 24 bytes, global, 1 named locals
 * CCallbackImpl<16>::~CCallbackImpl<16>
 * PDB type: void CCallbackImpl<16>::()
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
CCallbackImpl<16>::~CCallbackImpl()
{
    if ((m_nCallbackFlags & k_ECallbackFlagsRegistered) != 0) {
        SteamAPI_UnregisterCallback(this);
    }
}

/* 0xE3DA0, 24 bytes, global, 1 named locals
 * CCallbackImpl<24>::~CCallbackImpl<24>
 * PDB type: void CCallbackImpl<24>::()
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
CCallbackImpl<24>::~CCallbackImpl()
{
    if ((m_nCallbackFlags & k_ECallbackFlagsRegistered) != 0) {
        SteamAPI_UnregisterCallback(this);
    }
}

/* 0xE3DC0, 24 bytes, global, 1 named locals
 * CCallbackImpl<152>::~CCallbackImpl<152>
 * PDB type: void CCallbackImpl<152>::()
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
CCallbackImpl<152>::~CCallbackImpl()
{
    if ((m_nCallbackFlags & k_ECallbackFlagsRegistered) != 0) {
        SteamAPI_UnregisterCallback(this);
    }
}

/* 0xE3DE0, 20 bytes, global, 2 named locals
 * std::_Alloc_construct_ptr<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,bool>,void *> > >::~_Alloc_construct_ptr<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,bool>,void *> > >
 * PDB type: void std::_Alloc_construct_ptr<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0xE3E00, 42 bytes, global, 1 named locals
 * std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,bool,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,bool> > >::~map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,bool,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,bool> > >
 * PDB type: void std::map<std::basic_string<...
 * Source: no line mapping
 */

/* 0xE3E30, 112 bytes, global, 6 named locals
 * std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >::operator()
 * PDB type: bool std::less<std::basic_string...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\type_traits
 */

/* 0xE3EA0, 70 bytes, global, 1 named locals
 * CCallback<CSteamAchievements,UserAchievementStored_t,0>::`scalar deleting destructor'
 * PDB type: void* CCallback<CSteamAchievemen...
 * Source: no line mapping
 */

/* 0xE3EF0, 70 bytes, global, 1 named locals
 * CCallback<CSteamAchievements,UserStatsReceived_t,0>::`scalar deleting destructor'
 * PDB type: void* CCallback<CSteamAchievemen...
 * Source: no line mapping
 */

/* 0xE3F40, 70 bytes, global, 1 named locals
 * CCallback<CSteamAchievements,UserStatsStored_t,0>::`scalar deleting destructor'
 * PDB type: void* CCallback<CSteamAchievemen...
 * Source: no line mapping
 */

/* 0xE3F90, 70 bytes, global, 1 named locals
 * CCallbackImpl<16>::`scalar deleting destructor'
 * PDB type: void* CCallbackImpl<16>::(unsign...
 * Source: no line mapping
 */

/* 0xE3FE0, 70 bytes, global, 1 named locals
 * CCallbackImpl<24>::`scalar deleting destructor'
 * PDB type: void* CCallbackImpl<24>::(unsign...
 * Source: no line mapping
 */

/* 0xE4030, 70 bytes, global, 1 named locals
 * CCallbackImpl<152>::`scalar deleting destructor'
 * PDB type: void* CCallbackImpl<152>::(unsig...
 * Source: no line mapping
 */

/* 0xE4080, 23 bytes, global, 2 named locals
 * CSteamAchievements::GetAchievmentStatus
 * PDB type: int CSteamAchievements::(int)
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamAchievements.cpp
 */
int CSteamAchievements::GetAchievmentStatus(int achievement)
{
    return m_pAchievements[achievement].m_bAchieved;
}

/* 0xE40A0, 6 bytes, global, 1 named locals
 * CCallbackImpl<16>::GetCallbackSizeBytes
 * PDB type: int CCallbackImpl<16>::()
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
int CCallbackImpl<16>::GetCallbackSizeBytes()
{
    return 16;
}

/* 0xE40B0, 6 bytes, global, 1 named locals
 * CCallbackImpl<24>::GetCallbackSizeBytes
 * PDB type: int CCallbackImpl<24>::()
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
int CCallbackImpl<24>::GetCallbackSizeBytes()
{
    return 24;
}

/* 0xE40C0, 6 bytes, global, 1 named locals
 * CCallbackImpl<152>::GetCallbackSizeBytes
 * PDB type: int CCallbackImpl<152>::()
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
int CCallbackImpl<152>::GetCallbackSizeBytes()
{
    return 152;
}

/* 0xE40D0, 195 bytes, global, 5 named locals
 * CSteamAchievements::IsReadyToUnlockPlatinum
 * PDB type: bool CSteamAchievements::()
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamAchievements.cpp
 */
bool CSteamAchievements::IsReadyToUnlockPlatinum()
{
    for (const auto &status : m_achievementStatusMap) {
        if (status.first != kPlatinumAchievement && !status.second) {
            return false;
        }
    }
    return true;
}

/* 0xE41A0, 12 bytes, global, 2 named locals
 * CSteamAchievements::OnAchievementStored
 * PDB type: void CSteamAchievements::(UserAc...
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamAchievements.cpp
 */
void CSteamAchievements::OnAchievementStored(
    UserAchievementStored_t *)
{
    WriteToOutputFile("Achievement Stored");
}

/* 0xE41B0, 641 bytes, global, 17 named locals
 * CSteamAchievements::OnUserStatsReceived
 * PDB type: void CSteamAchievements::(UserSt...
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamAchievements.cpp
 */
void CSteamAchievements::OnUserStatsReceived(
    UserStatsReceived_t *pCallback)
{
    WriteToOutputFile("User Stats Received");
    if (m_iAppID != pCallback->m_nGameID) {
        return;
    }

    WriteToOutputFile("App ID Matches Callback Game ID");
    if (pCallback->m_eResult == k_EResultOK) {
        WriteToOutputFile(
            "Received Stats and achievements from steam");
        m_bInitialized = true;

        for (int index = 0; index < m_iNumAchievements; ++index) {
            Achievement_t &achievement = m_pAchievements[index];

            SteamStatsGetAchievement(
                SteamUserStats(),
                achievement.m_rgchName,
                &achievement.m_bAchieved);
            std::snprintf(
                achievement.m_rgchName,
                sizeof(achievement.m_rgchName),
                "%s",
                SteamStatsDisplayAttribute(
                    SteamUserStats(),
                    achievement.m_pchAchievementID,
                    "name"));
            std::snprintf(
                achievement.m_rgchDescription,
                sizeof(achievement.m_rgchDescription),
                "%s",
                SteamStatsDisplayAttribute(
                    SteamUserStats(),
                    achievement.m_pchAchievementID,
                    "desc"));
        }
        WriteToOutputFile("Achievements loaded");
        return;
    }

    const std::string result = std::to_string(pCallback->m_eResult);
    WriteToOutputFile(
        "Failed To Receive Stats and Achievements from steam");
    WriteToOutputFile(result.c_str());

    char buffer[128];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "RequestStats - failed, %d\n",
        static_cast<int>(pCallback->m_eResult));
}

/* 0xE4440, 122 bytes, global, 3 named locals
 * CSteamAchievements::OnUserStatsStored
 * PDB type: void CSteamAchievements::(UserSt...
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamAchievements.cpp
 */
void CSteamAchievements::OnUserStatsStored(
    UserStatsStored_t *pCallback)
{
    WriteToOutputFile("User Stats Stored");
    if (m_iAppID == pCallback->m_nGameID &&
        pCallback->m_eResult != k_EResultOK) {
        char buffer[128];
        std::snprintf(
            buffer,
            sizeof(buffer),
            "StatsStored - failed, %d\n",
            static_cast<int>(pCallback->m_eResult));
    }
}

/* 0xE44C0, 113 bytes, global, 1 named locals
 * CSteamAchievements::RequestStats
 * PDB type: bool CSteamAchievements::()
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamAchievements.cpp
 */
bool CSteamAchievements::RequestStats()
{
    if (SteamUserStats() == nullptr ||
        SteamUser() == nullptr ||
        !SteamUserLoggedOn(SteamUser())) {
        return false;
    }

    WriteToOutputFile("Steam Connection Verified");
    return SteamStatsRequestCurrent(SteamUserStats());
}

/* 0xE4540, 11 bytes, global, 2 named locals
 * CCallback<CSteamAchievements,UserAchievementStored_t,0>::Run
 * PDB type: void CCallback<CSteamAchievement...
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_internal.h
 */
template <>
void CCallback<CSteamAchievements,UserAchievementStored_t,0>::Run(
    void *pvParam)
{
    (m_pObj->*m_Func)(
        static_cast<UserAchievementStored_t *>(pvParam));
}

/* 0xE4550, 11 bytes, global, 2 named locals
 * CCallback<CSteamAchievements,UserStatsReceived_t,0>::Run
 * PDB type: void CCallback<CSteamAchievement...
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_internal.h
 */
template <>
void CCallback<CSteamAchievements,UserStatsReceived_t,0>::Run(
    void *pvParam)
{
    (m_pObj->*m_Func)(static_cast<UserStatsReceived_t *>(pvParam));
}

/* 0xE4560, 11 bytes, global, 2 named locals
 * CCallback<CSteamAchievements,UserStatsStored_t,0>::Run
 * PDB type: void CCallback<CSteamAchievement...
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_internal.h
 */
template <>
void CCallback<CSteamAchievements,UserStatsStored_t,0>::Run(
    void *pvParam)
{
    (m_pObj->*m_Func)(static_cast<UserStatsStored_t *>(pvParam));
}

/* 0xE4570, 7 bytes, global, 4 named locals
 * CCallbackImpl<16>::Run
 * PDB type: void CCallbackImpl<16>::(void*, ...
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
void CCallbackImpl<16>::Run(
    void *pvParam, bool, std::uint64_t)
{
    Run(pvParam);
}

/* 0xE4580, 7 bytes, global, 4 named locals
 * CCallbackImpl<24>::Run
 * PDB type: void CCallbackImpl<24>::(void*, ...
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
void CCallbackImpl<24>::Run(
    void *pvParam, bool, std::uint64_t)
{
    Run(pvParam);
}

/* 0xE4590, 7 bytes, global, 4 named locals
 * CCallbackImpl<152>::Run
 * PDB type: void CCallbackImpl<152>::(void*,...
 * Source: W:\SWJediPowerBattles\work\steam\include\steam_api_common.h
 */
template <>
void CCallbackImpl<152>::Run(
    void *pvParam, bool, std::uint64_t)
{
    Run(pvParam);
}

/* 0xE45A0, 449 bytes, global, 9 named locals
 * CSteamAchievements::SetAchievement
 * PDB type: bool CSteamAchievements::(const ...
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamAchievements.cpp
 */
bool CSteamAchievements::SetAchievement(const char *achievementID)
{
    if (!m_bInitialized) {
        return false;
    }

    SteamStatsSetAchievement(SteamUserStats(), achievementID);
    m_achievementStatusMap[achievementID] = true;

    if (IsReadyToUnlockPlatinum()) {
        SteamStatsSetAchievement(
            SteamUserStats(), kPlatinumAchievement);
    }

    return SteamStatsStore(SteamUserStats());
}

/* 0xE4770, 39 bytes, global, 1 named locals
 * SteamInternal_Init_SteamUser
 * PDB type: void (ISteamUser**)
 * Source: W:\SWJediPowerBattles\work\steam\include\isteamuser.h
 */
void SteamInternal_Init_SteamUser(ISteamUser **user)
{
    *user = static_cast<ISteamUser *>(
        SteamInternal_FindOrCreateUserInterface(
            SteamAPI_GetHSteamUser(), "SteamUser023"));
}

/* 0xE47A0, 39 bytes, global, 1 named locals
 * SteamInternal_Init_SteamUserStats
 * PDB type: void (ISteamUserStats**)
 * Source: W:\SWJediPowerBattles\work\steam\include\isteamuserstats.h
 */
void SteamInternal_Init_SteamUserStats(ISteamUserStats **stats)
{
    *stats = static_cast<ISteamUserStats *>(
        SteamInternal_FindOrCreateUserInterface(
            SteamAPI_GetHSteamUser(),
            "STEAMUSERSTATS_INTERFACE_VERSION012"));
}

/* 0xE47D0, 603 bytes, global, 13 named locals
 * std::_Tree_val<std::_Tree_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,bool> > >::_Insert_node
 * PDB type: std::_Tree_node<std::pair<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xE4A30, 91 bytes, global, 4 named locals
 * _snprintf
 * PDB type: int (char* const, const unsigned...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt\stdio.h
 */

/* 0x2701C0, 12 bytes, local, 1 named locals
 * `std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,bool,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,bool> > >::_Try_emplace<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2701D0, 16 bytes, local, 2 named locals
 * `CSteamAchievements::CSteamAchievements'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2701E0, 16 bytes, local, 2 named locals
 * `CSteamAchievements::CSteamAchievements'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2701F0, 16 bytes, local, 2 named locals
 * `CSteamAchievements::CSteamAchievements'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270200, 16 bytes, local, 2 named locals
 * `CSteamAchievements::CSteamAchievements'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270210, 12 bytes, local, 2 named locals
 * `CSteamAchievements::CSteamAchievements'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270220, 12 bytes, local, 0 named locals
 * `CSteamAchievements::SetAchievement'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */
