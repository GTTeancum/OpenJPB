#ifndef JPB_IO_H
#define JPB_IO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The reference stores a CRT FILE pointer in an unsigned __int64. uintptr_t
 * preserves that host-width handle contract without forcing 64-bit pointers
 * onto the 32-bit Xbox target.
 */
typedef uintptr_t JPBFileHandle;

enum {
    JPB_FILE_READ_STREAM = 0,
    JPB_FILE_READ_MEMORY = 1,
    JPB_FILE_READ_ALL = 2
};

uint64_t file_AppendFile(char *name, char *buf, int32_t size);
int file_CLOSE(JPBFileHandle *fd);
uint64_t file_GETSIZE(JPBFileHandle *fd);
int file_OPEN(char *name, JPBFileHandle *fd);
uint64_t file_READ(
    JPBFileHandle *fd, char *buf, int32_t size, int32_t flag);
unsigned file_ReadPC(char *name, char *buf);
uint64_t file_SEEK(JPBFileHandle *fd, int bytes);
uint64_t file_WriteFile(char *name, char *buf, int32_t size);
void file_gInitialise(void);
unsigned io_file_LoadFile(unsigned char *name, unsigned char **buffer);
char *io_file_LoadFile2Pool(char *name, int32_t *size, int memtype);

#ifdef __cplusplus
}
#endif

#endif
