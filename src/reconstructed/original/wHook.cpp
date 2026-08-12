/*
 * PARTIALLY REVIEWED RECONSTRUCTION.
 *
 * The exact platform achievement entry-point names and signatures are
 * retained here, but their Windows/Steam implementation is intentionally a
 * portable service boundary. These wrappers are substituted integration
 * code, not reconstructions of the original platform bodies.
 *
 * Provenance:
 *   direct      - platform_completeAchievement and
 *                 platform_getCompleteAchievement names/signatures/RVAs,
 *                 plus debug_drawsphere's name/signature/RVA and authored
 *                 arguments, from the exact matching PDB.
 *   decompiled  - debug_drawsphere's optimized release body was checked
 *                 against Ghidra and raw instructions; it emits no primitive.
 *   substituted - jpb_PlatformSetAchievementHooks and wrapper bodies provide
 *                 dependency-light platform integration; the optional debug
 *                 sphere publication hook realizes the otherwise inert call.
 *                 The screen-polygon hook retains the exact _StartPoly,
 *                 _SetVert, and _EndPoly payload while replacing only the
 *                 retail renderer-builder singleton.
 *
 * PDB module: 0095
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\wHook.obj
 * Primary source: W:\SWJediPowerBattles\Work\wHook.cpp
 * Compiler language: c++
 * Emitted procedures: 499
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/platform.h"
#include "jpb/debugtext.h"
#include "jpb/generic_hook.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/loader.h"
#include "jpb/resources.h"
#include "jpb/texture.h"
#include "jpb/whook.h"

#include <cctype>
#include <cstring>
#include <string>

static JPBPlatformAchievementHooks jpb_platform_achievement_hooks;
static void *jpb_platform_achievement_user_data;
static JPBDrawTextureHook jpb_draw_texture_hook;
static void *jpb_draw_texture_user_data;
static JPBDebugSphereHook jpb_debug_sphere_hook;
static void *jpb_debug_sphere_user_data;
static JPBScreenPolyHook jpb_screen_poly_hook;
static void *jpb_screen_poly_user_data;
static JPBLevelTransformation jpb_level_transformation;

/* Exact PDB global at matched-PC RVA 0x537DEC. */
int refreshFontAtlasFlag;
/* Exact PDB global at matched-PC RVA 0x539D78. */
_Material *whitemat;

static struct {
    _Material *material;
    int requestedVertexCount;
    JPBScreenPolyVertex vertices[JPB_SCREEN_POLY_VERTEX_CAPACITY];
} jpb_screen_poly_builder;

void jpb_PlatformSetAchievementHooks(
    const JPBPlatformAchievementHooks *hooks,
    void *user_data)
{
    if (hooks == nullptr) {
        std::memset(
            &jpb_platform_achievement_hooks,
            0,
            sizeof(jpb_platform_achievement_hooks));
        jpb_platform_achievement_user_data = nullptr;
        return;
    }
    jpb_platform_achievement_hooks = *hooks;
    jpb_platform_achievement_user_data = user_data;
}

void jpb_WHookSetDrawTextureHook(
    JPBDrawTextureHook hook, void *user_data)
{
    jpb_draw_texture_hook = hook;
    jpb_draw_texture_user_data = user_data;
}

void jpb_WHookSetDebugSphereHook(
    JPBDebugSphereHook hook, void *user_data)
{
    jpb_debug_sphere_hook = hook;
    jpb_debug_sphere_user_data = user_data;
}

void jpb_WHookSetScreenPolyHook(
    JPBScreenPolyHook hook, void *user_data)
{
    jpb_screen_poly_hook = hook;
    jpb_screen_poly_user_data = user_data;
}

const JPBLevelTransformation *jpb_WHookLevelTransformation(void)
{
    return &jpb_level_transformation;
}

/* 0x37C0, 32 bytes, local, 0 named locals
 * `dynamic initializer for 'chavo''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x37E0, 21 bytes, local, 0 named locals
 * `dynamic initializer for 'framelast''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x3800, 21 bytes, local, 0 named locals
 * `dynamic initializer for 'framenow''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x10ADB0, 332 bytes, global, 15 named locals
 * std::vector<unsigned int,std::allocator<unsigned int> >::_Assign_counted_range<unsigned int *>
 * PDB type: void std::vector<unsigned int,st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10AF00, 334 bytes, global, 14 named locals
 * std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Assign_counted_range<std::_Tgt_state_t<char const *>::_Grp_t *>
 * PDB type: void std::vector<std::_Tgt_state...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10B050, 143 bytes, global, 6 named locals
 * std::_Cmp_chrange<char const *,char const *,std::_Cmp_collate<std::regex_traits<char> > >
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10B0E0, 47 bytes, global, 6 named locals
 * std::_Cmp_chrange<char const *,char const *,std::_Cmp_cs<std::regex_traits<char> > >
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10B110, 147 bytes, global, 6 named locals
 * std::_Cmp_chrange<char const *,char const *,std::_Cmp_icase<std::regex_traits<char> > >
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10B1B0, 294 bytes, global, 13 named locals
 * std::_Compare<char const *,char const *,std::regex_traits<char> >
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10B2E0, 303 bytes, global, 10 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::_Construct<1,char16_t const *>
 * PDB type: void std::basic_string<char16_t,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10B410, 250 bytes, global, 3 named locals
 * std::filesystem::_Convert_Source_to_wide<char [256],std::filesystem::_Normal_conversion>
 * PDB type: std::basic_string<wchar_t,std::c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x10B510, 240 bytes, global, 3 named locals
 * std::filesystem::_Convert_stringoid_to_wide<std::filesystem::_Normal_conversion>
 * PDB type: std::basic_string<wchar_t,std::c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x10B600, 227 bytes, global, 4 named locals
 * std::filesystem::_Convert_wide_to_narrow_replace_chars<std::char_traits<char>,std::allocator<char> >
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x10B6F0, 23 bytes, global, 4 named locals
 * std::_Copy_backward_memmove<RESOLUTION *,RESOLUTION *>
 * PDB type: RESOLUTION* (RESOLUTION*, RESOLU...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B710, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<char *,char *>
 * PDB type: char* (char*, char*, char*)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B740, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<unsigned short *,unsigned short *>
 * PDB type: unsigned short* (unsigned short*...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B770, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<CD3DApplication::FBX_MESH * *,CD3DApplication::FBX_MESH * *>
 * PDB type: CD3DApplication::FBX_MESH** (CD3...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B7A0, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<std::_Tgt_state_t<char const *>::_Grp_t *,std::_Tgt_state_t<char const *>::_Grp_t *>
 * PDB type: std::_Tgt_state_t<char const *>:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B7D0, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<std::_Loop_vals_t *,std::_Loop_vals_t *>
 * PDB type: std::_Loop_vals_t* (std::_Loop_v...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B800, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<std::sub_match<char const *> *,std::sub_match<char const *> *>
 * PDB type: std::sub_match<char const *>* (s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B830, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<char const *,char *>
 * PDB type: char* (const char*, const char*,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B860, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<unsigned int const *,unsigned int *>
 * PDB type: unsigned* (const unsigned*, cons...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B890, 50 bytes, global, 5 named locals
 * std::_Copy_memmove_n<unsigned int *,unsigned int *>
 * PDB type: unsigned* (unsigned*, const unsi...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B8D0, 49 bytes, global, 5 named locals
 * std::_Copy_memmove_n<std::_Tgt_state_t<char const *>::_Grp_t *,std::_Tgt_state_t<char const *>::_Grp_t *>
 * PDB type: std::_Tgt_state_t<char const *>:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B910, 1074 bytes, global, 44 named locals
 * std::_Copy_vbool<std::_Vb_iterator<std::_Wrap_alloc<std::allocator<unsigned int> > >,std::_Vb_iterator<std::_Wrap_alloc<std::allocator<unsigned int> > > >
 * PDB type: std::_Vb_iterator<std::_Wrap_all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10BD50, 49 bytes, global, 3 named locals
 * std::_Destroy_range<std::allocator<CD3DApplication::SubMeshSet> >
 * PDB type: void (CD3DApplication::SubMeshSe...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x10BD90, 415 bytes, global, 23 named locals
 * std::vector<CD3DApplication::FBX_MESH *,std::allocator<CD3DApplication::FBX_MESH *> >::_Emplace_reallocate<CD3DApplication::FBX_MESH * const &>
 * PDB type: CD3DApplication::FBX_MESH** std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10BF30, 592 bytes, global, 19 named locals
 * std::vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >::_Emplace_reallocate<CD3DApplication::SubMeshSet const &>
 * PDB type: CD3DApplication::SubMeshSet* std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10C180, 540 bytes, global, 21 named locals
 * std::vector<Vertex,std::allocator<Vertex> >::_Emplace_reallocate<Vertex const &>
 * PDB type: Vertex* std::vector<Vertex,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10C3A0, 411 bytes, global, 24 named locals
 * std::vector<unsigned short,std::allocator<unsigned short> >::_Emplace_reallocate<unsigned short>
 * PDB type: unsigned short* std::vector<unsi...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10C540, 85 bytes, global, 4 named locals
 * std::_Tree_val<std::_Tree_simple_types<std::pair<unsigned short const ,unsigned int> > >::_Erase_tree<std::allocator<std::_Tree_node<std::pair<unsigned short const ,unsigned int>,void *> > >
 * PDB type: void std::_Tree_val<std::_Tree_s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0x10C5A0, 15 bytes, global, 2 named locals
 * std::_Fill_zero_memset<unsigned int *>
 * PDB type: void (unsigned*, const unsigned ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10C5B0, 204 bytes, global, 10 named locals
 * std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Find_last<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >
 * PDB type: std::_Hash_find_last_result<std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x10C680, 195 bytes, global, 6 named locals
 * std::_Tree<std::_Tmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Find_lower_bound<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >
 * PDB type: std::_Tree_find_result<std::_Tre...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0x10C750, 1223 bytes, global, 37 named locals
 * std::_Format_default<char const *,std::allocator<std::sub_match<char const *> >,char const *,std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
 * PDB type: std::back_insert_iterator<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10CC20, 550 bytes, global, 14 named locals
 * std::_Format_sed<char const *,std::allocator<std::sub_match<char const *> >,char const *,std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
 * PDB type: std::back_insert_iterator<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10CE50, 108 bytes, global, 6 named locals
 * std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *>::_Freenode<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_List_node<std::pair<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\list
 */

