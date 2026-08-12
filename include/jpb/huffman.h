#ifndef JPB_HUFFMAN_H
#define JPB_HUFFMAN_H

#include "jpb/unpack.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fixed sizes observed in the matched installation's three companion files.
 * The original anim_GlobalInit loads these into static buffers, then passes
 * them to unpack_init in tab/val/opt order.
 */
enum {
    JPB_HUFFMAN_OPTION_COUNT = 256,
    JPB_HUFFMAN_VALUE_COUNT = 115,
    JPB_HUFFMAN_TREE_COUNT = 2038,
    JPB_HUFFMAN_OPTION_BYTES =
        JPB_HUFFMAN_OPTION_COUNT * sizeof(_optab),
    JPB_HUFFMAN_VALUE_BYTES =
        JPB_HUFFMAN_VALUE_COUNT * sizeof(uint16_t),
    JPB_HUFFMAN_TREE_BYTES =
        JPB_HUFFMAN_TREE_COUNT * sizeof(uint32_t)
};

typedef enum JPBHuffmanResult {
    JPB_HUFFMAN_OK = 0,
    JPB_HUFFMAN_INVALID_ARGUMENT = -1,
    JPB_HUFFMAN_INVALID_SIZE = -2,
    JPB_HUFFMAN_IO_ERROR = -3,
    JPB_HUFFMAN_INVALID_TABLE = -4
} JPBHuffmanResult;

typedef struct JPBHuffmanTableSet {
    _optab options[JPB_HUFFMAN_OPTION_COUNT];
    uint16_t values[JPB_HUFFMAN_VALUE_COUNT];
    uint32_t tree[JPB_HUFFMAN_TREE_COUNT];
} JPBHuffmanTableSet;

/*
 * Portable host seam; these jpb_ names are descriptive additions, not
 * original PDB symbols. Paths correspond to huffman.tab, huffman.val, and
 * huffman.opt respectively.
 */
JPBHuffmanResult jpb_HuffmanLoadFiles(
    const char *table_path,
    const char *value_path,
    const char *option_path,
    JPBHuffmanTableSet *tables);
JPBHuffmanResult jpb_HuffmanValidateTables(
    const JPBHuffmanTableSet *tables);
void jpb_HuffmanUseTables(JPBHuffmanTableSet *tables);

#ifdef __cplusplus
}
#endif

#endif
