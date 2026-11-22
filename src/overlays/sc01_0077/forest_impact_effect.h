#ifndef BFM_FOREST_IMPACT_EFFECT_H
#define BFM_FOREST_IMPACT_EFFECT_H

#include "psx_types.h"

typedef struct {
    u16 kind;
    u16 state;
    u16 x_fraction;
    s16 x;
    u16 y_fraction;
    s16 y;
    u16 z_fraction;
    s16 z;
    u8 reserved10[12];
    u32 remaining;
} ForestImpactEffect;

typedef void (*ForestImpactHandler)(ForestImpactEffect *effect);
extern ForestImpactHandler D_80189B20[];

void func_80168AA8(ForestImpactEffect *effect);
void func_80168AE4(ForestImpactEffect *effect);
void func_80168B70(ForestImpactEffect *effect);

#endif
