#ifndef MUSASHI_MINE_ROTOR_DISPATCH_H
#define MUSASHI_MINE_ROTOR_DISPATCH_H

#include "mine_guest_abi.h"
#include "../main_0012/scene_object_probes.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_8017DDB0(Obj8014A2E4 *actor);
#else
typedef int (*MineRotorAction)(void *context, Obj8014A2E4 *actor);
typedef struct {
    void *context;
    MineRotorAction initial;
    MineRotorAction update;
    MineRotorAction synchronize;
} MineRotorActions;

/* A completed state action is followed by model synchronization using the
   mutated actor. If a required host service refuses, completed effects remain. */
int mine_rotor_dispatch(Obj8014A2E4 *actor, const MineRotorActions *actions);
#endif
#endif
