#include "jpb/reconstruction.h"

int main(void)
{
    return jpb_reconstruction_scaffold_version() == 1 ? 0 : 1;
}

