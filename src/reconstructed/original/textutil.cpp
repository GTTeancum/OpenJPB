/* COMPLETE REVIEWED RECONSTRUCTION. */

#include "jpb/textutil.h"

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <functional>
#include <locale>
#include <tuple>
#include <unordered_map>
#include <utility>

struct pair_hash
{
    std::size_t operator()(const std::pair<int, unsigned short> &value) const noexcept
    {
        std::size_t seed = std::hash<unsigned short>{}(value.second);
        seed ^= std::hash<int>{}(value.first) + 0x9e3779b9U +
                (seed << 6U) + (seed >> 2U);
        return seed;
    }
};

/*
 * GENERATED RECONSTRUCTION SHELL - no function bodies recovered here.
 * PDB module: 0086
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\textutil.obj
 * Primary source: W:\SWJediPowerBattles\work\textutil.cpp
 * Compiler language: c++
 * Emitted procedures: 25
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

/* 0x3740, 119 bytes, local, 1 named locals
 * `dynamic initializer for 'glyphMetricsCache''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\textutil.cpp
 */

/* 0x102170, 32 bytes, global, 3 named locals
 * std::_Fnv1a_append_value<unsigned short>
 * PDB type: unsigned __int64 (const unsigned...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\type_traits
 */

/* 0x102190, 42 bytes, global, 2 named locals
 * std::_Hash_representation<unsigned short>
 * PDB type: unsigned __int64 (const unsigned...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\type_traits
 */

/* 0x1021C0, 795 bytes, global, 23 named locals
 * std::_Hash<std::_Umap_traits<std::pair<int,unsigned short>,std::tuple<int,int,int,int,int>,std::_Uhash_compare<std::pair<int,unsigned short>,pair_hash,std::equal_to<std::pair<int,unsigned short> > >,std::allocator<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > >,0> >::_Try_emplace<std::pair<int,unsigned short> >
 * PDB type: std::pair<std::_List_node<std::p...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x1024E0, 132 bytes, global, 4 named locals
 * std::fill<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > > > > *,std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > > > > >
 * PDB type: void (std::_List_unchecked_itera...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x102570, 32 bytes, global, 4 named locals
 * std::uninitialized_fill<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > > > > *,std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > > > > >
 * PDB type: void (std::_List_unchecked_itera...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x102590, 20 bytes, global, 2 named locals
 * std::_Alloc_construct_ptr<std::allocator<std::_List_node<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> >,void *> > >::~_Alloc_construct_ptr<std::allocator<std::_List_node<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> >,void *> > >
 * PDB type: void std::_Alloc_construct_ptr<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x1025B0, 163 bytes, global, 8 named locals
 * std::_Hash<std::_Umap_traits<std::pair<int,unsigned short>,std::tuple<int,int,int,int,int>,std::_Uhash_compare<std::pair<int,unsigned short>,pair_hash,std::equal_to<std::pair<int,unsigned short> > >,std::allocator<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > >,0> >::~_Hash<std::_Umap_traits<std::pair<int,unsigned short>,std::tuple<int,int,int,int,int>,std::_Uhash_compare<std::pair<int,unsigned short>,pair_hash,std::equal_to<std::pair<int,unsigned short> > >,std::allocator<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > >,0> >
 * PDB type: void std::_Hash<std::_Umap_trait...
 * Source: no line mapping
 */

/* 0x102660, 91 bytes, global, 5 named locals
 * std::_Hash_vec<std::allocator<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > > > > > >::~_Hash_vec<std::allocator<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > > > > > >
 * PDB type: void std::_Hash_vec<std::allocat...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x1026C0, 20 bytes, global, 2 named locals
 * std::_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> >,void *> > >::~_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> >,void *> > >
 * PDB type: void std::_List_node_emplace_op2...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\list
 */

/* 0x1026E0, 92 bytes, global, 4 named locals
 * std::list<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> >,std::allocator<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > > >::~list<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> >,std::allocator<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > > >
 * PDB type: void std::list<std::pair<std::pa...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\list
 */

