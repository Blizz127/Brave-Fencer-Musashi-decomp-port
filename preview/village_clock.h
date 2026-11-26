/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_CLOCK_H
#define BFM_VILLAGE_CLOCK_H

#include "bedroom_record.h"

/* One 30 Hz outdoor player update. The caller owns scene, interaction, and
 * transition pauses; accelerated selects the player's faster clock mode. */
void bfm_village_clock_step(BfmSaveRecord *state, int accelerated);

#endif
