/*
 * REVIEWED RECONSTRUCTION OF ALL PROJECT PROCEDURES.
 * PDB module: 0063
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\CSteamRichPresence.obj
 * Primary source: W:\SWJediPowerBattles\work\platform\steam\CSteamRichPresence.cpp
 * Compiler language: c++
 * Emitted procedures: 74
 *
 * Project behavior and layouts come from matched-PC PDB types and direct
 * shipped-executable disassembly. Toolchain bodies remain compiler-owned.
 */

#include "jpb/steam_rich_presence.h"

#include <cstdint>
#include <iostream>

namespace {

bool SetSteamRichPresence(
    ISteamFriends *friends, const char *key, const char *value)
{
    typedef bool (*SetRichPresenceFn)(
        ISteamFriends *, const char *, const char *);
    void **vtable = *reinterpret_cast<void ***>(friends);

    return reinterpret_cast<SetRichPresenceFn>(vtable[43])(
        friends, key, value);
}

void ClearSteamRichPresence(ISteamFriends *friends)
{
    typedef void (*ClearRichPresenceFn)(ISteamFriends *);
    void **vtable = *reinterpret_cast<void ***>(friends);

    reinterpret_cast<ClearRichPresenceFn>(vtable[44])(friends);
}

} // namespace

/* Exact matched-PC PDB global at RVA 0x582630. */
CSteamRichPresence *g_SteamRicherPresence;

/* 0xE4BE0, 681 bytes, global, 13 named locals
 * std::operator<<<std::char_traits<char> >
 * PDB type: std::basic_ostream<char,std::cha...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0xE4E90, 8 bytes, global, 0 named locals
 * std::_Immortalize_memcpy_image<std::_Iostream_error_category2>
 * PDB type: const std::_Iostream_error_categ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0xE4EA0, 394 bytes, global, 20 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Reallocate_grow_by<<lambda_65e615be2a453ca0576c979606f46740>,char const *,unsigned __int64>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xE5030, 149 bytes, global, 2 named locals
 * std::endl<char,std::char_traits<char> >
 * PDB type: std::basic_ostream<char,std::cha...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0xE50D0, 270 bytes, global, 10 named locals
 * std::use_facet<std::ctype<char> >
 * PDB type: const std::ctype<char>& (const s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE51E0, 112 bytes, global, 1 named locals
 * CSteamRichPresence::CSteamRichPresence
 * PDB type: void CSteamRichPresence::()
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamRichPresence.cpp
 */
CSteamRichPresence::CSteamRichPresence()
    : m_initialized(0), m_inMenu(-1), m_currentLevel(-1)
{
    if (SteamUser() != nullptr && SteamFriends() != nullptr) {
        m_initialized = 1;
        return;
    }

    std::cerr
        << "Failed to initialize SteamAPI or retrieve SteamUser/SteamFriends."
        << std::endl;
}

/* 0xE5250, 742 bytes, global, 23 named locals
 * std::_System_error::_System_error
 * PDB type: void std::_System_error::(std::e...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0xE5540, 77 bytes, global, 2 named locals
 * std::_System_error::_System_error
 * PDB type: void std::_System_error::(const ...
 * Source: no line mapping
 */

/* 0xE5590, 60 bytes, global, 2 named locals
 * std::bad_cast::bad_cast
 * PDB type: void std::bad_cast::(const std::...
 * Source: no line mapping
 */

/* 0xE55D0, 33 bytes, global, 1 named locals
 * std::bad_cast::bad_cast
 * PDB type: void std::bad_cast::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vcruntime_typeinfo.h
 */

/* 0xE5600, 87 bytes, global, 2 named locals
 * std::ios_base::failure::failure
 * PDB type: void std::ios_base::failure::(co...
 * Source: no line mapping
 */

/* 0xE5660, 210 bytes, global, 6 named locals
 * std::ios_base::failure::failure
 * PDB type: void std::ios_base::failure::(co...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xiosbase
 */

