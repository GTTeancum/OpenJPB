/*
 * PARTIALLY REVIEWED RECONSTRUCTION
 * PDB module: 0071
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\TransparentTextureDatabase.obj
 * Primary source: W:\SWJediPowerBattles\work\rendering\TransparentTextureDatabase.cpp
 * Compiler language: c++
 * Emitted procedures: 278
 *
 * The two game-owned classifier bodies and their complete initializer data
 * are reviewed below. Standard-library implementation procedures remain as
 * inventory comments.
 */

#include "jpb/transparent_texture_database.h"

#include <cctype>
#include <cstddef>

namespace {

struct TextureNameSet {
    const char *const *names;
    std::size_t count;
};

#define TEXTURE_SET(array_name) \
    { array_name, sizeof(array_name) / sizeof(array_name[0]) }
#define EMPTY_TEXTURE_SET { nullptr, 0 }

/* Exact four-entry glassTextures initializer at RVA 0x1470. */
const char *const kGlassTextures[] = {
    "t_railing.tga",
    "t_orangeglass.tga",
    "t_orange.tga",
    "t_windows.tga"
};

/*
 * Exact unique membership of levelTextures[26], recovered from the
 * initializer at RVA 0x1640. Repeated initializer entries are intentionally
 * collapsed because the reference containers are std::set instances.
 */
const char *const kLevel1Textures[] = {
    "t_railing.tga", "t_o_floorbase.tga", "t_o_floor1.tga",
    "t_o_floor5.tga", "t_o_floor3b.tga", "t_o_floor6a.tga",
    "t_gtubes.tga", "t_o_floortile.tga", "o_floorbase.tga",
    "t_orangeglass.tga", "t_orange.tga"
};

const char *const kLevel2Textures[] = {
    "p_hang_vein.tga", "p_leaf10.tga", "p_grass3.tga",
    "p_fern06a.tga", "p_mush1.tga", "p_leaf3.tga",
    "p_fern02.tga", "p_leaf2.tga", "p_leaf5.tga",
    "p_leaf9.tga", "p_fern07a.tga", "p_fern08a.tga",
    "p_fern03.tga", "p_fern03a.tga", "p_fern05.tga",
    "p_fern01a.tga", "p_mush3aa.tga", "p_leaf6.tga",
    "p_leaf7.tga", "p_grass1.tga", "p_grass2.tga",
    "p_fern04.tga", "p_leaf4a.tga", "p_fern05a.tga",
    "p_leaf8.tga", "p_mush2.tga"
};

const char *const kLevel3Textures[] = {
    "theed0.tga", "theed1.tga", "theed2.tga", "theed3.tga",
    "p_arch2.tga", "p_arch2a.tga", "p_arch3.tga",
    "p_arch4.tga", "p_arch4a.tga", "p_arch3a.tga",
    "p_arch5.tga", "p_brchtop.tga", "p_brchfrnt.tga",
    "p_brchside.tga", "p_arch1.tga", "p_arch1b.tga",
    "p_arch1c.tga", "p_theed0_b.tga", "p_theed2_b.tga"
};

const char *const kLevel4Textures[] = {
    "leaf1.tga", "t_windows.tga", "p_tree.tga", "w_metal1.tga",
    "p_gridfloor.tga", "t_pal_16.tga", "p_tree_bottom.tga"
};

const char *const kCorusTextures[] = {
    "p_corus_window1.tga", "p_lights_closer.tga",
    "p_lights_large.tga", "p_small_lights.tga",
    "p_corus_26.tga", "p_corus_window2.tga", "t_windowtest.tga",
    "p_corus_window1.tgap_lights_closer.tgap_corus_26.tga"
    "p_corus_window2.tgap_small_lights.tgap_lights_large.tga"
    "t_windowtest.tga"
};

const char *const kLevel7Textures[] = {
    "p_leaf3.tga", "p_fern08a.tga", "p_fern06a.tga",
    "p_leaf2.tga", "p_leaf4a.tga", "p_leaf9.tga",
    "p_leaf8.tga", "p_leaf5.tga", "p_fern04.tga",
    "t_1moss.tga", "p_grass1.tga", "p_grass2.tga",
    "p_fern03.tga", "p_fern07a.tga", "t_dam_1wall.tga"
};

const char *const kLevel8Textures[] = {
    "p_streets0_b.tga", "p_brchtop.tga", "p_brchfrnt.tga",
    "p_brchside.tga", "t_water1a.tga", "p_streets2_b.tga",
    "p_streets3_b.tga"
};

const char *const kLevel9Textures[] = {
    "p_hang11.tga", "p_ivy.tga", "p_brchtop.tga",
    "p_brchfrnt.tga", "p_brchside.tga", "p_growthd.tga",
    "p_growthb.tga", "p_growtha.tga", "p_growthc.tga",
    "p_leaf1.tga", "t_water.tga", "t_waterStatic.tga"
};

const char *const kLevel10Textures[] = {
    "t_laser1.tga", "t_water.tga"
};

const char *const kLevel11Textures[] = {
    "t_orangefloor.tga", "t_floorctr.tga", "t_redfloor.tga",
    "t_bluefloor.tga", "pal_52.bmp", "leaf1.tga",
    "t_windows.tga", "t_floorctr1.tga"
};

const char *const kLevel13Textures[] = {
    "p_gnga0.tga", "p_gnga2.tga", "t_gngaglass.tga",
    "t_blueglass.tga", "t_orangeglass.tga", "t_yellowglass.tga"
};

const char *const kLevel16Textures[] = { "p_train4.tga" };
const char *const kLevel17Textures[] = { "p_train4.tga" };
const char *const kLevel21Textures[] = { "fade.bmpfade.bmp" };
const char *const kLevel22Textures[] = { "p_train2.tga" };
const char *const kLevel25Textures[] = {
    "t_o_floor6a.tga", "t_railing.tga", "fed2a.tga"
};

const TextureNameSet kLevelTextures[26] = {
    EMPTY_TEXTURE_SET,
    TEXTURE_SET(kLevel1Textures),
    TEXTURE_SET(kLevel2Textures),
    TEXTURE_SET(kLevel3Textures),
    TEXTURE_SET(kLevel4Textures),
    EMPTY_TEXTURE_SET,
    TEXTURE_SET(kCorusTextures),
    TEXTURE_SET(kLevel7Textures),
    TEXTURE_SET(kLevel8Textures),
    TEXTURE_SET(kLevel9Textures),
    TEXTURE_SET(kLevel10Textures),
    TEXTURE_SET(kLevel11Textures),
    EMPTY_TEXTURE_SET,
    TEXTURE_SET(kLevel13Textures),
    EMPTY_TEXTURE_SET,
    TEXTURE_SET(kCorusTextures),
    TEXTURE_SET(kLevel16Textures),
    TEXTURE_SET(kLevel17Textures),
    EMPTY_TEXTURE_SET,
    EMPTY_TEXTURE_SET,
    EMPTY_TEXTURE_SET,
    TEXTURE_SET(kLevel21Textures),
    TEXTURE_SET(kLevel22Textures),
    EMPTY_TEXTURE_SET,
    EMPTY_TEXTURE_SET,
    TEXTURE_SET(kLevel25Textures)
};

int equalFolded(const char *left, const char *right)
{
    if (left == nullptr || right == nullptr) {
        return 0;
    }
    while (*left != '\0' && *right != '\0') {
        const unsigned char leftCharacter =
            static_cast<unsigned char>(*left++);
        const unsigned char rightCharacter =
            static_cast<unsigned char>(*right++);
        if (std::tolower(leftCharacter) != std::tolower(rightCharacter)) {
            return 0;
        }
    }
    return *left == '\0' && *right == '\0';
}

int extensionEqualFolded(const char *left, const char *right)
{
    const char *leftDot = nullptr;
    const char *rightDot = nullptr;

    for (const char *cursor = left; cursor != nullptr && *cursor != '\0';
         ++cursor) {
        if (*cursor == '.') leftDot = cursor;
    }
    for (const char *cursor = right; cursor != nullptr && *cursor != '\0';
         ++cursor) {
        if (*cursor == '.') rightDot = cursor;
    }
    return leftDot != nullptr && rightDot != nullptr &&
           equalFolded(leftDot, rightDot);
}

int mirrorNameMatches(const char *mirrorName, const char *sourceName)
{
    std::size_t sourceStemLength = 0;
    std::size_t mirrorStemLength = 0;

    if (mirrorName == nullptr || sourceName == nullptr ||
        !extensionEqualFolded(mirrorName, sourceName)) {
        return 0;
    }
    if (equalFolded(mirrorName, sourceName)) {
        return 1;
    }
    while (sourceName[sourceStemLength] != '\0' &&
           sourceName[sourceStemLength] != '.') {
        ++sourceStemLength;
    }
    while (mirrorName[mirrorStemLength] != '\0' &&
           mirrorName[mirrorStemLength] != '.') {
        ++mirrorStemLength;
    }
    if (sourceStemLength <= 8) {
        return equalFolded(mirrorName, sourceName);
    }
    if (mirrorStemLength != 8) {
        return 0;
    }
    for (std::size_t index = 0; index < 7; ++index) {
        if (std::tolower(static_cast<unsigned char>(mirrorName[index])) !=
            std::tolower(static_cast<unsigned char>(sourceName[index]))) {
            return 0;
        }
    }
    return 1;
}

int nameInSet(const char *name, const TextureNameSet &set, int mirrorNames)
{
    for (std::size_t index = 0; index < set.count; ++index) {
        if ((mirrorNames && mirrorNameMatches(name, set.names[index])) ||
            (!mirrorNames && equalFolded(name, set.names[index]))) {
            return 1;
        }
    }
    return 0;
}

}  // namespace

