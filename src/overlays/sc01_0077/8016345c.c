#include "forest_projectile_contact.h"

extern s32 func_80156370(u16 attack);
extern s32 func_80163534(void *effect, u16 attack, u16 power, s32 flags,
                         u16 mask, ForestProjectileOffset *offset);

s32 func_8016345C(void *effect, u16 attack, s32 flags, u16 mask,
                  ForestProjectileOffset *offset)
{
    u16 power = func_80156370(attack);
    return func_80163534(effect, attack, power, flags, mask, offset);
}
