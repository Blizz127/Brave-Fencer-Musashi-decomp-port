#ifndef BFM_SC03_0116_ACTOR_334_H
#define BFM_SC03_0116_ACTOR_334_H

#include "psx_types.h"

/* This is the prefix shared with the room's phase-advance routine. */
typedef struct {
    u8 pad0[2];
    u16 cnt;
    u8 pad4[0x30];
    u16 clr;
} Actor334State;

typedef struct {
    Actor334State state;
    u8 before_counter[0xc6];
    u16 counter;
    u8 tail[0x0e];
} Actor334;

typedef void (*Actor334Phase)(Actor334 *actor);
extern Actor334Phase D_80186324[6];
extern u16 D_80186320;

void func_8012AD50(Actor334State *state);
void func_80180FBC(Actor334 *actor);
void func_80180FF8(Actor334 *actor);

#endif