/* 0x10CEC0, 8 bytes, global, 0 named locals
 * std::_Immortalize_memcpy_image<std::_Generic_error_category>
 * PDB type: const std::_Generic_error_catego...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x10CED0, 8 bytes, global, 0 named locals
 * std::_Immortalize_memcpy_image<std::_System_error_category>
 * PDB type: const std::_System_error_categor...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x10CEE0, 7 bytes, global, 1 named locals
 * std::_Is_all_bits_zero<unsigned int>
 * PDB type: bool (const unsigned&)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10CEF0, 137 bytes, global, 6 named locals
 * std::_Lookup_coll<char const *,char>
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10CF80, 1495 bytes, global, 49 named locals
 * std::_Lookup_equiv<char,std::regex_traits<char> >
 * PDB type: bool (unsigned char, const std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10D560, 55 bytes, global, 3 named locals
 * std::_Lookup_range<char>
 * PDB type: bool (unsigned, const std::_Buf<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10D5A0, 549 bytes, global, 10 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Match<std::allocator<std::sub_match<char const *> > >
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10D7D0, 125 bytes, global, 7 named locals
 * std::_Med3_unchecked<RESOLUTION *,bool (__cdecl*)(RESOLUTION const &,RESOLUTION const &)>
 * PDB type: void (RESOLUTION*, RESOLUTION*, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x10D850, 572 bytes, global, 17 named locals
 * std::_Partition_by_median_guess_unchecked<RESOLUTION *,bool (__cdecl*)(RESOLUTION const &,RESOLUTION const &)>
 * PDB type: std::pair<RESOLUTION *,RESOLUTIO...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x10DA90, 255 bytes, global, 10 named locals
 * std::_Pop_heap_hole_by_index<RESOLUTION *,RESOLUTION,bool (__cdecl*)(RESOLUTION const &,RESOLUTION const &)>
 * PDB type: void (RESOLUTION*, __int64, __in...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x10DB90, 365 bytes, global, 18 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Reallocate_grow_by<<lambda_319d5e083f45f90dcdce5dce53cbb275>,char>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10DD00, 331 bytes, global, 18 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Reallocate_grow_by<<lambda_9013ee9e23efe4882b67eff5b0ecf103> >
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10DE50, 449 bytes, global, 20 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::_Reallocate_grow_by<<lambda_a3050a43f3157934f354774ab3dd2e02>,unsigned __int64,wchar_t>
 * PDB type: std::basic_string<wchar_t,std::c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10E020, 395 bytes, global, 20 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Reallocate_grow_by<<lambda_e1befb086ad3257e3f042a63030725f7>,unsigned __int64,char>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10E1B0, 515 bytes, global, 21 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::_Reallocate_grow_by<<lambda_e749a49405295b58ff21f3ec583d0a05>,unsigned __int64,char16_t const *,unsigned __int64>
 * PDB type: std::basic_string<char16_t,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10E3C0, 733 bytes, global, 22 named locals
 * std::_Regex_replace1<std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,char const *,std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >
 * PDB type: std::back_insert_iterator<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10E6A0, 516 bytes, global, 12 named locals
 * std::_Regex_search2<char const *,std::allocator<std::sub_match<char const *> >,char,std::regex_traits<char>,char const *>
 * PDB type: bool (const char*, const char*, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10E8B0, 2188 bytes, global, 29 named locals
 * std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>
 * PDB type: void std::basic_regex<char,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10F140, 402 bytes, global, 21 named locals
 * std::vector<unsigned int,std::allocator<unsigned int> >::_Resize_reallocate<unsigned int>
 * PDB type: void std::vector<unsigned int,st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10F2E0, 353 bytes, global, 20 named locals
 * std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Resize_reallocate<std::_Value_init_tag>
 * PDB type: void std::vector<std::_Tgt_state...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10F450, 353 bytes, global, 20 named locals
 * std::vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >::_Resize_reallocate<std::_Value_init_tag>
 * PDB type: void std::vector<std::_Loop_vals...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10F5C0, 471 bytes, global, 18 named locals
 * std::vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >::_Resize_reallocate<std::_Value_init_tag>
 * PDB type: void std::vector<std::sub_match<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10F7A0, 738 bytes, global, 19 named locals
 * std::_Sort_unchecked<RESOLUTION *,bool (__cdecl*)(RESOLUTION const &,RESOLUTION const &)>
 * PDB type: void (RESOLUTION*, RESOLUTION*, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x10FA90, 33 bytes, global, 1 named locals
 * std::filesystem::_Stringoid_from_Source<char [256]>
 * PDB type: std::basic_string_view<char,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x10FAC0, 108 bytes, global, 2 named locals
 * std::_To_absolute_time<__int64,std::ratio<1,1000> >
 * PDB type: std::chrono::time_point<std::chr...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\thread
 */

/* 0x10FB30, 737 bytes, global, 20 named locals
 * std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Try_emplace<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const &>
 * PDB type: std::pair<std::_List_node<std::p...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x10FE20, 144 bytes, global, 11 named locals
 * std::_Uninitialized_move<CD3DApplication::SubMeshSet *,std::allocator<CD3DApplication::SubMeshSet> >
 * PDB type: CD3DApplication::SubMeshSet* (CD...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x10FEB0, 5 bytes, global, 3 named locals
 * __std_find_trivial<char const ,unsigned char>
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10FEC0, 132 bytes, global, 4 named locals
 * std::fill<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > > > *,std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > > > >
 * PDB type: void (std::_List_unchecked_itera...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10FF50, 322 bytes, global, 10 named locals
 * std::_Regex_traits<char>::lookup_classname<char const *>
 * PDB type: short std::_Regex_traits<char>::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1100A0, 65 bytes, global, 6 named locals
 * std::regex_replace<std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::_String_const_iterator<std::_String_val<std::_Simple_types<char> > >,std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >
 * PDB type: std::back_insert_iterator<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1100F0, 279 bytes, global, 10 named locals
 * std::regex_replace<std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110210, 353 bytes, global, 10 named locals
 * std::this_thread::sleep_for<__int64,std::ratio<1,1000> >
 * PDB type: void (const std::chrono::duratio...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\thread
 */

/* 0x110380, 313 bytes, global, 9 named locals
 * std::this_thread::sleep_until<std::chrono::steady_clock,std::chrono::duration<__int64,std::ratio<1,1000000000> > >
 * PDB type: void (const std::chrono::time_po...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\thread
 */

/* 0x1104C0, 576 bytes, global, 24 named locals
 * std::_Regex_traits<char>::transform_primary<char *>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110700, 576 bytes, global, 24 named locals
 * std::_Regex_traits<char>::transform_primary<char const *>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110940, 587 bytes, global, 23 named locals
 * std::_Regex_traits<char>::transform_primary<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110B90, 270 bytes, global, 10 named locals
 * std::use_facet<std::collate<char> >
 * PDB type: const std::collate<char>& (const...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0x110CA0, 175 bytes, global, 6 named locals
 * std::_Bt_state_t<char const *>::_Bt_state_t<char const *>
 * PDB type: void std::_Bt_state_t<char const...
 * Source: no line mapping
 */

/* 0x110D50, 145 bytes, global, 3 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Builder<char const *,char,std::regex_traits<char> >
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110DF0, 312 bytes, global, 13 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Matcher<char const *,char,std::regex_traits<char>,char const *>
 * PDB type: void std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110F30, 347 bytes, global, 7 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Parser<char const *,char,std::regex_traits<char> >
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x111090, 403 bytes, global, 12 named locals
 * std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >
 * PDB type: void std::basic_regex<char,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x111230, 299 bytes, global, 10 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >
 * PDB type: void std::basic_string<wchar_t,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x111360, 379 bytes, global, 15 named locals
 * CD3DApplication::SubMeshSet::SubMeshSet
 * PDB type: void CD3DApplication::SubMeshSet...
 * Source: no line mapping
 */

/* 0x1114E0, 4 bytes, global, 1 named locals
 * _D3DTLVERTEX::_D3DTLVERTEX
 * PDB type: void _D3DTLVERTEX::()
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\d3dtypes.h
 */

/* 0x1114F0, 4 bytes, global, 1 named locals
 * _linked_poly::_linked_poly
 * PDB type: void _linked_poly::()
 * Source: no line mapping
 */

