/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_TRIGGER_H
#define BFM_WOODS_ROOTRICK_TRIGGER_H

#include <stddef.h>
#include <stdint.h>

struct BfmGameplayRun;

typedef struct {
    int16_t actor_xyz[3];
    uint32_t opening_address; /* Mounted program selected after encounter staging. */
    enum { BFM_ROOTRICK_ENTERING, BFM_ROOTRICK_WAITING, BFM_ROOTRICK_INACTIVE,
           BFM_ROOTRICK_STARTED } state;
} BfmWoodsRootrickTrigger;

/* Locate Rootrick in 3026's static actor list. A fresh visit gets a fresh
 * actor. A failed mount leaves the caller's prior state untouched. */
int bfm_woods_rootrick_trigger_load(BfmWoodsRootrickTrigger *out, const uint8_t *scene, size_t bytes);

/* Run once per active field update after player motion. The first update
 * admits or removes the actor using the current story banks. On subsequent
 * updates, return 1 once when the bend starts the encounter's staging. */
int bfm_woods_rootrick_trigger_step(BfmWoodsRootrickTrigger *trigger, const struct BfmGameplayRun *run,
                                    const int16_t feet[3]);

#endif