extern "C" int jpb_IsGlassTexture(const char *texture_name)
{
    return nameInSet(
        texture_name, TEXTURE_SET(kGlassTextures), 0);
}

extern "C" int jpb_IsTextureTransparent(
    const char *texture_id, int level)
{
    if (level < 0 || level >= 26) {
        return 0;
    }
    return nameInSet(texture_id, kLevelTextures[level], 0);
}

extern "C" int jpb_IsGlassTextureForJpxMirror(
    const char *material_name)
{
    return nameInSet(
        material_name, TEXTURE_SET(kGlassTextures), 1);
}

extern "C" int jpb_IsTextureGlassForJpxMirror(
    const char *material_name, int level)
{
    if (level < 0 || level >= 26) {
        return 0;
    }
    for (std::size_t textureIndex = 0;
         textureIndex < kLevelTextures[level].count;
         ++textureIndex) {
        const char *sourceName =
            kLevelTextures[level].names[textureIndex];
        if (mirrorNameMatches(material_name, sourceName) &&
            nameInSet(sourceName, TEXTURE_SET(kGlassTextures), 0)) {
            return 1;
        }
    }
    return 0;
}

extern "C" int jpb_IsTextureTransparentForJpxMirror(
    const char *material_name, int level)
{
    if (level < 0 || level >= 26) {
        return 0;
    }
    return nameInSet(material_name, kLevelTextures[level], 1);
}