/* 0x102740, 314 bytes, global, 15 named locals
 * std::_Hash_vec<std::allocator<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > > > > > >::_Assign_grow
 * PDB type: void std::_Hash_vec<std::allocat...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x102880, 545 bytes, global, 23 named locals
 * std::_Hash<std::_Umap_traits<std::pair<int,unsigned short>,std::tuple<int,int,int,int,int>,std::_Uhash_compare<std::pair<int,unsigned short>,pair_hash,std::equal_to<std::pair<int,unsigned short> > >,std::allocator<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > >,0> >::_Forced_rehash
 * PDB type: void std::_Hash<std::_Umap_trait...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x102AB0, 130 bytes, global, 3 named locals
 * std::_Hash<std::_Umap_traits<std::pair<int,unsigned short>,std::tuple<int,int,int,int,int>,std::_Uhash_compare<std::pair<int,unsigned short>,pair_hash,std::equal_to<std::pair<int,unsigned short> > >,std::allocator<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > >,0> >::clear
 * PDB type: void std::_Hash<std::_Umap_trait...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x102B40, 161 bytes, global, 5 named locals
 * toLower
 * PDB type: std::basic_string<char,std::char...
 * Source: W:\SWJediPowerBattles\work\textutil.cpp
 */
std::unordered_map<
    std::pair<int, unsigned short>,
    std::tuple<int, int, int, int, int>,
    pair_hash>
    glyphMetricsCache = {};
static JPBTextUtilSetFontSizeHook jpb_set_font_size_hook;
static JPBTextUtilGlyphMetricsHook jpb_glyph_metrics_hook;
static JPBTextUtilGetErrorHook jpb_get_error_hook;

void jpb_TextUtilSetFontMetricsHooks(
    JPBTextUtilSetFontSizeHook set_font_size_hook,
    JPBTextUtilGlyphMetricsHook glyph_metrics_hook,
    JPBTextUtilGetErrorHook get_error_hook)
{
    jpb_set_font_size_hook = set_font_size_hook;
    jpb_glyph_metrics_hook = glyph_metrics_hook;
    jpb_get_error_hook = get_error_hook;
}