/* 0x111500, 576 bytes, global, 11 named locals
 * el_chavo::el_chavo
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x111740, 134 bytes, global, 2 named locals
 * std::filesystem::filesystem_error::filesystem_error
 * PDB type: void std::filesystem::filesystem...
 * Source: no line mapping
 */

/* 0x1117D0, 302 bytes, global, 7 named locals
 * std::filesystem::filesystem_error::filesystem_error
 * PDB type: void std::filesystem::filesystem...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x111900, 231 bytes, global, 7 named locals
 * std::system_error::system_error
 * PDB type: void std::system_error::(std::er...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x1119F0, 20 bytes, global, 2 named locals
 * std::_Alloc_construct_ptr<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >::~_Alloc_construct_ptr<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_Alloc_construct_ptr<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x111A10, 20 bytes, global, 2 named locals
 * std::_Alloc_construct_ptr<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >::~_Alloc_construct_ptr<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_Alloc_construct_ptr<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x111A30, 93 bytes, global, 5 named locals
 * std::_Bt_state_t<char const *>::~_Bt_state_t<char const *>
 * PDB type: void std::_Bt_state_t<char const...
 * Source: no line mapping
 */

/* 0x111A90, 133 bytes, global, 6 named locals
 * std::_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >::~_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_List_node_emplace_op2...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\list
 */

/* 0x111B20, 124 bytes, global, 5 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::~_Matcher<char const *,char,std::regex_traits<char>,char const *>
 * PDB type: void std::_Matcher<char const *,...
 * Source: no line mapping
 */

/* 0x111BA0, 93 bytes, global, 5 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::~_Parser<char const *,char,std::regex_traits<char> >
 * PDB type: void std::_Parser<char const *,c...
 * Source: no line mapping
 */

/* 0x111C00, 172 bytes, global, 9 named locals
 * std::_Tgt_state_t<char const *>::~_Tgt_state_t<char const *>
 * PDB type: void std::_Tgt_state_t<char cons...
 * Source: no line mapping
 */

/* 0x111CB0, 95 bytes, global, 4 named locals
 * std::_Tidy_guard<std::_Builder<char const *,char,std::regex_traits<char> > >::~_Tidy_guard<std::_Builder<char const *,char,std::regex_traits<char> > >
 * PDB type: void std::_Tidy_guard<std::_Buil...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x111D10, 20 bytes, global, 2 named locals
 * std::_Tree_temp_node_alloc<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >::~_Tree_temp_node_alloc<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_Tree_temp_node_alloc<...
 * Source: no line mapping
 */

/* 0x111D30, 91 bytes, global, 5 named locals
 * std::_Vb_val<std::allocator<bool> >::~_Vb_val<std::allocator<bool> >
 * PDB type: void std::_Vb_val<std::allocator...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x111D90, 5 bytes, global, 1 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::~basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >
 * PDB type: void std::basic_string<wchar_t,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x111DA0, 44 bytes, global, 1 named locals
 * std::collate<char>::~collate<char>
 * PDB type: void std::collate<char>::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x111DD0, 92 bytes, global, 3 named locals
 * std::map<unsigned short,unsigned int,std::less<unsigned short>,std::allocator<std::pair<unsigned short const ,unsigned int> > >::~map<unsigned short,unsigned int,std::less<unsigned short>,std::allocator<std::pair<unsigned short const ,unsigned int> > >
 * PDB type: void std::map<unsigned short,uns...
 * Source: no line mapping
 */

/* 0x111E30, 9 bytes, global, 1 named locals
 * std::match_results<char const *,std::allocator<std::sub_match<char const *> > >::~match_results<char const *,std::allocator<std::sub_match<char const *> > >
 * PDB type: void std::match_results<char con...
 * Source: no line mapping
 */

/* 0x111E40, 47 bytes, global, 1 named locals
 * std::regex_traits<char>::~regex_traits<char>
 * PDB type: void std::regex_traits<char>::()
 * Source: no line mapping
 */

/* 0x111E70, 20 bytes, global, 1 named locals
 * std::unique_ptr<std::_Node_assert,std::default_delete<std::_Node_assert> >::~unique_ptr<std::_Node_assert,std::default_delete<std::_Node_assert> >
 * PDB type: void std::unique_ptr<std::_Node_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\memory
 */

/* 0x111E90, 87 bytes, global, 6 named locals
 * std::vector<char,std::allocator<char> >::~vector<char,std::allocator<char> >
 * PDB type: void std::vector<char,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x111EF0, 93 bytes, global, 6 named locals
 * std::vector<unsigned short,std::allocator<unsigned short> >::~vector<unsigned short,std::allocator<unsigned short> >
 * PDB type: void std::vector<unsigned short,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x111F50, 173 bytes, global, 7 named locals
 * std::vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >::~vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >
 * PDB type: void std::vector<CD3DApplication...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x112000, 91 bytes, global, 5 named locals
 * std::vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >::~vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >
 * PDB type: void std::vector<std::_Loop_vals...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x112060, 122 bytes, global, 5 named locals
 * std::vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >::~vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >
 * PDB type: void std::vector<std::sub_match<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1120E0, 91 bytes, global, 5 named locals
 * std::vector<bool,std::allocator<bool> >::~vector<bool,std::allocator<bool> >
 * PDB type: void std::vector<bool,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x112140, 104 bytes, global, 6 named locals
 * CD3DApplication::SubMeshSet::~SubMeshSet
 * PDB type: void CD3DApplication::SubMeshSet...
 * Source: no line mapping
 */

/* 0x1121B0, 11 bytes, global, 1 named locals
 * std::_Node_base::~_Node_base
 * PDB type: void std::_Node_base::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1121C0, 8 bytes, global, 1 named locals
 * std::_System_error_message::~_System_error_message
 * PDB type: void std::_System_error_message:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x1121D0, 15 bytes, global, 1 named locals
 * el_chavo::~el_chavo
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1121E0, 138 bytes, global, 5 named locals
 * std::filesystem::filesystem_error::~filesystem_error
 * PDB type: void std::filesystem::filesystem...
 * Source: no line mapping
 */

/* 0x112270, 5 bytes, global, 1 named locals
 * std::filesystem::path::~path
 * PDB type: void std::filesystem::path::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x112280, 19 bytes, global, 1 named locals
 * std::runtime_error::~runtime_error
 * PDB type: void std::runtime_error::()
 * Source: no line mapping
 */

/* 0x1122A0, 131 bytes, global, 6 named locals
 * std::_Tgt_state_t<char const *>::operator=
 * PDB type: std::_Tgt_state_t<char const *>&...
 * Source: no line mapping
 */

/* 0x112330, 19 bytes, global, 3 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::operator[]
 * PDB type: char& std::basic_string<char,std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x112350, 233 bytes, global, 4 named locals
 * std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::operator[]
 * PDB type: _Material*& std::map<std::basic_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\map
 */

/* 0x112440, 111 bytes, global, 2 named locals
 * std::_Vb_iterator<std::_Wrap_alloc<std::allocator<unsigned int> > >::operator+
 * PDB type: std::_Vb_iterator<std::_Wrap_all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1124B0, 263 bytes, global, 5 named locals
 * std::_Node_class<char,std::regex_traits<char> >::`scalar deleting destructor'
 * PDB type: void* std::_Node_class<char,std:...
 * Source: no line mapping
 */

/* 0x1125C0, 66 bytes, global, 1 named locals
 * std::_Node_str<char>::`scalar deleting destructor'
 * PDB type: void* std::_Node_str<char>::(uns...
 * Source: no line mapping
 */

/* 0x112610, 76 bytes, global, 1 named locals
 * std::collate<char>::`scalar deleting destructor'
 * PDB type: void* std::collate<char>::(unsig...
 * Source: no line mapping
 */

/* 0x112660, 33 bytes, global, 1 named locals
 * std::_Generic_error_category::`scalar deleting destructor'
 * PDB type: void* std::_Generic_error_catego...
 * Source: no line mapping
 */

/* 0x112690, 129 bytes, global, 3 named locals
 * std::_Node_assert::`scalar deleting destructor'
 * PDB type: void* std::_Node_assert::(unsign...
 * Source: no line mapping
 */

/* 0x112720, 43 bytes, global, 1 named locals
 * std::_Node_back::`scalar deleting destructor'
 * PDB type: void* std::_Node_back::(unsigned...
 * Source: no line mapping
 */

/* 0x112750, 43 bytes, global, 1 named locals
 * std::_Node_base::`scalar deleting destructor'
 * PDB type: void* std::_Node_base::(unsigned...
 * Source: no line mapping
 */

/* 0x112780, 43 bytes, global, 1 named locals
 * std::_Node_capture::`scalar deleting destructor'
 * PDB type: void* std::_Node_capture::(unsig...
 * Source: no line mapping
 */

/* 0x1127B0, 43 bytes, global, 1 named locals
 * std::_Node_end_group::`scalar deleting destructor'
 * PDB type: void* std::_Node_end_group::(uns...
 * Source: no line mapping
 */

/* 0x1127E0, 43 bytes, global, 1 named locals
 * std::_Node_end_rep::`scalar deleting destructor'
 * PDB type: void* std::_Node_end_rep::(unsig...
 * Source: no line mapping
 */

