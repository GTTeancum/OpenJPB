#include "jpb/generic_hook.h"
#include "jpb/world.h"

#include <codecvt>
#include <cstring>
#include <locale>
#include <string>

/*
 * COMPLETE REVIEWED RECONSTRUCTION
 * PDB module: 0040
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\genericHook.obj
 * Primary source: W:\SWJediPowerBattles\work\genericHook.cpp
 * Compiler language: c++
 * Emitted procedures: 59
 *
 * All five project procedures are checked against their PDB procedure names,
 * RVAs, and shipped bodies below. Toolchain-owned standard-library procedures
 * remain inventory comments and are not project reconstruction claims.
 */

/* Exact PDB globals at matched-PC RVAs 0x4BAC88/8C and 0x537F10. */
int cachedTextureIndexForBus = -1;
int cachedTextureIndexForCoffin = -1;
int foundAllLevelColorOverrides;

/* 0xAB4E0, 911 bytes, global, 15 named locals
 * std::_Codecvt_do_length<std::codecvt<char16_t,char,_Mbstatet>,char,_Mbstatet>
 * PDB type: int (const std::codecvt<char16_t...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAB870, 455 bytes, global, 20 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::_Reallocate_grow_by<<lambda_23e601c987dc463d2d6489aeaa9668c8>,char16_t const *,unsigned __int64>
 * PDB type: std::basic_string<char16_t,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xABA40, 420 bytes, global, 19 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::_Reallocate_grow_by<<lambda_dc4862751a7e573d3d6a0a80489b9a2f>,char16_t>
 * PDB type: std::basic_string<char16_t,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xABBF0, 449 bytes, global, 20 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::_Reallocate_grow_by<<lambda_eb09786fe23d272ea1a0e0f8768901c6>,unsigned __int64,char16_t>
 * PDB type: std::basic_string<char16_t,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xABDC0, 299 bytes, global, 10 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >
 * PDB type: void std::basic_string<char16_t,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xABEF0, 689 bytes, global, 7 named locals
 * std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >
 * PDB type: void std::wstring_convert<std::c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocbuf
 */

/* 0xAC1B0, 60 bytes, global, 2 named locals
 * std::range_error::range_error
 * PDB type: void std::range_error::(const st...
 * Source: no line mapping
 */

/* 0xAC1F0, 71 bytes, global, 3 named locals
 * std::range_error::range_error
 * PDB type: void std::range_error::(const ch...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\stdexcept
 */

/* 0xAC240, 60 bytes, global, 2 named locals
 * std::runtime_error::runtime_error
 * PDB type: void std::runtime_error::(const ...
 * Source: no line mapping
 */

/* 0xAC280, 35 bytes, global, 1 named locals
 * std::_Yarn<char>::~_Yarn<char>
 * PDB type: void std::_Yarn<char>::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocinfo
 */

/* 0xAC2B0, 35 bytes, global, 1 named locals
 * std::_Yarn<wchar_t>::~_Yarn<wchar_t>
 * PDB type: void std::_Yarn<wchar_t>::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocinfo
 */

/* 0xAC2E0, 97 bytes, global, 5 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::~basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >
 * PDB type: void std::basic_string<char16_t,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xAC350, 11 bytes, global, 1 named locals
 * std::codecvt<char16_t,char,_Mbstatet>::~codecvt<char16_t,char,_Mbstatet>
 * PDB type: void std::codecvt<char16_t,char,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC360, 154 bytes, global, 5 named locals
 * std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::~wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >
 * PDB type: void std::wstring_convert<std::c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocbuf
 */

/* 0xAC400, 11 bytes, global, 1 named locals
 * std::_Facet_base::~_Facet_base
 * PDB type: void std::_Facet_base::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xfacet
 */

/* 0xAC410, 11 bytes, global, 1 named locals
 * std::codecvt_base::~codecvt_base
 * PDB type: void std::codecvt_base::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC420, 11 bytes, global, 1 named locals
 * std::locale::facet::~facet
 * PDB type: void std::locale::facet::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC430, 47 bytes, global, 1 named locals
 * std::locale::~locale
 * PDB type: void std::locale::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC460, 19 bytes, global, 1 named locals
 * std::range_error::~range_error
 * PDB type: void std::range_error::()
 * Source: no line mapping
 */

