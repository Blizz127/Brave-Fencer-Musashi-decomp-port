#ifndef BFM_MINE_COLLISION_ACTOR_ACTIVE_H
#define BFM_MINE_COLLISION_ACTOR_ACTIVE_H

#include "../main_0012/scene_object_probes.h"
#include "model_surface_probe.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_8017E500(Obj8014A2E4 *actor);
void func_8017E25C(Obj8014A2E4 *actor);
s32 func_8017E2F4(Obj8014A2E4 *actor, SceneSurfaceBox *box);
#else
#include <stdint.h>

/* The segment provider receives the three coordinates written by the caller. */
typedef struct {
    int16_t x, y, z;
} MineSweepPoint;

typedef struct {
    void *context;
    int16_t player_x;
    SceneSurfaceBox *box;
    uint16_t *event_code;
    uint16_t *event_time;
    Obj8014A2E4 *(*parent)(void *context, uint32_t actor_key);
    /* The returned model has at least 22 writable bytes. */
    uint8_t *(*model)(void *context, uint32_t model_key);
    int (*transform)(void *context, Obj8014A2E4 *actor);
    int (*segment)(void *context, Obj8014A2E4 *actor, MineSweepPoint *start,
                   MineSweepPoint *end, int *hit);
} MineActiveServices;

int mine_actor_motion(Obj8014A2E4 *actor, const MineActiveServices *services);
int mine_actor_sweep(Obj8014A2E4 *actor, const MineActiveServices *services, int *hit);
int mine_actor_active(Obj8014A2E4 *actor, const MineActiveServices *services);
#endif
#endif
