#ifndef BFM_SC03_0116_ACTOR_252_H
#define BFM_SC03_0116_ACTOR_252_H

#include "psx_types.h"

typedef struct {
    u16 type;
    u16 phase;
    u8 remaining[0x108];
} Actor252;

typedef void (*Actor252Phase)(Actor252 *actor);
extern Actor252Phase D_80195174[6];

void func_80182378(Actor252 *actor);

#endif
