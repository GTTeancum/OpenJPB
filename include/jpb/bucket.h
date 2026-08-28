#ifndef JPB_BUCKET_H
#define JPB_BUCKET_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_BUCKET_COMMAND_COUNT = 13,
    JPB_BUCKET_MODEL_COUNT = 25,
    JPB_BUCKET_LIST_COUNT = 4419,
    JPB_BUCKET_SUBNAME_CAPACITY = 128,
    JPB_BUCKET_LAST_NAME_CAPACITY = 256
};

extern char *bucketCom[JPB_BUCKET_COMMAND_COUNT];
extern uint16_t bucketModels[JPB_BUCKET_MODEL_COUNT];
extern char *bucketList[JPB_BUCKET_LIST_COUNT];
extern uint8_t bucketDebug;
extern uint8_t *bucketLoadFP;
extern uint32_t bucketLoadOffset;
extern uint32_t bucketLoaded;
extern uint32_t bucketWrapper;
extern uint32_t bucketUncomp1;
extern uint32_t bucketUncomp2;
extern char bucketSubName[JPB_BUCKET_SUBNAME_CAPACITY];
extern uint32_t bucketCompChecksum;
extern uint32_t bucketUncompChecksum;
extern uint32_t bucketSubCompressedSize;
extern uint32_t bucketSubUncompressedSize;
extern uint32_t bucketSubCompressed;
extern uint32_t bucketSubCurrentFP;
extern uint8_t *bucketSubaddress;
extern uint32_t bucketSubsize;
extern char lastBukLoaded[JPB_BUCKET_LAST_NAME_CAPACITY];
extern FILE *bucketFP;

void bucketCopy(uint8_t *dst, unsigned offset, unsigned size);
void bucketDecompressRLRandom(uint8_t *bufferIn, uint8_t *bufferOut,
    unsigned fsize, unsigned rdOffset, unsigned rdSize);
int bucketFindFile(char *name, unsigned *bukNameIndex);
int bucketFindSubname(char *name, uint8_t *loadaddress,
    unsigned *size, uint8_t **address);
void bucketFront(void);
int bucketHandleCommand(unsigned command);
int bucketHandler(int narg, char **arg_str, int *arg_int, float *arg_float);
void bucketJedi(void);
void bucketLevelLoad(unsigned level);
void bucketLevels(void);
int bucketLoadBuk(unsigned bucketIndex, char *subname);
void bucketRLDecompress(uint8_t *bufferIn, uint8_t *bufferOut, unsigned fsize);
void bucketStringToCaps(char *s);
unsigned checkBucketFP(unsigned val);
void closeBucketLog(void);
void debugBucket(char *string, ...);
int gammaHandler(int narg, char **arg_str, int *arg_int, float *arg_float);
int gdirHandler(int narg, char **arg_str, int *arg_int, float *arg_float);
void gdirOutput(char *text);
void initBucket(unsigned flag);
int memConsoleHandler(int narg, char **arg_str, int *arg_int, float *arg_float);
unsigned openBucket(char *in_name, long *fd, unsigned *offset);
void openBucketLog(char *name);
void pauseUnpauseBucket(void);
unsigned ramBukChecksum(uint8_t *buffer, unsigned count);
void updateBucket(char *name);
void updateBucketWrapper(uint8_t *name);
int vmemHandler(int narg, char **arg_str, int *arg_int, float *arg_float);
void writeBucketFileLog(char *name);

#ifdef __cplusplus
}
#endif

#endif
