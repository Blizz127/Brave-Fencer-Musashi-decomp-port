/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_TILES_H
#define BFM_WOODS_EARTH_SHRINE_TILES_H

#include "musashi_disc_media.h"

enum {
    BFM_WOODS_EARTH_RELEASE_TILES = 59,
    BFM_WOODS_EARTH_AWARD_TILES = 60,
    BFM_WOODS_EARTH_RESTORE_TILES = 61
};

/* The room owns current_member alongside its VRAM and clears it when that
 * VRAM is replaced. A repeated request succeeds without reading the disc.
 * A failed request preserves both the live tiles and current_member. */
int bfm_woods_earth_shrine_tiles(const MusashiDiscMedia *disc, unsigned member,
    uint16_t vram[512][1024], unsigned *current_member);

#endif
