/* Machine-readable golden poses from the reconstructed native decoder. */
#include "jpb/cad.h"
#include "jpb/huffman.h"
#include "jpb/unpack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    JPBCadView view;
    JPBHuffmanTableSet tables;
    unsigned char *storage;
    FILE *output;
    char tab[4096], val[4096], opt[4096];
    unsigned i;
    if (argc != 4) {
        fprintf(stderr, "usage: %s file.cad tables-directory output.bin\n", argv[0]);
        return 2;
    }
    storage = calloc(1, JPB_CAD_REFERENCE_CAPACITY);
    snprintf(tab, sizeof(tab), "%s/huffman.tab", argv[2]);
    snprintf(val, sizeof(val), "%s/huffman.val", argv[2]);
    snprintf(opt, sizeof(opt), "%s/huffman.opt", argv[2]);
    if (!storage || jpb_CadLoadFile(argv[1], storage, JPB_CAD_REFERENCE_CAPACITY, &view) != JPB_CAD_OK ||
        jpb_HuffmanLoadFiles(tab,val,opt,&tables) != JPB_HUFFMAN_OK) return 1;
    jpb_HuffmanUseTables(&tables);
    output = fopen(argv[3], "wb");
    if (!output) return 1;
    for (i = 0; i < view.sequence_count; ++i) {
        animObject anim = {0};
        animListNode node = {0};
        Motion motion = {0};
        _animTemplate seq = view.sequences[i];
        int frame;
        /* Dump complete authored stream, before gameplay cut-in/pre-roll. */
        seq.Fframe = 0; seq.pad1 = 0;
        if (seq.parts < 0 || seq.parts > 32 || seq.Lframe < 0 ||
            seq.FframeAddr >= view.depack_window_size) return 1;
        node.pAnimTemplate = &seq;
        node.pMotion = &motion;
        anim.pCurrentAnimSeq = &node;
        anim.pMotion = &motion;
        anim.pCurrentAnimFrame = &anim.AnimFrameBuffer[0];
        anim.pPreviousAnimFrame = &anim.AnimFrameBuffer[0];
        unpack_initcontext(&anim.depack_context, view.payload);
        for (frame = 0; frame < seq.Lframe; ++frame) {
            _animFrame *decoded = NULL;
            anim.animFrameAcc = 4096;
            if (jpb_AnimDecodeFrameState(&anim, &decoded) != JPB_ANIM_PARTIAL_OK) return 1;
            fwrite(&decoded->v3RootTranslation, sizeof(_svector), seq.parts+1, output);
        }
    }
    fclose(output);
    free(storage);
    return 0;
}
