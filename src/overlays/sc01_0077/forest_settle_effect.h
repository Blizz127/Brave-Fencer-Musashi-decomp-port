#ifndef BFM_FOREST_SETTLE_EFFECT_H
#define BFM_FOREST_SETTLE_EFFECT_H

#include "psx_types.h"

typedef struct {
    u16 type;
    u16 state;
    u8 reserved[0x18];
    s32 remaining;
} ForestSettleEffect;

void func_8017EA98(ForestSettleEffect *effect);
void func_8017EAC0(ForestSettleEffect *effect);
void func_8017EC40(ForestSettleEffect *effect);

#endif