/* 0xEEB90, 270 bytes in the matched executable. */
bool isGlassTexture(const std::string &textureName)
{
    return jpb_IsGlassTexture(textureName.c_str()) != 0;
}

/* 0xEECA0, 242 bytes in the matched executable. */
bool isTextureTransparent(const std::string &textureID, int level)
{
    return jpb_IsTextureTransparent(textureID.c_str(), level) != 0;
}

#undef EMPTY_TEXTURE_SET
#undef TEXTURE_SET

/* 0x1470, 461 bytes, local, 5 named locals
 * `dynamic initializer for 'glassTextures''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\rendering\TransparentTextureDatabase.cpp
 */

/* 0x1640, 8446 bytes, local, 6 named locals
 * `dynamic initializer for 'levelTextures''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\rendering\TransparentTextureDatabase.cpp
 */

/* 0xEE190, 112 bytes, global, 7 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Allocate_for_capacity<0>
 * PDB type: char* std::basic_string<char,std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xEE200, 191 bytes, global, 8 named locals
 * std::_Tree_val<std::_Tree_simple_types<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::_Erase_tree<std::allocator<std::_Tree_node<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,void *> > >
 * PDB type: void std::_Tree_val<std::_Tree_s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xEE2C0, 826 bytes, global, 13 named locals
 * std::_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >::_Find_hint<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >
 * PDB type: std::_Tree_find_hint_result<std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xEE600, 195 bytes, global, 6 named locals
 * std::_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >::_Find_lower_bound<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >
 * PDB type: std::_Tree_find_result<std::_Tre...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xEE6D0, 55 bytes, global, 2 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::basic_string<char,std::char_traits<char>,std::allocator<char> >
 * PDB type: void std::basic_string<char,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0xEE710, 310 bytes, global, 6 named locals
 * std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
 * PDB type: void std::set<std::basic_string<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\set
 */

