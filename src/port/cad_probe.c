#include "jpb/cad.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    uint8_t *buffer;
    JPBCadView view;
    JPBCadResult result;

    if (argc != 2 &&
        !(argc == 3 &&
          strcmp(argv[2], "--motions") == 0)) {
        fprintf(
            stderr,
            "usage: %s animation.cad [--motions]\n",
            argv[0]);
        return 2;
    }
    buffer = (uint8_t *)malloc(JPB_CAD_REFERENCE_CAPACITY);
    if (buffer == NULL) {
        fputs("failed to allocate CAD storage\n", stderr);
        return 1;
    }
    result = jpb_CadLoadFile(
        argv[1],
        buffer,
        JPB_CAD_REFERENCE_CAPACITY,
        &view);
    if (result != JPB_CAD_OK) {
        fprintf(
            stderr,
            "invalid CAD %s: result=%d\n",
            argv[1],
            (int)result);
        free(buffer);
        return 1;
    }
    printf(
        "cad=%s bytes=%zu sequences=%u parts=%u "
        "bitstream=%zu first=%.32s\n",
        argv[1],
        view.file_size,
        (unsigned)view.sequence_count,
        (unsigned)view.part_count,
        view.bitstream_size,
        view.sequence_count != 0
            ? view.motions[0].name
            : "");
    if (argc == 3) {
        uint32_t motion_index;

        for (motion_index = 0;
             motion_index < view.sequence_count;
             ++motion_index) {
            const Motion *motion =
                &view.motions[motion_index];
            const _animTemplate *sequence =
                motion->Seq < view.sequence_count
                    ? &view.sequences[motion->Seq]
                    : NULL;

            printf(
                "motion[%u] name=%.32s seq=%u flags=%08x "
                "attack=%08x damage=%u react=%u combo=%u "
                "fx=(%d@%d,%d@%d) callback=%d "
                "cut=(%u,%u) lock=%u speed=%d disp=%u "
                "frames=(%d,%d) pre_roll=%d parts=%d\n",
                (unsigned)motion_index,
                motion->name,
                (unsigned)motion->Seq,
                (unsigned)motion->motionFlags,
                (unsigned)motion->attackFlags,
                (unsigned)motion->Damage,
                (unsigned)motion->hitReact,
                (unsigned)motion->combo,
                (int)motion->fx1,
                (int)motion->fx1Delay,
                (int)motion->fx2,
                (int)motion->fx2Delay,
                (int)motion->FunctPtr,
                (unsigned)motion->cutin,
                (unsigned)motion->cutout,
                (unsigned)motion->Lock,
                (int)motion->Speed,
                (unsigned)motion->disp,
                sequence != NULL ? (int)sequence->Fframe : -1,
                sequence != NULL ? (int)sequence->Lframe : -1,
                sequence != NULL ? (int)sequence->pad1 : -1,
                sequence != NULL ? (int)sequence->parts : -1);
        }
    }
    free(buffer);
    return 0;
}
