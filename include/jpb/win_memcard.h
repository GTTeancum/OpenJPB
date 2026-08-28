#ifndef JPB_WIN_MEMCARD_H
#define JPB_WIN_MEMCARD_H

#ifdef __cplusplus
extern "C" {
#endif

extern int console_currentmemcard;

char *cardfile(int card, char *filename);
void kmMemcard_Delete(int card, char *ptrFileName);
void kmMemcard_Exit(void);
void kmMemcard_Load(int card, char *ptrFileName);
void kmMemcard_Save(
    int card, char *ptrFileName, void *ptrData, long size);
void kmMemcard_Update(void);
void memcard_DeleteFile(int card, char *filename);
int memcard_FileExists(int card, char *filename);
char *memcard_FindFirstFile(int card);
char *memcard_FindNextFile(int card);
int memcard_LoadFile(
    int card, char *filename, unsigned char **data, unsigned long *size);
void memcard_SaveFile(
    int card, char *filename, unsigned char *data, unsigned long size);
void memcard_off(void);
void memcard_on(void);

#ifdef __cplusplus
}
#endif

#endif
