/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_HINGE_DEBRIS_H
#define BFM_VALLEY_HINGE_DEBRIS_H

#include "gameplay_run.h"
#include "valley_debris_art.h"

/* Five puffs from each of the hinge's three rebounds can briefly overlap. */
enum { BFM_VALLEY_HINGE_PUFFS = 5, BFM_VALLEY_HINGE_CARDS = 3 * BFM_VALLEY_HINGE_PUFFS };

typedef struct {
    int16_t xyz[3];
    uint8_t cel, remaining;
} BfmValleyHingePuff;

typedef struct {
    BfmValleyDebrisArt art;
    BfmValleyHingePuff puffs[BFM_VALLEY_HINGE_CARDS];
} BfmValleyHingeDebris;

/* Advance once per active field. An impact supplies the strike point from
 * the last completed hinge transform and consumes the shared gameplay RNG. */
void bfm_valley_hinge_debris_step(BfmValleyHingeDebris *debris, BfmGameplayRun *run,
                                  const int16_t impact[3]);
unsigned bfm_valley_hinge_debris_project(const BfmValleyHingeDebris *debris,
    const BfmForestCamera *camera, BfmForestDustCard cards[BFM_VALLEY_HINGE_CARDS]);

#endif