/* 0xE5740, 122 bytes, global, 4 named locals
 * std::basic_ostream<char,std::char_traits<char> >::sentry::sentry
 * PDB type: void std::basic_ostream<char,std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0xE57C0, 87 bytes, global, 2 named locals
 * std::system_error::system_error
 * PDB type: void std::system_error::(const s...
 * Source: no line mapping
 */

/* 0xE5820, 73 bytes, global, 1 named locals
 * std::ctype<char>::~ctype<char>
 * PDB type: void std::ctype<char>::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE5870, 20 bytes, global, 1 named locals
 * std::unique_ptr<std::_Facet_base,std::default_delete<std::_Facet_base> >::~unique_ptr<std::_Facet_base,std::default_delete<std::_Facet_base> >
 * PDB type: void std::unique_ptr<std::_Facet...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\memory
 */

/* 0xE5890, 35 bytes, global, 1 named locals
 * CSteamRichPresence::~CSteamRichPresence
 * PDB type: void CSteamRichPresence::()
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamRichPresence.cpp
 */
CSteamRichPresence::~CSteamRichPresence()
{
    ClearSteamRichPresence(SteamFriends());
}

/* 0xE58C0, 36 bytes, global, 2 named locals
 * std::basic_ostream<char,std::char_traits<char> >::_Sentry_base::~_Sentry_base
 * PDB type: void std::basic_ostream<char,std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0xE58F0, 19 bytes, global, 1 named locals
 * std::bad_cast::~bad_cast
 * PDB type: void std::bad_cast::()
 * Source: no line mapping
 */

/* 0xE5910, 11 bytes, global, 1 named locals
 * std::ctype_base::~ctype_base
 * PDB type: void std::ctype_base::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE5920, 19 bytes, global, 1 named locals
 * std::ios_base::failure::~failure
 * PDB type: void std::ios_base::failure::()
 * Source: no line mapping
 */

/* 0xE5940, 60 bytes, global, 2 named locals
 * std::basic_ostream<char,std::char_traits<char> >::sentry::~sentry
 * PDB type: void std::basic_ostream<char,std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0xE5980, 19 bytes, global, 1 named locals
 * std::system_error::~system_error
 * PDB type: void std::system_error::()
 * Source: no line mapping
 */

/* 0xE59A0, 106 bytes, global, 1 named locals
 * std::ctype<char>::`scalar deleting destructor'
 * PDB type: void* std::ctype<char>::(unsigne...
 * Source: no line mapping
 */

/* 0xE5A10, 33 bytes, global, 1 named locals
 * std::_Iostream_error_category2::`scalar deleting destructor'
 * PDB type: void* std::_Iostream_error_categ...
 * Source: no line mapping
 */

/* 0xE5A40, 66 bytes, global, 1 named locals
 * std::_System_error::`scalar deleting destructor'
 * PDB type: void* std::_System_error::(unsig...
 * Source: no line mapping
 */

/* 0xE5A90, 66 bytes, global, 1 named locals
 * std::bad_cast::`scalar deleting destructor'
 * PDB type: void* std::bad_cast::(unsigned)
 * Source: no line mapping
 */

/* 0xE5AE0, 43 bytes, global, 1 named locals
 * std::ctype_base::`scalar deleting destructor'
 * PDB type: void* std::ctype_base::(unsigned...
 * Source: no line mapping
 */

/* 0xE5B10, 66 bytes, global, 1 named locals
 * std::ios_base::failure::`scalar deleting destructor'
 * PDB type: void* std::ios_base::failure::(u...
 * Source: no line mapping
 */

/* 0xE5B60, 66 bytes, global, 1 named locals
 * std::system_error::`scalar deleting destructor'
 * PDB type: void* std::system_error::(unsign...
 * Source: no line mapping
 */

/* 0xE5BB0, 34 bytes, global, 1 named locals
 * CSteamRichPresence::ClearRichPresence
 * PDB type: void CSteamRichPresence::()
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamRichPresence.cpp
 */
void CSteamRichPresence::ClearRichPresence()
{
    ClearSteamRichPresence(SteamFriends());
}

/* 0xE5BE0, 564 bytes, global, 4 named locals
 * CSteamRichPresence::SetRichPresence
 * PDB type: void CSteamRichPresence::(int, i...
 * Source: W:\SWJediPowerBattles\work\platform\steam\CSteamRichPresence.cpp
 */
void CSteamRichPresence::SetRichPresence(
    int inMenu, int currentLevel)
{
    const char *levelString;

    if (m_initialized == 0) {
        std::cerr
            << "SteamAPI is not initialized or SteamUser/SteamFriends is not available."
            << std::endl;
        return;
    }
    if (m_inMenu == inMenu && m_currentLevel == currentLevel) {
        return;
    }

    if (inMenu == 0) {
        switch (currentLevel) {
        case 1: levelString = "#Playing_Level_FED"; break;
        case 2: levelString = "#Playing_Level_MARSH"; break;
        case 3: levelString = "#Playing_Level_THEED"; break;
        case 4: levelString = "#Playing_Level_PALACE"; break;
        case 5: levelString = "#Playing_Level_TATOOINE"; break;
        case 6:
        case 15: levelString = "#Playing_Level_CORUS"; break;
        case 7: levelString = "#Playing_Level_RUINS"; break;
        case 8: levelString = "#Playing_Level_STREETS"; break;
        case 9: levelString = "#Playing_Level_HANGAR"; break;
        case 10: levelString = "#Playing_Level_CORE"; break;
        case 11: levelString = "#Playing_Level_MINI1"; break;
        case 12: levelString = "#Playing_Level_MINI2"; break;
        case 13: levelString = "#Playing_Level_MINI3"; break;
        case 14: levelString = "#Playing_Level_MINI4"; break;
        case 16: levelString = "#Playing_Level_TRAINING1"; break;
        case 17: levelString = "#Playing_Level_TRAINING2"; break;
        case 18: levelString = "#Playing_Level_TRAINING3"; break;
        case 19: levelString = "#Playing_Level_TRAINING4"; break;
        case 20: levelString = "#Playing_Level_TRAINING5"; break;
        case 21: levelString = "#Playing_Level_TRAINING6"; break;
        case 22: levelString = "#Playing_Level_TRAINING7"; break;
        case 25: levelString = "#Playing_Level_ARENA"; break;
        default: levelString = nullptr; break;
        }
        SetSteamRichPresence(
            SteamFriends(), "steam_display", levelString);
    } else {
        SetSteamRichPresence(
            SteamFriends(), "steam_display", "#Menu");
    }

    m_inMenu = inMenu;
    m_currentLevel = currentLevel;
}

/* 0xE5E20, 39 bytes, global, 1 named locals
 * SteamInternal_Init_SteamFriends
 * PDB type: void (ISteamFriends**)
 * Source: W:\SWJediPowerBattles\work\steam\include\isteamfriends.h
 */
void SteamInternal_Init_SteamFriends(ISteamFriends **p)
{
    *p = static_cast<ISteamFriends *>(
        SteamInternal_FindOrCreateUserInterface(
            SteamAPI_GetHSteamUser(), "SteamFriends017"));
}

/* 0xE5E50, 437 bytes, global, 2 named locals
 * std::ctype<char>::_Getcat
 * PDB type: unsigned __int64 std::ctype<char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE6010, 162 bytes, global, 5 named locals
 * std::basic_ostream<char,std::char_traits<char> >::_Osfx
 * PDB type: void std::basic_ostream<char,std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0xE60C0, 32 bytes, global, 0 named locals
 * std::_Throw_bad_cast
 * PDB type: void ()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\typeinfo
 */

