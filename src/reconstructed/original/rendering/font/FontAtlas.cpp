/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\rendering\font\FontAtlas.cpp.
 * PDB module: 0067
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\FontAtlas.obj
 */

#include "jpb/font_atlas.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static JPBFontAtlasPlatformHooks jpb_font_atlas_hooks;

static const JPBFontAtlasPlatformHooks &font_atlas_hooks()
{
    if (jpb_font_atlas_hooks.create_rgb_surface_with_format == nullptr ||
        jpb_font_atlas_hooks.free_surface == nullptr ||
        jpb_font_atlas_hooks.lock_surface == nullptr ||
        jpb_font_atlas_hooks.unlock_surface == nullptr ||
        jpb_font_atlas_hooks.upper_blit == nullptr ||
        jpb_font_atlas_hooks.get_error == nullptr ||
        jpb_font_atlas_hooks.ttf_was_init == nullptr ||
        jpb_font_atlas_hooks.ttf_init == nullptr ||
        jpb_font_atlas_hooks.ttf_open_font == nullptr ||
        jpb_font_atlas_hooks.ttf_close_font == nullptr ||
        jpb_font_atlas_hooks.ttf_set_font_size == nullptr ||
        jpb_font_atlas_hooks.ttf_glyph_metrics == nullptr ||
        jpb_font_atlas_hooks.ttf_render_glyph_blended == nullptr ||
        jpb_font_atlas_hooks.ttf_font_ascent == nullptr) {
        std::abort();
    }
    return jpb_font_atlas_hooks;
}

void jpb_FontAtlasSetPlatformHooks(
    const JPBFontAtlasPlatformHooks *hooks)
{
    if (hooks == nullptr) {
        std::memset(&jpb_font_atlas_hooks, 0, sizeof(jpb_font_atlas_hooks));
    } else {
        jpb_font_atlas_hooks = *hooks;
    }
}

SDL_Surface_Wrapper::SDL_Surface_Wrapper()
    : Surface(nullptr)
{
}

/* RVA 0xEC400 */
SDL_Surface_Wrapper::SDL_Surface_Wrapper(
    unsigned flags,
    int width,
    int height,
    int depth,
    unsigned format)
    : Surface(font_atlas_hooks().create_rgb_surface_with_format(
          flags, width, height, depth, format))
{
}

/* RVA 0xEC450 */
SDL_Surface_Wrapper::SDL_Surface_Wrapper(SDL_Surface *surface)
    : Surface(surface)
{
}

SDL_Surface_Wrapper::SDL_Surface_Wrapper(
    SDL_Surface_Wrapper &&other) noexcept
    : Surface(other.Surface)
{
    other.Surface = nullptr;
}

SDL_Surface_Wrapper &SDL_Surface_Wrapper::operator=(
    SDL_Surface_Wrapper &&other) noexcept
{
    if (this != &other) {
        font_atlas_hooks().free_surface(Surface);
        Surface = other.Surface;
        other.Surface = nullptr;
    }
    return *this;
}

/* RVA 0xEC9F0 */
SDL_Surface_Wrapper::~SDL_Surface_Wrapper()
{
    font_atlas_hooks().free_surface(Surface);
}

TTF_Font_Wrapper::TTF_Font_Wrapper()
    : Font(nullptr)
{
}

/* RVA 0xEC470 */
TTF_Font_Wrapper::TTF_Font_Wrapper(const char *font_file)
    : Font(font_atlas_hooks().ttf_open_font(font_file, 0))
{
    if (Font == nullptr) {
        std::printf("[Error] SDL TTF %s", font_atlas_hooks().get_error());
    }
}

/* RVA 0xEC460 */
TTF_Font_Wrapper::TTF_Font_Wrapper(_TTF_Font *font)
    : Font(font)
{
}

TTF_Font_Wrapper::TTF_Font_Wrapper(TTF_Font_Wrapper &&other) noexcept
    : Font(other.Font)
{
    other.Font = nullptr;
}

TTF_Font_Wrapper &TTF_Font_Wrapper::operator=(
    TTF_Font_Wrapper &&other) noexcept
{
    if (this != &other) {
        if (Font != nullptr) {
            font_atlas_hooks().ttf_close_font(Font);
        }
        Font = other.Font;
        other.Font = nullptr;
    }
    return *this;
}

/* RVA 0xECA00 */
TTF_Font_Wrapper::~TTF_Font_Wrapper()
{
    if (Font != nullptr) {
        font_atlas_hooks().ttf_close_font(Font);
    }
}

bool Glyph::operator==(const Glyph &other) const
{
    return FontFile == other.FontFile &&
           Character == other.Character &&
           FontSize == other.FontSize;
}

std::size_t std::hash<Glyph>::operator()(const Glyph &glyph) const noexcept
{
    constexpr std::size_t offset = UINT64_C(0xcbf29ce484222325);
    constexpr std::size_t prime = UINT64_C(0x100000001b3);
    std::size_t font_hash = offset;
    std::size_t size_hash = offset;
    const auto *size_bytes = reinterpret_cast<const unsigned char *>(
        &glyph.FontSize);

    for (unsigned char byte : glyph.FontFile) {
        font_hash = (font_hash ^ byte) * prime;
    }
    for (std::size_t index = 0; index < sizeof(glyph.FontSize); ++index) {
        size_hash = (size_hash ^ size_bytes[index]) * prime;
    }
    return font_hash ^ size_hash ^ glyph.Character;
}

