#include "forest_impact_effect.h"

extern void func_80168BDC(ForestImpactEffect *effect, s32 kind, s16 variant, s16 count);
extern void func_80146C3C(ForestImpactEffect *effect);

void func_80168B70(ForestImpactEffect *effect)
{
    effect->remaining--;
    if (effect->remaining != (u32)-1) {
        func_80168BDC(effect, 9, 3, 1);
    } else {
        func_80168BDC(effect, 9, 2, 1);
        func_80146C3C(effect);
    }
}
