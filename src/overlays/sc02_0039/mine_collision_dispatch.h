#ifndef BFM_MINE_COLLISION_DISPATCH_H
#define BFM_MINE_COLLISION_DISPATCH_H
#include "../main_0012/scene_object_probes.h"

typedef void (*MineCollisionActorAction)(Obj8014A2E4 *actor);
#if defined(__mips__) && !defined(__LP64__)
void func_8017DFE8(Obj8014A2E4 *actor);
#else
typedef struct {
    MineCollisionActorAction initial;
    MineCollisionActorAction update;
} MineCollisionActorActions;

int bfm_mine_collision_actor_update(Obj8014A2E4 *actor, const MineCollisionActorActions *actions);
#endif
#endif
