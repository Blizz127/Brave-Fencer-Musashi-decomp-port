#ifndef MUSASHI_MINE_ROTOR_INIT_H
#define MUSASHI_MINE_ROTOR_INIT_H

#include "mine_rotor_spawn.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_8017E428(Obj8014A2E4 *actor);
#else
typedef struct {
    u8 bytes[0x2c];
} MineRotorModelFields;

typedef enum {
    MINE_ROTOR_PRIMARY_MODEL,
    MINE_ROTOR_ACCESSORY_MODEL
} MineRotorResource;

typedef struct {
    void *context;
    u32 (*allocate_model)(void *context);
    void (*remove_actor)(void *context, Obj8014A2E4 *actor);
    void (*configure)(void *context, u32 key, MineRotorResource resource);
    MineRotorModelFields *(*model)(void *context, u32 key);
    MineRotorSpawn children;
} MineRotorInit;

/* Models are resolved per use. A failed lookup or unavailable child service
   returns zero after the effects completed before that boundary. */
int mine_rotor_initialize(Obj8014A2E4 *actor, const MineRotorInit *services);
#endif
#endif
