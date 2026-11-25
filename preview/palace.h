/* SPDX-License-Identifier: MIT */
#ifndef BFM_OPENING_PALACE_H
#define BFM_OPENING_PALACE_H
#include "musashi_disc_media.h"
#include <stdint.h>
typedef struct Palace Palace;
Palace *bfm_palace_load(const MusashiDiscMedia *disc, unsigned member);
void bfm_palace_draw(Palace *palace, const uint16_t tiles[512][1024],
                     unsigned field, uint32_t *pixels);
void bfm_palace_free(Palace *palace);
#endif
