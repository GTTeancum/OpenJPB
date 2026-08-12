#include "jpb/huffman.h"
#include "jpb/io.h"

#include <limits.h>
#include <string.h>

static int huffman_tree_reference_is_valid(uint16_t reference)
{
    if ((reference & 0x8000u) != 0) {
        return (reference & 0x7fffu) <= 0x0fffu;
    }
    return reference < JPB_HUFFMAN_TREE_COUNT;
}

static JPBHuffmanResult huffman_load_exact_file(
    const char *path, void *destination, size_t expected_size)
{
    JPBFileHandle file = 0;
    uint64_t file_size;
    uint64_t bytes_read;

    if (!file_OPEN((char *)path, &file)) {
        return JPB_HUFFMAN_IO_ERROR;
    }
    file_size = file_GETSIZE(&file);
    if (file_size != expected_size ||
        expected_size > INT32_MAX) {
        (void)file_CLOSE(&file);
        return JPB_HUFFMAN_INVALID_SIZE;
    }
    bytes_read = file_READ(
        &file,
        (char *)destination,
        (int32_t)expected_size,
        JPB_FILE_READ_STREAM);
    (void)file_CLOSE(&file);
    if (bytes_read != expected_size) {
        return JPB_HUFFMAN_IO_ERROR;
    }
    return JPB_HUFFMAN_OK;
}

JPBHuffmanResult jpb_HuffmanLoadFiles(
    const char *table_path,
    const char *value_path,
    const char *option_path,
    JPBHuffmanTableSet *tables)
{
    JPBHuffmanResult result;

    if (table_path == NULL ||
        value_path == NULL ||
        option_path == NULL ||
        tables == NULL) {
        return JPB_HUFFMAN_INVALID_ARGUMENT;
    }
    memset(tables, 0, sizeof(*tables));

    result = huffman_load_exact_file(
        table_path, tables->tree, sizeof(tables->tree));
    if (result != JPB_HUFFMAN_OK) {
        return result;
    }
    result = huffman_load_exact_file(
        value_path, tables->values, sizeof(tables->values));
    if (result != JPB_HUFFMAN_OK) {
        return result;
    }
    result = huffman_load_exact_file(
        option_path, tables->options, sizeof(tables->options));
    if (result != JPB_HUFFMAN_OK) {
        return result;
    }
    return jpb_HuffmanValidateTables(tables);
}

JPBHuffmanResult jpb_HuffmanValidateTables(
    const JPBHuffmanTableSet *tables)
{
    size_t index;

    if (tables == NULL) {
        return JPB_HUFFMAN_INVALID_ARGUMENT;
    }
    for (index = 0;
         index < JPB_HUFFMAN_OPTION_COUNT;
         ++index) {
        const _optab *option = &tables->options[index];

        if (option->vals >= 0) {
            uint32_t buffered_count =
                (uint32_t)option->vals >> 28;
            uint32_t value_offset =
                (uint32_t)option->vals & 0x7fffu;

            if (value_offset + buffered_count + 1u >
                JPB_HUFFMAN_VALUE_COUNT) {
                return JPB_HUFFMAN_INVALID_TABLE;
            }
        } else {
            uint32_t tree_root =
                (uint32_t)option->gotto & 0x7fffu;

            if (tree_root >= JPB_HUFFMAN_TREE_COUNT) {
                return JPB_HUFFMAN_INVALID_TABLE;
            }
        }
    }
    for (index = 0;
         index < JPB_HUFFMAN_TREE_COUNT;
         ++index) {
        uint32_t branches = tables->tree[index];

        if (!huffman_tree_reference_is_valid(
                (uint16_t)branches) ||
            !huffman_tree_reference_is_valid(
                (uint16_t)(branches >> 16))) {
            return JPB_HUFFMAN_INVALID_TABLE;
        }
    }
    return JPB_HUFFMAN_OK;
}

void jpb_HuffmanUseTables(JPBHuffmanTableSet *tables)
{
    if (tables == NULL) {
        return;
    }
    unpack_init(
        tables->options,
        tables->values,
        tables->tree,
        JPB_HUFFMAN_TREE_COUNT);
}
