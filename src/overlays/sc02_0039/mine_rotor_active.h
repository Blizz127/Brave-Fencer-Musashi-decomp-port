#ifndef BFM_MINE_ROTOR_ACTIVE_H
#define BFM_MINE_ROTOR_ACTIVE_H

#include "mine_guest_abi.h"
#include "../main_0012/scene_object_probes.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_8017DC7C(Obj8014A2E4 *actor);
#else
#include <stdint.h>

typedef struct {
    uint8_t bytes[22];
} MineRotorModel;

typedef struct {
    void *context;
    int16_t player_x;
    MineRotorModel *(*model)(void *context, uint32_t key);
    int (*angular_step)(void *context, int16_t row, uint16_t *step);
    int (*sound)(void *context, uint16_t id, uint16_t argument, uint16_t extra);
} MineRotorActive;

int mine_rotor_update(Obj8014A2E4 *actor, const MineRotorActive *services);
#endif
#endif