/* 0x112810, 43 bytes, global, 1 named locals
 * std::_Node_endif::`scalar deleting destructor'
 * PDB type: void* std::_Node_endif::(unsigne...
 * Source: no line mapping
 */

/* 0x112840, 188 bytes, global, 6 named locals
 * std::_Node_if::`scalar deleting destructor'
 * PDB type: void* std::_Node_if::(unsigned)
 * Source: no line mapping
 */

/* 0x112900, 43 bytes, global, 1 named locals
 * std::_Node_rep::`scalar deleting destructor'
 * PDB type: void* std::_Node_rep::(unsigned)
 * Source: no line mapping
 */

/* 0x112930, 43 bytes, global, 1 named locals
 * std::_Root_node::`scalar deleting destructor'
 * PDB type: void* std::_Root_node::(unsigned...
 * Source: no line mapping
 */

/* 0x112960, 33 bytes, global, 1 named locals
 * std::_System_error_category::`scalar deleting destructor'
 * PDB type: void* std::_System_error_categor...
 * Source: no line mapping
 */

/* 0x112990, 172 bytes, global, 5 named locals
 * std::filesystem::filesystem_error::`scalar deleting destructor'
 * PDB type: void* std::filesystem::filesyste...
 * Source: no line mapping
 */

/* 0x112A40, 1227 bytes, global, 16 named locals
 * el_chavo::ApplyCulling
 * PDB type: int el_chavo::(&, int, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x112F10, 169 bytes, global, 5 named locals
 * el_chavo::ApplyLevelTransformation
 * PDB type: void el_chavo::(MATRIX*, float, ...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x112FC0, 142 bytes, global, 10 named locals
 * el_chavo::ApplyProjection
 * PDB type: void el_chavo::(FVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x113050, 614 bytes, global, 11 named locals
 * el_chavo::ApplyProjectionPolyArray
 * PDB type: void el_chavo::(&, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1132C0, 307 bytes, global, 5 named locals
 * el_chavo::CleanupFBXData
 * PDB type: void el_chavo::(std::vector<CD3D...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x113400, 21 bytes, global, 1 named locals
 * el_chavo::DeleteDeviceObjects
 * PDB type: HRESULT el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x113420, 3 bytes, global, 6 named locals
 * el_chavo::DrawLine2d
 * PDB type: void el_chavo::(int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x113430, 108 bytes, global, 10 named locals
 * el_chavo::DrawLine
 * PDB type: void el_chavo::(int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1134A0, 212 bytes, global, 10 named locals
 * el_chavo::DrawSphere
 * PDB type: void el_chavo::(int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x113580, 4125 bytes, global, 78 named locals
 * el_chavo::DrawUITextUTF16
 * PDB type: void el_chavo::(unsigned short*,...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1145A0, 4261 bytes, global, 79 named locals
 * el_chavo::DrawUITextUTF16Depth
 * PDB type: void el_chavo::(unsigned short*,...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x115650, 2103 bytes, global, 55 named locals
 * el_chavo::DrawUITextUTF16_3D
 * PDB type: void el_chavo::(unsigned short*,...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x115E90, 5208 bytes, global, 50 named locals
 * el_chavo::EndPoly
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1172F0, 28 bytes, global, 1 named locals
 * el_chavo::FinalCleanup
 * PDB type: HRESULT el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x117310, 560 bytes, global, 1 named locals
 * GetAchNameFromIndex
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x117540, 99 bytes, global, 2 named locals
 * el_chavo::InitDeviceObjects
 * PDB type: HRESULT el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1175B0, 6700 bytes, global, 124 named locals
 * el_chavo::InitFBXLevelData
 * PDB type: void el_chavo::(ufbx_scene*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x118FE0, 3 bytes, global, 3 named locals
 * el_chavo::InitFBXTextureData
 * PDB type: void el_chavo::(char*, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x118FF0, 65 bytes, global, 1 named locals
 * el_chavo::InitTransPolys
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x119040, 42 bytes, global, 2 named locals
 * IsNullTerminated
 * PDB type: bool (const char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x119070, 289 bytes, global, 9 named locals
 * el_chavo::LoadTexture
 * PDB type: Texture* el_chavo::(char*, unsig...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1191A0, 220 bytes, global, 5 named locals
 * ModifyFilename
 * PDB type: char* (const char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x119280, 5529 bytes, global, 65 named locals
 * el_chavo::NoScaleEndPoly
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x11A820, 56 bytes, global, 2 named locals
 * el_chavo::OnKeyUp
 * PDB type: void el_chavo::(int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x11A860, 3 bytes, global, 1 named locals
 * el_chavo::OneTimeSceneInit
 * PDB type: HRESULT el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x11A870, 38 bytes, global, 1 named locals
 * el_chavo::PlotTransPolys
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x11A8A0, 99 bytes, global, 6 named locals
 * SetFilenameExtension
 * PDB type: void (char*, char*, char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x11A910, 399 bytes, global, 8 named locals
 * el_chavo::SetVert
 * PDB type: void el_chavo::(int, float, floa...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x11AAA0, 187 bytes, global, 3 named locals
 * el_chavo::StartPoly
 * PDB type: void el_chavo::(int, _Material*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x11AB60, 388 bytes, global, 10 named locals
 * UpdateValidResolutions
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x11ACF0, 138 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_backreference
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11AD80, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_bol
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11AD90, 287 bytes, global, 4 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_char
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11AEB0, 177 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_char_to_array
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11AF70, 131 bytes, global, 4 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_char_to_bitmap
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B000, 131 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_char_to_class
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B090, 146 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_class
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B130, 27 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_coll
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B150, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_dot
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B160, 230 bytes, global, 6 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_elts
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B250, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_eol
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B260, 394 bytes, global, 16 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_equiv
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B3F0, 216 bytes, global, 6 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_named_class
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B4D0, 404 bytes, global, 10 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_range
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B670, 809 bytes, global, 15 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_rep
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B9A0, 130 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_str_node
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11BA30, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_wbound
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11BA40, 1587 bytes, global, 15 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Alternative
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C080, 384 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_AtomEscape
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C200, 24 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Beg_expr
 * PDB type: bool std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C220, 53 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Beg_expr
 * PDB type: bool std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C260, 209 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_assert_group
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C340, 141 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_capture_group
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C3D0, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_group
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C3E0, 242 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_if
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C4E0, 168 bytes, global, 3 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Better_match
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C590, 138 bytes, global, 6 named locals
 * std::vector<Vertex,std::allocator<Vertex> >::_Buy_nonzero
 * PDB type: void std::vector<Vertex,std::all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x11C620, 135 bytes, global, 6 named locals
 * std::vector<unsigned int,std::allocator<unsigned int> >::_Buy_raw
 * PDB type: void std::vector<unsigned int,st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x11C6B0, 134 bytes, global, 6 named locals
 * std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Buy_raw
 * PDB type: void std::vector<std::_Tgt_state...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x11C740, 224 bytes, global, 4 named locals
 * std::_Calculate_loop_simplicity
 * PDB type: void (std::_Node_base*, std::_No...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C820, 234 bytes, global, 8 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Char_to_elts
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C910, 270 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_CharacterClass
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11CA20, 168 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_CharacterClassEscape
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11CAD0, 1088 bytes, global, 9 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_CharacterEscape
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11CF10, 522 bytes, global, 3 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_ClassAtom
 * PDB type: std::_Prs_ret std::_Parser<char ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D120, 132 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_ClassEscape
 * PDB type: std::_Prs_ret std::_Parser<char ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D1B0, 348 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_ClassRanges
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D310, 145 bytes, global, 5 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Compile
 * PDB type: std::_Root_node* std::_Parser<ch...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D3B0, 240 bytes, global, 6 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_DecimalDigits
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D4A0, 604 bytes, global, 11 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Disjunction
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D700, 230 bytes, global, 5 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_assert_group
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D7F0, 123 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_capture_group
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D870, 386 bytes, global, 12 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_class
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DA00, 279 bytes, global, 7 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_digits
 * PDB type: int std::_Parser<char const *,ch...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DB20, 414 bytes, global, 8 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_ex_class
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DCC0, 78 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_ffn
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DD10, 35 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_ffnx
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DD40, 1069 bytes, global, 33 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E170, 70 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_noncapture_group
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E1C0, 1063 bytes, global, 34 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E5F0, 759 bytes, global, 25 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E8F0, 172 bytes, global, 6 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Else_if
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E9A0, 36 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_End_assert_group
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E9D0, 177 bytes, global, 3 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_End_group
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EA90, 28 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_End_pattern
 * PDB type: std::_Root_node* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EAB0, 12 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Error
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EAC0, 106 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Expect
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EB30, 861 bytes, global, 40 named locals
 * std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Forced_rehash
 * PDB type: void std::_Hash<std::_Umap_trait...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x11EE90, 4 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Get_bmax
 * PDB type: unsigned std::_Builder<char cons...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EEA0, 7 bytes, global, 1 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Get_ncap
 * PDB type: unsigned std::_Matcher<char cons...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EEB0, 4 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Get_tmax
 * PDB type: unsigned std::_Builder<char cons...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EEC0, 516 bytes, global, 6 named locals
 * std::collate<char>::_Getcat
 * PDB type: unsigned __int64 std::collate<ch...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x11F0D0, 5 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Getmark
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F0E0, 239 bytes, global, 6 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_HexDigits
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F1D0, 392 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_IdentityEscape
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F360, 25 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Insert_node
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F380, 603 bytes, global, 13 named locals
 * std::_Tree_val<std::_Tree_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::_Insert_node
 * PDB type: std::_Tree_node<std::pair<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0x11F5E0, 553 bytes, global, 9 named locals
 * std::vector<bool,std::allocator<bool> >::_Insert_x
 * PDB type: unsigned __int64 std::vector<boo...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x11F810, 288 bytes, global, 1 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_IsIdentityEscape
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F930, 55 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Is_esc
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F970, 115 bytes, global, 1 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Is_wbound
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F9F0, 53 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Link_node
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11FA30, 17 bytes, global, 1 named locals
 * std::_Make_ec
 * PDB type: std::error_code (__std_win_error...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x11FA50, 9 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Mark_final
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11FA60, 1576 bytes, global, 23 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Match_pat
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120090, 9 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Negate
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1200A0, 134 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_New_node
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120130, 80 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Next
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120180, 212 bytes, global, 5 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_OctalDigits
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120260, 743 bytes, global, 28 named locals
 * std::filesystem::filesystem_error::_Pretty_message
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x120550, 411 bytes, global, 7 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Quantifier
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1206F0, 191 bytes, global, 8 named locals
 * std::match_results<char const *,std::allocator<std::sub_match<char const *> > >::_Resize
 * PDB type: void std::match_results<char con...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1207B0, 8 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Setlong
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1207C0, 736 bytes, global, 16 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Skip
 * PDB type: const char* std::_Matcher<char c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120AA0, 85 bytes, global, 3 named locals
 * std::filesystem::_Throw_fs_error
 * PDB type: void (const char*, const std::er...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x120B00, 102 bytes, global, 3 named locals
 * std::filesystem::_Throw_fs_error
 * PDB type: void (const char*, __std_win_err...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x120B70, 57 bytes, global, 1 named locals
 * std::_Throw_system_error
 * PDB type: void (const std::errc)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x120BB0, 57 bytes, global, 1 named locals
 * std::_Throw_system_error_from_std_win_error
 * PDB type: void (const __std_win_error)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x120BF0, 98 bytes, global, 3 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Tidy
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120C60, 125 bytes, global, 3 named locals
 * std::basic_regex<char,std::regex_traits<char> >::_Tidy
 * PDB type: void std::basic_regex<char,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120CE0, 118 bytes, global, 5 named locals
 * std::vector<Vertex,std::allocator<Vertex> >::_Tidy
 * PDB type: void std::vector<Vertex,std::all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x120D60, 97 bytes, global, 5 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::_Tidy_deallocate
 * PDB type: void std::basic_string<char16_t,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x120DD0, 97 bytes, global, 5 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::_Tidy_deallocate
 * PDB type: void std::basic_string<wchar_t,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x120E40, 516 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Trans
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x121050, 108 bytes, global, 3 named locals
 * std::vector<bool,std::allocator<bool> >::_Trim
 * PDB type: void std::vector<bool,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1210C0, 348 bytes, global, 6 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Wrapped_disjunction
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x121220, 17 bytes, global, 0 named locals
 * std::vector<bool,std::allocator<bool> >::_Xlen
 * PDB type: void std::vector<bool,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121240, 17 bytes, global, 0 named locals
 * std::vector<char,std::allocator<char> >::_Xlength
 * PDB type: void std::vector<char,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121260, 17 bytes, global, 0 named locals
 * std::vector<unsigned short,std::allocator<unsigned short> >::_Xlength
 * PDB type: void std::vector<unsigned short,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121280, 17 bytes, global, 0 named locals
 * std::vector<CD3DApplication::FBX_MESH *,std::allocator<CD3DApplication::FBX_MESH *> >::_Xlength
 * PDB type: void std::vector<CD3DApplication...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1212A0, 17 bytes, global, 0 named locals
 * std::vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >::_Xlength
 * PDB type: void std::vector<CD3DApplication...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1212C0, 17 bytes, global, 0 named locals
 * std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Xlength
 * PDB type: void std::vector<std::_Tgt_state...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1212E0, 17 bytes, global, 0 named locals
 * std::vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >::_Xlength
 * PDB type: void std::vector<std::_Loop_vals...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121300, 17 bytes, global, 0 named locals
 * std::vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >::_Xlength
 * PDB type: void std::vector<std::sub_match<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121320, 17 bytes, global, 0 named locals
 * std::_String_val<std::_Simple_types<char16_t> >::_Xran
 * PDB type: void std::_String_val<std::_Simp...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x121340, 62 bytes, global, 7 named locals
 * std::allocator<unsigned short>::deallocate
 * PDB type: void std::allocator<unsigned sho...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x121380, 66 bytes, global, 7 named locals
 * std::allocator<CD3DApplication::FBX_MESH *>::deallocate
 * PDB type: void std::allocator<CD3DApplicat...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x1213D0, 62 bytes, global, 7 named locals
 * std::allocator<CD3DApplication::SubMeshSet>::deallocate
 * PDB type: void std::allocator<CD3DApplicat...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x121410, 65 bytes, global, 7 named locals
 * std::allocator<std::_Tgt_state_t<char const *>::_Grp_t>::deallocate
 * PDB type: void std::allocator<std::_Tgt_st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x121460, 65 bytes, global, 7 named locals
 * std::allocator<std::_Loop_vals_t>::deallocate
 * PDB type: void std::allocator<std::_Loop_v...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x1214B0, 66 bytes, global, 7 named locals
 * std::allocator<std::sub_match<char const *> >::deallocate
 * PDB type: void std::allocator<std::sub_mat...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x121500, 114 bytes, global, 3 named locals
 * std::_System_error_category::default_error_condition
 * PDB type: std::error_condition std::_Syste...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121580, 73 bytes, global, 6 named locals
 * std::collate<char>::do_compare
 * PDB type: int std::collate<char>::(const c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x1215D0, 53 bytes, global, 5 named locals
 * std::collate<char>::do_hash
 * PDB type: long std::collate<char>::(const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x121610, 415 bytes, global, 12 named locals
 * std::collate<char>::do_transform
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x1217B0, 500 bytes, global, 6 named locals
 * std::vector<bool,std::allocator<bool> >::erase
 * PDB type: std::_Vb_iterator<std::_Wrap_all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1219B0, 295 bytes, global, 11 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::insert
 * PDB type: std::basic_string<char16_t,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x121AE0, 17 bytes, global, 1 named locals
 * std::make_error_code
 * PDB type: std::error_code (std::errc)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121B00, 78 bytes, global, 3 named locals
 * std::_Generic_error_category::message
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121B50, 151 bytes, global, 4 named locals
 * std::_System_error_category::message
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121BF0, 8 bytes, global, 1 named locals
 * std::_Generic_error_category::name
 * PDB type: const char* std::_Generic_error_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121C00, 8 bytes, global, 1 named locals
 * std::_System_error_category::name
 * PDB type: const char* std::_System_error_c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121C10, 366 bytes, global, 7 named locals
 * std::locale::name
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0x121D80, 216 bytes, global, 5 named locals
 * std::chrono::steady_clock::now
 * PDB type: std::chrono::time_point<std::chr...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\__msvc_chrono.hpp
 */

