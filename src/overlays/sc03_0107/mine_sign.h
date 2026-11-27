#ifndef SC03_0107_MINE_SIGN_H
#define SC03_0107_MINE_SIGN_H

#include "psx_types.h"

typedef struct {
    u16 kind;
    u16 state;
    u8 unused04[0x6c];
    s16 variant;
    u8 unused72[0x4e];
    u8 field_c0;
} MineSign;

#endif
