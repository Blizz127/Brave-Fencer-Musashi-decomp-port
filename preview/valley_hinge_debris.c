/* SPDX-License-Identifier: MIT */
#include "valley_hinge_debris.h"

/* Each offset consumes separate magnitude and direction draws. */
static int scatter(BfmGameplayRun *run, unsigned range)
{
    int distance = bfm_gameplay_random(run) % range;
    return (bfm_gameplay_random(run) & 1) ? distance : -distance;
}

void bfm_valley_hinge_debris_step(BfmValleyHingeDebris *debris, BfmGameplayRun *run,
                                  const int16_t impact[3])
{
    for (unsigned i = 0; i < BFM_VALLEY_HINGE_CARDS; ++i) {
        BfmValleyHingePuff *puff = &debris->puffs[i];
        if (!puff->remaining || --puff->remaining) continue;
        ++puff->cel;
        if (puff->cel < BFM_VALLEY_DEBRIS_CELS)
            puff->remaining = debris->art.duration[puff->cel];
    }
    if (!impact || !run) return;

    for (unsigned n = 0; n < BFM_VALLEY_HINGE_PUFFS; ++n) {
        int x = impact[0] + scatter(run, 96);
        int z = impact[2] + scatter(run, 5);
        for (unsigned i = 0; i < BFM_VALLEY_HINGE_CARDS; ++i) {
            if (!debris->puffs[i].remaining) {
                debris->puffs[i] = (BfmValleyHingePuff){
                    .xyz = {(int16_t)x, impact[1], (int16_t)z},
                    .remaining = debris->art.duration[0]
                };
                break;
            }
        }
    }
}

unsigned bfm_valley_hinge_debris_project(const BfmValleyHingeDebris *debris,
    const BfmForestCamera *camera, BfmForestDustCard cards[BFM_VALLEY_HINGE_CARDS])
{
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_VALLEY_HINGE_CARDS; ++i) {
        const BfmValleyHingePuff *puff = &debris->puffs[i];
        if (puff->remaining)
            count += bfm_valley_debris_art_project(&debris->art, puff->cel, puff->xyz,
                                                   camera, i, &cards[count]);
    }
    return count;
}
