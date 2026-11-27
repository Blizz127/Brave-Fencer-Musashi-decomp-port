/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_ACTOR_H
#define BFM_WOODS_ROOTRICK_ACTOR_H

#include <stddef.h>
#include <stdint.h>

typedef struct BfmVillageGround BfmVillageGround;

typedef struct {
    int16_t xyz[3];
    unsigned yaw;
    int pitch;
    uint32_t clip;
    unsigned elapsed;

    const BfmVillageGround *ground;
    int32_t jump[3], climb[3];
    int16_t probe[3];
    uint16_t fraction[3];
    int32_t velocity[3], gravity;
    unsigned stage, timer, turn_fields, target_yaw, pose_held;
} BfmWoodsRootrickActor;

enum {
    BFM_ROOTRICK_ACTOR_ERROR = -1,
    BFM_ROOTRICK_ACTOR_RELEASE = 1,
    BFM_ROOTRICK_ACTOR_IMPACT = 2,       /* Two dust puffs, camera shake, sound 0x5ca. */
    BFM_ROOTRICK_ACTOR_START_WALL = 4,   /* Install the hero's scripted approach to the race wall. */
    BFM_ROOTRICK_ACTOR_AT_WALL = 8      /* Hand the current rival root to the race controller. */
};

/* Initialize at the actual cast bind, using the stage loader's mounted kind-128
 * placement. The caller owns the room ground throughout the actor's lifetime. */
int bfm_woods_rootrick_actor_begin(BfmWoodsRootrickActor *actor, const BfmVillageGround *ground,
                                   const uint8_t *scene, size_t bytes, const int16_t xyz[3]);
void bfm_woods_rootrick_actor_bind_clip(BfmWoodsRootrickActor *actor, uint32_t clip);
void bfm_woods_rootrick_actor_set_yaw(BfmWoodsRootrickActor *actor, unsigned yaw);
void bfm_woods_rootrick_actor_turn_to(BfmWoodsRootrickActor *actor, const int16_t player[3]);
int bfm_woods_rootrick_actor_stage(BfmWoodsRootrickActor *actor, unsigned script_stage);

/* On opening-script completion, arm race score 28 and queue this once.
 * The next actor update starts the hero program and Rootrick's own wall jump. */
void bfm_woods_rootrick_actor_begin_wall(BfmWoodsRootrickActor *actor);

/* Apply a local 16.16 velocity in the current model orientation. The same
 * actor carries its trajectory, fractions and pose from the opening to the race. */
void bfm_woods_rootrick_actor_impulse(BfmWoodsRootrickActor *actor, const int32_t velocity_q16[3],
                                     int32_t gravity_q16);
/* Add acceleration and then velocity. A world-space probe offset enables the
 * mounted segment/terrain query, retaining fractions at the projected position.
 * NULL advances freely. Contact uses 0x2000 / 0x4000 / 0x8000 surface bits. */
int bfm_woods_rootrick_actor_move(BfmWoodsRootrickActor *actor, const int16_t probe[3],
                                 unsigned *contact);
/* Sweep a walking probe, then confirm its footing with a short vertical
 * sample. This lets a walking actor travel naturally beyond the cliff rim. */
int bfm_woods_rootrick_actor_walk(BfmWoodsRootrickActor *actor, const int16_t probe[3],
                                 unsigned *contact);
void bfm_woods_rootrick_actor_step_turn(BfmWoodsRootrickActor *actor);

/* Once per field, before polling the script. score_requested is the score
 * service's outstanding request after it has polled/acknowledged audio. */
int bfm_woods_rootrick_actor_step(BfmWoodsRootrickActor *actor, int score_requested);

#endif
