/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_AUDIO_H
#define BFM_VILLAGE_MINE_LAKE_AUDIO_H

#include "bedroom_effects.h"
#include <stdint.h>

enum { BFM_MINE_LAKE_SHOAL = 0x6fe, BFM_MINE_LAKE_CURRENT = 0x738,
       BFM_MINE_LAKE_ARGUMENT = 0x4000,
       BFM_MINE_LAKE_FALL_SOUND = 0x453, BFM_MINE_LAKE_BOUNCE_SOUND = 0x454,
       BFM_MINE_LAKE_TUMBLE_SOUND = 0x456 };

BfmBedroomEffects *bfm_mine_lake_effects_load(const MusashiDiscMedia *disc);

static inline int bfm_mine_lake_surface_cue(unsigned cue)
{
    return (cue >= 0x406 && cue <= 0x409) || cue == 0x41e || cue == 0x42f;
}

static inline int bfm_mine_lake_defeat_cue(unsigned cue)
{
    return cue == BFM_MINE_LAKE_FALL_SOUND || cue == BFM_MINE_LAKE_BOUNCE_SOUND ||
           cue == BFM_MINE_LAKE_TUMBLE_SOUND;
}

/* The current keeps a single sustained note. Each shoal request carries the
 * full 16-bit value, including the wrapped underwater pitch argument. */
static inline unsigned bfm_mine_lake_sound_event(unsigned cue, unsigned value)
{
    if (cue == 4 && value == BFM_MINE_LAKE_CURRENT)
        return 0x10000u | BFM_MINE_LAKE_CURRENT;
    if (bfm_mine_lake_surface_cue(cue) || bfm_mine_lake_defeat_cue(cue))
        return value ? 0 : cue;
    if (cue == BFM_MINE_LAKE_CURRENT) {
        if ((value & ~255u) != 0x1000u) return 0;
    } else if (cue != BFM_MINE_LAKE_SHOAL || value > UINT16_MAX || !(value & 0x1000u)) {
        return 0;
    }
    return value << 16 | BFM_MINE_LAKE_ARGUMENT | cue;
}
#endif