/* 0xAC480, 43 bytes, global, 1 named locals
 * std::codecvt<char16_t,char,_Mbstatet>::`scalar deleting destructor'
 * PDB type: void* std::codecvt<char16_t,char...
 * Source: no line mapping
 */

/* 0xAC4B0, 52 bytes, global, 1 named locals
 * std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::`scalar deleting destructor'
 * PDB type: void* std::wstring_convert<std::...
 * Source: no line mapping
 */

/* 0xAC4F0, 43 bytes, global, 1 named locals
 * std::_Facet_base::`scalar deleting destructor'
 * PDB type: void* std::_Facet_base::(unsigne...
 * Source: no line mapping
 */

/* 0xAC520, 43 bytes, global, 1 named locals
 * std::codecvt_base::`scalar deleting destructor'
 * PDB type: void* std::codecvt_base::(unsign...
 * Source: no line mapping
 */

/* 0xAC550, 43 bytes, global, 1 named locals
 * std::locale::facet::`scalar deleting destructor'
 * PDB type: void* std::locale::facet::(unsig...
 * Source: no line mapping
 */

/* 0xAC580, 66 bytes, global, 1 named locals
 * std::range_error::`scalar deleting destructor'
 * PDB type: void* std::range_error::(unsigne...
 * Source: no line mapping
 */

/* 0xAC5D0, 66 bytes, global, 1 named locals
 * std::runtime_error::`scalar deleting destructor'
 * PDB type: void* std::runtime_error::(unsig...
 * Source: no line mapping
 */

/* 0xAC620, 15 bytes, global, 1 named locals
 * std::locale::facet::_Decref
 * PDB type: std::_Facet_base* std::locale::f...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC630, 5 bytes, global, 1 named locals
 * std::locale::facet::_Incref
 * PDB type: void std::locale::facet::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC640, 35 bytes, global, 1 named locals
 * std::_Throw_range_error
 * PDB type: void (const char* const)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\stdexcept
 */

/* 0xAC670, 3 bytes, global, 1 named locals
 * std::codecvt<char16_t,char,_Mbstatet>::do_always_noconv
 * PDB type: bool std::codecvt<char16_t,char,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC680, 3 bytes, global, 1 named locals
 * std::codecvt_base::do_always_noconv
 * PDB type: bool std::codecvt_base::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC690, 3 bytes, global, 1 named locals
 * std::codecvt<char16_t,char,_Mbstatet>::do_encoding
 * PDB type: int std::codecvt<char16_t,char,_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC6A0, 6 bytes, global, 1 named locals
 * std::codecvt_base::do_encoding
 * PDB type: int std::codecvt_base::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC6B0, 700 bytes, global, 12 named locals
 * std::codecvt<char16_t,char,_Mbstatet>::do_in
 * PDB type: int std::codecvt<char16_t,char,_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC970, 5 bytes, global, 5 named locals
 * std::codecvt<char16_t,char,_Mbstatet>::do_length
 * PDB type: int std::codecvt<char16_t,char,_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC980, 22 bytes, global, 1 named locals
 * std::codecvt<char16_t,char,_Mbstatet>::do_max_length
 * PDB type: int std::codecvt<char16_t,char,_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC9A0, 6 bytes, global, 1 named locals
 * std::codecvt_base::do_max_length
 * PDB type: int std::codecvt_base::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xAC9B0, 507 bytes, global, 14 named locals
 * std::codecvt<char16_t,char,_Mbstatet>::do_out
 * PDB type: int std::codecvt<char16_t,char,_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xACBB0, 22 bytes, global, 5 named locals
 * std::codecvt<char16_t,char,_Mbstatet>::do_unshift
 * PDB type: int std::codecvt<char16_t,char,_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0xACBD0, 1093 bytes, global, 31 named locals
 * std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::from_bytes
 * PDB type: std::basic_string<char16_t,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocbuf
 */

/* 0xAD020, 31 bytes, global, 0 named locals
 * ClearCachedTextureIndices
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\genericHook.cpp
 */
void ClearCachedTextureIndices(void)
{
    cachedTextureIndexForCoffin = -1;
    cachedTextureIndexForBus = -1;
    foundAllLevelColorOverrides = 0;
}

/* 0xAD040, 145 bytes, global, 5 named locals
 * IsBusTextureForCorus2
 * PDB type: int (int, const char*, int)
 * Source: W:\SWJediPowerBattles\work\genericHook.cpp
 */
