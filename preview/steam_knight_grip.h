/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_GRIP_H
#define BFM_STEAM_KNIGHT_GRIP_H

#include "forest_guard_contact.h"

typedef struct {
    BfmForestGuardBox body, pickup;
    int16_t grip[3];
} BfmSteamKnightGrip;

int bfm_steam_knight_grip_load(BfmSteamKnightGrip *out, const uint8_t *scene, size_t bytes);

/* Project grounded movement along the selected machine surface. A miss leaves
 * corrected untouched. The caller retains floor height and sub-unit motion. */
int bfm_steam_knight_grip_brace(const BfmForestGuardBox *surface,
                                const BfmForestTransform *machine,
                                const int16_t previous[3], const int16_t proposed[3],
                                int16_t corrected[3]);

/* Query the selected surface with the player's facing at the current feet. */
int bfm_steam_knight_grip_reaches(const BfmForestGuardBox *surface,
                                  const BfmForestTransform *machine,
                                  const BfmForestTransform *hero, const int16_t feet[3]);

#endif
