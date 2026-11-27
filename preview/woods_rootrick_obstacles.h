/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_OBSTACLES_H
#define BFM_WOODS_ROOTRICK_OBSTACLES_H

#include "forest_guard_contact.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    BFM_PLATFORM_CHOOSE_DWELL,
    BFM_PLATFORM_DWELL,
    BFM_PLATFORM_HELD,
    BFM_PLATFORM_EXTEND,
    BFM_PLATFORM_RETRACT,
    BFM_PLATFORM_READ_STORY,
    BFM_PLATFORM_WAIT_SIGNAL,
    BFM_PLATFORM_RETRACT_ONCE,
    BFM_PLATFORM_PARKED
} BfmRootrickPlatformPhase;

typedef struct {
    int16_t xyz[3];
    uint16_t flags;      /* Placement behavior, including the live dwell selector. */
    uint16_t body_flags; /* Mounted model's contact flags, updated as the platform travels. */
    uint16_t timer;
    int16_t signal;
    unsigned extended;
    BfmRootrickPlatformPhase phase;
} BfmRootrickPlatform;

enum { BFM_ROOTRICK_PLATFORMS = 3 };
typedef struct BfmRootrickObstacleField {
    unsigned count;
    BfmRootrickPlatform platform[BFM_ROOTRICK_PLATFORMS];
    BfmForestGuardBox platform_box[2], rival;
    uint32_t model[2];
} BfmRootrickObstacleField;

/* Place the room's platforms in scene order and copy the mounted contact profiles.
 * Tick on each effective actor update after placement, before updating Rootrick. */
int bfm_rootrick_obstacles_mount(BfmRootrickObstacleField *field, const uint8_t *scene, size_t bytes);
void bfm_rootrick_obstacles_tick(BfmRootrickObstacleField *field, int story158);

/* Called once on the challenge entrance and when the race ends/releases its hold. */
void bfm_rootrick_obstacles_challenge(BfmRootrickObstacleField *field);
void bfm_rootrick_obstacles_hold(BfmRootrickObstacleField *field, int hold);

/* Return the slot's live solid for the wall and rider queries. Slots follow
 * mounted room order, including while the solids retract behind the wall. */
int bfm_rootrick_obstacle_body(const BfmRootrickObstacleField *field, unsigned slot,
                               BfmForestGuardBox *shape, BfmForestTransform *root);
uint16_t bfm_rootrick_obstacle_flags(const BfmRootrickObstacleField *field, unsigned slot);
uint32_t bfm_rootrick_obstacle_model(const BfmRootrickObstacleField *field, unsigned slot);

/* Check the proposed rival model's four directed edges per enabled X/Y/Z axis
 * (bits 1, 2 and 4). Recheck the previous model's Z edges against the same first
 * obstacle. Returns -1 for invalid input, 0 for clear, 1 for rollback or 2 for
 * rollback with a stumble. The caller owns the rollback and pose update. */
int bfm_rootrick_obstacle_crossing(const BfmRootrickObstacleField *field,
                                   const BfmForestTransform *proposed, const BfmForestTransform *previous,
                                   unsigned axis_mask);

#endif