/* 0x121E60, 189 bytes, global, 6 named locals
 * el_chavo::renderLoadProgress
 * PDB type: void el_chavo::(int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x121F20, 852 bytes, global, 19 named locals
 * el_chavo::renderVideoFrame
 * PDB type: void el_chavo::(SDL_Surface*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122280, 147 bytes, global, 7 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::resize
 * PDB type: void std::basic_string<char,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x122320, 137 bytes, global, 8 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::resize
 * PDB type: void std::basic_string<wchar_t,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x1223B0, 642 bytes, global, 16 named locals
 * std::vector<bool,std::allocator<bool> >::resize
 * PDB type: void std::vector<bool,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x122640, 16 bytes, global, 2 named locals
 * resolutionComparison
 * PDB type: bool (const RESOLUTION&, const R...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122650, 165 bytes, global, 8 named locals
 * std::_Regex_traits<char>::translate
 * PDB type: char std::_Regex_traits<char>::(...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x122700, 15 bytes, global, 2 named locals
 * std::filesystem::filesystem_error::what
 * PDB type: const char* std::filesystem::fil...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x122710, 330 bytes, global, 2 named locals
 * CleanupLevelData
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122860, 59 bytes, global, 0 named locals
 * ClearWindow
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1228A0, 26 bytes, global, 0 named locals
 * CtrlKeyDown
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1228C0, 35 bytes, global, 5 named locals
 * DrawTile
 * PDB type: int (int, int, int, int, unsigne...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1228F0, 3 bytes, global, 5 named locals
 * DrawUIRect
 * PDB type: void (int, int, int, int, long)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122900, 25 bytes, global, 2 named locals
 * GetWindowSize
 * PDB type: void (int*, int*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122920, 15 bytes, global, 1 named locals
 * KeyHeld
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122930, 15 bytes, global, 1 named locals
 * KeyPressed
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122940, 15 bytes, global, 1 named locals
 * KeyReleased
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122950, 7 bytes, global, 0 named locals
 * LastKey
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122960, 1273 bytes, global, 22 named locals
 * LoadGameData
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x122E60, 2152 bytes, global, 31 named locals
 * LoadOptionsData
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1236D0, 11 bytes, global, 0 named locals
 * MarkFontAtlasForRefresh
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void MarkFontAtlasForRefresh(void)
{
    refreshFontAtlasFlag = 1;
}

/* 0x1236E0, 150 bytes, global, 4 named locals
 * OutputTextXY
 * PDB type: void (int, int, char*, <no type>...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x123780, 4600 bytes, global, 107 named locals
 * PlayVideo
 * PDB type: void (char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x124980, 3 bytes, global, 0 named locals
 * PresentWindow
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x124990, 22 bytes, global, 0 named locals
 * RefreshFontAtlas
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1249B0, 161 bytes, global, 7 named locals
 * RenderUIText
 * PDB type: void (int, int, int, int, SDL_Su...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x124A60, 223 bytes, global, 9 named locals
 * RenderUITexture
 * PDB type: void (_Material*, SDL_Rect, SDL_...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x124B40, 21 bytes, global, 0 named locals
 * SDL_ResetClipRect
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x124B60, 79 bytes, global, 5 named locals
 * SDL_SetClip
 * PDB type: void (int, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x124BB0, 1228 bytes, global, 23 named locals
 * SaveGameData
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125080, 1241 bytes, global, 24 named locals
 * SaveSettingsData
 * PDB type: void (optionstruct)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125560, 20 bytes, global, 1 named locals
 * SetInMenu
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125580, 26 bytes, global, 0 named locals
 * ShiftKeyDown
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1255A0, 20 bytes, global, 3 named locals
 * UpdateResolution
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1255C0, 497 bytes, global, 11 named locals
 * WaitVBlank
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1257C0, 590 bytes, global, 6 named locals
 * WinMain
 * PDB type: int (HINSTANCE__*, HINSTANCE__*,...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125A10, 3 bytes, global, 1 named locals
 * WriteToOutputFile
 * PDB type: void (const char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125A20, 221 bytes, global, 4 named locals
 * _ApplyLevelTransformation
 * PDB type: void (MATRIX*, float, float, flo...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _ApplyLevelTransformation(
    MATRIX *matrix, float x_scale, float y_scale, float z_scale)
{
    int row;
    int column;

    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            jpb_level_transformation.world[column][row] =
                matrix->m[row][column];
        }
    }
    jpb_level_transformation.world[3][0] = (float)matrix->t[0];
    jpb_level_transformation.world[3][1] = (float)matrix->t[1];
    jpb_level_transformation.world[3][2] = (float)matrix->t[2];
    jpb_level_transformation.scale[0] = x_scale;
    jpb_level_transformation.scale[1] = y_scale;
    jpb_level_transformation.scale[2] = z_scale;
    jpb_level_transformation.scale[3] = 1.0f;
}

/* 0x125B00, 108 bytes, global, 9 named locals
 * _ApplyProjection
 * PDB type: void (FVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125B70, 81 bytes, global, 1 named locals
 * _ClearTextureCache
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125BD0, 309 bytes, global, 6 named locals
 * _DrawTexture
 * PDB type: void (_Material*, SCREENRECT, co...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _DrawTexture(
    _Material *texture,
    SCREENRECT destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth)
{
    if (jpb_draw_texture_hook != nullptr) {
        jpb_draw_texture_hook(
            jpb_draw_texture_user_data,
            texture,
            &destination,
            source,
            color,
            layer_depth);
    }
}

/* 0x125D10, 339 bytes, global, 7 named locals
 * _DrawTextureClipped
 * PDB type: void (_Material*, SCREENRECT, co...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125E70, 53 bytes, global, 5 named locals
 * _DrawUITextUTF16
 * PDB type: void (unsigned short*, SCREENREC...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125EB0, 71 bytes, global, 6 named locals
 * _DrawUITextUTF16Depth
 * PDB type: void (unsigned short*, SCREENREC...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125F00, 63 bytes, global, 7 named locals
 * _DrawUITextUTF16_3D
 * PDB type: void (unsigned short*, float, fl...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x125F40, 12 bytes, global, 0 named locals
 * _EndPoly
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
static void jpb_whook_publish_screen_poly(int no_scale)
{
    if (jpb_screen_poly_hook != nullptr &&
        jpb_screen_poly_builder.requestedVertexCount > 0 &&
        jpb_screen_poly_builder.requestedVertexCount <=
            JPB_SCREEN_POLY_VERTEX_CAPACITY) {
        jpb_screen_poly_hook(
            jpb_screen_poly_user_data,
            jpb_screen_poly_builder.material,
            jpb_screen_poly_builder.requestedVertexCount,
            jpb_screen_poly_builder.vertices,
            no_scale);
    }
    jpb_screen_poly_builder.material = nullptr;
    jpb_screen_poly_builder.requestedVertexCount = 0;
}

void _EndPoly(void)
{
    jpb_whook_publish_screen_poly(0);
}

/* Reference RVA 0x125F50; the platform destructor is isolated by texture.c. */
void _FreeTexture(_Material *texture)
{
    if (texture == nullptr || texture->texture == nullptr) {
        return;
    }
    jpb_TextureUnloadPlatformResource(texture->texture);
    texture->texture = nullptr;
}