/* 0xE60E0, 131 bytes, global, 6 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::append
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xE6170, 4 bytes, global, 3 named locals
 * std::ctype<char>::do_narrow
 * PDB type: char std::ctype<char>::(char, ch...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE6180, 31 bytes, global, 5 named locals
 * std::ctype<char>::do_narrow
 * PDB type: const char* std::ctype<char>::(c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE61A0, 14 bytes, global, 2 named locals
 * std::ctype<char>::do_tolower
 * PDB type: char std::ctype<char>::(char)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE61B0, 72 bytes, global, 3 named locals
 * std::ctype<char>::do_tolower
 * PDB type: const char* std::ctype<char>::(c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE6200, 14 bytes, global, 2 named locals
 * std::ctype<char>::do_toupper
 * PDB type: char std::ctype<char>::(char)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE6210, 72 bytes, global, 3 named locals
 * std::ctype<char>::do_toupper
 * PDB type: const char* std::ctype<char>::(c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE6260, 4 bytes, global, 2 named locals
 * std::ctype<char>::do_widen
 * PDB type: char std::ctype<char>::(char)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE6270, 29 bytes, global, 4 named locals
 * std::ctype<char>::do_widen
 * PDB type: const char* std::ctype<char>::(c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xE6290, 301 bytes, global, 9 named locals
 * std::basic_ostream<char,std::char_traits<char> >::flush
 * PDB type: std::basic_ostream<char,std::cha...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0xE63C0, 17 bytes, global, 1 named locals
 * std::make_error_code
 * PDB type: std::error_code (std::io_errc)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0xE63E0, 178 bytes, global, 5 named locals
 * std::_Iostream_error_category2::message
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0xE64A0, 8 bytes, global, 1 named locals
 * std::_Iostream_error_category2::name
 * PDB type: const char* std::_Iostream_error...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0xE64B0, 418 bytes, global, 11 named locals
 * std::basic_ostream<char,std::char_traits<char> >::put
 * PDB type: std::basic_ostream<char,std::cha...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0x270230, 12 bytes, local, 2 named locals
 * `std::operator<<<std::char_traits<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270240, 12 bytes, local, 2 named locals
 * `std::operator<<<std::char_traits<char> >'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270250, 92 bytes, local, 4 named locals
 * `std::operator<<<std::char_traits<char> >'::`1'::catch$4
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0x2702B0, 12 bytes, local, 0 named locals
 * `std::endl<char,std::char_traits<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2702C0, 12 bytes, local, 0 named locals
 * `std::use_facet<std::ctype<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2702D0, 12 bytes, local, 0 named locals
 * `std::use_facet<std::ctype<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2702E0, 12 bytes, local, 1 named locals
 * `std::_System_error::_System_error'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2702F0, 12 bytes, local, 1 named locals
 * `std::_System_error::_System_error'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270300, 12 bytes, local, 1 named locals
 * `std::ios_base::failure::failure'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270310, 12 bytes, local, 1 named locals
 * `std::basic_ostream<char,std::char_traits<char> >::sentry::sentry'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270320, 32 bytes, local, 0 named locals
 * `std::ctype<char>::_Getcat'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270340, 12 bytes, local, 0 named locals
 * `std::ctype<char>::_Getcat'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270350, 16 bytes, local, 0 named locals
 * `std::ctype<char>::_Getcat'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270360, 16 bytes, local, 0 named locals
 * `std::ctype<char>::_Getcat'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270370, 16 bytes, local, 0 named locals
 * `std::ctype<char>::_Getcat'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270380, 16 bytes, local, 0 named locals
 * `std::ctype<char>::_Getcat'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270390, 16 bytes, local, 0 named locals
 * `std::ctype<char>::_Getcat'::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2703A0, 16 bytes, local, 0 named locals
 * `std::ctype<char>::_Getcat'::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2703B0, 30 bytes, local, 0 named locals
 * `std::basic_ostream<char,std::char_traits<char> >::_Osfx'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0x2703D0, 12 bytes, local, 2 named locals
 * `std::basic_ostream<char,std::char_traits<char> >::flush'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2703E0, 92 bytes, local, 4 named locals
 * `std::basic_ostream<char,std::char_traits<char> >::flush'::`1'::catch$9
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */

/* 0x270440, 12 bytes, local, 2 named locals
 * `std::basic_ostream<char,std::char_traits<char> >::put'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270450, 12 bytes, local, 2 named locals
 * `std::basic_ostream<char,std::char_traits<char> >::put'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270460, 92 bytes, local, 4 named locals
 * `std::basic_ostream<char,std::char_traits<char> >::put'::`1'::catch$4
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\ostream
 */
