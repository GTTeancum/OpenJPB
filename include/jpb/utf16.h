#ifndef JPB_UTF16_H
#define JPB_UTF16_H

#include <stddef.h>
#include <stdint.h>

static inline size_t jpb_utf16_length(const uint16_t *text)
{
    const uint16_t *cursor = text;

    while (*cursor != 0) {
        ++cursor;
    }
    return (size_t)(cursor - text);
}

static inline void jpb_utf16_copy(
    uint16_t *destination, const uint16_t *source, size_t count)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        destination[index] = source[index];
    }
}

static inline void jpb_utf16_fill(
    uint16_t *destination, uint16_t value, size_t count)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        destination[index] = value;
    }
}

static inline int jpb_utf16_compare(
    const uint16_t *left, const uint16_t *right)
{
    while (*left != 0 && *left == *right) {
        ++left;
        ++right;
    }
    return (int)*left - (int)*right;
}

#endif
