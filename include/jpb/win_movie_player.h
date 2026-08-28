#ifndef JPB_WIN_MOVIE_PLAYER_H
#define JPB_WIN_MOVIE_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

extern const char *ptrMovies[10];

void winMovie_Init(void);
void winMovie_Play(int movieIndex, void *unused);

#ifdef __cplusplus
}
#endif

#endif