/* 0xEE850, 55 bytes, global, 2 named locals
 * std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
 * PDB type: void std::set<std::basic_string<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\set
 */

/* 0xEE890, 20 bytes, global, 2 named locals
 * std::_Alloc_construct_ptr<std::allocator<std::_Tree_node<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,void *> > >::~_Alloc_construct_ptr<std::allocator<std::_Tree_node<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,void *> > >
 * PDB type: void std::_Alloc_construct_ptr<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0xEE8B0, 42 bytes, global, 1 named locals
 * std::_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >::~_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >
 * PDB type: void std::_Tree<std::_Tset_trait...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xEE8E0, 20 bytes, global, 2 named locals
 * std::_Tree_temp_node_alloc<std::allocator<std::_Tree_node<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,void *> > >::~_Tree_temp_node_alloc<std::allocator<std::_Tree_node<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,void *> > >
 * PDB type: void std::_Tree_temp_node_alloc<...
 * Source: no line mapping
 */

/* 0xEE900, 42 bytes, global, 1 named locals
 * std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::~set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
 * PDB type: void std::set<std::basic_string<...
 * Source: no line mapping
 */

/* 0xEE930, 603 bytes, global, 13 named locals
 * std::_Tree_val<std::_Tree_simple_types<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::_Insert_node
 * PDB type: std::_Tree_node<std::basic_strin...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0xEEB90, 270 bytes, global, 8 named locals
 * isGlassTexture
 * PDB type: int (const std::basic_string<cha...
 * Source: W:\SWJediPowerBattles\work\rendering\TransparentTextureDatabase.cpp
 */

/* 0xEECA0, 242 bytes, global, 6 named locals
 * isTextureTransparent
 * PDB type: int (const std::basic_string<cha...
 * Source: W:\SWJediPowerBattles\work\rendering\TransparentTextureDatabase.cpp
 */

