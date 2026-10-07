#ifndef OPENJPB_XBOX_UI_H
#define OPENJPB_XBOX_UI_H

int jpb_XboxUiInit(void);
int jpb_XboxUiLoadGameplay(void);
int jpb_XboxUiTakeMovie(unsigned *movie, int *flags);
int jpb_XboxUiMoviesPending(void);

#endif
