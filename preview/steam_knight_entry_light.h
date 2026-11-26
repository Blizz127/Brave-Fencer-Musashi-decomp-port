/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_ENTRY_LIGHT_H
#define BFM_STEAM_KNIGHT_ENTRY_LIGHT_H
#include "forest_dust.h"

enum { BFM_STEAM_ENTRY_LIGHT_CARDS = 16 * 16 };

/* Screened by the field depth sorter along with Musashi and the doorway. */
size_t bfm_steam_entry_light(const BfmForestCamera *camera, unsigned age,
                             BfmForestDustCard cards[BFM_STEAM_ENTRY_LIGHT_CARDS]);
#endif