/* 0x1262D0, 15 bytes, global, 1 named locals
 * _InitFBXLevelData
 * PDB type: void (ufbx_scene*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* Reference RVA 0x1262E0, excluding the original D3D12 resource internals. */
_Material *_LoadTexture(
    char *filename, TT_TEXTYPE texturetype, unsigned long option)
{
    const char *resolvedFilename = filename;
    const char *baseName;
    _Material *material;
    void *texture;
    int materialtype = 0;
    int16_t width = 0;
    int16_t height = 0;
    size_t filenameLength;
    int index;

    if (resolvedFilename == nullptr) {
        resolvedFilename = resource_getPath(
            "white.png", JPB_RESOURCE_DEFAULT);
    }
    if (resolvedFilename == nullptr) {
        return nullptr;
    }
    for (index = 0; index < JPB_TEXTURE_MATERIAL_CAPACITY; ++index) {
        material = &g_material[index];
        if (material->type != TT_FREE &&
            material->texture != nullptr &&
            std::strcmp(material->filename, resolvedFilename) == 0) {
            return material;
        }
    }

    material = texture_GetMaterial(texturetype);
    if (material == nullptr) {
        return nullptr;
    }
    filenameLength = std::strlen(resolvedFilename);
    if (filenameLength >= sizeof(material->filename)) {
        texture_FreeMaterial(material);
        return nullptr;
    }
    std::memcpy(
        material->filename,
        resolvedFilename,
        filenameLength + 1);

    baseName = resolvedFilename;
    for (const char *cursor = resolvedFilename;
         *cursor != '\0'; ++cursor) {
        if (*cursor == '\\' || *cursor == '/' || *cursor == ':') {
            baseName = cursor + 1;
        }
    }
    /* Exact retail prefix classification: lowercase a_/p_ only. */
    if (baseName[0] == 'a' && baseName[1] == '_') {
        materialtype = 2;
    } else if (baseName[0] == 'p' && baseName[1] == '_') {
        materialtype = 1;
    }

    texture = jpb_TextureLoadPlatformResource(
        resolvedFilename,
        static_cast<unsigned>(option) & UINT32_C(0x02000000),
        materialtype,
        &width,
        &height);
    if (texture == nullptr) {
        texture = jpb_TextureLoadPlatformResource(
            "../../../res/default\\o_default.tga",
            0,
            0,
            &width,
            &height);
    }
    material->samplerType =
        jpb_TextureIsPartOfAtlas(material->filename)
            ? TEXTURSAMPLER_POINTCLAMP
            : TEXTURESAMPLER_LINEARCLAMP;
    if (texture == nullptr) {
        texture_FreeMaterial(material);
        return nullptr;
    }
    material->texture = texture;
    material->iw = width;
    material->ih = height;
    SetTextureColorOverride((int)(int8_t)LevelSelect, material);
    return material;
}

