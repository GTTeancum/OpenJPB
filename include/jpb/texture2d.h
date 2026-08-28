#ifndef JPB_TEXTURE2D_H
#define JPB_TEXTURE2D_H

#include <cstddef>
#include <cstdint>

class Texture;
class el_chavo;
class CD3DFramework12;
Texture *CreateNonTexturedTexture(int type);
Texture *CreateTextureFromFile(
    char *name,
    unsigned long stage,
    unsigned long flags,
    int flip,
    int desired_format,
    int type,
    CD3DFramework12 *framework);

namespace PHL {

enum class TextureFormat : unsigned {
    RGBA8888 = 0
};

class Texture2D {
public:
    virtual ~Texture2D();
    virtual void UpdateTexture(void *data, std::uint64_t size) = 0;
    virtual void *GetNativeResource() = 0;
    virtual void SetDebugName(const char *name) = 0;

    std::uint64_t GetWidth() const { return Width; }
    std::uint64_t GetHeight() const { return Height; }

    static Texture2D *CreateTexture(
        std::uint64_t width,
        std::uint64_t height,
        TextureFormat format);

protected:
    Texture2D(
        std::uint64_t width,
        std::uint64_t height,
        TextureFormat format)
        : m_type(0), Width(width), Height(height), Format(format)
    {
    }

private:
    friend class ::el_chavo;
    friend Texture *::CreateNonTexturedTexture(int type);
    friend Texture *::CreateTextureFromFile(
        char *, unsigned long, unsigned long, int, int, int,
        CD3DFramework12 *);

    unsigned m_type;
    std::uint64_t Width;
    std::uint64_t Height;
    TextureFormat Format;
};

static_assert(sizeof(Texture2D) == 40,
              "Texture2D PDB layout changed");

} // namespace PHL

#endif