std::string toLower(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

/* 0x102BF0, 12 bytes, global, 0 named locals
 * ClearGlyphCache
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\textutil.cpp
 */
void ClearGlyphCache(void)
{
    glyphMetricsCache.clear();
}

/* 0x102C00, 428 bytes, global, 13 named locals
 * ConvertToUTF16
 * PDB type: void (const char*, unsigned shor...
 * Source: W:\SWJediPowerBattles\work\textutil.cpp
 */
void ConvertToUTF16(const char *string, unsigned short **utfString)
{
    try
    {
        std::wstring_convert<
            std::codecvt<char16_t, char, std::mbstate_t>,
            char16_t>
            converter;
        const std::size_t sourceLength = std::strlen(string);
        const std::u16string convertedString =
            converter.from_bytes(string, string + sourceLength);
        const int size = static_cast<int>(convertedString.size());

        *utfString = static_cast<unsigned short *>(
            std::malloc(static_cast<std::size_t>(size * 2 + 2)));
        std::memcpy(*utfString, convertedString.c_str(), size * 2 + 2);
        (*utfString)[size] = 0;
    }
    catch (...)
    {
        if (*utfString != nullptr)
        {
            std::free(*utfString);
            *utfString = nullptr;
        }

        *utfString = static_cast<unsigned short *>(std::malloc(6));
        std::memcpy(*utfString, "ERROR", 6);
    }
}

/* 0x102DB0, 864 bytes, global, 27 named locals
 * SizeText
 * PDB type: void (_TTF_Font*, int, const uns...
 * Source: W:\SWJediPowerBattles\work\textutil.cpp
 */
void SizeText(
    _TTF_Font *font,
    int point_size,
    const unsigned short *string,
    int *width,
    int *height)
{
    const unsigned short *end;
    const unsigned short *cursor;
    int line_width = 0;
    int line_minimum_y = 0;
    int line_maximum_y = 0;

    if (font == nullptr || string == nullptr ||
        width == nullptr || height == nullptr) {
        std::printf(
            "[Font Atlas][Error] Invalid parameters, got urslef a nullpointer\n");
        return;
    }

    if (jpb_set_font_size_hook == nullptr ||
        jpb_glyph_metrics_hook == nullptr ||
        jpb_get_error_hook == nullptr) {
        std::abort();
    }

    *width = 0;
    *height = 0;
    if (jpb_set_font_size_hook(font, point_size) != 0) {
        std::printf("[Font Atlas][Error] %s", jpb_get_error_hook());
    }

    end = string;
    while (*end != 0) {
        ++end;
    }
    for (cursor = string; cursor != end; ++cursor) {
        const unsigned short glyph = *cursor;

        if (glyph == static_cast<unsigned short>('\n')) {
            *height += line_maximum_y - line_minimum_y;
            line_width = 0;
            line_minimum_y = 0;
            line_maximum_y = 0;
            continue;
        }

        const std::pair<int, unsigned short> key(
            static_cast<int>(glyph),
            static_cast<unsigned short>(point_size));
        auto found = glyphMetricsCache.find(key);
        int minimum_x;
        int maximum_x;
        int minimum_y;
        int maximum_y;
        int advance;

        if (found == glyphMetricsCache.end()) {
            if (jpb_glyph_metrics_hook(
                    font,
                    glyph,
                    &minimum_x,
                    &maximum_x,
                    &minimum_y,
                    &maximum_y,
                    &advance) != 0) {
                std::printf(
                    "[Font Atlas][Error] %s\n", jpb_get_error_hook());
                continue;
            }
            glyphMetricsCache.emplace(
                key,
                std::make_tuple(
                    minimum_x,
                    maximum_x,
                    minimum_y,
                    maximum_y,
                    advance));
        } else {
            std::tie(
                minimum_x,
                maximum_x,
                minimum_y,
                maximum_y,
                advance) = found->second;
        }

        line_width += cursor == end - 1
            ? maximum_x - minimum_x
            : advance - minimum_x;
        line_maximum_y = std::max(line_maximum_y, maximum_y);
        line_minimum_y = std::min(line_minimum_y, minimum_y);
    }

    *height += line_maximum_y - line_minimum_y;
    *width = std::max(*width, line_width);
}

/* 0x271910, 12 bytes, local, 0 named locals
 * `std::_Hash<std::_Umap_traits<std::pair<int,unsigned short>,std::tuple<int,int,int,int,int>,std::_Uhash_compare<std::pair<int,unsigned short>,pair_hash,std::equal_to<std::pair<int,unsigned short> > >,std::allocator<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > >,0> >::_Try_emplace<std::pair<int,unsigned short> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271920, 12 bytes, local, 0 named locals
 * `std::_Hash<std::_Umap_traits<std::pair<int,unsigned short>,std::tuple<int,int,int,int,int>,std::_Uhash_compare<std::pair<int,unsigned short>,pair_hash,std::equal_to<std::pair<int,unsigned short> > >,std::allocator<std::pair<std::pair<int,unsigned short> const ,std::tuple<int,int,int,int,int> > >,0> >::_Try_emplace<std::pair<int,unsigned short> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271930, 16 bytes, local, 0 named locals
 * ``dynamic initializer for 'glyphMetricsCache'''::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271940, 16 bytes, local, 0 named locals
 * ``dynamic initializer for 'glyphMetricsCache'''::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271950, 12 bytes, local, 2 named locals
 * ConvertToUTF16$dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271960, 89 bytes, local, 3 named locals
 * ConvertToUTF16$catch$6
 * PDB type: unknown
 * Source: W:\SWJediPowerBattles\work\textutil.cpp
 */

/* 0x279F20, 12 bytes, local, 0 named locals
 * `dynamic atexit destructor for 'glyphMetricsCache''
 * PDB type: void ()
 * Source: no line mapping
 */
