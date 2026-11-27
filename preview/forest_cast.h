/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_CAST_H
#define BFM_FOREST_CAST_H

#include "forest_status.h"

enum { BFM_CAST_MAX_KEYS = 32 };
typedef struct {
    uint32_t pose, visible;
    int launch;
} BfmForestCastKey;
typedef struct {
    BfmForestCastKey keys[BFM_CAST_MAX_KEYS];
    unsigned count, hold_after;
} BfmForestCastClip;
typedef enum {
    BFM_CAST_GROUND, BFM_CAST_STANCE, BFM_CAST_THROW, BFM_CAST_LAUNCH, BFM_CAST_SWING
} BfmForestCastEvent;
typedef struct {
    BfmForestCastClip stance, strafe, throw_clip, award;
    BfmForestCastEvent action;
    unsigned moving, key;
    /* Rooms with the persistent charge upgrade supply 3 or 6 per active
     * stance update, before the current input is dispatched. */
    unsigned active_stance_gain;
    uint32_t pose, visible;
} BfmForestCast;

int bfm_forest_cast_load(BfmForestCast *cast, const MusashiDiscMedia *disc,
                         uint32_t animation_table);
/* The ground action dispatcher supplies the normal held shoulder and fresh
 * Square edge once per simulation update. An undercharged Square requests
 * the existing ordinary swing. LAUNCH is the throw clip's blade-release event.
 * The independent flight owner can remain active after this action finishes.
 * The caller applies movement, supplies its current visibility on stance
 * entry, and uses the returned pose and part visibility. */
BfmForestCastEvent bfm_forest_cast_tick(BfmForestCast *cast, BfmForestStatus *status,
    int r1_held, int square_pressed, int moving, int blade_away);
#endif
