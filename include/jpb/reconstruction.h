#ifndef JPB_RECONSTRUCTION_H
#define JPB_RECONSTRUCTION_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This symbol keeps the generated, initially body-free reconstruction archive
 * linkable while recovered functions are introduced module by module.
 */
int jpb_reconstruction_scaffold_version(void);

#ifdef __cplusplus
}
#endif

#endif

