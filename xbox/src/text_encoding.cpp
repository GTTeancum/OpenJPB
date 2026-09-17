/* Exception-free UTF-8 conversion for nxdk; output uses the game's malloc ABI. */
#include <cstdlib>
#include <cstring>
#include <cstdint>

void jpb_XboxConvertToUTF16(const char *text, unsigned short **out)
{
    if (!out) return;
    *out = nullptr;
    if (!text) return;
    const size_t length = std::strlen(text);
    auto *result = static_cast<unsigned short *>(std::malloc((length + 1) * 2));
    if (!result) return;
    size_t src = 0, dst = 0;
    while (src < length) {
        const unsigned char first = static_cast<unsigned char>(text[src++]);
        uint32_t code = first;
        unsigned extra = 0;
        uint32_t minimum = 0;
        if (first >= 0xc2 && first <= 0xdf) { code &= 31; extra = 1; minimum = 0x80; }
        else if (first >= 0xe0 && first <= 0xef) { code &= 15; extra = 2; minimum = 0x800; }
        else if (first >= 0xf0 && first <= 0xf4) { code &= 7; extra = 3; minimum = 0x10000; }
        else if (first >= 0x80) { code = 0xfffd; }
        bool valid = src + extra <= length;
        for (unsigned i = 0; valid && i < extra; ++i) {
            unsigned char c = static_cast<unsigned char>(text[src + i]);
            if ((c & 0xc0) != 0x80) valid = false;
            else code = (code << 6) | (c & 63);
        }
        if (valid && extra) {
            src += extra;
            valid = code >= minimum && code <= 0x10ffff &&
                !(code >= 0xd800 && code <= 0xdfff);
        }
        if (!valid) code = 0xfffd;
        if (code > 0xffff) {
            code -= 0x10000;
            result[dst++] = static_cast<unsigned short>(0xd800 + (code >> 10));
            result[dst++] = static_cast<unsigned short>(0xdc00 + (code & 1023));
        } else result[dst++] = static_cast<unsigned short>(code);
    }
    result[dst] = 0;
    *out = result;
}
