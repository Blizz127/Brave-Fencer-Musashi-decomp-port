#ifndef BFM_MINE_COLLISION_ACTOR_SETUP_H
#define BFM_MINE_COLLISION_ACTOR_SETUP_H
#include "../main_0012/scene_object_probes.h"
#include "mine_guest_abi.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_8017DE84(Obj8014A2E4 *actor);
#else
#include <stdint.h>

typedef enum {
    MINE_SETUP_LIST_A,
    MINE_SETUP_LIST_B
} MineSetupList;

typedef struct {
    void *context;
    /* The room owner validates the identity of this untagged box address. */
    uint32_t box_address;
    uint32_t (*allocate)(void *);
    void (*remove_actor)(void *, Obj8014A2E4 *);
    void (*configure)(void *, uint32_t, MineSetupList);
    void (*spawn)(void *, int16_t, int16_t, Obj8014A2E4 *);
    void (*prepare_child)(void *, uint32_t);
    void (*link_child)(void *, uint32_t, uint32_t);
    const Obj8014A2E4 *(*actor)(void *, uint32_t);
    uint8_t *(*model_flags)(void *, uint32_t);
} MineSetupServices;

/* Accepted inputs are complete typed actors; each provider owns its own side effects.
 * Unknown parent/model keys stop at their original lookup boundary. */
int mine_setup_actor(Obj8014A2E4 *actor, const MineSetupServices *services);
#endif
#endif