/* 0x1266D0, 12 bytes, global, 0 named locals
 * _NoScaleEndPoly
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _NoScaleEndPoly(void)
{
    jpb_whook_publish_screen_poly(1);
}

/* 0x1266E0, 70 bytes, global, 7 named locals
 * _SetVert
 * PDB type: void (int, float, float, float, ...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _SetVert(
    int vertex,
    float x,
    float y,
    float z,
    unsigned long argb,
    float tu,
    float tv)
{
    JPBScreenPolyVertex *destination;

    if (vertex < 0 ||
        vertex >= jpb_screen_poly_builder.requestedVertexCount ||
        vertex >= JPB_SCREEN_POLY_VERTEX_CAPACITY) {
        return;
    }
    destination = &jpb_screen_poly_builder.vertices[vertex];
    destination->x = x;
    destination->y = y;
    destination->z = z;
    destination->argb = static_cast<uint32_t>(argb);
    destination->tu = tu;
    destination->tv = tv;
}

/* 0x126730, 17 bytes, global, 2 named locals
 * _StartPoly
 * PDB type: void (int, _Material*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _StartPoly(int vertex_count, _Material *material)
{
    jpb_screen_poly_builder.material = material;
    jpb_screen_poly_builder.requestedVertexCount = vertex_count;
    std::memset(
        jpb_screen_poly_builder.vertices,
        0,
        sizeof(jpb_screen_poly_builder.vertices));
}

/* 0x126750, 3 bytes, global, 0 named locals
 * _StoreDescriptorHeapOffsetsEnd
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x126760, 3 bytes, global, 0 named locals
 * _StoreDescriptorHeapOffsetsStart
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* Reference RVA 0x126770; fixed storage replaces the retail std::map owner. */
_Material *_TryLoadTexture(
    const char *baseFileName,
    TT_TEXTYPE texturetype,
    unsigned long option)
{
    struct TryTextureEntry {
        std::string name;
        _Material *material;
    };
    static TryTextureEntry entries[JPB_TEXTURE_MATERIAL_CAPACITY];
    static size_t entryCount;
    std::string levelName;
    std::string convertedFileName;
    const char *fullFilePath;
    _Material *materialHandle;
    size_t cachedIndex = JPB_TEXTURE_MATERIAL_CAPACITY;
    size_t index;

    if (baseFileName == nullptr) {
        return nullptr;
    }
    for (index = 0; index < entryCount; ++index) {
        if (entries[index].name == baseFileName) {
            cachedIndex = index;
            if (entries[index].material != nullptr &&
                entries[index].material->texture != nullptr) {
                return entries[index].material;
            }
            break;
        }
    }
    levelName = loader_GetLevelName();
    if (levelName == "arena") {
        levelName = "fed";
    }
    convertedFileName = levelName + "/" + baseFileName;
    fullFilePath = resource_getPath(
        convertedFileName.c_str(), JPB_RESOURCE_LEVEL_JPX);
    if (fullFilePath == nullptr) {
        return nullptr;
    }
    materialHandle = _LoadTexture(
        const_cast<char *>(fullFilePath), texturetype, option);
    if (cachedIndex < entryCount) {
        entries[cachedIndex].material = materialHandle;
    } else if (entryCount < JPB_TEXTURE_MATERIAL_CAPACITY) {
        entries[entryCount].name = baseFileName;
        entries[entryCount].material = materialHandle;
        ++entryCount;
    }
    return materialHandle;
}

/* 0x126DA0, 71 bytes, global, 0 named locals
 * __EndRender
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x126DF0, 23 bytes, global, 0 named locals
 * __HandleWindow
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x126E10, 3 bytes, global, 0 named locals
 * __InitSystem
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x126E20, 136 bytes, global, 2 named locals
 * __PCTrace
 * PDB type: void (char*, <no type>)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x126EB0, 236 bytes, global, 5 named locals
 * __RenderLoad
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x126FA0, 71 bytes, global, 0 named locals
 * __StartRender
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x126FF0, 699 bytes, local, 11 named locals
 * audio_callback
 * PDB type: void (void*, unsigned char*, int...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1272B0, 5 bytes, global, 0 named locals
 * clearzerobss
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1272C0, 652 bytes, global, 13 named locals
 * cliptoscreen
 * PDB type: int (short*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127550, 23 bytes, global, 4 named locals
 * console_TextureListCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127570, 141 bytes, global, 2 named locals
 * dbgprintf
 * PDB type: void (char*, <no type>)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127600, 764 bytes, global, 27 named locals
 * debug_box
 * PDB type: void (_svector*, _svector*, unsi...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127900, 100 bytes, global, 9 named locals
 * debug_drawline
 * PDB type: void (int, int, int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127970, 156 bytes, global, 10 named locals
 * debug_drawpoint
 * PDB type: void (int, int, int, int, int, u...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127A10, 35 bytes, global, 5 named locals
 * debug_drawpoint2d
 * PDB type: void (int, int, int, int, unsign...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127A40, 200 bytes, global, 9 named locals
 * debug_drawsphere
 * PDB type: void (int, int, int, int, unsign...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
static int jpb_whook_fixed_product(int left, int right)
{
    uint32_t bits = (uint32_t)left * (uint32_t)right;

    if ((bits & UINT32_C(0x80000000)) != 0) {
        bits = ~(~bits >> 12);
    } else {
        bits >>= 12;
    }
    int32_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

int cliptoscreen(short *pos)
{
    enum {
        LEFT = 1,
        RIGHT = 2,
        TOP = 4,
        BOTTOM = 8,
        MIN_X = 0x18,
        MAX_X = 0x768,
        MIN_Y = 8,
        MAX_Y = 0x430
    };
    int original_x = pos[0];
    int original_y = pos[1];
    int x = original_x;
    int y = original_y;
    int clipcode = 0;
    int alpha = 0xff;
    int center_delta_x;
    int center_delta_y;
    int y_over_x = 0;
    int x_over_y = 0;
    int vertical_distance;
    int horizontal_distance;

    if (x < MIN_X) {
        clipcode |= LEFT;
    }
    if (x > MAX_X) {
        clipcode |= RIGHT;
    }
    if (y < MIN_Y) {
        clipcode |= TOP;
    }
    if (y > MAX_Y) {
        clipcode |= BOTTOM;
    }
    if (clipcode == 0) {
        return alpha;
    }

    center_delta_y = (OptionStruct.ScreenHeight >> 1) - y;
    center_delta_x = (OptionStruct.ScreenWidth >> 1) - x;
    if (center_delta_x != 0) {
        y_over_x = (center_delta_y * 0x1000) / center_delta_x;
    }

    vertical_distance =
        (clipcode & TOP) != 0 ? MIN_Y - y : y - MAX_Y;
    horizontal_distance =
        (clipcode & LEFT) != 0 ? MIN_X - x : x - MAX_X;
    if (vertical_distance < (horizontal_distance >> 1)) {
        vertical_distance = horizontal_distance >> 1;
    }
    alpha = vertical_distance < 0x80
        ? 0xff - vertical_distance * 2
        : 0;

    if (center_delta_y != 0) {
        x_over_y = (center_delta_x * 0x1000) / center_delta_y;
    }
    if ((clipcode & (LEFT | RIGHT)) != 0 && center_delta_y == 0) {
        x = center_delta_x > 0 ? MIN_X : MAX_X;
        pos[0] = (short)x;
        pos[1] = (short)y;
        return alpha;
    }
    if ((clipcode & (TOP | BOTTOM)) != 0 && center_delta_x == 0) {
        y = center_delta_y > 0 ? MIN_Y : MAX_Y;
        pos[0] = (short)x;
        pos[1] = (short)y;
        return alpha;
    }

    switch (clipcode) {
    case LEFT:
        y += jpb_whook_fixed_product(MIN_X - x, y_over_x);
        x = MIN_X;
        break;
    case RIGHT:
        y += jpb_whook_fixed_product(MAX_X - x, y_over_x);
        x = MAX_X;
        break;
    case TOP:
        y = MIN_Y;
        x = original_x +
            jpb_whook_fixed_product(MIN_Y - original_y, x_over_y);
        break;
    case LEFT | TOP:
        y += jpb_whook_fixed_product(MIN_X - x, y_over_x);
        x = MIN_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MIN_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MIN_Y - original_y, x_over_y);
        }
        break;
    case RIGHT | TOP:
        y += jpb_whook_fixed_product(MAX_X - x, y_over_x);
        x = MAX_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MIN_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MIN_Y - original_y, x_over_y);
        }
        break;
    case BOTTOM:
        y = MAX_Y;
        x = original_x +
            jpb_whook_fixed_product(MAX_Y - original_y, x_over_y);
        break;
    case LEFT | BOTTOM:
        y += jpb_whook_fixed_product(MIN_X - x, y_over_x);
        x = MIN_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MAX_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MAX_Y - original_y, x_over_y);
        }
        break;
    case RIGHT | BOTTOM:
        y += jpb_whook_fixed_product(MAX_X - x, y_over_x);
        x = MAX_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MAX_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MAX_Y - original_y, x_over_y);
        }
        break;
    default:
        break;
    }
    pos[0] = (short)x;
    pos[1] = (short)y;
    return alpha;
}
void debug_drawsphere(
    int x, int y, int z, int radius, uint32_t color)
{
    /*
     * The matched optimized body retains legacy camera/transform arithmetic
     * but emits no render primitive. Publish the exact authored arguments
     * through a portable renderer seam and otherwise preserve that inert
     * release behavior.
     */
    if (jpb_debug_sphere_hook != nullptr) {
        jpb_debug_sphere_hook(
            jpb_debug_sphere_user_data,
            x,
            y,
            z,
            radius,
            color);
    }
}

/* 0x127B10, 3 bytes, global, 5 named locals
 * debug_line2d
 * PDB type: void (int, int, int, int, unsign...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127B20, 18 bytes, global, 1 named locals
 * debug_printf
 * PDB type: int (char*, <no type>)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int debug_printf(char *format, ...)
{
    /*
     * The matched release executable's diagnostic entry point is
     * intentionally inert.
     */
    (void)format;
    return 0;
}

