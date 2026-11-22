#include "forest_impact_effect.h"

extern void func_80168BDC(ForestImpactEffect *effect, s32 kind, s16 variant, s16 count);
extern s32 func_80146A6C(s32 kind, void *owner, s32 x, s32 y, s32 z,
                         s32 argument, s32 flags);

void func_80168AE4(ForestImpactEffect *effect)
{
    func_80168BDC(effect, 9, 3, 2);
    func_80168BDC(effect, 9, 2, 1);
    func_80146A6C(3, effect, effect->x, effect->y, effect->z, -5, (s32)0xC0009C00);
    effect->remaining = 1;
    effect->state++;
}