/* RVA 0xEC240 */
FontAtlas::FontAtlas(unsigned format, unsigned width, unsigned height)
    : AtlasSurface(0, width, height, 32, format),
      PackingPolicy(),
      FontCache(),
      AtlasGlyphs(),
      Width(width),
      Height(height),
      Texture(PHL::Texture2D::CreateTexture(
          width, height, PHL::TextureFormat::RGBA8888)),
      bIsDirty(false)
{
    const JPBFontAtlasPlatformHooks &hooks = font_atlas_hooks();

    if (hooks.ttf_was_init() == 0 && hooks.ttf_init() != 0) {
        std::printf("SDL_ttf init Error %s", hooks.get_error());
    }
    PackingPolicy = std::make_unique<ShelfGlyphPackingPolicy>(width, height);
    Texture->SetDebugName("FontAtlas");
}

FontAtlas::~FontAtlas() = default;

/* RVA 0xECA10 */
std::optional<std::pair<Glyph, GlyphEntry>> FontAtlas::AddGlyph(
    const char *font_file,
    unsigned short character,
    int size)
{
    Glyph new_glyph = {font_file, character, size, {0, 0, 0, 0, 0}};
    auto found = AtlasGlyphs.find(new_glyph);

    if (found != AtlasGlyphs.end()) {
        return std::make_pair(found->first, found->second);
    }

    _TTF_Font *font = GetFont(std::string(font_file));
    const JPBFontAtlasPlatformHooks &hooks = font_atlas_hooks();
    if (hooks.ttf_set_font_size(font, size) != 0) {
        std::printf("[Font Atlas][Error] %s", hooks.get_error());
    }
    if (hooks.ttf_glyph_metrics(
            font,
            character,
            &new_glyph.Metrics.minx,
            &new_glyph.Metrics.maxx,
            &new_glyph.Metrics.miny,
            &new_glyph.Metrics.maxy,
            &new_glyph.Metrics.advance) != 0) {
        std::printf("[Font Atlas][Error] %s", hooks.get_error());
    }
    if (character == 0x2019U) {
        new_glyph.Metrics.advance = 10;
    } else if (character == 0x201DU) {
        new_glyph.Metrics.advance = 18;
    }

    std::optional<GlyphEntry> entry =
        PackingPolicy->AddGlyphEntry(&new_glyph.Metrics);
    if (!entry.has_value()) {
        return std::nullopt;
    }

    auto inserted = AtlasGlyphs.emplace(new_glyph, entry.value());
    std::optional<std::pair<Glyph, GlyphEntry>> result =
        std::make_pair(inserted.first->first, inserted.first->second);
    RenderGlyph(result.value().first, result.value().second);
    return result;
}

/* RVA 0xED0F0: shipped no-op. */
void FontAtlas::ClearAtlas()
{
}

/* RVA 0xED100 */
bool FontAtlas::Contains(const Glyph &glyph) const
{
    return AtlasGlyphs.find(glyph) != AtlasGlyphs.end();
}

/* RVA 0xED160 */
SDL_Surface *FontAtlas::GetAtlasSurface() const
{
    return AtlasSurface.GetSurface();
}

/* RVA 0xED170 */
_TTF_Font *FontAtlas::GetFont(const std::string &font_file)
{
    return FontCache.try_emplace(font_file, font_file.c_str())
        .first->second.GetFont();
}

/* RVA 0xED1B0 */
std::optional<GlyphEntry> FontAtlas::GetGlyphEntry(
    const Glyph &glyph) const
{
    std::optional<GlyphEntry> result;
    auto found = AtlasGlyphs.find(glyph);
    if (found != AtlasGlyphs.end()) {
        result = found->second;
    }
    return result;
}

/* RVA 0xED230 */
void FontAtlas::RenderGlyph(
    const Glyph &glyph,
    const GlyphEntry &entry)
{
    const JPBFontAtlasPlatformHooks &hooks = font_atlas_hooks();
    _TTF_Font *font = GetFont(glyph.FontFile);

    if (hooks.ttf_set_font_size(font, glyph.FontSize) != 0) {
        std::printf("[Font Atlas][Error] %s", hooks.get_error());
    }
    SDL_Surface *surface = hooks.ttf_render_glyph_blended(
        font, glyph.Character, UINT32_C(0xffffffff));
    SDL_Rect destination = {
        static_cast<int>(entry.Left),
        static_cast<int>(entry.Top),
        static_cast<int>(entry.Right - entry.Left),
        static_cast<int>(entry.Bottom - entry.Top)};
    SDL_Rect source = {
        std::max(glyph.Metrics.minx, 0),
        hooks.ttf_font_ascent(font) - glyph.Metrics.maxy,
        glyph.Metrics.maxx - glyph.Metrics.minx,
        glyph.Metrics.maxy - glyph.Metrics.miny};

    if (hooks.upper_blit(
            surface, &source, AtlasSurface.GetSurface(), &destination) != 0) {
        std::printf("[Font Atlas][Error] %s", hooks.get_error());
    }
    bIsDirty = true;
    hooks.free_surface(surface);
}

/* RVA 0xED380 */
void FontAtlas::UpdateTexture()
{
    if (Texture) {
        SDL_Surface *surface = AtlasSurface.GetSurface();
        if (surface != nullptr) {
            const JPBFontAtlasPlatformHooks &hooks = font_atlas_hooks();
            if (hooks.lock_surface(surface) != 0) {
                std::printf("[Font Atlas][Error] %s", hooks.get_error());
            }
            const std::uint64_t size = static_cast<std::uint64_t>(
                static_cast<std::int64_t>(surface->pitch * surface->h));
            Texture->UpdateTexture(surface->pixels, size);
            hooks.unlock_surface(surface);
            bIsDirty = false;
        }
    }
}