/* 0x270690, 12 bytes, local, 1 named locals
 * `std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2706A0, 12 bytes, local, 1 named locals
 * `std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2706B0, 12 bytes, local, 1 named locals
 * `std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2706C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'glassTextures'''::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2706D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'glassTextures'''::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2706E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'glassTextures'''::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2706F0, 42 bytes, local, 0 named locals
 * ``dynamic initializer for 'glassTextures'''::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270720, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'glassTextures'''::`1'::dtor$13
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270730, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'glassTextures'''::`1'::dtor$14
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270740, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270750, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270760, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270770, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270780, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270790, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2707A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2707B0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2707C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2707D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2707E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$10
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2707F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$11
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270800, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$12
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270810, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$13
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270820, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$14
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270830, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$15
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270840, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$16
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270850, 42 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$18
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270880, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$19
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270890, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$20
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2708A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$21
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2708B0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$22
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2708C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$23
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2708D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$24
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2708E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$25
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2708F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$26
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270900, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$27
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270910, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$28
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270920, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$29
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270930, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$30
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270940, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$31
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270950, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$32
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270960, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$33
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270970, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$34
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270980, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$35
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270990, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$36
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2709A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$37
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2709B0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$38
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2709C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$39
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2709D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$40
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2709E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$41
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2709F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$42
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270A00, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$43
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270A10, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$44
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270A20, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$46
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270A50, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$47
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270A60, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$48
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270A70, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$49
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270A80, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$50
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270A90, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$51
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270AA0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$52
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270AB0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$53
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270AC0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$54
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270AD0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$55
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270AE0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$56
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270AF0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$57
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270B00, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$58
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270B10, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$59
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270B20, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$60
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270B30, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$61
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270B40, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$62
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270B50, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$63
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270B60, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$64
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270B70, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$65
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270B80, 42 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$67
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270BB0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$68
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270BC0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$69
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270BD0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$70
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270BE0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$71
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270BF0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$72
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270C00, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$73
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270C10, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$74
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270C20, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$75
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270C30, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$77
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270C60, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$78
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270C70, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$79
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270C80, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$80
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270C90, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$81
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270CA0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$82
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270CB0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$83
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270CC0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$84
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270CD0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$85
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270CE0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$86
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270CF0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$87
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D00, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$88
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D10, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$89
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D20, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$90
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D30, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$91
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D40, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$92
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D50, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$93
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D60, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$94
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D70, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$95
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D80, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$96
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270D90, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$97
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270DA0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$98
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270DB0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$99
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270DC0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$100
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270DD0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$101
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270DE0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$102
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270DF0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$103
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270E00, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$104
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270E10, 42 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$106
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270E40, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$107
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270E50, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$108
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270E60, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$109
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270E70, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$110
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270E80, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$111
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270E90, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$112
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270EA0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$113
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270EB0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$114
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270EC0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$115
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270ED0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$116
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270EE0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$117
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270EF0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$118
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F00, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$119
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F10, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$120
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F20, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$121
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F30, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$122
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F40, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$123
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F50, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$124
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F60, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$125
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F70, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$126
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F80, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$127
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270F90, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$128
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270FA0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$129
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270FB0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$130
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270FC0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$131
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270FD0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$132
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270FE0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$133
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270FF0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$134
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271000, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$135
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271010, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$136
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271020, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$137
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271030, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$138
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271040, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$139
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271050, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$140
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271060, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$141
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271070, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$142
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271080, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$143
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271090, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$144
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2710A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$145
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2710B0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$146
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2710C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$147
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2710D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$148
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2710E0, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$150
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271110, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$151
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271120, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$152
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271130, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$153
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271140, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$154
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271150, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$155
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271160, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$156
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271170, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$157
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271180, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$158
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271190, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$159
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2711A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$160
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2711B0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$161
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2711C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$162
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2711D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$163
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2711E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$164
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2711F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$165
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271200, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$166
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271210, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$167
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271220, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$168
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271230, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$169
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271240, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$171
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271270, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$172
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271280, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$173
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271290, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$174
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2712A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$175
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2712B0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$176
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2712C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$177
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2712D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$178
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2712E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$179
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2712F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$180
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271300, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$181
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271310, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$182
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271320, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$183
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271330, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$184
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271340, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$185
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271350, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$186
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271360, 42 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$188
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271390, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$189
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2713A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$190
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2713B0, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$192
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2713E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$193
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2713F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$194
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271400, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$195
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271410, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$196
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271420, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$197
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271430, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$198
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271440, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$199
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271450, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$200
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271460, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$201
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271470, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$202
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271480, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$203
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271490, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$204
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2714A0, 42 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$206
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2714D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$207
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2714E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$208
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2714F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$209
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271500, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$210
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271510, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$211
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271520, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$212
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271530, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$213
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271540, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$215
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271570, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$216
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271580, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$217
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271590, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$218
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2715A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$219
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2715B0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$220
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2715C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$221
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2715D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$222
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2715E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$223
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2715F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$224
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271600, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$225
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271610, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$226
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271620, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$227
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271630, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$228
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271640, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$229
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271650, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$230
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271660, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$231
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271670, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$232
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271680, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$233
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271690, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$234
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2716A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$235
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2716B0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$236
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2716C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$237
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2716D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$238
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2716E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$239
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2716F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$240
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271700, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$241
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271710, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$242
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271720, 42 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$244
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271750, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$245
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271760, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$247
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271790, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$248
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2717A0, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$250
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2717D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$251
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2717E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$252
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2717F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$253
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271800, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$254
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271810, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$256
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271840, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$257
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271850, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$259
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271880, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$260
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271890, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$261
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2718A0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$262
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2718B0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$263
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2718C0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$264
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2718D0, 45 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$266
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271900, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'levelTextures'''::`1'::dtor$267
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x279E80, 127 bytes, local, 4 named locals
 * `dynamic atexit destructor for 'glassTextures''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x279F00, 28 bytes, local, 0 named locals
 * `dynamic atexit destructor for 'levelTextures''
 * PDB type: void ()
 * Source: no line mapping
 */