/* 0x127B40, 161 bytes, global, 2 named locals
 * debug_printf1
 * PDB type: int (char*, <no type>)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127BF0, 125 bytes, global, 10 named locals
 * debug_vectoroffset
 * PDB type: void (int, int, int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127C70, 21 bytes, global, 1 named locals
 * deserializeGameStruct
 * PDB type: void (unsigned char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127C90, 33 bytes, global, 2 named locals
 * deserializeOptionStruct
 * PDB type: void (unsigned char*, optionstru...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127CC0, 166 bytes, global, 6 named locals
 * frontEndPoly
 * PDB type: void (_Material*, int, FRONTENDV...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127D70, 165 bytes, global, 5 named locals
 * getDefaultResolutionIndex
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127E20, 3 bytes, global, 0 named locals
 * initXAstuff
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127E30, 1400 bytes, global, 2 named locals
 * platform_completeAchievement
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int platform_completeAchievement(int id)
{
    if (jpb_platform_achievement_hooks.complete == nullptr) {
        return 0;
    }
    return jpb_platform_achievement_hooks.complete(
        id, jpb_platform_achievement_user_data);
}

/* 0x1283B0, 14 bytes, global, 1 named locals
 * platform_getCompleteAchievement
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int platform_getCompleteAchievement(int id)
{
    if (jpb_platform_achievement_hooks.get_complete == nullptr) {
        return 0;
    }
    return jpb_platform_achievement_hooks.get_complete(
        id, jpb_platform_achievement_user_data);
}

/* 0x1283C0, 280 bytes, global, 3 named locals
 * platform_getSystemLanguage
 * PDB type: unsigned char ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1284E0, 84 bytes, global, 3 named locals
 * platform_openURL
 * PDB type: int (const char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x128540, 42 bytes, global, 0 named locals
 * platform_update
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x128570, 3 bytes, global, 2 named locals
 * seecull
 * PDB type: int (FVECTOR4*, FVECTOR4*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x128580, 60 bytes, global, 1 named locals
 * serializeGameStruct
 * PDB type: unsigned char* ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1285C0, 68 bytes, global, 2 named locals
 * serializeOptionsStruct
 * PDB type: unsigned char* (optionstruct*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x128610, 3 bytes, global, 0 named locals
 * texture_GarbageCollect
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x128620, 12 bytes, global, 0 named locals
 * whook_RestoreTextures
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x2719C0, 38 bytes, local, 1 named locals
 * `std::filesystem::_Convert_Source_to_wide<char [256],std::filesystem::_Normal_conversion>'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2719F0, 38 bytes, local, 1 named locals
 * `std::filesystem::_Convert_stringoid_to_wide<std::filesystem::_Normal_conversion>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271A20, 38 bytes, local, 1 named locals
 * `std::filesystem::_Convert_wide_to_narrow_replace_chars<std::char_traits<char>,std::allocator<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271A50, 40 bytes, local, 2 named locals
 * `std::vector<CD3DApplication::FBX_MESH *,std::allocator<CD3DApplication::FBX_MESH *> >::_Emplace_reallocate<CD3DApplication::FBX_MESH * const &>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271A80, 70 bytes, local, 2 named locals
 * `std::vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >::_Emplace_reallocate<CD3DApplication::SubMeshSet const &>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271AD0, 43 bytes, local, 1 named locals
 * `std::vector<Vertex,std::allocator<Vertex> >::_Emplace_reallocate<Vertex const &>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271B00, 40 bytes, local, 2 named locals
 * `std::vector<unsigned short,std::allocator<unsigned short> >::_Emplace_reallocate<unsigned short>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271B30, 12 bytes, local, 2 named locals
 * `std::_Lookup_equiv<char,std::regex_traits<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271B40, 12 bytes, local, 2 named locals
 * `std::_Lookup_equiv<char,std::regex_traits<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271B50, 41 bytes, local, 2 named locals
 * `std::_Lookup_equiv<char,std::regex_traits<char> >'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271B80, 12 bytes, local, 2 named locals
 * `std::_Lookup_equiv<char,std::regex_traits<char> >'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271B90, 12 bytes, local, 1 named locals
 * `std::_Regex_replace1<std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,char const *,std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BA0, 12 bytes, local, 1 named locals
 * `std::_Regex_search2<char const *,std::allocator<std::sub_match<char const *> >,char,std::regex_traits<char>,char const *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BB0, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BC0, 16 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BD0, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BE0, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BF0, 40 bytes, local, 1 named locals
 * `std::vector<unsigned int,std::allocator<unsigned int> >::_Resize_reallocate<unsigned int>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271C20, 40 bytes, local, 2 named locals
 * `std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Resize_reallocate<std::_Value_init_tag>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271C50, 40 bytes, local, 2 named locals
 * `std::vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >::_Resize_reallocate<std::_Value_init_tag>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271C80, 40 bytes, local, 2 named locals
 * `std::vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >::_Resize_reallocate<std::_Value_init_tag>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271CB0, 12 bytes, local, 1 named locals
 * `std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Try_emplace<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const &>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271CC0, 12 bytes, local, 1 named locals
 * `std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Try_emplace<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const &>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271CD0, 38 bytes, local, 1 named locals
 * `std::regex_replace<std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D00, 12 bytes, local, 1 named locals
 * `std::regex_replace<std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D10, 38 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<char *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D40, 12 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<char *>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D50, 38 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<char const *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D80, 12 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<char const *>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D90, 38 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271DC0, 12 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271DD0, 12 bytes, local, 0 named locals
 * `std::use_facet<std::collate<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271DE0, 12 bytes, local, 0 named locals
 * `std::use_facet<std::collate<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271DF0, 12 bytes, local, 1 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Matcher<char const *,char,std::regex_traits<char>,char const *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E00, 16 bytes, local, 1 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Matcher<char const *,char,std::regex_traits<char>,char const *>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E10, 19 bytes, local, 1 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Matcher<char const *,char,std::regex_traits<char>,char const *>'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E30, 16 bytes, local, 0 named locals
 * `std::_Parser<char const *,char,std::regex_traits<char> >::_Parser<char const *,char,std::regex_traits<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E40, 12 bytes, local, 0 named locals
 * `std::_Parser<char const *,char,std::regex_traits<char> >::_Parser<char const *,char,std::regex_traits<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E50, 16 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E60, 16 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E70, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E80, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E90, 16 bytes, local, 1 named locals
 * `CD3DApplication::SubMeshSet::SubMeshSet'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EA0, 12 bytes, local, 1 named locals
 * `el_chavo::el_chavo'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EB0, 12 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EC0, 16 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271ED0, 16 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EE0, 12 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EF0, 16 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F00, 16 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F10, 12 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F20, 12 bytes, local, 0 named locals
 * `std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::operator[]'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F30, 12 bytes, local, 0 named locals
 * `std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::operator[]'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F40, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F50, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F60, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F70, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16Depth'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F80, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16Depth'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F90, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16Depth'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FA0, 12 bytes, local, 2 named locals
 * `el_chavo::DrawUITextUTF16_3D'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FB0, 12 bytes, local, 2 named locals
 * `el_chavo::DrawUITextUTF16_3D'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FC0, 12 bytes, local, 0 named locals
 * `el_chavo::EndPoly'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FD0, 12 bytes, local, 0 named locals
 * `el_chavo::EndPoly'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FE0, 12 bytes, local, 0 named locals
 * `el_chavo::EndPoly'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FF0, 12 bytes, local, 0 named locals
 * `el_chavo::EndPoly'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272000, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272010, 47 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272040, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272050, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272060, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272070, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272080, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272090, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2720A0, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2720B0, 47 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$11
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2720E0, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$12
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2720F0, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$13
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272100, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$14
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272110, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$15
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272120, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$16
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272130, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$17
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272140, 12 bytes, local, 0 named locals
 * `el_chavo::NoScaleEndPoly'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272150, 12 bytes, local, 0 named locals
 * `el_chavo::NoScaleEndPoly'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272160, 12 bytes, local, 0 named locals
 * `el_chavo::NoScaleEndPoly'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272170, 12 bytes, local, 0 named locals
 * `el_chavo::NoScaleEndPoly'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272180, 12 bytes, local, 3 named locals
 * `std::_Builder<char const *,char,std::regex_traits<char> >::_Add_equiv'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272190, 12 bytes, local, 0 named locals
 * `std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_assert_group'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721A0, 12 bytes, local, 0 named locals
 * `std::_Parser<char const *,char,std::regex_traits<char> >::_Compile'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721B0, 12 bytes, local, 0 named locals
 * `std::_Parser<char const *,char,std::regex_traits<char> >::_Do_assert_group'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721C0, 12 bytes, local, 2 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721D0, 12 bytes, local, 2 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721E0, 12 bytes, local, 2 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721F0, 12 bytes, local, 2 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272200, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272210, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272220, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272230, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272240, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272250, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272260, 32 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272280, 41 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722B0, 12 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722C0, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722D0, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722E0, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722F0, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272300, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272310, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272320, 12 bytes, local, 1 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Match_pat'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272330, 38 bytes, local, 2 named locals
 * `std::filesystem::filesystem_error::_Pretty_message'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272360, 12 bytes, local, 2 named locals
 * `std::filesystem::filesystem_error::_Pretty_message'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272370, 12 bytes, local, 2 named locals
 * `std::filesystem::filesystem_error::_Pretty_message'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272380, 12 bytes, local, 0 named locals
 * `std::filesystem::_Throw_fs_error'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272390, 12 bytes, local, 0 named locals
 * `std::filesystem::_Throw_fs_error'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2723A0, 38 bytes, local, 0 named locals
 * `std::collate<char>::do_transform'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2723D0, 12 bytes, local, 2 named locals
 * `std::_System_error_category::message'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2723E0, 12 bytes, local, 4 named locals
 * `el_chavo::renderVideoFrame'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x279F30, 26 bytes, local, 0 named locals
 * `dynamic atexit destructor for 'chavo''
 * PDB type: void ()
 * Source: no line mapping
 */
