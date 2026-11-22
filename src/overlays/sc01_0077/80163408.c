#include "forest_projectile_contact.h"

/* Give each projectile query its own copy of the scene's offset record. */
s32 func_80163408(void *effect, u16 attack, s32 flags, u16 mask)
{
    ForestProjectileOffset offset = D_801D8BB0;
    return func_8016345C(effect, attack, flags, mask, &offset);
}
