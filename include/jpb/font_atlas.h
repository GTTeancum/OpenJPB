#ifndef JPB_FONT_ATLAS_H
#define JPB_FONT_ATLAS_H

#include "jpb/glyph_packing.h"
#include "jpb/sdl_abi.h"
#include "jpb/textutil.h"
#include "jpb/texture2d.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

struct JPBFontAtlasPlatformHooks {
    SDL_Surface *(*create_rgb_surface_with_format)(
        std::uint32_t flags,
        int width,
        int height,
        int depth,
        std::uint32_t format);
    void (*free_surface)(SDL_Surface *surface);
    int (*lock_surface)(SDL_Surface *surface);
    void (*unlock_surface)(SDL_Surface *surface);
    int (*upper_blit)(
        SDL_Surface *source,
        const SDL_Rect *source_rect,
        SDL_Surface *destination,
        SDL_Rect *destination_rect);
    const char *(*get_error)();
    int (*ttf_was_init)();
    int (*ttf_init)();
    _TTF_Font *(*ttf_open_font)(const char *file_name, int point_size);
    void (*ttf_close_font)(_TTF_Font *font);
    int (*ttf_set_font_size)(_TTF_Font *font, int point_size);
    int (*ttf_glyph_metrics)(
        _TTF_Font *font,
        unsigned short character,
        int *minimum_x,
        int *maximum_x,
        int *minimum_y,
        int *maximum_y,
        int *advance);
    SDL_Surface *(*ttf_render_glyph_blended)(
        _TTF_Font *font,
        unsigned short character,
        std::uint32_t color);
    int (*ttf_font_ascent)(_TTF_Font *font);
};

void jpb_FontAtlasSetPlatformHooks(
    const JPBFontAtlasPlatformHooks *hooks);

class SDL_Surface_Wrapper {
public:
    SDL_Surface_Wrapper();
    SDL_Surface_Wrapper(
        unsigned flags,
        int width,
        int height,
        int depth,
        unsigned format);
    explicit SDL_Surface_Wrapper(SDL_Surface *surface);
    SDL_Surface_Wrapper(const SDL_Surface_Wrapper &) = delete;
    SDL_Surface_Wrapper &operator=(const SDL_Surface_Wrapper &) = delete;
    SDL_Surface_Wrapper(SDL_Surface_Wrapper &&other) noexcept;
    SDL_Surface_Wrapper &operator=(SDL_Surface_Wrapper &&other) noexcept;
    ~SDL_Surface_Wrapper();

    SDL_Surface *GetSurface() const { return Surface; }

private:
    SDL_Surface *Surface;
};

class TTF_Font_Wrapper {
public:
    TTF_Font_Wrapper();
    explicit TTF_Font_Wrapper(const char *font_file);
    explicit TTF_Font_Wrapper(_TTF_Font *font);
    TTF_Font_Wrapper(const TTF_Font_Wrapper &) = delete;
    TTF_Font_Wrapper &operator=(const TTF_Font_Wrapper &) = delete;
    TTF_Font_Wrapper(TTF_Font_Wrapper &&other) noexcept;
    TTF_Font_Wrapper &operator=(TTF_Font_Wrapper &&other) noexcept;
    ~TTF_Font_Wrapper();

    _TTF_Font *GetFont() const { return Font; }

private:
    _TTF_Font *Font;
};

struct Glyph {
    std::string FontFile;
    unsigned short Character;
    int FontSize;
    GlyphMetrics Metrics;

    bool operator==(const Glyph &other) const;
};

namespace std {
template <>
struct hash<Glyph> {
    std::size_t operator()(const Glyph &glyph) const noexcept;
};
} // namespace std

class FontAtlas {
public:
    FontAtlas(unsigned format, unsigned width, unsigned height);
    ~FontAtlas();

    std::optional<std::pair<Glyph, GlyphEntry>> AddGlyph(
        const char *font_file,
        unsigned short character,
        int size);
    bool Contains(const Glyph &glyph) const;
    bool IsDirty() const { return bIsDirty; }
    void UpdateTexture();
    void ClearAtlas();
    SDL_Surface *GetAtlasSurface() const;
    unsigned GetWidth() const { return Width; }
    unsigned GetHeight() const { return Height; }
    _TTF_Font *GetFont(const std::string &font_file);
    std::optional<GlyphEntry> GetGlyphEntry(const Glyph &glyph) const;
    PHL::Texture2D *GetTexture() const { return Texture.get(); }

private:
    void RenderGlyph(const Glyph &glyph, const GlyphEntry &entry);

    SDL_Surface_Wrapper AtlasSurface;
    std::unique_ptr<GlyphPackingPolicy> PackingPolicy;
    std::unordered_map<std::string, TTF_Font_Wrapper> FontCache;
    std::unordered_map<Glyph, GlyphEntry> AtlasGlyphs;
    unsigned Width;
    unsigned Height;
    std::unique_ptr<PHL::Texture2D> Texture;
    bool bIsDirty;
};

static_assert(sizeof(SDL_Rect) == 16, "SDL_Rect PDB layout changed");
static_assert(sizeof(SDL_Surface) == 96,
              "SDL_Surface PDB layout changed");
static_assert(sizeof(SDL_Surface_Wrapper) == 8,
              "SDL_Surface_Wrapper PDB layout changed");
static_assert(sizeof(TTF_Font_Wrapper) == 8,
              "TTF_Font_Wrapper PDB layout changed");
#if defined(_MSC_VER) && _ITERATOR_DEBUG_LEVEL == 0
static_assert(offsetof(Glyph, Character) == 32,
              "Glyph.Character PDB offset changed");
static_assert(offsetof(Glyph, FontSize) == 36,
              "Glyph.FontSize PDB offset changed");
static_assert(offsetof(Glyph, Metrics) == 40,
              "Glyph.Metrics PDB offset changed");
static_assert(sizeof(Glyph) == 64, "Glyph PDB layout changed");
static_assert(sizeof(FontAtlas) == 168,
              "FontAtlas PDB layout changed");
#endif

#endif
