/* SPDX-License-Identifier: MIT */
#ifndef BFM_NIGHTMARE_WAKE_H
#define BFM_NIGHTMARE_WAKE_H

#include "bedroom_field.h"
#include "forest_dust.h"

typedef struct {
    int16_t position[3], left, top;
    uint16_t page;
    uint8_t u, v, width, height, remaining;
} BfmNightmareWake;

/* Start on the player update that spawns the flash. It stays at the initial
 * world position as the player recovers. Step once per subsequent update. */
int bfm_nightmare_wake_start(BfmNightmareWake *effect, const uint8_t *scene,
                             size_t bytes, uint32_t address, const int16_t player[3]);
void bfm_nightmare_wake_step(BfmNightmareWake *effect);

/* Return one card to append to the field's figure queue when visible. */
int bfm_nightmare_wake_card(const BfmNightmareWake *effect, const BfmBedroomView *view,
                            BfmForestDustCard *out);
#endif
