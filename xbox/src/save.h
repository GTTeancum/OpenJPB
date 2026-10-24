#pragma once
#include "jpb/game.h"

int jpb_XboxSaveInit(void);
void jpb_XboxSaveLoadOptions(void);
void jpb_XboxSaveLoadGame(void);
int jpb_XboxSaveHasGame(void);
void jpb_XboxSaveWriteGame(void);
void jpb_XboxSaveWriteOptions(const optionstruct *options);
