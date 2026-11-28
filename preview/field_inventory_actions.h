/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_INVENTORY_ACTIONS_H
#define BFM_FIELD_INVENTORY_ACTIONS_H

#include "gameplay_run.h"
#include <stddef.h>

/* Slots are numbered 0..11 in the Items grid. main is the resident
 * data payload returned by bfm_bedroom_player_data. */
int bfm_field_inventory_use(const uint8_t *main, size_t main_bytes, unsigned slot,
                            BfmSaveRecord *save, BfmGameplayRun *run);
int bfm_field_inventory_discard(const uint8_t *main, size_t main_bytes, unsigned slot,
                                BfmSaveRecord *save, BfmGameplayRun *run);
int bfm_field_inventory_swap(unsigned a, unsigned b, BfmSaveRecord *save, BfmGameplayRun *run);
/* First menu cue for a Use, selected before consumption. */
unsigned bfm_field_inventory_use_sound(unsigned item);

#endif
