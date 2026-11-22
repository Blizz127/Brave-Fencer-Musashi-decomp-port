#ifndef FOREST_PROJECTILE_CONTACT_H
#define FOREST_PROJECTILE_CONTACT_H

#include "psx_types.h"

typedef struct {
    u8 bytes[8];
} ForestProjectileOffset;

extern ForestProjectileOffset D_801D8BB0;
s32 func_80163408(void *effect, u16 attack, s32 flags, u16 mask);
s32 func_8016345C(void *effect, u16 attack, s32 flags, u16 mask,
                  ForestProjectileOffset *offset);

#endif
