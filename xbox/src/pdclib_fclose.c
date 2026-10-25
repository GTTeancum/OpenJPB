/* fclose( FILE * ) for the Xbox build.

   Based on nxdk's pdclib functions/stdio/fclose.c (Public Domain C Library,
   CC0). nxdk's pdclib fopen creates a kernel mutant for every stream
   (mtx_init -> NtCreateMutant) and its fclose frees the stream without
   mtx_destroy, leaking about 70 bytes of kernel pool per file: about
   100 KB per level-to-menu round trip. This definition is linked ahead of
   the library's, so every caller (game code, SDL, SDL_image, SDL_ttf)
   releases the mutant too.
*/

#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#include "pdclib/_PDCLIB_glue.h"

extern mtx_t _PDCLIB_filelist_mtx;
extern struct _PDCLIB_file_t *_PDCLIB_filelist;

int fclose(struct _PDCLIB_file_t *stream)
{
    _PDCLIB_LOCK(stream->mtx);

    if (_PDCLIB_getstream(stream))
    {
        _PDCLIB_UNLOCK(stream->mtx);
        return EOF;
    }

    if (stream->status & _PDCLIB_FWRITE)
    {
        if (_PDCLIB_flushbuffer(stream) == EOF)
        {
            _PDCLIB_UNLOCK(stream->mtx);
            return EOF;
        }
    }

    _PDCLIB_close(stream->handle);

    if (stream->status & _PDCLIB_DELONCLOSE)
    {
        remove(stream->filename);
    }

    if (stream->status & _PDCLIB_FREEBUFFER)
    {
        free(stream->buffer);
    }

    if (stream->status & _PDCLIB_FREENAME)
    {
        free(stream->filename);
    }

    _PDCLIB_UNLOCK(stream->mtx);

    if (!(stream->status & _PDCLIB_STATIC))
    {
        mtx_destroy(&stream->mtx);
        free(stream);
    }

    return 0;
}
