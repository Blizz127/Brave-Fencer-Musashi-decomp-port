#ifndef BFM_SC03_0116_STAGE_252_LIGHT_H
#define BFM_SC03_0116_STAGE_252_LIGHT_H

#include "actor_252.h"

typedef struct {
    int x, y, z;
    unsigned char r, g, b, pad;
} Actor252Light;

extern Actor252Light D_800A5EA8;
extern Actor252Light D_801950BC;

s32 func_8004787C(s32 angle);
void func_80028620(int index, Actor252Light *source);

#endif
