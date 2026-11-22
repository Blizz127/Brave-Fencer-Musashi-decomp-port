#ifndef MUSASHI_MINE_ROTOR_SPAWN_H
#define MUSASHI_MINE_ROTOR_SPAWN_H

#include "mine_guest_abi.h"
#include "../main_0012/scene_object_probes.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_8017DE00(Obj8014A2E4 *rotor);
#else
typedef struct {
    void *context;
    int (*read_option)(void *context, s16 row, unsigned column, u16 *out);
    s32 (*spawn)(void *context, s16 kind, s16 option, Obj8014A2E4 *parent);
} MineRotorSpawn;

/* Reads one room-table entry per call. Prior spawns may alter the row in the
   actor. A refused table access preserves earlier completed spawn calls. */
int mine_rotor_spawn_children(Obj8014A2E4 *actor, const MineRotorSpawn *services);
#endif
#endif
