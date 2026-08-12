/*
 * PARTIALLY REVIEWED RECONSTRUCTION.
 * PDB module: 0069
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\SpriteAtlasTextureDatabase.obj
 * Primary source: W:\SWJediPowerBattles\work\rendering\SpriteAtlasTextureDatabase.cpp
 * Compiler language: c++
 * Emitted procedures: 15
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/texture.h"

#include <algorithm>
#include <cctype>
#include <string>

/* 0x1280, 496 bytes, local, 6 named locals
 * `dynamic initializer for 'atlasTextures''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\rendering\SpriteAtlasTextureDatabase.cpp
 */

/* 0xEDDC0, 132 bytes, global, 7 named locals
 * std::_Destroy_range<std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
 * PDB type: void (std::basic_string<char,std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0xEDE50, 13 bytes, global, 1 named locals
 * std::_Tidy_guard<std::vector<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > > >::~_Tidy_guard<std::vector<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > > >
 * PDB type: void std::_Tidy_guard<std::vecto...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0xEDE60, 16 bytes, global, 1 named locals
 * std::_Uninitialized_backout_al<std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::~_Uninitialized_backout_al<std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
 * PDB type: void std::_Uninitialized_backout...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0xEDE70, 106 bytes, global, 5 named locals
 * std::vector<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::_Tidy
 * PDB type: void std::vector<std::basic_stri...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/*
 * Reference RVA 0xEDEE0. The five strings come from the matched executable's
 * atlasTextures dynamic initializer at RVA 0x1280. The retail body extracts
 * the basename at either slash style, lowercases it, then performs an exact
 * lookup in this list.
 */
int isTexturePartofAtlas(const std::string &textureName)
{
    static const char *const atlasTextures[] = {
        "_winif2.bmp",
        "winif2.tga",
        "winif2.png",
        "winif2_cleanup.png",
        "loadbar_gradient.png"
    };
    const std::string::size_type separator =
        textureName.find_last_of("/\\");
    std::string baseName = textureName.substr(
        separator == std::string::npos ? 0 : separator + 1);

    std::transform(
        baseName.begin(),
        baseName.end(),
        baseName.begin(),
        [](unsigned char value) {
            return static_cast<char>(std::tolower(value));
        });
    for (const char *candidate : atlasTextures) {
        if (baseName == candidate) {
            return 1;
        }
    }
    return 0;
}

extern "C" int jpb_TextureIsPartOfAtlas(const char *filename)
{
    return filename != nullptr &&
        isTexturePartofAtlas(std::string(filename));
}

/* 0x2705D0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'atlasTextures'''::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2705E0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'atlasTextures'''::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2705F0, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'atlasTextures'''::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270600, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'atlasTextures'''::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270610, 42 bytes, local, 0 named locals
 * ``dynamic initializer for 'atlasTextures'''::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270640, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'atlasTextures'''::`1'::dtor$16
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270650, 12 bytes, local, 0 named locals
 * ``dynamic initializer for 'atlasTextures'''::`1'::dtor$17
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270660, 12 bytes, local, 1 named locals
 * `isTexturePartofAtlas'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x279E00, 127 bytes, local, 4 named locals
 * `dynamic atexit destructor for 'atlasTextures''
 * PDB type: void ()
 * Source: no line mapping
 */