int IsBusTextureForCorus2(
    int level, const char *filename, int texture_index)
{
    const char *base_name;

    if (level != 6 && level != 15) {
        return 0;
    }
    if (cachedTextureIndexForBus == -1 && filename != nullptr &&
        (base_name = std::strrchr(filename, '/')) != nullptr &&
        std::strcmp(base_name + 1, "bus.tga") == 0) {
        cachedTextureIndexForBus = texture_index;
        return 1;
    }
    return cachedTextureIndexForBus == texture_index;
}

/* 0xAD0E0, 140 bytes, global, 6 named locals
 * IsCoffinTextureForPalace
 * PDB type: int (int, const char*, int)
 * Source: W:\SWJediPowerBattles\work\genericHook.cpp
 */
int IsCoffinTextureForPalace(
    int level, const char *filename, int texture_index)
{
    const char *base_name;

    if (level != 4 || gpWorld == nullptr ||
        (gpWorld->aDolly[gpWorld->currentDolly].flags &
         UINT32_C(0x400)) == 0) {
        return 0;
    }
    if (cachedTextureIndexForCoffin == -1 &&
        filename != nullptr &&
        (base_name = std::strrchr(filename, '/')) != nullptr &&
        std::strcmp(base_name + 1, "coffin256.tga") == 0) {
        cachedTextureIndexForCoffin = texture_index;
        return 1;
    }
    return cachedTextureIndexForCoffin == texture_index;
}

/* 0xAD170, 388 bytes, global, 3 named locals
 * SetTextureColorOverride
 * PDB type: void (int, _Material*)
 * Source: W:\SWJediPowerBattles\work\genericHook.cpp
 */
void SetTextureColorOverride(int level, _Material *material)
{
    const char *base_name;

    if (material == nullptr || level == 0) {
        return;
    }
    base_name = std::strrchr(material->filename, '/');
    if (base_name == nullptr) {
        return;
    }
    ++base_name;
    if (std::strcmp(base_name, "Loadbody.tga") == 0) {
        material->colorOverride = 0x80;
        return;
    }
    if (level == 5 && !foundAllLevelColorOverrides &&
        std::strcmp(base_name, "boulder.tga") == 0) {
        material->colorOverride = -1000;
        foundAllLevelColorOverrides = 1;
        return;
    }
    if (level == 2 && !foundAllLevelColorOverrides &&
        std::strcmp(base_name, "ful_body.tga") == 0) {
        material->colorOverride = 0xc0;
        foundAllLevelColorOverrides = 1;
        return;
    }
    if ((level == 6 || level == 15) &&
        std::strcmp(base_name, "bus.tga") == 0) {
        material->flags = JPB_MATERIAL_MODE_TWO_SIDED;
        material->colorOverride = 100;
        foundAllLevelColorOverrides = 1;
        return;
    }
    if (std::strcmp(base_name, "qui_hair.tga") == 0) {
        material->colorOverride = 0xc0;
    }
}

/* 0xAD300, 416 bytes, global, 15 named locals
 * _DrawUIText
 * PDB type: void (const char*, SCREENRECT, i...
 * Source: W:\SWJediPowerBattles\work\genericHook.cpp
 */
void _DrawUIText(
    const char *text,
    SCREENRECT destination,
    int font_style,
    int point_size,
    CVECTOR color)
{
    std::wstring_convert<
        std::codecvt<char16_t, char, std::mbstate_t>,
        char16_t> converter;
    const std::u16string converted = converter.from_bytes(
        text, text + std::strlen(text));
    _DrawUITextUTF16(
        reinterpret_cast<uint16_t *>(
            const_cast<char16_t *>(converted.c_str())),
        destination,
        font_style,
        point_size,
        color);
}

/* 0x270090, 16 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2700A0, 16 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2700B0, 16 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2700C0, 32 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2700E0, 12 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$11
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2700F0, 12 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$15
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270100, 16 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$16
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270110, 16 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$17
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270120, 16 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$18
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270130, 16 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$19
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270140, 16 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$20
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270150, 16 bytes, local, 1 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >'::`1'::dtor$21
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270160, 12 bytes, local, 3 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::from_bytes'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270170, 12 bytes, local, 3 named locals
 * `std::wstring_convert<std::codecvt<char16_t,char,_Mbstatet>,char16_t,std::allocator<char16_t>,std::allocator<char> >::from_bytes'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */
