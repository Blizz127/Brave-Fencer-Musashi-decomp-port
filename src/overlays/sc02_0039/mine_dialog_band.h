#ifndef BFM_MINE_DIALOG_BAND_H
#define BFM_MINE_DIALOG_BAND_H

#include "psx_types.h"

typedef struct {
    u8 red;
    u8 green;
    u8 blue;
    u8 pad;
} MineBandColor;

extern u8 D_801A0D8A;
extern u8 D_801A0CB2;
extern u8 D_801A0C90;
extern MineBandColor D_8019F950[];

void func_801299C8(s16 enabled, s16 selector, u8 *band);

#endif
