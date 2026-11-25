/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_STATUS_H
#define BFM_FOREST_STATUS_H

#include "musashi_disc_media.h"

typedef struct {
    /* Mutable resident player record belonging to this New Game. */
    uint8_t record[152];
} BfmForestStatus;

typedef struct {
    uint32_t wallet;
    uint16_t max_hp, max_bp, day, minutes_of_day;
    int16_t hp, bp;
    uint8_t weekday, ability, ability_label;
    uint8_t charge, orb;
    uint16_t face_gauge;
} BfmForestStatusValues;

/* Read the resident New Game defaults from the mounted USA executable.
 * Initialize once for a new game, before the first forest HUD update. */
int bfm_forest_status_load(const MusashiDiscMedia *disc, BfmForestStatus *out);
/* Read the current values of a successfully initialized status record.
 * The wallet uses stored units. The HUD supplies its trailing zero. */
BfmForestStatusValues bfm_forest_status_values(const BfmForestStatus *status);

#endif
