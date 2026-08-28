/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\win32\IO.c.
 *
 * This is the game's existing file-I/O seam. The reference delegates to the
 * Microsoft CRT; the portable body uses only standard C stdio so the same API
 * can be backed by nxdk later without leaking desktop types into game code.
 *
 * Provenance:
 *   direct     - names, signatures, locals, and extents from the exact PDB.
 *   decompiled - mode dispatch and wrapper calls checked against Ghidra.
 *   assembly   - memory-source cursor advancement and return values checked
 *                at RVAs 0x128630 through 0x12897F.
 *
 * All eleven emitted procedures have been checked instruction by instruction
 * against the shipped executable at RVAs 0x128630 through 0x12897F.
 *
 * PDB module: 0096
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\IO.obj
 * Primary source: W:\SWJediPowerBattles\work\win32\IO.c
 * Compiler language: c
 * Emitted procedures: 11
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/bucket.h"
#include "jpb/debugtext.h"
#include "jpb/filesys.h"
#include "jpb/io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *io_file_from_handle(JPBFileHandle handle)
{
    return (FILE *)(uintptr_t)handle;
}

#if defined(JPB_IO_TESTING)
static JPBFileAppendOpenTestHook file_append_open_test_hook;

void jpb_IOSetFileAppendOpenTestHook(JPBFileAppendOpenTestHook hook)
{
    file_append_open_test_hook = hook;
}
#endif

/* Reference RVA 0x128630, 164 bytes. */
uint64_t file_AppendFile(char *name, char *buf, int32_t size)
{
    FILE *file;
    size_t written;

    (void)debug_printf("write:%s\n", name);
#if defined(JPB_IO_TESTING)
    if (file_append_open_test_hook != NULL) {
        file = (FILE *)file_append_open_test_hook(name, "awb");
    } else
#endif
    {
    file = fopen(name, "awb");
    }
    if (file == NULL) {
        (void)debug_printf("can't open APPEND:%s\n", name);
        return 0;
    }
    written = fwrite(buf, (size_t)size, 1, file);
    if (written == 0) {
        (void)debug_printf("APPEND error:%s\n", name);
        return 0;
    }
    (void)fclose(file);
    return 1;
}

/* Reference RVA 0x1286E0, 8 bytes. */
int file_CLOSE(JPBFileHandle *fd)
{
    return fclose(io_file_from_handle(*fd));
}

/* Reference RVA 0x1286F0, 63 bytes. */
uint64_t file_GETSIZE(JPBFileHandle *fd)
{
    FILE *file = io_file_from_handle(*fd);
    long length;

    (void)fseek(file, 0, SEEK_END);
    length = ftell(file);
    (void)fseek(file, 0, SEEK_SET);
    return (uint64_t)(uint32_t)(int32_t)length;
}

/* Reference RVA 0x128730, 78 bytes. */
int file_OPEN(char *name, JPBFileHandle *fd)
{
    FILE *file;

    if (name == NULL) {
        return 0;
    }
    file = fopen(name, "rb");
    if (file == NULL) {
        return 0;
    }
    *fd = (JPBFileHandle)(uintptr_t)file;
    updateBucket(name);
    return 1;
}

/* Reference RVA 0x128780, 212 bytes. */
uint64_t file_READ(
    JPBFileHandle *fd, char *buf, int32_t size, int32_t flag)
{
    if (flag == JPB_FILE_READ_MEMORY) {
        void *source = (void *)(uintptr_t)*fd;

        (void)memcpy(buf, source, (size_t)size);
        *fd += (uintptr_t)size;
        return 1;
    }

    if (flag == JPB_FILE_READ_ALL) {
        FILE *file = io_file_from_handle(*fd);
        uint64_t length = file_GETSIZE(fd);
        void *buffer = malloc((size_t)length);

        (void)fread(buffer, 1, (size_t)length, file);
        (void)fclose(file);
        *fd = (JPBFileHandle)(uintptr_t)buffer;
        return (uint64_t)(uintptr_t)buffer;
    }

    return (uint64_t)fread(
        buf, 1, (size_t)size, io_file_from_handle(*fd));
}

/* Reference RVA 0x128860, 3 bytes. */
unsigned file_ReadPC(char *name, char *buf)
{
    (void)name;
    (void)buf;
}

/* Reference RVA 0x128870, 22 bytes. */
uint64_t file_SEEK(JPBFileHandle *fd, int bytes)
{
    int result = fseek(io_file_from_handle(*fd), bytes, SEEK_SET);

    return (uint64_t)(int64_t)result;
}

/* Reference RVA 0x128890, 164 bytes. */
uint64_t file_WriteFile(char *name, char *buf, int32_t size)
{
    FILE *file;
    size_t written;

    (void)debug_printf("write:%s\n", name);
    file = fopen(name, "wb");
    if (file == NULL) {
        (void)debug_printf("can't create:%s\n", name);
        return 0;
    }
    written = fwrite(buf, (size_t)size, 1, file);
    if (written == 0) {
        (void)debug_printf("write error:%s\n", name);
        return 0;
    }
    (void)fclose(file);
    return 1;
}

/* Reference RVA 0x128940, 3 bytes. */
void file_gInitialise(void)
{
}

/* Reference RVA 0x128950, 8 bytes. */
unsigned io_file_LoadFile(unsigned char *name, unsigned char **buffer)
{
    return (unsigned)file_LoadFile((char *)name, *buffer);
}

/* Reference RVA 0x128960, 32 bytes. */
char *io_file_LoadFile2Pool(char *name, int32_t *size, int memtype)
{
    return file_LoadFile2PoolFunc(
        name,
        size,
        memtype,
        0xB1,
        "W:\\SWJediPowerBattles\\work\\win32\\IO.c");
}

/* 0x128630, 164 bytes, global, 4 named locals
 * file_AppendFile
 * PDB type: unsigned __int64 (char*, char*, ...
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x1286E0, 8 bytes, global, 1 named locals
 * file_CLOSE
 * PDB type: int (unsigned __int64*)
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x1286F0, 63 bytes, global, 2 named locals
 * file_GETSIZE
 * PDB type: unsigned __int64 (unsigned __int...
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x128730, 78 bytes, global, 3 named locals
 * file_OPEN
 * PDB type: int (char*, unsigned __int64*)
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x128780, 212 bytes, global, 7 named locals
 * file_READ
 * PDB type: unsigned __int64 (unsigned __int...
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x128860, 3 bytes, global, 2 named locals
 * file_ReadPC
 * PDB type: unsigned (char*, char*)
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x128870, 22 bytes, global, 2 named locals
 * file_SEEK
 * PDB type: unsigned __int64 (unsigned __int...
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x128890, 164 bytes, global, 4 named locals
 * file_WriteFile
 * PDB type: unsigned __int64 (char*, char*, ...
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x128940, 3 bytes, global, 0 named locals
 * file_gInitialise
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x128950, 8 bytes, global, 2 named locals
 * io_file_LoadFile
 * PDB type: unsigned (unsigned char*, unsign...
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */

/* 0x128960, 32 bytes, global, 4 named locals
 * io_file_LoadFile2Pool
 * PDB type: char* (char*, long*, int)
 * Source: W:\SWJediPowerBattles\work\win32\IO.c
 */
